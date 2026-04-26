#pragma once
#include <stdint.h>
#include <windows.h>
#include "preflight.h"
#include "setup_assistant.h"
#include "hv_init.h"
#include "hv_init_amd.h"
#include "pe_reloc.h"
#include "pe_imports.h"

/**
 * Finalized Single-File Cheat Loader with Automated Setup
 */

namespace Cheat {
    namespace Loader {

        enum class CpuVendor { Unknown, Intel, Amd };

        class UniversalLoader {
        public:
            static CpuVendor GetVendor() {
                int cpuInfo[4];
                __cpuid(cpuInfo, 0);
                if (memcmp(&cpuInfo[1], "GneD", 4) == 0) return CpuVendor::Amd;
                if (memcmp(&cpuInfo[1], "Genu", 4) == 0) return CpuVendor::Intel;
                return CpuVendor::Unknown;
            }

            static void Start() {
                // 1. Initial State Check
                if (!Preflight::IsSystemReady()) {
                    // Automatically configure BIOS/OS settings and restart
                    Setup::NotifyAndReboot();
                    return; // Execution stops here, system reboots
                }

                // 2. Hardware Detection and Virtualization
                CpuVendor vendor = GetVendor();
                if (vendor == CpuVendor::Intel) {
                    Hv::VmxState state{};
                    if (!Hv::StartVmx(state)) return;
                }
                else if (vendor == CpuVendor::Amd) {
                    // Setup AMD SVM logic from hv_init_amd.h
                }

                // 3. Continue with Stealth Manual Map
                // ManualMapper::Execute();
            }
        };
    }
}

int main() {
    Cheat::Loader::UniversalLoader::Start();
    return 0;
}
