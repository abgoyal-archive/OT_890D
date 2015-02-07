

#ifndef __ASM_AU1100_MMC_H
#define __ASM_AU1100_MMC_H

#include <linux/leds.h>

struct au1xmmc_platform_data {
	int(*cd_setup)(void *mmc_host, int on);
	int(*card_inserted)(void *mmc_host);
	int(*card_readonly)(void *mmc_host);
	void(*set_power)(void *mmc_host, int state);
	struct led_classdev *led;
};

#define SD0_BASE	0xB0600000
#define SD1_BASE	0xB0680000


#define SD_TXPORT	(0x0000)
#define SD_RXPORT	(0x0004)
#define SD_CONFIG	(0x0008)
#define SD_ENABLE	(0x000C)
#define SD_CONFIG2	(0x0010)
#define SD_BLKSIZE	(0x0014)
#define SD_STATUS	(0x0018)
#define SD_DEBUG	(0x001C)
#define SD_CMD		(0x0020)
#define SD_CMDARG	(0x0024)
#define SD_RESP3	(0x0028)
#define SD_RESP2	(0x002C)
#define SD_RESP1	(0x0030)
#define SD_RESP0	(0x0034)
#define SD_TIMEOUT	(0x0038)


#define SD_TXPORT_TXD	(0x000000ff)


#define SD_RXPORT_RXD	(0x000000ff)


#define SD_CONFIG_DIV	(0x000001ff)
#define SD_CONFIG_DE	(0x00000200)
#define SD_CONFIG_NE	(0x00000400)
#define SD_CONFIG_TU	(0x00000800)
#define SD_CONFIG_TO	(0x00001000)
#define SD_CONFIG_RU	(0x00002000)
#define SD_CONFIG_RO	(0x00004000)
#define SD_CONFIG_I	(0x00008000)
#define SD_CONFIG_CR	(0x00010000)
#define SD_CONFIG_RAT	(0x00020000)
#define SD_CONFIG_DD	(0x00040000)
#define SD_CONFIG_DT	(0x00080000)
#define SD_CONFIG_SC	(0x00100000)
#define SD_CONFIG_RC	(0x00200000)
#define SD_CONFIG_WC	(0x00400000)
#define SD_CONFIG_xxx	(0x00800000)
#define SD_CONFIG_TH	(0x01000000)
#define SD_CONFIG_TE	(0x02000000)
#define SD_CONFIG_TA	(0x04000000)
#define SD_CONFIG_RH	(0x08000000)
#define SD_CONFIG_RA	(0x10000000)
#define SD_CONFIG_RF	(0x20000000)
#define SD_CONFIG_CD	(0x40000000)
#define SD_CONFIG_SI	(0x80000000)


#define SD_ENABLE_CE	(0x00000001)
#define SD_ENABLE_R	(0x00000002)


#define SD_CONFIG2_EN	(0x00000001)
#define SD_CONFIG2_FF	(0x00000002)
#define SD_CONFIG2_xx1	(0x00000004)
#define SD_CONFIG2_DF	(0x00000008)
#define SD_CONFIG2_DC	(0x00000010)
#define SD_CONFIG2_xx2	(0x000000e0)
#define SD_CONFIG2_WB	(0x00000100)
#define SD_CONFIG2_RW	(0x00000200)


#define SD_BLKSIZE_BS	(0x000007ff)
#define SD_BLKSIZE_BS_SHIFT	 (0)
#define SD_BLKSIZE_BC	(0x01ff0000)
#define SD_BLKSIZE_BC_SHIFT	(16)


#define SD_STATUS_DCRCW	(0x00000007)
#define SD_STATUS_xx1	(0x00000008)
#define SD_STATUS_CB	(0x00000010)
#define SD_STATUS_DB	(0x00000020)
#define SD_STATUS_CF	(0x00000040)
#define SD_STATUS_D3	(0x00000080)
#define SD_STATUS_xx2	(0x00000300)
#define SD_STATUS_NE	(0x00000400)
#define SD_STATUS_TU	(0x00000800)
#define SD_STATUS_TO	(0x00001000)
#define SD_STATUS_RU	(0x00002000)
#define SD_STATUS_RO	(0x00004000)
#define SD_STATUS_I	(0x00008000)
#define SD_STATUS_CR	(0x00010000)
#define SD_STATUS_RAT	(0x00020000)
#define SD_STATUS_DD	(0x00040000)
#define SD_STATUS_DT	(0x00080000)
#define SD_STATUS_SC	(0x00100000)
#define SD_STATUS_RC	(0x00200000)
#define SD_STATUS_WC	(0x00400000)
#define SD_STATUS_xx3	(0x00800000)
#define SD_STATUS_TH	(0x01000000)
#define SD_STATUS_TE	(0x02000000)
#define SD_STATUS_TA	(0x04000000)
#define SD_STATUS_RH	(0x08000000)
#define SD_STATUS_RA	(0x10000000)
#define SD_STATUS_RF	(0x20000000)
#define SD_STATUS_CD	(0x40000000)
#define SD_STATUS_SI	(0x80000000)


#define SD_CMD_GO	(0x00000001)
#define SD_CMD_RY	(0x00000002)
#define SD_CMD_xx1	(0x0000000c)
#define SD_CMD_CT_MASK	(0x000000f0)
#define SD_CMD_CT_0	(0x00000000)
#define SD_CMD_CT_1	(0x00000010)
#define SD_CMD_CT_2	(0x00000020)
#define SD_CMD_CT_3	(0x00000030)
#define SD_CMD_CT_4	(0x00000040)
#define SD_CMD_CT_5	(0x00000050)
#define SD_CMD_CT_6	(0x00000060)
#define SD_CMD_CT_7	(0x00000070)
#define SD_CMD_CI	(0x0000ff00)
#define SD_CMD_CI_SHIFT		(8)
#define SD_CMD_RT_MASK	(0x00ff0000)
#define SD_CMD_RT_0	(0x00000000)
#define SD_CMD_RT_1	(0x00010000)
#define SD_CMD_RT_2	(0x00020000)
#define SD_CMD_RT_3	(0x00030000)
#define SD_CMD_RT_4	(0x00040000)
#define SD_CMD_RT_5	(0x00050000)
#define SD_CMD_RT_6	(0x00060000)
#define SD_CMD_RT_1B	(0x00810000)


#endif /* __ASM_AU1100_MMC_H */

