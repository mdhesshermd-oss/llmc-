#pragma once
#include <stdint.h>
#include <intrin.h>

/**
 * Universal Hypervisor (Ring -1) Communication Interface
 * Supports both VMCALL (Intel) and VMMCALL (AMD).
 */

namespace Cheat {
    namespace Hv {

        constexpr uint64_t HV_SECRET_KEY = 0xDEADBEEFCAFEBABE;
        constexpr uint64_t HV_IO_READ_VIRTUAL = 0x103;
        constexpr uint64_t HV_IO_GET_CR3 = 0x102;

        extern "C" uint64_t InternalVMCALL(uint64_t key, uint64_t code, uint64_t arg1, uint64_t arg2);
        extern "C" uint64_t InternalVMMCALL(uint64_t key, uint64_t code, uint64_t arg1, uint64_t arg2);

        inline bool IsAmd() {
            int cpuInfo[4];
            __cpuid(cpuInfo, 0);
            return (cpuInfo[1] == 0x68747541); // "Auth" in "AuthenticAMD"
        }

        /**
         * Generic hypercall wrapper that selects the correct instruction for the CPU.
         */
        inline uint64_t Hypercall(uint64_t code, uint64_t arg1, uint64_t arg2) {
            if (IsAmd()) {
                return InternalVMMCALL(HV_SECRET_KEY, code, arg1, arg2);
            } else {
                return InternalVMCALL(HV_SECRET_KEY, code, arg1, arg2);
            }
        }

        template <typename T>
        inline T ReadVirtual(uint32_t pid, uint64_t virtual_addr) {
            T buffer{};
            // The hypervisor handles address translation and memory access
            Hypercall(HV_IO_READ_VIRTUAL, virtual_addr, reinterpret_cast<uint64_t>(&buffer));
            return buffer;
        }

        inline uint64_t GetProcessCR3(uint32_t pid) {
            return Hypercall(HV_IO_GET_CR3, static_cast<uint64_t>(pid), 0);
        }
    }
}
