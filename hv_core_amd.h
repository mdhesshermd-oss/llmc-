#pragma once
#include <stdint.h>

/**
 * Advanced AMD SVM Core Logic
 * Includes Nested Page Table (NPT) structures and IDT for host protection.
 */

namespace Cheat {
    namespace Hv {
        namespace Amd {

            // --- NPT Structure (64-bit) ---
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
                    uint64_t pat : 1;
                    uint64_t global : 1;
                    uint64_t avl : 3;
                    uint64_t pfn : 40;     // Physical Frame Number
                    uint64_t reserved : 11;
                    uint64_t nx : 1;       // No-Execute bit
                } bits;
            };

            struct CloakedPage {
                uint64_t guest_pa;    // Guest physical address
                uint64_t original_pa; // Clean page
                uint64_t shadow_pa;   // Cheat-infected page
                bool is_executing;    // State tracking
            };

            // IDT entry for hypervisor host state
            struct IdtEntry {
                uint16_t low_offset;
                uint16_t selector;
                uint16_t flags;
                uint16_t mid_offset;
                uint32_t high_offset;
                uint32_t reserved;
            };

            inline CloakedPage g_CloakedPages[64];
            inline uint32_t g_CloakedCount = 0;
            inline IdtEntry g_HostIdt[32];

            /**
             * Traverses NPT tables to find the entry for a physical address.
             * (Conceptual implementation of table walk)
             */
            inline NptEntry* GetNptEntry(uint64_t fault_pa) {
                // In a full implementation, this would walk PML4->PDPT->PD->PT
                // based on the NPT root pointer (n_cr3) in the VMCB.
                return nullptr;
            }

            struct GuestRegisters {
                uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
            };
        }
    }
}
