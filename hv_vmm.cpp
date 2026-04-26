#include "hv_core_amd.h"
#include "hypervisor_io.h"
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
             * Handles Nested Page Faults (Exit Code 0x400).
             * Implements the "Shadow Pages" mechanism to hide cheat memory.
             */
            void HandleNestedPageFault(void* vmcb, GuestRegisters* regs) {
                uint8_t* pVmcb = (uint8_t*)vmcb;

                // Physical address that caused the fault
                uint64_t fault_pa = *(uint64_t*)(pVmcb + 0x80); // VMCB_EXITINFO2
                uint64_t error_code = *(uint64_t*)(pVmcb + 0x78); // VMCB_EXITINFO1

                bool is_id_fetch = (error_code & (1ULL << 4)); // ID bit (Instruction Fetch)

                for (uint32_t i = 0; i < g_CloakedCount; i++) {
                    if ((fault_pa & ~0xFFF) == g_CloakedPages[i].guest_pa) {
                        NptEntry* entry = GetNptEntry(fault_pa);
                        if (!entry) return;

                        if (is_id_fetch) {
                            // EXECUTION mode -> Redirect to Shadow page (Cheat code)
                            entry->bits.pfn = g_CloakedPages[i].shadow_pa >> 12;
                            entry->bits.nx = 0; // Allow execute
                            g_CloakedPages[i].is_executing = true;
                        } else {
                            // READ/WRITE mode -> Redirect to Original page (Clean code)
                            entry->bits.pfn = g_CloakedPages[i].original_pa >> 12;
                            entry->bits.nx = 1; // Disallow execute to catch next run
                            g_CloakedPages[i].is_executing = false;
                        }

                        // Flush TLB to apply changes
                        *(uint32_t*)(pVmcb + 0x58) = 1; // TLB_CONTROL_FLUSH_ALL_ASID
                        return;
                    }
                }
            }

            /**
             * Emergency shutdown if the hypervisor crashes.
             */
            extern "C" void HvPanicHandler() {
                uint64_t efer = __readmsr(0xC0000080);
                __writemsr(0xC0000080, efer & ~(1ULL << 12)); // Disable SVM
                // Attempt to return to guest or hang to avoid Triple Fault
            }

            void SetupHostExceptionHandling(void* vmcb) {
                extern void* hv_exception_stub;
                uintptr_t handler = (uintptr_t)&hv_exception_stub;

                for (int i = 0; i < 32; i++) {
                    g_HostIdt[i].low_offset = (uint16_t)handler;
                    g_HostIdt[i].mid_offset = (uint16_t)(handler >> 16);
                    g_HostIdt[i].high_offset = (uint32_t)(handler >> 32);
                    g_HostIdt[i].selector = 0x10;
                    g_HostIdt[i].flags = 0x8E00;
                }
                // Host IDT would be loaded into the VMCB's Host State Area here.
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
                                    // Direct copy bypassing all Ring 0 hooks
                                    memcpy(rw->buffer, (void*)rw->address, rw->size);
                                    regs->rax = (uint64_t)HvStatus::Success;
                                    break;
                                }
                                case Command::GetProcessCr3: {
                                    // Simplified: in real implementation, find EPROCESS by PID
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
                            }
                        }
                        *(uint64_t*)(pVmcb + 0x570) += 3; // Skip VMMCALL
                        break;

                    case 0x400: // VMEXIT_NPF
                        HandleNestedPageFault(vmcb, regs);
                        break;

                    case 0x72: // VMEXIT_CPUID
                        if (regs->rax == 0x40000000) regs->rax = 0; // Hide HV presence
                        *(uint64_t*)(pVmcb + 0x570) += 2; // Skip CPUID
                        break;

                    default:
                        // Step over instructions that caused generic exits
                        *(uint64_t*)(pVmcb + 0x570) = *(uint64_t*)(pVmcb + 0x578);
                        break;
                }
            }
        }
    }
}
