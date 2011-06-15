///ethernet_test.h

//------------------------------------------------------------------------------
// LAN911X Defines 

/*
 ****************************************************************************
 ****************************************************************************
 *	TX/RX FIFO Port Register (Direct Address)
 *	Offset (from Base Address)
 *	and bit definitions
 ****************************************************************************
 ****************************************************************************
 */
#define RX_DATA_FIFO_PORT	(0x00UL)
#define TX_DATA_FIFO_PORT	(0x20UL)
	#define TX_CMD_A_INT_ON_COMP_		(0x80000000UL)
	#define TX_CMD_A_INT_BUF_END_ALGN_	(0x03000000UL)
	#define TX_CMD_A_INT_4_BYTE_ALGN_	(0x00000000UL)
	#define TX_CMD_A_INT_16_BYTE_ALGN_	(0x01000000UL)
	#define TX_CMD_A_INT_32_BYTE_ALGN_	(0x02000000UL)
	#define TX_CMD_A_INT_DATA_OFFSET_	(0x001F0000UL)
	#define TX_CMD_A_INT_FIRST_SEG_ 	(0x00002000UL)
	#define TX_CMD_A_INT_LAST_SEG_		(0x00001000UL)
	#define TX_CMD_A_BUF_SIZE_			(0x000007FFUL)

	#define TX_CMD_B_PKT_TAG_			(0xFFFF0000UL)
	#define TX_CMD_B_ADD_CRC_DISABLE_	(0x00002000UL)
	#define TX_CMD_B_DISABLE_PADDING_	(0x00001000UL)
	#define TX_CMD_B_PKT_BYTE_LENGTH_	(0x000007FFUL)

#define RX_STATUS_FIFO_PORT (0x00000040UL)
#define RX_STS_ES			(0x00008000UL)
#define RX_STS_LENGTH_ERR	(0x00001000UL)
#define RX_STS_MULTICAST	(0x00000400UL)
#define RX_STS_FRAME_TYPE	(0x00000020UL)
#define RX_FIFO_PEEK		(0x00000044UL)

#define TX_STS_ES			(0x00008000UL)

#define TX_STATUS_FIFO_PORT (0x00000048UL)
#define TX_FIFO_PEEK		(0x0000004CUL)

/*
 ****************************************************************************
 ****************************************************************************
 *	Slave Interface Module Control and Status Register (Direct Address)
 *	Offset (from Base Address)
 *	and bit definitions
 ****************************************************************************
 ****************************************************************************
 */
#define ID_REV				(0x50UL)
	#define ID_REV_CHIP_ID_ 	(0xFFFF0000UL)	// RO	default 0x011X
	#define ID_REV_REV_ID_		(0x0000FFFFUL)	// RO

#define INT_CFG 			(0x54UL)
	#define INT_CFG_INT_DEAS_	(0xFF000000UL)	// R/W
	#define INT_CFG_IRQ_INT_	(0x00001000UL)	// RO
	#define INT_CFG_IRQ_EN_ 	(0x00000100UL)	// R/W
	#define INT_CFG_IRQ_POL_	(0x00000010UL)	// R/W Not Affected by SW Reset
	#define INT_CFG_IRQ_TYPE_	(0x00000001UL)	// R/W Not Affected by SW Reset
	#define INT_CFG_IRQ_RESERVED_	(0x00FFCEEEUL)	//Reserved bits mask

