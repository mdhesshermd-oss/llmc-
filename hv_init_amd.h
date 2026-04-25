#pragma once
#include <stdint.h>
#include <windows.h>
#include <intrin.h>

/**
 * AMD SVM (AMD-V) Initialization
 * Manages VMCB setup and SVM transition for Ryzen CPUs.
 */

namespace Cheat {
    namespace Hv {
        namespace Amd {

            // Virtual Machine Control Block (VMCB) - Simplified
            struct __declspec(align(4096)) Vmcb {
                uint8_t control_area[1024];
                uint8_t state_save_area[3072];
            };

            struct SvmState {
                uint64_t vmcb_physical;
                uint64_t host_state_physical;
                Vmcb* vmcb;
                void* host_state_area;
            };

            /**
             * Checks if the CPU supports AMD-V (SVM).
             */
            inline bool IsSvmSupported() {
                int cpuInfo[4];

                // 1. Check Vendor
                __cpuid(cpuInfo, 0);
                if (memcmp(&cpuInfo[1], "GneD", 4) != 0) return false; // "AuthenticAMD" check (simplified)

                // 2. Check SVM Support
                __cpuid(cpuInfo, 0x80000001);
                return (cpuInfo[2] & (1 << 2)) != 0; // ECX bit 2: SVM
            }

            /**
             * Initializes the SVM state for an AMD Ryzen core.
             */
            inline bool SetupSvmCore(SvmState& state) {
                if (!IsSvmSupported()) return false;

                // 1. Allocate VMCB (Must be 4KB aligned and contiguous)
                state.vmcb = static_cast<Vmcb*>(VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
                // state.vmcb_physical = GetPhysicalAddress(state.vmcb);

                // 2. Allocate Host State Save Area
                state.host_state_area = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                // state.host_state_physical = GetPhysicalAddress(state.host_state_area);

                // 3. Enable SVM in EFER MSR (Extended Feature Enable Register)
                // uint64_t efer = __readmsr(0xC0000080);
                // __writemsr(0xC0000080, efer | (1ULL << 12)); // SVME bit

                return true;
            }
        }
    }
}
