#pragma once
#include <stdint.h>
#include <windows.h>
#include "resource.h"
#include "pe_reloc.h"
#include "pe_imports.h"

/**
 * Single-File Cheat Loader with LZMA Decompression logic.
 * Mirrors the original 'start' function's range decoding implementation.
 */

namespace Cheat {
    namespace Loader {

        // LZMA state and probability tables based on 'start' logic
        struct RangeDecoder {
            const uint8_t* input;
            uint32_t range;
            uint32_t code;
            uint16_t probs[1846 + (768 << 4)]; // Prob tables initialized to 1024 (0.5)

            void Init(const uint8_t* data) {
                input = data;
                range = 0xFFFFFFFF;
                code = 0;
                for (int i = 0; i < 5; i++) code = (code << 8) | *input++;
                for (auto& p : probs) p = 1024;
            }

            // Implementation of the bit decoding loop found in the original code
            uint32_t DecodeBit(uint16_t& prob) {
                uint32_t bound = (range >> 11) * prob;
                if (code < bound) {
                    range = bound;
                    prob += (2048 - prob) >> 5;
                    return 0;
                } else {
                    range -= bound;
                    code -= bound;
                    prob -= prob >> 5;
                    return 1;
                }
            }
        };

        class LZMADecompressor {
        public:
            static uint8_t* Decompress(const uint8_t* source, uint32_t& outSize) {
                // 1. Read LZMA Header (Props, Dictionary Size, Uncompressed Size)
                uint8_t props = source[0];
                uint32_t dictSize = *reinterpret_cast<const uint32_t*>(&source[1]);
                uint64_t uncompressedSize = *reinterpret_cast<const uint64_t*>(&source[5]);

                RangeDecoder decoder;
                decoder.Init(source + 13); // Data starts after 13-byte header

                auto buffer = static_cast<uint8_t*>(VirtualAlloc(nullptr, uncompressedSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
                if (!buffer) return nullptr;

                // 2. LZMA Decoding Loop (simplified representation of original logic)
                // This would involve decoding literals, matches, and distances
                // exactly as the original start() function does.

                outSize = static_cast<uint32_t>(uncompressedSize);
                return buffer;
            }
        };

        class ManualMapper {
        public:
            static void* MapFromResources() {
                HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
                if (!hRes) return nullptr;

                DWORD payloadSize = SizeofResource(NULL, hRes);
                HGLOBAL hData = LoadResource(NULL, hRes);
                auto packedData = static_cast<uint8_t*>(LockResource(hData));

                uint32_t outSize = 0;
                auto baseAddress = LZMADecompressor::Decompress(packedData, outSize);
                if (!baseAddress) return nullptr;

                ApplyRelocations(baseAddress);
                ResolveImports(baseAddress);

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
