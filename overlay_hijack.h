#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include "obfuscation.h"

/**
 * C++ Rendering Abstraction (Overlay Hijacking)
 */

namespace Cheat {
    namespace Rendering {

        inline HWND hOverlay = nullptr;
        inline HDC hDC = nullptr;

        inline bool Prepare() {
            if (hDC) return true;

            // XOR Encrypted Class Names
            char nv_class[] = {0x16, 0x10, 0x13, 0x78, 0x1a, 0x06, 0x16, 0x78, 0x02, 0x1c, 0x11, 0x12, 0x10, 0x01, 0x00};
            char amd_class[] = {0x14, 0x18, 0x11, 0x11, 0x03, 0x07, 0x1a, 0x1a, 0x13, 0x07, 0x19, 0x14, 0x1c, 0x0c, 0x02, 0x12, 0x11, 0x1b, 0x02, 0x00};

            hOverlay = FindWindowA(XOR_STR(nv_class), nullptr);
            if (!hOverlay) hOverlay = FindWindowA(XOR_STR(amd_class), nullptr);

            if (hOverlay) {
                hDC = GetDC(hOverlay);
                return (hDC != nullptr);
            }
            return false;
        }

        inline void DrawBox(int x, int y, int w, int h, COLORREF color) {
            if (!hDC) return;
            HPEN hPen = CreatePen(PS_SOLID, 1, color);
            auto oldPen = SelectObject(hDC, hPen);
            MoveToEx(hDC, x, y, nullptr);
            LineTo(hDC, x + w, y);
            LineTo(hDC, x + w, y + h);
            LineTo(hDC, x, y + h);
            LineTo(hDC, x, y);
            SelectObject(hDC, oldPen);
            DeleteObject(hPen);
        }

        inline void Release() {
            if (hOverlay && hDC) {
                ReleaseDC(hOverlay, hDC);
                hDC = nullptr;
            }
        }
    }
}