#define INT_STS 			(0x58UL)
	#define INT_STS_SW_INT_ 	(0x80000000UL)	// R/WC
	#define INT_STS_TXSTOP_INT_ (0x02000000UL)	// R/WC
	#define INT_STS_RXSTOP_INT_ (0x01000000UL)	// R/WC
	#define INT_STS_RXDFH_INT_	(0x00800000UL)	// R/WC
	#define INT_STS_RXDF_INT_	(0x00400000UL)	// R/WC
	#define INT_STS_TX_IOC_ 	(0x00200000UL)	// R/WC
	#define INT_STS_RXD_INT_	(0x00100000UL)	// R/WC
	#define INT_STS_GPT_INT_	(0x00080000UL)	// R/WC
	#define INT_STS_PHY_INT_	(0x00040000UL)	// RO
	#define INT_STS_PME_INT_	(0x00020000UL)	// R/WC
	#define INT_STS_TXSO_		(0x00010000UL)	// R/WC
	#define INT_STS_RWT_		(0x00008000UL)	// R/WC
	#define INT_STS_RXE_		(0x00004000UL)	// R/WC
	#define INT_STS_TXE_		(0x00002000UL)	// R/WC
	#define INT_STS_ERX_		(0x00001000UL)	// R/WC
	#define INT_STS_TDFU_		(0x00000800UL)	// R/WC
	#define INT_STS_TDFO_		(0x00000400UL)	// R/WC
	#define INT_STS_TDFA_		(0x00000200UL)	// R/WC
	#define INT_STS_TSFF_		(0x00000100UL)	// R/WC
	#define INT_STS_TSFL_		(0x00000080UL)	// R/WC
	#define INT_STS_RDFO_		(0x00000040UL)	// R/WC
	#define INT_STS_RDFL_		(0x00000020UL)	// R/WC
	#define INT_STS_RSFF_		(0x00000010UL)	// R/WC
	#define INT_STS_RSFL_		(0x00000008UL)	// R/WC
	#define INT_STS_GPIO2_INT_	(0x00000004UL)	// R/WC
	#define INT_STS_GPIO1_INT_	(0x00000002UL)	// R/WC
	#define INT_STS_GPIO0_INT_	(0x00000001UL)	// R/WC

#define INT_EN				(0x5CUL)
	#define INT_EN_SW_INT_EN_		(0x80000000UL)	// R/W
	#define INT_EN_TXSTOP_INT_EN_	(0x02000000UL)	// R/W
	#define INT_EN_RXSTOP_INT_EN_	(0x01000000UL)	// R/W
	#define INT_EN_RXDFH_INT_EN_	(0x00800000UL)	// R/W
	#define INT_EN_RXDF_INT_EN_ 	(0x00400000UL)	// R/W
	#define INT_EN_TIOC_INT_EN_ 	(0x00200000UL)	// R/W
	#define INT_EN_RXD_INT_EN_		(0x00100000UL)	// R/W
	#define INT_EN_GPT_INT_EN_		(0x00080000UL)	// R/W
	#define INT_EN_PHY_INT_EN_		(0x00040000UL)	// R/W
	#define INT_EN_PME_INT_EN_		(0x00020000UL)	// R/W
	#define INT_EN_TXSO_EN_ 		(0x00010000UL)	// R/W
	#define INT_EN_RWT_EN_			(0x00008000UL)	// R/W
	#define INT_EN_RXE_EN_			(0x00004000UL)	// R/W
	#define INT_EN_TXE_EN_			(0x00002000UL)	// R/W
	#define INT_EN_ERX_EN_			(0x00001000UL)	// R/W
	#define INT_EN_TDFU_EN_ 		(0x00000800UL)	// R/W
	#define INT_EN_TDFO_EN_ 		(0x00000400UL)	// R/W
	#define INT_EN_TDFA_EN_ 		(0x00000200UL)	// R/W
	#define INT_EN_TSFF_EN_ 		(0x00000100UL)	// R/W
	#define INT_EN_TSFL_EN_ 		(0x00000080UL)	// R/W
	#define INT_EN_RDFO_EN_ 		(0x00000040UL)	// R/W
	#define INT_EN_RDFL_EN_ 		(0x00000020UL)	// R/W
	#define INT_EN_RSFF_EN_ 		(0x00000010UL)	// R/W
	#define INT_EN_RSFL_EN_ 		(0x00000008UL)	// R/W
	#define INT_EN_GPIO2_INT_		(0x00000004UL)	// R/W
	#define INT_EN_GPIO1_INT_		(0x00000002UL)	// R/W
	#define INT_EN_GPIO0_INT_		(0x00000001UL)	// R/W

