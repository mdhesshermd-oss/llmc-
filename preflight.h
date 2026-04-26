#pragma once
#include <stdint.h>
#include <windows.h>
#include <intrin.h>

/**
 * Pre-flight Validation Engine
 * Checks if the system is ready for Ring -1 Hypervisor execution.
 */

namespace Cheat {
    namespace Preflight {

        /**
         * Checks if VT-x (Intel) or SVM (AMD) is enabled in BIOS.
         */
        inline bool IsVirtualizationEnabled() {
            int cpuInfo[4];
            __cpuid(cpuInfo, 1);
            bool intel_vt = (cpuInfo[2] & (1 << 5)) != 0;

            __cpuid(cpuInfo, 0x80000001);
            bool amd_svm = (cpuInfo[2] & (1 << 2)) != 0;

            return intel_vt || amd_svm;
        }

        /**
         * Detects Secure Boot status via UEFI variables.
         */
        inline bool IsSecureBootDisabled() {
            uint8_t secureBoot = 0;
            DWORD size = GetFirmwareEnvironmentVariableA("SecureBoot", "{8be4df61-93ca-11d2-aa0d-00e098032b8c}", &secureBoot, sizeof(secureBoot));
            if (size == 0 && GetLastError() == ERROR_INVALID_FUNCTION) {
                // Legacy BIOS or Secure Boot unsupported
                return true;
            }
            return (secureBoot == 0);
        }

        /**
         * Detects if Hyper-V is currently active.
         */
        inline bool IsHyperVEnabled() {
            // Simple check: CPUID leaves for Hypervisor
            int cpuInfo[4];
            __cpuid(cpuInfo, 1);
            return (cpuInfo[2] & (1 << 31)) != 0; // Hypervisor bit
        }

        inline bool IsSystemReady() {
            return IsVirtualizationEnabled() && IsSecureBootDisabled() && IsHyperVEnabled();
        }
    }
}
