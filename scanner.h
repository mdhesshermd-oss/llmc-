#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include "hypervisor_io.h"

/**
 * Advanced Pattern Scanner
 * Supports finding instruction patterns in both local and remote process memory.
 */

namespace Cheat {
    namespace Scanner {

        /**
         * Converts "AA BB ? DD" to a byte vector.
         */
        inline std::vector<int> ParsePattern(const std::string& pattern) {
            std::vector<int> bytes;
            std::stringstream ss(pattern);
            std::string item;
            while (ss >> item) {
                if (item == "?" || item == "??") {
                    bytes.push_back(-1);
                } else {
                    bytes.push_back(std::stoi(item, nullptr, 16));
                }
            }
            return bytes;
        }

        /**
         * Поиск паттерна в уже считанном массиве байт.
         */
        inline uintptr_t FindPatternInBuffer(const uint8_t* buffer, size_t size, const std::string& pattern) {
            auto sig = ParsePattern(pattern);
            if (sig.empty()) return 0;

            for (size_t i = 0; i <= size - sig.size(); ++i) {
                bool found = true;
                for (size_t j = 0; j < sig.size(); ++j) {
                    if (sig[j] != -1 && buffer[i + j] != (uint8_t)sig[j]) {
                        found = false;
                        break;
                    }
                }
                if (found) return i;
            }
            return 0;
        }

        /**
         * Internal memory scanning for ntoskrnl or hypervisor logic.
         */
        inline uintptr_t FindPatternInternal(uintptr_t base, size_t size, const std::string& pattern) {
            uintptr_t offset = FindPatternInBuffer((const uint8_t*)base, size, pattern);
            return offset ? base + offset : 0;
        }

        /**
         * Standard process memory scanning using the Hypervisor.
         */
        inline uintptr_t FindPattern(uint64_t cr3, uintptr_t base, size_t size, const std::string& pattern) {
            std::vector<uint8_t> buffer(size);
            if (!Cheat::Hv::ReadRaw(cr3, base, buffer.data(), size)) return 0;

            uintptr_t offset = FindPatternInBuffer(buffer.data(), size, pattern);
            return offset ? base + offset : 0;
        }

        /**
         * Resolves relative addresses (e.g., E8, E9, 48 8B 05) via Hypervisor.
         */
        inline uintptr_t ResolveRelativeAddr(uint64_t cr3, uintptr_t instrAddr, uint32_t offset, uint32_t instrSize) {
            int32_t displacement = Cheat::Hv::Read<int32_t>(cr3, instrAddr + offset);
            return instrAddr + instrSize + displacement;
        }
    }
}