#define DMA_CFG 			(0x60UL)
	#define DMA_CFG_DRQ1_DEAS_		(0xFF000000UL)	// R/W
	#define DMA_CFG_DMA1_MODE_		(0x00200000UL)	// R/W
	#define DMA_CFG_DMA1_FUNC_		(0x00180000UL)	// R/W
		#define DMA_CFG_DMA1_FUNC_DISABLED_ 	(0x000000000UL) // R/W
		#define DMA_CFG_DMA1_FUNC_TX_DMA_		(0x000800000UL) // R/W
		#define DMA_CFG_DMA1_FUNC_RX_DMA_		(0x001000000UL) // R/W
	#define DMA_CFG_DRQ1_BUFF_		(0x00040000UL)	// R/W
	#define DMA_CFG_DRQ1_POL_		(0x00020000UL)	// R/W
	#define DMA_CFG_DAK1_POL_		(0x00010000UL)	// R/W
	#define DMA_CFG_DRQ0_DEAS_		(0x0000FF00UL)	// R/W
	#define DMA_CFG_DMA0_MODE_		(0x00000020UL)	// R/W
	#define DMA_CFG_DMA0_FUNC_		(0x00000018UL)	// R/W
		#define DMA_CFG_DMA0_FUNC_FIFO_SEL_ 	(0x000000000UL) // R/W
		#define DMA_CFG_DMA0_FUNC_TX_DMA_		(0x000000008UL) // R/W
		#define DMA_CFG_DMA0_FUNC_RX_DMA_		(0x000000010UL) // R/W
	#define DMA_CFG_DRQ0_BUFF_		(0x00000004UL)	// R/W
	#define DMA_CFG_DRQ0_POL_		(0x00000002UL)	// R/W
	#define DMA_CFG_DAK0_POL_		(0x00000001UL)	// R/W

#define BYTE_TEST			(0x64UL)	// RO default 0x87654321

#define FIFO_INT			(0x68UL)
	#define FIFO_INT_TX_AVAIL_LEVEL_	(0xFF000000UL)	// R/W
	#define FIFO_INT_TX_STS_LEVEL_		(0x00FF0000UL)	// R/W
	#define FIFO_INT_RX_AVAIL_LEVEL_	(0x0000FF00UL)	// R/W
	#define FIFO_INT_RX_STS_LEVEL_		(0x000000FFUL)	// R/W

#define RX_CFG				(0x6CUL)
	#define RX_CFG_RX_END_ALGN_ 	(0xC0000000UL)	// R/W
		#define RX_CFG_RX_END_ALGN4_		(0x00000000UL)	// R/W
		#define RX_CFG_RX_END_ALGN16_		(0x40000000UL)	// R/W
		#define RX_CFG_RX_END_ALGN32_		(0x80000000UL)	// R/W
	#define RX_CFG_RX_DMA_CNT_		(0x0FFF0000UL)	// R/W
	#define RX_CFG_RX_DUMP_ 		(0x00008000UL)	// R/W
	#define RX_CFG_RXDOFF_			(0x00001F00UL)	// R/W
	#define RX_CFG_RXBAD_			(0x00000001UL)	// R/W

#define TX_CFG				(0x70UL)
	#define TX_CFG_TX_DMA_LVL_		(0xE0000000UL)	// R/W
	#define TX_CFG_TX_DMA_CNT_		(0x0FFF0000UL)	// R/W Self Clearing
	#define TX_CFG_TXS_DUMP_		(0x00008000UL)	// Self Clearing
	#define TX_CFG_TXD_DUMP_		(0x00004000UL)	// Self Clearing
	#define TX_CFG_TXSAO_			(0x00000004UL)	// R/W
	#define TX_CFG_TX_ON_			(0x00000002UL)	// R/W
	#define TX_CFG_STOP_TX_ 		(0x00000001UL)	// Self Clearing

#define HW_CFG				(0x74UL)
	#define HW_CFG_TTM_ 			(0x00200000UL)	// R/W
	#define HW_CFG_SF_				(0x00100000UL)	// R/W
	#define HW_CFG_TX_FIF_SZ_		(0x000F0000UL)	// R/W
	#define HW_CFG_TR_				(0x00003000UL)	// R/W
	#define HW_CFG_PHY_CLK_SEL_		(0x00000060UL)  // R/W
	#define   HW_CFG_PHY_CLK_SEL_INT_PHY_	(0x00000000UL) 	// R/W
	#define   HW_CFG_PHY_CLK_SEL_EXT_PHY_	(0x00000020UL) 	// R/W
	#define   HW_CFG_PHY_CLK_SEL_CLK_DIS_ 	(0x00000040UL) 	// R/W
	#define HW_CFG_SMI_SEL_			(0x00000010UL)  // R/W
	#define HW_CFG_EXT_PHY_DET_		(0x00000008UL)  // RO
	#define HW_CFG_EXT_PHY_EN_		(0x00000004UL)  // R/W
	#define HW_CFG_SRST_TO_			(0x00000002UL)  // RO
	#define HW_CFG_SRST_			(0x00000001UL)	// Self Clearing

