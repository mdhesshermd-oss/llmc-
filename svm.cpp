#include "vmm.h"
#include "svm.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <string.h>

/**
 * Advanced AMD SVM Backend (Combat-Ready v11 Professional)
 * Finalized: Implements NPF Shadow Pages, TSC Latency Compensation, and Dynamic Allocation.
 */

namespace Npt {
    UINT64 GbhvNptInitializeIdentity();
}

namespace Svm {

    /**
     * Safe physical to virtual mapping for hypervisor context.
     */
    inline void* GetVirtualAddress(UINT64 Pa, SIZE_T Size = PAGE_SIZE) {
        PHYSICAL_ADDRESS Phys; Phys.QuadPart = Pa;
        return MmMapIoSpace(Phys, Size, MmNonCached);
    }

    // Global storage for cloaked pages (Sync across cores)
    typedef struct _CLOAKED_PAGE {
        UINT64 GuestPa;
        UINT64 OriginalPa;
        UINT64 ShadowPa;
    } CLOAKED_PAGE, *PCLOAKED_PAGE;

    CLOAKED_PAGE g_CloakedPages[64];
    volatile LONG g_CloakedCount = 0;

    /**
     * Traverses AMD NPT tables to find the PHYSICAL address of the entry for a physical address.
     */
    UINT64 GetNptEntryPa(UINT64 NptPml4Pa, UINT64 FaultPa) {
        UINT64 Pml4Idx = (FaultPa >> 39) & 0x1FF;
        UINT64 PdptIdx = (FaultPa >> 30) & 0x1FF;
        UINT64 PdIdx   = (FaultPa >> 21) & 0x1FF;
        UINT64 PteIdx  = (FaultPa >> 12) & 0x1FF;

        UINT64* Pml4 = (UINT64*)GetVirtualAddress(NptPml4Pa);
        if (!Pml4 || !(Pml4[Pml4Idx] & 1)) { if(Pml4) MmUnmapIoSpace(Pml4, PAGE_SIZE); return 0; }
        UINT64 PdptPa = Pml4[Pml4Idx] & ~0xFFF;
        MmUnmapIoSpace(Pml4, PAGE_SIZE);

        UINT64* Pdpt = (UINT64*)GetVirtualAddress(PdptPa);
        if (!Pdpt || !(Pdpt[PdptIdx] & 1)) { if(Pdpt) MmUnmapIoSpace(Pdpt, PAGE_SIZE); return 0; }
        UINT64 PdPa = Pdpt[PdptIdx] & ~0xFFF;
        MmUnmapIoSpace(Pdpt, PAGE_SIZE);

        UINT64* Pd = (UINT64*)GetVirtualAddress(PdPa);
        if (!Pd || !(Pd[PdIdx] & 1)) { if(Pd) MmUnmapIoSpace(Pd, PAGE_SIZE); return 0; }
        if (Pd[PdIdx] & 0x80) { MmUnmapIoSpace(Pd, PAGE_SIZE); return 0; }
        UINT64 PtPa = Pd[PdIdx] & ~0xFFF;
        MmUnmapIoSpace(Pd, PAGE_SIZE);

        return PtPa + (PteIdx * 8);
    }

    /**
     * Finds a cave of null-filled memory in the guest process (Code Cave).
     * Uses safe mapping. Limited scan range to improve performance during VM-Exit.
     */
    UINT64 FindGuestCodeCave(UINT64 Cr3, UINT64 StartVa, SIZE_T Size) {
        // Reduced scan range for performance (1MB instead of 256MB)
        for (UINT64 Current = StartVa; Current < StartVa + 0x100000; Current += 0x1000) {
            UINT64 Pa = TranslateVa(Cr3, Current);
            if (!Pa) continue;

            PVOID Mapped = GetVirtualAddress(Pa, Size);
            if (!Mapped) continue;

            BOOLEAN IsCave = TRUE;
            UINT8* Buffer = (UINT8*)Mapped;
            // Check first 128 bytes only as a heuristic for efficiency
            for (SIZE_T i = 0; i < (Size < 128 ? Size : 128); i++) {
                if (Buffer[i] != 0x00 && Buffer[i] != 0xCC) {
                    IsCave = FALSE;
                    break;
                }
            }

            MmUnmapIoSpace(Mapped, Size);
            if (IsCave) return Current;
        }
        return 0;
    }

