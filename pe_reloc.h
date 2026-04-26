#pragma once
#include <windows.h>
#include <stdint.h>

namespace Pe {
    /**
     * PE Relocation Fixups for Remote Process
     * Adjusts absolute addresses in the mapped image to match the new base address.
     */
    inline bool ApplyRelocations(HANDLE hProcess, void* pRemoteBase, PIMAGE_NT_HEADERS pNt, uintptr_t delta) {
        auto& relocDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.Size == 0) return true;

        // Note: In a real implementation, we would read the reloc table from the local buffer,
        // but perform the additions on the remote memory.
        // For brevity in this refactored project, we assume the caller passes the necessary pointers.

        uintptr_t currentRelocPos = (uintptr_t)pRemoteBase + relocDir.VirtualAddress;
        uint32_t bytesProcessed = 0;

        while (bytesProcessed < relocDir.Size) {
            IMAGE_BASE_RELOCATION block;
            ReadProcessMemory(hProcess, (LPCVOID)currentRelocPos, &block, sizeof(block), nullptr);

            if (block.SizeOfBlock == 0) break;

            uint32_t count = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
            std::vector<uint16_t> entries(count);
            ReadProcessMemory(hProcess, (LPCVOID)(currentRelocPos + sizeof(IMAGE_BASE_RELOCATION)), entries.data(), count * sizeof(uint16_t), nullptr);

            for (uint32_t i = 0; i < count; i++) {
                uint16_t type = entries[i] >> 12;
                uint16_t offset = entries[i] & 0xFFF;

                if (type == IMAGE_REL_BASED_DIR64) {
                    uintptr_t targetAddr = (uintptr_t)pRemoteBase + block.VirtualAddress + offset;
                    uint64_t value;
                    ReadProcessMemory(hProcess, (LPCVOID)targetAddr, &value, sizeof(value), nullptr);
                    value += delta;
                    WriteProcessMemory(hProcess, (LPVOID)targetAddr, &value, sizeof(value), nullptr);
                }
            }

            currentRelocPos += block.SizeOfBlock;
            bytesProcessed += block.SizeOfBlock;
        }
        return true;
    }
}
