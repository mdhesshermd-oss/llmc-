#pragma once
#include <windows.h>
#include <vector>
#include <string>
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
         * Maps an image into a target process using the Hypervisor for stealth.
         * Bypasses User-Mode APIs like WriteProcessMemory.
         */
        static MappingData MapImage(uint64_t cr3, const std::vector<uint8_t>& rawData) {
            MappingData result = { nullptr, nullptr, false };

            PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)rawData.data();
            PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(rawData.data() + pDos->e_lfanew);

            // 1. Target Address (Ideally allocated via VMMCALL)
            void* pRemoteImage = (void*)0x140000000;

            // 2. Map Headers
            Cheat::Hv::WriteRaw(cr3, (uintptr_t)pRemoteImage, (void*)rawData.data(), pNt->OptionalHeader.SizeOfHeaders);

            // 3. Map Sections
            PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
            for (int i = 0; i < pNt->FileHeader.NumberOfSections; ++i) {
                if (pSection[i].SizeOfRawData > 0) {
                    Cheat::Hv::WriteRaw(cr3, (uintptr_t)pRemoteImage + pSection[i].VirtualAddress,
                                     (void*)((uintptr_t)rawData.data() + pSection[i].PointerToRawData),
                                     pSection[i].SizeOfRawData);
                }
            }

            // 4. Perform Relocations & Resolve Imports via Hypervisor
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
