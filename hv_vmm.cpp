#include "hv_core_amd.h"
#include "hypervisor_io.h"
#include "stealth_cleanup.h"
#include <intrin.h>
#include <string.h>

/**
 * AMD SVM VM-Exit Handler (Production Version)
 * Handles Hypercalls (VMMCALL), NPT Faults (#NPF), and CPUID.
 */

namespace Cheat {
    namespace Hv {
        namespace Amd {

            /**
             * Переводит гостевой Виртуальный адрес в Физический, используя CR3 процесса.
             */
            uint64_t HvTranslateVa(uint64_t guest_cr3, uint64_t va) {
                uint64_t pml4e_idx = (va >> 39) & 0x1FF;
                uint64_t pdpte_idx = (va >> 30) & 0x1FF;
                uint64_t pde_idx   = (va >> 21) & 0x1FF;
                uint64_t pte_idx   = (va >> 12) & 0x1FF;

                // Note: Physical addresses must be accessed via a 1:1 mapping or
                // by temporarily mapping them. In this simplified HV, we assume
                // physical memory is accessible or use a identity-mapped region.

                NptEntry* pml4 = (NptEntry*)(guest_cr3 & ~0xFFF);
                if (!pml4[pml4e_idx].bits.present) return 0;

                NptEntry* pdpt = (NptEntry*)(pml4[pml4e_idx].bits.pfn << 12);
                if (!pdpt[pdpte_idx].bits.present) return 0;

                NptEntry* pd = (NptEntry*)(pdpt[pdpte_idx].bits.pfn << 12);
                if (!pd[pde_idx].bits.present) return 0;

                NptEntry* pt = (NptEntry*)(pd[pde_idx].bits.pfn << 12);
                if (!pt[pte_idx].bits.present) return 0;

                return (pt[pte_idx].bits.pfn << 12) | (va & 0xFFF);
            }

            /**
             * Handles Nested Page Faults (Exit Code 0x400).
             */
            void HandleNestedPageFault(void* vmcb, GuestRegisters* regs) {
                uint8_t* pVmcb = (uint8_t*)vmcb;
                uint64_t fault_pa = *(uint64_t*)(pVmcb + 0x80); // VMCB_EXITINFO2
                uint64_t error_code = *(uint64_t*)(pVmcb + 0x78); // VMCB_EXITINFO1
                uint64_t npt_pml4 = *(uint64_t*)(pVmcb + 0xB0); // n_cr3

                bool is_id_fetch = (error_code & (1ULL << 4));

                for (uint32_t i = 0; i < g_CloakedCount; i++) {
                    if ((fault_pa & ~0xFFF) == g_CloakedPages[i].guest_pa) {
                        NptEntry* entry = GetNptEntry(npt_pml4, fault_pa);
                        if (!entry) return;

                        if (is_id_fetch) {
                            entry->bits.pfn = g_CloakedPages[i].shadow_pa >> 12;
                            entry->bits.nx = 0;
                            g_CloakedPages[i].is_executing = true;
                        } else {
                            entry->bits.pfn = g_CloakedPages[i].original_pa >> 12;
                            entry->bits.nx = 1;
                            g_CloakedPages[i].is_executing = false;
                        }

                        *(uint32_t*)(pVmcb + 0x58) = 1;
                        return;
                    }
                }
            }

            extern "C" void HvPanicHandler() {
                uint64_t efer = __readmsr(0xC0000080);
                __writemsr(0xC0000080, efer & ~(1ULL << 12));
            }

            extern "C" void HandleVmExit(void* vmcb, GuestRegisters* regs) {
                uint8_t* pVmcb = (uint8_t*)vmcb;
                uint64_t exit_code = *(uint64_t*)(pVmcb + 0x70);

                switch (exit_code) {
                    case 0x81: // VMEXIT_VMMCALL
                        if (regs->rcx != HV_SECRET_KEY) {
                            regs->rax = (uint64_t)HvStatus::InvalidKey;
                        } else {
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
                                case Command::GetProcessCr3: {
                                    regs->rax = __readcr3();
                                    break;
                                }
                                case Command::CloakPage: {
                                    CloakArgs* ca = (CloakArgs*)args;
                                    if (g_CloakedCount < 64) {
                                        g_CloakedPages[g_CloakedCount].guest_pa = ca->guest_va & ~0xFFF;
                                        g_CloakedPages[g_CloakedCount].original_pa = ca->guest_va & ~0xFFF;
                                        g_CloakedPages[g_CloakedCount].shadow_pa = (uintptr_t)ca->shadow_buffer;
                                        g_CloakedCount++;
                                        regs->rax = (uint64_t)HvStatus::Success;
                                    }
                                    break;
                                }
                                case Command::TriggerDeepClean: {
                                    Cheat::Cleanup::DeepClean();
                                    regs->rax = (uint64_t)HvStatus::Success;
                                    break;
                                }
                            }
                        }
                        *(uint64_t*)(pVmcb + 0x570) += 3;
                        break;

                    case 0x400: // VMEXIT_NPF
                        HandleNestedPageFault(vmcb, regs);
                        break;

                    case 0x72: // VMEXIT_CPUID
                        if (regs->rax == 0x40000000) regs->rax = 0;
                        *(uint64_t*)(pVmcb + 0x570) += 2;
                        break;

                    default:
                        *(uint64_t*)(pVmcb + 0x570) = *(uint64_t*)(pVmcb + 0x578);
                        break;
                }
            }
        }
    }
}
