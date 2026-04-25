#pragma once
#include <stdint.h>
#include <windows.h>
#include "hv_init.h"
#include "hv_init_amd.h"
#include "pe_reloc.h"
#include "pe_imports.h"

/**
 * Universal Multi-Vendor VMM Loader
 * Supports both Intel VT-x and AMD SVM (Ryzen).
 */

namespace Cheat {
    namespace Loader {

        enum class CpuVendor { Unknown, Intel, Amd };

        class UniversalVmm {
        public:
            static CpuVendor GetVendor() {
                int cpuInfo[4];
                __cpuid(cpuInfo, 0);
                if (memcmp(&cpuInfo[1], "GneD", 4) == 0) return CpuVendor::Amd; // "AuthenticAMD"
                if (memcmp(&cpuInfo[1], "Genu", 4) == 0) return CpuVendor::Intel; // "GenuineIntel"
                return CpuVendor::Unknown;
            }

            static bool StartHypervisor() {
                CpuVendor vendor = GetVendor();

                if (vendor == CpuVendor::Intel) {
                    if (!Hv::IsVmxSupported()) return false;
                    Hv::VmxState state{};
                    return Hv::SetupVmxCore(state);
                }
                else if (vendor == CpuVendor::Amd) {
                    if (!Hv::Amd::IsSvmSupported()) return false;
                    Hv::Amd::SvmState state{};
                    return Hv::Amd::SetupSvmCore(state);
                }

                return false;
            }

            static void Initialize() {
                if (!StartHypervisor()) {
                    // Fail-safe or fallback to Driver-only mode
                    MessageBoxA(NULL, "Hardware Virtualization Initialization Failed.", "Error", MB_ICONERROR);
                    return;
                }

                // Continue with Manual Map...
            }
        };
    }
}

int main() {
    Cheat::Loader::UniversalVmm::Initialize();
    return 0;
}
