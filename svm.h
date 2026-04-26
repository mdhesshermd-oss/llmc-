#pragma once
#include <ntddk.h>

/**
 * Gbhv-style AMD SVM Backend
 */

#ifdef __cplusplus
extern "C" {
#endif

void GbhvSvmLaunch(UINT64 VmcbPa, UINT64 HsavePa, PVOID Context);

BOOLEAN GbhvSvmInitialize(PVMM_PROCESSOR_CONTEXT ProcessorContext);

#ifdef __cplusplus
}
#endif
