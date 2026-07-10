//
// Copyright (c) 2026 viZPilot
//

#include <ntddk.h>
#include <wdf.h>

#include "UfsState.h"
#include "Driver.h"

NTSTATUS
SlsiUfsSetState(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ SLSI_UFS_STATE State
)
{
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->State = State;

    return STATUS_SUCCESS;
}


SLSI_UFS_STATE
SlsiUfsGetState(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    if (Context == NULL)
    {
        return UfsStateError;
    }

    return Context->State;
}