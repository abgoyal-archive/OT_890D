

#ifndef	__iwl_prph_h__
#define __iwl_prph_h__

#define PRPH_BASE	(0x00000)
#define PRPH_END	(0xFFFFF)

/* APMG (power management) constants */
#define APMG_BASE			(PRPH_BASE + 0x3000)
#define APMG_CLK_CTRL_REG		(APMG_BASE + 0x0000)
#define APMG_CLK_EN_REG			(APMG_BASE + 0x0004)
#define APMG_CLK_DIS_REG		(APMG_BASE + 0x0008)
#define APMG_PS_CTRL_REG		(APMG_BASE + 0x000c)
#define APMG_PCIDEV_STT_REG		(APMG_BASE + 0x0010)
#define APMG_RFKILL_REG			(APMG_BASE + 0x0014)
#define APMG_RTC_INT_STT_REG		(APMG_BASE + 0x001c)
#define APMG_RTC_INT_MSK_REG		(APMG_BASE + 0x0020)

#define APMG_CLK_VAL_DMA_CLK_RQT	(0x00000200)
#define APMG_CLK_VAL_BSM_CLK_RQT	(0x00000800)


#define APMG_PS_CTRL_EARLY_PWR_OFF_RESET_DIS	(0x00400000)
#define APMG_PS_CTRL_VAL_RESET_REQ		(0x04000000)
#define APMG_PS_CTRL_MSK_PWR_SRC		(0x03000000)
#define APMG_PS_CTRL_VAL_PWR_SRC_VMAIN		(0x00000000)
#define APMG_PS_CTRL_VAL_PWR_SRC_MAX		(0x01000000) /* 3945 only */
#define APMG_PS_CTRL_VAL_PWR_SRC_VAUX		(0x02000000)


#define APMG_PCIDEV_STT_VAL_L1_ACT_DIS		(0x00000800)


/* BSM bit fields */
#define BSM_WR_CTRL_REG_BIT_START     (0x80000000) /* start boot load now */
#define BSM_WR_CTRL_REG_BIT_START_EN  (0x40000000) /* enable boot after pwrup*/
#define BSM_DRAM_INST_LOAD            (0x80000000) /* start program load now */

/* BSM addresses */
#define BSM_BASE                     (PRPH_BASE + 0x3400)
#define BSM_END                      (PRPH_BASE + 0x3800)

#define BSM_WR_CTRL_REG              (BSM_BASE + 0x000) /* ctl and status */
#define BSM_WR_MEM_SRC_REG           (BSM_BASE + 0x004) /* source in BSM mem */
#define BSM_WR_MEM_DST_REG           (BSM_BASE + 0x008) /* dest in SRAM mem */
#define BSM_WR_DWCOUNT_REG           (BSM_BASE + 0x00C) /* bytes */
#define BSM_WR_STATUS_REG            (BSM_BASE + 0x010) /* bit 0:  1 == done */

#define BSM_DRAM_INST_PTR_REG        (BSM_BASE + 0x090)
#define BSM_DRAM_INST_BYTECOUNT_REG  (BSM_BASE + 0x094)
#define BSM_DRAM_DATA_PTR_REG        (BSM_BASE + 0x098)
#define BSM_DRAM_DATA_BYTECOUNT_REG  (BSM_BASE + 0x09C)

#define BSM_SRAM_LOWER_BOUND         (PRPH_BASE + 0x3800)
#define BSM_SRAM_SIZE			(1024) /* bytes */


/* 3945 Tx scheduler registers */
#define ALM_SCD_BASE                        (PRPH_BASE + 0x2E00)
#define ALM_SCD_MODE_REG                    (ALM_SCD_BASE + 0x000)
#define ALM_SCD_ARASTAT_REG                 (ALM_SCD_BASE + 0x004)
#define ALM_SCD_TXFACT_REG                  (ALM_SCD_BASE + 0x010)
#define ALM_SCD_TXF4MF_REG                  (ALM_SCD_BASE + 0x014)
#define ALM_SCD_TXF5MF_REG                  (ALM_SCD_BASE + 0x020)
#define ALM_SCD_SBYP_MODE_1_REG             (ALM_SCD_BASE + 0x02C)
#define ALM_SCD_SBYP_MODE_2_REG             (ALM_SCD_BASE + 0x030)


#define SCD_WIN_SIZE				64
#define SCD_FRAME_LIMIT				64

/* SCD registers are internal, must be accessed via HBUS_TARG_PRPH regs */
#define IWL49_SCD_START_OFFSET		0xa02c00

#define IWL49_SCD_SRAM_BASE_ADDR           (IWL49_SCD_START_OFFSET + 0x0)

#define IWL49_SCD_EMPTY_BITS               (IWL49_SCD_START_OFFSET + 0x4)

#define IWL49_SCD_DRAM_BASE_ADDR           (IWL49_SCD_START_OFFSET + 0x10)

#define IWL49_SCD_TXFACT                   (IWL49_SCD_START_OFFSET + 0x1c)
#define IWL49_SCD_QUEUE_WRPTR(x)  (IWL49_SCD_START_OFFSET + 0x24 + (x) * 4)

#define IWL49_SCD_QUEUE_RDPTR(x)  (IWL49_SCD_START_OFFSET + 0x64 + (x) * 4)

#define IWL49_SCD_QUEUECHAIN_SEL  (IWL49_SCD_START_OFFSET + 0xd0)

#define IWL49_SCD_INTERRUPT_MASK  (IWL49_SCD_START_OFFSET + 0xe4)

#define IWL49_SCD_QUEUE_STATUS_BITS(x)\
	(IWL49_SCD_START_OFFSET + 0x104 + (x) * 4)

