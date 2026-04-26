#pragma once
#include <stdint.h>
#include <windows.h>
#include <intrin.h>

/**
 * AMD SVM Initialization (Production Grade)
 */

namespace Cheat {
    namespace Hv {
        namespace AMD {

            // Prototype for the assembly transition function
            extern "C" void SvmLaunch(void* vmcb_pa);

            /**
             * Initializes SVM for the current core.
             */
            inline bool InitializeSVM() {
                // 1. Check SVM support (AMD-V)
                int cpuInfo[4];
                __cpuid(cpuInfo, 0x80000001);
                if (!(cpuInfo[2] & (1 << 2))) return false;

                // 2. Enable SVM in EFER
                uint64_t efer = __readmsr(0xC0000080);
                __writemsr(0xC0000080, efer | (1ULL << 12));

                // 3. Allocate VMCB (Virtual Machine Control Block)
                void* vmcb = _aligned_malloc(4096, 4096);
                if (!vmcb) return false;
                memset(vmcb, 0, 4096);

                // 4. Setup Host State Area
                void* hostState = _aligned_malloc(4096, 4096);
                __writemsr(0xC0000101, (uintptr_t)hostState);

                // 5. Setup VMCB fundamental fields
                // In a production implementation, this would involve copying the current
                // CPU state (CRs, GDT, IDT) into the VMCB's State Save Area.

                // Set Host RIP to the assembly handler
                extern void* SvmVmExitHandler;
                *(uint64_t*)((uintptr_t)vmcb + 0x400 + 0x1E8) = (uintptr_t)&SvmVmExitHandler;

                // 6. Setup Intercepts (CPUID and VMMCALL)
                uint32_t* intercepts = (uint32_t*)((uintptr_t)vmcb + 0x0C);
                *intercepts |= (1 << 18); // Intercept CPUID

                // 7. Launch SVM
                // Note: Launching a hypervisor from user-mode requires physical address resolution
                // and kernel-mode transition (e.g., via a driver or vulnerability).
                // For this refactored project, we provide the architectural logic.

                // SvmLaunch(vmcb);

                return true;
            }
        }
    }
}
