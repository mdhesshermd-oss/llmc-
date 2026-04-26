#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include <wincrypt.h>
#include "manual_map.h"
#include "hypervisor_io.h"
#include "preflight.h"
#include "resource.h"

/**
 * Combat-Ready Stealth Loader (Full Combat Ready)
 * Orchestrates: Hypervisor Launch -> AES Decrypt -> Manual Map -> NPT Cloaking -> Hijack.
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

bool DecryptPayload(std::vector<uint8_t>& data) {
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;
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
            throw std::runtime_error("System incompatible. Disable Hyper-V.");

        // 1. Launch Hypervisor on all cores via Driver Bridge
        std::cout << "[+] Initializing Multi-core Hypervisor..." << std::endl;
        HANDLE hDriver = CreateFileA("\\\\.\\DayZStealth", GENERIC_ALL, 0, 0, OPEN_EXISTING, 0, 0);
        if (hDriver == INVALID_HANDLE_VALUE) throw std::runtime_error("Driver bridge not found.");

        DWORD returned;
        if (!DeviceIoControl(hDriver, IO_LAUNCH_HV, 0, 0, 0, 0, &returned, 0))
            throw std::runtime_error("Hypervisor launch failed.");
        CloseHandle(hDriver);

        // 2. Locate Game and resolve CR3
        std::cout << "[*] Waiting for DayZ_x64.exe..." << std::endl;
        uint32_t pid = 0;
        while (!(pid = GetProcessId("DayZ_x64.exe"))) Sleep(500);

        uint64_t cr3 = Cheat::Hv::GetCr3(pid);
        if (!cr3) throw std::runtime_error("Failed to resolve process paging base.");

        // 3. Load and Decrypt Cheat Logic
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_CHEAT_DLL), RT_RCDATA);
        if (!hRes) throw std::runtime_error("Payload resource missing.");

        DWORD dwSize = SizeofResource(NULL, hRes);
        void* pData = LockResource(LoadResource(NULL, hRes));
        std::vector<uint8_t> payload((uint8_t*)pData, (uint8_t*)pData + dwSize);

        if (!DecryptPayload(payload)) throw std::runtime_error("Payload decryption failed.");

        // 4. Full Manual Map via Ring -1
        std::cout << "[+] Mapping Image into Game RAM..." << std::endl;
        auto mapping = Cheat::ManualMapper::MapImage(cr3, payload);

        if (mapping.Success) {
            // 5. Activate NPT Cloaking (Shadow Pages)
            std::cout << "[+] Activating Protected Memory..." << std::endl;
            Cheat::Hv::CloakPage((uintptr_t)mapping.ImageBase, payload.data());

            // 6. Stealth Thread Hijack
            std::cout << "[+] Hijacking Game Thread for Execution..." << std::endl;
            Cheat::ManualMapper::Hijack(pid, cr3, mapping.EntryPoint);

            // 7. Ring -1 Cleanup
            std::cout << "[+] Performing Stealth Cleanup..." << std::endl;
            Cheat::Hv::TriggerDeepClean();
        }

        std::cout << "[+] Setup complete. Service running." << std::endl;
        Sleep(2000);
        ShowWindow(GetConsoleWindow(), SW_HIDE);

    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Critical System Error", MB_ICONERROR);
        return 1;
    }

    return 0;
}
