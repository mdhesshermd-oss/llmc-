#pragma once
#include <windows.h>
#include <stdint.h>
#include <vector>
#include "hypervisor_io.h"

namespace Pe {
    /**
     * PE Import Resolution for Remote Process via Hypervisor
     */
    inline bool ResolveImports(uint64_t cr3, void* pRemoteBase, PIMAGE_NT_HEADERS pNt) {
        auto& importDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (importDir.Size == 0) return true;

        uintptr_t currentDescriptor = (uintptr_t)pRemoteBase + importDir.VirtualAddress;

        while (true) {
            IMAGE_IMPORT_DESCRIPTOR desc = Cheat::Hv::Read<IMAGE_IMPORT_DESCRIPTOR>(cr3, currentDescriptor);
            if (desc.Name == 0) break;

            char libName[256];
            Cheat::Hv::ReadRaw(cr3, (uintptr_t)pRemoteBase + desc.Name, libName, sizeof(libName));

            HMODULE hLocalMod = LoadLibraryA(libName);
            if (!hLocalMod) return false;

            uintptr_t thunkRef = (uintptr_t)pRemoteBase + desc.FirstThunk;
            uintptr_t originalThunkRef = (uintptr_t)pRemoteBase + desc.OriginalFirstThunk;

            while (true) {
                IMAGE_THUNK_DATA thunk = Cheat::Hv::Read<IMAGE_THUNK_DATA>(cr3, originalThunkRef);
                if (thunk.u1.AddressOfData == 0) break;

                uintptr_t funcAddr = 0;
                if (IMAGE_SNAP_BY_ORDINAL(thunk.u1.Ordinal)) {
                    funcAddr = (uintptr_t)GetProcAddress(hLocalMod, (LPCSTR)IMAGE_ORDINAL(thunk.u1.Ordinal));
                } else {
                    char funcName[256];
                    Cheat::Hv::ReadRaw(cr3, (uintptr_t)pRemoteBase + thunk.u1.AddressOfData + offsetof(IMAGE_IMPORT_BY_NAME, Name), funcName, sizeof(funcName));
                    funcAddr = (uintptr_t)GetProcAddress(hLocalMod, funcName);
                }

                if (!funcAddr) return false;

                Cheat::Hv::Write<uintptr_t>(cr3, thunkRef, funcAddr);

                thunkRef += sizeof(IMAGE_THUNK_DATA);
                originalThunkRef += sizeof(IMAGE_THUNK_DATA);
            }

            currentDescriptor += sizeof(IMAGE_IMPORT_DESCRIPTOR);
        }

        return true;
    }
}
