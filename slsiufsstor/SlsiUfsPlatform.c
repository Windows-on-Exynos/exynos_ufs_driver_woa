#include <ntddk.h>
#include "ufs_cal_adapter.h"

#define UFS_TAG '1SfU'

#define UFS_LOG0(_level, _message) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, (_level), \
               "slsiufsstor: " _message)
#define UFS_LOG(_level, _format, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, (_level), \
               "slsiufsstor: " _format, __VA_ARGS__)

/* Exynos9610 resources, verified against the troika kernel and LK port. */
#define UFS_HCI_PHYS            0x13520000ULL
#define UFS_HCI_SIZE            0x00000200UL
#define UFS_VS_PHYS             0x13521100ULL
#define UFS_VS_SIZE             0x00000200UL
#define UFS_UNIPRO_PHYS         0x13510000ULL
#define UFS_UNIPRO_SIZE         0x00008000UL
#define UFS_PMA_PHYS            0x13524000ULL
#define UFS_PMA_SIZE            0x00000800UL
#define PMU_PHYS                0x11860000ULL
#define PMU_SIZE                0x00001000UL
#define SYSREG_FSYS_PHYS        0x13410000ULL
#define SYSREG_FSYS_SIZE        0x00002000UL
#define GPIO_FSYS_PHYS          0x13490000ULL
#define GPIO_FSYS_SIZE          0x00001000UL
#define GPIO_TOP_PHYS           0x139B0000ULL
#define GPIO_TOP_SIZE           0x00001000UL

#define UFS_ACPI_GSIV           189UL
#define UFS_MCLK_HZ             166000000UL

/* System and GPIO registers. */
#define PMU_UFS_PHY_CTRL        0x0724UL
#define SYSREG_UFS_COHERENCY    0x1010UL
#define SYSREG_UFS_COHERENCY_M  ((1UL << 8) | (1UL << 9))
#define GPIO_FSYS_CON           0x0000UL
#define GPIO_FSYS_PUD           0x0008UL
#define GPIO_TOP_UFS_POWER      0x0144UL

/* Standard UFSHCI registers. */
#define HCI_CAP                 0x0000UL
#define HCI_VER                 0x0008UL
#define HCI_IS                  0x0020UL
#define HCI_IE                  0x0024UL
#define HCI_HCS                 0x0030UL
#define HCI_HCE                 0x0034UL
#define HCI_UICCMD              0x0090UL
#define HCI_UICARG1             0x0094UL
#define HCI_UICARG2             0x0098UL
#define HCI_UICARG3             0x009CUL

#define HCS_DEVICE_PRESENT      (1UL << 0)
#define HCS_UIC_COMMAND_READY   (1UL << 3)
#define IS_UIC_ERROR            (1UL << 2)
#define IS_UIC_POWER_MODE       (1UL << 4)
#define IS_UIC_LINK_STARTUP     (1UL << 8)
#define IS_UIC_COMMAND_COMPLETE (1UL << 10)
#define IS_FATAL_ERRORS         ((1UL << 7) | (1UL << 11) | \
                                 (1UL << 16) | (1UL << 17))

#define UIC_CMD_DME_GET         0x01UL
#define UIC_CMD_DME_SET         0x02UL
#define UIC_CMD_DME_PEER_SET    0x04UL
#define UIC_CMD_DME_LINK_START  0x16UL
#define PA_AVAIL_RX_LANES       (0x1540UL << 16)

/* Exynos vendor HCI registers, relative to UFS_VS_PHYS. */
#define VS_TXPRDT_ENTRY_SIZE    0x000UL
#define VS_RXPRDT_ENTRY_SIZE    0x004UL
#define VS_1US_TO_CNT_VAL       0x00CUL
#define VS_INVALID_UPIU_CTRL    0x010UL
#define VS_IS                   0x038UL
#define VS_UTRL_NEXUS_TYPE      0x040UL
#define VS_UTMRL_NEXUS_TYPE     0x044UL
#define VS_SW_RST               0x050UL
#define VS_DATA_REORDER         0x060UL
#define VS_AXI_DMA_BURST_LEN    0x06CUL
#define VS_GPIO_OUT             0x070UL
#define VS_UFSHCI_V2P1_CTRL     0x08CUL
#define VS_CLKSTOP_CTRL         0x0B0UL
#define VS_FORCE_HCS            0x0B4UL
#define VS_UFS_ACG_DISABLE      0x0FCUL
#define VS_IOP_ACG_DISABLE      0x100UL
#define VS_CPORT_PTR            0x110UL
#define VS_CPORT_DATA           0x114UL

