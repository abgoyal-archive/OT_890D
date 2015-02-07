
#ifndef __ASM_PB1200_H
#define __ASM_PB1200_H

#include <linux/types.h>
#include <asm/mach-au1x00/au1xxx_psc.h>

#define DBDMA_AC97_TX_CHAN	DSCR_CMD0_PSC1_TX
#define DBDMA_AC97_RX_CHAN	DSCR_CMD0_PSC1_RX
#define DBDMA_I2S_TX_CHAN	DSCR_CMD0_PSC1_TX
#define DBDMA_I2S_RX_CHAN	DSCR_CMD0_PSC1_RX

#define SPI_PSC_BASE		PSC0_BASE_ADDR
#define SMBUS_PSC_BASE		PSC0_BASE_ADDR
#define AC97_PSC_BASE       PSC1_BASE_ADDR
#define I2S_PSC_BASE		PSC1_BASE_ADDR

#define BCSR_KSEG1_ADDR 0xAD800000

typedef volatile struct
{
	/*00*/	u16 whoami;
		u16 reserved0;
	/*04*/	u16 status;
		u16 reserved1;
	/*08*/	u16 switches;
		u16 reserved2;
	/*0C*/	u16 resets;
		u16 reserved3;

	/*10*/	u16 pcmcia;
		u16 reserved4;
	/*14*/	u16 board;
		u16 reserved5;
	/*18*/	u16 disk_leds;
		u16 reserved6;
	/*1C*/	u16 system;
		u16 reserved7;

	/*20*/	u16 intclr;
		u16 reserved8;
	/*24*/	u16 intset;
		u16 reserved9;
	/*28*/	u16 intclr_mask;
		u16 reserved10;
	/*2C*/	u16 intset_mask;
		u16 reserved11;

	/*30*/	u16 sig_status;
		u16 reserved12;
	/*34*/	u16 int_status;
		u16 reserved13;
	/*38*/	u16 reserved14;
		u16 reserved15;
	/*3C*/	u16 reserved16;
		u16 reserved17;

} BCSR;

static BCSR * const bcsr = (BCSR *)BCSR_KSEG1_ADDR;

#define BCSR_WHOAMI_DCID	0x000F
#define BCSR_WHOAMI_CPLD	0x00F0
#define BCSR_WHOAMI_BOARD	0x0F00

#define BCSR_STATUS_PCMCIA0VS	0x0003
#define BCSR_STATUS_PCMCIA1VS	0x000C
#define BCSR_STATUS_SWAPBOOT	0x0040
#define BCSR_STATUS_FLASHBUSY	0x0100
#define BCSR_STATUS_IDECBLID	0x0200
#define BCSR_STATUS_SD0WP	0x0400
#define BCSR_STATUS_SD1WP	0x0800
#define BCSR_STATUS_U0RXD	0x1000
#define BCSR_STATUS_U1RXD	0x2000

#define BCSR_SWITCHES_OCTAL	0x00FF
#define BCSR_SWITCHES_DIP_1	0x0080
#define BCSR_SWITCHES_DIP_2	0x0040
#define BCSR_SWITCHES_DIP_3	0x0020
#define BCSR_SWITCHES_DIP_4	0x0010
#define BCSR_SWITCHES_DIP_5	0x0008
#define BCSR_SWITCHES_DIP_6	0x0004
#define BCSR_SWITCHES_DIP_7	0x0002
#define BCSR_SWITCHES_DIP_8	0x0001
#define BCSR_SWITCHES_ROTARY	0x0F00

#define BCSR_RESETS_ETH		0x0001
#define BCSR_RESETS_CAMERA	0x0002
#define BCSR_RESETS_DC		0x0004
#define BCSR_RESETS_IDE		0x0008
/* not resets but in the same register */
#define BCSR_RESETS_WSCFSM	0x0800
#define BCSR_RESETS_PCS0MUX	0x1000
#define BCSR_RESETS_PCS1MUX	0x2000
#define BCSR_RESETS_SPISEL	0x4000
#define BCSR_RESETS_SD1MUX	0x8000

#define BCSR_PCMCIA_PC0VPP	0x0003
#define BCSR_PCMCIA_PC0VCC	0x000C
#define BCSR_PCMCIA_PC0DRVEN	0x0010
#define BCSR_PCMCIA_PC0RST	0x0080
#define BCSR_PCMCIA_PC1VPP	0x0300
#define BCSR_PCMCIA_PC1VCC	0x0C00
#define BCSR_PCMCIA_PC1DRVEN	0x1000
#define BCSR_PCMCIA_PC1RST	0x8000

