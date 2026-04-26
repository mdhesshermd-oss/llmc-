#include "vmm.h"
#include "svm.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <string.h>

/**
 * Gbhv-style AMD SVM Backend Implementation
 */

namespace Svm {

    /**
     * Translates Guest VA to Physical Address via Page Table Walking.
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

        if (Pd[PdIdx] & 0x80) { // Huge Page
            return (Pd[PdIdx] & ~0x1FFFFF) + (Va & 0x1FFFFF);
        }

        UINT64* Pt = (UINT64*)(Pd[PdIdx] & ~0xFFF);
        if (!(Pt[PteIdx] & 1)) return 0;

        return (Pt[PteIdx] & ~0xFFF) + (Va & 0xFFF);
    }

    extern "C" BOOLEAN GbhvSvmInitialize(PVMM_PROCESSOR_CONTEXT ProcessorContext) {
        // 1. Allocate VMCB and Host State
        PHYSICAL_ADDRESS High; High.QuadPart = ~0ULL;
        ProcessorContext->VmcbVirtual = MmAllocateContiguousMemory(PAGE_SIZE, High);
        ProcessorContext->HostSaveVirtual = MmAllocateContiguousMemory(PAGE_SIZE, High);

        if (!ProcessorContext->VmcbVirtual || !ProcessorContext->HostSaveVirtual) return FALSE;

        ProcessorContext->VmcbPhysical = MmGetPhysicalAddress(ProcessorContext->VmcbVirtual).QuadPart;
        ProcessorContext->HostSavePhysical = MmGetPhysicalAddress(ProcessorContext->HostSaveVirtual).QuadPart;

        RtlZeroMemory(ProcessorContext->VmcbVirtual, PAGE_SIZE);

        // 2. Setup VMCB fundamental fields
        UINT8* Vmcb = (UINT8*)ProcessorContext->VmcbVirtual;

        // Host RIP setup
        extern void* SvmVmExitHandler;
        *(UINT64*)(Vmcb + 0x400 + 0x1E8) = (UINT64)&SvmVmExitHandler;

        // Guest State Copy (Conceptual - real code would sync GDT/IDT/CRs)
        *(UINT64*)(Vmcb + 0x400 + 0x1F8) = __readmsr(0xC0000080) | (1ULL << 12); // EFER.SVME
        *(UINT64*)(Vmcb + 0x400 + 0x140) = __readcr3(); // Guest CR3

        // 3. Launch
        GbhvSvmLaunch(ProcessorContext->VmcbPhysical, ProcessorContext->HostSavePhysical, ProcessorContext);

        return TRUE;
    }

    extern "C" NTSTATUS HandleVmExit(UINT64 VmcbPa, PGUEST_REGISTERS Registers) {
        // Note: In Ring -1 context, VmcbPa must be translated back to virtual
        // For this architectural overview, we assume VMCB is accessible
        UINT8* Vmcb = (UINT8*)VmcbPa;
        UINT64 ExitCode = *(UINT64*)(Vmcb + 0x70);

        if (Registers->Rcx == HV_SECRET_KEY) {
            using namespace Cheat::Hv;
            Command Cmd = (Command)Registers->Rdx;
            void* Args = (void*)Registers->R8;

            switch (Cmd) {
                case Command::ReadVirtual: {
                    ReadWriteArgs* A = (ReadWriteArgs*)Args;
                    UINT64 Pa = TranslateVa(A->cr3, A->addr);
                    if (Pa) memcpy(A->buffer, (void*)Pa, A->size);
                    Registers->Rax = 0;
                    break;
                }
                case Command::WriteVirtual: {
                    ReadWriteArgs* A = (ReadWriteArgs*)Args;
                    UINT64 Pa = TranslateVa(A->cr3, A->addr);
                    if (Pa) memcpy((void*)Pa, A->buffer, A->size);
                    Registers->Rax = 0;
                    break;
                }
                case Command::GetCr3: {
                    Registers->Rax = __readcr3();
                    break;
                }
                case Command::AllocateVirtual: {
                    AllocArgs* A = (AllocArgs*)Args;
                    A->out_addr = 0x140000000 + 0x5000000;
                    Registers->Rax = 0;
                    break;
                }
                case Command::TriggerDeepClean: {
                    Cheat::Cleanup::DeepClean();
                    Registers->Rax = 0;
                    break;
                }
            }
        }

        // Advance RIP (VMMCALL is 3 bytes)
        *(UINT64*)(Vmcb + 0x400 + 0x170) += 3;
        return STATUS_SUCCESS;
    }
}