    /**
     * Translates Guest VA to PA.
     */
    UINT64 TranslateVa(UINT64 Cr3, UINT64 Va) {
        UINT64 Pml4Idx = (Va >> 39) & 0x1FF;
        UINT64 PdptIdx = (Va >> 30) & 0x1FF;
        UINT64 PdIdx   = (Va >> 21) & 0x1FF;
        UINT64 PteIdx  = (Va >> 12) & 0x1FF;

        UINT64* Pml4 = (UINT64*)GetVirtualAddress(Cr3);
        if (!Pml4 || !(Pml4[Pml4Idx] & 1)) { if(Pml4) MmUnmapIoSpace(Pml4, PAGE_SIZE); return 0; }

        UINT64* Pdpt = (UINT64*)GetVirtualAddress(Pml4[Pml4Idx] & ~0xFFF);
        MmUnmapIoSpace(Pml4, PAGE_SIZE);
        if (!Pdpt || !(Pdpt[PdptIdx] & 1)) { if(Pdpt) MmUnmapIoSpace(Pdpt, PAGE_SIZE); return 0; }

        UINT64* Pd = (UINT64*)GetVirtualAddress(Pdpt[PdptIdx] & ~0xFFF);
        MmUnmapIoSpace(Pdpt, PAGE_SIZE);
        if (!Pd || !(Pd[PdIdx] & 1)) { if(Pd) MmUnmapIoSpace(Pd, PAGE_SIZE); return 0; }

        if (Pd[PdIdx] & 0x80) {
            UINT64 res = (Pd[PdIdx] & ~0x1FFFFF) + (Va & 0x1FFFFF);
            MmUnmapIoSpace(Pd, PAGE_SIZE);
            return res;
        }

        UINT64* Pt = (UINT64*)GetVirtualAddress(Pd[PdIdx] & ~0xFFF);
        MmUnmapIoSpace(Pd, PAGE_SIZE);
        if (!Pt || !(Pt[PteIdx] & 1)) { if(Pt) MmUnmapIoSpace(Pt, PAGE_SIZE); return 0; }

        UINT64 res = (Pt[PteIdx] & ~0xFFF) + (Va & 0xFFF);
        MmUnmapIoSpace(Pt, PAGE_SIZE);
        return res;
    }

    /**
     * Handles Nested Page Faults (0x400) - The Shadow Pages Core.
     */
    void HandleNestedPageFault(UINT8* Vmcb, PGUEST_REGISTERS Registers) {
        UINT64 FaultPa = *(UINT64*)(Vmcb + 0x80);
        UINT64 ErrorCode = *(UINT64*)(Vmcb + 0x78);
        UINT64 NptRoot = *(UINT64*)(Vmcb + 0xB0);

        // ID bit (Bit 4) indicates instruction fetch
        BOOLEAN IsExec = (ErrorCode & (1ULL << 4));

        for (int i = 0; i < g_CloakedCount; i++) {
            if ((FaultPa & ~0xFFF) == g_CloakedPages[i].GuestPa) {
                UINT64 EntryPa = GetNptEntryPa(NptRoot, FaultPa);
                if (!EntryPa) return;

                UINT64* PageBase = (UINT64*)GetVirtualAddress(EntryPa & ~0xFFF);
                if (!PageBase) return;

                UINT64* Entry = &PageBase[(EntryPa & 0xFFF) / 8];

                if (IsExec) {
                    // CPU execution -> Point to Shadow (Infected)
                    *Entry = (g_CloakedPages[i].ShadowPa & ~0xFFF) | (*Entry & 0xFFF);
                    *Entry &= ~(1ULL << 63); // Clear NX
                } else {
                    // Memory scan/read -> Point to Original (Clean)
                    *Entry = (g_CloakedPages[i].OriginalPa & ~0xFFF) | (*Entry & 0xFFF);
                    *Entry |= (1ULL << 63); // Set NX to catch next execution
                }

                MmUnmapIoSpace(PageBase, PAGE_SIZE);

                // Invalidate TLB for this address
                __svm_invlpga(FaultPa, 0);
                return;
            }
        }
    }

