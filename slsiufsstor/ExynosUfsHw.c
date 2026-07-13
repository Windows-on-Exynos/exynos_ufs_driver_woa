//
// Copyright (c) 2026 viZPilot
//

#include "ExynosUfsHw.h"
#include "ExynosUfsRegs.h"

ULONG
SlsiExynosReadRegister(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Offset
)
{
    return READ_REGISTER_ULONG(
        (PULONG)((PUCHAR)Context->Registers + Offset)
    );
}

VOID
SlsiExynosWriteRegister(
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
