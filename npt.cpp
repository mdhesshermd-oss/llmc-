#include "vmm.h"
#include <intrin.h>

/**
 * Gbhv-style Nested Page Table (NPT) Manager
 */

namespace Npt {

    typedef union _NPT_ENTRY {
        UINT64 Raw;
        struct {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 User : 1;
            UINT64 Pwt : 1;
            UINT64 Pcd : 1;
            UINT64 Accessed : 1;
            UINT64 Dirty : 1;
            UINT64 Pat : 1; // PS bit for 2MB
            UINT64 Global : 1;
            UINT64 Avl : 3;
            UINT64 Pfn : 40;
            UINT64 Reserved : 11;
            UINT64 Nx : 1;
        } Bits;
    } NPT_ENTRY, *PNPT_ENTRY;

    inline PVOID AllocateContiguous(SIZE_T Size) {
        PHYSICAL_ADDRESS High;
        High.QuadPart = ~0ULL;
        return MmAllocateContiguousMemory(Size, High);
    }

    inline UINT64 GetPhysical(PVOID Virtual) {
        return MmGetPhysicalAddress(Virtual).QuadPart;
    }

    UINT64 GbhvNptInitializeIdentity() {
        PNPT_ENTRY Pml4 = (PNPT_ENTRY)AllocateContiguous(PAGE_SIZE);
        PNPT_ENTRY Pdpt = (PNPT_ENTRY)AllocateContiguous(PAGE_SIZE);
        if (!Pml4 || !Pdpt) return 0;

        RtlZeroMemory(Pml4, PAGE_SIZE);
        RtlZeroMemory(Pdpt, PAGE_SIZE);

        Pml4[0].Bits.Pfn = GetPhysical(Pdpt) >> 12;
        Pml4[0].Bits.Present = 1;
        Pml4[0].Bits.Write = 1;

        // Map 16GB via 2MB Huge Pages
        for (int i = 0; i < 16; i++) {
            PNPT_ENTRY Pd = (PNPT_ENTRY)AllocateContiguous(PAGE_SIZE);
            if (!Pd) break;
            RtlZeroMemory(Pd, PAGE_SIZE);

            Pdpt[i].Bits.Pfn = GetPhysical(Pd) >> 12;
            Pdpt[i].Bits.Present = 1;
            Pdpt[i].Bits.Write = 1;

            for (int j = 0; j < 512; j++) {
                Pd[j].Raw = 0;
                Pd[j].Bits.Pfn = (UINT64)((i * 512) + j) * (2048 * 1024) >> 12;
                Pd[j].Bits.Present = 1;
                Pd[j].Bits.Write = 1;
                Pd[j].Bits.Pat = 1; // Huge Page
            }
        }

        return GetPhysical(Pml4);
    }
}
