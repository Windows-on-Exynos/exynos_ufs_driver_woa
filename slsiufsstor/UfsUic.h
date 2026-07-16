//
// Copyright (c) 2026 viZPilot
//

#pragma once

typedef struct _SLSI_UIC_COMMAND
{
    ULONG CmdOpcode;
    ULONG CmdArgument1;
    ULONG CmdArgument2;
    ULONG CmdArgument3;

} SLSI_UIC_COMMAND, * PSLSI_UIC_COMMAND;

NTSTATUS
SlsiUfsSendUicCommand(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ PSLSI_UIC_COMMAND Command
);
