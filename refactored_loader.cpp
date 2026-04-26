#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include <wincrypt.h>
#include "manual_map.h"
#include "hypervisor_io.h"
#include "overlay_hijack.h"
#include "preflight.h"
#include "stealth_cleanup.h"
#include "resource.h"
#include "hv_init_amd.h"

/**
 * Combat-Ready Stealth Loader
 * Implements AES-128 decryption, Hypervisor injection, and deep cleanup.
 */

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

bool DecryptPayload(std::vector<uint8_t>& data) {
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;

    // Secret AES Key (Must match encrypt_and_pack.py)
    uint8_t rawKey[16] = { 0x5A, 0x4F, 0x52, 0x4F, 0x5F, 0x44, 0x41, 0x59, 0x5A, 0x4F, 0x52, 0x4F, 0x5F, 0x44, 0x41, 0x59 };

    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
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
        memcpy(blob.rgbKeyData, rawKey, 16);

        if (CryptImportKey(hProv, (BYTE*)&blob, sizeof(blob), 0, 0, &hKey)) {
            DWORD dwDataLen = (DWORD)data.size();
            if (CryptDecrypt(hKey, 0, TRUE, 0, data.data(), &dwDataLen)) {
                data.resize(dwDataLen);
                CryptDestroyKey(hKey);
                CryptReleaseContext(hProv, 0);
                SecureZeroMemory(rawKey, 16);
                return true;
            }
            CryptDestroyKey(hKey);
        }
        CryptReleaseContext(hProv, 0);
    }
    SecureZeroMemory(rawKey, 16);
    return false;
}

int main() {
    SetConsoleTitleA("System Update Service");

    try {
        if (!Cheat::Preflight::RunAllChecks())
            throw std::runtime_error("System incompatible. Disable Hyper-V and Secure Boot.");

        // 1. Initialize Ring -1 Environment
        std::cout << "[+] Virtualizing..." << std::endl;
        if (!Cheat::Hv::AMD::InitializeSVM())
            throw std::runtime_error("Hypervisor failed to hijack the system.");

        // 2. Locate Game
        std::cout << "[*] Searching for DayZ_x64.exe..." << std::endl;
        uint32_t pid = 0;
        while (!(pid = GetProcessId("DayZ_x64.exe"))) Sleep(500);

        uint64_t cr3 = Cheat::Hv::GetCr3(pid);
        if (!cr3) throw std::runtime_error("Failed to resolve process paging base.");

        // 3. Load and Decrypt Payload
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_CHEAT_DLL), RT_RCDATA);
        if (!hRes) throw std::runtime_error("Resource not found.");

        DWORD dwSize = SizeofResource(NULL, hRes);
        void* pData = LockResource(LoadResource(NULL, hRes));
        std::vector<uint8_t> payload((uint8_t*)pData, (uint8_t*)pData + dwSize);

        if (!DecryptPayload(payload))
            throw std::runtime_error("Payload decryption failed.");

        // 4. Inject using Cloaking
        std::cout << "[+] Mapping Logic..." << std::endl;
        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        auto map = Cheat::ManualMapper::MapImage(hProc, payload);

        if (map.Success) {
            // Apply Memory Cloaking to the entry point and headers
            Cheat::Hv::CloakPage((uintptr_t)map.ImageBase, payload.data());

            // Start the cheat logic
            CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)map.EntryPoint, nullptr, 0, nullptr);

            // 5. Deep Cleanup
            std::cout << "[+] Performing Stealth Cleanup..." << std::endl;
            Cheat::Cleanup::ErasePeHeaders((uintptr_t)map.ImageBase);
            Cheat::Cleanup::DeepClean();
        }

        CloseHandle(hProc);
        std::cout << "[+] System stabilized. Execution continues in background." << std::endl;
        Sleep(2000);
        ShowWindow(GetConsoleWindow(), SW_HIDE);

    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Critical System Error", MB_ICONERROR);
        return 1;
    }

    return 0;
}
