#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "driver_io.h"

/**
 * Robust Signature Scanner (AOB)
 * Supports IDA-style patterns with wildcards.
 */

namespace Cheat {
    namespace Scanner {

        inline uintptr_t FindPattern(uint32_t pid, uintptr_t base, size_t size, const char* pattern) {
            // 1. Parse pattern string into bytes and masks
            std::vector<int> sig;
            const char* current = pattern;
            while (*current) {
                if (*current == ' ') { current++; continue; }
                if (*current == '?') {
                    sig.push_back(-1);
                    current++;
                    if (*current == '?') current++;
                    continue;
                }
                sig.push_back(static_cast<int>(strtol(current, const_cast<char**>(&current), 16)));
            }

            if (sig.empty()) return 0;

            // 2. Read module memory into local buffer (Fast Scan)
            std::vector<uint8_t> data(size);
            if (!Driver::ReadRaw(pid, base, data.data(), size)) return 0;

            // 3. Scan for match
            for (size_t i = 0; i < size - sig.size(); ++i) {
                bool found = true;
                for (size_t j = 0; j < sig.size(); ++j) {
                    if (sig[j] != -1 && data[i + j] != static_cast<uint8_t>(sig[j])) {
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