    /**
     * Minimal VMCB Setup for Guest-Mode transition.
     */
    void SetupVmcb(UINT8* Vmcb, PVMM_PROCESSOR_CONTEXT ctx) {
        // Control Area (Offsets relative to VMCB Base)
        *(UINT32*)(Vmcb + 0x00) = (1 << 18); // Intercept VMMCALL
        *(UINT32*)(Vmcb + 0x04) = (1 << 14); // Intercept CPUID
        *(UINT32*)(Vmcb + 0x08) = (1 << 14) | (1 << 27); // Intercept RDTSC, RDTSCP
        *(UINT32*)(Vmcb + 0x0C) = (1 << 16); // Intercept VMRUN (Nested protection)

        *(UINT64*)(Vmcb + 0xB0) = ctx->NptRootPhysical; // NP_CR3
        *(UINT64*)(Vmcb + 0xB8) = 1ULL; // Enable Nested Paging

        *(UINT32*)(Vmcb + 0x18) = 0x1; // ASID (Address Space Identifier)

        // Guest State (Offsets start at 0x400)
        UINT8* Guest = Vmcb + 0x400;
        *(UINT64*)(Guest + 0x140) = __readcr3(); // Guest CR3
        *(UINT64*)(Guest + 0x148) = __readcr4();
        *(UINT64*)(Guest + 0x150) = __readcr0();
        *(UINT64*)(Guest + 0x170) = ctx->GuestRip;
        *(UINT64*)(Guest + 0x178) = ctx->GuestRflags;
        *(UINT64*)(Guest + 0x1D8) = __readcr2();
        *(UINT64*)(Guest + 0x1D0) = ctx->GuestRsp;

        // EFER.SVME must be set in Guest state
        *(UINT64*)(Guest + 0x1E0) = __readmsr(0xC0000080) | (1ULL << 12);

        // CS Segment (Basic Flat 64-bit)
        *(UINT16*)(Guest + 0x00) = 0x10; // Selector
        *(UINT16*)(Guest + 0x02) = 0x209B; // Attribs
        *(UINT64*)(Guest + 0x08) = 0xFFFFFFFFFFFFFFFF; // Limit
    }

    extern "C" void SvmVmExitHandler();

    extern "C" BOOLEAN GbhvSvmInitialize(PVMM_PROCESSOR_CONTEXT ctx) {
        PHYSICAL_ADDRESS High; High.QuadPart = ~0ULL;

        // 1. Allocate VMCB (4KB)
        ctx->VmcbVirtual = MmAllocateContiguousMemory(PAGE_SIZE, High);
        if (!ctx->VmcbVirtual) return FALSE;
        ctx->VmcbPhysical = MmGetPhysicalAddress(ctx->VmcbVirtual).QuadPart;
        RtlZeroMemory(ctx->VmcbVirtual, PAGE_SIZE);

        // 2. Allocate Host Save Area (4KB)
        ctx->HostSaveVirtual = MmAllocateContiguousMemory(PAGE_SIZE, High);
        if (!ctx->HostSaveVirtual) return FALSE;
        ctx->HostSavePhysical = MmGetPhysicalAddress(ctx->HostSaveVirtual).QuadPart;
        RtlZeroMemory(ctx->HostSaveVirtual, PAGE_SIZE);

        // 3. Setup NPT for this core (Identity Mapping)
        ctx->NptRootPhysical = Npt::GbhvNptInitializeIdentity();
        if (!ctx->NptRootPhysical) return FALSE;

        // 4. Configure VMCB with Guest/Host state
        SetupVmcb((UINT8*)ctx->VmcbVirtual, ctx);

        // 5. Enable SVME bit in EFER
        __writemsr(0xC0000080, __readmsr(0xC0000080) | (1ULL << 12));

        // 6. Set HSAVE_PA MSR
        __writemsr(0xC0000101, ctx->HostSavePhysical);

        // 7. Launch into Guest Mode (Never returns unless error)
        GbhvSvmLaunch(ctx->VmcbPhysical, ctx->HostSavePhysical, ctx);

        return TRUE;
    }

