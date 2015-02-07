

#ifndef _MAGICIAN_H_
#define _MAGICIAN_H_

#include <mach/irqs.h>


#define GPIO0_MAGICIAN_KEY_POWER		0
#define GPIO9_MAGICIAN_UNKNOWN			9
#define GPIO10_MAGICIAN_GSM_IRQ			10
#define GPIO11_MAGICIAN_GSM_OUT1		11
#define GPIO13_MAGICIAN_CPLD_IRQ		13
#define GPIO18_MAGICIAN_UNKNOWN			18
#define GPIO22_MAGICIAN_VIBRA_EN		22
#define GPIO26_MAGICIAN_GSM_POWER		26
#define GPIO27_MAGICIAN_USBC_PUEN		27
#define GPIO30_MAGICIAN_nCHARGE_EN		30
#define GPIO37_MAGICIAN_KEY_HANGUP		37
#define GPIO38_MAGICIAN_KEY_CONTACTS		38
#define GPIO40_MAGICIAN_GSM_OUT2		40
#define GPIO48_MAGICIAN_UNKNOWN			48
#define GPIO56_MAGICIAN_UNKNOWN			56
#define GPIO57_MAGICIAN_CAM_RESET		57
#define GPIO75_MAGICIAN_SAMSUNG_POWER		75
#define GPIO83_MAGICIAN_nIR_EN			83
#define GPIO86_MAGICIAN_GSM_RESET		86
#define GPIO87_MAGICIAN_GSM_SELECT		87
#define GPIO90_MAGICIAN_KEY_CALENDAR		90
#define GPIO91_MAGICIAN_KEY_CAMERA		91
#define GPIO93_MAGICIAN_KEY_UP			93
#define GPIO94_MAGICIAN_KEY_DOWN		94
#define GPIO95_MAGICIAN_KEY_LEFT		95
#define GPIO96_MAGICIAN_KEY_RIGHT		96
#define GPIO97_MAGICIAN_KEY_ENTER		97
#define GPIO98_MAGICIAN_KEY_RECORD		98
#define GPIO99_MAGICIAN_HEADPHONE_IN		99
#define GPIO100_MAGICIAN_KEY_VOL_UP		100
#define GPIO101_MAGICIAN_KEY_VOL_DOWN 		101
#define GPIO102_MAGICIAN_KEY_PHONE		102
#define GPIO103_MAGICIAN_LED_KP			103
#define GPIO104_MAGICIAN_LCD_POWER_1 		104
#define GPIO105_MAGICIAN_LCD_POWER_2		105
#define GPIO106_MAGICIAN_LCD_POWER_3		106
#define GPIO107_MAGICIAN_DS1WM_IRQ		107
#define GPIO108_MAGICIAN_GSM_READY		108
#define GPIO114_MAGICIAN_UNKNOWN		114
#define GPIO115_MAGICIAN_nPEN_IRQ		115
#define GPIO116_MAGICIAN_nCAM_EN		116
#define GPIO119_MAGICIAN_UNKNOWN		119
#define GPIO120_MAGICIAN_UNKNOWN		120


#define IRQ_MAGICIAN_SD		(IRQ_BOARD_START + 0)
#define IRQ_MAGICIAN_EP		(IRQ_BOARD_START + 1)
#define IRQ_MAGICIAN_BT		(IRQ_BOARD_START + 2)
#define IRQ_MAGICIAN_AC		(IRQ_BOARD_START + 3)


#define MAGICIAN_EGPIO_BASE			0x80 /* GPIO_BOARD_START */
#define MAGICIAN_EGPIO(reg,bit) \
	(MAGICIAN_EGPIO_BASE + 8*reg + bit)

/* output */

#define EGPIO_MAGICIAN_TOPPOLY_POWER		MAGICIAN_EGPIO(0, 2)
#define EGPIO_MAGICIAN_LED_POWER		MAGICIAN_EGPIO(0, 5)
#define EGPIO_MAGICIAN_GSM_RESET		MAGICIAN_EGPIO(0, 6)
#define EGPIO_MAGICIAN_LCD_POWER		MAGICIAN_EGPIO(0, 7)
#define EGPIO_MAGICIAN_SPK_POWER		MAGICIAN_EGPIO(1, 0)
#define EGPIO_MAGICIAN_EP_POWER			MAGICIAN_EGPIO(1, 1)
#define EGPIO_MAGICIAN_IN_SEL0			MAGICIAN_EGPIO(1, 2)
#define EGPIO_MAGICIAN_IN_SEL1			MAGICIAN_EGPIO(1, 3)
#define EGPIO_MAGICIAN_MIC_POWER		MAGICIAN_EGPIO(1, 4)
#define EGPIO_MAGICIAN_CODEC_RESET		MAGICIAN_EGPIO(1, 5)
#define EGPIO_MAGICIAN_CODEC_POWER		MAGICIAN_EGPIO(1, 6)
#define EGPIO_MAGICIAN_BL_POWER			MAGICIAN_EGPIO(1, 7)
#define EGPIO_MAGICIAN_SD_POWER			MAGICIAN_EGPIO(2, 0)
#define EGPIO_MAGICIAN_CARKIT_MIC		MAGICIAN_EGPIO(2, 1)
#define EGPIO_MAGICIAN_UNKNOWN_WAVEDEV_DLL	MAGICIAN_EGPIO(2, 2)
#define EGPIO_MAGICIAN_FLASH_VPP		MAGICIAN_EGPIO(2, 3)
#define EGPIO_MAGICIAN_BL_POWER2		MAGICIAN_EGPIO(2, 4)
#define EGPIO_MAGICIAN_CHARGE_EN		MAGICIAN_EGPIO(2, 5)
#define EGPIO_MAGICIAN_GSM_POWER		MAGICIAN_EGPIO(2, 7)

/* input */

#define EGPIO_MAGICIAN_CABLE_STATE_AC		MAGICIAN_EGPIO(4, 0)
#define EGPIO_MAGICIAN_CABLE_STATE_USB		MAGICIAN_EGPIO(4, 1)

#define EGPIO_MAGICIAN_BOARD_ID0		MAGICIAN_EGPIO(5, 0)
#define EGPIO_MAGICIAN_BOARD_ID1		MAGICIAN_EGPIO(5, 1)
#define EGPIO_MAGICIAN_BOARD_ID2		MAGICIAN_EGPIO(5, 2)
#define EGPIO_MAGICIAN_LCD_SELECT		MAGICIAN_EGPIO(5, 3)
#define EGPIO_MAGICIAN_nSD_READONLY		MAGICIAN_EGPIO(5, 4)

#define EGPIO_MAGICIAN_EP_INSERT		MAGICIAN_EGPIO(6, 1)

#endif /* _MAGICIAN_H_ */