#define VS_SW_RESET_MASK        0x3UL
#define CLK_STOP_MASK           ((1UL << 4) | (1UL << 2) | \
                                 (1UL << 1) | (1UL << 0))
#define FORCE_HCS_CLOCK_MASK    0x0DE0UL
#define IA_TICK_SEL             (1UL << 16)
#define DMA_WLU_ENABLE          (1UL << 31)
#define DMA_BURST_MASK          ((0xFUL << 27) | 0xFUL)
#define DMA_BURST(_value)       (((_value) << 27) | (_value))
#define PRDT_STANDARD_SIZE      12UL

#define RESET_TIMEOUT_US        100000UL
#define UIC_TIMEOUT_US          500000UL

typedef struct _SLSI_UFS_EXTENSION {
    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT PhysicalDevice;
    PUCHAR Hci;
    PUCHAR Vs;
    PUCHAR Unipro;
    PUCHAR Pma;
    PUCHAR Pmu;
    PUCHAR SysregFsys;
    PUCHAR GpioFsys;
    PUCHAR GpioTop;
    struct ufs_cal_param Cal;
    NTSTATUS LastUicStatus;
    ULONG RawInterrupt;
    ULONG HciResourceLength;
    BOOLEAN Started;
} SLSI_UFS_EXTENSION, *PSLSI_UFS_EXTENSION;

DRIVER_INITIALIZE DriverEntry;
DRIVER_ADD_DEVICE SlsiUfsAddDevice;
DRIVER_UNLOAD SlsiUfsUnload;

static NTSTATUS SlsiUfsDispatchPass(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static NTSTATUS SlsiUfsDispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static NTSTATUS SlsiUfsDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp);

static __forceinline ULONG
RegisterRead(PUCHAR Base, ULONG Offset)
{
    return READ_REGISTER_ULONG((volatile ULONG *)(Base + Offset));
}

static __forceinline VOID
RegisterWrite(PUCHAR Base, ULONG Offset, ULONG Value)
{
    WRITE_REGISTER_ULONG((volatile ULONG *)(Base + Offset), Value);
}

static PUCHAR
MapPhysical(ULONGLONG Address, SIZE_T Length)
{
    PHYSICAL_ADDRESS physicalAddress;

    physicalAddress.QuadPart = Address;
    return (PUCHAR)MmMapIoSpaceEx(physicalAddress, Length,
                                  PAGE_READWRITE | PAGE_NOCACHE);
}

static VOID
SlsiUfsUnmapRegisters(PSLSI_UFS_EXTENSION Extension)
{
#define UNMAP_REGION(_field, _length)                  \
    do {                                                \
        if (Extension->_field != NULL) {                \
            MmUnmapIoSpace(Extension->_field, _length); \
            Extension->_field = NULL;                   \
        }                                               \
    } while (0)

    UNMAP_REGION(GpioTop, GPIO_TOP_SIZE);
    UNMAP_REGION(GpioFsys, GPIO_FSYS_SIZE);
    UNMAP_REGION(SysregFsys, SYSREG_FSYS_SIZE);
    UNMAP_REGION(Pmu, PMU_SIZE);
    UNMAP_REGION(Pma, UFS_PMA_SIZE);
    UNMAP_REGION(Unipro, UFS_UNIPRO_SIZE);
    UNMAP_REGION(Vs, UFS_VS_SIZE);
    UNMAP_REGION(Hci, UFS_HCI_SIZE);

#undef UNMAP_REGION
}

