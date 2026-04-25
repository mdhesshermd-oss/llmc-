#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "driver_io.h"
#include "overlay_hijack.h"
#include "scanner.h"

/**
 * Finalized Production Player ESP
 * Full logical implementation of WorldToScreen and Entity Iteration.
 */

namespace Cheat {
    namespace Features {

        struct Vector3 { float x, y, z; };
        struct Vector2 { float x, y; };
        struct Matrix4x4 { float m[4][4]; };

        class ESP {
        private:
            static constexpr const char* SIG_WORLD = "48 8B 05 ? ? ? ? 48 8B 48 08";
            static constexpr uintptr_t OFFSET_ENTITY_LIST = 0x1E88;
            static constexpr uintptr_t OFFSET_ENTITY_COUNT = 0x1E90;

            inline static uintptr_t CachedWorldPtr = 0;
            inline static bool Initialized = false;

        public:
            /**
             * Resolves GameWorld once to ensure stable performance.
             */
            static bool Initialize(uint32_t pid, uintptr_t base, size_t size) {
                if (Initialized) return true;

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
             * Standard Enfusion Engine (DayZ) math.
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

            static void Update(uint32_t pid, uintptr_t base) {
                if (!Initialized || !Driver::Init() || !Rendering::Prepare()) return;

                uintptr_t world = Driver::Read<uintptr_t>(pid, CachedWorldPtr);
                if (!world) return;

                // Read matrix and list headers
                Matrix4x4 viewMatrix = Driver::Read<Matrix4x4>(pid, world + 0x1B0);
                uintptr_t entityList = Driver::Read<uintptr_t>(pid, world + OFFSET_ENTITY_LIST);
                uint32_t count = Driver::Read<uint32_t>(pid, world + OFFSET_ENTITY_COUNT);

                if (count > 5000) return; // Fail-safe sanity check

                Rendering::StartFrame();
                for (uint32_t i = 0; i < count; i++) {
                    uintptr_t entity = Driver::Read<uintptr_t>(pid, entityList + (i * 8));
                    if (!entity) continue;

                    // Filter Human Players only (Type 1)
                    if (Driver::Read<uint32_t>(pid, entity + 0x158) != 0x1) continue;

                    Vector3 pos = Driver::Read<Vector3>(pid, entity + 0x2C0);
                    Vector2 screen;

                    // Render using the universal AMD/NVIDIA overlay system
                    if (WorldToScreen(pos, screen, viewMatrix, 1920.0f, 1080.0f)) {
                        Rendering::DrawESP(screen.x, screen.y - 50.0f, screen.y, "Player", 0.0f);
                    }
                }
                Rendering::EndFrame();
            }
        };
    }
}
