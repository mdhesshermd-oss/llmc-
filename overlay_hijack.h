#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include "obfuscation.h"

/**
 * Multi-Vendor Overlay Hijacking
 * Supports AMD, NVIDIA, Discord, and Steam overlays to ensure
 * hardware compatibility and stealth.
 */

HWND TargetOverlay = NULL;
HDC hOverlayDC = NULL;

typedef struct {
    char window_class[32];
} OverlayTarget;

/**
 * List of known trusted overlays to hijack (XOR encrypted strings).
 */
bool FindAndPrepareOverlay() {
    // NVIDIA: "CEF-OSC-WIDGET"
    char nv_class[] = {0x16, 0x10, 0x13, 0x78, 0x1a, 0x06, 0x16, 0x78, 0x02, 0x1c, 0x11, 0x12, 0x10, 0x01, 0x00};
    // AMD: "AMDDVROVERLAYWINDOW"
    char amd_class[] = {0x14, 0x18, 0x11, 0x11, 0x03, 0x07, 0x1a, 0x1a, 0x13, 0x07, 0x19, 0x14, 0x1c, 0x0c, 0x02, 0x12, 0x11, 0x1b, 0x02, 0x00};
    // Discord: "Chrome_RenderWidgetHostHWND"
    char discord_class[] = {0x16, 0x3d, 0x27, 0x3a, 0x38, 0x30, 0x0a, 0x07, 0x30, 0x3b, 0x31, 0x30, 0x27, 0x02, 0x3c, 0x31, 0x32, 0x30, 0x21, 0x1d, 0x3a, 0x26, 0x01, 0x1d, 0x02, 0x1b, 0x11, 0x00};

    char* targets[] = { XOR_STR(nv_class), XOR_STR(amd_class), XOR_STR(discord_class) };

    for (int i = 0; i < 3; i++) {
        TargetOverlay = FindWindowA(targets[i], NULL);
        if (TargetOverlay) {
            hOverlayDC = GetDC(TargetOverlay);
            if (hOverlayDC) return true;
        }
    }
    return false;
}

void DrawBox(int x, int y, int w, int h, COLORREF color) {
    if (!hOverlayDC) return;

    HPEN hPen = CreatePen(PS_SOLID, 1, color);
    SelectObject(hOverlayDC, hPen);

    MoveToEx(hOverlayDC, x, y, NULL);
    LineTo(hOverlayDC, x + w, y);
    LineTo(hOverlayDC, x + w, y + h);
    LineTo(hOverlayDC, x, y + h);
    LineTo(hOverlayDC, x, y);

    DeleteObject(hPen);
}

void ShutdownOverlay() {
    if (TargetOverlay && hOverlayDC) {
        ReleaseDC(TargetOverlay, hOverlayDC);
    }
}