#define RX_DP_CTL			(0x78UL)
	#define RX_DP_CTL_FFWD_BUSY_	(0x80000000UL)	// R/?
	#define RX_DP_CTL_RX_FFWD_		(0x00000FFFUL)	// R/W

#define RX_FIFO_INF 		(0x7CUL)
	#define RX_FIFO_INF_RXSUSED_	(0x00FF0000UL)	// RO
	#define RX_FIFO_INF_RXDUSED_	(0x0000FFFFUL)	// RO

#define TX_FIFO_INF 		(0x80UL)
	#define TX_FIFO_INF_TSFREE_ 	(0x00FF0000UL)	// RO for PAS V.1.3
	#define TX_FIFO_INF_TSUSED_ 	(0x00FF0000UL)	// RO for PAS V.1.4
	#define TX_FIFO_INF_TDFREE_ 	(0x0000FFFFUL)	// RO

#define PMT_CTRL			(0x84UL)
	#define PMT_CTRL_PM_MODE_			(0x00003000UL)	// Self Clearing
		#define PMT_CTRL_PM_MODE_GP_		(0x00003000UL)	// Self Clearing
		#define PMT_CTRL_PM_MODE_ED_		(0x00002000UL)	// Self Clearing
		#define PMT_CTRL_PM_MODE_WOL_		(0x00001000UL)	// Self Clearing
	#define PMT_CTRL_PHY_RST_			(0x00000400UL)	// Self Clearing
	#define PMT_CTRL_WOL_EN_			(0x00000200UL)	// R/W
	#define PMT_CTRL_ED_EN_ 			(0x00000100UL)	// R/W
	#define PMT_CTRL_PME_TYPE_			(0x00000040UL)	// R/W Not Affected by SW Reset
	#define PMT_CTRL_WUPS_				(0x00000030UL)	// R/WC
		#define PMT_CTRL_WUPS_NOWAKE_		(0x00000000UL)	// R/WC
		#define PMT_CTRL_WUPS_ED_			(0x00000010UL)	// R/WC
		#define PMT_CTRL_WUPS_WOL_			(0x00000020UL)	// R/WC
		#define PMT_CTRL_WUPS_MULTI_		(0x00000030UL)	// R/WC
	#define PMT_CTRL_PME_IND_		(0x00000008UL)	// R/W
	#define PMT_CTRL_PME_POL_		(0x00000004UL)	// R/W
	#define PMT_CTRL_PME_EN_		(0x00000002UL)	// R/W Not Affected by SW Reset
	#define PMT_CTRL_READY_ 		(0x00000001UL)	// RO

#define GPIO_CFG			(0x88UL)
	#define GPIO_CFG_LED3_EN_		(0x40000000UL)	// R/W
	#define GPIO_CFG_LED2_EN_		(0x20000000UL)	// R/W
	#define GPIO_CFG_LED1_EN_		(0x10000000UL)	// R/W
	#define GPIO_CFG_GPIO2_INT_POL_ (0x04000000UL)	// R/W
	#define GPIO_CFG_GPIO1_INT_POL_ (0x02000000UL)	// R/W
	#define GPIO_CFG_GPIO0_INT_POL_ (0x01000000UL)	// R/W
	#define GPIO_CFG_EEPR_EN_		(0x00700000UL)	// R/W
	#define GPIO_CFG_GPIOBUF2_		(0x00040000UL)	// R/W
	#define GPIO_CFG_GPIOBUF1_		(0x00020000UL)	// R/W
	#define GPIO_CFG_GPIOBUF0_		(0x00010000UL)	// R/W
	#define GPIO_CFG_GPIODIR2_		(0x00000400UL)	// R/W
	#define GPIO_CFG_GPIODIR1_		(0x00000200UL)	// R/W
	#define GPIO_CFG_GPIODIR0_		(0x00000100UL)	// R/W
	#define GPIO_CFG_GPIOD4_		(0x00000020UL)	// R/W
	#define GPIO_CFG_GPIOD3_		(0x00000010UL)	// R/W
	#define GPIO_CFG_GPIOD2_		(0x00000004UL)	// R/W
	#define GPIO_CFG_GPIOD1_		(0x00000002UL)	// R/W
	#define GPIO_CFG_GPIOD0_		(0x00000001UL)	// R/W

