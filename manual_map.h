#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "pe_reloc.h"
#include "pe_imports.h"
#include "stealth.h"

namespace Cheat {
    class ManualMapper {
    public:
        struct MappingData {
            void* ImageBase;
            void* EntryPoint;
            bool Success;
        };

        static MappingData MapImage(HANDLE hProcess, const std::vector<uint8_t>& rawData) {
            MappingData result = { nullptr, nullptr, false };

            PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)rawData.data();
            if (pDos->e_magic != IMAGE_DOS_SIGNATURE) return result;

            PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(rawData.data() + pDos->e_lfanew);
            if (pNt->Signature != IMAGE_NT_SIGNATURE) return result;

            // 1. Allocate memory in target process
            // For stealth, we should ideally find a 'stompable' module,
            // but for this implementation we use standard allocation with stealth flags.
            void* pRemoteImage = VirtualAllocEx(hProcess, nullptr, pNt->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (!pRemoteImage) return result;

            // 2. Map Headers
            WriteProcessMemory(hProcess, pRemoteImage, rawData.data(), pNt->OptionalHeader.SizeOfHeaders, nullptr);

            // 3. Map Sections
            PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
            for (int i = 0; i < pNt->FileHeader.NumberOfSections; ++i) {
                if (pSection[i].SizeOfRawData > 0) {
                    WriteProcessMemory(hProcess, (LPVOID)((uintptr_t)pRemoteImage + pSection[i].VirtualAddress),
                                     (LPVOID)((uintptr_t)rawData.data() + pSection[i].PointerToRawData),
                                     pSection[i].SizeOfRawData, nullptr);
                }
            }

            // 4. Perform Relocations
            if (!RelocateImage(hProcess, pRemoteImage, pNt, rawData.data())) {
                VirtualFreeEx(hProcess, pRemoteImage, 0, MEM_RELEASE);
                return result;
            }

            // 5. Resolve Imports
            if (!ResolveImports(hProcess, pRemoteImage, pNt)) {
                VirtualFreeEx(hProcess, pRemoteImage, 0, MEM_RELEASE);
                return result;
            }

            result.ImageBase = pRemoteImage;
            result.EntryPoint = (void*)((uintptr_t)pRemoteImage + pNt->OptionalHeader.AddressOfEntryPoint);
            result.Success = true;

            return result;
        }

    private:
        static bool RelocateImage(HANDLE hProcess, void* pRemoteBase, PIMAGE_NT_HEADERS pNt, uint8_t* pLocalRaw) {
            // Delta calculation
            uintptr_t delta = (uintptr_t)pRemoteBase - pNt->OptionalHeader.ImageBase;
            if (delta == 0) return true; // No relocation needed

            auto& relocDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
            if (relocDir.Size == 0) return true;

            // We iterate through the relocation table.
            // This logic is abstracted in pe_reloc.h, which we call here.
            return Pe::ApplyRelocations(hProcess, pRemoteBase, pNt, delta);
        }

        static bool ResolveImports(HANDLE hProcess, void* pRemoteBase, PIMAGE_NT_HEADERS pNt) {
            auto& importDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
            if (importDir.Size == 0) return true;

            // This logic is abstracted in pe_imports.h
            return Pe::ResolveImports(hProcess, pRemoteBase, pNt);
        }
    };
}
