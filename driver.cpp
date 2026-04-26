#include <ntddk.h>
#include <ntstrsafe.h>
#include "vmm.h"
#include "svm.h"

/**
 * Advanced Kernel Bridge Driver (Gbhv Style)
 * Orchestrates multi-core hypervisor initialization.
 */

#define IO_LAUNCH_HV CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0805, METHOD_BUFFERED, FILE_ANY_ACCESS)

// External VMM allocators
extern "C" PVMM_CONTEXT GbhvAllocateVmmContext();
extern "C" VOID GbhvInitializeAllProcessors(PVMM_CONTEXT GlobalContext);

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
        // 1. Allocate the Global VMM Context
        PVMM_CONTEXT GlobalContext = GbhvAllocateVmmContext();
        if (GlobalContext) {
            // 2. Broadcast initialization to all logical cores
            GbhvInitializeAllProcessors(GlobalContext);
            DbgPrint("[+] DayZStealth: Hypervisor launched on all cores.\n");
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
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

    DbgPrint("[+] DayZStealth: Bridge Driver Loaded.\n");
    return STATUS_SUCCESS;
}