#define GPT_CFG 			(0x8CUL)
	#define GPT_CFG_TIMER_EN_		(0x20000000UL)	// R/W
	#define GPT_CFG_GPT_LOAD_		(0x0000FFFFUL)	// R/W

#define GPT_CNT 			(0x90UL)
	#define GPT_CNT_GPT_CNT_		(0x0000FFFFUL)	// RO

#define FPGA_REV			(0x94UL)
	#define FPGA_REV_FPGA_REV_		(0x0000FFFFUL)	// RO

#define ENDIAN				(0x98UL)	// R/W Not Affected by SW Reset

#define FREE_RUN			(0x9CUL)	// RO

#define RX_DROP 			(0xA0UL)	// R/WC

#define MAC_CSR_CMD 		(0xA4UL)
	#define MAC_CSR_CMD_CSR_BUSY_	(0x80000000UL)	// Self Clearing
	#define MAC_CSR_CMD_R_NOT_W_	(0x40000000UL)	// R/W
	#define MAC_CSR_CMD_CSR_ADDR_	(0x000000FFUL)	// R/W

#define MAC_CSR_DATA		(0xA8UL)	// R/W

#define AFC_CFG 			(0xACUL)
	#define AFC_CFG_AFC_HI_ 		(0x00FF0000UL)	// R/W
	#define AFC_CFG_AFC_LO_ 		(0x0000FF00UL)	// R/W
	#define AFC_CFG_BACK_DUR_		(0x000000F0UL)	// R/W
	#define AFC_CFG_FCMULT_ 		(0x00000008UL)	// R/W
	#define AFC_CFG_FCBRD_			(0x00000004UL)	// R/W
	#define AFC_CFG_FCADD_			(0x00000002UL)	// R/W
	#define AFC_CFG_FCANY_			(0x00000001UL)	// R/W

#define E2P_CMD 			(0xB0UL)
	#define E2P_CMD_EPC_BUSY_		(0x80000000UL)	// Self Clearing
	#define E2P_CMD_EPC_CMD_		(0x70000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_READ_	(0x00000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_EWDS_	(0x10000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_EWEN_	(0x20000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_WRITE_	(0x30000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_WRAL_	(0x40000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_ERASE_	(0x50000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_ERAL_	(0x60000000UL)	// R/W
		#define E2P_CMD_EPC_CMD_RELOAD_ (0x70000000UL)	// R/W
	#define E2P_CMD_EPC_TIMEOUT_	(0x08000000UL)	// RO
	#define E2P_CMD_E2P_PROG_GO_	(0x00000200UL)	// WO
	#define E2P_CMD_E2P_PROG_DONE_	(0x00000100UL)	// RO
	#define E2P_CMD_EPC_ADDR_		(0x000000FFUL)	// R/W

#define E2P_DATA			(0xB4UL)
	#define E2P_DATA_EEPROM_DATA_	(0x000000FFUL)	// R/W

#define TEST_REG_A			(0xC0UL)
	#define TEST_REG_A_FR_CNT_BYP_	(0x00000008UL)	// R/W
	#define TEST_REG_A_PME50M_BYP_	(0x00000004UL)	// R/W
	#define TEST_REG_A_PULSE_BYP_	(0x00000002UL)	// R/W
	#define TEST_REG_A_PS_BYP_		(0x00000001UL)	// R/W

