


#if !defined(IRQS_H)
#define IRQS_H
#include "mt6516_reg_base.h"


#define MT6516_NUM_EINT   16
#define NR_IRQS                 (64+MT6516_NUM_EINT)

#define MT6516_IRQ_SEL0  ((volatile unsigned int   *)(CIRQ_BASE+0x0000))   
#define MT6516_IRQ_SEL1  ((volatile unsigned int   *)(CIRQ_BASE+0x0004))   
#define MT6516_IRQ_SEL2  ((volatile unsigned int   *)(CIRQ_BASE+0x0008))   
#define MT6516_IRQ_SEL3  ((volatile unsigned int   *)(CIRQ_BASE+0x000C))   
#define MT6516_IRQ_SEL4  ((volatile unsigned int   *)(CIRQ_BASE+0x0010))   
#define MT6516_IRQ_SEL5  ((volatile unsigned int   *)(CIRQ_BASE+0x0014))   
#define MT6516_IRQ_SEL6  ((volatile unsigned int   *)(CIRQ_BASE+0x0018))   
#define MT6516_IRQ_SEL7  ((volatile unsigned int   *)(CIRQ_BASE+0x001c))   


#define MT6516_FIQ_SEL     ((volatile unsigned int   *)(CIRQ_BASE+0x0034)) 

#define MT6516_IRQ_MASKL  ((volatile unsigned int   *)(CIRQ_BASE+0x0038))  
#define MT6516_IRQ_MASKH  ((volatile unsigned int   *)(CIRQ_BASE+0x003c))  

#define MT6516_IRQ_MASK_CLRL  ((volatile unsigned int   *)(CIRQ_BASE+0x0040))
#define MT6516_IRQ_MASK_CLRH  ((volatile unsigned int   *)(CIRQ_BASE+0x0044))

#define MT6516_IRQ_MASK_SETL ((volatile unsigned int   *)(CIRQ_BASE+0x0048)) 
#define MT6516_IRQ_MASK_SETH ((volatile unsigned int   *)(CIRQ_BASE+0x004c)) 

#define MT6516_IRQ_STAL   ((volatile unsigned int   *)(CIRQ_BASE+0x0050))   
#define MT6516_IRQ_STAH   ((volatile unsigned int   *)(CIRQ_BASE+0x0054))   

#define MT6516_IRQ_EOIL   ((volatile unsigned int   *)(CIRQ_BASE+0x0058))   
#define MT6516_IRQ_EOIH   ((volatile unsigned int   *)(CIRQ_BASE+0x005c))   

#define MT6516_IRQ_SENSL  ((volatile unsigned int   *)(CIRQ_BASE+0x0060))   
#define MT6516_IRQ_SENSH  ((volatile unsigned int   *)(CIRQ_BASE+0x0064))   

#define MT6516_IRQ_SOFTL  ((volatile unsigned int   *)(CIRQ_BASE+0x0068))   
#define MT6516_IRQ_SOFTH  ((volatile unsigned int   *)(CIRQ_BASE+0x006c))   

#define MT6516_FIQ_CON    ((volatile unsigned int   *)(CIRQ_BASE+0x0070))   
#define MT6516_FIQ_EOI    ((volatile unsigned int   *)(CIRQ_BASE+0x0074))   

#define MT6516_IRQ_STA2   ((volatile unsigned int   *)(CIRQ_BASE+0x0078))   
#define MT6516_IRQ_EOI2   ((volatile unsigned int   *)(CIRQ_BASE+0x007c))   
#define MT6516_IRQ_SOFT2  ((volatile unsigned int   *)(CIRQ_BASE+0x0080))   

