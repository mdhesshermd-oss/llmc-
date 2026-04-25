#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <vector>
#include "driver_io.h"
#include "overlay_hijack.h"

/**
 * C++ Production ESP Logic
 */

namespace Cheat {
    namespace Features {

        struct Vector3 { float x, y, z; };

        class ESP {
        private:
            static constexpr uintptr_t OFFSET_WORLD = 0x413A238;
            static constexpr uintptr_t OFFSET_ENTITY_LIST = 0x1E88;
            static constexpr uintptr_t OFFSET_ENTITY_COUNT = 0x1E90;
            static constexpr uintptr_t OFFSET_PLAYER_POS = 0x2C0;

        public:
            static void Update(uint32_t pid, uintptr_t base) {
                if (!Driver::Init()) return;

                uintptr_t world = Driver::Read<uintptr_t>(pid, base + OFFSET_WORLD);
                uintptr_t entity_list = Driver::Read<uintptr_t>(pid, world + OFFSET_ENTITY_LIST);
                uint32_t entity_count = Driver::Read<uint32_t>(pid, world + OFFSET_ENTITY_COUNT);

                for (uint32_t i = 0; i < entity_count; i++) {
                    uintptr_t entity = Driver::Read<uintptr_t>(pid, entity_list + (i * 8));
                    if (!entity) continue;

                    // Filter: Player Only
                    uint32_t type_id = Driver::Read<uint32_t>(pid, entity + 0x158);
                    if (type_id != 0x1) continue;

                    Vector3 pos = Driver::Read<Vector3>(pid, entity + OFFSET_PLAYER_POS);

                    // Rendering Logic
                    if (Rendering::Prepare()) {
                        // Rendering::DrawBox(...)
                    }
                }
            }
        };
    }
}
