#pragma once
#include <windows.h>
#include <stdint.h>

/**
 * Advanced Kernel Driver Interface with Raw Block Reading
 */

namespace Cheat {
    namespace Driver {

        #define IO_READ_RAW  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

        struct ReadRequest {
            uint32_t pid;
            uintptr_t address;
            void* buffer;
            size_t size;
        };

        inline HANDLE hDriver = INVALID_HANDLE_VALUE;

        inline bool Init() {
            if (hDriver != INVALID_HANDLE_VALUE) return true;
            hDriver = CreateFileA("\\\\.\\MyCheatDriver", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            return (hDriver != INVALID_HANDLE_VALUE);
        }

        /**
         * Reads a raw block of memory of arbitrary size.
         */
        inline bool ReadRaw(uint32_t pid, uintptr_t address, void* buffer, size_t size) {
            ReadRequest request = { pid, address, buffer, size };
            DWORD returned;
            return DeviceIoControl(hDriver, IO_READ_RAW, &request, sizeof(request), &request, sizeof(request), &returned, nullptr);
        }

        /**
         * Template for reading specific types (e.g., struct, int).
         */
        template <typename T>
        inline T Read(uint32_t pid, uintptr_t address) {
            T buffer{};
            ReadRaw(pid, address, &buffer, sizeof(T));
            return buffer;
        }
    }
}
