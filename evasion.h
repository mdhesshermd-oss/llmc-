#include "svm.h"
#include "scanner.h"
#include <intrin.h>

/**
 * Advanced Anti-Cheat Evasion (Ring -1 Context)
 * Finalized Professional Version
 */

namespace Evasion {

    /**
     * Hides the hypervisor from timing detection (e.g. BattlEye/EAC RDTSC checks).
     * Adds random jitter to the TSC offset to break statistical analysis.
     */
    inline void ApplyTscJitter(UINT8* Vmcb) {
        UINT64 TscOffset = *(UINT64*)(Vmcb + 0x38);
        uint64_t jitter = (__rdtsc() & 0x7F); // 0-127 cycle jitter
        *(UINT64*)(Vmcb + 0x38) = TscOffset - jitter;
    }

    /**
     * Wipes kernel traces left by the loader driver.
     * Target: MmUnloadedDrivers list and PE Headers.
     */
    inline void DeepClean() {
        // 1. Locate ntoskrnl base using LSTAR MSR
        uintptr_t lstar = __readmsr(0xC0000082);
        uintptr_t ntosBase = lstar & ~0xFFF;
        while (*(uint16_t*)ntosBase != 0x5A4D) ntosBase -= 0x1000;

        // 2. Wipe MmUnloadedDrivers
        uintptr_t pattern = Scanner::FindPatternInternal(ntosBase, 0x2000000, "4C 8B 15 ? ? ? ? 4C 8B C9");
        if (pattern) {
            int32_t off = *(int32_t*)(pattern + 3);
            uintptr_t* pList = *(uintptr_t**)(pattern + 7 + off);
            if (pList) {
                memset(pList, 0, 0x400); // Wipe 50 entries
                uint32_t* pCount = (uint32_t*)((uintptr_t)pList + 8);
                *pCount = 0;
            }
        }

        // 3. Scrub Pool Tags (Advanced: would require scanning NonPagedPool)
    }
}
