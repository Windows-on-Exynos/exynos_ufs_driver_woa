//
// Copyright (c) 2026 viZPilot
//

#include <ntddk.h>
#include "UfsHw.h"
#include "UfsRegs.h"
#include "ExynosUfsRegs.h"

ULONG
SlsiUfsReadRegister(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Offset
)
{
    return READ_REGISTER_ULONG(
        (PULONG)((PUCHAR)Context->Registers + Offset)
    );
}

VOID
SlsiUfsWriteRegister(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Offset,
    _In_ ULONG Value
)
{
    WRITE_REGISTER_ULONG(
        (PULONG)((PUCHAR)Context->Registers + Offset),
        Value
    );
}

ULONG
SlsiUfsGetControllerStatus(
    PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    return SlsiUfsReadRegister(
        Context,
        UFSHCI_REG_CONTROLLER_STATUS
    );
}

NTSTATUS
SlsiUfsInitializeController(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    if (Context == NULL || Context->Registers == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    Context->Capabilities =
        SlsiUfsReadRegister(
            Context,
            UFSHCI_REG_CAP
        );

    Context->UfsVersion =
        SlsiUfsReadRegister(
            Context,
            UFSHCI_REG_VER
        );

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "SlsiUfsStor: CAP = 0x%08X\n",
        Context->Capabilities));

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "SlsiUfsStor: VER = 0x%08X\n",
        Context->UfsVersion));

    if (Context->UfsVersion == 0xFFFFFFFF)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,
            DPFLTR_ERROR_LEVEL,
            "SlsiUfsStor: Invalid UFS version register\n"));

        return STATUS_DEVICE_HARDWARE_ERROR;
    }

    Context->State = UfsStateInitializing;

    return STATUS_SUCCESS;
}