/* Bit field positions */
#define IWL49_SCD_QUEUE_STTS_REG_POS_ACTIVE	(0)
#define IWL49_SCD_QUEUE_STTS_REG_POS_TXF	(1)
#define IWL49_SCD_QUEUE_STTS_REG_POS_WSL	(5)
#define IWL49_SCD_QUEUE_STTS_REG_POS_SCD_ACK	(8)

/* Write masks */
#define IWL49_SCD_QUEUE_STTS_REG_POS_SCD_ACT_EN	(10)
#define IWL49_SCD_QUEUE_STTS_REG_MSK		(0x0007FC00)


#define IWL49_SCD_CONTEXT_DATA_OFFSET			0x380
#define IWL49_SCD_CONTEXT_QUEUE_OFFSET(x) \
			(IWL49_SCD_CONTEXT_DATA_OFFSET + ((x) * 8))

#define IWL49_SCD_QUEUE_CTX_REG1_WIN_SIZE_POS		(0)
#define IWL49_SCD_QUEUE_CTX_REG1_WIN_SIZE_MSK		(0x0000007F)
#define IWL49_SCD_QUEUE_CTX_REG2_FRAME_LIMIT_POS	(16)
#define IWL49_SCD_QUEUE_CTX_REG2_FRAME_LIMIT_MSK	(0x007F0000)

#define IWL49_SCD_TX_STTS_BITMAP_OFFSET		0x400

#define IWL49_SCD_TRANSLATE_TBL_OFFSET		0x500

/* Find translation table dword to read/write for given queue */
#define IWL49_SCD_TRANSLATE_TBL_OFFSET_QUEUE(x) \
	((IWL49_SCD_TRANSLATE_TBL_OFFSET + ((x) * 2)) & 0xfffffffc)

#define IWL_SCD_TXFIFO_POS_TID			(0)
#define IWL_SCD_TXFIFO_POS_RA			(4)
#define IWL_SCD_QUEUE_RA_TID_MAP_RATID_MSK	(0x01FF)

/* 5000 SCD */
#define IWL50_SCD_QUEUE_STTS_REG_POS_TXF	(0)
#define IWL50_SCD_QUEUE_STTS_REG_POS_ACTIVE	(3)
#define IWL50_SCD_QUEUE_STTS_REG_POS_WSL	(4)
#define IWL50_SCD_QUEUE_STTS_REG_POS_SCD_ACT_EN (19)
#define IWL50_SCD_QUEUE_STTS_REG_MSK		(0x00FF0000)

#define IWL50_SCD_QUEUE_CTX_REG1_CREDIT_POS		(8)
#define IWL50_SCD_QUEUE_CTX_REG1_CREDIT_MSK		(0x00FFFF00)
#define IWL50_SCD_QUEUE_CTX_REG1_SUPER_CREDIT_POS	(24)
#define IWL50_SCD_QUEUE_CTX_REG1_SUPER_CREDIT_MSK	(0xFF000000)
#define IWL50_SCD_QUEUE_CTX_REG2_WIN_SIZE_POS		(0)
#define IWL50_SCD_QUEUE_CTX_REG2_WIN_SIZE_MSK		(0x0000007F)
#define IWL50_SCD_QUEUE_CTX_REG2_FRAME_LIMIT_POS	(16)
#define IWL50_SCD_QUEUE_CTX_REG2_FRAME_LIMIT_MSK	(0x007F0000)

#define IWL50_SCD_CONTEXT_DATA_OFFSET		(0x600)
#define IWL50_SCD_TX_STTS_BITMAP_OFFSET		(0x7B1)
#define IWL50_SCD_TRANSLATE_TBL_OFFSET		(0x7E0)

#define IWL50_SCD_CONTEXT_QUEUE_OFFSET(x)\
	(IWL50_SCD_CONTEXT_DATA_OFFSET + ((x) * 8))

#define IWL50_SCD_TRANSLATE_TBL_OFFSET_QUEUE(x) \
	((IWL50_SCD_TRANSLATE_TBL_OFFSET + ((x) * 2)) & 0xfffc)

#define IWL50_SCD_QUEUECHAIN_SEL_ALL(x)		(((1<<(x)) - 1) &\
	(~(1<<IWL_CMD_QUEUE_NUM)))

#define IWL50_SCD_BASE			(PRPH_BASE + 0xa02c00)

#define IWL50_SCD_SRAM_BASE_ADDR         (IWL50_SCD_BASE + 0x0)
#define IWL50_SCD_DRAM_BASE_ADDR	 (IWL50_SCD_BASE + 0x8)
#define IWL50_SCD_AIT                    (IWL50_SCD_BASE + 0x0c)
#define IWL50_SCD_TXFACT                 (IWL50_SCD_BASE + 0x10)
#define IWL50_SCD_ACTIVE		 (IWL50_SCD_BASE + 0x14)
#define IWL50_SCD_QUEUE_WRPTR(x)         (IWL50_SCD_BASE + 0x18 + (x) * 4)
#define IWL50_SCD_QUEUE_RDPTR(x)         (IWL50_SCD_BASE + 0x68 + (x) * 4)
#define IWL50_SCD_QUEUECHAIN_SEL         (IWL50_SCD_BASE + 0xe8)
#define IWL50_SCD_AGGR_SEL	     	 (IWL50_SCD_BASE + 0x248)
#define IWL50_SCD_INTERRUPT_MASK         (IWL50_SCD_BASE + 0x108)
#define IWL50_SCD_QUEUE_STATUS_BITS(x)   (IWL50_SCD_BASE + 0x10c + (x) * 4)

/*********************** END TX SCHEDULER *************************************/

#endif				/* __iwl_prph_h__ */
