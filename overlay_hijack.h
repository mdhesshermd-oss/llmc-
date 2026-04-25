#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include "obfuscation.h"

/**
 * Improved Overlay Hijacking
 * Renders ESP using GDI on an external trusted window.
 */

HWND TargetOverlay = NULL;
HDC hOverlayDC = NULL;

bool FindAndPrepareOverlay() {
    // Target: NVIDIA GeForce Overlay
    TargetOverlay = FindWindowA(XOR_STR("\x3b\x13\x1d\x39\x31\x3c\x13\x11\x1b\x23\x30\x27\x3a\x23"), NULL);

    if (TargetOverlay) {
        hOverlayDC = GetDC(TargetOverlay);
        return (hOverlayDC != NULL);
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
