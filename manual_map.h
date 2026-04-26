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

        /**
         * Шеллкод: вызывает EntryPoint DLL и возвращается на оригинальный RIP игры.
         */
        static inline uint8_t hijack_shellcode[] = {
            0x48, 0x83, 0xEC, 0x28,                                     // 0: sub rsp, 28h
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4: mov rax, <EntryPoint>
            0xFF, 0xD0,                                                 // 14: call rax
            0x48, 0x83, 0xC4, 0x28,                                     // 16: add rsp, 28h
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 20: mov rax, <Original_RIP>
            0xFF, 0xE0                                                  // 30: jmp rax
        };

        /**
         * Перехватывает поток игры для выполнения внедренного кода.
         */
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

                    // 1. Область для шеллкода (в идеале выделенная через гипервизор)
                    uintptr_t shellcodeAddr = 0x140010000;

                    // 2. Патчим шеллкод адресом входа и адресом возврата
                    *(uint64_t*)(hijack_shellcode + 6) = entryPoint;
                    *(uint64_t*)(hijack_shellcode + 22) = ctx.Rip;

                    // 3. Записываем шеллкод и перенаправляем поток
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

        /**
         * Маппинг PE-образа в процесс через гипервизор.
         */
        static MappingData MapImage(uint64_t cr3, const std::vector<uint8_t>& rawData) {
            MappingData result = { 0, 0, false };

            PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)rawData.data();
            PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(rawData.data() + pDos->e_lfanew);

            uintptr_t remoteBase = 0x140000000 + pNt->OptionalHeader.SizeOfImage;

            Hv::WriteRaw(cr3, remoteBase, (void*)rawData.data(), pNt->OptionalHeader.SizeOfHeaders);

            PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
            for (int i = 0; i < pNt->FileHeader.NumberOfSections; ++i) {
                if (pSection[i].SizeOfRawData > 0) {
                    Hv::WriteRaw(cr3, remoteBase + pSection[i].VirtualAddress,
                                 (void*)(rawData.data() + pSection[i].PointerToRawData),
                                 pSection[i].SizeOfRawData);
                }
            }

            uintptr_t delta = remoteBase - pNt->OptionalHeader.ImageBase;
            if (!Pe::ApplyRelocations(cr3, (void*)remoteBase, pNt, delta)) return result;
            if (!Pe::ResolveImports(cr3, (void*)remoteBase, pNt)) return result;

            result.ImageBase = remoteBase;
            result.EntryPoint = remoteBase + pNt->OptionalHeader.AddressOfEntryPoint;
            result.Success = true;

            return result;
        }
    };
}
