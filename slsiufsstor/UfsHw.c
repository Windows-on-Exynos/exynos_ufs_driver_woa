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

NTSTATUS
SlsiUfsInitializeController(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    UNREFERENCED_PARAMETER(Context);

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

    Context->ControllerInitialized = TRUE;
    Context->State = UfsStateReady;

    return STATUS_SUCCESS;
}