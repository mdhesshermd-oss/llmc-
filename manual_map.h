#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <tlhelp32.h>
#include "pe_reloc.h"
#include "pe_imports.h"
#include "hypervisor_io.h"

namespace Cheat {
    class ManualMapper {
    public:
        struct MappingData {
            void* ImageBase;
            void* EntryPoint;
            bool Success;
        };

        /**
         * Shellcode for injection: calls LoadLibrary and DLL entry point, then returns to original RIP.
         */
        inline static uint8_t injection_shellcode[] = {
            0x48, 0x83, 0xEC, 0x28,                                     // sub rsp, 28h
            0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rcx, <DLL_PATH_ADDR>
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, <LoadLibraryA_ADDR>
            0xFF, 0xD0,                                                 // call rax
            0x48, 0x83, 0xC4, 0x28,                                     // add rsp, 28h
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, <Original_RIP>
            0xFF, 0xE0                                                  // jmp rax (Return to game)
        };

        /**
         * Hijacks an existing game thread to execute the payload.
         * Bypasses CreateRemoteThread detection.
         */
        static void HijackThread(uint32_t pid, uint64_t cr3, uintptr_t entryPoint) {
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            THREADENTRY32 te = { sizeof(te) };
            if (!Thread32First(hSnap, &te)) return;

            uint32_t targetThreadId = 0;
            do {
                if (te.th32OwnerProcessID == pid) {
                    targetThreadId = te.th32ThreadID;
                    break;
                }
            } while (Thread32Next(hSnap, &te));
            CloseHandle(hSnap);

            if (!targetThreadId) return;

            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, targetThreadId);
            SuspendThread(hThread);

            CONTEXT ctx = { CONTEXT_CONTROL };
            GetThreadContext(hThread, &ctx);

            // Prepare shellcode with original RIP
            uintptr_t remoteShellcode = 0x140010000; // Placeholder for hypervisor-allocated shellcode area
            *(uint64_t*)(injection_shellcode + 28) = ctx.Rip;

            // Write shellcode to the game via Hypervisor
            Cheat::Hv::WriteRaw(cr3, remoteShellcode, injection_shellcode, sizeof(injection_shellcode));

            // Redirect thread
            ctx.Rip = remoteShellcode;
            SetThreadContext(hThread, &ctx);

            ResumeThread(hThread);
            CloseHandle(hThread);
        }

        static MappingData MapImage(uint64_t cr3, const std::vector<uint8_t>& rawData) {
            MappingData result = { nullptr, nullptr, false };

            PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)rawData.data();
            PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(rawData.data() + pDos->e_lfanew);

            void* pRemoteImage = (void*)0x140000000;

            // Map via Hypervisor
            Cheat::Hv::WriteRaw(cr3, (uintptr_t)pRemoteImage, (void*)rawData.data(), pNt->OptionalHeader.SizeOfHeaders);

            PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
            for (int i = 0; i < pNt->FileHeader.NumberOfSections; ++i) {
                if (pSection[i].SizeOfRawData > 0) {
                    Cheat::Hv::WriteRaw(cr3, (uintptr_t)pRemoteImage + pSection[i].VirtualAddress,
                                     (void*)((uintptr_t)rawData.data() + pSection[i].PointerToRawData),
                                     pSection[i].SizeOfRawData);
                }
            }

            uintptr_t delta = (uintptr_t)pRemoteImage - pNt->OptionalHeader.ImageBase;
            Pe::ApplyRelocations(cr3, pRemoteImage, pNt, delta);
            Pe::ResolveImports(cr3, pRemoteImage, pNt);

            result.ImageBase = pRemoteImage;
            result.EntryPoint = (void*)((uintptr_t)pRemoteImage + pNt->OptionalHeader.AddressOfEntryPoint);
            result.Success = true;

            return result;
        }
    };
}
