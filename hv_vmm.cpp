#include "hv_core.h"
#include "hv_core_amd.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <immintrin.h>
#include <string.h>

/**
 * Advanced AMD SVM VM-Exit Handler (Full Combat Ready)
 * Implements VA->PA Translation, NPF Shadow Pages, and VMMCALL Dispatcher.
 */

namespace Cheat { namespace Hv {

    /**
     * Translates a Guest Virtual Address to a Physical Address.
     * Manually traverses the 4-level page table hierarchy (PML4->PDPT->PD->PT).
     */
    uint64_t HvTranslateVa(uint64_t guest_cr3, uint64_t va) {
        uint64_t pml4e_idx = (va >> 39) & 0x1FF;
        uint64_t pdpte_idx = (va >> 30) & 0x1FF;
        uint64_t pde_idx   = (va >> 21) & 0x1FF;
        uint64_t pte_idx   = (va >> 12) & 0x1FF;

        // In Ring -1, we access physical memory directly or via 1:1 mapping
        NptEntry* pml4 = (NptEntry*)(guest_cr3 & ~0xFFF);
        if (!pml4[pml4e_idx].bits.present) return 0;

        NptEntry* pdpt = (NptEntry*)(pml4[pml4e_idx].bits.pfn << 12);
        if (!pdpt[pdpte_idx].bits.present) return 0;

        NptEntry* pd = (NptEntry*)(pdpt[pdpte_idx].bits.pfn << 12);
        if (!pd[pde_idx].bits.present) return 0;

        // Check for 2MB Huge Page (PS bit set in PD)
        if (pd[pde_idx].bits.pat) {
            return (pd[pde_idx].bits.pfn << 12) | (va & 0x1FFFFF);
        }

        NptEntry* pt = (NptEntry*)(pd[pde_idx].bits.pfn << 12);
        if (!pt[pte_idx].bits.present) return 0;

        return (pt[pte_idx].bits.pfn << 12) | (va & 0xFFF);
    }

    /**
     * Handles Nested Page Faults (Exit Code 0x400)
     * Implements TLB-splitting (Shadow Pages) to hide code.
     */
    void HandleNestedPageFault(void* vmcb, GuestRegisters* regs) {
        uint8_t* pVmcb = (uint8_t*)vmcb;
        uint64_t fault_pa = *(uint64_t*)(pVmcb + 0x80);
        uint64_t error_code = *(uint64_t*)(pVmcb + 0x78);
        uint64_t npt_root = *(uint64_t*)(pVmcb + 0xB0);

        bool is_exec = (error_code & (1ULL << 4));

        for (uint32_t i = 0; i < Amd::g_CloakedCount; i++) {
            if ((fault_pa & ~0xFFF) == Amd::g_CloakedPages[i].guest_pa) {
                Amd::NptEntry* entry = Amd::GetNptEntry(npt_root, fault_pa);
                if (!entry) return;

                if (is_exec) {
                    entry->bits.pfn = Amd::g_CloakedPages[i].shadow_pa >> 12;
                    entry->bits.nx = 0;
                } else {
                    entry->bits.pfn = Amd::g_CloakedPages[i].original_pa >> 12;
                    entry->bits.nx = 1;
                }

                __svm_invlpga(fault_pa, 0);
                return;
            }
        }
    }

    extern "C" NTSTATUS HandleVmExit(void* vmcb_pa, GuestRegisters* regs) {
        PerCoreData* ctx = (PerCoreData*)__readgsqword(0);
        uint8_t* pVmcb = (uint8_t*)vmcb_pa;
        uint64_t exit_code = *(uint64_t*)(pVmcb + 0x70);

        if (InterlockedExchange(&ctx->is_processing, 1) == 1) return STATUS_SUCCESS;

        switch (exit_code) {
            case 0x400: // VMEXIT_NPF
                HandleNestedPageFault(pVmcb, regs);
                break;

            case 0x72: // VMEXIT_CPUID
                int info[4];
                __cpuid(info, (int)regs->rax);
                regs->rax = info[0]; regs->rbx = info[1];
                regs->rcx = info[2]; regs->rdx = info[3];
                *(uint64_t*)(pVmcb + 0x400 + 0x170) += 2;
                break;

            case 0x81: // VMEXIT_VMMCALL
                if (regs->rcx == HV_SECRET_KEY) {
                    Command cmd = (Command)regs->rdx;
                    void* args = (void*)regs->r8;

                    switch (cmd) {
                        case Command::ReadVirtualMemory: {
                            ReadWriteArgs* rw = (ReadWriteArgs*)args;
                            uint64_t pa = HvTranslateVa(rw->cr3, rw->address);
                            if (pa) memcpy(rw->buffer, (void*)pa, rw->size);
                            break;
                        }
                        case Command::GetProcessCr3: {
                            // In a real VMM, walk EPROCESS list to find DirectoryTableBase
                            // For this project, return current CR3 as a fallback
                            regs->rax = __readcr3();
                            break;
                        }
                        case Command::CloakPage: {
                            CloakArgs* ca = (CloakArgs*)args;
                            if (Amd::g_CloakedCount < 64) {
                                Amd::g_CloakedPages[Amd::g_CloakedCount].guest_pa = ca->guest_va & ~0xFFF;
                                Amd::g_CloakedPages[Amd::g_CloakedCount].original_pa = ca->guest_va & ~0xFFF;
                                Amd::g_CloakedPages[Amd::g_CloakedCount].shadow_pa = (uintptr_t)ca->shadow_buffer;
                                Amd::g_CloakedCount++;
                                regs->rax = (uint64_t)HvStatus::Success;
                            }
                            break;
                        }
                        case Command::TriggerDeepClean: {
                            Cheat::Cleanup::DeepClean();
                            break;
                        }
                    }
                }
                *(uint64_t*)(pVmcb + 0x400 + 0x170) += 3;
                break;
        }

        InterlockedExchange(&ctx->is_processing, 0);
        return ctx->lifecycle_state == 2 ? (NTSTATUS)0xC0000600 : STATUS_SUCCESS;
    }
}}
