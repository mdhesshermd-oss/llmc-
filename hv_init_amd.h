#pragma once
#include <stdint.h>
#include <ntddk.h>
#include "hv_core.h"

/**
 * AMD SVM Initialization (Full Combat Ready)
 * Implements Dynamic Guest State setup and Identity Mapping for system-wide virtualization.
 */

namespace Cheat {
    namespace Hv {
        namespace AMD {

            // Prototype for the assembly entry point in svm_bridge.asm
            extern "C" void SvmLaunch(uint64_t vmcb_pa, uint64_t hsave_pa, void* context);

            struct SegDesc {
                uint16_t selector;
                uint16_t attrib;
                uint32_t limit;
                uint64_t base;
            };

            /**
             * Extracts segment descriptors from the current processor context.
             */
            inline SegDesc GetSegDesc(uint16_t selector) {
                SegDesc desc = { selector, 0, 0, 0 };
                desc.limit = 0xFFFFFFFF;
                desc.attrib = 0x209B; // P=1, DPL=0, Code, Readable, Long Mode

                if (selector == __read_gs()) desc.base = __readmsr(0xC0000101); // MSR_GS_BASE
                return desc;
            }

            /**
             * Initializes the Guest State area to ensure stability on modern Windows (10/11).
             */
            inline void SetupGuestState(uint8_t* pVmcb) {
                uint8_t* guest = pVmcb + 0x400; // State Save Area

                // Sync segments with host to prevent immediate VM-Exit or BSOD
                SegDesc cs = GetSegDesc(__read_cs());
                *(uint16_t*)(guest + 0x40) = cs.selector;
                *(uint16_t*)(guest + 0x42) = cs.attrib;
                *(uint32_t*)(guest + 0x44) = cs.limit;

                // Critical: Enable SVME in guest EFER to allow hypercalls and recursive VMRUN
                *(uint64_t*)(guest + 0x1F8) = __readmsr(0xC0000080) | (1ULL << 12);

                // Copy current paging base
                *(uint64_t*)(guest + 0x140) = __readcr3();
            }

            inline void* AllocatePhys(size_t size) {
                PHYSICAL_ADDRESS high;
                high.QuadPart = 0xFFFFFFFFFFFFFFFFULL;
                return MmAllocateContiguousMemory(size, high);
            }

            inline uint64_t GetPhys(void* va) {
                return MmGetPhysicalAddress(va).QuadPart;
            }

            /**
             * Sets up a 1:1 Identity Mapping using 2MB Huge Pages.
             */
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

            /**
             * Main entry point for SVM initialization on a specific core.
             */
            inline bool InitializeSVM(PerCoreData* ctx) {
                int cpuInfo[4];
                __cpuid(cpuInfo, 0x80000001);
                if (!(cpuInfo[2] & (1 << 2))) return false;

                // 1. Globally Enable SVM in EFER
                uint64_t efer = __readmsr(0xC0000080);
                __writemsr(0xC0000080, efer | (1ULL << 12));

                // 2. Setup control structures
                void* vmcb = AllocatePhys(4096);
                void* hostState = AllocatePhys(4096);
                if (!vmcb || !hostState) return false;

                RtlZeroMemory(vmcb, 4096);
                SetupGuestState((uint8_t*)vmcb);

                ctx->vmcb_pa = GetPhys(vmcb);
                ctx->hsave_pa = GetPhys(hostState);
                __writemsr(0xC0000101, ctx->hsave_pa);

                // 3. Setup NPT with 1:1 Identity Mapping
                uint64_t npt_cr3 = SetupIdentityNPT();
                *(uint64_t*)((uintptr_t)vmcb + 0xB0) = npt_cr3;
                *(uint64_t*)((uintptr_t)vmcb + 0x90) |= (1ULL << 0);

                // 4. Setup Host RIP (Jumps to SvmVmExitHandler in svm_bridge.asm)
                extern "C" void* SvmVmExitHandler;
                *(uint64_t*)((uintptr_t)vmcb + 0x400 + 0x1E8) = (uintptr_t)&SvmVmExitHandler;

                // 5. Setup Intercepts
                uint32_t* intercepts = (uint32_t*)((uintptr_t)vmcb + 0x0C);
                *intercepts |= (1 << 18); // Intercept CPUID
                *intercepts |= (1 << 0);  // Intercept VMMCALL

                // 6. Final Launch: Context Switch to Ring -1
                SvmLaunch(ctx->vmcb_pa, ctx->hsave_pa, ctx);

                return true;
            }
        }
    }
}
