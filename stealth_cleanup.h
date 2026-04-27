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

#ifndef _USER_MODE
        /**
         * Locates the base address of ntoskrnl.exe from Ring -1 or Ring 0.
         * Scans memory below LSTAR MSR (KiSystemCall64).
         * Only available in Kernel Mode.
         */
        inline uintptr_t FindNtoskrnlBase() {
            uintptr_t lstar = __readmsr(0xC0000082);
            uintptr_t ptr = lstar & ~0xFFF;

            // Search for MZ signature
            for (int i = 0; i < 1024; i++) {
                // Safely check for MZ signature (in hypervisor we would use safe mapping)
                // For this cleanup, we assume the pointer is valid if we reached here.
                if (*(uint16_t*)ptr == 0x5A4D) return ptr;
                ptr -= 0x1000;
            }
            return 0;
        }

        /**
         * Completely wipes the MmUnloadedDrivers list.
         * Only available in Kernel Mode.
         */
        inline void WipeMmUnloadedDrivers() {
            uintptr_t ntosBase = FindNtoskrnlBase();
            if (!ntosBase) return;

            uintptr_t patternAddr = Scanner::FindPatternInternal(ntosBase, 0x2000000, "4C 8B 15 ? ? ? ? 4C 8B C9");
            if (patternAddr) {
                int32_t offset = *(int32_t*)(patternAddr + 3);
                uintptr_t* pMmUnloadedDrivers = *(uintptr_t**)(patternAddr + 7 + offset);

                if (pMmUnloadedDrivers) {
                    memset(pMmUnloadedDrivers, 0, 0x400);
                    uint32_t* pMmLastUnloadedDriver = (uint32_t*)((uintptr_t)pMmUnloadedDrivers + 8);
                    if (pMmLastUnloadedDriver) *pMmLastUnloadedDriver = 0;
                }
            }
        }

        inline void DeepClean() {
            WipeMmUnloadedDrivers();
        }
#endif

        /**
         * Erases the PE headers of the mapped module in the target process.
         * Uses hypercalls to wipe memory remotely and stealthily.
         * Available in both User and Kernel mode.
         */
        inline void ErasePeHeaders(uint64_t cr3, uintptr_t base) {
            uint8_t zeroPage[4096] = { 0 };
            Hv::WriteRaw(cr3, base, zeroPage, 4096);
        }
    }
}
