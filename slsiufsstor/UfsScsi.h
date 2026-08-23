//
// Copyright (c) 2026 viZPilot
//

#pragma once

#define UFS_INQUIRY_DATA_LENGTH 36

NTSTATUS
SlsiUfsBuildTestUnitReady(
    _Out_ PUFS_COMMAND_UPIU Upiu,
    _In_ UCHAR Lun,
    _In_ UCHAR TaskTag
);

NTSTATUS
SlsiUfsBuildCommandDescriptor(
    _Out_ PUFS_COMMAND_DESCRIPTOR Descriptor,
    _In_ PUFS_COMMAND_UPIU CommandUpiu
);

NTSTATUS
SlsiUfsBuildInquiry(
    _Out_ PUFS_COMMAND_UPIU Upiu,
    _In_ UCHAR Lun,
    _In_ UCHAR TaskTag,
    _In_ USHORT AllocationLength
);

VOID
SlsiUfsTestInquiry(
    VOID
);
