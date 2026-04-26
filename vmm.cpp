#include "vmm.h"
#include "svm.h"
#include <intrin.h>

/**
 * Gbhv-style Virtual Machine Manager (AMD Implementation)
 * Production Grade: Randomized Pool Tags and proper core synchronization.
 */

// Randomized non-descriptive Pool Tags to evade forensic scanning
#define TAG_GLOBAL_CTX 'Gst1'
#define TAG_PROC_LIST  'Lst2'
#define TAG_CORE_CTX   'Ctx3'

PVMM_CONTEXT GbhvAllocateVmmContext() {
    PVMM_CONTEXT Context = (PVMM_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(VMM_CONTEXT), TAG_GLOBAL_CTX);
    if (!Context) return NULL;
    RtlZeroMemory(Context, sizeof(VMM_CONTEXT));

    Context->ProcessorCount = KeQueryActiveProcessorCount(NULL);
    Context->SystemDirectoryTableBase = __readcr3();

    // Allocate array of pointers for processor contexts
    Context->ProcessorContexts = (PVMM_PROCESSOR_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PVMM_PROCESSOR_CONTEXT) * Context->ProcessorCount, TAG_PROC_LIST);
    if (!Context->ProcessorContexts) {
        ExFreePoolWithTag(Context, TAG_GLOBAL_CTX);
        return NULL;
    }

    for (UINT32 i = 0; i < Context->ProcessorCount; i++) {
        Context->ProcessorContexts[i] = (PVMM_PROCESSOR_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(VMM_PROCESSOR_CONTEXT), TAG_CORE_CTX);
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

    if (CoreContext && GbhvSvmInitialize(CoreContext)) {
        CoreContext->HasLaunched = TRUE;
        InterlockedIncrement((volatile LONG*)&GlobalContext->SuccessfulInitializationsCount);
    }

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
}

VOID GbhvInitializeAllProcessors(PVMM_CONTEXT GlobalContext) {
    KeGenericCallDpc(GbhvDpcBroadcastFunction, GlobalContext);
}
