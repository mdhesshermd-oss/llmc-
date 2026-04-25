#include <windows.h>
#include <stdint.h>

/**
 * Kernel Driver Communication Interface
 * Allows the cheat to read/write game memory from Ring 0.
 */

#define IO_READ_REQUEST  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_GET_BASE_ADDR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DRIVER_READ_REQUEST {
    uint32_t process_id;
    uintptr_t address;
    void* buffer;
    size_t size;
} DRIVER_READ_REQUEST;

HANDLE hDriver = INVALID_HANDLE_VALUE;

bool InitDriverInterface() {
    hDriver = CreateFileA("\\\\.\\MyCheatDriver", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    return (hDriver != INVALID_HANDLE_VALUE);
}

template <typename T>
T ReadMemory(uint32_t pid, uintptr_t address) {
    T buffer;
    DRIVER_READ_REQUEST request = { pid, address, &buffer, sizeof(T) };
    DWORD bytesReturned;

    DeviceIoControl(hDriver, IO_READ_REQUEST, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL);
    return buffer;
}

uintptr_t GetProcessBaseAddress(uint32_t pid) {
    uintptr_t baseAddr = 0;
    DeviceIoControl(hDriver, IO_GET_BASE_ADDR, &pid, sizeof(pid), &baseAddr, sizeof(baseAddr), NULL, NULL);
    return baseAddr;
}
