
#ifndef __ASM_ARCH_AT32AP700X_H__
#define __ASM_ARCH_AT32AP700X_H__

#define GPIO_PERIPH_A	0
#define GPIO_PERIPH_B	1

#define GPIO_PIOA_BASE	(0)
#define GPIO_PIOB_BASE	(GPIO_PIOA_BASE + 32)
#define GPIO_PIOC_BASE	(GPIO_PIOB_BASE + 32)
#define GPIO_PIOD_BASE	(GPIO_PIOC_BASE + 32)
#define GPIO_PIOE_BASE	(GPIO_PIOD_BASE + 32)

#define GPIO_PIN_PA(N)	(GPIO_PIOA_BASE + (N))
#define GPIO_PIN_PB(N)	(GPIO_PIOB_BASE + (N))
#define GPIO_PIN_PC(N)	(GPIO_PIOC_BASE + (N))
#define GPIO_PIN_PD(N)	(GPIO_PIOD_BASE + (N))
#define GPIO_PIN_PE(N)	(GPIO_PIOE_BASE + (N))


#define DMAC_MCI_RX		0
#define DMAC_MCI_TX		1
#define DMAC_DAC_TX		2
#define DMAC_AC97_A_RX		3
#define DMAC_AC97_A_TX		4
#define DMAC_AC97_B_RX		5
#define DMAC_AC97_B_TX		6
#define DMAC_DMAREQ_0		7
#define DMAC_DMAREQ_1		8
#define DMAC_DMAREQ_2		9
#define DMAC_DMAREQ_3		10

/* HSB master IDs */
#define HMATRIX_MASTER_CPU_DCACHE		0
#define HMATRIX_MASTER_CPU_ICACHE		1
#define HMATRIX_MASTER_PDC			2
#define HMATRIX_MASTER_ISI			3
#define HMATRIX_MASTER_USBA			4
#define HMATRIX_MASTER_LCDC			5
#define HMATRIX_MASTER_MACB0			6
#define HMATRIX_MASTER_MACB1			7
#define HMATRIX_MASTER_DMACA_M0			8
#define HMATRIX_MASTER_DMACA_M1			9

/* HSB slave IDs */
#define HMATRIX_SLAVE_SRAM0			0
#define HMATRIX_SLAVE_SRAM1			1
#define HMATRIX_SLAVE_PBA			2
#define HMATRIX_SLAVE_PBB			3
#define HMATRIX_SLAVE_EBI			4
#define HMATRIX_SLAVE_USBA			5
#define HMATRIX_SLAVE_LCDC			6
#define HMATRIX_SLAVE_DMACA			7

/* Bits in HMATRIX SFR4 (EBI) */
#define HMATRIX_EBI_SDRAM_ENABLE		(1 << 1)
#define HMATRIX_EBI_NAND_ENABLE			(1 << 3)
#define HMATRIX_EBI_CF0_ENABLE			(1 << 4)
#define HMATRIX_EBI_CF1_ENABLE			(1 << 5)
#define HMATRIX_EBI_PULLUP_DISABLE		(1 << 8)

#define PM_BASE		0xfff00000
#define HMATRIX_BASE	0xfff00800
#define SDRAMC_BASE	0xfff03800

/* LCDC on port C */
#define ATMEL_LCDC_PC_CC	(1ULL << 19)
#define ATMEL_LCDC_PC_HSYNC	(1ULL << 20)
#define ATMEL_LCDC_PC_PCLK	(1ULL << 21)
#define ATMEL_LCDC_PC_VSYNC	(1ULL << 22)
#define ATMEL_LCDC_PC_DVAL	(1ULL << 23)
#define ATMEL_LCDC_PC_MODE	(1ULL << 24)
#define ATMEL_LCDC_PC_PWR	(1ULL << 25)
#define ATMEL_LCDC_PC_DATA0	(1ULL << 26)
#define ATMEL_LCDC_PC_DATA1	(1ULL << 27)
#define ATMEL_LCDC_PC_DATA2	(1ULL << 28)
#define ATMEL_LCDC_PC_DATA3	(1ULL << 29)
#define ATMEL_LCDC_PC_DATA4	(1ULL << 30)
#define ATMEL_LCDC_PC_DATA5	(1ULL << 31)