static NTSTATUS
SlsiUfsMapRegisters(PSLSI_UFS_EXTENSION Extension)
{
    Extension->Hci = MapPhysical(UFS_HCI_PHYS, UFS_HCI_SIZE);
    Extension->Vs = MapPhysical(UFS_VS_PHYS, UFS_VS_SIZE);
    Extension->Unipro = MapPhysical(UFS_UNIPRO_PHYS, UFS_UNIPRO_SIZE);
    Extension->Pma = MapPhysical(UFS_PMA_PHYS, UFS_PMA_SIZE);
    Extension->Pmu = MapPhysical(PMU_PHYS, PMU_SIZE);
    Extension->SysregFsys = MapPhysical(SYSREG_FSYS_PHYS,
                                        SYSREG_FSYS_SIZE);
    Extension->GpioFsys = MapPhysical(GPIO_FSYS_PHYS, GPIO_FSYS_SIZE);
    Extension->GpioTop = MapPhysical(GPIO_TOP_PHYS, GPIO_TOP_SIZE);

    if (Extension->Hci == NULL || Extension->Vs == NULL ||
        Extension->Unipro == NULL || Extension->Pma == NULL ||
        Extension->Pmu == NULL || Extension->SysregFsys == NULL ||
        Extension->GpioFsys == NULL || Extension->GpioTop == NULL) {
        SlsiUfsUnmapRegisters(Extension);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
WaitForRegister(PUCHAR Base,
                ULONG Offset,
                ULONG Mask,
                ULONG Expected,
                ULONG TimeoutMicroseconds)
{
    ULONG elapsed;

    for (elapsed = 0; elapsed < TimeoutMicroseconds; ++elapsed) {
        if ((RegisterRead(Base, Offset) & Mask) == Expected) {
            return STATUS_SUCCESS;
        }
        KeStallExecutionProcessor(1);
    }

    return STATUS_IO_TIMEOUT;
}

static NTSTATUS
SlsiUfsValidateResources(PIO_STACK_LOCATION Stack,
                         PSLSI_UFS_EXTENSION Extension)
{
    PCM_RESOURCE_LIST list;
    ULONG fullIndex;
    ULONG partialIndex;
    BOOLEAN foundHci = FALSE;
    BOOLEAN foundInterrupt = FALSE;

    Extension->RawInterrupt = 0;
    Extension->HciResourceLength = 0;

    list = Stack->Parameters.StartDevice.AllocatedResources;
    if (list == NULL) {
        UFS_LOG0(DPFLTR_ERROR_LEVEL, "START has no raw resources\n");
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    for (fullIndex = 0; fullIndex < list->Count; ++fullIndex) {
        PCM_PARTIAL_RESOURCE_LIST partial =
            &list->List[fullIndex].PartialResourceList;

        for (partialIndex = 0; partialIndex < partial->Count;
             ++partialIndex) {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor =
                &partial->PartialDescriptors[partialIndex];

            if (descriptor->Type == CmResourceTypeMemory &&
                descriptor->u.Memory.Start.QuadPart == UFS_HCI_PHYS) {
                Extension->HciResourceLength = descriptor->u.Memory.Length;
                foundHci = TRUE;
            } else if (descriptor->Type == CmResourceTypeInterrupt) {
                Extension->RawInterrupt = descriptor->u.Interrupt.Level;
                foundInterrupt = TRUE;
            }
        }
    }

    if (!foundHci || Extension->HciResourceLength < UFS_HCI_SIZE) {
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "invalid HCI resource: found=%u length=0x%lx\n",
                (ULONG)foundHci, Extension->HciResourceLength);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (!foundInterrupt || Extension->RawInterrupt != UFS_ACPI_GSIV) {
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "invalid ACPI interrupt: found=%u GSIV=%lu expected=%lu\n",
                (ULONG)foundInterrupt, Extension->RawInterrupt,
                UFS_ACPI_GSIV);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    UFS_LOG(DPFLTR_INFO_LEVEL,
            "resources verified: HCI length=0x%lx GSIV=%lu\n",
            Extension->HciResourceLength, Extension->RawInterrupt);
    return STATUS_SUCCESS;
}

static VOID
SlsiUfsConfigureSystem(PSLSI_UFS_EXTENSION Extension)
{
    ULONG value;

    /* Configure GPF0 reset/refclk pins as in platform/maestro9610/ufs.c. */
    value = RegisterRead(Extension->GpioFsys, GPIO_FSYS_PUD);
    RegisterWrite(Extension->GpioFsys, GPIO_FSYS_PUD, value & ~0xFFUL);
    value = RegisterRead(Extension->GpioFsys, GPIO_FSYS_CON);
    RegisterWrite(Extension->GpioFsys, GPIO_FSYS_CON,
                  (value & ~0xFFUL) | 0x33UL);

    /* Bypass MPHY isolation. */
    value = RegisterRead(Extension->Pmu, PMU_UFS_PHY_CTRL);
    RegisterWrite(Extension->Pmu, PMU_UFS_PHY_CTRL, value | 1UL);

    /* Match dma-coherent in the Linux DT: FSYS SYSREG bits 8 and 9. */
    value = RegisterRead(Extension->SysregFsys, SYSREG_UFS_COHERENCY);
    RegisterWrite(Extension->SysregFsys, SYSREG_UFS_COHERENCY,
                  value | SYSREG_UFS_COHERENCY_M);

    /* Enable the fixed UFS VCC rail. */
    value = RegisterRead(Extension->GpioTop, GPIO_TOP_UFS_POWER);
    RegisterWrite(Extension->GpioTop, GPIO_TOP_UFS_POWER, value | 1UL);
    KeStallExecutionProcessor(1000);
}

static VOID
SlsiUfsConfigureVendorHci(PSLSI_UFS_EXTENSION Extension)
{
    ULONG value;

    /* Keep all UFS internal clocks available during bring-up. */
    value = RegisterRead(Extension->Vs, VS_CLKSTOP_CTRL);
    RegisterWrite(Extension->Vs, VS_CLKSTOP_CTRL,
                  value & ~CLK_STOP_MASK);
    RegisterWrite(Extension->Vs, VS_FORCE_HCS, FORCE_HCS_CLOCK_MASK);

    value = RegisterRead(Extension->Vs, VS_UFS_ACG_DISABLE);
    RegisterWrite(Extension->Vs, VS_UFS_ACG_DISABLE, value | 1UL);
    value = RegisterRead(Extension->Vs, VS_IOP_ACG_DISABLE);
    RegisterWrite(Extension->Vs, VS_IOP_ACG_DISABLE, value & ~1UL);

    value = RegisterRead(Extension->Vs, VS_UFSHCI_V2P1_CTRL);
    RegisterWrite(Extension->Vs, VS_UFSHCI_V2P1_CTRL,
                  value | IA_TICK_SEL);
    RegisterWrite(Extension->Vs, VS_1US_TO_CNT_VAL,
                  (UFS_MCLK_HZ / 1000000UL) & 0x3FFUL);

    RegisterWrite(Extension->Vs, VS_DATA_REORDER, 0xAUL);
    RegisterWrite(Extension->Vs, VS_TXPRDT_ENTRY_SIZE,
                  PRDT_STANDARD_SIZE);
    RegisterWrite(Extension->Vs, VS_RXPRDT_ENTRY_SIZE,
                  PRDT_STANDARD_SIZE);
    RegisterWrite(Extension->Vs, VS_UTRL_NEXUS_TYPE, 0xFFFFFFFFUL);
    RegisterWrite(Extension->Vs, VS_UTMRL_NEXUS_TYPE, 0xFFFFFFFFUL);

    value = RegisterRead(Extension->Vs, VS_AXI_DMA_BURST_LEN);
    value &= ~DMA_BURST_MASK;
    value |= DMA_WLU_ENABLE | DMA_BURST(3UL);
    RegisterWrite(Extension->Vs, VS_AXI_DMA_BURST_LEN, value);

    RegisterWrite(Extension->Vs, VS_INVALID_UPIU_CTRL, 0UL);
    RegisterWrite(Extension->Vs, VS_CPORT_DATA, 0x22UL);
    RegisterWrite(Extension->Vs, VS_CPORT_PTR, 1UL);
}

static NTSTATUS
SlsiUfsResetController(PSLSI_UFS_EXTENSION Extension)
{
    ULONG value;
    NTSTATUS status;

    value = RegisterRead(Extension->Vs, VS_FORCE_HCS);
    if (((value >> 4) & 0xFUL) != 0) {
        RegisterWrite(Extension->Vs, VS_FORCE_HCS, 0UL);
    }

    RegisterWrite(Extension->Vs, VS_SW_RST, VS_SW_RESET_MASK);
    status = WaitForRegister(Extension->Vs, VS_SW_RST,
                             VS_SW_RESET_MASK, 0UL,
                             RESET_TIMEOUT_US);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    value = RegisterRead(Extension->Vs, VS_IS);
    if ((value & (1UL << 20)) != 0) {
        RegisterWrite(Extension->Vs, VS_IS, value);
    }

    /* Toggle the embedded UFS device reset after VCC is stable. */
    RegisterWrite(Extension->Vs, VS_GPIO_OUT, 0UL);
    KeStallExecutionProcessor(5);
    RegisterWrite(Extension->Vs, VS_GPIO_OUT, 1UL);

    RegisterWrite(Extension->Hci, HCI_HCE, 1UL);
    status = WaitForRegister(Extension->Hci, HCI_HCE, 1UL, 1UL,
                             RESET_TIMEOUT_US);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    SlsiUfsConfigureVendorHci(Extension);
    return WaitForRegister(Extension->Hci, HCI_HCS,
                           HCS_UIC_COMMAND_READY,
                           HCS_UIC_COMMAND_READY,
                           RESET_TIMEOUT_US);
}

static NTSTATUS
SlsiUfsSendUic(PSLSI_UFS_EXTENSION Extension,
               ULONG Command,
               ULONG Argument1,
               ULONG Argument3,
               PULONG Result)
{
    NTSTATUS status;
    ULONG interruptStatus = 0;
    ULONG elapsed;
    ULONG commandStatus;

    status = WaitForRegister(Extension->Hci, HCI_HCS,
                             HCS_UIC_COMMAND_READY,
                             HCS_UIC_COMMAND_READY,
                             UIC_TIMEOUT_US);
    if (!NT_SUCCESS(status)) {
        return STATUS_DEVICE_NOT_READY;
    }

    interruptStatus = RegisterRead(Extension->Hci, HCI_IS);
    if (interruptStatus != 0) {
        RegisterWrite(Extension->Hci, HCI_IS, interruptStatus);
    }

    RegisterWrite(Extension->Hci, HCI_UICARG1, Argument1);
    RegisterWrite(Extension->Hci, HCI_UICARG2, 0UL);
    RegisterWrite(Extension->Hci, HCI_UICARG3, Argument3);
    KeMemoryBarrier();
    RegisterWrite(Extension->Hci, HCI_UICCMD, Command);

    for (elapsed = 0; elapsed < UIC_TIMEOUT_US; ++elapsed) {
        interruptStatus = RegisterRead(Extension->Hci, HCI_IS);
        if ((interruptStatus & IS_UIC_COMMAND_COMPLETE) != 0) {
            break;
        }
        if ((interruptStatus & IS_FATAL_ERRORS) != 0) {
            break;
        }
        if (Command != UIC_CMD_DME_LINK_START &&
            (interruptStatus & IS_UIC_ERROR) != 0) {
            break;
        }
        KeStallExecutionProcessor(1);
    }

    if (interruptStatus != 0) {
        RegisterWrite(Extension->Hci, HCI_IS, interruptStatus);
    }

    if (elapsed == UIC_TIMEOUT_US) {
        return STATUS_IO_TIMEOUT;
    }
    if ((interruptStatus & IS_FATAL_ERRORS) != 0) {
        return STATUS_DEVICE_HARDWARE_ERROR;
    }
    if (Command != UIC_CMD_DME_LINK_START &&
        (interruptStatus & IS_UIC_ERROR) != 0) {
        return STATUS_IO_DEVICE_ERROR;
    }

    commandStatus = RegisterRead(Extension->Hci, HCI_UICARG2) & 0xFFUL;
    if (commandStatus != 0) {
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "UIC command 0x%lx failed with status 0x%lx\n",
                Command, commandStatus);
        return STATUS_IO_DEVICE_ERROR;
    }

    if (Result != NULL) {
        *Result = RegisterRead(Extension->Hci, HCI_UICARG3);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
SlsiUfsInitializeController(PSLSI_UFS_EXTENSION Extension)
{
    NTSTATUS status;
    ufs_cal_errno calStatus;
    ULONG lanes = 0;
    ULONG capabilities;
    ULONG version;
    ULONG hostStatus;

    UFS_LOG0(DPFLTR_INFO_LEVEL, "starting Exynos9610 initialization\n");

    SlsiUfsUnmapRegisters(Extension);
    status = SlsiUfsMapRegisters(Extension);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    SlsiUfsConfigureSystem(Extension);
    status = SlsiUfsResetController(Extension);
    if (!NT_SUCCESS(status)) {
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "controller reset failed: 0x%08lx\n", (ULONG)status);
        goto Failure;
    }

    capabilities = RegisterRead(Extension->Hci, HCI_CAP);
    version = RegisterRead(Extension->Hci, HCI_VER);
    if (capabilities == 0xFFFFFFFFUL || version == 0xFFFFFFFFUL ||
        capabilities == 0UL || version == 0UL) {
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "invalid HCI registers: CAP=0x%08lx VER=0x%08lx\n",
                capabilities, version);
        status = STATUS_DEVICE_HARDWARE_ERROR;
        goto Failure;
    }

    RtlZeroMemory(&Extension->Cal, sizeof(Extension->Cal));
    Extension->Cal.host = Extension;
    Extension->Cal.mclk_rate = UFS_MCLK_HZ;
    Extension->Cal.board = 0;
    Extension->LastUicStatus = STATUS_SUCCESS;

    calStatus = ufs_cal_init(&Extension->Cal, 0);
    if (calStatus != UFS_CAL_NO_ERROR) {
        status = STATUS_DEVICE_HARDWARE_ERROR;
        goto Failure;
    }

    status = SlsiUfsSendUic(Extension, UIC_CMD_DME_GET,
                            PA_AVAIL_RX_LANES, 0UL, &lanes);
    if (!NT_SUCCESS(status) || lanes == 0UL || lanes > 2UL) {
        UFS_LOG(DPFLTR_WARNING_LEVEL,
                "lane query failed (status=0x%08lx value=%lu); using one lane\n",
                (ULONG)status, lanes);
        lanes = 1UL;
        Extension->LastUicStatus = STATUS_SUCCESS;
    }

    /* Both fields are required; target_lane==0 makes the CAL loop a no-op. */
    Extension->Cal.available_lane = (u8)lanes;
    Extension->Cal.target_lane = (u8)lanes;

    calStatus = ufs_cal_pre_link(&Extension->Cal);
    if (calStatus != UFS_CAL_NO_ERROR ||
        !NT_SUCCESS(Extension->LastUicStatus)) {
        status = NT_SUCCESS(Extension->LastUicStatus) ?
            STATUS_DEVICE_HARDWARE_ERROR : Extension->LastUicStatus;
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "pre-link calibration failed: cal=%lu status=0x%08lx\n",
                (ULONG)calStatus, (ULONG)status);
        goto Failure;
    }

    status = SlsiUfsSendUic(Extension, UIC_CMD_DME_LINK_START,
                            0UL, 0UL, NULL);
    if (!NT_SUCCESS(status)) {
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "link startup failed: 0x%08lx\n", (ULONG)status);
        goto Failure;
    }

    hostStatus = RegisterRead(Extension->Hci, HCI_HCS);
    if ((hostStatus & HCS_DEVICE_PRESENT) == 0) {
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "link completed without device present, HCS=0x%08lx\n",
                hostStatus);
        status = STATUS_DEVICE_NOT_CONNECTED;
        goto Failure;
    }

    Extension->LastUicStatus = STATUS_SUCCESS;
    calStatus = ufs_cal_post_link(&Extension->Cal);
    if (calStatus != UFS_CAL_NO_ERROR ||
        !NT_SUCCESS(Extension->LastUicStatus)) {
        status = NT_SUCCESS(Extension->LastUicStatus) ?
            STATUS_DEVICE_HARDWARE_ERROR : Extension->LastUicStatus;
        UFS_LOG(DPFLTR_ERROR_LEVEL,
                "post-link calibration failed: cal=%lu status=0x%08lx\n",
                (ULONG)calStatus, (ULONG)status);
        goto Failure;
    }

    /* StorUFS owns normal interrupts and the transfer lists from here. */
    RegisterWrite(Extension->Hci, HCI_IE, 0UL);
    hostStatus = RegisterRead(Extension->Hci, HCI_IS);
    if (hostStatus != 0) {
        RegisterWrite(Extension->Hci, HCI_IS, hostStatus);
    }

    Extension->Started = TRUE;
    UFS_LOG(DPFLTR_INFO_LEVEL,
            "initialized: CAP=0x%08lx VER=0x%08lx lanes=%lu HCS=0x%08lx\n",
            capabilities, version, lanes,
            RegisterRead(Extension->Hci, HCI_HCS));
    return STATUS_SUCCESS;

