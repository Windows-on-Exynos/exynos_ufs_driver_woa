//
// Copyright (c) 2026 viZPilot
//

#include <ntddk.h>
#include <wdf.h>

#include "UfsHw.h"
#include "UfsState.h"
#include "Driver.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KmdfSlsiUfsStorEvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE SlsiUfsStorEvtPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE SlsiUfsStorEvtReleaseHardware;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_ PUNICODE_STRING    RegistryPath
)
{
    // NTSTATUS variable to record success or failure
    NTSTATUS status = STATUS_SUCCESS;

    // Allocate the driver configuration object
    WDF_DRIVER_CONFIG config;

    // Print "Hello World" for DriverEntry
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SlsiUfsStor: DriverEntry\n"));

    // Initialize the driver configuration object to register the
    // entry point for the EvtDeviceAdd callback, KmdfHelloWorldEvtDeviceAdd
    WDF_DRIVER_CONFIG_INIT(&config,
        KmdfSlsiUfsStorEvtDeviceAdd
    );

    // Finally, create the driver object
    status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );
    return status;
}

NTSTATUS
SlsiUfsStorEvtPrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);

    NTSTATUS status;

    PSLSI_UFS_DEVICE_CONTEXT context;

    context = SlsiUfsGetContext(Device);

    ULONG count;
    ULONG i;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "SlsiUfsStor: PrepareHardware\n"));

    if (ResourcesTranslated == NULL)
    {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    count = WdfCmResourceListGetCount(ResourcesTranslated);

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "SlsiUfsStor: %lu translated resources\n",
        count));

    context->State = UfsStateReset;
    context->ControllerInitialized = FALSE;
    context->DevicePresent = FALSE;

    for (i = 0; i < count; i++)
    {
        descriptor =
            WdfCmResourceListGetDescriptor(
                ResourcesTranslated,
                i);

        if (descriptor == NULL)
        {
            continue;
        }

        switch (descriptor->Type)
        {
        case CmResourceTypeMemory:

            context->RegistersBase = descriptor->u.Memory.Start;
            context->RegistersLength = descriptor->u.Memory.Length;

            break;

        case CmResourceTypeInterrupt:

            context->InterruptVector = descriptor->u.Interrupt.Vector;
            context->InterruptLevel = descriptor->u.Interrupt.Level;
            context->InterruptAffinity = descriptor->u.Interrupt.Affinity;

            break;

        default:

            KdPrintEx((DPFLTR_IHVDRIVER_ID,
                DPFLTR_INFO_LEVEL,
                "Resource type %u\n",
                descriptor->Type));

            break;
        }
    }

    if (context->RegistersLength == 0)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,
            DPFLTR_ERROR_LEVEL,
            "SlsiUfsStor: No MMIO resource found\n"));

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (context->Registers != NULL)
    {
        MmUnmapIoSpace(
            context->Registers,
            context->RegistersLength
        );

        context->Registers = NULL;
    }

    context->Registers =
        MmMapIoSpaceEx(
            context->RegistersBase,
            context->RegistersLength,
            PAGE_READWRITE | PAGE_NOCACHE
        );

    if (context->Registers == NULL)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,
            DPFLTR_ERROR_LEVEL,
            "SlsiUfsStor: Failed to map MMIO\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "SlsiUfsStor: MMIO PA %llX Length %lu\n",
        context->RegistersBase.QuadPart,
        context->RegistersLength));

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "SlsiUfsStor: MMIO mapped\n"));

    context->State = UfsStateInitializing;

    status = SlsiUfsInitializeController(context);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SlsiUfsStorEvtReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PSLSI_UFS_DEVICE_CONTEXT context;

    context = SlsiUfsGetContext(Device);

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "SlsiUfsStor: ReleaseHardware\n"));

    if (context->Registers != NULL)
    {
        MmUnmapIoSpace(
            context->Registers,
            context->RegistersLength
        );

        context->Registers = NULL;
    }

    context->RegistersLength = 0;
    context->RegistersBase.QuadPart = 0;

    context->ControllerInitialized = FALSE;
    context->DevicePresent = FALSE;
    context->State = UfsStateReset;

    return STATUS_SUCCESS;
}

NTSTATUS
KmdfSlsiUfsStorEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    // We're not using the driver object,
    // so we need to mark it as unreferenced
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;

    // Allocate the device object
    WDFDEVICE hDevice;

    // Print "Hello World"
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SlsiUfsStor: KmdfSlsiUfsStorEvtDeviceAdd\n"));

    WDF_OBJECT_ATTRIBUTES deviceAttributes;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &deviceAttributes,
        SLSI_UFS_DEVICE_CONTEXT
    );

    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

    pnpCallbacks.EvtDevicePrepareHardware =
        SlsiUfsStorEvtPrepareHardware;

    pnpCallbacks.EvtDeviceReleaseHardware =
        SlsiUfsStorEvtReleaseHardware;

    WdfDeviceInitSetPnpPowerEventCallbacks(
        DeviceInit,
        &pnpCallbacks
    );

    status = WdfDeviceCreate(
        &DeviceInit,
        &deviceAttributes,
        &hDevice
    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return status;
}
