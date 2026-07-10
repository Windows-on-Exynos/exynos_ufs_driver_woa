//
// Copyright (c) 2026 viZPilot
//

#pragma once

#include <ntddk.h>
#include <wdf.h>

#include "Driver.h"

ULONG
SlsiUfsReadRegister(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Offset
);

VOID
SlsiUfsWriteRegister(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Offset,
    _In_ ULONG Value
);

NTSTATUS
SlsiUfsInitializeController(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
);