#define LAN_REGISTER_EXTENT 		(0x00000100UL)
/*
 ****************************************************************************
 ****************************************************************************
 *	MAC Control and Status Register (Indirect Address)
 *	Offset (through the MAC_CSR CMD and DATA port)
 ****************************************************************************
 ****************************************************************************
 *
 */
#define MAC_CR				(0x01UL)	// R/W

	/* MAC_CR - MAC Control Register */
	#define MAC_CR_RXALL_		(0x80000000UL)
	#define MAC_CR_HBDIS_		(0x10000000UL)
	#define MAC_CR_RCVOWN_		(0x00800000UL)
	#define MAC_CR_LOOPBK_		(0x00200000UL)
	#define MAC_CR_FDPX_		(0x00100000UL)
	#define MAC_CR_MCPAS_		(0x00080000UL)
	#define MAC_CR_PRMS_		(0x00040000UL)
	#define MAC_CR_INVFILT_ 	(0x00020000UL)
	#define MAC_CR_PASSBAD_ 	(0x00010000UL)
	#define MAC_CR_HFILT_		(0x00008000UL)
	#define MAC_CR_HPFILT_		(0x00002000UL)
	#define MAC_CR_LCOLL_		(0x00001000UL)
	#define MAC_CR_BCAST_		(0x00000800UL)
	#define MAC_CR_DISRTY_		(0x00000400UL)
	#define MAC_CR_PADSTR_		(0x00000100UL)
	#define MAC_CR_BOLMT_MASK_	(0x000000C0UL)
	#define MAC_CR_DFCHK_		(0x00000020UL)
	#define MAC_CR_TXEN_		(0x00000008UL)
	#define MAC_CR_RXEN_		(0x00000004UL)

#define ADDRH				(0x02UL)	// R/W mask 0x0000FFFFUL
#define ADDRL				(0x03UL)	// R/W mask 0xFFFFFFFFUL
#define HASHH				(0x04UL)	// R/W
#define HASHL				(0x05UL)	// R/W

#define MII_ACC 			(0x06UL)	// R/W
	#define MII_ACC_PHY_ADDR_	(0x0000F800UL)
	#define MII_ACC_MIIRINDA_	(0x000007C0UL)
	#define MII_ACC_MII_WRITE_	(0x00000002UL)
	#define MII_ACC_MII_BUSY_	(0x00000001UL)

#define MII_DATA			(0x07UL)	// R/W mask 0x0000FFFFUL

#define MAC_FLOW				(0x08UL)	// R/W
	#define FLOW_FCPT_			(0xFFFF0000UL)
	#define FLOW_FCPASS_		(0x00000004UL)
	#define FLOW_FCEN_			(0x00000002UL)
	#define FLOW_FCBSY_ 		(0x00000001UL)

#define VLAN1				(0x09UL)	// R/W mask 0x0000FFFFUL
#define VLAN2				(0x0AUL)	// R/W mask 0x0000FFFFUL

#define WUFF				(0x0BUL)	// WO
	#define FILTER3_COMMAND 	(0x0F000000UL)
	#define FILTER2_COMMAND 	(0x000F0000UL)
	#define FILTER1_COMMAND 	(0x00000F00UL)
	#define FILTER0_COMMAND 	(0x0000000FUL)
		#define FILTER3_ADDR_TYPE	  (0x04000000UL)
		#define FILTER3_ENABLE	   (0x01000000UL)
		#define FILTER2_ADDR_TYPE	  (0x00040000UL)
		#define FILTER2_ENABLE	   (0x00010000UL)
		#define FILTER1_ADDR_TYPE	  (0x00000400UL)
		#define FILTER1_ENABLE	   (0x00000100UL)
		#define FILTER0_ADDR_TYPE	  (0x00000004UL)
		#define FILTER0_ENABLE	   (0x00000001UL)
	#define FILTER3_OFFSET		(0xFF000000UL)
	#define FILTER2_OFFSET		(0x00FF0000UL)
	#define FILTER1_OFFSET		(0x0000FF00UL)
	#define FILTER0_OFFSET		(0x000000FFUL)

	#define FILTER3_CRC 		(0xFFFF0000UL)
	#define FILTER2_CRC 		(0x0000FFFFUL)
	#define FILTER1_CRC 		(0xFFFF0000UL)
	#define FILTER0_CRC 		(0x0000FFFFUL)