/* LCDC on port D */
#define ATMEL_LCDC_PD_DATA6	(1ULL << 0)
#define ATMEL_LCDC_PD_DATA7	(1ULL << 1)
#define ATMEL_LCDC_PD_DATA8	(1ULL << 2)
#define ATMEL_LCDC_PD_DATA9	(1ULL << 3)
#define ATMEL_LCDC_PD_DATA10	(1ULL << 4)
#define ATMEL_LCDC_PD_DATA11	(1ULL << 5)
#define ATMEL_LCDC_PD_DATA12	(1ULL << 6)
#define ATMEL_LCDC_PD_DATA13	(1ULL << 7)
#define ATMEL_LCDC_PD_DATA14	(1ULL << 8)
#define ATMEL_LCDC_PD_DATA15	(1ULL << 9)
#define ATMEL_LCDC_PD_DATA16	(1ULL << 10)
#define ATMEL_LCDC_PD_DATA17	(1ULL << 11)
#define ATMEL_LCDC_PD_DATA18	(1ULL << 12)
#define ATMEL_LCDC_PD_DATA19	(1ULL << 13)
#define ATMEL_LCDC_PD_DATA20	(1ULL << 14)
#define ATMEL_LCDC_PD_DATA21	(1ULL << 15)
#define ATMEL_LCDC_PD_DATA22	(1ULL << 16)
#define ATMEL_LCDC_PD_DATA23	(1ULL << 17)

/* LCDC on port E */
#define ATMEL_LCDC_PE_CC	(1ULL << (32 + 0))
#define ATMEL_LCDC_PE_DVAL	(1ULL << (32 + 1))
#define ATMEL_LCDC_PE_MODE	(1ULL << (32 + 2))
#define ATMEL_LCDC_PE_DATA0	(1ULL << (32 + 3))
#define ATMEL_LCDC_PE_DATA1	(1ULL << (32 + 4))
#define ATMEL_LCDC_PE_DATA2	(1ULL << (32 + 5))
#define ATMEL_LCDC_PE_DATA3	(1ULL << (32 + 6))
#define ATMEL_LCDC_PE_DATA4	(1ULL << (32 + 7))
#define ATMEL_LCDC_PE_DATA8	(1ULL << (32 + 8))
#define ATMEL_LCDC_PE_DATA9	(1ULL << (32 + 9))
#define ATMEL_LCDC_PE_DATA10	(1ULL << (32 + 10))
#define ATMEL_LCDC_PE_DATA11	(1ULL << (32 + 11))
#define ATMEL_LCDC_PE_DATA12	(1ULL << (32 + 12))
#define ATMEL_LCDC_PE_DATA16	(1ULL << (32 + 13))
#define ATMEL_LCDC_PE_DATA17	(1ULL << (32 + 14))
#define ATMEL_LCDC_PE_DATA18	(1ULL << (32 + 15))
#define ATMEL_LCDC_PE_DATA19	(1ULL << (32 + 16))
#define ATMEL_LCDC_PE_DATA20	(1ULL << (32 + 17))
#define ATMEL_LCDC_PE_DATA21	(1ULL << (32 + 18))


#define ATMEL_LCDC(PORT, PIN)	(ATMEL_LCDC_##PORT##_##PIN)


#define ATMEL_LCDC_PRI_24B_DATA	(					\
		ATMEL_LCDC(PC, DATA0)  | ATMEL_LCDC(PC, DATA1)  |	\
		ATMEL_LCDC(PC, DATA2)  | ATMEL_LCDC(PC, DATA3)  |	\
		ATMEL_LCDC(PC, DATA4)  | ATMEL_LCDC(PC, DATA5)  |	\
		ATMEL_LCDC(PD, DATA6)  | ATMEL_LCDC(PD, DATA7)  |	\
		ATMEL_LCDC(PD, DATA8)  | ATMEL_LCDC(PD, DATA9)  |	\
		ATMEL_LCDC(PD, DATA10) | ATMEL_LCDC(PD, DATA11) |	\
		ATMEL_LCDC(PD, DATA12) | ATMEL_LCDC(PD, DATA13) |	\
		ATMEL_LCDC(PD, DATA14) | ATMEL_LCDC(PD, DATA15) |	\
		ATMEL_LCDC(PD, DATA16) | ATMEL_LCDC(PD, DATA17) |	\
		ATMEL_LCDC(PD, DATA18) | ATMEL_LCDC(PD, DATA19) |	\
		ATMEL_LCDC(PD, DATA20) | ATMEL_LCDC(PD, DATA21) |	\
		ATMEL_LCDC(PD, DATA22) | ATMEL_LCDC(PD, DATA23))

