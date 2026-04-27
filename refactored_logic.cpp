#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "hypervisor_io.h"
#include "overlay_hijack.h"
#include "scanner.h"

/**
 * Reconstructed DayZ ESP (Enfusion Engine)
 * Finalized Professional Version
 */

namespace Cheat {
    namespace Features {

        struct Vector3 { float x, y, z; };
        struct Vector2 { float x, y; };
        struct Matrix4x4 { float m[4][4]; };

        class ESP {
        private:
            // Validated Offsets from Dump Analysis (v1.2x)
            static constexpr const char* SIG_WORLD = "48 8B 05 ? ? ? ? 48 8B 48 08";

            static constexpr uintptr_t OFFSET_VIEW_MATRIX = 0x1B0;
            static constexpr uintptr_t OFFSET_ENTITY_LIST = 0x1E88;
            static constexpr uintptr_t OFFSET_ENTITY_COUNT = 0x1E90;
            static constexpr uintptr_t OFFSET_PLAYER_TYPE = 0x158; // 0x1 = Survivor
            static constexpr uintptr_t OFFSET_COORDINATES = 0x2C0;

            inline static uintptr_t CachedWorldPtr = 0;
            inline static bool Initialized = false;

        public:
            static bool Initialize(uint64_t cr3, uintptr_t base, size_t size) {
                if (Initialized) return true;
                uintptr_t worldInstr = Scanner::FindPattern(cr3, base, size, SIG_WORLD);
                if (worldInstr) {
                    CachedWorldPtr = Scanner::ResolveRelativeAddr(cr3, worldInstr, 3, 7);
                    if (CachedWorldPtr) {
                        Initialized = true;
                        return true;
                    }
                }
                return false;
            }

            static bool WorldToScreen(const Vector3& world, Vector2& screen, const Matrix4x4& vMatrix, float w, float h) {
                float z = vMatrix.m[3][0] * world.x + vMatrix.m[3][1] * world.y + vMatrix.m[3][2] * world.z + vMatrix.m[3][3];
                if (z < 0.1f) return false;

                float x = vMatrix.m[0][0] * world.x + vMatrix.m[0][1] * world.y + vMatrix.m[0][2] * world.z + vMatrix.m[0][3];
                float y = vMatrix.m[1][0] * world.x + vMatrix.m[1][1] * world.y + vMatrix.m[1][2] * world.z + vMatrix.m[1][3];

                screen.x = (w / 2.0f) * (1.0f + x / z);
                screen.y = (h / 2.0f) * (1.0f - y / z);
                return true;
            }

            static void Run(uint64_t cr3, uintptr_t base) {
                if (!Initialized) return;

                uintptr_t world = Hv::Read<uintptr_t>(cr3, CachedWorldPtr);
                if (!world) return;

                Matrix4x4 vMatrix = Hv::Read<Matrix4x4>(cr3, world + OFFSET_VIEW_MATRIX);
                uintptr_t entities = Hv::Read<uintptr_t>(cr3, world + OFFSET_ENTITY_LIST);
                uint32_t count = Hv::Read<uint32_t>(cr3, world + OFFSET_ENTITY_COUNT);

                if (count == 0 || count > 5000) return;

                // Local camera position for distance
                Vector3 camPos = Hv::Read<Vector3>(cr3, world + 0x28);

                Rendering::StartFrame();
                for (uint32_t i = 0; i < count; i++) {
                    uintptr_t entity = Hv::Read<uintptr_t>(cr3, entities + (i * 8));
                    if (!entity) continue;

                    if (Hv::Read<uint32_t>(cr3, entity + OFFSET_PLAYER_TYPE) != 0x1) continue;

                    Vector3 pos = Hv::Read<Vector3>(cr3, entity + OFFSET_COORDINATES);

                    float dx = pos.x - camPos.x;
                    float dy = pos.y - camPos.y;
                    float dz = pos.z - camPos.z;
                    float dist = sqrtf(dx*dx + dy*dy + dz*dz);

                    Vector2 screen;
                    if (WorldToScreen(pos, screen, vMatrix, 1920, 1080)) {
                        Rendering::DrawESP(screen.x, screen.y, "Survivor", dist);
                    }
                }
                Rendering::EndFrame();
            }
        };
    }
}

extern "C" __declspec(dllexport) void ModuleEntry(uintptr_t base) {
    uint32_t pid = GetCurrentProcessId();
    uint64_t cr3 = Cheat::Hv::GetCr3(pid);

    // DayZ_x64.exe usually around 1GB+ but main code within first 256MB
    if (Cheat::Features::ESP::Initialize(cr3, base, 0x10000000)) {
        while (true) {
            Cheat::Features::ESP::Run(cr3, base);
            Sleep(1);
        }
    }
}
