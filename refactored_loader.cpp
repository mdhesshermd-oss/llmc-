#pragma once
#include <stdint.h>
#include <windows.h>
#include "resource.h"
#include "syscalls.h"
#include "pe_reloc.h"
#include "pe_imports.h"

/**
 * HIGH-STEALTH C++ Manual Map Loader
 * Uses Direct Syscalls and Module Stomping to evade AC detection.
 */

namespace Cheat {
    namespace Loader {

        struct LZMAHeader {
            uint8_t props;
            uint32_t dictSize;
            uint64_t uncompressedSize;
        };

        extern "C" int LzmaUncompress(uint8_t* dest, size_t* destLen, const uint8_t* src, size_t* srcLen, const uint8_t* props, size_t propsSize);

        class ManualMapper {
        public:
            static void MapCheat() {
                InitSyscalls();

                HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
                HGLOBAL hData = LoadResource(NULL, hRes);
                size_t payloadSize = SizeofResource(NULL, hRes);
                auto packedData = static_cast<uint8_t*>(LockResource(hData));

                auto header = reinterpret_cast<LZMAHeader*>(packedData);
                size_t destLen = static_cast<size_t>(header->uncompressedSize);
                size_t srcLen = payloadSize - 13;

                /**
                 * STEALTH: MODULE STOMPING
                 * Instead of VirtualAlloc, we stomp a legitimate system DLL to hide the VAD entry.
                 */
                HMODULE hSacrificial = LoadLibraryA(XOR_STR("\x23\x30\x27\x26\x3c\x3a\x3b\x75\x31\x39\x39")); // "version.dll"
                if (!hSacrificial) return;

                PVOID base = reinterpret_cast<PVOID>(hSacrificial);
                SIZE_T regionSize = destLen;

                // 1. Change protection to RW using Direct Syscall
                ULONG oldProtect;
                DirectNtProtectVirtualMemory(GetCurrentProcess(), &base, &regionSize, PAGE_READWRITE, &oldProtect);

                // 2. Decompress Payload into stomped module
                if (LzmaUncompress(static_cast<uint8_t*>(base), &destLen, packedData + 13, &srcLen, &header->props, 5) != 0) {
                    return;
                }

                // 3. Fix PE Headers and Imports
                ApplyRelocations(static_cast<uint8_t*>(base));
                ResolveImports(static_cast<uint8_t*>(base));

                // 4. Set final protection to RX (Execute/Read) to avoid RWX detection
                DirectNtProtectVirtualMemory(GetCurrentProcess(), &base, &regionSize, PAGE_EXECUTE_READ, &oldProtect);

                // 5. Jump to Entry
                using DllMain_t = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);
                auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<uint8_t*>(base) + reinterpret_cast<PIMAGE_DOS_HEADER>(base)->e_lfanew);
                auto Entry = reinterpret_cast<DllMain_t>(static_cast<uint8_t*>(base) + nt->OptionalHeader.AddressOfEntryPoint);
                Entry(static_cast<HINSTANCE>(base), DLL_PROCESS_ATTACH, NULL);
            }
        };
    }
}
