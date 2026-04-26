#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include "vmm.h"
#include "hypervisor_io.h"
#include "manual_map.h"
#include "preflight.h"
#include "resource.h"

/**
 * Gbhv-style DayZ Loader
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

int main() {
    SetConsoleTitleA("Gbhv System Controller");

    try {
        if (!Cheat::Preflight::RunAllChecks()) return 1;

        // 1. Launch Gbhv-based Hypervisor
        HANDLE hDriver = CreateFileA("\\\\.\\DayZStealth", GENERIC_ALL, 0, 0, OPEN_EXISTING, 0, 0);
        if (hDriver != INVALID_HANDLE_VALUE) {
            DWORD ret;
            DeviceIoControl(hDriver, IO_LAUNCH_HV, 0, 0, 0, 0, &ret, 0);
            CloseHandle(hDriver);
        }

        // 2. Prepare Payload
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PAYLOAD_BIN), RT_RCDATA);
        DWORD sz = SizeofResource(NULL, hRes);
        uint8_t* pRes = (uint8_t*)LockResource(LoadResource(NULL, hRes));
        std::vector<uint8_t> payload(pRes, pRes + sz);

        // 3. Target Game
        uint32_t pid = 0;
        while (!(pid = GetProcessId("DayZ_x64.exe"))) Sleep(500);
        uint64_t cr3 = Cheat::Hv::GetCr3(pid);

        // 4. Inject
        auto mapping = Cheat::ManualMapper::MapImage(cr3, payload);
        if (mapping.Success) {
            Cheat::ManualMapper::Hijack(pid, cr3, mapping.EntryPoint);
        }

        std::cout << "[+] System virtualized. Gbhv active." << std::endl;
        Sleep(2000);

    } catch (...) {
        return 1;
    }

    return 0;
}
