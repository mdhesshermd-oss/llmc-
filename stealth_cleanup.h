#pragma once
#include <windows.h>
#include <winternl.h>
#include "scanner.h"

/**
 * Advanced Stealth & Cleanup Module
 * Responsible for wiping all traces of the cheat and the loader from the kernel.
 */

namespace Cheat {
    namespace Cleanup {

        /**
         * Locates the base address of ntoskrnl.exe from Ring -1 or Ring 0.
         * Scans memory below LSTAR MSR (KiSystemCall64).
         */
        inline uintptr_t FindNtoskrnlBase() {
            uintptr_t lstar = __readmsr(0xC0000082);
            uintptr_t ptr = lstar & ~0xFFF;

            // Search for MZ signature
            for (int i = 0; i < 1024; i++) {
                if (*(uint16_t*)ptr == 0x5A4D) return ptr;
                ptr -= 0x1000;
            }
            return 0;
        }

        /**
         * Completely wipes the MmUnloadedDrivers list.
         * This list is frequently checked by anti-cheats (BattlEye/EAC).
         */
        inline void WipeMmUnloadedDrivers() {
            uintptr_t ntosBase = FindNtoskrnlBase();
            if (!ntosBase) return;

            // Pattern for MmUnloadedDrivers list access
            uintptr_t patternAddr = Scanner::FindPatternInternal(ntosBase, 0x2000000, "4C 8B 15 ? ? ? ? 4C 8B C9");
            if (patternAddr) {
                // Calculate RIP-relative address
                int32_t offset = *(int32_t*)(patternAddr + 3);
                uintptr_t* pMmUnloadedDrivers = *(uintptr_t**)(patternAddr + 7 + offset);

                if (pMmUnloadedDrivers) {
                    // Zero out the first 50 entries
                    memset(pMmUnloadedDrivers, 0, 0x400);

                    // Reset the counter
                    uint32_t* pMmLastUnloadedDriver = (uint32_t*)((uintptr_t)pMmUnloadedDrivers + 8);
                    if (pMmLastUnloadedDriver) *pMmLastUnloadedDriver = 0;
                }
            }
        }

        /**
         * Erases the PE headers of the mapped module in the target process.
         */
        inline void ErasePeHeaders(uintptr_t base) {
            DWORD old;
            VirtualProtect((void*)base, 0x1000, PAGE_READWRITE, &old);
            SecureZeroMemory((void*)base, 0x1000);
            VirtualProtect((void*)base, 0x1000, old, &old);
        }

        inline void DeepClean() {
            WipeMmUnloadedDrivers();
            // Additional cleanup (Pool Tag Scanning, ShimCache wiping) goes here
        }
    }
}
