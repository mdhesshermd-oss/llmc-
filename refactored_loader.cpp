#include <windows.h>
#include <wincrypt.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include "resource.h"
#include "manual_map.h"
#include "hypervisor_io.h"
#include "preflight.h"

#pragma comment(lib, "crypt32.lib")

/**
 * Combat-Ready Stealth Loader
 * Finalized: Implements AES decryption with IV handling and full Hypervisor orchestration.
 */

#define IO_LAUNCH_HV CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0805, METHOD_BUFFERED, FILE_ANY_ACCESS)

uint32_t GetProcessId(const char* procName) {
    uint32_t pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(hSnap, &pe)) {
            do {
                if (!_stricmp(pe.szExeFile, procName)) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    return pid;
}

/**
 * Decrypts AES-128 payload, correctly extracting the IV from the start of the buffer.
 */
void DecryptAES128(std::vector<uint8_t>& data, const std::string& key_hex) {
    if (data.size() < 16) return;

    HCRYPTPROV hProv;
    HCRYPTKEY hKey;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return;

    struct KeyBlob {
        BLOBHEADER hdr;
        DWORD cbKeySize;
        BYTE rgbKeyData[16];
    } blob;

    blob.hdr.bType = PLAINTEXTKEYBLOB;
    blob.hdr.bVersion = CUR_BLOB_VERSION;
    blob.hdr.reserved = 0;
    blob.hdr.aiKeyAlg = CALG_AES_128;
    blob.cbKeySize = 16;

    for (int i = 0; i < 16; i++) {
        std::string byteString = key_hex.substr(i * 2, 2);
        blob.rgbKeyData[i] = (BYTE)strtol(byteString.c_str(), NULL, 16);
    }

    if (CryptImportKey(hProv, (BYTE*)&blob, sizeof(blob), 0, 0, &hKey)) {
        // First 16 bytes are the IV
        uint8_t iv[16];
        memcpy(iv, data.data(), 16);
        CryptSetKeyParam(hKey, KP_IV, iv, 0);

        // Actual ciphertext starts at offset 16
        DWORD ciphertext_size = (DWORD)data.size() - 16;
        uint8_t* ciphertext_ptr = data.data() + 16;

        if (CryptDecrypt(hKey, 0, TRUE, 0, ciphertext_ptr, &ciphertext_size)) {
            // Remove IV from the front and resize to decrypted data size
            std::vector<uint8_t> decrypted(ciphertext_ptr, ciphertext_ptr + ciphertext_size);
            data.swap(decrypted);
        }
        CryptDestroyKey(hKey);
    }
    CryptReleaseContext(hProv, 0);
}

int main() {
    SetConsoleTitleA("System Update Service");

    try {
        if (!Cheat::Preflight::RunAllChecks())
            throw std::runtime_error("System incompatible.");

        // 1. Launch Hypervisor
        HANDLE hDriver = CreateFileA("\\\\.\\DayZStealth", GENERIC_ALL, 0, 0, OPEN_EXISTING, 0, 0);
        if (hDriver != INVALID_HANDLE_VALUE) {
            DWORD ret;
            DeviceIoControl(hDriver, IO_LAUNCH_HV, 0, 0, 0, 0, &ret, 0);
            CloseHandle(hDriver);
        }

        // 2. Extract and Decrypt Payload
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
        DWORD sz = SizeofResource(NULL, hRes);
        uint8_t* pRes = (uint8_t*)LockResource(LoadResource(NULL, hRes));
        std::vector<uint8_t> encrypted_payload(pRes, pRes + sz);

        DecryptAES128(encrypted_payload, "5A4F524F5F4441595A4F524F5F444159");

        // 3. Wait for Game and resolve CR3
        uint32_t pid = 0;
        while (!(pid = GetProcessId("DayZ_x64.exe"))) Sleep(500);
        uint64_t cr3 = Cheat::Hv::GetCr3(pid);

        // 4. Manual Map via Hypervisor
        auto mapping = Cheat::ManualMapper::MapImage(cr3, encrypted_payload);

        if (mapping.Success) {
            // 5. Protected Memory Cloaking
            Cheat::Hv::CloakPage(mapping.ImageBase, encrypted_payload.data());

            // 6. Stealth Hijack
            Cheat::ManualMapper::Hijack(pid, cr3, mapping.EntryPoint);

            // 7. Cleanup
            Cheat::Hv::TriggerDeepClean();
        }

        std::cout << "[+] Setup complete." << std::endl;
        Sleep(2000);
        ShowWindow(GetConsoleWindow(), SW_HIDE);

    } catch (const std::exception& e) {
        return 1;
    }

    return 0;
}
