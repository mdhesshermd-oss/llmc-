#include <windows.h>
#include <iostream>
#include "overlay_hijack.h"

/**
 * Advanced Overlay Hijacking
 * Instead of raw GDI on random windows, we target specific trusted overlays
 * and use a transparent layered window if they are not found.
 */

namespace Cheat {
    namespace Rendering {

        static HWND TargetHwnd = NULL;
        static HDC TargetDC = NULL;
        static HBRUSH PlayerBrush = CreateSolidBrush(RGB(255, 0, 0));

        bool Initialize() {
            // Priority 1: Hijack trusted overlays
            TargetHwnd = FindWindowA("CEF-OSC-WIDGET", "Discord Overlay");
            if (!TargetHwnd) TargetHwnd = FindWindowA("RadeonSettings", "Radeon Settings");

            if (TargetHwnd) {
                TargetDC = GetDC(TargetHwnd);
                return true;
            }

            // Priority 2: Create a transparent top-most window
            // (Simplified for this refactor)
            return false;
        }

        void StartFrame() {
            if (!TargetDC) Initialize();
        }

        void DrawESP(float x, float y, const char* label, float dist) {
            if (!TargetDC) return;
            RECT rect = { (long)x - 20, (long)y - 20, (long)x + 20, (long)y + 20 };
            FrameRect(TargetDC, &rect, PlayerBrush);
            TextOutA(TargetDC, (int)x, (int)y + 25, label, (int)strlen(label));
        }

        void EndFrame() {
            // Optional: Release DC if needed, but for performance we keep it
        }
    }
}