#define WUCSR				(0x0CUL)	// R/W
	#define WUCSR_GUE_			(0x00000200UL)
	#define WUCSR_WUFR_ 		(0x00000040UL)
	#define WUCSR_MPR_			(0x00000020UL)
	#define WUCSR_WAKE_EN_		(0x00000004UL)
	#define WUCSR_MPEN_ 		(0x00000002UL)


/*
 ****************************************************************************
 *	Chip Specific MII Defines
 ****************************************************************************
 *
 *	Phy register offsets and bit definitions
 *
 */
#define LAN911X_PHY_ID	(0x00C0001CUL)

#define PHY_BCR 	((unsigned int)0U)
#define PHY_BCR_RESET_				((unsigned short)0x8000U)
#define PHY_BCR_SPEED_SELECT_		((unsigned short)0x2000U)
#define PHY_BCR_AUTO_NEG_ENABLE_	((unsigned short)0x1000U)
#define PHY_BCR_POWER_DOWN_			((unsigned short)0x0800U)
#define PHY_BCR_RESTART_AUTO_NEG_	((unsigned short)0x0200U)
#define PHY_BCR_DUPLEX_MODE_		((unsigned short)0x0100U)

#define PHY_BSR 	((unsigned int)1U)
	#define PHY_BSR_LINK_STATUS_	((unsigned short)0x0004U)
	#define PHY_BSR_REMOTE_FAULT_	((unsigned short)0x0010U)
	#define PHY_BSR_AUTO_NEG_COMP_	((unsigned short)0x0020U)
	#define PHY_BSR_ANEG_ABILITY_	((unsigned short)0x0008U)

#define PHY_ID_1	((unsigned int)2U)
#define PHY_ID_2	((unsigned int)3U)

#define PHY_ANEG_ADV	((unsigned int)4U)
#define PHY_ANEG_ADV_PAUSE_OP_		((unsigned short)0x0C00)
#define PHY_ANEG_ADV_ASYM_PAUSE_	((unsigned short)0x0800)
#define PHY_ANEG_ADV_SYM_PAUSE_ 	((unsigned short)0x0400)
#define PHY_ANEG_ADV_10H_			((unsigned short)0x0020)
#define PHY_ANEG_ADV_10F_			((unsigned short)0x0040)
#define PHY_ANEG_ADV_100H_			((unsigned short)0x0080)
#define PHY_ANEG_ADV_100F_			((unsigned short)0x0100)
#define PHY_ANEG_ADV_SPEED_ 		((unsigned short)0x01E0)

#define PHY_ANEG_LPA	((unsigned int)5U)
#define PHY_ANEG_LPA_100FDX_	((unsigned short)0x0100)
#define PHY_ANEG_LPA_100HDX_	((unsigned short)0x0080)
#define PHY_ANEG_LPA_10FDX_ 	((unsigned short)0x0040)
#define PHY_ANEG_LPA_10HDX_ 	((unsigned short)0x0020)

#define PHY_ANEG_EXP	((unsigned int)6U)
#define PHY_ANEG_EXP_PDF_			((unsigned short)0x0010)
#define PHY_ANEG_EXP_LPNPA_ 		((unsigned short)0x0008)
#define PHY_ANEG_EXP_NPA_			((unsigned short)0x0004)
#define PHY_ANEG_EXP_PAGE_RX_		((unsigned short)0x0002)
#define PHY_ANEG_EXP_LPANEG_ABLE_	((unsigned short)0x0001)

#define PHY_MODE_CTRL_STS		((unsigned int)17) // Mode Control/Status Register
	#define MODE_CTRL_STS_FASTRIP_		((unsigned short)0x4000U)
	#define MODE_CTRL_STS_EDPWRDOWN_	((unsigned short)0x2000U)
	#define MODE_CTRL_STS_LOWSQEN_		((unsigned short)0x0800U)
	#define MODE_CTRL_STS_MDPREBP_		((unsigned short)0x0400U)
	#define MODE_CTRL_STS_FARLOOPBACK_	((unsigned short)0x0200U)
	#define MODE_CTRL_STS_FASTEST_		((unsigned short)0x0100U)
	#define MODE_CTRL_STS_REFCLKEN_ 	((unsigned short)0x0010U)
	#define MODE_CTRL_STS_PHYADBP_		((unsigned short)0x0008U)
	#define MODE_CTRL_STS_FORCE_G_LINK_ ((unsigned short)0x0004U)
	#define MODE_CTRL_STS_ENERGYON_ 	((unsigned short)0x0002U)