Failure:
    Extension->Started = FALSE;
    SlsiUfsUnmapRegisters(Extension);
    return status;
}

/* Adapter entry points used by the Samsung UFS CAL source. */
void
ufs_lld_dme_set(void *Host, u32 Address, u32 Value)
{
    PSLSI_UFS_EXTENSION extension = (PSLSI_UFS_EXTENSION)Host;
    NTSTATUS status;

    if (!NT_SUCCESS(extension->LastUicStatus)) {
        return;
    }
    status = SlsiUfsSendUic(extension, UIC_CMD_DME_SET,
                            Address, Value, NULL);
    if (!NT_SUCCESS(status)) {
        extension->LastUicStatus = status;
    }
}

void
ufs_lld_dme_get(void *Host, u32 Address, u32 *Value)
{
    PSLSI_UFS_EXTENSION extension = (PSLSI_UFS_EXTENSION)Host;
    NTSTATUS status;

    if (Value == NULL) {
        return;
    }
    *Value = 0;
    if (!NT_SUCCESS(extension->LastUicStatus)) {
        return;
    }
    status = SlsiUfsSendUic(extension, UIC_CMD_DME_GET,
                            Address, 0UL, Value);
    if (!NT_SUCCESS(status)) {
        extension->LastUicStatus = status;
    }
}

