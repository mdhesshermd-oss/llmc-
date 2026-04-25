#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "resource.h"

/**
 * Refactored Single-File Cheat Loader
 * This loader extracts the encrypted DLL from its own resources.
 */

// Simple zlib decompression logic stub
extern int uncompress(unsigned char* dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);

const unsigned char XOR_KEY = 0xAA;

uint8_t* UnpackPayloadFromResource() {
    // 1. Find the resource in our own EXE
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
    if (!hRes) return NULL;

    DWORD payloadSize = SizeofResource(NULL, hRes);
    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return NULL;

    uint8_t* packedData = (uint8_t*)LockResource(hData);

    // 2. Allocate and Decrypt
    uint8_t* decryptedBuffer = (uint8_t*)malloc(payloadSize);
    if (!decryptedBuffer) return NULL;

    for (DWORD i = 0; i < payloadSize; ++i) {
        decryptedBuffer[i] = packedData[i] ^ XOR_KEY;
    }

    // 3. Decompress (Assume we know the original size or extract it from metadata)
    // Original size would ideally be stored in the first few bytes of the payload
    uint32_t originalSize = *(uint32_t*)decryptedBuffer;
    uint8_t* compressedDataOffset = decryptedBuffer + sizeof(uint32_t);
    unsigned long actualCompressedSize = payloadSize - sizeof(uint32_t);

    uint8_t* finalBuffer = (uint8_t*)VirtualAlloc(NULL, originalSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!finalBuffer) {
        free(decryptedBuffer);
        return NULL;
    }

    unsigned long destLen = originalSize;
    if (uncompress(finalBuffer, &destLen, compressedDataOffset, actualCompressedSize) != 0) {
        VirtualFree(finalBuffer, 0, MEM_RELEASE);
        free(decryptedBuffer);
        return NULL;
    }

    free(decryptedBuffer);
    return finalBuffer;
}

void ApplyRelocations(uintptr_t image_base) { /* ... */ }
void ResolveImports(uintptr_t image_base) { /* ... */ }

int main() {
    uint8_t* mapped_image = UnpackPayloadFromResource();
    if (!mapped_image) {
        MessageBoxA(NULL, "Failed to unpack resources.", "Error", MB_ICONERROR);
        return -1;
    }

    ApplyRelocations((uintptr_t)mapped_image);
    ResolveImports((uintptr_t)mapped_image);

    typedef BOOL (WINAPI *DllMain_t)(HINSTANCE, DWORD, LPVOID);
    DllMain_t entry = (DllMain_t)((uintptr_t)mapped_image + 0x1000);
    entry((HINSTANCE)mapped_image, DLL_PROCESS_ATTACH, NULL);

    return 0;
}
