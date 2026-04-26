#include <ntddk.h>
#include <ntstrsafe.h>
#include "hv_core.h"

/**
 * Advanced Kernel Bridge Driver
 * Orchestrates multi-core hypervisor initialization.
 */

#define IO_LAUNCH_HV CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0805, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Standard Pool Tag for Hypervisor
#define HV_POOL_TAG 'HVMx'

// Forward declaration matching hv_init_amd.h
namespace Cheat { namespace Hv { namespace AMD {
    bool InitializeSVM(PerCoreData* ctx);
}}}

/**
 * DPC routine called for each logical core to transition to Ring -1.
 */
void NTAPI HvKernelBootstrap(PKDPC Dpc, PVOID Context, PVOID SystemArgument1, PVOID SystemArgument2) {
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Context);

    // Allocate core-specific context in NonPagedPool
    Cheat::Hv::PerCoreData* ctx = (Cheat::Hv::PerCoreData*)ExAllocatePoolWithTag(NonPagedPool, sizeof(Cheat::Hv::PerCoreData), HV_POOL_TAG);

    if (ctx) {
        RtlZeroMemory(ctx, sizeof(Cheat::Hv::PerCoreData));
        ctx->self_va = ctx;
        ctx->lifecycle_state = 1; // Running

        // Enter hypervisor mode on this core
        if (!Cheat::Hv::AMD::InitializeSVM(ctx)) {
            ExFreePoolWithTag(ctx, HV_POOL_TAG);
        }
    }

    // Signal completion of DPC on this core
    KeSignalCallDpcDone(SystemArgument1);
}

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    if (stack->Parameters.DeviceIoControl.IoControlCode == IO_LAUNCH_HV) {
        // Force all cores to virtualize simultaneously using a generic DPC call
        KeGenericCallDpc(HvKernelBootstrap, NULL);
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING dev = RTL_CONSTANT_STRING(L"\\Device\\DayZStealth");
    UNICODE_STRING sym = RTL_CONSTANT_STRING(L"\\??\\DayZStealth");
    PDEVICE_OBJECT deviceObj;

    NTSTATUS status = IoCreateDevice(DriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObj);
    if (!NT_SUCCESS(status)) return status;

    status = IoCreateSymbolicLink(&sym, &dev);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObj);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;

    return STATUS_SUCCESS;
}