void
ufs_lld_dme_peer_set(void *Host, u32 Address, u32 Value)
{
    PSLSI_UFS_EXTENSION extension = (PSLSI_UFS_EXTENSION)Host;
    NTSTATUS status;

    if (!NT_SUCCESS(extension->LastUicStatus)) {
        return;
    }
    status = SlsiUfsSendUic(extension, UIC_CMD_DME_PEER_SET,
                            Address, Value, NULL);
    if (!NT_SUCCESS(status)) {
        extension->LastUicStatus = status;
    }
}

void
ufs_lld_pma_write(void *Host, u32 Value, u32 Address)
{
    PSLSI_UFS_EXTENSION extension = (PSLSI_UFS_EXTENSION)Host;
    RegisterWrite(extension->Pma, Address, Value);
}

u32
ufs_lld_pma_read(void *Host, u32 Address)
{
    PSLSI_UFS_EXTENSION extension = (PSLSI_UFS_EXTENSION)Host;
    return RegisterRead(extension->Pma, Address);
}

void
ufs_lld_unipro_write(void *Host, u32 Value, u32 Address)
{
    PSLSI_UFS_EXTENSION extension = (PSLSI_UFS_EXTENSION)Host;
    RegisterWrite(extension->Unipro, Address, Value);
}

void
ufs_lld_udelay(u32 Value)
{
    KeStallExecutionProcessor(Value);
}

