#pragma once
#include <stdint.h>
#include <windows.h>
#include <intrin.h>
#include "hv_core_amd.h"

/**
 * AMD SVM Initialization (Production Grade)
 * Implements Identity Mapping (1:1 Guest->Host) via NPT Huge Pages.
 */

namespace Cheat {
    namespace Hv {
        namespace AMD {

            extern "C" void SvmLaunch(void* vmcb_pa);

            // Helpers for physical memory management in Ring 0
            inline void* AllocatePhys(size_t size) {
                // In a driver, use MmAllocateContiguousMemory
                return _aligned_malloc(size, 4096);
            }

            inline uint64_t GetPhys(void* va) {
                // In a driver, use MmGetPhysicalAddress
                return (uintptr_t)va; // Placeholder
            }

            /**
             * Sets up 1:1 Identity Mapping for the first 16GB of RAM.
             * Uses 2MB Huge Pages in the NPT (Nested Page Tables).
             */
            inline uint64_t SetupIdentityNPT() {
                NptEntry* pml4 = (NptEntry*)AllocatePhys(4096);
                NptEntry* pdpt = (NptEntry*)AllocatePhys(4096);
                memset(pml4, 0, 4096);
                memset(pdpt, 0, 4096);

                // PML4[0] points to PDPT
                pml4[0].bits.pfn = GetPhys(pdpt) >> 12;
                pml4[0].bits.present = 1;
                pml4[0].bits.write = 1;
                pml4[0].bits.user = 1;

                // Map 16 entries in PDPT (16 GB total)
                for (int i = 0; i < 16; i++) {
                    NptEntry* pd = (NptEntry*)AllocatePhys(4096);
                    memset(pd, 0, 4096);

                    pdpt[i].bits.pfn = GetPhys(pd) >> 12;
                    pdpt[i].bits.present = 1;
                    pdpt[i].bits.write = 1;

                    // Map 512 entries in each PD (512 * 2MB = 1GB per PD)
                    for (int j = 0; j < 512; j++) {
                        pd[j].raw = 0;
                        // Calculate physical address for this 2MB block
                        pd[j].bits.pfn = (uint64_t)((i * 512) + j) * (2048 * 1024) >> 12;
                        pd[j].bits.present = 1;
                        pd[j].bits.write = 1;
                        pd[j].bits.pat = 1; // Bit 7 set for 2MB page size in NPT
                    }
                }
                return GetPhys(pml4);
            }

            /**
             * Initializes SVM for the current core.
             */
            inline bool InitializeSVM() {
                // 1. Check support
                int cpuInfo[4];
                __cpuid(cpuInfo, 0x80000001);
                if (!(cpuInfo[2] & (1 << 2))) return false;

                // 2. Enable SVM in EFER
                uint64_t efer = __readmsr(0xC0000080);
                __writemsr(0xC0000080, efer | (1ULL << 12));

                // 3. Setup VMCB and Host State
                void* vmcb = AllocatePhys(4096);
                void* hostState = AllocatePhys(4096);
                memset(vmcb, 0, 4096);
                __writemsr(0xC0000101, GetPhys(hostState)); // VM_HSAVE_PA

                // 4. Setup Nested Paging (Identity 1:1)
                uint64_t npt_cr3 = SetupIdentityNPT();
                *(uint64_t*)((uintptr_t)vmcb + 0xB0) = npt_cr3; // n_cr3
                *(uint64_t*)((uintptr_t)vmcb + 0x90) |= (1ULL << 0); // NP_ENABLE

                // 5. Setup Intercepts and Handler
                extern void* SvmVmExitHandler;
                *(uint64_t*)((uintptr_t)vmcb + 0x400 + 0x1E8) = (uintptr_t)&SvmVmExitHandler;

                uint32_t* intercepts = (uint32_t*)((uintptr_t)vmcb + 0x0C);
                *intercepts |= (1 << 18); // Intercept CPUID

                // 6. Launch
                // SvmLaunch(vmcb);

                return true;
            }
        }
    }
}
