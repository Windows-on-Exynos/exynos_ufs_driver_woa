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
