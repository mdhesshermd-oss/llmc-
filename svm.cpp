#include "vmm.h"
#include "svm.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <string.h>

/**
 * Advanced AMD SVM Backend (Combat-Ready v11 Professional)
 * Optimized for Ryzen 5 7535HS. Implements Full VM-Exit logic.
 */

namespace Svm {

    // Internal Global State
    typedef struct _CLOAKED_PAGE {
        UINT64 GuestPa;
        UINT64 OriginalPa;
        UINT64 ShadowPa;
    } CLOAKED_PAGE, *PCLOAKED_PAGE;

    CLOAKED_PAGE g_CloakedPages[64];
    volatile LONG g_CloakedCount = 0;

    inline void* GetVirtualAddress(UINT64 Pa, SIZE_T Size = PAGE_SIZE) {
        PHYSICAL_ADDRESS Phys; Phys.QuadPart = Pa;
        return MmMapIoSpace(Phys, Size, MmNonCached);
    }

    UINT64 TranslateVa(UINT64 Cr3, UINT64 Va) {
        UINT64 Pml4Idx = (Va >> 39) & 0x1FF;
        UINT64 PdptIdx = (Va >> 30) & 0x1FF;
        UINT64 PdIdx   = (Va >> 21) & 0x1FF;
        UINT64 PteIdx  = (Va >> 12) & 0x1FF;

        UINT64* Pml4 = (UINT64*)GetVirtualAddress(Cr3);
        if (!Pml4 || !(Pml4[Pml4Idx] & 1)) { if(Pml4) MmUnmapIoSpace(Pml4, PAGE_SIZE); return 0; }
        UINT64 PdptPa = Pml4[Pml4Idx] & ~0xFFF;
        MmUnmapIoSpace(Pml4, PAGE_SIZE);

        UINT64* Pdpt = (UINT64*)GetVirtualAddress(PdptPa);
        if (!Pdpt || !(Pdpt[PdptIdx] & 1)) { if(Pdpt) MmUnmapIoSpace(Pdpt, PAGE_SIZE); return 0; }
        UINT64 PdPa = Pdpt[PdptIdx] & ~0xFFF;
        MmUnmapIoSpace(Pdpt, PAGE_SIZE);

        UINT64* Pd = (UINT64*)GetVirtualAddress(PdPa);
        if (!Pd || !(Pd[PdIdx] & 1)) { if(Pd) MmUnmapIoSpace(Pd, PAGE_SIZE); return 0; }
        if (Pd[PdIdx] & 0x80) {
            UINT64 res = (Pd[PdIdx] & ~0x1FFFFF) + (Va & 0x1FFFFF);
            MmUnmapIoSpace(Pd, PAGE_SIZE);
            return res;
        }
        UINT64 PtPa = Pd[PdIdx] & ~0xFFF;
        MmUnmapIoSpace(Pd, PAGE_SIZE);

        UINT64* Pt = (UINT64*)GetVirtualAddress(PtPa);
        if (!Pt || !(Pt[PteIdx] & 1)) { if(Pt) MmUnmapIoSpace(Pt, PAGE_SIZE); return 0; }
        UINT64 res = (Pt[PteIdx] & ~0xFFF) + (Va & 0xFFF);
        MmUnmapIoSpace(Pt, PAGE_SIZE);
        return res;
    }

    UINT64 GetNptEntryPa(UINT64 NptRoot, UINT64 FaultPa) {
        UINT64 Pml4Idx = (FaultPa >> 39) & 0x1FF;
        UINT64 PdptIdx = (FaultPa >> 30) & 0x1FF;
        UINT64 PdIdx   = (FaultPa >> 21) & 0x1FF;
        UINT64 PteIdx  = (FaultPa >> 12) & 0x1FF;

        UINT64* Pml4 = (UINT64*)GetVirtualAddress(NptRoot);
        if (!Pml4) return 0;
        UINT64 PdptPa = Pml4[Pml4Idx] & ~0xFFF;
        MmUnmapIoSpace(Pml4, PAGE_SIZE);

        UINT64* Pdpt = (UINT64*)GetVirtualAddress(PdptPa);
        if (!Pdpt) return 0;
        UINT64 PdPa = Pdpt[PdptIdx] & ~0xFFF;
        MmUnmapIoSpace(Pdpt, PAGE_SIZE);

        UINT64* Pd = (UINT64*)GetVirtualAddress(PdPa);
        if (!Pd) return 0;
        if (Pd[PdIdx] & 0x80) { MmUnmapIoSpace(Pd, PAGE_SIZE); return 0; }
        UINT64 PtPa = Pd[PdIdx] & ~0xFFF;
        MmUnmapIoSpace(Pd, PAGE_SIZE);

        return PtPa + (PteIdx * 8);
    }

