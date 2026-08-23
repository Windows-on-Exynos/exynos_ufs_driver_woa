#include "UfsCommon.h"

VOID
SlsiUfsInitializeUpiuHeader(
    UFS_UPIU_HEADER *Header,
    UCHAR Type,
    UCHAR Lun,
    UCHAR TaskTag
)
{
    RtlZeroMemory(Header, sizeof(*Header));

    Header->Lun = Lun;
    Header->TaskTag = TaskTag;
}

NTSTATUS
SlsiUfsInitializeTransferRequest(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Slot
)
{
    PUFS_UTRD utrd;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->UtrdList == NULL ||
        Context->CommandDescriptor == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (Slot >= UFS_MAX_TRANSFER_REQUESTS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    utrd = &Context->UtrdList[Slot];

    RtlZeroMemory(
        utrd,
        sizeof(UFS_UTRD)
    );

    //
    // Command Descriptor physical address
    //
    utrd->CommandDescriptorBaseAddressLow =
        Context->CommandDescriptorPhysical.LowPart;

    utrd->CommandDescriptorBaseAddressHigh =
        Context->CommandDescriptorPhysical.HighPart;

    //
    // Response UPIU offset.
    //
    utrd->ResponseUpiuOffset =
        (USHORT)(
            FIELD_OFFSET(
                UFS_COMMAND_DESCRIPTOR,
                ResponseUpiu
            ) / sizeof(ULONG)
            );

    //
    // PRDT offset.
    //
    utrd->PrdtOffset =
        (USHORT)(
            FIELD_OFFSET(
                UFS_COMMAND_DESCRIPTOR,
                PrdtTable
            ) / sizeof(ULONG)
            );

    //
    // No PRDT entries yet.
    //
    utrd->PrdtLength = 0;

    //
    // Use the actual UPIU size expected by our descriptor.
    //
    utrd->ResponseUpiuLength =
        (USHORT)(
            sizeof(UFS_UPIU) / sizeof(ULONG)
            );

    return STATUS_SUCCESS;
}

NTSTATUS
SlsiUfsBuildCommandUpiu(
    _Out_ PUFS_COMMAND_UPIU Upiu,
    _In_reads_bytes_(CdbLength) PUCHAR Cdb,
    _In_ UCHAR CdbLength,
    _In_ ULONG TransferLength,
    _In_ UCHAR Lun,
    _In_ UCHAR TaskTag
)
{
    if (Upiu == NULL || Cdb == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // UFS admite CDB de hasta 16 bytes en un Command UPIU.
    //
    if (CdbLength > 16)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Limpiar completamente el UPIU.
    //
    RtlZeroMemory(
        Upiu,
        sizeof(UFS_COMMAND_UPIU));

    //
    // -----------------------------
    // UPIU Header (DW0)
    // -----------------------------
    //

    //
    // Command UPIU
    // (usa el define que tengas para este tipo)
    //
    Upiu->Header.Type = UPIU_TRANSACTION_COMMAND;

    //
    // Simple Task Attribute
    //
    Upiu->Header.Flags = 0;

    Upiu->Header.Lun = Lun;
    Upiu->Header.TaskTag = TaskTag;

    //
    // -----------------------------
    // DW1
    // -----------------------------
    //

    //
    // SCSI Command Set
    //
    Upiu->Header.CommandSetType = 0;

    Upiu->Header.Function = 0;
    Upiu->Header.Response = 0;
    Upiu->Header.Status = 0;

    //
    // -----------------------------
    // DW2
    // -----------------------------
    //

    Upiu->Header.EhsLength = 0;
    Upiu->Header.DeviceInfo = 0;

    //
    // Data Segment Length
    // (por ahora dejamos 0)
    //
    Upiu->Header.DataLength = 0;

    //
    // Expected Data Transfer Length
    // (Big Endian según UFS)
    //
    Upiu->ExpectedDataTransferLength[0] =
        (UCHAR)((TransferLength >> 24) & 0xFF);

    Upiu->ExpectedDataTransferLength[1] =
        (UCHAR)((TransferLength >> 16) & 0xFF);

    Upiu->ExpectedDataTransferLength[2] =
        (UCHAR)((TransferLength >> 8) & 0xFF);

    Upiu->ExpectedDataTransferLength[3] =
        (UCHAR)(TransferLength & 0xFF);

    //
    // Copiar el CDB SCSI.
    //
    RtlCopyMemory(
        Upiu->Cdb,
        Cdb,
        CdbLength);

    return STATUS_SUCCESS;
}