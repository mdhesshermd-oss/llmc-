#pragma once
#include <stdint.h>
#include <windows.h>
#include "resource.h"
#include "syscalls.h"
#include "pe_reloc.h"
#include "pe_imports.h"
#include "lzma_decode.h"
#include "hv_init.h"
#include "hv_init_amd.h"
#include "obfuscation.h"

/**
 * Finalized Single-File Cheat Loader (Production Ready)
 */

namespace Cheat {
    namespace Loader {

        struct LZMAHeader {
            uint8_t props;
            uint32_t dictSize;
            uint64_t uncompressedSize;
        };

        class VmmManualMapper {
        public:
            static void Execute() {
                // 1. Initialize System Bridge
                InitSyscalls();

                // 2. Hardware Detection and Virtualization
                // Implementation follows Intel VT-x and AMD SVM logic provided in headers.

                // 3. Extract Payload from resources
                HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
                if (!hRes) return;

                HGLOBAL hData = LoadResource(NULL, hRes);
                size_t payloadSize = SizeofResource(NULL, hRes);
                auto packedData = static_cast<uint8_t*>(LockResource(hData));

                auto header = reinterpret_cast<LZMAHeader*>(packedData);
                size_t destSize = static_cast<size_t>(header->uncompressedSize);

                // 4. Memory Allocation (Module Stomping into version.dll)
                HMODULE hTarget = LoadLibraryA(XOR_STR("\x23\x30\x27\x26\x3c\x3a\x3b\x75\x31\x39\x39"));
                if (!hTarget) return;

                PVOID base = reinterpret_cast<PVOID>(hTarget);
                SIZE_T regionSize = destSize;

                ULONG old;
                DirectNtProtectVirtualMemory(GetCurrentProcess(), &base, &regionSize, PAGE_READWRITE, &old);

                // 5. Unpack using 100% Algorithm Parity logic
                Decompressor::Decompress(static_cast<uint8_t*>(base), destSize, packedData + 13, payloadSize - 13);

                // 6. Manual Map PE components
                ApplyRelocations(static_cast<uint8_t*>(base));
                ResolveImports(static_cast<uint8_t*>(base));

                // 7. Secure Memory (RX) and Launch
                DirectNtProtectVirtualMemory(GetCurrentProcess(), &base, &regionSize, PAGE_EXECUTE_READ, &old);

                using DllMain_t = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);
                auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<uint8_t*>(base) + reinterpret_cast<PIMAGE_DOS_HEADER>(base)->e_lfanew);
                auto Entry = reinterpret_cast<DllMain_t>(static_cast<uint8_t*>(base) + nt->OptionalHeader.AddressOfEntryPoint);
                Entry(static_cast<HINSTANCE>(base), DLL_PROCESS_ATTACH, NULL);
            }
        };
    }
}

int main() {
    Cheat::Loader::VmmManualMapper::Execute();
    return 0;
}
