#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>
#include "driver_io.h"

/**
 * Functional Signature Scanner (AOB)
 */

namespace Cheat {
    namespace Scanner {

        /**
         * Converts IDA-style pattern "48 8B 05 ? ? ? ?" to bytes and mask.
         */
        inline std::vector<int> ParsePattern(const std::string& pattern) {
            std::vector<int> bytes;
            std::stringstream ss(pattern);
            std::string word;
            while (ss >> word) {
                if (word == "?" || word == "??") {
                    bytes.push_back(-1);
                } else {
                    bytes.push_back(std::stoi(word, nullptr, 16));
                }
            }
            return bytes;
        }

        inline uintptr_t FindPattern(uint32_t pid, uintptr_t base, size_t size, const std::string& pattern) {
            auto sig = ParsePattern(pattern);
            if (sig.empty()) return 0;

            // Read the entire region into local memory for speed
            std::vector<uint8_t> buffer(size);
            if (!Driver::ReadRaw(pid, base, buffer.data(), size)) return 0;

            for (size_t i = 0; i <= size - sig.size(); ++i) {
                bool found = true;
                for (size_t j = 0; j < sig.size(); ++j) {
                    if (sig[j] != -1 && buffer[i + j] != static_cast<uint8_t>(sig[j])) {
                        found = false;
                        break;
                    }
                }
                if (found) return base + i;
            }

            return 0;
        }

        inline uintptr_t ResolveRelativeAddr(uint32_t pid, uintptr_t instruction_addr, uint32_t offset_in_instr, uint32_t total_instr_len) {
            int32_t rip_offset = Driver::Read<int32_t>(pid, instruction_addr + offset_in_instr);
            return instruction_addr + total_instr_len + rip_offset;
        }
    }
}
