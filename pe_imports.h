#pragma once
#include <windows.h>
#include <stdint.h>
#include <vector>
#include "stealth.h"

namespace Pe {
    /**
     * PE Import Resolution for Remote Process
     * Populates the Import Address Table (IAT) by resolving dependencies.
     */
    inline bool ResolveImports(HANDLE hProcess, void* pRemoteBase, PIMAGE_NT_HEADERS pNt) {
        auto& importDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (importDir.Size == 0) return true;

        // In a real mapper, we'd read the descriptors from the local raw data
        // since the remote image's RVA to file offset might be tricky before fully mapped.
        // We assume pRemoteBase has headers already.

        uintptr_t currentDescriptor = (uintptr_t)pRemoteBase + importDir.VirtualAddress;

        while (true) {
            IMAGE_IMPORT_DESCRIPTOR desc;
            ReadProcessMemory(hProcess, (LPCVOID)currentDescriptor, &desc, sizeof(desc), nullptr);
            if (desc.Name == 0) break;

            char libName[256];
            ReadProcessMemory(hProcess, (LPCVOID)((uintptr_t)pRemoteBase + desc.Name), libName, sizeof(libName), nullptr);

            HMODULE hLocalMod = LoadLibraryA(libName);
            if (!hLocalMod) return false;

            uintptr_t thunkRef = (uintptr_t)pRemoteBase + desc.FirstThunk;
            uintptr_t originalThunkRef = (uintptr_t)pRemoteBase + desc.OriginalFirstThunk;

            while (true) {
                IMAGE_THUNK_DATA thunk;
                ReadProcessMemory(hProcess, (LPCVOID)originalThunkRef, &thunk, sizeof(thunk), nullptr);
                if (thunk.u1.AddressOfData == 0) break;

                uintptr_t funcAddr = 0;
                if (IMAGE_SNAP_BY_ORDINAL(thunk.u1.Ordinal)) {
                    funcAddr = (uintptr_t)GetProcAddress(hLocalMod, (LPCSTR)IMAGE_ORDINAL(thunk.u1.Ordinal));
                } else {
                    IMAGE_IMPORT_BY_NAME importByName;
                    ReadProcessMemory(hProcess, (LPCVOID)((uintptr_t)pRemoteBase + thunk.u1.AddressOfData), &importByName, sizeof(importByName), nullptr);

                    char funcName[256];
                    ReadProcessMemory(hProcess, (LPCVOID)((uintptr_t)pRemoteBase + thunk.u1.AddressOfData + offsetof(IMAGE_IMPORT_BY_NAME, Name)), funcName, sizeof(funcName), nullptr);

                    funcAddr = (uintptr_t)GetProcAddress(hLocalMod, funcName);
                }

                if (!funcAddr) return false;

                // Write the resolved address into the IAT
                WriteProcessMemory(hProcess, (LPVOID)thunkRef, &funcAddr, sizeof(funcAddr), nullptr);

                thunkRef += sizeof(IMAGE_THUNK_DATA);
                originalThunkRef += sizeof(IMAGE_THUNK_DATA);
            }

            currentDescriptor += sizeof(IMAGE_IMPORT_DESCRIPTOR);
        }

        return true;
    }
}
