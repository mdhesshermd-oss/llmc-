#pragma once
#include <stdint.h>
#include <windows.h>

/**
 * Unified Hypercall Interface (Ring -1)
 * Finalized: Added dynamic remote memory allocation.
 */

namespace Cheat { namespace Hv {
    constexpr uint64_t HV_SECRET_KEY = 0x5A4F524F5F444159;

    enum class Command : uint64_t {
        GetVersion = 0x1,
        GetCr3 = 0x2,
        ReadVirtual = 0x3,
        WriteVirtual = 0x4,
        CloakPage = 0x5,
        TriggerDeepClean = 0x6,
        AllocateVirtual = 0x7
    };

    // Operation status codes
    enum class HvStatus : uint64_t {
        Success = 0,
        InvalidKey = 1,
        InvalidCommand = 2,
        MemoryFault = 3
    };

    extern "C" uint64_t _hv_call(uint64_t key, Command cmd, void* args);

    struct AllocArgs {
        uint64_t cr3;
        size_t size;
        uintptr_t out_addr;
    };

    struct ReadWriteArgs {
        uint64_t cr3;
        uintptr_t addr;
        void* buffer;
        size_t size;
    };

    struct CloakArgs {
        uintptr_t guest_va;
        void* shadow_buffer;
    };

    /**
     * Dynamically allocates memory in the target process via Hypervisor.
     */
    inline uintptr_t AllocateRemoteMemory(uint64_t cr3, size_t size) {
        AllocArgs args = { cr3, size, 0 };
        _hv_call(HV_SECRET_KEY, Command::AllocateVirtual, &args);
        return args.out_addr;
    }

    /**
     * Writes raw memory to the target process via Hypervisor.
     */
    inline bool WriteRaw(uint64_t cr3, uintptr_t addr, void* buf, size_t sz) {
        ReadWriteArgs args = { cr3, addr, buf, sz };
        return _hv_call(HV_SECRET_KEY, Command::WriteVirtual, &args) == (uint64_t)HvStatus::Success;
    }

    /**
     * Reads raw memory from the target process via Hypervisor.
     */
    inline bool ReadRaw(uint64_t cr3, uintptr_t addr, void* buf, size_t sz) {
        ReadWriteArgs args = { cr3, addr, buf, sz };
        return _hv_call(HV_SECRET_KEY, Command::ReadVirtual, &args) == (uint64_t)HvStatus::Success;
    }

    /**
     * Resolves process CR3 base.
     */
    inline uint64_t GetCr3(uint32_t pid) {
        return _hv_call(HV_SECRET_KEY, Command::GetCr3, (void*)(uintptr_t)pid);
    }

    /**
     * Requests the hypervisor to cloak a page (Shadow Pages).
     */
    inline bool CloakPage(uintptr_t target_va, void* payload_buffer) {
        void* shadow_page = VirtualAlloc(nullptr, 4096, MEM_COMMIT, PAGE_READWRITE);
        if (!shadow_page) return false;

        memcpy(shadow_page, payload_buffer, 4096);

        CloakArgs args;
        args.guest_va = target_va;
        args.shadow_buffer = shadow_page;

        return _hv_call(HV_SECRET_KEY, Command::CloakPage, &args) == (uint64_t)HvStatus::Success;
    }

    /**
     * Triggers kernel trace removal from Ring -1 context.
     */
    inline void TriggerDeepClean() {
        _hv_call(HV_SECRET_KEY, Command::TriggerDeepClean, nullptr);
    }

    // Helper templates
    template <typename T>
    inline T Read(uint64_t cr3, uintptr_t address) {
        T buffer{};
        ReadRaw(cr3, address, &buffer, sizeof(T));
        return buffer;
    }

    template <typename T>
    inline bool Write(uint64_t cr3, uintptr_t address, T value) {
        return WriteRaw(cr3, address, &value, sizeof(T));
    }
}}
