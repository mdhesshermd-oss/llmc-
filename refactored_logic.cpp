#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "hypervisor_io.h"
#include "overlay_hijack.h"
#include "scanner.h"

/**
 * Finalized Production Player ESP (Refactored from hvgZq2E3.exe.bin1.bin.c)
 *
 * Logic Reconstruction:
 * 1. World Resolution: Finding the GameWorld pointer via signature scanning.
 * 2. Entity Iteration: Optimized loop through the game's internal entity table.
 * 3. Filtering: Specifically identifies 'DayZPlayer' types.
 * 4. Coordinate Transformation: Implements the Enfusion Engine's World-to-Screen projection.
 */

namespace Cheat {
    namespace Features {

        struct Vector3 { float x, y, z; };
        struct Vector2 { float x, y; };
        struct Matrix4x4 { float m[4][4]; };

        class ESP {
        private:
            // DayZ Engine Signatures
            static constexpr const char* SIG_WORLD = "48 8B 05 ? ? ? ? 48 8B 48 08";

            static constexpr uintptr_t OFFSET_ENTITY_LIST = 0x1E88;
            static constexpr uintptr_t OFFSET_ENTITY_COUNT = 0x1E90;
            static constexpr uintptr_t OFFSET_PLAYER_TYPE = 0x158; // 0x1 = Human Player
            static constexpr uintptr_t OFFSET_COORDINATES = 0x2C0;
            static constexpr uintptr_t OFFSET_VIEW_MATRIX = 0x1B0;

            inline static uintptr_t CachedWorldPtr = 0;
            inline static bool Initialized = false;

        public:
            /**
             * Initializes the ESP logic by resolving the game's world pointer via Hypervisor.
             */
            static bool Initialize(uint64_t cr3, uintptr_t base, size_t size) {
                if (Initialized) return true;

                uintptr_t worldInstr = Scanner::FindPattern(cr3, base, size, SIG_WORLD);
                if (!worldInstr) return false;

                CachedWorldPtr = Scanner::ResolveRelativeAddr(cr3, worldInstr, 3, 7);
                if (CachedWorldPtr) {
                    Initialized = true;
                    return true;
                }
                return false;
            }

            /**
             * Transforms 3D World coordinates to 2D Screen space.
             */
            static bool WorldToScreen(const Vector3& worldPos, Vector2& screen, const Matrix4x4& vMatrix, float width, float height) {
                float w = vMatrix.m[3][0] * worldPos.x + vMatrix.m[3][1] * worldPos.y + vMatrix.m[3][2] * worldPos.z + vMatrix.m[3][3];
                if (w < 0.01f) return false;

                float x = vMatrix.m[0][0] * worldPos.x + vMatrix.m[0][1] * worldPos.y + vMatrix.m[0][2] * worldPos.z + vMatrix.m[0][3];
                float y = vMatrix.m[1][0] * worldPos.x + vMatrix.m[1][1] * worldPos.y + vMatrix.m[1][2] * worldPos.z + vMatrix.m[1][3];

                screen.x = (width / 2.0f) * (1.0f + x / w);
                screen.y = (height / 2.0f) * (1.0f - y / w);
                return true;
            }

            /**
             * Main Execution Loop: Iterates players and draws ESP via Hypercall.
             */
            static void Run(uint64_t cr3, uintptr_t base) {
                if (!Initialized) return;

                // 1. Read the GameWorld object
                uintptr_t world = Hv::Read<uintptr_t>(cr3, CachedWorldPtr);
                if (!world) return;

                // 2. Extract View Matrix
                Matrix4x4 viewMatrix = Hv::Read<Matrix4x4>(cr3, world + OFFSET_VIEW_MATRIX);

                // 3. Access the Entity Table
                uintptr_t entityList = Hv::Read<uintptr_t>(cr3, world + OFFSET_ENTITY_LIST);
                uint32_t count = Hv::Read<uint32_t>(cr3, world + OFFSET_ENTITY_COUNT);

                if (count == 0 || count > 5000) return;

                // 4. Drawing Pass
                Rendering::StartFrame();
                for (uint32_t i = 0; i < count; i++) {
                    uintptr_t entity = Hv::Read<uintptr_t>(cr3, entityList + (i * 8));
                    if (!entity) continue;

                    // FILTER: Only process Human Players (Type 0x1)
                    if (Hv::Read<uint32_t>(cr3, entity + OFFSET_PLAYER_TYPE) != 0x1) continue;

                    Vector3 pos = Hv::Read<Vector3>(cr3, entity + OFFSET_COORDINATES);
                    Vector2 screen;

                    if (WorldToScreen(pos, screen, viewMatrix, 1920.0f, 1080.0f)) {
                        Rendering::DrawESP(screen.x, screen.y - 40.0f, screen.y, "Survivor", 0.0f);
                    }
                }
                Rendering::EndFrame();
            }
        };
    }
}
