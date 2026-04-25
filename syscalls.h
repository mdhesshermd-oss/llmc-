#pragma once
#include <stdint.h>
#include <windows.h>
#include "stealth.h"

/**
 * Direct Syscall Implementation with valid wrappers.
 */

extern "C" NTSTATUS InternalSyscall(uint32_t ssn, ...);

// Dynamic SSNs resolved via Hell's Gate
inline uint32_t SSN_NtAllocateVirtualMemory = 0;
inline uint32_t SSN_NtProtectVirtualMemory = 0;

inline void InitSyscalls() {
    SSN_NtAllocateVirtualMemory = Cheat::Stealth::GetSyscallNumber("NtAllocateVirtualMemory");
    SSN_NtProtectVirtualMemory = Cheat::Stealth::GetSyscallNumber("NtProtectVirtualMemory");
}

static inline NTSTATUS DirectNtAllocateVirtualMemory(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    ULONG_PTR ZeroBits,
    PSIZE_T RegionSize,
    ULONG AllocationType,
    ULONG Protect)
{
    return InternalSyscall(SSN_NtAllocateVirtualMemory, ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

static inline NTSTATUS DirectNtProtectVirtualMemory(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    PSIZE_T RegionSize,
    ULONG NewProtect,
    PULONG OldProtect)
{
    return InternalSyscall(SSN_NtProtectVirtualMemory, ProcessHandle, BaseAddress, RegionSize, NewProtect, OldProtect);
}
