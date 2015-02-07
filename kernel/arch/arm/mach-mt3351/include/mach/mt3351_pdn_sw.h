


#ifndef __PDN_SW_H__
#define __PDN_SW_H__
#include "mt3351_pll.h"

// PDNCONA
// PDNSETA
// PDNCLRa
typedef enum
{
    PDN_PERI_DMA	=	0,
    PDN_PERI_USB	=	1,
    PDN_PERI_RESERVED0	=	2,
    PDN_PERI_SPI	=	3,
    PDN_PERI_GPT	=	4,
    PDN_PERI_UART0	=	5,
    PDN_PERI_UART1	=	6,
    PDN_PERI_UART2	=	7,
    PDN_PERI_UART3	=	8,
    PDN_PERI_UART4	=	9,
    PDN_PERI_PWM	=	10,
    PDN_PERI_PWM0CLK	=	11,
    PDN_PERI_PWM1CLK	=	12,
    PDN_PERI_PWM2CLK	=	13,
    PDN_PERI_MSDC0	=	14,
    PDN_PERI_MSDC1	=	15,
    PDN_PERI_MSDC2	=	16,
    PDN_PERI_NFI	=	17,
    PDN_PERI_IRDA	=	18,
    PDN_PERI_I2C	=	19,
    PDN_PERI_AUXADC	=	20,
    PDN_PERI_TOUCH	=	21,
    PDN_PERI_SYSROM	=	22,
    PDN_PERI_KEYPAD	=	23
} PDNCONA_MODE;
// PDNCONB
// PDNSETB
// PDNCLRB
typedef enum
{
    PDN_MM_GMC	=		0,
    PDN_MM_G2D		=	1,
    PDN_MM_GCMQ		=	2,
    PDN_MM_RESERVED0 =	3,
    PDN_MM_IMAGEDMA	=	4,
    PDN_MM_IMAGEPROCESSOR	=	5,
    PDN_MM_JPEG		=	6,
    PDN_MM_DCT		=	7,
    PDN_MM_ISP		=	8,
    PDN_MM_PRZ		=	9,
    PDN_MM_CRZ		=	10,
    PDN_MM_DRZ		=	11,
    PDN_MM_SPI		=	12,
    PDN_MM_ASM		=	13,
    PDN_MM_I2S		=	14,
    PDN_MM_RESIZE_LB=	15,
    PDN_MM_LCD		=	16,
    PDN_MM_DPI		=	17,
    PDN_MM_VFE		=	18,
    PDN_MM_AFE		=	19,
    PDN_MM_BLS		=	20
} PDNCONB_MODE;


kal_bool PDN_Get_Peri_Status (PDNCONA_MODE mode);
kal_bool PDN_Get_MM_Status (PDNCONB_MODE mode);
void PDN_Power_CONA_DOWN(PDNCONA_MODE mode, kal_bool enable);
void PDN_Power_CONB_DOWN(PDNCONB_MODE mode, kal_bool enable);
void PDN_Power_DSP(kal_bool enable);
void APLL_set_CLK(APLL_FREQ APLL_CLK);
void Set_CLK_DLY(UINT8 delay);
void ARM_CLK_GATE_Enable(kal_bool enable);

#endif /* __PDN_SW_H__ */

