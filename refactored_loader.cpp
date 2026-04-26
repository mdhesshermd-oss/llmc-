#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include "manual_map.h"
#include "driver_io.h"
#include "overlay_hijack.h"
#include "preflight.h"
#include "resource.h"
#include "stealth.h"
#include "hv_init_amd.h"
#include "hv_init.h"

/**
 * Main Cheat Entry & Controller
 * Orchestrates Driver Loading, Hypervisor Initialization, and Injection.
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

int main() {
    SetConsoleTitleA("System Update Service");

    std::cout << "[+] Initializing System Components..." << std::endl;

    // 1. Preflight Checks
    if (!Cheat::Preflight::RunAllChecks()) {
        std::cerr << "[-] Error: System incompatible. Check BIOS (VT-x/SVM) and Disable Hyper-V." << std::endl;
        Sleep(3000);
        return 1;
    }

    // 2. Initialize Hypervisor (Ring -1)
    std::cout << "[+] Virtualizing Environment..." << std::endl;
    // Detect CPU Vendor
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    if (memcmp(&cpuInfo[1], "Auth", 4) == 0) {
        if (!Cheat::Hv::AMD::InitializeSVM()) {
            std::cerr << "[-] SVM Initialization failed." << std::endl;
        }
    } else {
        if (!Cheat::Hv::Intel::InitializeVT()) {
            std::cerr << "[-] VT-x Initialization failed." << std::endl;
        }
    }

    // 3. Initialize Kernel Driver (Ring 0)
    if (!Cheat::Driver::Init()) {
        std::cout << "[!] Driver not found. Attempting manual load..." << std::endl;
        // In a real loader, we would drop and load the driver here.
    }

    // 4. Wait for Game
    std::cout << "[*] Waiting for DayZ_x64.exe..." << std::endl;
    uint32_t targetPid = 0;
    while (!(targetPid = GetProcessId("DayZ_x64.exe"))) {
        Sleep(1000);
    }

    std::cout << "[+] Found DayZ (PID: " << targetPid << ")" << std::endl;

    // 5. Injection (Manual Map)
    // For this demonstration, we assume the cheat logic is compiled into a DLL
    // and stored as a resource IDR_CHEAT_DLL
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_CHEAT_DLL), RT_RCDATA);
    if (!hRes) {
        std::cerr << "[-] Internal Error: Logic payload missing." << std::endl;
        return 1;
    }

    DWORD dwSize = SizeofResource(NULL, hRes);
    void* pData = LockResource(LoadResource(NULL, hRes));

    std::vector<uint8_t> rawDll((uint8_t*)pData, (uint8_t*)pData + dwSize);

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPid);
    auto mappingResult = Cheat::ManualMapper::MapImage(hProc, rawDll);

    if (mappingResult.Success) {
        std::cout << "[+] Cheat logic successfully mapped at " << mappingResult.ImageBase << std::endl;
        // Start the logic entry point in the game process
        CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)mappingResult.EntryPoint, nullptr, 0, nullptr);
    } else {
        std::cerr << "[-] Injection failed." << std::endl;
    }

    CloseHandle(hProc);
    std::cout << "[+] Setup complete. Cleaning up..." << std::endl;
    Sleep(3000);

    return 0;
}