#define BCSR_BOARD_LCDVEE	0x0001
#define BCSR_BOARD_LCDVDD	0x0002
#define BCSR_BOARD_LCDBL	0x0004
#define BCSR_BOARD_CAMSNAP	0x0010
#define BCSR_BOARD_CAMPWR	0x0020
#define BCSR_BOARD_SD0PWR	0x0040
#define BCSR_BOARD_SD1PWR	0x0080

#define BCSR_LEDS_DECIMALS	0x00FF
#define BCSR_LEDS_LED0		0x0100
#define BCSR_LEDS_LED1		0x0200
#define BCSR_LEDS_LED2		0x0400
#define BCSR_LEDS_LED3		0x0800

#define BCSR_SYSTEM_VDDI	0x001F
#define BCSR_SYSTEM_POWEROFF	0x4000
#define BCSR_SYSTEM_RESET	0x8000

/* Bit positions for the different interrupt sources */
#define BCSR_INT_IDE		0x0001
#define BCSR_INT_ETH		0x0002
#define BCSR_INT_PC0		0x0004
#define BCSR_INT_PC0STSCHG	0x0008
#define BCSR_INT_PC1		0x0010
#define BCSR_INT_PC1STSCHG	0x0020
#define BCSR_INT_DC		0x0040
#define BCSR_INT_FLASHBUSY	0x0080
#define BCSR_INT_PC0INSERT	0x0100
#define BCSR_INT_PC0EJECT	0x0200
#define BCSR_INT_PC1INSERT	0x0400
#define BCSR_INT_PC1EJECT	0x0800
#define BCSR_INT_SD0INSERT	0x1000
#define BCSR_INT_SD0EJECT	0x2000
#define BCSR_INT_SD1INSERT	0x4000
#define BCSR_INT_SD1EJECT	0x8000

#define SMC91C111_PHYS_ADDR	0x0D000300
#define SMC91C111_INT		PB1200_ETH_INT

#define IDE_PHYS_ADDR		0x0C800000
#define IDE_REG_SHIFT		5
#define IDE_PHYS_LEN		(16 << IDE_REG_SHIFT)
#define IDE_INT 		PB1200_IDE_INT
#define IDE_DDMA_REQ		DSCR_CMD0_DMA_REQ1
#define IDE_RQSIZE		128

#define NAND_PHYS_ADDR 	0x1C000000

#define NAND_T_H		(18 >> 2)
#define NAND_T_PUL		(30 >> 2)
#define NAND_T_SU		(30 >> 2)
#define NAND_T_WH		(30 >> 2)

/* Bitfield shift amounts */
#define NAND_T_H_SHIFT		0
#define NAND_T_PUL_SHIFT	4
#define NAND_T_SU_SHIFT		8
#define NAND_T_WH_SHIFT		12

#define NAND_TIMING	(((NAND_T_H   & 0xF) << NAND_T_H_SHIFT)   | \
			 ((NAND_T_PUL & 0xF) << NAND_T_PUL_SHIFT) | \
			 ((NAND_T_SU  & 0xF) << NAND_T_SU_SHIFT)  | \
			 ((NAND_T_WH  & 0xF) << NAND_T_WH_SHIFT))

enum external_pb1200_ints {
	PB1200_INT_BEGIN	= AU1000_MAX_INTR + 1,

	PB1200_IDE_INT		= PB1200_INT_BEGIN,
	PB1200_ETH_INT,
	PB1200_PC0_INT,
	PB1200_PC0_STSCHG_INT,
	PB1200_PC1_INT,
	PB1200_PC1_STSCHG_INT,
	PB1200_DC_INT,
	PB1200_FLASHBUSY_INT,
	PB1200_PC0_INSERT_INT,
	PB1200_PC0_EJECT_INT,
	PB1200_PC1_INSERT_INT,
	PB1200_PC1_EJECT_INT,
	PB1200_SD0_INSERT_INT,
	PB1200_SD0_EJECT_INT,
	PB1200_SD1_INSERT_INT,
	PB1200_SD1_EJECT_INT,

	PB1200_INT_END		= PB1200_INT_BEGIN + 15
};

#define PCMCIA_MAX_SOCK  1
#define PCMCIA_NUM_SOCKS (PCMCIA_MAX_SOCK + 1)

/* VPP/VCC */
#define SET_VCC_VPP(VCC, VPP, SLOT) \
	((((VCC) << 2) | ((VPP) << 0)) << ((SLOT) * 8))

#define BOARD_PC0_INT	PB1200_PC0_INT
#define BOARD_PC1_INT	PB1200_PC1_INT
#define BOARD_CARD_INSERTED(SOCKET) bcsr->sig_status & (1 << (8 + (2 * SOCKET)))

/* NAND chip select */
#define NAND_CS 1

#endif /* __ASM_PB1200_H */
