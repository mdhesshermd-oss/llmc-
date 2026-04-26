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
             * Вычисляет адрес записи в таблице NPT для гостевого физического адреса.
             * npt_pml4_pa - физический адрес корня таблиц NPT (из VMCB).
             */
            inline NptEntry* GetNptEntry(uint64_t npt_pml4_pa, uint64_t fault_pa) {
                // Индексы для каждого уровня
                uint64_t pml4e_idx = (fault_pa >> 39) & 0x1FF;
                uint64_t pdpte_idx = (fault_pa >> 30) & 0x1FF;
                uint64_t pde_idx   = (fault_pa >> 21) & 0x1FF;
                uint64_t pte_idx   = (fault_pa >> 12) & 0x1FF;

                // PML4 -> PDPT
                NptEntry* pml4 = (NptEntry*)npt_pml4_pa;
                if (!pml4[pml4e_idx].bits.present) return nullptr;

                // PDPT -> PD
                NptEntry* pdpt = (NptEntry*)(pml4[pml4e_idx].bits.pfn << 12);
                if (!pdpt[pdpte_idx].bits.present) return nullptr;

                // PD -> PT
                NptEntry* pd = (NptEntry*)(pdpt[pdpte_idx].bits.pfn << 12);
                if (!pd[pde_idx].bits.present) return nullptr;

                // PT -> Entry
                NptEntry* pt = (NptEntry*)(pd[pde_idx].bits.pfn << 12);
                return &pt[pte_idx];
            }

            struct GuestRegisters {
                uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
            };
        }
    }
}
