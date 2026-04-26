#pragma once
#include <windows.h>
#include <stdint.h>

/**
 * Advanced Kernel Driver Interface for DayZ Stealth Driver
 */

namespace Cheat {
    namespace Driver {

        // --- IOCTL Definitions ---
        // Must match driver.c
        #define IO_READ_REQUEST  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_BUFFERED, FILE_ANY_ACCESS)
        #define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_BUFFERED, FILE_ANY_ACCESS)

        typedef struct _KERNEL_READ_REQUEST {
            ULONG ProcessId;
            ULONGLONG Address;
            PVOID Response;
            SIZE_T Size;
        } KERNEL_READ_REQUEST, *PKERNEL_READ_REQUEST;

        typedef struct _KERNEL_WRITE_REQUEST {
            ULONG ProcessId;
            ULONGLONG Address;
            PVOID Value;
            SIZE_T Size;
        } KERNEL_WRITE_REQUEST, *PKERNEL_WRITE_REQUEST;

        inline HANDLE hDriver = INVALID_HANDLE_VALUE;

        inline bool Init() {
            if (hDriver != INVALID_HANDLE_VALUE) return true;
            // Use the symbolic link created in driver.c
            hDriver = CreateFileA("\\\\.\\DayZStealth", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            return (hDriver != INVALID_HANDLE_VALUE);
        }

        /**
         * Reads a raw block of memory of arbitrary size from Ring 0.
         */
        inline bool ReadRaw(uint32_t pid, uintptr_t address, void* buffer, size_t size) {
            if (hDriver == INVALID_HANDLE_VALUE && !Init()) return false;

            KERNEL_READ_REQUEST request = { pid, (ULONGLONG)address, buffer, size };
            DWORD returned;
            return DeviceIoControl(hDriver, IO_READ_REQUEST, &request, sizeof(request), &request, sizeof(request), &returned, nullptr);
        }

        /**
         * Writes a raw block of memory to Ring 0.
         */
        inline bool WriteRaw(uint32_t pid, uintptr_t address, void* buffer, size_t size) {
            if (hDriver == INVALID_HANDLE_VALUE && !Init()) return false;

            KERNEL_WRITE_REQUEST request = { pid, (ULONGLONG)address, buffer, size };
            DWORD returned;
            return DeviceIoControl(hDriver, IO_WRITE_REQUEST, &request, sizeof(request), &request, sizeof(request), &returned, nullptr);
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

        /**
         * Template for writing specific types.
         */
        template <typename T>
        inline bool Write(uint32_t pid, uintptr_t address, T value) {
            return WriteRaw(pid, address, &value, sizeof(T));
        }
    }
}
