#include "vmm.h"
#include "svm.h"
#include <intrin.h>

/**
 * Gbhv-style Virtual Machine Manager (AMD Implementation)
 * Robust: Synchronized multi-core initialization.
 */

PVMM_CONTEXT GbhvAllocateVmmContext() {
    PVMM_CONTEXT Context = (PVMM_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(VMM_CONTEXT), 'Gst1');
    if (!Context) return NULL;
    RtlZeroMemory(Context, sizeof(VMM_CONTEXT));

    Context->ProcessorCount = KeQueryActiveProcessorCount(NULL);
    Context->SystemDirectoryTableBase = __readcr3();

    Context->ProcessorContexts = (PVMM_PROCESSOR_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PVMM_PROCESSOR_CONTEXT) * Context->ProcessorCount, 'Lst2');
    if (!Context->ProcessorContexts) {
        ExFreePoolWithTag(Context, 'Gst1');
        return NULL;
    }

    for (UINT32 i = 0; i < Context->ProcessorCount; i++) {
        Context->ProcessorContexts[i] = (PVMM_PROCESSOR_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(VMM_PROCESSOR_CONTEXT), 'Ctx3');
        if (Context->ProcessorContexts[i]) {
            RtlZeroMemory(Context->ProcessorContexts[i], sizeof(VMM_PROCESSOR_CONTEXT));
            Context->ProcessorContexts[i]->GlobalContext = Context;
        }
    }
    return Context;
}

VOID NTAPI GbhvDpcBroadcastFunction(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2) {
    UNREFERENCED_PARAMETER(Dpc);
    PVMM_CONTEXT GlobalContext = (PVMM_CONTEXT)DeferredContext;
    UINT32 CoreIndex = KeGetCurrentProcessorNumber();
    PVMM_PROCESSOR_CONTEXT CoreContext = GlobalContext->ProcessorContexts[CoreIndex];

    if (CoreContext) {
        if (GbhvSvmInitialize(CoreContext)) {
            InterlockedIncrement(&GlobalContext->SuccessfulInitializationsCount);
        }
    }

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
}

VOID GbhvInitializeAllProcessors(PVMM_CONTEXT GlobalContext) {
    KeGenericCallDpc(GbhvDpcBroadcastFunction, GlobalContext);

    // Wait for all cores to confirm virtualization
    while (GlobalContext->SuccessfulInitializationsCount < (LONG)GlobalContext->ProcessorCount) {
        YieldProcessor();
    }
}
