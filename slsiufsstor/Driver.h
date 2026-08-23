//
// Copyright (c) 2026 viZPilot
//

#pragma once
#include <ntddk.h>
#include <wdf.h>

#include "UfsState.h"
#include "UfsUpiu.h"

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

    ULONG ControllerStatus;
    ULONG ControllerEnable;
    ULONG InterruptStatus;
    ULONG InterruptEnable;

    //
    // UTP Transfer Request List
    //
    PUFS_UTRD UtrdList;
    PHYSICAL_ADDRESS UtrdListPhysical;
    SIZE_T UtrdListLength;

    //
    // UTP Command Descriptor
    //
    PUFS_COMMAND_DESCRIPTOR CommandDescriptor;
    PHYSICAL_ADDRESS CommandDescriptorPhysical;
    SIZE_T CommandDescriptorLength;
} SLSI_UFS_DEVICE_CONTEXT, * PSLSI_UFS_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    SLSI_UFS_DEVICE_CONTEXT,
    SlsiUfsGetContext
);

