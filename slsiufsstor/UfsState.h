//
// Copyright (c) 2026 viZPilot
//

#pragma once

typedef struct _SLSI_UFS_DEVICE_CONTEXT SLSI_UFS_DEVICE_CONTEXT, * PSLSI_UFS_DEVICE_CONTEXT;

typedef enum _SLSI_UFS_STATE
{
    UfsStateUnknown = 0,
    UfsStateReset,
    UfsStateInitializing,
    UfsStateLinkStartup,
    UfsStateDeviceInit,
    UfsStateReady,
    UfsStateError

} SLSI_UFS_STATE;


NTSTATUS
SlsiUfsSetState(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ SLSI_UFS_STATE State
);


SLSI_UFS_STATE
SlsiUfsGetState(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
);