void
ufs_lld_usleep_delay(u32 Minimum, u32 Maximum)
{
    UNREFERENCED_PARAMETER(Minimum);
    KeStallExecutionProcessor(Maximum);
}

unsigned long
ufs_lld_get_time_count(unsigned long Offset)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter = KeQueryPerformanceCounter(&frequency);
    ULONGLONG ticks = (ULONGLONG)counter.QuadPart;
    ULONGLONG ticksPerSecond = (ULONGLONG)frequency.QuadPart;
    ULONGLONG milliseconds =
        (ticks / ticksPerSecond) * 1000ULL +
        ((ticks % ticksPerSecond) * 1000ULL) / ticksPerSecond;
    return (unsigned long)milliseconds + Offset;
}

unsigned long
ufs_lld_calc_timeout(const unsigned int Milliseconds)
{
    return (unsigned long)Milliseconds;
}

static NTSTATUS
SlsiUfsSignalCompletion(PDEVICE_OBJECT DeviceObject,
                        PIRP Irp,
                        PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS
SlsiUfsForwardSynchronously(PSLSI_UFS_EXTENSION Extension, PIRP Irp)
{
    KEVENT event;
    NTSTATUS status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, SlsiUfsSignalCompletion, &event,
                           TRUE, TRUE, TRUE);

    status = IoCallDriver(Extension->LowerDevice, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    return Irp->IoStatus.Status;
}

static NTSTATUS
SlsiUfsDispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PSLSI_UFS_EXTENSION extension =
        (PSLSI_UFS_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        status = SlsiUfsForwardSynchronously(extension, Irp);
        if (NT_SUCCESS(status)) {
            status = SlsiUfsValidateResources(stack, extension);
        }
        if (NT_SUCCESS(status)) {
            status = SlsiUfsInitializeController(extension);
        }
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    case IRP_MN_STOP_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
        extension->Started = FALSE;
        SlsiUfsUnmapRegisters(extension);
        break;

    case IRP_MN_REMOVE_DEVICE:
        extension->Started = FALSE;
        SlsiUfsUnmapRegisters(extension);
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(extension->LowerDevice, Irp);
        IoDetachDevice(extension->LowerDevice);
        IoDeleteDevice(DeviceObject);
        return status;

    default:
        break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(extension->LowerDevice, Irp);
}

static NTSTATUS
SlsiUfsDispatchPass(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PSLSI_UFS_EXTENSION extension =
        (PSLSI_UFS_EXTENSION)DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(extension->LowerDevice, Irp);
}

static NTSTATUS
SlsiUfsDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PSLSI_UFS_EXTENSION extension =
        (PSLSI_UFS_EXTENSION)DeviceObject->DeviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(extension->LowerDevice, Irp);
}

