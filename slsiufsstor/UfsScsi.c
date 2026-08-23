//
// Copyright (c) 2026 viZPilot
//

#include "UfsCommon.h"

NTSTATUS
SlsiUfsBuildTestUnitReady(
    _Out_ PUFS_COMMAND_UPIU Upiu,
    _In_ UCHAR Lun,
    _In_ UCHAR TaskTag
)
{
    UCHAR cdb[6];

    RtlZeroMemory(cdb, sizeof(cdb));

    //
    // Operation Code
    //
    cdb[0] = 0x00;

    return SlsiUfsBuildCommandUpiu(
        Upiu,
        cdb,
        sizeof(cdb),
        0,
        Lun,
        TaskTag);
}

NTSTATUS
SlsiUfsBuildInquiry(
    _Out_ PUFS_COMMAND_UPIU Upiu,
    _In_ UCHAR Lun,
    _In_ UCHAR TaskTag,
    _In_ USHORT AllocationLength
)
{
    UCHAR cdb[6];

    RtlZeroMemory(
        cdb,
        sizeof(cdb));

    //
    // SCSI INQUIRY
    //
    cdb[0] = 0x12;

    //
    // EVPD = 0
    // Standard Inquiry Data
    //
    cdb[1] = 0x00;

    //
    // Page Code
    //
    cdb[2] = 0x00;

    //
    // Allocation Length (Big Endian)
    //
    cdb[3] = (UCHAR)((AllocationLength >> 8) & 0xFF);
    cdb[4] = (UCHAR)(AllocationLength & 0xFF);

    //
    // Control
    //
    cdb[5] = 0x00;

    return SlsiUfsBuildCommandUpiu(
        Upiu,
        cdb,
        sizeof(cdb),
        AllocationLength,
        Lun,
        TaskTag);
}


NTSTATUS
SlsiUfsBuildCommandDescriptor(
    _Out_ PUFS_COMMAND_DESCRIPTOR Descriptor,
    _In_ PUFS_COMMAND_UPIU CommandUpiu
)
{
    if (Descriptor == NULL ||
        CommandUpiu == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(
        Descriptor,
        sizeof(UFS_COMMAND_DESCRIPTOR));

    RtlCopyMemory(
        &Descriptor->CommandUpiu,
        CommandUpiu,
        sizeof(UFS_COMMAND_UPIU));

    return STATUS_SUCCESS;
}

VOID
SlsiUfsTestInquiry(
    VOID
)
{
    UFS_COMMAND_UPIU commandUpiu;
    UFS_COMMAND_DESCRIPTOR descriptor;

    NTSTATUS status;

    SlsiUfsBuildInquiry(
        &commandUpiu,
        0,
        1,
        36);

    SlsiUfsBuildCommandDescriptor(
        &descriptor,
        &commandUpiu);

    if (NT_SUCCESS(status))
    {
        KdPrintEx((
            DPFLTR_IHVDRIVER_ID,
            DPFLTR_INFO_LEVEL,
            "INQUIRY CDB: %02X %02X %02X %02X %02X %02X\n",
            commandUpiu.Cdb[0],
            commandUpiu.Cdb[1],
            commandUpiu.Cdb[2],
            commandUpiu.Cdb[3],
            commandUpiu.Cdb[4],
            commandUpiu.Cdb[5]
            ));
    }
}

NTSTATUS
SlsiUfsPrepareInquiryBuffer(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Context->InquiryBuffer = NULL;
    Context->InquiryBufferLength = 0;
    Context->InquiryBufferPhysical.QuadPart = 0;

    //
    // La reserva DMA real la conectaremos aquí.
    //
    // Por ahora esta función solamente establece
    // el tamaño requerido.
    //
    Context->InquiryBufferLength =
        UFS_INQUIRY_DATA_LENGTH;

    return STATUS_SUCCESS;
}

NTSTATUS
SlsiUfsInitializeInquiryPrdt(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context
)
{
    PUFS_PRDT prdt;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->CommandDescriptor == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (Context->InquiryBuffer == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    prdt =
        &Context->CommandDescriptor->PrdtTable[0];

    RtlZeroMemory(
        prdt,
        sizeof(UFS_PRDT)
    );

    prdt->BaseAddressLow =
        Context->InquiryBufferPhysical.LowPart;

    prdt->BaseAddressHigh =
        Context->InquiryBufferPhysical.HighPart;

    prdt->Reserved = 0;

    prdt->Size =
        UFS_INQUIRY_DATA_LENGTH;

    return STATUS_SUCCESS;
}

NTSTATUS
SlsiUfsSubmitTransferRequest(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Slot
)
{
    ULONG doorbell;

    if (Context == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Context->UtrdList == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (Slot >= UFS_MAX_TRANSFER_REQUESTS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    doorbell = 1UL << Slot;

    SlsiUfsWriteRegister(
        Context,
        REG_UTP_TRANSFER_REQ_DOOR_BELL,
        doorbell
    );

    return STATUS_SUCCESS;
}