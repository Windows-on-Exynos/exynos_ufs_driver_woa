#pragma once

#include "UfsHw.h"
#include "UfsRegs.h"
#include "UfsState.h"
#include "UfsUic.h"
#include "UfsUpiu.h"
#include "UfsScsi.h"
#include "ExynosUfsHw.h"
#include "ExynosUfsRegs.h"

#define UFS_UIC_TIMEOUT_ITERATIONS 1000000

#define UFS_TRANSFER_SLOT_0 0

#define UFS_TRANSFER_SLOT_BIT(Slot) \
    (1UL << (Slot))

//
// I took them from Android (downstream) kernel-slsi from Motorola
//

/*
 * Unipro attribute value
 */
#define TXTRAILINGCLOCKS	0x10
#define TACTIVATE_10_USEC	400	/* unit: 10us */

 /* Device ID */
#define DEV_ID	0x00
#define PEER_DEV_ID	0x01
#define PEER_CPORT_ID	0x00
#define TRAFFIC_CLASS	0x00

#define IATOVAL_NSEC		20000	/* unit: ns */

