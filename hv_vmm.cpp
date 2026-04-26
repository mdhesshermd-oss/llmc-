#include "hv_core.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <immintrin.h>
#include <string.h>

/**
 * Advanced AMD SVM VM-Exit Handler
 * Implements Command Dispatcher for memory operations and cloaking.
 */

namespace Cheat { namespace Hv {

    // Forward declaration of translator
    uint64_t HvTranslateVa(uint64_t guest_cr3, uint64_t va);

    extern "C" NTSTATUS HandleVmExit(void* vmcb_pa, GuestRegisters* regs) {
        PerCoreData* ctx = (PerCoreData*)__readgsqword(0);
        uint8_t* pVmcb = (uint8_t*)vmcb_pa;
        uint64_t exit_code = *(uint64_t*)(pVmcb + 0x70);

        if (InterlockedExchange(&ctx->is_processing, 1) == 1) return STATUS_SUCCESS;

        switch (exit_code) {
            case 0x72: { // VMEXIT_CPUID
                int info[4];
                __cpuid(info, (int)regs->rax);
                regs->rax = info[0]; regs->rbx = info[1];
                regs->rcx = info[2]; regs->rdx = info[3];
                *(uint64_t*)(pVmcb + 0x400 + 0x170) += 2;
                break;
            }

            case 0x81: { // VMEXIT_VMMCALL
                if (regs->rcx == HV_SECRET_KEY) {
                    Command cmd = (Command)regs->rdx;
                    void* args = (void*)regs->r8;

                    switch (cmd) {
                        case Command::ReadVirtualMemory: {
                            ReadWriteArgs* rw = (ReadWriteArgs*)args;
                            uint64_t pa = HvTranslateVa(rw->cr3, rw->address);
                            if (pa) {
                                memcpy(rw->buffer, (void*)pa, rw->size);
                                regs->rax = (uint64_t)HvStatus::Success;
                            } else {
                                regs->rax = (uint64_t)HvStatus::MemoryFault;
                            }
                            break;
                        }
                        case Command::WriteVirtualMemory: {
                            ReadWriteArgs* rw = (ReadWriteArgs*)args;
                            uint64_t pa = HvTranslateVa(rw->cr3, rw->address);
                            if (pa) {
                                memcpy((void*)pa, rw->buffer, rw->size);
                                regs->rax = (uint64_t)HvStatus::Success;
                            }
                            break;
                        }
                        case Command::GetProcessCr3: {
                            // Returns the current system CR3 for the process
                            regs->rax = __readcr3();
                            break;
                        }
                        case Command::TriggerDeepClean: {
                            Cheat::Cleanup::DeepClean();
                            regs->rax = (uint64_t)HvStatus::Success;
                            break;
                        }
                    }
                }
                *(uint64_t*)(pVmcb + 0x400 + 0x170) += 3;
                break;
            }
        }

        InterlockedExchange(&ctx->is_processing, 0);
        return ctx->lifecycle_state == 2 ? (NTSTATUS)0xC0000600 : STATUS_SUCCESS;
    }
}}
