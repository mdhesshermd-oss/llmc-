#include "vmm.h"
#include "svm.h"
#include <intrin.h>

/**
 * Gbhv-style Virtual Machine Manager (AMD Implementation)
 */

PVMM_CONTEXT GbhvAllocateVmmContext() {
    PVMM_CONTEXT Context = (PVMM_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(VMM_CONTEXT), 'VMMG');
    if (!Context) return NULL;
    RtlZeroMemory(Context, sizeof(VMM_CONTEXT));

    Context->ProcessorCount = KeQueryActiveProcessorCount(NULL);
    Context->SystemDirectoryTableBase = __readcr3();

    Context->ProcessorContexts = (PVMM_PROCESSOR_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PVMM_PROCESSOR_CONTEXT) * Context->ProcessorCount, 'VMMP');
    if (!Context->ProcessorContexts) {
        ExFreePool(Context);
        return NULL;
    }

    for (UINT32 i = 0; i < Context->ProcessorCount; i++) {
        Context->ProcessorContexts[i] = (PVMM_PROCESSOR_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(VMM_PROCESSOR_CONTEXT), 'VMMC');
        RtlZeroMemory(Context->ProcessorContexts[i], sizeof(VMM_PROCESSOR_CONTEXT));
        Context->ProcessorContexts[i]->GlobalContext = Context;
    }

    return Context;
}

VOID NTAPI GbhvDpcBroadcastFunction(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2) {
    UNREFERENCED_PARAMETER(Dpc);
    PVMM_CONTEXT GlobalContext = (PVMM_CONTEXT)DeferredContext;
    UINT32 CoreIndex = KeGetCurrentProcessorNumber();
    PVMM_PROCESSOR_CONTEXT CoreContext = GlobalContext->ProcessorContexts[CoreIndex];

    if (GbhvSvmInitialize(CoreContext)) {
        CoreContext->HasLaunched = TRUE;
    }

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
}

VOID GbhvInitializeAllProcessors(PVMM_CONTEXT GlobalContext) {
    KeGenericCallDpc(GbhvDpcBroadcastFunction, GlobalContext);
}
