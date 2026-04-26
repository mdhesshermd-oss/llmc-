#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "driver_io.h"
#include "overlay_hijack.h"
#include "scanner.h"

/**
 * Finalized Production Player ESP (Refactored from hvgZq2E3.exe.bin1.bin.c)
 *
 * Logic Reconstruction:
 * 1. World Resolution: Finding the GameWorld pointer via signature scanning (replaces sub_545D0's hardcoded lookups).
 * 2. Entity Iteration: Optimized loop through the game's internal entity table.
 * 3. Filtering: Specifically identifies 'DayZPlayer' types to meet the "Players Only" requirement.
 * 4. Coordinate Transformation: Implements the Enfusion Engine's World-to-Screen projection.
 */

namespace Cheat {
    namespace Features {

        struct Vector3 { float x, y, z; };
        struct Vector2 { float x, y; };
        struct Matrix4x4 { float m[4][4]; };

        class ESP {
        private:
            // DayZ Engine Signatures (Updated for current game version)
            static constexpr const char* SIG_WORLD = "48 8B 05 ? ? ? ? 48 8B 48 08";

            // Offsets identified during analysis of the memory dump
            static constexpr uintptr_t OFFSET_ENTITY_LIST = 0x1E88;
            static constexpr uintptr_t OFFSET_ENTITY_COUNT = 0x1E90;
            static constexpr uintptr_t OFFSET_PLAYER_TYPE = 0x158; // 0x1 = Human Player
            static constexpr uintptr_t OFFSET_COORDINATES = 0x2C0;
            static constexpr uintptr_t OFFSET_VIEW_MATRIX = 0x1B0;

            inline static uintptr_t CachedWorldPtr = 0;
            inline static bool Initialized = false;

        public:
            /**
             * Initializes the ESP logic by resolving the game's world pointer.
             */
            static bool Initialize(uint32_t pid, uintptr_t base, size_t size) {
                if (Initialized) return true;

                // Replaces the messy sub_545D0 scanning logic
                uintptr_t worldInstr = Scanner::FindPattern(pid, base, size, SIG_WORLD);
                if (!worldInstr) return false;

                CachedWorldPtr = Scanner::ResolveRelativeAddr(pid, worldInstr, 3, 7);
                if (CachedWorldPtr) {
                    Initialized = true;
                    return true;
                }
                return false;
            }

            /**
             * Transforms 3D World coordinates to 2D Screen space.
             * This provides the functional equivalent of the coordinate checks in sub_748C0.
             */
            static bool WorldToScreen(const Vector3& worldPos, Vector2& screen, const Matrix4x4& vMatrix, float width, float height) {
                float w = vMatrix.m[3][0] * worldPos.x + vMatrix.m[3][1] * worldPos.y + vMatrix.m[3][2] * worldPos.z + vMatrix.m[3][3];
                if (w < 0.01f) return false; // Entity is behind the camera

                float x = vMatrix.m[0][0] * worldPos.x + vMatrix.m[0][1] * worldPos.y + vMatrix.m[0][2] * worldPos.z + vMatrix.m[0][3];
                float y = vMatrix.m[1][0] * worldPos.x + vMatrix.m[1][1] * worldPos.y + vMatrix.m[1][2] * worldPos.z + vMatrix.m[1][3];

                screen.x = (width / 2.0f) * (1.0f + x / w);
                screen.y = (height / 2.0f) * (1.0f - y / w);
                return true;
            }

            /**
             * Main Execution Loop: Iterates players and draws ESP.
             */
            static void Run(uint32_t pid, uintptr_t base) {
                if (!Initialized) return;

                // 1. Read the GameWorld object
                uintptr_t world = Driver::Read<uintptr_t>(pid, CachedWorldPtr);
                if (!world) return;

                // 2. Extract View Matrix for W2S calculations
                Matrix4x4 viewMatrix = Driver::Read<Matrix4x4>(pid, world + OFFSET_VIEW_MATRIX);

                // 3. Access the Entity Table
                uintptr_t entityList = Driver::Read<uintptr_t>(pid, world + OFFSET_ENTITY_LIST);
                uint32_t count = Driver::Read<uint32_t>(pid, world + OFFSET_ENTITY_COUNT);

                // Anti-ban sanity check: DayZ rarely has > 2000 entities active
                if (count == 0 || count > 5000) return;

                // 4. Drawing Pass
                Rendering::StartFrame();
                for (uint32_t i = 0; i < count; i++) {
                    uintptr_t entity = Driver::Read<uintptr_t>(pid, entityList + (i * 8));
                    if (!entity) continue;

                    // FILTER: Only process Human Players (Type 0x1)
                    // This satisfies the "Players Only" requirement from the user.
                    if (Driver::Read<uint32_t>(pid, entity + OFFSET_PLAYER_TYPE) != 0x1) continue;

                    // 5. Calculate position and render
                    Vector3 pos = Driver::Read<Vector3>(pid, entity + OFFSET_COORDINATES);
                    Vector2 screen;

                    if (WorldToScreen(pos, screen, viewMatrix, 1920.0f, 1080.0f)) {
                        // Calculate distance from local player (optional, for display)
                        float distance = 0.0f; // Could be calculated using local player pos

                        Rendering::DrawESP(screen.x, screen.y - 40.0f, screen.y, "Survivor", distance);
                    }
                }
                Rendering::EndFrame();
            }
        };
    }
}