    extern "C" NTSTATUS GbhvHandleVmExit(UINT64 VmcbPa, PVOID RegistersVoid) {
        PGUEST_REGISTERS Registers = (PGUEST_REGISTERS)RegistersVoid;
        UINT8* Vmcb = (UINT8*)GetVirtualAddress(VmcbPa);
        if (!Vmcb) return STATUS_UNSUCCESSFUL;

        UINT64 ExitCode = *(UINT64*)(Vmcb + 0x70);
        PVMM_PROCESSOR_CONTEXT ctx = (PVMM_PROCESSOR_CONTEXT)__readgsqword(0);

        if (_InterlockedExchange((volatile long*)&ctx->HasLaunched, 1) == 1) {
            MmUnmapIoSpace(Vmcb, PAGE_SIZE);
            return STATUS_SUCCESS;
        }

        switch (ExitCode) {
            case 0x400: // VMEXIT_NPF
                HandleNestedPageFault(Vmcb, Registers);
                break;

            case 0x6E:   // VMEXIT_RDTSC
            case 0x7B: { // VMEXIT_RDTSCP
                UINT64 Tsc = __rdtsc() + *(UINT64*)(Vmcb + 0x38);
                Registers->Rax = Tsc & 0xFFFFFFFF;
                Registers->Rdx = Tsc >> 32;
                if (ExitCode == 0x7B) Registers->Rcx = *(UINT64*)(Vmcb + 0x400 + 0x148); // TSC_AUX
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 2;
                break;
            }

            case 0x72: { // VMEXIT_CPUID
                UINT64 StartTsc = __rdtsc();
                int Info[4];
                __cpuid(Info, (int)Registers->Rax);
                Registers->Rax = Info[0]; Registers->Rbx = Info[1];
                Registers->Rcx = Info[2]; Registers->Rdx = Info[3];

                // Advance RIP
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 2;

                // Timing Protection: Hide hypervisor processing time
                UINT64 Latency = __rdtsc() - StartTsc;
                *(UINT64*)(Vmcb + 0x38) -= (Latency + 100);
                break;
            }

            case 0x81: { // VMEXIT_VMMCALL
                if (Registers->Rcx == HV_SECRET_KEY) {
                    using namespace Cheat::Hv;
                    Command Cmd = (Command)Registers->Rdx;
                    uint64_t GuestCr3 = *(uint64_t*)(Vmcb + 0x400 + 0x140);
                    UINT64 ArgsPa = TranslateVa(GuestCr3, Registers->R8);

                    if (ArgsPa || Cmd == Command::GetCr3) {
                        switch (Cmd) {
                            case Command::GetCr3: {
                                Registers->Rax = GuestCr3;
                                break;
                            }
                            case Command::ReadVirtual:
                            case Command::WriteVirtual: {
                                ReadWriteArgs* A = (ReadWriteArgs*)GetVirtualAddress(ArgsPa, sizeof(ReadWriteArgs));
                                if (A) {
                                    // Safety: Limit transfer size to 16MB
                                    if (A->size > 0x1000000) {
                                        Registers->Rax = 1;
                                        MmUnmapIoSpace(A, sizeof(ReadWriteArgs));
                                        break;
                                    }

                                    UINT64 SrcPa, DstPa;
                                    if (Cmd == Command::ReadVirtual) {
                                        SrcPa = TranslateVa(A->cr3, A->addr);
                                        DstPa = TranslateVa(GuestCr3, (uintptr_t)A->buffer);
                                    } else {
                                        SrcPa = TranslateVa(GuestCr3, (uintptr_t)A->buffer);
                                        DstPa = TranslateVa(A->cr3, A->addr);
                                    }

                                    if (SrcPa && DstPa) {
                                        PVOID V1 = GetVirtualAddress(SrcPa, A->size);
                                        PVOID V2 = GetVirtualAddress(DstPa, A->size);
                                        if (V1 && V2) memcpy(V2, V1, A->size);
                                        if (V1) MmUnmapIoSpace(V1, A->size);
                                        if (V2) MmUnmapIoSpace(V2, A->size);
                                    }
                                    MmUnmapIoSpace(A, sizeof(ReadWriteArgs));
                                }
                                Registers->Rax = 0;
                                break;
                            }
                            case Command::AllocateVirtual: {
                                AllocArgs* A = (AllocArgs*)GetVirtualAddress(ArgsPa, sizeof(AllocArgs));
                                if (A) {
                                    A->out_addr = FindGuestCodeCave(GuestCr3, 0x140000000, 0x1000);
                                    Registers->Rax = (A->out_addr != 0) ? 0 : 1;
                                    MmUnmapIoSpace(A, sizeof(AllocArgs));
                                }
                                break;
                            }
                            case Command::TriggerDeepClean: {
                                Cheat::Cleanup::DeepClean();
                                Registers->Rax = 0;
                                break;
                            }
                            case Command::CloakPage: {
                                CloakArgs* A = (CloakArgs*)GetVirtualAddress(ArgsPa, sizeof(CloakArgs));
                                if (!A) { Registers->Rax = 1; break; }

                                if (g_CloakedCount < 64) {
                                    UINT64 GuestPa = TranslateVa(GuestCr3, A->guest_va);
                                    if (GuestPa) {
                                        g_CloakedPages[g_CloakedCount].GuestPa = GuestPa & ~0xFFF;
                                        g_CloakedPages[g_CloakedCount].OriginalPa = GuestPa & ~0xFFF;
                                        g_CloakedPages[g_CloakedCount].ShadowPa = TranslateVa(GuestCr3, (uintptr_t)A->shadow_buffer);
                                        g_CloakedCount++;
                                        Registers->Rax = 0;
                                    } else {
                                        Registers->Rax = 1;
                                    }
                                    MmUnmapIoSpace(A, sizeof(CloakArgs));
                                }
                                break;
                            }
                        }
                    }
                }
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 3;
                break;
            }

            default: {
                *(UINT64*)(Vmcb + 0x400 + 0x170) = *(UINT64*)(Vmcb + 0x400 + 0x178);
                break;
            }
        }

        MmUnmapIoSpace(Vmcb, PAGE_SIZE);

        _InterlockedExchange((volatile long*)&ctx->HasLaunched, 0);
        return STATUS_SUCCESS;
    }
}
