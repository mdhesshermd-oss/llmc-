#pragma once
#include <stdint.h>
#include <windows.h>

/**
 * HyperBone-based VMM Initialization
 * Manages VT-x transition and VMCS setup.
 */

namespace Cheat {
    namespace Hv {

        // VMX Structures (Simplified for Loader integration)
        struct VmxState {
            uint64_t vmxon_physical;
            uint64_t vmcs_physical;
            void* vmxon_region;
            void* vmcs_region;
            uint64_t ept_pointer;
        };

        /**
         * Checks if the CPU supports VT-x and EPT.
         */
        inline bool IsVmxSupported() {
            int cpuInfo[4];
            __cpuid(cpuInfo, 1);
            return (cpuInfo[2] & (1 << 5)) != 0; // VMX bit
        }

        /**
         * Allocates physically contiguous memory for VMX structures.
         */
        inline void* AllocateContiguous(size_t size, uint64_t* physical_addr) {
            // In a driver, this would be MmAllocateContiguousMemory
            // In the loader, we assume coordination with a supporting kernel component.
            return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        }

        /**
         * Initializes the VMX state for a single core.
         */
        inline bool SetupVmxCore(VmxState& state) {
            if (!IsVmxSupported()) return false;

            // 1. Setup VMXON region
            state.vmxon_region = AllocateContiguous(4096, &state.vmxon_physical);
            // Set VMX Revision ID (from MSR_IA32_VMX_BASIC)
            *static_cast<uint32_t*>(state.vmxon_region) = static_cast<uint32_t>(__readmsr(0x480));

            // 2. Setup VMCS region
            state.vmcs_region = AllocateContiguous(4096, &state.vmcs_physical);
            *static_cast<uint32_t*>(state.vmcs_region) = static_cast<uint32_t>(__readmsr(0x480));

            return true;
        }
    }
}
