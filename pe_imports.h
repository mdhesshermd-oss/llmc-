#include <stdint.h>
#include <windows.h>
#include "stealth.h" // For GetProcAddressStealth and GetModuleHandleStealth

/**
 * PE Import Resolution
 * Populates the Import Address Table (IAT) by resolving dependencies.
 */
void ResolveImports(uint8_t* mapped_base) {
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)mapped_base;
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(mapped_base + dos_header->e_lfanew);

    PIMAGE_DATA_DIRECTORY import_dir = &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (import_dir->Size == 0) return;

    PIMAGE_IMPORT_DESCRIPTOR import_desc = (PIMAGE_IMPORT_DESCRIPTOR)(mapped_base + import_dir->VirtualAddress);

    while (import_desc->Name != 0) {
        const char* lib_name = (const char*)(mapped_base + import_desc->Name);
        HMODULE hMod = LoadLibraryA(lib_name); // Stealthier: use custom LoadLibrary if available

        if (hMod) {
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)(mapped_base + import_desc->FirstThunk);
            PIMAGE_THUNK_DATA orig_thunk = (PIMAGE_THUNK_DATA)(mapped_base + import_desc->OriginalFirstThunk);

            while (orig_thunk->u1.AddressOfData != 0) {
                if (IMAGE_SNAP_BY_ORDINAL(orig_thunk->u1.Ordinal)) {
                    thunk->u1.Function = (uintptr_t)GetProcAddress(hMod, (LPCSTR)IMAGE_ORDINAL(orig_thunk->u1.Ordinal));
                } else {
                    PIMAGE_IMPORT_BY_NAME import_by_name = (PIMAGE_IMPORT_BY_NAME)(mapped_base + orig_thunk->u1.AddressOfData);
                    // Use stealth hashing to resolve functions without using names
                    uint32_t func_hash = HashString(import_by_name->Name);
                    thunk->u1.Function = (uintptr_t)GetProcAddressStealth(hMod, func_hash);
                }
                thunk++;
                orig_thunk++;
            }
        }
        import_desc++;
    }
}
