#include <windows.h>
#include <stdint.h>
#include "refactored_logic.cpp"

/**
 * Core Payload (core.dll) Source
 * Fully integrated DayZ Logic.
 */

// Exported for the thread hijacker
extern "C" __declspec(dllexport) void RunESP(uintptr_t base) {
    ModuleEntry(base);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Disable thread library calls for stealth
        DisableThreadLibraryCalls(hModule);
        break;
    }
    return TRUE;
}
