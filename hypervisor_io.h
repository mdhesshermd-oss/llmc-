#pragma once
#include <stdint.h>
#include <intrin.h>

/**
 * Unified Hypercall Interface (Ring -1)
 * Replaces the Kernel Driver with direct Ring -1 communication via VMMCALL.
 */

namespace Cheat {
    namespace Hv {
        // Secret key for hypercall authorization
        constexpr uint64_t HV_SECRET_KEY = 0x5A4F524F5F444159; // "ZORO_DAY"

        enum class Command : uint64_t {
            GetVersion = 0x1,
            GetProcessCr3 = 0x2,
            ReadVirtualMemory = 0x3,
            WriteVirtualMemory = 0x4,
            CloakPage = 0x5
        };

        // Operation status codes
        enum class HvStatus : uint64_t {
            Success = 0,
            InvalidKey = 1,
            InvalidCommand = 2,
            MemoryFault = 3
        };

        struct ReadWriteArgs {
            uint64_t cr3;
            uint64_t address;
            void* buffer;
            uint64_t size;
        };

        struct CloakArgs {
            uint64_t guest_va;
            void* shadow_buffer;
        };

        // External assembly procedure to trigger VMMCALL/VMCALL
        extern "C" uint64_t _hv_call(uint64_t key, Command cmd, void* args);

        /**
         * Safe memory reading via Hypervisor.
         */
        template <typename T>
        inline T Read(uint64_t cr3, uint64_t address) {
            T buffer{};
            ReadWriteArgs args = { cr3, address, &buffer, sizeof(T) };
            _hv_call(HV_SECRET_KEY, Command::ReadVirtualMemory, &args);
            return buffer;
        }

        /**
         * Resolves DirectoryTableBase (CR3) for a specific PID.
         */
        inline uint64_t GetCr3(uint32_t pid) {
            // Note: In the VMM handler, RAX will be populated with the CR3
            return _hv_call(HV_SECRET_KEY, Command::GetProcessCr3, (void*)(uintptr_t)pid);
        }

        /**
         * Requests the hypervisor to cloak a page (Shadow Pages).
         */
        inline bool CloakPage(uint64_t target_va, void* payload_buffer) {
            // Allocate a dedicated page for the shadow copy
            void* shadow_page = VirtualAlloc(nullptr, 4096, MEM_COMMIT, PAGE_READWRITE);
            if (!shadow_page) return false;

            memcpy(shadow_page, payload_buffer, 4096);

            CloakArgs args;
            args.guest_va = target_va;
            args.shadow_buffer = shadow_page;

            return _hv_call(HV_SECRET_KEY, Command::CloakPage, &args) == (uint64_t)HvStatus::Success;
        }
    }
}
