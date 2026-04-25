#pragma once
#include <stdint.h>

/**
 * Hypervisor (Ring -1) Communication Interface
 * Uses VMCALL instructions to perform stealthy memory operations.
 */

namespace Cheat {
    namespace Hv {

        // VMCALL Interface Constants
        constexpr uint64_t HV_SECRET_KEY = 0xDEADBEEFCAFEBABE;
        constexpr uint64_t HV_IO_READ_PHYSICAL = 0x101;
        constexpr uint64_t HV_IO_GET_CR3 = 0x102;

        /**
         * Low-level VMCALL wrapper (requires MASM InternalVMCALL)
         */
        extern "C" uint64_t InternalVMCALL(uint64_t key, uint64_t code, uint64_t arg1, uint64_t arg2);

        /**
         * Reads physical memory directly bypassing all OS hooks and AC protections.
         */
        inline bool ReadPhysical(uint64_t physical_addr, void* buffer, size_t size) {
            return InternalVMCALL(HV_SECRET_KEY, HV_IO_READ_PHYSICAL, physical_addr, reinterpret_cast<uint64_t>(buffer)) == 0;
        }

        /**
         * Gets the Directory Table Base (CR3) of the target process for address translation.
         */
        inline uint64_t GetProcessCR3(uint32_t pid) {
            return InternalVMCALL(HV_SECRET_KEY, HV_IO_GET_CR3, static_cast<uint64_t>(pid), 0);
        }

        /**
         * Translates virtual address to physical address using the process CR3.
         * This logic is typically handled inside the hypervisor for speed and stealth.
         */
        template <typename T>
        inline T ReadVirtual(uint32_t pid, uint64_t virtual_addr) {
            T buffer{};
            uint64_t cr3 = GetProcessCR3(pid);
            // In a production Hv, the VMCALL would handle the walking of PML4/PDPT/PD/PT tables
            // to ensure the read is atomic and invisible to the guest OS.
            InternalVMCALL(HV_SECRET_KEY, 0x103 /* HV_IO_READ_VIRTUAL */, virtual_addr, reinterpret_cast<uint64_t>(&buffer));
            return buffer;
        }
    }
}
