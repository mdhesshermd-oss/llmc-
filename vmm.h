#pragma once
#include <ntddk.h>

/**
 * Gbhv-style Core Definitions for AMD SVM
 */

typedef struct _GUEST_REGISTERS {
    UINT64 Rax, Rcx, Rdx, Rbx, Rsp, Rbp, Rsi, Rdi, R8, R9, R10, R11, R12, R13, R14, R15;
} GUEST_REGISTERS, *PGUEST_REGISTERS;

typedef struct _VMM_PROCESSOR_CONTEXT {
    UINT64 GuestRip;
    UINT64 GuestRsp;
    UINT64 GuestRflags;
    UINT64 VmcbPhysical;
    PVOID VmcbVirtual;
    UINT64 HostSavePhysical;
    PVOID HostSaveVirtual;
    UINT64 NptRootPhysical;
    PVOID NptRootVirtual;
    struct _VMM_CONTEXT* GlobalContext;
    volatile LONG HasLaunched;
} VMM_PROCESSOR_CONTEXT, *PVMM_PROCESSOR_CONTEXT;

typedef struct _VMM_CONTEXT {
    UINT32 ProcessorCount;
    volatile LONG SuccessfulInitializationsCount;
    PVMM_PROCESSOR_CONTEXT* ProcessorContexts;
    UINT64 SystemDirectoryTableBase;
} VMM_CONTEXT, *PVMM_CONTEXT;

// Hypercall secret matching the user requirement
#define HV_SECRET_KEY 0x5A4F524F5F444159
