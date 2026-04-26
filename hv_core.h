#pragma once
#include <ntddk.h>

/**
 * Hypervisor Core Contexts and NPT Structures
 * Defines the Ring -1 data structures for multi-core management.
 */

namespace Cheat { namespace Hv {
    // Secret authorization key for hypercalls
    constexpr uint64_t HV_SECRET_KEY = 0x5A4F524F5F444159;

    #pragma pack(push, 1)
    union NptEntry {
        uint64_t raw;
        struct {
            uint64_t present : 1;
            uint64_t write : 1;
            uint64_t user : 1;
            uint64_t pwt : 1;
            uint64_t pcd : 1;
            uint64_t accessed : 1;
            uint64_t dirty : 1;
            uint64_t pat : 1;      // Bit 7: 1 for 2MB Huge Page, 0 for pointer to next level
            uint64_t global : 1;
            uint64_t avl : 3;
            uint64_t pfn : 40;     // Physical Frame Number
            uint64_t reserved : 11;
            uint64_t nx : 1;       // No-Execute bit
        } bits;
    };

    struct PerCoreData {
        uint64_t vmcb_pa;
        uint64_t hsave_pa;
        uint64_t npt_root_pa;
        PerCoreData* self_va; // Pointer for GS-relative access in Ring -1
        volatile long lifecycle_state; // 0:Off, 1:Running, 2:Unload
        volatile long is_processing;   // Recursion protection
        uint64_t exit_count;
        uint64_t tsc_total_latency;
    };
    #pragma pack(pop)

    struct GuestRegisters {
        uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
    };
}}
