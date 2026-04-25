#pragma once
#include <windows.h>
#include <winternl.h>
#include <stdint.h>

/**
 * Advanced Stealth, Dynamic Syscall Resolution, and API Hashing
 */

extern "C" NTSTATUS InternalSyscall(uint32_t ssn, ...);

namespace Cheat {
    namespace Stealth {

        // FNV-1a Hashing for strings
        static inline uint32_t HashString(const char* str) {
            uint32_t hash = 0x811c9dc5;
            while (*str) {
                hash ^= static_cast<uint32_t>(static_cast<uint8_t>(*str++));
                hash *= 0x01000193;
            }
            return hash;
        }

        static inline uint32_t HashStringW(const wchar_t* str) {
            uint32_t hash = 0x811c9dc5;
            while (*str) {
                wchar_t c = *str++;
                if (c >= L'A' && c <= L'Z') c += 32;
                hash ^= static_cast<uint32_t>(c);
                hash *= 0x01000193;
            }
            return hash;
        }

        /**
         * Resolves function address using name hashing to bypass IAT analysis.
         */
        inline void* GetProcAddressStealth(HMODULE hMod, uint32_t funcHash) {
            auto base = reinterpret_cast<uint8_t*>(hMod);
            auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
            auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dosHeader->e_lfanew);
            auto exportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(base +
                ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

            auto names = reinterpret_cast<uint32_t*>(base + exportDir->AddressOfNames);
            auto ordinals = reinterpret_cast<uint16_t*>(base + exportDir->AddressOfNameOrdinals);
            auto functions = reinterpret_cast<uint32_t*>(base + exportDir->AddressOfFunctions);

            for (uint32_t i = 0; i < exportDir->NumberOfNames; i++) {
                const char* name = reinterpret_cast<const char*>(base + names[i]);
                if (HashString(name) == funcHash) {
                    return reinterpret_cast<void*>(base + functions[ordinals[i]]);
                }
            }
            return nullptr;
        }

        inline HMODULE GetModuleStealth(uint32_t modHash) {
            auto peb = reinterpret_cast<PPEB>(__readgsqword(0x60));
            auto listHead = &peb->Ldr->InMemoryOrderModuleList;

            for (auto curr = listHead->Flink; curr != listHead; curr = curr->Flink) {
                auto entry = CONTAINING_RECORD(curr, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
                if (entry->FullDllName.Buffer && HashStringW(entry->FullDllName.Buffer) == modHash) {
                    return static_cast<HMODULE>(entry->DllBase);
                }
            }
            return nullptr;
        }

        inline uint32_t GetSyscallNumber(const char* funcName) {
            HMODULE ntdll = GetModuleHandleA("ntdll.dll");
            auto funcAddr = reinterpret_cast<uintptr_t>(GetProcAddress(ntdll, funcName));
            if (!funcAddr) return 0;
            for (int i = 0; i < 32; ++i) {
                if (*reinterpret_cast<uint8_t*>(funcAddr + i) == 0xB8) {
                    return *reinterpret_cast<uint32_t*>(funcAddr + i + 1);
                }
            }
            return 0;
        }
    }
}
