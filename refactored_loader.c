#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "resource.h"
#include "stealth.h"
#include "obfuscation.h"

/**
 * Enhanced Single-File Cheat Loader with Stealth Features
 */

// Function pointer for VirtualAlloc resolved via stealth hashing
typedef LPVOID (WINAPI *VirtualAlloc_t)(LPVOID, SIZE_T, DWORD, DWORD);
extern int uncompress(unsigned char* dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);

uint8_t* UnpackPayloadFromResource() {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
    if (!hRes) return NULL;

    DWORD payloadSize = SizeofResource(NULL, hRes);
    HGLOBAL hData = LoadResource(NULL, hRes);
    uint8_t* packedData = (uint8_t*)LockResource(hData);

    uint8_t* decryptedBuffer = (uint8_t*)malloc(payloadSize);
    for (DWORD i = 0; i < payloadSize; ++i) {
        decryptedBuffer[i] = packedData[i] ^ 0xAA;
    }

    uint32_t originalSize = *(uint32_t*)decryptedBuffer;
    uint8_t* compressedData = decryptedBuffer + 4;

    // Use Stealth API Resolution for memory allocation
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    VirtualAlloc_t pVirtualAlloc = (VirtualAlloc_t)GetProcAddressStealth(hKernel32, HASH_VIRTUALALLOC);

    uint8_t* finalBuffer = (uint8_t*)pVirtualAlloc(NULL, originalSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    unsigned long destLen = originalSize;
    uncompress(finalBuffer, &destLen, compressedData, payloadSize - 4);

    free(decryptedBuffer);
    return finalBuffer;
}

int main() {
    // Obfuscated error message
    char errMsg[] = {0x13, 0x34, 0x3c, 0x39, 0x30, 0x31, 0x00}; // "Failed" ^ 0x55

    uint8_t* mapped_image = UnpackPayloadFromResource();
    if (!mapped_image) {
        MessageBoxA(NULL, XOR_STR(errMsg), "!", MB_ICONERROR);
        return -1;
    }

    // Module Stomping or Manual Map entry...
    typedef BOOL (WINAPI *DllMain_t)(HINSTANCE, DWORD, LPVOID);
    DllMain_t entry = (DllMain_t)((uintptr_t)mapped_image + 0x1000);
    entry((HINSTANCE)mapped_image, DLL_PROCESS_ATTACH, NULL);

    return 0;
}
