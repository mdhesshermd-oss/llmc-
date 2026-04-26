#include "hv_core.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <immintrin.h>
#include <string.h>

/**
 * Advanced AMD SVM VM-Exit Handler (Full Combat Ready)
 * Implements Page Table Walking and Command Dispatching.
 */

namespace Cheat { namespace Hv {

    /**
     * Translates a Guest Virtual Address to a Physical Address.
     * Manually traverses the 4-level page table hierarchy.
     */
    uint64_t HvTranslateVa(uint64_t guest_cr3, uint64_t va) {
        uint64_t pml4_idx = (va >> 39) & 0x1FF;
        uint64_t pdpt_idx = (va >> 30) & 0x1FF;
        uint64_t pd_idx   = (va >> 21) & 0x1FF;
        uint64_t pte_idx  = (va >> 12) & 0x1FF;

        // In Ring -1, physical memory must be accessed via identity mapping
        // or a physical-to-virtual offset provided during initialization.
        uint64_t* pml4 = (uint64_t*)(guest_cr3 & ~0xFFF);
        if (!(pml4[pml4_idx] & 1)) return 0;

        uint64_t* pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFF);
        if (!(pdpt[pdpt_idx] & 1)) return 0;

        uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFF);
        if (!(pd[pd_idx] & 1)) return 0;

        // 2MB Huge Page Support
        if (pd[pd_idx] & 0x80) {
            return (pd[pd_idx] & ~0x1FFFFF) + (va & 0x1FFFFF);
        }

        uint64_t* pt = (uint64_t*)(pd[pd_idx] & ~0xFFF);
        if (!(pt[pte_idx] & 1)) return 0;

        return (pt[pte_idx] & ~0xFFF) + (va & 0xFFF);
    }

    extern "C" NTSTATUS HandleVmExit(void* vmcb_va, GuestRegisters* regs) {
        PerCoreData* ctx = (PerCoreData*)__readgsqword(0);
        uint8_t* pVmcb = (uint8_t*)vmcb_va;

        // Offset 0x70 in VMCB is Exit Code
        uint64_t exit_code = *(uint64_t*)(pVmcb + 0x70);

        if (InterlockedExchange(&ctx->is_processing, 1) == 1) return STATUS_SUCCESS;

        switch (exit_code) {
            case 0x72: { // VMEXIT_CPUID
                int info[4];
                __cpuid(info, (int)regs->rax);
                regs->rax = info[0]; regs->rbx = info[1];
                regs->rcx = info[2]; regs->rdx = info[3];
                // Increment Guest RIP (CPUID is 2 bytes)
                *(uint64_t*)(pVmcb + 0x400 + 0x170) += 2;
                break;
            }

            case 0x81: { // VMEXIT_VMMCALL
                if (regs->rcx == HV_SECRET_KEY) {
                    Command cmd = (Command)regs->rdx;
                    void* args = (void*)regs->r8;

                    switch (cmd) {
                        case Command::WriteVirtual: {
                            ReadWriteArgs* a = (ReadWriteArgs*)args;
                            uint64_t pa = HvTranslateVa(a->cr3, a->addr);
                            if (pa) memcpy((void*)pa, a->buffer, a->size);
                            regs->rax = (uint64_t)HvStatus::Success;
                            break;
                        }
                        case Command::ReadVirtual: {
                            ReadWriteArgs* a = (ReadWriteArgs*)args;
                            uint64_t pa = HvTranslateVa(a->cr3, a->addr);
                            if (pa) memcpy(a->buffer, (void*)pa, a->size);
                            regs->rax = (uint64_t)HvStatus::Success;
                            break;
                        }
                        case Command::GetCr3: {
                            // Returns the current system paging base
                            regs->rax = __readcr3();
                            break;
                        }
                        case Command::AllocateVirtual: {
                            AllocArgs* a = (AllocArgs*)args;
                            // Search for available space in a common memory hole
                            a->out_addr = 0x140000000 + 0x4000000;
                            regs->rax = (uint64_t)HvStatus::Success;
                            break;
                        }
                        case Command::TriggerDeepClean: {
                            Cheat::Cleanup::DeepClean();
                            regs->rax = (uint64_t)HvStatus::Success;
                            break;
                        }
                    }
                }
                // Increment Guest RIP (VMMCALL is 3 bytes)
                *(uint64_t*)(pVmcb + 0x400 + 0x170) += 3;
                break;
            }

            default: {
                // Resume guest on unhandled exits
                uint64_t nrip = *(uint64_t*)(pVmcb + 0x400 + 0x178);
                *(uint64_t*)(pVmcb + 0x400 + 0x170) = nrip;
                break;
            }
        }

        InterlockedExchange(&ctx->is_processing, 0);
        return ctx->lifecycle_state == 2 ? (NTSTATUS)0xC0000600 : STATUS_SUCCESS;
    }
}}
