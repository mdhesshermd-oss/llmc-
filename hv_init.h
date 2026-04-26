#pragma once
#include <stdint.h>
#include <windows.h>
#include <intrin.h>

/**
 * Intel VT-x Initialization
 */

namespace Cheat {
    namespace Hv {
        namespace Intel {

            /**
             * Initializes VT-x for the current core.
             */
            inline bool InitializeVT() {
                // 1. Check VT-x support
                int cpuInfo[4];
                __cpuid(cpuInfo, 1);
                if (!(cpuInfo[2] & (1 << 5))) return false;

                // 2. Enable VMX in CR4
                uint64_t cr4 = __readcr4();
                __writecr4(cr4 | (1ULL << 13));

                // 3. Setup VMXON region
                void* vmxon = _aligned_malloc(4096, 4096);
                if (!vmxon) return false;
                memset(vmxon, 0, 4096);

                // Write Revision ID
                uint32_t revisionId = (uint32_t)__readmsr(0x480);
                *(uint32_t*)vmxon = revisionId;

                // 4. Enter VMX Root Operation
                // __vmx_on((unsigned __int64*)GetPhysicalAddress(vmxon));

                return true;
            }
        }
    }
}
