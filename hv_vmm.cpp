#include "hv_init_amd.h"
#include "hypervisor_io.h"
#include <string.h>

/**
 * AMD SVM VM-Exit Handler
 * Executes in Ring -1 context.
 */

namespace Cheat {
    namespace Hv {
        namespace Amd {

            struct GuestRegisters {
                uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
            };

            // VMCB State Area RIP/NRIP offsets from AMD APM
            #define VMCB_RIP_OFFSET 0x570
            #define VMCB_NRIP_OFFSET 0x578

            extern "C" void HandleVmExit(void* vmcb, GuestRegisters* regs) {
                uint64_t* pVmcb = reinterpret_cast<uint64_t*>(vmcb);
                uint64_t exit_code = *(reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(vmcb) + 0x70));

                switch (exit_code) {
                    case 0x81: // VMEXIT_VMMCALL
                        if (regs->rcx == HV_SECRET_KEY) {
                            switch (regs->rdx) {
                                case HV_IO_GET_CR3:
                                    regs->rax = __readcr3();
                                    break;
                                case HV_IO_READ_VIRTUAL:
                                    // Direct memory access bypassing guest protections
                                    memcpy(reinterpret_cast<void*>(regs->r9), reinterpret_cast<void*>(regs->r8), 8);
                                    regs->rax = 1;
                                    break;
                            }
                        }
                        // Skip the vmmcall instruction
                        *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(vmcb) + VMCB_RIP_OFFSET) += 3;
                        break;

                    case 0x72: // VMEXIT_CPUID
                        if (regs->rax == 0x40000000) {
                            regs->rax = 0; // Hide hypervisor presence
                        }
                        *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(vmcb) + VMCB_RIP_OFFSET) += 2;
                        break;

                    default:
                        // Resume from next sequential instruction
                        *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(vmcb) + VMCB_RIP_OFFSET) =
                            *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(vmcb) + VMCB_NRIP_OFFSET);
                        break;
                }
            }
        }
    }
}
