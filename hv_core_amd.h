#pragma once
#include <stdint.h>

/**
 * AMD Nested Page Tables (NPT) Management
 * AMD equivalent of Intel EPT for memory cloaking and isolation.
 */

namespace Cheat {
    namespace Hv {
        namespace Amd {

            // NPT Entry Bits (similar but not identical to Intel EPT)
            constexpr uint64_t NPT_READ = (1ULL << 0);
            constexpr uint64_t NPT_WRITE = (1ULL << 1);
            constexpr uint64_t NPT_USER = (1ULL << 2);
            constexpr uint64_t NPT_NX = (1ULL << 63); // No-Execute bit

            union NptEntry {
                uint64_t value;
                struct {
                    uint64_t read : 1;
                    uint64_t write : 1;
                    uint64_t user : 1;
                    uint64_t pwt : 1;
                    uint64_t pcd : 1;
                    uint64_t accessed : 1;
                    uint64_t dirty : 1;
                    uint64_t pat : 1;
                    uint64_t global : 1;
                    uint64_t ignored_1 : 3;
                    uint64_t pfn : 40;
                    uint64_t ignored_2 : 11;
                    uint64_t nx : 1;
                } bits;
            };

            /**
             * Implements memory cloaking using NPT.
             */
            inline void CloakMemoryNpt(uint64_t guest_phys, uint64_t shadow_phys) {
                // 1. Locate NPT entry for guest_phys.
                // 2. Set NX bit to trigger a nested page fault on execution.
                // 3. On #NPF:
                //    - If ErrorCode indicates Execute: Map shadow_phys.
                //    - If ErrorCode indicates Read/Write: Map guest_phys.
            }
        }
    }
}
