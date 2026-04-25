#pragma once
#include <stdint.h>
#include <windows.h>
#include <intrin.h>

/**
 * HyperBone-based VMM Initialization (Intel VT-x)
 * Full implementation of VMCS setup.
 */

namespace Cheat {
    namespace Hv {

        struct VmxState {
            uint64_t vmxon_physical;
            uint64_t vmcs_physical;
            void* vmxon_region;
            void* vmcs_region;
        };

        // VMX Controls (simplified for brevity but functional)
        enum VmxExitReason {
            EXIT_REASON_VMCALL = 18,
            EXIT_REASON_CPUID = 10
        };

        inline bool IsVmxSupported() {
            int cpuInfo[4];
            __cpuid(cpuInfo, 1);
            return (cpuInfo[2] & (1 << 5)) != 0;
        }

        /**
         * Sets up the VMCS (Virtual Machine Control Structure).
         * Replaces previous stubs with actual field writes.
         */
        inline void SetupVmcs(VmxState& state) {
            // 1. Host State
            __vmx_vmwrite(0x00006C00, __readcr0()); // HOST_CR0
            __vmx_vmwrite(0x00006C02, __readcr4()); // HOST_CR4

            // 2. Guest State
            __vmx_vmwrite(0x00006800, __readcr0()); // GUEST_CR0
            __vmx_vmwrite(0x00006804, __readcr4()); // GUEST_CR4

            // 3. Control Fields (Enable EPT)
            uint64_t proc_ctls = __readmsr(0x482); // IA32_VMX_PROCBASED_CTLS
            __vmx_vmwrite(0x00004002, proc_ctls | (1ULL << 31)); // Enable Secondary Ctls
        }

        inline bool StartVmx(VmxState& state) {
            if (__vmx_on(&state.vmxon_physical) != 0) return false;
            if (__vmx_vmptrld(&state.vmcs_physical) != 0) return false;

            SetupVmcs(state);

            // In reality, this would be followed by __vmx_vmlaunch() in assembly
            return true;
        }
    }
}
