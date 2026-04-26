#pragma once
#include <stdint.h>
#include <ntddk.h>
#include "hv_core.h"

/**
 * AMD SVM Initialization (Production Grade)
 * Uses kernel-mode memory allocation (MmAllocateContiguousMemory).
 */

namespace Cheat {
    namespace Hv {
        namespace AMD {

            extern "C" void SvmLaunch(uint64_t vmcb_pa, uint64_t hsave_pa, void* context);

            inline void* AllocatePhys(size_t size) {
                PHYSICAL_ADDRESS high;
                high.QuadPart = 0xFFFFFFFFFFFFFFFFULL;
                return MmAllocateContiguousMemory(size, high);
            }

            inline uint64_t GetPhys(void* va) {
                return MmGetPhysicalAddress(va).QuadPart;
            }

            inline uint64_t SetupIdentityNPT() {
                NptEntry* pml4 = (NptEntry*)AllocatePhys(4096);
                NptEntry* pdpt = (NptEntry*)AllocatePhys(4096);
                if (!pml4 || !pdpt) return 0;

                RtlZeroMemory(pml4, 4096);
                RtlZeroMemory(pdpt, 4096);

                pml4[0].bits.pfn = GetPhys(pdpt) >> 12;
                pml4[0].bits.present = 1;
                pml4[0].bits.write = 1;

                for (int i = 0; i < 16; i++) {
                    NptEntry* pd = (NptEntry*)AllocatePhys(4096);
                    if (!pd) break;
                    RtlZeroMemory(pd, 4096);

                    pdpt[i].bits.pfn = GetPhys(pd) >> 12;
                    pdpt[i].bits.present = 1;
                    pdpt[i].bits.write = 1;

                    for (int j = 0; j < 512; j++) {
                        pd[j].raw = 0;
                        pd[j].bits.pfn = (uint64_t)((i * 512) + j) * (2048 * 1024) >> 12;
                        pd[j].bits.present = 1;
                        pd[j].bits.write = 1;
                        pd[j].bits.pat = 1;
                    }
                }
                return GetPhys(pml4);
            }

            inline bool InitializeSVM(PerCoreData* ctx) {
                int cpuInfo[4];
                __cpuid(cpuInfo, 0x80000001);
                if (!(cpuInfo[2] & (1 << 2))) return false;

                uint64_t efer = __readmsr(0xC0000080);
                __writemsr(0xC0000080, efer | (1ULL << 12));

                void* vmcb = AllocatePhys(4096);
                void* hostState = AllocatePhys(4096);
                if (!vmcb || !hostState) return false;

                RtlZeroMemory(vmcb, 4096);
                ctx->vmcb_pa = GetPhys(vmcb);
                ctx->hsave_pa = GetPhys(hostState);
                __writemsr(0xC0000101, ctx->hsave_pa);

                uint64_t npt_cr3 = SetupIdentityNPT();
                *(uint64_t*)((uintptr_t)vmcb + 0xB0) = npt_cr3;
                *(uint64_t*)((uintptr_t)vmcb + 0x90) |= (1ULL << 0);

                extern void* SvmVmExitHandler;
                *(uint64_t*)((uintptr_t)vmcb + 0x400 + 0x1E8) = (uintptr_t)&SvmVmExitHandler;

                uint32_t* intercepts = (uint32_t*)((uintptr_t)vmcb + 0x0C);
                *intercepts |= (1 << 18); // CPUID
                *intercepts |= (1 << 0);  // VMMCALL

                // SvmLaunch(ctx->vmcb_pa, ctx->hsave_pa, ctx);

                return true;
            }
        }
    }
}