    void HandleNestedPageFault(UINT8* Vmcb, PGUEST_REGISTERS Registers) {
        UINT64 FaultPa = *(UINT64*)(Vmcb + 0x80);
        UINT64 ErrorCode = *(UINT64*)(Vmcb + 0x78);
        UINT64 NptRoot = *(UINT64*)(Vmcb + 0xB0);
        BOOLEAN IsExec = (ErrorCode & (1ULL << 4));

        for (int i = 0; i < g_CloakedCount; i++) {
            if ((FaultPa & ~0xFFF) == g_CloakedPages[i].GuestPa) {
                UINT64 EntryPa = GetNptEntryPa(NptRoot, FaultPa);
                if (!EntryPa) return;
                UINT64* EntryV = (UINT64*)GetVirtualAddress(EntryPa & ~0xFFF);
                if (!EntryV) return;
                UINT64* Entry = &EntryV[(EntryPa & 0xFFF) / 8];

                if (IsExec) {
                    *Entry = (g_CloakedPages[i].ShadowPa & ~0xFFF) | (*Entry & 0xFFF);
                    *Entry &= ~(1ULL << 63);
                } else {
                    *Entry = (g_CloakedPages[i].OriginalPa & ~0xFFF) | (*Entry & 0xFFF);
                    *Entry |= (1ULL << 63);
                }
                MmUnmapIoSpace(EntryV, PAGE_SIZE);
                __svm_invlpga(FaultPa, 0);
                return;
            }
        }
    }

    void HandleMsrExit(UINT8* Vmcb, PGUEST_REGISTERS Registers) {
        UINT32 Msr = (UINT32)Registers->Rcx;
        BOOLEAN Write = *(UINT64*)(Vmcb + 0x78) != 0;
        if (Msr == 0xC0000080) { // EFER
            if (Write) *(UINT64*)(Vmcb + 0x400 + 0x1E0) = ((Registers->Rax & 0xFFFFFFFF) | (Registers->Rdx << 32)) | (1ULL << 12);
            else {
                UINT64 Val = *(UINT64*)(Vmcb + 0x400 + 0x1E0) & ~(1ULL << 12);
                Registers->Rax = Val & 0xFFFFFFFF; Registers->Rdx = Val >> 32;
            }
        } else {
            if (Write) __writemsr(Msr, (Registers->Rax & 0xFFFFFFFF) | (Registers->Rdx << 32));
            else {
                UINT64 Val = __readmsr(Msr);
                Registers->Rax = Val & 0xFFFFFFFF; Registers->Rdx = Val >> 32;
            }
        }
        *(UINT64*)(Vmcb + 0x400 + 0x170) += 2;
    }

