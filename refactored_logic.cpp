#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "hypervisor_io.h" // Switched from driver_io.h
#include "overlay_hijack.h"
#include "scanner.h"

/**
 * HYPERVISOR-BASED Player ESP Logic
 * Uses Ring -1 (VMM) memory access for maximum stealth.
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

            inline static uintptr_t CachedWorldPtr = 0;
            inline static bool Initialized = false;

        public:
            static bool Initialize(uint32_t pid, uintptr_t base, size_t size) {
                if (Initialized) return true;

                // Signature scanning must now use the Hypervisor interface
                // We assume Scanner::FindPattern is updated to use Cheat::Hv::ReadVirtual
                uintptr_t worldInstr = Scanner::FindPattern(pid, base, size, SIG_WORLD);
                if (!worldInstr) return false;

                CachedWorldPtr = Scanner::ResolveRelativeAddr(pid, worldInstr, 3, 7);
                Initialized = (CachedWorldPtr != 0);
                return Initialized;
            }

            static void Update(uint32_t pid, uintptr_t base) {
                if (!Initialized || !Rendering::Prepare()) return;

                // Read via VMCALL
                uintptr_t world = Hv::ReadVirtual<uintptr_t>(pid, CachedWorldPtr);
                if (!world) return;

                Matrix4x4 vMatrix = Hv::ReadVirtual<Matrix4x4>(pid, world + 0x1B0);
                uintptr_t entityList = Hv::ReadVirtual<uintptr_t>(pid, world + OFFSET_ENTITY_LIST);
                uint32_t count = Hv::ReadVirtual<uint32_t>(pid, world + OFFSET_ENTITY_COUNT);

                if (count > 5000) return;

                Rendering::StartFrame();
                for (uint32_t i = 0; i < count; i++) {
                    uintptr_t entity = Hv::ReadVirtual<uintptr_t>(pid, entityList + (i * 8));
                    if (!entity) continue;

                    if (Hv::ReadVirtual<uint32_t>(pid, entity + 0x158) != 0x1) continue;

                    Vector3 pos = Hv::ReadVirtual<Vector3>(pid, entity + 0x2C0);
                    // ... rendering logic using overlay hijacking ...
                }
                Rendering::EndFrame();
            }
        };
    }
}
