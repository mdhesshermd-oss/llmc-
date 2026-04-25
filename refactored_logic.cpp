#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "driver_io.h"
#include "overlay_hijack.h"
#include "scanner.h"

/**
 * C++ Production ESP Logic with Sanity Checks for Ban Prevention
 */

namespace Cheat {
    namespace Features {

        struct Vector3 { float x, y, z; };

        class ESP {
        private:
            // SIGNATURES (More stable than offsets)
            // Example: 48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74 2B (World Pointer pattern)
            static constexpr const char* SIG_WORLD = "48 8B 05 ? ? ? ? 48 8B 48 08";

            static constexpr uintptr_t OFFSET_ENTITY_LIST = 0x1E88;
            static constexpr uintptr_t OFFSET_ENTITY_COUNT = 0x1E90;
            static constexpr uintptr_t OFFSET_PLAYER_POS = 0x2C0;

            inline static bool IsEmergencyShutdown = false;

        public:
            static void Update(uint32_t pid, uintptr_t base, size_t module_size) {
                if (IsEmergencyShutdown || !Driver::Init()) return;

                // 1. DYNAMIC LOOKUP
                uintptr_t world_instr = Scanner::FindPattern(pid, base, module_size, SIG_WORLD);
                if (!world_instr) {
                    // Fail-safe: If the code pattern changed, stop immediately to avoid ban
                    IsEmergencyShutdown = true;
                    return;
                }
                uintptr_t world_ptr = Scanner::ResolveRelativeAddr(pid, world_instr, 3, 7);
                uintptr_t world = Driver::Read<uintptr_t>(pid, world_ptr);

                // 2. SANITY CHECKS
                if (world < 0x10000 || world > 0x7FFFFFFFFFFF) return; // Basic pointer validation

                uintptr_t entity_list = Driver::Read<uintptr_t>(pid, world + OFFSET_ENTITY_LIST);
                uint32_t entity_count = Driver::Read<uint32_t>(pid, world + OFFSET_ENTITY_COUNT);

                // If entity count is unrealistically high, offsets are likely outdated
                if (entity_count > 5000) {
                    IsEmergencyShutdown = true;
                    return;
                }

                for (uint32_t i = 0; i < entity_count; i++) {
                    uintptr_t entity = Driver::Read<uintptr_t>(pid, entity_list + (i * 8));
                    if (entity < 0x10000) continue;

                    // Verify player type
                    uint32_t type_id = Driver::Read<uint32_t>(pid, entity + 0x158);
                    if (type_id > 100) { // Should be small IDs (1, 2, 4...)
                        IsEmergencyShutdown = true;
                        return;
                    }

                    if (type_id != 0x1) continue;

                    Vector3 pos = Driver::Read<Vector3>(pid, entity + OFFSET_PLAYER_POS);

                    // Validate position (if coords are NaN or Inf, something is wrong)
                    if (isnan(pos.x) || isinf(pos.x)) continue;

                    if (Rendering::Prepare()) {
                        // Drawing logic...
                    }
                }
            }
        };
    }
}
