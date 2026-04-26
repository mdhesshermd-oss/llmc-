#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include "hypervisor_io.h"
#include "pe_reloc.h"
#include "pe_imports.h"

namespace Cheat {
    class ManualMapper {
    public:
        struct MappingData {
            void* ImageBase;
            void* EntryPoint;
            bool Success;
        };

        // Shellcode: calls LoadLibraryA, calls EntryPoint, then jumps back to original RIP
        static inline uint8_t hijack_shellcode[] = {
            0x48, 0x83, 0xEC, 0x28,                                     // sub rsp, 28h
            0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rcx, <DLL_PATH_ADDR>
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, <LoadLibraryA_ADDR>
            0xFF, 0xD0,                                                 // call rax
            0x48, 0x83, 0xC4, 0x28,                                     // add rsp, 28h
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, <Original_RIP>
            0xFF, 0xE0                                                  // jmp rax
        };

        /**
         * Hijacks an existing game thread to execute the shellcode.
         * Bypasses 'CreateRemoteThread' detection by using the game's own context.
         */
        static bool Hijack(uint32_t pid, uint64_t cr3, uintptr_t remoteShellcodeAddr) {
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            THREADENTRY32 te = { sizeof(te) };
            if (!Thread32First(hSnap, &te)) return false;

            do {
                if (te.th32OwnerProcessID == pid) {
                    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                    if (!hThread) continue;

                    SuspendThread(hThread);

                    CONTEXT ctx = { CONTEXT_CONTROL };
                    GetThreadContext(hThread, &ctx);

                    // Patch shellcode with the thread's original return address
                    *(uint64_t*)(hijack_shellcode + 28) = ctx.Rip;

                    // Write shellcode to the target process via Hypercall
                    Hv::WriteRaw(cr3, remoteShellcodeAddr, hijack_shellcode, sizeof(hijack_shellcode));

                    // Redirect the thread to the shellcode
                    ctx.Rip = remoteShellcodeAddr;
                    SetThreadContext(hThread, &ctx);

                    ResumeThread(hThread);
                    CloseHandle(hThread);
                    break;
                }
            } while (Thread32Next(hSnap, &te));

            CloseHandle(hSnap);
            return true;
        }

        static MappingData MapImage(uint64_t cr3, const std::vector<uint8_t>& rawData) {
            MappingData result = { nullptr, nullptr, false };

            PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)rawData.data();
            PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(rawData.data() + pDos->e_lfanew);

            // Manual mapping logic (Headers -> Sections -> Relocs -> Imports)
            // Simplified for this architectural overview
            void* pRemoteImage = (void*)0x140000000;

            Hv::WriteRaw(cr3, (uintptr_t)pRemoteImage, (void*)rawData.data(), pNt->OptionalHeader.SizeOfHeaders);

            result.ImageBase = pRemoteImage;
            result.EntryPoint = (void*)((uintptr_t)pRemoteImage + pNt->OptionalHeader.AddressOfEntryPoint);
            result.Success = true;

            return result;
        }
    };
}
