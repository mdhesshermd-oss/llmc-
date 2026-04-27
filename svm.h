#pragma once
#include "vmm.h"

/**
 * Advanced AMD SVM Hypervisor (Combat-Ready v11)
 * Fully implemented: All critical VM-Exits, Multi-core Sync, MSR Masking, and NPF Cloaking.
 */

namespace Svm {
    // VM-Exit Codes
    enum ExitCode : uint64_t {
        VMEXIT_INVALID = -1,
        VMEXIT_CPUID = 0x72,
        VMEXIT_VMMCALL = 0x81,
        VMEXIT_NPF = 0x400,
        VMEXIT_MSR = 0x7C,
        VMEXIT_RDTSC = 0x6E,
        VMEXIT_RDTSCP = 0x7B
    };

    // VMCB Control Area Offsets
    struct VMCB_CONTROL {
        static constexpr uint32_t INTERCEPT_CR = 0x00;
        static constexpr uint32_t INTERCEPT_DR = 0x04;
        static constexpr uint32_t INTERCEPT_EXCEPTION = 0x08;
        static constexpr uint32_t INTERCEPT_INTR = 0x0C;
        static constexpr uint32_t ASID = 0x18;
        static constexpr uint32_t EXIT_CODE = 0x70;
        static constexpr uint32_t EXIT_INFO1 = 0x78;
        static constexpr uint32_t EXIT_INFO2 = 0x80;
        static constexpr uint32_t NP_ENABLE = 0xB8;
        static constexpr uint32_t NCR3 = 0xB0;
    };

    // VMCB State Save Area Offsets
    struct VMCB_STATE {
        static constexpr uint32_t CR0 = 0x400 + 0x150;
        static constexpr uint32_t CR3 = 0x400 + 0x140;
        static constexpr uint32_t CR4 = 0x400 + 0x148;
        static constexpr uint32_t RIP = 0x400 + 0x170;
        static constexpr uint32_t RSP = 0x400 + 0x1D0;
        static constexpr uint32_t EFER = 0x400 + 0x1E0;
    };

    extern "C" NTSTATUS GbhvHandleVmExit(UINT64 VmcbPa, PVOID RegistersVoid);
    extern "C" BOOLEAN GbhvSvmInitialize(PVMM_PROCESSOR_CONTEXT ctx);
    extern "C" void SvmVmExitHandler();
}
