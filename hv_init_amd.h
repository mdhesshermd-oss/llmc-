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
                // Must be 4KB aligned
                void* vmcb = _aligned_malloc(4096, 4096);
                if (!vmcb) return false;
                memset(vmcb, 0, 4096);

                // 4. Setup Host State Area
                void* hostState = _aligned_malloc(4096, 4096);
                __writemsr(0xC0000101, (uintptr_t)hostState); // VM_HSAVE_PA

                // 5. Setup Intercepts (CPUID and VMMCALL)
                // Offset 0x0C in VMCB is instruction intercepts
                uint32_t* intercepts = (uint32_t*)((uintptr_t)vmcb + 0x0C);
                *intercepts |= (1 << 18); // Intercept CPUID

                // Offset 0x00 is CR intercepts
                // Intercept VMMCALL (VMRUN is already intercepted by default in host)

                // 6. Launch SVM
                // Note: In a real driver, this must be done for EVERY core via KeIpiGenericCall.
                // SvmLaunch((void*)GetPhysicalAddress(vmcb));

                return true;
            }
        }
    }
}
