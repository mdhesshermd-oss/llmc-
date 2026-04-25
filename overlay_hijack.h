#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include "obfuscation.h"

/**
 * Advanced Overlay Hijacking (Production Ready)
 */

namespace Cheat {
    namespace Rendering {

        inline HWND hOverlay = nullptr;
        inline HDC hDC = nullptr;

        /**
         * Enhanced search for available trusted overlays.
         */
        inline bool Prepare() {
            if (hDC) return true;

            // Updated targets for latest Radeon and GeForce Experience versions
            const char* nv_class = XOR_STR("\x16\x10\x13\x78\x1a\x06\x16\x78\x02\x1c\x11\x12\x10\x01"); // CEF-OSC-WIDGET
            const char* amd_class = XOR_STR("\x14\x18\x11\x11\x03\x07\x1a\x1a\x13\x07\x19\x14\x1c\x0c\x02\x12\x11\x1b\x02"); // AMDDVROVERLAYWINDOW

            hOverlay = FindWindowA(nv_class, nullptr);
            if (!hOverlay) hOverlay = FindWindowA(amd_class, nullptr);

            if (hOverlay) {
                // Ensure the window is visible and active
                if (!IsWindowVisible(hOverlay)) return false;

                hDC = GetDC(hOverlay);
                // Make transparent for input
                SetWindowLongPtr(hOverlay, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED);
                return (hDC != nullptr);
            }
            return false;
        }

        inline void DrawBox(int x, int y, int w, int h, COLORREF color) {
            if (!hDC) return;
            HPEN hPen = CreatePen(PS_SOLID, 1, color);
            auto old = SelectObject(hDC, hPen);
            MoveToEx(hDC, x, y, NULL);
            LineTo(hDC, x + w, y);
            LineTo(hDC, x + w, y + h);
            LineTo(hDC, x, y + h);
            LineTo(hDC, x, y);
            SelectObject(hDC, old);
            DeleteObject(hPen);
        }

        inline void Release() {
            if (hOverlay && hDC) {
                ReleaseDC(hOverlay, hDC);
                hDC = nullptr;
                hOverlay = nullptr;
            }
        }
    }
}