#define MT6516_EINT_STATUS   ((volatile unsigned int   *)(CIRQ_BASE+0x0100)) 
#define MT6516_EINT_MASK     ((volatile unsigned int   *)(CIRQ_BASE+0x0104)) 
#define MT6516_EINT_MASK_CLR ((volatile unsigned int   *)(CIRQ_BASE+0x0108)) 
#define MT6516_EINT_MASK_SET ((volatile unsigned int   *)(CIRQ_BASE+0x010C)) 
#define MT6516_EINT_INTACK   ((volatile unsigned int   *)(CIRQ_BASE+0x0110)) 
#define MT6516_EINT_SENS     ((volatile unsigned int   *)(CIRQ_BASE+0x0114)) 

#define MT6516_EINT0_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0120)) 
#define MT6516_EINT1_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0130)) 
#define MT6516_EINT2_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0140)) 
#define MT6516_EINT3_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0150)) 
#define MT6516_EINT4_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0160)) 
#define MT6516_EINT5_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0170)) 
#define MT6516_EINT6_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0180)) 
#define MT6516_EINT7_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0190)) 
#define MT6516_EINT8_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x01a0)) 
#define MT6516_EINT9_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x01b0)) 
#define MT6516_EINT10_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x01c0))
#define MT6516_EINT11_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x01d0))
#define MT6516_EINT12_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x01e0))
#define MT6516_EINT13_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x01f0))
#define MT6516_EINT14_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0200))
#define MT6516_EINT15_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0210))
#define MT6516_EINT16_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0220))
#define MT6516_EINT17_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0230))
#define MT6516_EINT18_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0240))
#define MT6516_EINT19_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0250))
#define MT6516_EINT20_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0260))
#define MT6516_EINT21_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0270))
#define MT6516_EINT22_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0280))
#define MT6516_EINT23_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0290))

#define EINTaddr(_no) (*(volatile unsigned int *)((CIRQ_BASE+0x0120)+(0x10*_no)))


#define  EINT_CON_DEBOUNCE    0x07ff   
#define  EINT_CON_DEBOUNCE_EN 0x8000


#define  EINT_CON_HIGHLEVEL   0x0800
#define  EINT_CON_LOWLEVEL    0x0000


#define EINT7_SOURCE_0   13
#define EINT7_SOURCE_1   19   

#define EINT_MAX_CHANNEL    24

