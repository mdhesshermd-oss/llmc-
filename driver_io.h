#pragma once
#include <windows.h>
#include <stdint.h>

/**
 * C++ Driver Communication Interface
 */

namespace Cheat {
    namespace Driver {

        #define IO_READ_REQUEST  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

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

        template <typename T>
        inline T Read(uint32_t pid, uintptr_t address) {
            T buffer{};
            ReadRequest request = { pid, address, &buffer, sizeof(T) };
            DWORD returned;
            DeviceIoControl(hDriver, IO_READ_REQUEST, &request, sizeof(request), &request, sizeof(request), &returned, nullptr);
            return buffer;
        }
    }
}