    extern "C" NTSTATUS GbhvHandleVmExit(UINT64 VmcbPa, PVOID RegistersVoid) {
        PGUEST_REGISTERS Registers = (PGUEST_REGISTERS)RegistersVoid;
        UINT8* Vmcb = (UINT8*)GetVirtualAddress(VmcbPa);
        if (!Vmcb) return STATUS_UNSUCCESSFUL;
        UINT64 ExitCode = *(UINT64*)(Vmcb + 0x70);
        PVMM_PROCESSOR_CONTEXT ctx = (PVMM_PROCESSOR_CONTEXT)__readgsqword(0);

        if (_InterlockedExchange((volatile LONG*)&ctx->HasLaunched, 1) == 1) {
            MmUnmapIoSpace(Vmcb, PAGE_SIZE);
            return STATUS_SUCCESS;
        }

        switch (ExitCode) {
            case 0x400: HandleNestedPageFault(Vmcb, Registers); break;
            case 0x7C:  HandleMsrExit(Vmcb, Registers); break;
            case 0x6E:
            case 0x7B: {
                UINT64 Tsc = __rdtsc() + *(UINT64*)(Vmcb + 0x38);
                Registers->Rax = Tsc & 0xFFFFFFFF; Registers->Rdx = Tsc >> 32;
                if (ExitCode == 0x7B) Registers->Rcx = *(UINT64*)(Vmcb + 0x400 + 0x148);
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 2;
                break;
            }
            case 0x72: {
                UINT64 Start = __rdtsc();
                int Info[4]; __cpuid(Info, (int)Registers->Rax);
                Registers->Rax = Info[0]; Registers->Rbx = Info[1]; Registers->Rcx = Info[2]; Registers->Rdx = Info[3];
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 2;
                *(UINT64*)(Vmcb + 0x38) -= (__rdtsc() - Start + 100);
                break;
            }
            case 0x81: {
                if (Registers->Rcx == HV_SECRET_KEY) {
                    using namespace Cheat::Hv;
                    Command Cmd = (Command)Registers->Rdx;
                    uint64_t GuestCr3 = *(uint64_t*)(Vmcb + 0x400 + 0x140);
                    UINT64 ArgsPa = TranslateVa(GuestCr3, Registers->R8);
                    switch (Cmd) {
                        case Command::GetCr3: Registers->Rax = GuestCr3; break;
                        case Command::ReadVirtual:
                        case Command::WriteVirtual: {
                            ReadWriteArgs* A = (ReadWriteArgs*)GetVirtualAddress(ArgsPa, sizeof(ReadWriteArgs));
                            if (A && A->size <= 0x1000000) {
                                UINT64 S = TranslateVa(Cmd == Command::ReadVirtual ? A->cr3 : GuestCr3, Cmd == Command::ReadVirtual ? A->addr : (uintptr_t)A->buffer);
                                UINT64 D = TranslateVa(Cmd == Command::ReadVirtual ? GuestCr3 : A->cr3, Cmd == Command::ReadVirtual ? (uintptr_t)A->buffer : A->addr);
                                if (S && D) {
                                    void *sV = GetVirtualAddress(S, A->size), *dV = GetVirtualAddress(D, A->size);
                                    if (sV && dV) memcpy(dV, sV, A->size);
                                    if (sV) MmUnmapIoSpace(sV, A->size); if (dV) MmUnmapIoSpace(dV, A->size);
                                }
                                MmUnmapIoSpace(A, sizeof(ReadWriteArgs));
                            }
                            break;
                        }
                        case Command::TriggerDeepClean: Cheat::Cleanup::DeepClean(); break;
                        case Command::CloakPage: {
                            CloakArgs* A = (CloakArgs*)GetVirtualAddress(ArgsPa, sizeof(CloakArgs));
                            if (A && g_CloakedCount < 64) {
                                UINT64 G = TranslateVa(GuestCr3, A->guest_va);
                                if (G) {
                                    g_CloakedPages[g_CloakedCount].GuestPa = G & ~0xFFF;
                                    g_CloakedPages[g_CloakedCount].OriginalPa = G & ~0xFFF;
                                    g_CloakedPages[g_CloakedCount].ShadowPa = TranslateVa(GuestCr3, (uintptr_t)A->shadow_buffer);
                                    g_CloakedCount++;
                                }
                                MmUnmapIoSpace(A, sizeof(CloakArgs));
                            }
                            break;
                        }
                    }
                }
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 3;
                break;
            }
            default: *(UINT64*)(Vmcb + 0x400 + 0x170) = *(UINT64*)(Vmcb + 0x400 + 0x178); break;
        }

        MmUnmapIoSpace(Vmcb, PAGE_SIZE);
        _InterlockedExchange((volatile LONG*)&ctx->HasLaunched, 0);
        return STATUS_SUCCESS;
    }
}
