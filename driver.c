#include <ntddk.h>
#include <ntstrsafe.h>

// --- IOCTL Definitions ---
// Must match driver_io.h in the user-mode application
#define IO_READ_REQUEST  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_GET_BASE_ADDR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0803, METHOD_BUFFERED, FILE_ANY_ACCESS)

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

// --- Prototypes ---
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriverObject);
NTSTATUS IoControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS CreateCall(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS CloseCall(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

// Helper for memory operations
NTSTATUS KeReadVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size) {
    PSIZE_T Bytes;
    return MmCopyVirtualMemory(Process, SourceAddress, IoGetCurrentProcess(), TargetAddress, Size, KernelMode, &Bytes);
}

NTSTATUS KeWriteVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size) {
    PSIZE_T Bytes;
    return MmCopyVirtualMemory(IoGetCurrentProcess(), SourceAddress, Process, TargetAddress, Size, KernelMode, &Bytes);
}

PDEVICE_OBJECT pDeviceObject;
UNICODE_STRING dev, dos;

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
    UNREFERENCED_PARAMETER(pRegistryPath);

    RtlInitUnicodeString(&dev, L"\\Device\\DayZStealth");
    RtlInitUnicodeString(&dos, L"\\DosDevices\\DayZStealth");

    IoCreateDevice(pDriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
    IoCreateSymbolicLink(&dos, &dev);

    pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;
    pDriverObject->DriverUnload = UnloadDriver;

    pDeviceObject->Flags |= DO_DIRECT_IO;
    pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriverObject) {
    UNREFERENCED_PARAMETER(pDriverObject);
    IoDeleteSymbolicLink(&dos);
    IoDeleteDevice(pDeviceObject);
    return STATUS_SUCCESS;
}

NTSTATUS CreateCall(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) {
    UNREFERENCED_PARAMETER(pDeviceObject);
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS CloseCall(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) {
    UNREFERENCED_PARAMETER(pDeviceObject);
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS IoControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) {
    UNREFERENCED_PARAMETER(pDeviceObject);
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG ByteCount = 0;
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG ControlCode = stack->Parameters.DeviceIoControl.IoControlCode;

    if (ControlCode == IO_READ_REQUEST) {
        PKERNEL_READ_REQUEST ReadInput = (PKERNEL_READ_REQUEST)pIrp->AssociatedIrp.SystemBuffer;
        PEPROCESS Process;
        if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)ReadInput->ProcessId, &Process))) {
            KeReadVirtualMemory(Process, (PVOID)ReadInput->Address, ReadInput->Response, ReadInput->Size);
            ObDereferenceObject(Process);
            Status = STATUS_SUCCESS;
            ByteCount = sizeof(KERNEL_READ_REQUEST);
        }
    }
    else if (ControlCode == IO_WRITE_REQUEST) {
        PKERNEL_WRITE_REQUEST WriteInput = (PKERNEL_WRITE_REQUEST)pIrp->AssociatedIrp.SystemBuffer;
        PEPROCESS Process;
        if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)WriteInput->ProcessId, &Process))) {
            KeWriteVirtualMemory(Process, WriteInput->Value, (PVOID)WriteInput->Address, WriteInput->Size);
            ObDereferenceObject(Process);
            Status = STATUS_SUCCESS;
            ByteCount = sizeof(KERNEL_WRITE_REQUEST);
        }
    }

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = ByteCount;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}
