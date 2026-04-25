#pragma once
#include <stdint.h>
#include <windows.h>
#include "resource.h"
#include "syscalls.h"
#include "pe_reloc.h"
#include "pe_imports.h"
#include "lzma_decode.h"

/**
 * Finalized Production Single-File Cheat Loader
 */

namespace Cheat {
    namespace Loader {

        struct LZMAHeader {
            uint8_t props;
            uint32_t dictSize;
            uint64_t uncompressedSize;
        };

        class ManualMapper {
        public:
            static void MapCheat() {
                InitSyscalls();

                HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
                HGLOBAL hData = LoadResource(NULL, hRes);
                size_t payloadSize = SizeofResource(NULL, hRes);
                auto packedData = static_cast<uint8_t*>(LockResource(hData));

                auto header = reinterpret_cast<LZMAHeader*>(packedData);
                size_t destSize = static_cast<size_t>(header->uncompressedSize);

                // Stealth Module Stomping
                HMODULE hStomp = LoadLibraryA(XOR_STR("\x23\x30\x27\x26\x3c\x3a\x3b\x75\x31\x39\x39"));
                if (!hStomp) return;

                PVOID base = reinterpret_cast<PVOID>(hStomp);
                SIZE_T regionSize = destSize;

                ULONG oldProtect;
                DirectNtProtectVirtualMemory(GetCurrentProcess(), &base, &regionSize, PAGE_READWRITE, &oldProtect);

                // Actual Decompression (Algorithm Parity with original start())
                Decompressor::LzmaUncompressFunctional(static_cast<uint8_t*>(base), destSize, packedData + 13, payloadSize - 13);

                ApplyRelocations(static_cast<uint8_t*>(base));
                ResolveImports(static_cast<uint8_t*>(base));

                DirectNtProtectVirtualMemory(GetCurrentProcess(), &base, &regionSize, PAGE_EXECUTE_READ, &oldProtect);

                using DllMain_t = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);
                auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<uint8_t*>(base) + reinterpret_cast<PIMAGE_DOS_HEADER>(base)->e_lfanew);
                auto Entry = reinterpret_cast<DllMain_t>(static_cast<uint8_t*>(base) + nt->OptionalHeader.AddressOfEntryPoint);
                Entry(static_cast<HINSTANCE>(base), DLL_PROCESS_ATTACH, NULL);
            }
        };
    }
}
