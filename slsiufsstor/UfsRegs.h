//
// Copyright (c) 2026 viZPilot
//

#pragma once

//
// UFSHCI Registers (I took them from Little Kernel (LK) for Exynos 9610)
//

#define UFSHCI_REG_INTERRUPT_STATUS    0x20
#define UFSHCI_REG_INTERRUPT_ENABLE    0x24

#define UFSHCI_REG_CONTROLLER_STATUS   0x30
#define UFSHCI_REG_CONTROLLER_ENABLE   0x34

//
// Register UIC Commands (I took them from Little Kernel (LK) for Exynos 9610)
//
#define REG_UIC_COMMAND		  0x90
#define REG_UIC_COMMAND_ARG_1 0x94
#define REG_UIC_COMMAND_ARG_2 0x98
#define REG_UIC_COMMAND_ARG_3 0x9C

//
// UIC DME Commands (I took them from Little Kernel (LK) for Exynos 9610)
//
#define UIC_CMD_DME_GET		     0x01
#define UIC_CMD_DME_SET		     0x02
#define UIC_CMD_DME_PEER_GET     0x03
#define UIC_CMD_DME_PEER_SET     0x04
#define UIC_CMD_DME_POWERON	     0x10
#define UIC_CMD_DME_POWEROFF	 0x11
#define UIC_CMD_DME_ENABLE	     0x12
#define UIC_CMD_DME_RESET	     0x14
#define UIC_CMD_DME_END_PT_RST	 0x15
#define UIC_CMD_DME_LINK_STARTUP 0x16
#define UIC_CMD_DME_HIBER_ENTER  0x17
#define UIC_CMD_DME_HIBER_EXIT   0x18
#define UIC_CMD_DME_TEST_MODE	 0x1A
#define UIC_CMD_WAIT			 0x80
#define UIC_CMD_WAIT_ISR		 0x90
#define PHY_PMA_COMN_SET		 0xF0
#define PHY_PMA_TRSV_SET		 0xF1
#define PHY_PMA_COMN_WAIT		 0xF2
#define PHY_PMA_TRSV_WAIT		 0xF3
#define UIC_CMD_REGISTER_SET	 0xFF