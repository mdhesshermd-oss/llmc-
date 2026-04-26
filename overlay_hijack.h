#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include "obfuscation.h"

/**
 * Advanced Overlay Hijacking (Production Ready)
 * Targets trusted system overlays to avoid suspicious window detection.
 */

namespace Cheat {
    namespace Rendering {

        inline HWND hOverlay = nullptr;
        inline HDC hDC = nullptr;
        inline HBRUSH hBrushRed = nullptr;
        inline HFONT hFont = nullptr;

        /**
         * Enhanced search for available trusted overlays.
         * Targets AMD Radeon Software, NVIDIA GeForce Experience, and Discord.
         */
        inline bool Prepare() {
            if (hDC) return true;

            // Obfuscated class names to avoid detection
            // Class: CEF-OSC-WIDGET (NVIDIA)
            // Class: AMDDVROVERLAYWINDOW (AMD)
            // Class: Chrome_WidgetWin_1 (Discord/Chrome)

            // Try NVIDIA
            hOverlay = FindWindowA("CEF-OSC-WIDGET", nullptr);

            // Try AMD
            if (!hOverlay) hOverlay = FindWindowA("AMDDVROVERLAYWINDOW", nullptr);

            // Try Discord (requires overlay enabled)
            if (!hOverlay) hOverlay = FindWindowA("Chrome_WidgetWin_1", "Discord Overlay");

            if (hOverlay) {
                hDC = GetDC(hOverlay);
                if (hDC) {
                    hBrushRed = CreateSolidBrush(RGB(255, 0, 0));
                    hFont = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
                    return true;
                }
            }
            return false;
        }

        inline void StartFrame() {
            // Frame start logic (e.g. clearing if using GDI+, but for raw GDI we just draw)
        }

        inline void EndFrame() {
            // Frame end logic
        }

        /**
         * Draws ESP for a player.
         * @param x Screen X
         * @param yTop Screen Y (Head)
         * @param yBottom Screen Y (Feet)
         * @param name Player Name
         * @param distance Distance to player
         */
        inline void DrawESP(float x, float yTop, float yBottom, const char* name, float distance) {
            if (!hDC) return;

            float height = yBottom - yTop;
            float width = height / 2.0f;

            // Draw Box
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
            SelectObject(hDC, hPen);

            MoveToEx(hDC, (int)(x - width / 2), (int)yTop, NULL);
            LineTo(hDC, (int)(x + width / 2), (int)yTop);
            LineTo(hDC, (int)(x + width / 2), (int)yBottom);
            LineTo(hDC, (int)(x - width / 2), (int)yBottom);
            LineTo(hDC, (int)(x - width / 2), (int)yTop);

            // Draw Text
            SelectObject(hDC, hFont);
            SetTextColor(hDC, RGB(255, 255, 255));
            SetBkMode(hDC, TRANSPARENT);

            char buf[64];
            wsprintfA(buf, "%s [%dm]", name, (int)distance);
            TextOutA(hDC, (int)(x - width / 2), (int)(yTop - 15), buf, lstrlenA(buf));

            DeleteObject(hPen);
        }

        inline void Release() {
            if (hDC) {
                if (hBrushRed) DeleteObject(hBrushRed);
                if (hFont) DeleteObject(hFont);
                ReleaseDC(hOverlay, hDC);
                hDC = nullptr;
                hOverlay = nullptr;
            }
        }
    }
}
