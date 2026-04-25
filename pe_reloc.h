#include <stdint.h>
#include <windows.h>

/**
 * PE Relocation Fixups
 * Adjusts absolute addresses in the mapped image to match the new base address.
 */
void ApplyRelocations(uint8_t* mapped_base) {
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)mapped_base;
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(mapped_base + dos_header->e_lfanew);

    // Calculate the delta between the preferred base and the current mapped base
    uintptr_t delta = (uintptr_t)mapped_base - (uintptr_t)nt_headers->OptionalHeader.ImageBase;

    if (delta == 0) return; // No relocation needed

    PIMAGE_DATA_DIRECTORY reloc_dir = &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (reloc_dir->Size == 0) return;

    PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)(mapped_base + reloc_dir->VirtualAddress);

    while (reloc->VirtualAddress != 0) {
        uint32_t count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
        uint16_t* list = (uint16_t*)(reloc + 1);

        for (uint32_t i = 0; i < count; i++) {
            uint16_t type = list[i] >> 12;
            uint16_t offset = list[i] & 0xFFF;

            if (type == IMAGE_REL_BASED_DIR64) {
                uint64_t* ptr = (uint64_t*)(mapped_base + reloc->VirtualAddress + offset);
                *ptr += delta;
            }
        }
        reloc = (PIMAGE_BASE_RELOCATION)((uint8_t*)reloc + reloc->SizeOfBlock);
    }
}
