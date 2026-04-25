#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "driver_io.h"
#include "overlay_hijack.h"
#include "scanner.h"

/**
 * OPTIMIZED C++ Player ESP
 * Address caching prevents performance bottlenecks from repeated AOB scanning.
 */

namespace Cheat {
    namespace Features {

        struct Vector3 { float x, y, z; };
        struct Matrix4x4 { float m[4][4]; };

        class ESP {
        private:
            static constexpr const char* SIG_WORLD = "48 8B 05 ? ? ? ? 48 8B 48 08";
            static constexpr uintptr_t OFFSET_ENTITY_LIST = 0x1E88;
            static constexpr uintptr_t OFFSET_ENTITY_COUNT = 0x1E90;

            // Cached Addresses
            inline static uintptr_t CachedWorldPtr = 0;
            inline static bool Initialized = false;

        public:
            /**
             * Performs expensive signature scanning only once.
             */
            static bool Initialize(uint32_t pid, uintptr_t base, size_t size) {
                if (Initialized) return true;

                uintptr_t worldInstr = Scanner::FindPattern(pid, base, size, SIG_WORLD);
                if (!worldInstr) return false;

                CachedWorldPtr = Scanner::ResolveRelativeAddr(pid, worldInstr, 3, 7);
                Initialized = true;
                return true;
            }

            static bool WorldToScreen(Vector3 pos, ImVec2& screen, Matrix4x4 viewMatrix, float width, float height) {
                float w = viewMatrix.m[3][0] * pos.x + viewMatrix.m[3][1] * pos.y + viewMatrix.m[3][2] * pos.z + viewMatrix.m[3][3];
                if (w < 0.01f) return false;

                float x = viewMatrix.m[0][0] * pos.x + viewMatrix.m[0][1] * pos.y + viewMatrix.m[0][2] * pos.z + viewMatrix.m[0][3];
                float y = viewMatrix.m[1][0] * pos.x + viewMatrix.m[1][1] * pos.y + viewMatrix.m[1][2] * pos.z + viewMatrix.m[1][3];

                screen.x = (width / 2) * (1 + x / w);
                screen.y = (height / 2) * (1 - y / w);
                return true;
            }

            /**
             * Fast Frame Update: uses cached addresses.
             */
            static void Update(uint32_t pid, uintptr_t base) {
                if (!Initialized || !Driver::Init() || !Rendering::Init()) return;

                uintptr_t world = Driver::Read<uintptr_t>(pid, CachedWorldPtr);
                if (!world) return;

                Matrix4x4 viewMatrix = Driver::Read<Matrix4x4>(pid, world + 0x1B0);
                uintptr_t entity_list = Driver::Read<uintptr_t>(pid, world + OFFSET_ENTITY_LIST);
                uint32_t entity_count = Driver::Read<uint32_t>(pid, world + OFFSET_ENTITY_COUNT);

                if (entity_count > 5000) return; // Sanity check

                Rendering::StartFrame();
                for (uint32_t i = 0; i < entity_count; i++) {
                    uintptr_t entity = Driver::Read<uintptr_t>(pid, entity_list + (i * 8));
                    if (!entity) continue;

                    // Filter only Players
                    if (Driver::Read<uint32_t>(pid, entity + 0x158) != 0x1) continue;

                    Vector3 pos = Driver::Read<Vector3>(pid, entity + 0x2C0);
                    ImVec2 screen;
                    if (WorldToScreen(pos, screen, viewMatrix, 1920.0f, 1080.0f)) {
                        Rendering::DrawESP(screen.x, screen.y - 50, screen.y, "Player", 100.0f);
                    }
                }
                Rendering::EndFrame();
            }
        };
    }
}
