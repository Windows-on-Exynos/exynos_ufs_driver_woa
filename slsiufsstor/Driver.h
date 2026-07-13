//
// Copyright (c) 2026 viZPilot
//

#pragma once
#include <ntddk.h>
#include <wdf.h>

#include "UfsState.h"

//
// I took this from kernel-slsi (downstream kernel) for Motorola One Action (troika)
//

/*
 * Unipro attribute value
 */
#define TXTRAILINGCLOCKS	0x10
#define TACTIVATE_10_USEC	400	/* unit: 10us */

 /* Device ID */
#define DEV_ID	0x00
#define PEER_DEV_ID	0x01
#define PEER_CPORT_ID	0x00
#define TRAFFIC_CLASS	0x00

#define IATOVAL_NSEC		20000	/* unit: ns */


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
} SLSI_UFS_DEVICE_CONTEXT, * PSLSI_UFS_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    SLSI_UFS_DEVICE_CONTEXT,
    SlsiUfsGetContext
);

