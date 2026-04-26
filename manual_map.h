#pragma once
#include <windows.h>
#include <vector>
#include <tlhelp32.h>
#include "hypervisor_io.h"
#include "pe_reloc.h"
#include "pe_imports.h"

namespace Cheat {
    class ManualMapper {
    public:
        struct MappingData {
            uintptr_t ImageBase;
            uintptr_t EntryPoint;
            bool Success;
        };

        static inline uint8_t hijack_shellcode[] = {
            0x48, 0x83, 0xEC, 0x28,                                     // 0: sub rsp, 28h
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4: mov rax, <EntryPoint>
            0xFF, 0xD0,                                                 // 14: call rax
            0x48, 0x83, 0xC4, 0x28,                                     // 16: add rsp, 28h
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 20: mov rax, <Original_RIP>
            0xFF, 0xE0                                                  // 30: jmp rax
        };

        static bool Hijack(uint32_t pid, uint64_t cr3, uintptr_t entryPoint) {
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

                    uintptr_t shellcodeAddr = Hv::AllocateRemoteMemory(cr3, 4096);
                    if (!shellcodeAddr) { CloseHandle(hThread); continue; }

                    *(uint64_t*)(hijack_shellcode + 6) = entryPoint;
                    *(uint64_t*)(hijack_shellcode + 22) = ctx.Rip;

                    Hv::WriteRaw(cr3, shellcodeAddr, hijack_shellcode, sizeof(hijack_shellcode));

                    ctx.Rip = shellcodeAddr;
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
            MappingData res = { 0, 0, false };
            PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)rawData.data();
            PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(rawData.data() + dos->e_lfanew);

            // 1. Allocate memory through hypervisor
            uintptr_t remoteBase = Hv::AllocateRemoteMemory(cr3, nt->OptionalHeader.SizeOfImage);
            if (!remoteBase) return res;

            // 2. Map Headers
            Hv::WriteRaw(cr3, remoteBase, (void*)rawData.data(), nt->OptionalHeader.SizeOfHeaders);

            // 3. Map Sections
            PIMAGE_SECTION_HEADER sect = IMAGE_FIRST_SECTION(nt);
            for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
                if (sect[i].SizeOfRawData > 0) {
                    Hv::WriteRaw(cr3, remoteBase + sect[i].VirtualAddress,
                                 (void*)(rawData.data() + sect[i].PointerToRawData),
                                 sect[i].SizeOfRawData);
                }
            }

            // 4. Apply Relocations
            uintptr_t delta = remoteBase - nt->OptionalHeader.ImageBase;
            if (!Pe::ApplyRelocations(cr3, (void*)remoteBase, nt, delta)) return res;

            // 5. Resolve Imports
            if (!Pe::ResolveImports(cr3, (void*)remoteBase, nt)) return res;

            res.ImageBase = remoteBase;
            res.EntryPoint = remoteBase + nt->OptionalHeader.AddressOfEntryPoint;
            res.Success = true;
            return res;
        }
    };
}
