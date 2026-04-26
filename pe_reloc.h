#pragma once
#include <windows.h>
#include <stdint.h>
#include <vector>
#include "hypervisor_io.h"

namespace Pe {
    /**
     * PE Relocation Fixups for Remote Process via Hypervisor
     */
    inline bool ApplyRelocations(uint64_t cr3, void* pRemoteBase, PIMAGE_NT_HEADERS pNt, uintptr_t delta) {
        auto& relocDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.Size == 0) return true;

        uintptr_t currentRelocPos = (uintptr_t)pRemoteBase + relocDir.VirtualAddress;
        uint32_t bytesProcessed = 0;

        while (bytesProcessed < relocDir.Size) {
            IMAGE_BASE_RELOCATION block = Cheat::Hv::Read<IMAGE_BASE_RELOCATION>(cr3, currentRelocPos);

            if (block.SizeOfBlock == 0) break;

            uint32_t count = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
            std::vector<uint16_t> entries(count);
            Cheat::Hv::ReadRaw(cr3, currentRelocPos + sizeof(IMAGE_BASE_RELOCATION), entries.data(), count * sizeof(uint16_t));

            for (uint32_t i = 0; i < count; i++) {
                uint16_t type = entries[i] >> 12;
                uint16_t offset = entries[i] & 0xFFF;

                if (type == IMAGE_REL_BASED_DIR64) {
                    uintptr_t targetAddr = (uintptr_t)pRemoteBase + block.VirtualAddress + offset;
                    uint64_t value = Cheat::Hv::Read<uint64_t>(cr3, targetAddr);
                    value += delta;
                    Cheat::Hv::Write<uint64_t>(cr3, targetAddr, value);
                }
            }

            currentRelocPos += block.SizeOfBlock;
            bytesProcessed += block.SizeOfBlock;
        }
        return true;
    }
}
