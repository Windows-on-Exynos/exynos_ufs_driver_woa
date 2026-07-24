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
    _Out_ PUFS_UTRD Utrd,
    _In_ PHYSICAL_ADDRESS CommandDescriptorPhysical,
    _In_ USHORT PrdtEntries
)
{
    if (Context == NULL ||
        Utrd == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Clear the Transfer Request Descriptor.
    //
    RtlZeroMemory(
        Utrd,
        sizeof(UFS_UTRD));

    //
    // DW0-DW3
    // Reserved for now.
    //
    Utrd->Dw0 = 0;
    Utrd->Dw1 = 0;
    Utrd->Dw2 = 0;
    Utrd->Dw3 = 0;

    //
    // Physical address of the Command Descriptor.
    //
    Utrd->CommandDescriptorBaseAddressLow =
        CommandDescriptorPhysical.LowPart;

    Utrd->CommandDescriptorBaseAddressHigh =
        CommandDescriptorPhysical.HighPart;

    //
    // Response UPIU.
    //
    Utrd->ResponseUpiuOffset =
        (USHORT)FIELD_OFFSET(
            UFS_COMMAND_DESCRIPTOR,
            ResponseUpiu);

    Utrd->ResponseUpiuLength =
        sizeof(UFS_UPIU);

    //
    // PRDT.
    //
    Utrd->PrdtOffset =
        (USHORT)FIELD_OFFSET(
            UFS_COMMAND_DESCRIPTOR,
            PrdtTable);

    Utrd->PrdtLength = PrdtEntries;

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