//
// Copyright (c) 2026 viZPilot
//

#include "UfsCommon.h"

//
// I ported these functions from Little Kernel for Exynos 9610
//
NTSTATUS
SlsiUfsDmeSet(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Attribute,
    _In_ ULONG Value
)
{
    SLSI_UIC_COMMAND command;

    RtlZeroMemory(&command, sizeof(command));

    command.CmdOpcode = UIC_CMD_DME_SET;
    command.CmdArgument1 = Attribute;
    command.CmdArgument2 = 0;
    command.CmdArgument3 = Value;

    return SlsiUfsSendUicCommand(
        Context, &command);
}

NTSTATUS
SlsiUfsDmeGet(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Attribute,
    _Out_ PULONG Value
)
{
    NTSTATUS status;
    SLSI_UIC_COMMAND command;

    if (Value == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&command, sizeof(command));

    command.CmdOpcode = UIC_CMD_DME_GET;
    command.CmdArgument1 = Attribute;
    command.CmdArgument2 = 0;
    command.CmdArgument3 = 0;

    status = SlsiUfsSendUicCommand(
        Context, &command);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *Value = command.CmdArgument3;

    return STATUS_SUCCESS;
}

NTSTATUS
SlsiUfsDmePeerSet(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Attribute,
    _In_ ULONG Value
)
{
    SLSI_UIC_COMMAND command;

    RtlZeroMemory(&command, sizeof(command));

    command.CmdOpcode = UIC_CMD_DME_PEER_SET;
    command.CmdArgument1 = Attribute;
    command.CmdArgument2 = 0;
    command.CmdArgument3 = Value;

    return SlsiUfsSendUicCommand(
        Context, &command);
}

NTSTATUS
SlsiUfsDmePeerGet(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Attribute,
    _Out_ PULONG Value
)
{
    NTSTATUS status;
    SLSI_UIC_COMMAND command;

    if (Value == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&command, sizeof(command));

    command.CmdOpcode = UIC_CMD_DME_PEER_GET;
    command.CmdArgument1 = Attribute;
    command.CmdArgument2 = 0;
    command.CmdArgument3 = 0;

    status = SlsiUfsSendUicCommand(
        Context, &command);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *Value = command.CmdArgument3;

    return STATUS_SUCCESS;
}

NTSTATUS
SlsiUfsWaitUicCompletion(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    ULONG interruptStatus;
    ULONG timeout;

    if (Context == NULL || Context->Registers == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (timeout = 0;
        timeout < UFS_UIC_TIMEOUT_ITERATIONS;
        timeout++)
    {
        interruptStatus =
            SlsiUfsReadRegister(
                Context,
                UFSHCI_REG_INTERRUPT_STATUS);

        if (interruptStatus & UIC_COMMAND_COMPL)
        {
            //
            // Clear interrupt (Write-1-to-clear)
            //
            SlsiUfsWriteRegister(
                Context,
                UFSHCI_REG_INTERRUPT_STATUS,
                interruptStatus);

            KdPrintEx((DPFLTR_IHVDRIVER_ID,
                DPFLTR_INFO_LEVEL,
                "SlsiUfsStor: UIC command completed\n"));

            return STATUS_SUCCESS;
        }

        //
        // Small delay to avoid busy-looping the CPU.
        //
        KeStallExecutionProcessor(1);
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_ERROR_LEVEL,
        "SlsiUfsStor: UIC command timeout\n"));

    return STATUS_IO_TIMEOUT;
}

NTSTATUS
SlsiUfsSendUicCommand(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _Inout_ PSLSI_UIC_COMMAND Command
) 
{
    NTSTATUS status;
    ULONG uicStatus;

    SlsiUfsWriteRegister(Context,
        REG_UIC_COMMAND_ARG_1,
        Command->CmdArgument1);

    SlsiUfsWriteRegister(Context,
        REG_UIC_COMMAND_ARG_2,
        Command->CmdArgument2);

    SlsiUfsWriteRegister(Context,
        REG_UIC_COMMAND_ARG_3,
        Command->CmdArgument3);

    SlsiUfsWriteRegister(Context,
        REG_UIC_COMMAND,
        Command->CmdOpcode);

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "UIC: CMD=0x%08X ARG1=0x%08X ARG2=0x%08X ARG3=0x%08X\n",
        Command->CmdOpcode,
        Command->CmdArgument1,
        Command->CmdArgument2,
        Command->CmdArgument3));

    status = SlsiUfsWaitUicCompletion(Context);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    uicStatus =
        SlsiUfsReadRegister(
            Context,
            REG_UIC_COMMAND_ARG_2);

    if (uicStatus != 0)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID,
            DPFLTR_ERROR_LEVEL,
            "SlsiUfsStor: UIC command failed (0x%08X)\n",
            uicStatus));

        return STATUS_IO_DEVICE_ERROR;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "UIC: Status=0x%08X\n",
        uicStatus));

    if (Command->CmdOpcode == UIC_CMD_DME_GET ||
        Command->CmdOpcode == UIC_CMD_DME_PEER_GET)
    {
        Command->CmdArgument3 =
            SlsiUfsReadRegister(
                Context,
                REG_UIC_COMMAND_ARG_3);
        KdPrintEx((DPFLTR_IHVDRIVER_ID,
            DPFLTR_INFO_LEVEL,
            "UIC: DME_GET returned ARG3 = 0x%08X\n",
            Command->CmdArgument3));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SlsiUfsLinkStartup(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    NTSTATUS status;
    SLSI_UIC_COMMAND command;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->State = UfsStateLinkStartup;

    //
    // DME_RESET
    //
    RtlZeroMemory(&command, sizeof(command));

    command.CmdOpcode = UIC_CMD_DME_RESET;

    status = SlsiUfsSendUicCommand(Context, &command);

    if (!NT_SUCCESS(status))
    {
        Context->State = UfsStateError;
        return status;
    }

    //
    // DME_ENABLE
    //
    RtlZeroMemory(&command, sizeof(command));

    command.CmdOpcode = UIC_CMD_DME_ENABLE;

    status = SlsiUfsSendUicCommand(Context, &command);

    if (!NT_SUCCESS(status))
    {
        Context->State = UfsStateError;
        return status;
    }

    //
    // LINK_STARTUP
    //
    RtlZeroMemory(&command, sizeof(command));

    command.CmdOpcode = UIC_CMD_DME_LINK_STARTUP;

    status = SlsiUfsSendUicCommand(Context, &command);

    if (!NT_SUCCESS(status))
    {
        Context->State = UfsStateError;
        return status;
    }

    Context->ControllerInitialized = TRUE;
    Context->State = UfsStateDeviceInit;

    return STATUS_SUCCESS;
}