#define ATMEL_LCDC_ALT_24B_DATA (					\
		ATMEL_LCDC(PE, DATA0)  | ATMEL_LCDC(PE, DATA1)  |	\
		ATMEL_LCDC(PE, DATA2)  | ATMEL_LCDC(PE, DATA3)  |	\
		ATMEL_LCDC(PE, DATA4)  | ATMEL_LCDC(PC, DATA5)  |	\
		ATMEL_LCDC(PD, DATA6)  | ATMEL_LCDC(PD, DATA7)  |	\
		ATMEL_LCDC(PE, DATA8)  | ATMEL_LCDC(PE, DATA9)  |	\
		ATMEL_LCDC(PE, DATA10) | ATMEL_LCDC(PE, DATA11) |	\
		ATMEL_LCDC(PE, DATA12) | ATMEL_LCDC(PD, DATA13) |	\
		ATMEL_LCDC(PD, DATA14) | ATMEL_LCDC(PD, DATA15) |	\
		ATMEL_LCDC(PE, DATA16) | ATMEL_LCDC(PE, DATA17) |	\
		ATMEL_LCDC(PE, DATA18) | ATMEL_LCDC(PE, DATA19) |	\
		ATMEL_LCDC(PE, DATA20) | ATMEL_LCDC(PE, DATA21) |	\
		ATMEL_LCDC(PD, DATA22) | ATMEL_LCDC(PD, DATA23))

#define ATMEL_LCDC_PRI_15B_DATA (					\
		ATMEL_LCDC(PC, DATA0)  | ATMEL_LCDC(PC, DATA1)  |	\
		ATMEL_LCDC(PC, DATA2)  | ATMEL_LCDC(PC, DATA3)  |	\
		ATMEL_LCDC(PC, DATA4)  | ATMEL_LCDC(PC, DATA5)  |	\
		ATMEL_LCDC(PD, DATA8)  | ATMEL_LCDC(PD, DATA9)  |	\
		ATMEL_LCDC(PD, DATA10) | ATMEL_LCDC(PD, DATA11) |	\
		ATMEL_LCDC(PD, DATA12) | ATMEL_LCDC(PD, DATA16) |	\
		ATMEL_LCDC(PD, DATA17) | ATMEL_LCDC(PD, DATA18) |	\
		ATMEL_LCDC(PD, DATA19) | ATMEL_LCDC(PD, DATA20))

#define ATMEL_LCDC_ALT_15B_DATA	(					\
		ATMEL_LCDC(PE, DATA0)  | ATMEL_LCDC(PE, DATA1)  |	\
		ATMEL_LCDC(PE, DATA2)  | ATMEL_LCDC(PE, DATA3)  |	\
		ATMEL_LCDC(PE, DATA4)  | ATMEL_LCDC(PC, DATA5)  |	\
		ATMEL_LCDC(PE, DATA8)  | ATMEL_LCDC(PE, DATA9)  |	\
		ATMEL_LCDC(PE, DATA10) | ATMEL_LCDC(PE, DATA11) |	\
		ATMEL_LCDC(PE, DATA12) | ATMEL_LCDC(PE, DATA16) |	\
		ATMEL_LCDC(PE, DATA17) | ATMEL_LCDC(PE, DATA18) |	\
		ATMEL_LCDC(PE, DATA19) | ATMEL_LCDC(PE, DATA20))

#define ATMEL_LCDC_PRI_CONTROL (					\
		ATMEL_LCDC(PC, CC)   | ATMEL_LCDC(PC, DVAL) |		\
		ATMEL_LCDC(PC, MODE) | ATMEL_LCDC(PC, PWR))

#define ATMEL_LCDC_ALT_CONTROL (					\
		ATMEL_LCDC(PE, CC)   | ATMEL_LCDC(PE, DVAL) |		\
		ATMEL_LCDC(PE, MODE) | ATMEL_LCDC(PC, PWR))

#define ATMEL_LCDC_CONTROL (						\
		ATMEL_LCDC(PC, HSYNC) | ATMEL_LCDC(PC, VSYNC) |		\
		ATMEL_LCDC(PC, PCLK))

#define ATMEL_LCDC_PRI_24BIT	(ATMEL_LCDC_CONTROL | ATMEL_LCDC_PRI_24B_DATA)

#define ATMEL_LCDC_ALT_24BIT	(ATMEL_LCDC_CONTROL | ATMEL_LCDC_ALT_24B_DATA)

#define ATMEL_LCDC_PRI_15BIT	(ATMEL_LCDC_CONTROL | ATMEL_LCDC_PRI_15B_DATA)

#define ATMEL_LCDC_ALT_15BIT	(ATMEL_LCDC_CONTROL | ATMEL_LCDC_ALT_15B_DATA)

/* Bitmask for all EBI data (D16..D31) pins on port E */
#define ATMEL_EBI_PE_DATA_ALL  (0x0000FFFF)

#endif /* __ASM_ARCH_AT32AP700X_H__ */
