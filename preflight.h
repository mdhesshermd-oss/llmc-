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
         * Checks if VT-x (Intel) or SVM (AMD) is supported and enabled.
         */
        inline bool IsVirtualizationSupported() {
            int cpuInfo[4];

            // Check Vendor
            __cpuid(cpuInfo, 0);
            bool isIntel = memcmp(&cpuInfo[1], "Genu", 4) == 0;
            bool isAMD = memcmp(&cpuInfo[1], "Auth", 4) == 0;

            if (isIntel) {
                __cpuid(cpuInfo, 1);
                return (cpuInfo[2] & (1 << 5)) != 0; // VT-x
            } else if (isAMD) {
                __cpuid(cpuInfo, 0x80000001);
                return (cpuInfo[2] & (1 << 2)) != 0; // SVM
            }

            return false;
        }

        /**
         * Detects Secure Boot status via UEFI variables.
         */
        inline bool IsSecureBootDisabled() {
            // Note: Accessing firmware variables requires SeSystemEnvironmentPrivilege
            // For a user-mode pre-check, we try to read it.
            uint8_t secureBoot = 0;
            DWORD size = GetFirmwareEnvironmentVariableA("SecureBoot", "{8be4df61-93ca-11d2-aa0d-00e098032b8c}", &secureBoot, sizeof(secureBoot));

            if (size == 0) {
                DWORD err = GetLastError();
                if (err == ERROR_INVALID_FUNCTION) return true; // BIOS/No Secure Boot support
                if (err == ERROR_PRIVILEGE_NOT_HELD) return true; // Assume disabled if we can't check, or handle in driver
            }

            return (secureBoot == 0);
        }

        /**
         * Detects if another Hypervisor (like Hyper-V) is currently active.
         * Our hypervisor cannot nest easily without complex logic.
         */
        inline bool IsHyperVEnabled() {
            int cpuInfo[4];
            __cpuid(cpuInfo, 1);
            return (cpuInfo[2] & (1 << 31)) != 0; // Hypervisor bit
        }

        inline bool RunAllChecks() {
            bool supported = IsVirtualizationSupported();
            bool secureBootOff = IsSecureBootDisabled();
            bool hyperVOff = !IsHyperVEnabled();

            return supported && secureBootOff && hyperVOff;
        }
    }
}
