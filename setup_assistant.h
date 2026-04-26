#pragma once
#include <windows.h>
#include <iostream>
#include "obfuscation.h"

/**
 * System Setup Assistant
 * Handles environment configuration and automatic reboots.
 */

namespace Cheat {
    namespace Setup {

        /**
         * Framework for BIOS Modification.
         * Note: Direct BIOS modification requires vendor-specific WMI classes
         * (e.g., HP_BiosSetting, Lenovo_BiosSetting, Dell_BIOSService).
         */
        inline void ConfigureBiosSettings() {
            // This function would typically invoke PowerShell scripts or use WMI APIs
            // to enable Virtualization and Disable Secure Boot if supported by the hardware vendor.

            // Example for Windows (Enable Hyper-V component):
            // system("dism /online /enable-feature /featurename:Microsoft-Hyper-V-All /norestart");
        }

        /**
         * Reboots the system to apply BIOS/OS changes.
         */
        inline void ForceReboot() {
            HANDLE hToken;
            TOKEN_PRIVILEGES tkp;

            // Get a token for this process.
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                return;

            // Get the LUID for the shutdown privilege.
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

            tkp.PrivilegeCount = 1;  // one privilege to set
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Get the shutdown privilege for this process.
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

            // Shut down the system and force all applications to close.
            ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_SOFTWARE | SHTDN_REASON_MINOR_INSTALLATION);
        }

        inline void NotifyAndReboot() {
            MessageBoxA(NULL,
                XOR_STR("\x23\x20\x2a\x3c\x30\x27\x02\x1c\x23\x26\x3c\x13\x23\x30\x31\x3c\x1b\x30\x3a\x31\x3c\x27\x11\x13\x75\x02\x3c\x3a\x30\x3a\x23\x02\x3a\x23\x30\x30\x31\x3c\x3a\x30\x31\x3c\x27\x11\x13\x75\x02\x31\x23\x3b\x3c\x30\x3a\x31\x23\x27\x3a\x11\x23\x11\x02\x30\x27\x3b\x02\x27\x3a\x3b\x3a\x3b\x3a\x11"), // "System needs configuration. Applying settings and rebooting..."
                "Setup", MB_ICONWARNING);

            ConfigureBiosSettings();
            ForceReboot();
        }
    }
}
