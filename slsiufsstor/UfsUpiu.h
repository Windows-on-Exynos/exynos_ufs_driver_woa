#pragma once

#include <ntddk.h>

#define UFS_TRANSACTION_SPECIFIC_LENGTH   20

//
// El tamaño final del UPIU depende del Command Descriptor.
// Por ahora podemos dejar un buffer provisional.
//
#define UFS_UPIU_DATA_LENGTH              256

#pragma pack(push, 1)

//
// UPIU Header
//
typedef struct _UFS_UPIU_HEADER
{
    //
    // DW0
    //
    UCHAR Type;
    UCHAR Flags;
    UCHAR Lun;
    UCHAR TaskTag;

    //
    // DW1
    //
    UCHAR CommandSetType;
    UCHAR Function;
    UCHAR Response;
    UCHAR Status;

    //
    // DW2
    //
    UCHAR EhsLength;
    UCHAR DeviceInfo;
    USHORT DataLength;

} UFS_UPIU_HEADER, * PUFS_UPIU_HEADER;

//
// Generic UPIU
//
typedef struct _UFS_UPIU
{
    UFS_UPIU_HEADER Header;

    //
    // DW3-DW7
    //
    UCHAR TransactionSpecific[UFS_TRANSACTION_SPECIFIC_LENGTH];

    //
    // Payload
    //
    UCHAR Data[UFS_UPIU_DATA_LENGTH];

} UFS_UPIU, * PUFS_UPIU;

//
// Physical Region Descriptor Table Entry.
//
typedef struct _UFS_PRDT
{
    ULONG BaseAddressLow;
    ULONG BaseAddressHigh;
    ULONG Reserved;
    ULONG Size;

} UFS_PRDT, * PUFS_PRDT;

//
// UTP Transfer Request Descriptor (UTRD)
//
typedef struct _UFS_UTRD
{
    //
    // DW0-DW3
    //
    ULONG Dw0;
    ULONG Dw1;
    ULONG Dw2;
    ULONG Dw3;

    //
    // DW4-DW5
    //
    ULONG CommandDescriptorBaseAddressLow;
    ULONG CommandDescriptorBaseAddressHigh;

    //
    // DW6
    //
    USHORT ResponseUpiuLength;
    USHORT ResponseUpiuOffset;

    //
    // DW7
    //
    USHORT PrdtLength;
    USHORT PrdtOffset;

} UFS_UTRD, * PUFS_UTRD;

//
// UTP Command Descriptor
//
typedef struct _UFS_COMMAND_DESCRIPTOR
{
    //
    // Command Request UPIU
    //
    UFS_UPIU CommandUpiu;

    //
    // Command Response UPIU
    //
    UFS_UPIU ResponseUpiu;

    //
    // Physical Region Descriptor Table.
    //
    UFS_PRDT PrdtTable[128];

} UFS_COMMAND_DESCRIPTOR, * PUFS_COMMAND_DESCRIPTOR;

typedef struct _UFS_COMMAND_UPIU
{
    UFS_UPIU_HEADER Header;

    //
    // DW3-DW7
    //
    UCHAR ExpectedDataTransferLength[4];

    UCHAR Reserved1[4];

    UCHAR Cdb[16];

} UFS_COMMAND_UPIU, * PUFS_COMMAND_UPIU;

#define UPIU_TRANSACTION_COMMAND       0x01

#define UFS_MAX_TRANSFER_REQUESTS      32
#define UFS_MAX_TASK_REQUESTS          8

#define UFS_RESPONSE_UPIU_OFFSET \
    FIELD_OFFSET(UFS_COMMAND_DESCRIPTOR, ResponseUpiu)

#define UFS_PRDT_OFFSET \
    FIELD_OFFSET(UFS_COMMAND_DESCRIPTOR, PrdtTable)

NTSTATUS
SlsiUfsInitializeTransferRequest(
    _In_ PSLSI_UFS_DEVICE_CONTEXT Context,
    _In_ ULONG Slot
);

NTSTATUS
SlsiUfsBuildCommandUpiu(
    _Out_ PUFS_COMMAND_UPIU Upiu,
    _In_reads_bytes_(CdbLength) PUCHAR Cdb,
    _In_ UCHAR CdbLength,
    _In_ ULONG TransferLength,
    _In_ UCHAR Lun,
    _In_ UCHAR TaskTag
);

#pragma pack(pop)