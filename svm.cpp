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

    // Global storage for cloaked pages (Sync across cores)
    typedef struct _CLOAKED_PAGE {
        UINT64 GuestPa;
        UINT64 OriginalPa;
        UINT64 ShadowPa;
    } CLOAKED_PAGE, *PCLOAKED_PAGE;

    CLOAKED_PAGE g_CloakedPages[64];
    volatile LONG g_CloakedCount = 0;

    /**
     * Traverses AMD NPT tables to find the entry for a physical address.
     */
    UINT64* GetNptEntry(UINT64 NptPml4Pa, UINT64 FaultPa) {
        UINT64 Pml4Idx = (FaultPa >> 39) & 0x1FF;
        UINT64 PdptIdx = (FaultPa >> 30) & 0x1FF;
        UINT64 PdIdx   = (FaultPa >> 21) & 0x1FF;
        UINT64 PteIdx  = (FaultPa >> 12) & 0x1FF;

        UINT64* Pml4 = (UINT64*)(NptPml4Pa & ~0xFFF);
        if (!(Pml4[Pml4Idx] & 1)) return nullptr;

        UINT64* Pdpt = (UINT64*)(Pml4[Pml4Idx] & ~0xFFF);
        if (!(Pdpt[PdptIdx] & 1)) return nullptr;

        UINT64* Pd = (UINT64*)(Pdpt[PdptIdx] & ~0xFFF);
        if (!(Pd[PdIdx] & 1)) return nullptr;

        // Skip Huge Pages for cloaking (assume 4KB for simplicity)
        if (Pd[PdIdx] & 0x80) return nullptr;

        UINT64* Pt = (UINT64*)(Pd[PdIdx] & ~0xFFF);
        return &Pt[PteIdx];
    }

    /**
     * Finds a cave of null-filled memory in the guest process (Code Cave).
     * Uses MmMapIoSpace for safe physical memory access.
     */
    UINT64 FindGuestCodeCave(UINT64 Cr3, UINT64 StartVa, SIZE_T Size) {
        for (UINT64 Current = StartVa; Current < StartVa + 0x10000000; Current += 0x1000) {
            UINT64 Pa = TranslateVa(Cr3, Current);
            if (!Pa) continue;

            PHYSICAL_ADDRESS Phys; Phys.QuadPart = Pa;
            PVOID Mapped = MmMapIoSpace(Phys, Size, MmNonCached);
            if (!Mapped) continue;

            BOOLEAN IsCave = TRUE;
            UINT8* Buffer = (UINT8*)Mapped;
            for (SIZE_T i = 0; i < Size; i++) {
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

        UINT64* Pml4 = (UINT64*)(Cr3 & ~0xFFF);
        if (!(Pml4[Pml4Idx] & 1)) return 0;
        UINT64* Pdpt = (UINT64*)(Pml4[Pml4Idx] & ~0xFFF);
        if (!(Pdpt[PdptIdx] & 1)) return 0;
        UINT64* Pd = (UINT64*)(Pdpt[PdptIdx] & ~0xFFF);
        if (!(Pd[PdIdx] & 1)) return 0;
        if (Pd[PdIdx] & 0x80) return (Pd[PdIdx] & ~0x1FFFFF) + (Va & 0x1FFFFF);
        UINT64* Pt = (UINT64*)(Pd[PdIdx] & ~0xFFF);
        if (!(Pt[PteIdx] & 1)) return 0;
        return (Pt[PteIdx] & ~0xFFF) + (Va & 0xFFF);
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
                UINT64* Entry = GetNptEntry(NptRoot, FaultPa);
                if (!Entry) return;

                if (IsExec) {
                    // CPU execution -> Point to Shadow (Infected)
                    *Entry = (g_CloakedPages[i].ShadowPa & ~0xFFF) | (*Entry & 0xFFF);
                    *Entry &= ~(1ULL << 63); // Clear NX
                } else {
                    // Memory scan/read -> Point to Original (Clean)
                    *Entry = (g_CloakedPages[i].OriginalPa & ~0xFFF) | (*Entry & 0xFFF);
                    *Entry |= (1ULL << 63); // Set NX to catch next execution
                }

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
        UINT8* Vmcb = (UINT8*)VmcbPa;
        UINT64 ExitCode = *(UINT64*)(Vmcb + 0x70);
        PVMM_PROCESSOR_CONTEXT ctx = (PVMM_PROCESSOR_CONTEXT)__readgsqword(0);

        if (_InterlockedExchange((volatile long*)&ctx->HasLaunched, 1) == 1) return STATUS_SUCCESS;

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
                // By subtracting the elapsed host cycles from the TSC_OFFSET,
                // the guest's RDTSC remains consistent.
                UINT64 Latency = __rdtsc() - StartTsc;
                *(UINT64*)(Vmcb + 0x38) -= (Latency + 100); // 100 is an estimated fixed overhead for VM-Exit/Entry
                break;
            }

            case 0x81: { // VMEXIT_VMMCALL
                if (Registers->Rcx == HV_SECRET_KEY) {
                    using namespace Cheat::Hv;
                    Command Cmd = (Command)Registers->Rdx;
                    uint64_t GuestCr3 = *(uint64_t*)(Vmcb + 0x400 + 0x140);
                    void* ArgsPa = (void*)TranslateVa(GuestCr3, Registers->R8);

                    if (ArgsPa) {
                        switch (Cmd) {
                            case Command::ReadVirtual: {
                                ReadWriteArgs* A = (ReadWriteArgs*)ArgsPa;
                                UINT64 SrcPa = TranslateVa(A->cr3, A->addr);
                                UINT64 DstPa = TranslateVa(GuestCr3, (uintptr_t)A->buffer);

                                if (SrcPa && DstPa) {
                                    PHYSICAL_ADDRESS P1; P1.QuadPart = SrcPa;
                                    PHYSICAL_ADDRESS P2; P2.QuadPart = DstPa;
                                    PVOID V1 = MmMapIoSpace(P1, A->size, MmNonCached);
                                    PVOID V2 = MmMapIoSpace(P2, A->size, MmNonCached);

                                    if (V1 && V2) {
                                        memcpy(V2, V1, A->size);
                                    }

                                    if (V1) MmUnmapIoSpace(V1, A->size);
                                    if (V2) MmUnmapIoSpace(V2, A->size);
                                }
                                Registers->Rax = 0;
                                break;
                            }
                            case Command::AllocateVirtual: {
                                AllocArgs* A = (AllocArgs*)ArgsPa;
                                // Search for a 4KB code cave starting from the base of the game
                                A->out_addr = FindGuestCodeCave(GuestCr3, 0x140000000, 0x1000);
                                Registers->Rax = (A->out_addr != 0) ? 0 : 1;
                                break;
                            }
                            case Command::CloakPage: {
                                CloakArgs* A = (CloakArgs*)ArgsPa;
                                if (g_CloakedCount < 64) {
                                    g_CloakedPages[g_CloakedCount].GuestPa = A->guest_va & ~0xFFF;
                                    g_CloakedPages[g_CloakedCount].OriginalPa = A->guest_va & ~0xFFF;
                                    g_CloakedPages[g_CloakedCount].ShadowPa = TranslateVa(GuestCr3, (uintptr_t)A->shadow_buffer);
                                    g_CloakedCount++;
                                    Registers->Rax = 0;
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

        _InterlockedExchange((volatile long*)&ctx->HasLaunched, 0);
        return STATUS_SUCCESS;
    }
}