#define PHY_INT_SRC 		((unsigned int)29)
#define PHY_INT_SRC_ENERGY_ON_			((unsigned short)0x0080U)
#define PHY_INT_SRC_ANEG_COMP_			((unsigned short)0x0040U)
#define PHY_INT_SRC_REMOTE_FAULT_		((unsigned short)0x0020U)
#define PHY_INT_SRC_LINK_DOWN_			((unsigned short)0x0010U)

#define PHY_INT_MASK		((unsigned int)30)
#define PHY_INT_MASK_ENERGY_ON_ 	((unsigned short)0x0080U)
#define PHY_INT_MASK_ANEG_COMP_ 	((unsigned short)0x0040U)
#define PHY_INT_MASK_REMOTE_FAULT_	((unsigned short)0x0020U)
#define PHY_INT_MASK_LINK_DOWN_ 	((unsigned short)0x0010U)

#define PHY_SPECIAL 		((unsigned int)31)
#define PHY_SPECIAL_SPD_	((unsigned short)0x001CU)
#define PHY_SPECIAL_SPD_10HALF_ 	((unsigned short)0x0004U)
#define PHY_SPECIAL_SPD_10FULL_ 	((unsigned short)0x0014U)
#define PHY_SPECIAL_SPD_100HALF_	((unsigned short)0x0008U)
#define PHY_SPECIAL_SPD_100FULL_	((unsigned short)0x0018U)

#define TIMEOUT_VALUE           2000        // 2 seconds.

#define LINK_NO_LINK			(0UL)
#define LINK_10MPS_HALF 		(1UL)
#define LINK_10MPS_FULL 		(2UL)
#define LINK_100MPS_HALF		(3UL)
#define LINK_100MPS_FULL		(4UL)

//------------------------------------------------------------------------------
// Local Variables 
static const char date_code[] = "062806";
static unsigned int g_pLAN911x;
static unsigned int dwPhyAddr;
static unsigned int g_chipRevision;

#define OAL_ERROR		0


#define EdbgOutputDebugString(...)
#define KITLOutputDebugString(a)
#define OALMSGS(a,b)

/*
 * @struct EDBG_ADDR | Addressing info for the debug Ethernet subsystem
 * 
 * For speed, all values are stored in net byte order (big endian).  Use the
 * htonl/ntohl/htons/ntohs macros to convert to/from local byte order.
 */
typedef struct _EDBG_ADDR {
	unsigned int  dwIP;         // @field IP address (net byte order)
	unsigned short wMAC[3];      // @field Ethernet address (net byte order)
	unsigned short wPort;        // @field UDP port # (net byte order) - only used if appropriate
    
} EDBG_ADDR;


/*
 *  @struct EDBG_ADAPTER | Adapter info struct
 *
 *  Contains information pertaining to the debug Ethernet adapter.
 */
typedef struct _EDBG_ADAPTER
{
    EDBG_ADDR Addr;       // @field Ethernet and IP Address of device
    unsigned int     SysIntrVal; // @field Logical interrupt for Ethernet controller. If set to EDBG_SYSINTR_NOINTR,
                          //        no interrupt will be registered, and the adapter will only be accessed
                          //        in polled mode.  This may be used for systems with limited IRQ lines, but
                          //        is not recommended due to delays in the system which may occur when packets
                          //        are dropped on the network.
    unsigned int DHCPLeaseTime;  // @field Duration of DHCP IP lease in seconds
    unsigned int EdbgFlags;      // @field Information about ethernet system
} EDBG_ADAPTER;



////////////////////////
// LAN9128 is memory mapped to async memory bank 1 on BF548
// 0x24000000 - 0x240000007F
#define SMSC911x_PHYS_ADDR		0x24000000

void OALStall(unsigned int microSeconds);
