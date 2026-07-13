//
// Copyright (c) 2026 viZPilot
//

#pragma once

#include <ntddk.h>
#include "Driver.h"

ULONG
SlsiExynosReadRegister(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Offset
);

VOID
SlsiExynosWriteRegister(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Offset,
    _In_ ULONG Value
);
