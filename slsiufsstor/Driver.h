//
// Copyright (c) 2026 viZPilot
//

#pragma once
#include "UfsState.h"

typedef struct _SLSI_UFS_DEVICE_CONTEXT
{
    //
    // MMIO Resources
    //
    PHYSICAL_ADDRESS RegistersBase;
    ULONG RegistersLength;
    PVOID Registers;

    //
    // Interruptors
    //
    ULONG InterruptLevel;
    ULONG InterruptVector;
    KAFFINITY InterruptAffinity;

    //
    // WDF Resources
    //
    WDFINTERRUPT Interrupt;

    //
    // UFS Status
    //
    SLSI_UFS_STATE State;

    BOOLEAN ControllerInitialized;
    BOOLEAN DevicePresent;

    ULONG UfsVersion;
    ULONG Capabilities;
} SLSI_UFS_DEVICE_CONTEXT, * PSLSI_UFS_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    SLSI_UFS_DEVICE_CONTEXT,
    SlsiUfsGetContext
);