#define MT6516_GPI_FIQ_IRQ_LINE        0
#define MT6516_SIM2_IRQ_LINE            1
#define MT6516_DMA_IRQ_LINE          2
#define MT6516_UART1_IRQ_LINE          3
#define MT6516_KP_IRQ_LINE          4
#define MT6516_UART2_IRQ_LINE          5
#define MT6516_GPT_IRQ_LINE          6
#define MT6516_EIT_IRQ_LINE            7
#define MT6516_USB_IRQ_LINE            8
#define MT6516_RTC_IRQ_LINE           9
#define MT6516_MSDC1_IRQ_LINE            10
#define MT6516_IRDA_IRQ_LINE           11
#define MT6516_LCD_IRQ_LINE          12
#define MT6516_UART3_IRQ_LINE          13
#define MT6516_GPI_IRQ_LINE     14
#define MT6516_WDT_IRQ_LINE    15
#define MT6516_TVC_IRQ_LINE    16
#define MT6516_I2C3_IRQ_LINE            17
#define MT6516_NFI_IRQ_LINE            18
#define MT6516_I2C2_IRQ_LINE            19
#define MT6516_IMGDMA_IRQ_LINE            20
#define MT6516_IMGDMA2_IRQ_LINE            21
#define MT6516_PNG_IRQ_LINE         22
#define MT6516_I2C_IRQ_LINE		       23
#define MT6516_G2D_IRQ_LINE            24
#define MT6516_IMGPROC_IRQ_LINE            25
#define MT6516_CAMERA_IRQ_LINE        26
#define MT6516_MPEG4_DEC_IRQ_LINE         27 
#define MT6516_MPEG4_ENC_IRQ_LINE       28
#define MT6516_JPEG_DEC_IRQ_LINE       29
#define MT6516_JPEG_ENC_IRQ_LINE            30
#define MT6516_CRZ_IRQ_LINE            31
#define MT6516_DRZ_IRQ_LINE            32
#define MT6516_PRZ_IRQ_LINE            33
#define MT6516_TVE_IRQ_LINE            34
#define MT6516_USBDMA_IRQ_LINE        35
#define MT6516_PWM_IRQ_LINE            36
#define MT6516_MPEG4DEBLK_IRQ_LINE      37
#define MT6516_H264DEC_IRQ_LINE           38
#define MT6516_MSDC1EVENT_IRQ_LINE            39
#define MT6516_DPI_IRQ_LINE           40
#define MT6516_APCCIF_IRQ_LINE           41
#define MT6516_M3D_IRQ_LINE         42
#define MT6516_EMI_IRQ_LINE           43
#define MT6516_MSDC2_IRQ_LINE        44
#define MT6516_MSDC2EVENT_IRQ_LINE		       45		
#define MT6516_RESERVED_IRQ_LINE        46		
#define MT6516_CEVACCIF_IRQ_LINE        	   47		
#define MT6516_NFIECC_IRQ_LINE             48
#define MT6516_WAVETABLE_IRQ_LINE             49
#define MT6516_DVF_IRQ_LINE             50
#define MT6516_RESERVED2_IRQ_LINE             51
#define MT6516_GMC1_IRQ_LINE             52
#define MT6516_GMC2_IRQ_LINE             53
#define MT6516_APSLEEP_IRQ_LINE            54
#define MT6516_ASM_IRQ_LINE            55
#define MT6516_TOUCH_IRQ_LINE             56 
#define MT6516_APXGPT_IRQ_LINE             57
#define MT6516_LOWBAT_IRQ_LINE             58
#define MT6516_TVSPI_IRQ_LINE             59
#define MT6516_UART4_IRQ_LINE             60
#define MT6516_MSDC3_IRQ_LINE             61
#define MT6516_MSDC3EVENT_IRQ_LINE             62
#define MT6516_ONEWIRE_IRQ_LINE             63
#define MT6516_NUM_IRQ_LINE            64


#define MT6516_EINT0_NUM_IRQ_LINE            64
#define MT6516_EINT1_NUM_IRQ_LINE            65
#define MT6516_EINT2_NUM_IRQ_LINE            66
#define MT6516_EINT3_NUM_IRQ_LINE            67
#define MT6516_EINT4_NUM_IRQ_LINE            68
#define MT6516_EINT5_NUM_IRQ_LINE            69
#define MT6516_EINT6_NUM_IRQ_LINE            70
#define MT6516_EINT7_NUM_IRQ_LINE            70
#define MT6516_EINT8_NUM_IRQ_LINE            72
#define MT6516_EINT9_NUM_IRQ_LINE            73
#define MT6516_EINT10_NUM_IRQ_LINE            74
#define MT6516_EINT11_NUM_IRQ_LINE            75
#define MT6516_EINT12_NUM_IRQ_LINE            76
#define MT6516_EINT13_NUM_IRQ_LINE            77
#define MT6516_EINT14_NUM_IRQ_LINE            78
#define MT6516_EINT15_NUM_IRQ_LINE            79
#define MT6516_EINT16_NUM_IRQ_LINE            80
#define MT6516_EINT17_NUM_IRQ_LINE            81
#define MT6516_EINT18_NUM_IRQ_LINE            82
#define MT6516_EINT19_NUM_IRQ_LINE            83
#define MT6516_EINT20_NUM_IRQ_LINE            84
#define MT6516_EINT21_NUM_IRQ_LINE            85
#define MT6516_EINT22_NUM_IRQ_LINE            86
#define MT6516_EINT23_NUM_IRQ_LINE            87




#define MT6516_EDGE_SENSITIVE     0
#define MT6516_LEVEL_SENSITIVE    1

#define MT6516_NEG_POL     0
#define MT6516_POS_POL    1


#endif /*IRQS_H*/
