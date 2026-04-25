#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "resource.h"
#include "stealth.h"
#include "obfuscation.h"
#include "syscalls.h"
#include "pe_reloc.h"
#include "pe_imports.h"

/**
 * C++ Production Loader Logic
 */

namespace Cheat {
    namespace Loader {

        // Zlib stub
        extern "C" int uncompress(unsigned char* dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);

        class ManualMapper {
        public:
            static void* MapFromResources() {
                HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
                if (!hRes) return nullptr;

                DWORD payloadSize = SizeofResource(NULL, hRes);
                HGLOBAL hData = LoadResource(NULL, hRes);
                auto packedData = static_cast<uint8_t*>(LockResource(hData));

                auto decryptedBuffer = new uint8_t[payloadSize];
                for (DWORD i = 0; i < payloadSize; ++i) decryptedBuffer[i] = packedData[i] ^ 0xAA;

                uint32_t originalSize = *reinterpret_cast<uint32_t*>(decryptedBuffer);

                // Module Stomping into version.dll
                HMODULE hTargetMod = LoadLibraryA(XOR_STR("\x23\x30\x27\x26\x3c\x3a\x3b\x75\x31\x39\x39"));
                if (!hTargetMod) {
                    delete[] decryptedBuffer;
                    return nullptr;
                }

                auto baseAddress = reinterpret_cast<void*>(hTargetMod);
                unsigned long destLen = originalSize;
                uncompress(static_cast<uint8_t*>(baseAddress), &destLen, decryptedBuffer + 4, payloadSize - 4);

                // PE Setup
                ApplyRelocations(static_cast<uint8_t*>(baseAddress));
                ResolveImports(static_cast<uint8_t*>(baseAddress));

                delete[] decryptedBuffer;
                return baseAddress;
            }
        };
    }
}

int main() {
    auto image = Cheat::Loader::ManualMapper::MapFromResources();
    if (image) {
        using DllMain_t = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);
        auto entry = reinterpret_cast<DllMain_t>(reinterpret_cast<uintptr_t>(image) + 0x1000);
        entry(static_cast<HINSTANCE>(image), DLL_PROCESS_ATTACH, nullptr);
    }
    return 0;
}