NTSTATUS
SlsiUfsAddDevice(PDRIVER_OBJECT DriverObject,
                 PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT filterDevice = NULL;
    PSLSI_UFS_EXTENSION extension;
    NTSTATUS status;

    status = IoCreateDevice(DriverObject, sizeof(SLSI_UFS_EXTENSION),
                            NULL, FILE_DEVICE_UNKNOWN, 0, FALSE,
                            &filterDevice);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    extension = (PSLSI_UFS_EXTENSION)filterDevice->DeviceExtension;
    RtlZeroMemory(extension, sizeof(*extension));
    extension->PhysicalDevice = PhysicalDeviceObject;
    extension->LowerDevice = IoAttachDeviceToDeviceStack(filterDevice,
                                                          PhysicalDeviceObject);
    if (extension->LowerDevice == NULL) {
        IoDeleteDevice(filterDevice);
        return STATUS_NO_SUCH_DEVICE;
    }

    filterDevice->DeviceType = extension->LowerDevice->DeviceType;
    filterDevice->Characteristics = extension->LowerDevice->Characteristics;
    filterDevice->AlignmentRequirement =
        extension->LowerDevice->AlignmentRequirement;
    filterDevice->Flags |= extension->LowerDevice->Flags &
        (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
    filterDevice->Flags &= ~DO_DEVICE_INITIALIZING;

    UFS_LOG0(DPFLTR_INFO_LEVEL, "attached below StorUFS\n");
    return STATUS_SUCCESS;
}

void
SlsiUfsUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    ULONG index;

    UNREFERENCED_PARAMETER(RegistryPath);

    for (index = 0; index <= IRP_MJ_MAXIMUM_FUNCTION; ++index) {
        DriverObject->MajorFunction[index] = SlsiUfsDispatchPass;
    }
    DriverObject->MajorFunction[IRP_MJ_PNP] = SlsiUfsDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = SlsiUfsDispatchPower;
    DriverObject->DriverExtension->AddDevice = SlsiUfsAddDevice;
    DriverObject->DriverUnload = SlsiUfsUnload;

    UFS_LOG0(DPFLTR_INFO_LEVEL,
             "loaded (Windows 10 2004+, ARM64)\n");
    return STATUS_SUCCESS;
}
