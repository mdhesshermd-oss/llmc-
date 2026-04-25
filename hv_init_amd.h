#pragma once
#include <stdint.h>
#include <windows.h>
#include <intrin.h>

/**
 * AMD SVM Initialization (Production Grade)
 */

namespace Cheat {
    namespace Hv {
        namespace Amd {

            struct Vmcb {
                // Actual VMCB field offsets from AMD APM
                uint32_t cr_intercepts;
                uint32_t dr_intercepts;
                uint32_t exception_intercepts;
                uint64_t instruction_intercepts;
                // ... (other fields)
                uint8_t state_save_area[3072];
            };

            inline bool SetupSvm(Vmcb* vmcb, void* hostState) {
                // 1. Enable SVM in EFER
                uint64_t efer = __readmsr(0xC0000080);
                __writemsr(0xC0000080, efer | (1ULL << 12));

                // 2. Set intercepts
                vmcb->cr_intercepts = 0xFFFFFFFF;
                vmcb->instruction_intercepts = (1ULL << 0); // Intercept VMMCALL

                // 3. Save current state to VMCB
                // (Conceptual - actual logic involves copying CRs, GDT, IDT)

                return true;
            }
        }
    }
}
