#include "vmm.h"
#include "svm.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <string.h>

/**
 * Advanced AMD SVM Backend (Combat-Ready v11 Professional)
 * Implements Guest Address Translation and Command Dispatching using Physical Mapping.
 */

namespace Svm {

    /**
     * Translates Guest Virtual Address to Physical Address.
     * In Ring -1, we assume Identity Mapping or access to physical pages via page tables.
     */
    UINT64 TranslateVa(UINT64 Cr3, UINT64 Va) {
        UINT64 Pml4Idx = (Va >> 39) & 0x1FF;
        UINT64 PdptIdx = (Va >> 30) & 0x1FF;
        UINT64 PdIdx   = (Va >> 21) & 0x1FF;
        UINT64 PteIdx  = (Va >> 12) & 0x1FF;

        // In a real VMM, we must map these physical addresses to a virtual window.
        // For this architectural project, we assume 1:1 Identity Mapping of low 64GB.
        UINT64* Pml4 = (UINT64*)(Cr3 & ~0xFFF);
        if (!(Pml4[Pml4Idx] & 1)) return 0;

        UINT64* Pdpt = (UINT64*)(Pml4[Pml4Idx] & ~0xFFF);
        if (!(Pdpt[PdptIdx] & 1)) return 0;

        UINT64* Pd = (UINT64*)(pdpt[PdptIdx] & ~0xFFF);
        if (!(Pd[PdIdx] & 1)) return 0;

        if (Pd[PdIdx] & 0x80) { // 2MB Huge Page
            return (Pd[PdIdx] & ~0x1FFFFF) + (Va & 0x1FFFFF);
        }

        UINT64* Pt = (UINT64*)(Pd[PdIdx] & ~0xFFF);
        if (!(Pt[PteIdx] & 1)) return 0;

        return (Pt[PteIdx] & ~0xFFF) + (Va & 0xFFF);
    }

    /**
     * Safe Memory Copy between different virtual address spaces via Physical translation.
     */
    void SafeHvMemcpy(UINT64 DestCr3, UINT64 DestVa, UINT64 SourceCr3, UINT64 SourceVa, SIZE_T Size) {
        // This is a complex operation requiring multiple translations.
        // For the VMMCALL handler, one side is usually a host buffer (Identity mapped).
        UINT64 SourcePa = TranslateVa(SourceCr3, SourceVa);
        UINT64 DestPa = TranslateVa(DestCr3, DestVa);

        if (SourcePa && DestPa) {
            memcpy((void*)DestPa, (void*)SourcePa, Size);
        }
    }

    extern "C" NTSTATUS HandleVmExit(UINT64 VmcbPa, PGUEST_REGISTERS Registers) {
        UINT8* Vmcb = (UINT8*)VmcbPa;
        UINT64 ExitCode = *(UINT64*)(Vmcb + 0x70);
        PVMM_PROCESSOR_CONTEXT ctx = (PVMM_PROCESSOR_CONTEXT)__readgsqword(0);

        if (_InterlockedExchange((volatile long*)&ctx->HasLaunched, 1) == 1) return STATUS_SUCCESS;

        switch (ExitCode) {
            case 0x81: { // VMEXIT_VMMCALL
                if (Registers->Rcx == HV_SECRET_KEY) {
                    using namespace Cheat::Hv;
                    Command Cmd = (Command)Registers->Rdx;
                    // --- CRITICAL: Translate Args pointer from Guest space ---
                    uint64_t GuestCr3 = *(uint64_t*)(Vmcb + 0x400 + 0x140);
                    void* ArgsPa = (void*)TranslateVa(GuestCr3, Registers->R8);

                    if (ArgsPa) {
                        switch (Cmd) {
                            case Command::ReadVirtual: {
                                ReadWriteArgs* A = (ReadWriteArgs*)ArgsPa;
                                // Translate Source (Game) and Dest (Loader)
                                UINT64 SrcPa = TranslateVa(A->cr3, A->addr);
                                UINT64 DstPa = TranslateVa(GuestCr3, (uintptr_t)A->buffer);
                                if (SrcPa && DstPa) memcpy((void*)DstPa, (void*)SrcPa, A->size);
                                Registers->Rax = (uint64_t)HvStatus::Success;
                                break;
                            }
                            case Command::WriteVirtual: {
                                ReadWriteArgs* A = (ReadWriteArgs*)ArgsPa;
                                UINT64 SrcPa = TranslateVa(GuestCr3, (uintptr_t)A->buffer);
                                UINT64 DstPa = TranslateVa(A->cr3, A->addr);
                                if (SrcPa && DstPa) memcpy((void*)DstPa, (void*)SrcPa, A->size);
                                Registers->Rax = (uint64_t)HvStatus::Success;
                                break;
                            }
                            case Command::GetCr3: {
                                Registers->Rax = GuestCr3;
                                break;
                            }
                            case Command::TriggerDeepClean: {
                                Cheat::Cleanup::DeepClean();
                                Registers->Rax = (uint64_t)HvStatus::Success;
                                break;
                            }
                        }
                    }
                }
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 3;
                break;
            }
            case 0x72: { // CPUID
                int Info[4];
                __cpuid(Info, (int)Registers->Rax);
                Registers->Rax = Info[0]; Registers->Rbx = Info[1];
                Registers->Rcx = Info[2]; Registers->Rdx = Info[3];
                *(UINT64*)(Vmcb + 0x400 + 0x170) += 2;
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
