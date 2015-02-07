

#if !defined(IRQS_H)
#define IRQS_H
#include "mt3351_reg_base.h"
#include "irqs_driver.h"

#if 0
#define IRQ_IRRX                0           // Infrared
#define IRQ_SIF                 1           // Serial interface / I2C
#define IRQ_NAND                2           // NAND Flash
#define IRQ_T0                  3           // Timer 0
#define IRQ_T1                  4           // Timer 1
#define IRQ_T2                  5           // Timer 2
#define IRQ_RTCAL               6           // RTC alarm

#define IRQ_ATA1                8           // ATA1
#define IRQ_IRTX                9           // ATA2
#define IRQ_13941               10          // 1394 1
#define IRQ_13942               11          // 1394 2
#define IRQ_GFXSCL              12          // Graphic scaler
#define IRQ_PSRP                13          // Parser p buffer
#define IRQ_SPDIF               14          // SPDIF
#define IRQ_SDIO                15          // SDIO
#define IRQ_DSP                 16          // Audio DSP
#define IRQ_UART1               17          // RS232 1
#define IRQ_UART2               18          // RS232 2
#define IRQ_PCMCIA              19          // PCMCIA
#define IRQ_VDO                 20          // Video in
#define IRQ_BLK2RS1             21          // Block to raster 1
#define IRQ_BLK2RS2             22          // Block to raster 2
#define IRQ_EXT1                23          // External interrupt 1
#define IRQ_VLD                 24          // VLD
#define IRQ_GFX                 25          // Graphic
#define IRQ_DEMUX               26          // Demuxer
#define IRQ_SMARTCARD           27          // Smart card
#define IRQ_FLASHCARD           28          // Flash card
#define IRQ_EXT2                29          // External interrupt 2
#define IRQ_DRAMC               30          // DRAM controller
#define IRQ_IDETP               31          // IDE test port


#define IRQ_TIMER0              3
#define ARCH_TIMER_IRQ          5    // system tick timer irq
#define IRQBIT_IR               (1 << IRQ_IR)        // Infrared
#define IRQBIT_SERIAL           (1 << IRQ_SERIAL)    // Serial interface
#define IRQBIT_NAND             (1 << IRQ_NAND)      // NAND Flash
#define IRQBIT_T0               (1 << IRQ_T0)        // Timer 0
#define IRQBIT_T1               (1 << IRQ_T1)        // Timer 1
#define IRQBIT_T2               (1 << IRQ_T2)        // Timer 2
#define IRQBIT_RTCAL            (1 << IRQ_RTCAL)     // RTC alarm
#define IRQBIT_RTCPR            (1 << IRQ_RTCPR)     // RTC periodic
#define IRQBIT_ATA1             (1 << IRQ_ATA1)      // ATA1
#define IRQBIT_ATA2             (1 << IRQ_ATA2)      // ATA2
#define IRQBIT_13941            (1 << IRQ_13941)        // 1394 1
#define IRQBIT_13942            (1 << IRQ_13942)        // 1394 2
#define IRQBIT_SPSR             (1 << IRQ_SPSR)     // Soft parser
#define IRQBIT_HPSR             (1 << IRQ_HPSR)     // Hard parser
#define IRQBIT_SPDIF1           (1 << IRQ_SPDIF1)   // SPDIF
#define IRQBIT_SPDIF2           (1 << IRQ_SPDIF2)   // DEC2 SPDIF
#define IRQBIT_AUDIO            (1 << IRQ_AUDIO)        // Audio DSP
#define IRQBIT_RS232            (1 << IRQ_RS232)        // RS232
#define IRQBIT_PVR              (1 << IRQ_PVR)      // PVR
#define IRQBIT_PCMCIA           (1 << IRQ_PCMCIA)   // PCMCIA
#define IRQBIT_TVE              (1 << IRQ_TVE)      // TV encoder
#define IRQBIT_DISPLAY          (1 << IRQ_DISPLAY)  // Main display
#define IRQBIT_AUX              (1 << IRQ_AUX)      // Aux display
#define IRQBIT_FEND             (1 << IRQ_FEND)     // Frame end
#define IRQBIT_VLD              (1 << IRQ_VLD)      // VLD
#define IRQBIT_GFX              (1 << IRQ_GFX)      // Graphic
#define IRQBIT_DEMUX            (1 << IRQ_DEMUX)        // Demuxer
#define IRQBIT_SMARTCARD        (1 << IRQ_SMARTCARD) // Smart card
#define IRQBIT_FLASHCARD        (1 << IRQ_FLASHCARD)    // Flash card
#define IRQBIT_EXT              (1 << IRQ_EXT)      // External interrupt
#define IRQBIT_DRAMC            (1 << IRQ_DRAMC)        // DRAM controller
#define IRQBIT_VDOIN            (1 << IRQ_VDOIN)        // Video in
#endif

#define MT3351_NUM_EINT   16
#define NR_IRQS                 (64+MT3351_NUM_EINT)

#define MT3351_IRQ_SEL0  ((volatile unsigned int   *)(CIRQ_BASE+0x0000))   /* Source selection 0 to 4 */
#define MT3351_IRQ_SEL1  ((volatile unsigned int   *)(CIRQ_BASE+0x0004))   /* Source selection 5 to 9 */
#define MT3351_IRQ_SEL2  ((volatile unsigned int   *)(CIRQ_BASE+0x0008))   /* Source selection 10 to 14 */
#define MT3351_IRQ_SEL3  ((volatile unsigned int   *)(CIRQ_BASE+0x000C))   /* Source selection 15 to 19 */
#define MT3351_IRQ_SEL4  ((volatile unsigned int   *)(CIRQ_BASE+0x0010))   /* Source selection 20 to 24 */
#define MT3351_IRQ_SEL5  ((volatile unsigned int   *)(CIRQ_BASE+0x0014))   /* Source selection 25 to 29 */
#define MT3351_IRQ_SEL6  ((volatile unsigned int   *)(CIRQ_BASE+0x0018))   /* Source selection 30 to 34 */
#define MT3351_IRQ_SEL7  ((volatile unsigned int   *)(CIRQ_BASE+0x001c))   /* Source selection 35 to 39 */
#define MT3351_IRQ_SEL8  ((volatile unsigned int   *)(CIRQ_BASE+0x0020))   /* Source selection 40 to 44 */
#define MT3351_IRQ_SEL9  ((volatile unsigned int   *)(CIRQ_BASE+0x0024))   /* Source selection 45 to 49 */
#define MT3351_IRQ_SEL10  ((volatile unsigned int   *)(CIRQ_BASE+0x0028))   /* Source selection 50 to 54 */
#define MT3351_IRQ_SEL11  ((volatile unsigned int   *)(CIRQ_BASE+0x002C))   /* Source selection 55 to 59 */
#define MT3351_IRQ_SEL12  ((volatile unsigned int   *)(CIRQ_BASE+0x0030))   /* Source selection 60 to 64 */


#define MT3351_FIQ_SEL     ((volatile unsigned int   *)(CIRQ_BASE+0x0034))   /* FIQ Source selection */

#define MT3351_IRQ_MASKL  ((volatile unsigned int   *)(CIRQ_BASE+0x0038))   /* IRQ Mask 0 to 31 */
#define MT3351_IRQ_MASKH  ((volatile unsigned int   *)(CIRQ_BASE+0x003c))   /* IRQ Mask 32 to 38*/

#define MT3351_IRQ_MASK_CLRL  ((volatile unsigned int   *)(CIRQ_BASE+0x0040))   /* IRQ Mask Clear Register (LSB) */
#define MT3351_IRQ_MASK_CLRH  ((volatile unsigned int   *)(CIRQ_BASE+0x0044))   /* IRQ Mask Clear Register (MSB) */

#define MT3351_IRQ_MASK_SETL ((volatile unsigned int   *)(CIRQ_BASE+0x0048))   /* IRQ Mask Set Register (LSB) */
#define MT3351_IRQ_MASK_SETH ((volatile unsigned int   *)(CIRQ_BASE+0x004c))   /* IRQ Mask Set Register (MSB) */

#define MT3351_IRQ_STAL   ((volatile unsigned int   *)(CIRQ_BASE+0x0050))   /* IRQ Source Status Register (LSB) */
#define MT3351_IRQ_STAH   ((volatile unsigned int   *)(CIRQ_BASE+0x0054))   /* IRQ Source Status Register (MSB) */

#define MT3351_IRQ_EOIL   ((volatile unsigned int   *)(CIRQ_BASE+0x0058))   /* End of Interrupt Register (LSB) */
#define MT3351_IRQ_EOIH   ((volatile unsigned int   *)(CIRQ_BASE+0x005c))   /* End of Interrupt Register (MSB) */

#define MT3351_IRQ_SENSL  ((volatile unsigned int   *)(CIRQ_BASE+0x0060))   /* IRQ Sensitivity Register (LSB) */
#define MT3351_IRQ_SENSH  ((volatile unsigned int   *)(CIRQ_BASE+0x0064))   /* IRQ Sensitivity Register (MSB) */

#define MT3351_IRQ_SOFTL  ((volatile unsigned int   *)(CIRQ_BASE+0x0068))   /* Software Interrupt Register (LSB) */
#define MT3351_IRQ_SOFTH  ((volatile unsigned int   *)(CIRQ_BASE+0x006c))   /* Software Interrupt Register (MSB) */

#define MT3351_FIQ_CON    ((volatile unsigned int   *)(CIRQ_BASE+0x0070))   /* FIQ Control Register */
#define MT3351_FIQ_EOI    ((volatile unsigned int   *)(CIRQ_BASE+0x0074))   /* FIQ EOI Register */

#define MT3351_IRQ_STA2   ((volatile unsigned int   *)(CIRQ_BASE+0x0078))   /* Binary Code Value of IRQ_STATUS */
#define MT3351_IRQ_EOI2   ((volatile unsigned int   *)(CIRQ_BASE+0x007c))   /* Binary Code Value of IRQ_EOI */
#define MT3351_IRQ_SOFT2  ((volatile unsigned int   *)(CIRQ_BASE+0x0080))   /* Binary Code Value of IRQ_SOFT  */

#define MT3351_EINT_STATUS   ((volatile unsigned int   *)(CIRQ_BASE+0x0100))   /*  external interrupt control */
#define MT3351_EINT_MASK     ((volatile unsigned int   *)(CIRQ_BASE+0x0104))   /*  external interrupt control */
#define MT3351_EINT_MASK_CLR ((volatile unsigned int   *)(CIRQ_BASE+0x0108))   /*  external interrupt control */
#define MT3351_EINT_MASK_SET ((volatile unsigned int   *)(CIRQ_BASE+0x010C))   /*  external interrupt control */
#define MT3351_EINT_INTACK   ((volatile unsigned int   *)(CIRQ_BASE+0x0110))   /*  external interrupt control */
#define MT3351_EINT_SENS     ((volatile unsigned int   *)(CIRQ_BASE+0x0114))   /*  external interrupt control */
#define MT3351_EINT0_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0120))   /*  external interrupt control */
#define MT3351_EINT1_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0130))   /*  external interrupt control */
#define MT3351_EINT2_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0140))   /*  external interrupt control */
#define MT3351_EINT3_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0150))   /*  external interrupt control */
#define MT3351_EINT4_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0160))   /*  external interrupt control */
#define MT3351_EINT5_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0170))   /*  external interrupt control */
#define MT3351_EINT6_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0180))   /*  external interrupt control */
#define MT3351_EINT7_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x0190))   /*  external interrupt control */
#define MT3351_EINT8_CON     ((volatile unsigned int   *)(CIRQ_BASE+0x01a0))   /*  external interrupt control */






#define  EINT_CON_HIGHLEVEL   0x0800
#define  EINT_CON_LOWLEVEL    0x0000

#define MT3551_EINT_Base          (CIRQ_BASE+0x120)
#define MT3351_EINT_CONaddr(_no)  (*(volatile kal_uint16 *)(MT3551_EINT_Base+(0x10*_no)))

#define EINT7_SOURCE_0   13
#define EINT7_SOURCE_1   19   


#ifndef GPIO_base
#define GPIO_base        	(0xF0002000)     
#endif 

#define MAX_GPIO_PIN 123
#define MAX_GPIO_REG_BITS 32

#define MT3351_GPI_FIQ_IRQ_CODE        0
#define MT3351_DMA_IRQ_CODE            1
#define MT3351_UART1_IRQ_CODE          2
#define MT3351_UART2_IRQ_CODE          3
#define MT3351_UART3_IRQ_CODE          4
#define MT3351_UART4_IRQ_CODE          5
#define MT3351_UART5_IRQ_CODE          6
#define MT3351_EIT_IRQ_CODE            7
#define MT3351_USB_IRQ_CODE            8
#define MT3351_USB1_IRQ_CODE           9
#define MT3351_RTC_IRQ_CODE            10
#define MT3351_MSDC_IRQ_CODE           11
#define MT3351_MSDC2_IRQ_CODE          12
#define MT3351_MSDC3_IRQ_CODE          13
#define MT3351_MSDC_EVENT_IRQ_CODE     14
#define MT3351_MSDC2_EVENT_IRQ_CODE    15
#define MT3351_MSDC3_EVENT_IRQ_CODE    16
#define MT3351_LCD_IRQ_CODE            17
#define MT3351_GPI_IRQ_CODE            18
#define MT3351_WDT_IRQ_CODE            19
#define MT3351_NFI_IRQ_CODE            20
#define MT3351_SPI_IRQ_CODE            21
#define MT3351_IMGDMA_IRQ_CODE         22
#define MT3351_ECC_IRQ_CODE		       23
#define MT3351_I2C_IRQ_CODE            24
#define MT3351_G2D_IRQ_CODE            25
#define MT3351_IMGPROC_IRQ_CODE        26
#define MT3351_CAMERA_IRQ_CODE         27 
#define MT3351_JPEG_DEC_IRQ_CODE       28
#define MT3351_JPEG_ENC_IRQ_CODE       29
#define MT3351_CRZ_IRQ_CODE            30
#define MT3351_DRZ_IRQ_CODE            31
#define MT3351_PRZ_IRQ_CODE            32
#define MT3351_PWM_IRQ_CODE            33
#define MT3351_DPI_IRQ_CODE            34
#define MT3351_DSP2MCU_IRQ_CODE        35
#define MT3351_EMI_IRQ_CODE            36
#define MT3351_SLEEPCTRL_IRQ_CODE      37
#define MT3351_KPAD_IRQ_CODE           38
#define MT3351_GPT_IRQ_CODE            39
#define MT3351_IFLT_IRQ_CODE           40
#define MT3351_IrDA_IRQ_CODE           41
#define MT3351_MTVSPI_IRQ_CODE         42
#define MT3351_DVFC_IRQ_CODE           43
#define MT3351_CHR_DET_IRQ_CODE        44
#define MT3351_BLS_IRQ_CODE		       45		//backlight
#define MT3351_LOW_BAT_IRQ_CODE        46		//Low voltage battery
#define MT3351_TS_IRQ_CODE        	   47		//touch screen
#define MT3351_48_IRQ_CODE             48
#define MT3351_49_IRQ_CODE             49
#define MT3351_50_IRQ_CODE             50
#define MT3351_51_IRQ_CODE             51
#define MT3351_52_IRQ_CODE             52
#define MT3351_53_IRQ_CODE             53
#define MT3351_VFE_IRQ_CODE            54
#define MT3351_ASM_IRQ_CODE            55
//Myron, not the final value
#define MT3351_56_IRQ_CODE             56 
#define MT3351_57_IRQ_CODE             57
#define MT3351_58_IRQ_CODE             58
#define MT3351_59_IRQ_CODE             59
#define MT3351_60_IRQ_CODE             60
#define MT3351_61_IRQ_CODE             61
#define MT3351_62_IRQ_CODE             62
#define MT3351_63_IRQ_CODE             63

#define MT3351_NUM_IRQ_SOURCES         64




#define MT3351_IRQ_GPI_FIQ             0
#define MT3351_IRQ_DMA_CODE            1
#define MT3351_IRQ_UART1_CODE          2
#define MT3351_IRQ_UART2_CODE          3
#define MT3351_IRQ_UART3_CODE          4
#define MT3351_IRQ_UART4_CODE          5
#define MT3351_IRQ_UART5_CODE          6
#define MT3351_IRQ_EIT_CODE            7
#define MT3351_IRQ_USB_CODE            8
#define MT3351_IRQ_USB1_CODE           9
#define MT3351_IRQ_RTC_CODE            10
#define MT3351_IRQ_MSDC_CODE           11
#define MT3351_IRQ_MSDC2_CODE          12
#define MT3351_IRQ_MSDC3_CODE          13
#define MT3351_IRQ_MSDC_EVENT_CODE     14
#define MT3351_IRQ_MSDC2_EVENT_CODE    15
#define MT3351_IRQ_MSDC3_EVENT_CODE    16
#define MT3351_IRQ_LCD_CODE            17
#define MT3351_IRQ_GPI_CODE            18
#define MT3351_IRQ_WDT_CODE            19
#define MT3351_IRQ_NFI_CODE            20
#define MT3351_IRQ_SPI_CODE            21
#define MT3351_IRQ_IMGDMA_CODE         22
#define MT3351_IRQ_ECC_CODE		       23
#define MT3351_IRQ_I2C_CODE            24
#define MT3351_IRQ_G2D_CODE            25
#define MT3351_IRQ_IMGPROC_CODE        26
#define MT3351_IRQ_CAMERA_CODE         27 
#define MT3351_IRQ_JPEG_DEC_CODE       28
#define MT3351_IRQ_JPEG_ENC_CODE       29
#define MT3351_IRQ_CRZ_CODE            30
#define MT3351_IRQ_DRZ_CODE            31
#define MT3351_IRQ_PRZ_CODE            32
#define MT3351_IRQ_PWM_CODE            33
#define MT3351_IRQ_DPI_CODE            34
#define MT3351_IRQ_DSP2MCU_CODE        35
#define MT3351_IRQ_EMI_CODE            36
#define MT3351_IRQ_SLEEPCTRL_CODE      37
#define MT3351_IRQ_KPAD_CODE           38
#define MT3351_IRQ_GPT_CODE            39
#define MT3351_IRQ_IFLT_CODE           40
#define MT3351_IRQ_IrDA_CODE           41
#define MT3351_IRQ_MTVSPI_CODE         42
#define MT3351_IRQ_DVFC_CODE           43
#define MT3351_IRQ_CHR_DET             44
#define MT3351_IRQ_BLS		           45		//backlight
#define MT3351_IRQ_LOW_BAT             46		//Low voltage battery
#define MT3351_IRQ_TS_CODE        	   47		//touch screen
#define MT3351_IRQ_48_CODE             48
#define MT3351_IRQ_49_CODE             49
#define MT3351_IRQ_50_CODE             50
#define MT3351_IRQ_51_CODE             51
#define MT3351_IRQ_52_CODE             52
#define MT3351_IRQ_53_CODE             53
#define MT3351_IRQ_VFE_CODE            54
#define MT3351_IRQ_ASM_CODE            55
//Myron, not the final value
#define MT3351_IRQ_56_CODE             56 
#define MT3351_IRQ_57_CODE             57
#define MT3351_IRQ_58_CODE             58
#define MT3351_IRQ_59_CODE             59
#define MT3351_IRQ_60_CODE             60
#define MT3351_IRQ_61_CODE             61
#define MT3351_IRQ_62_CODE             62
#define MT3351_IRQ_63_CODE             63

#define MT3351_NUM_IRQ_SOURCES         64



#if 0
#define MT3351_IRQ0_CODE           MT3351_IRQ_GPI_FIQ
#define MT3351_IRQ1_CODE           MT3351_IRQ_DMA_CODE
#define MT3351_IRQ2_CODE           MT3351_IRQ_UART1_CODE
#define MT3351_IRQ3_CODE           MT3351_IRQ_UART2_CODE
#define MT3351_IRQ4_CODE           MT3351_IRQ_UART3_CODE
#define MT3351_IRQ5_CODE           MT3351_IRQ_UART4_CODE
#define MT3351_IRQ6_CODE           MT3351_IRQ_UART5_CODE
#define MT3351_IRQ7_CODE           MT3351_IRQ_EIT_CODE
#define MT3351_IRQ8_CODE           MT3351_IRQ_USB_CODE
#define MT3351_IRQ9_CODE           MT3351_IRQ_USB1_CODE
#define MT3351_IRQA_CODE           MT3351_IRQ_RTC_CODE
#define MT3351_IRQB_CODE           MT3351_IRQ_MSDC_CODE
#define MT3351_IRQC_CODE           MT3351_IRQ_MSDC2_CODE
#define MT3351_IRQD_CODE           MT3351_IRQ_MSDC3_CODE
#define MT3351_IRQE_CODE           MT3351_IRQ_MSDC_EVENT_CODE
#define MT3351_IRQF_CODE           MT3351_IRQ_MSDC2_EVENT_CODE
#define MT3351_IRQ10_CODE          MT3351_IRQ_MSDC3_EVENT_CODE
#define MT3351_IRQ11_CODE          MT3351_IRQ_LCD_CODE
#define MT3351_IRQ12_CODE          MT3351_IRQ_GPI_CODE
#define MT3351_IRQ13_CODE          MT3351_IRQ_WDT_CODE
#define MT3351_IRQ14_CODE          MT3351_IRQ_NFI_CODE
#define MT3351_IRQ15_CODE          MT3351_IRQ_SPI_CODE
#define MT3351_IRQ16_CODE          MT3351_IRQ_IMGDMA_CODE
#define MT3351_IRQ17_CODE          MT3351_IRQ_ECC_CODE
#define MT3351_IRQ18_CODE          MT3351_IRQ_I2C_CODE
#define MT3351_IRQ19_CODE          MT3351_IRQ_G2D_CODE
#define MT3351_IRQ1A_CODE          MT3351_IRQ_IMGPROC_CODE
#define MT3351_IRQ1B_CODE          MT3351_IRQ_CAMERA_CODE

#define MT3351_IRQ1C_CODE          MT3351_IRQ_JPEG_DEC_CODE
#define MT3351_IRQ1D_CODE          MT3351_IRQ_JPEG_ENC_CODE
#define MT3351_IRQ1E_CODE          MT3351_IRQ_CRZ_CODE
#define MT3351_IRQ1F_CODE          MT3351_IRQ_DRZ_CODE
#define MT3351_IRQ20_CODE          MT3351_IRQ_PRZ_CODE
#define MT3351_IRQ21_CODE          MT3351_IRQ_PWM_CODE
#define MT3351_IRQ22_CODE          MT3351_IRQ_DPI_CODE
#define MT3351_IRQ23_CODE          MT3351_IRQ_DSP2MCU_CODE
#define MT3351_IRQ24_CODE          MT3351_IRQ_EMI_CODE
#define MT3351_IRQ25_CODE          MT3351_IRQ_SLEEPCTRL_CODE
#define MT3351_IRQ26_CODE          MT3351_IRQ_KPAD_CODE
#define MT3351_IRQ27_CODE          MT3351_IRQ_GPT_CODE
#define MT3351_IRQ28_CODE          MT3351_IRQ_IFLT_CODE
#define MT3351_IRQ29_CODE          MT3351_IRQ_IrDA_CODE
#define MT3351_IRQ2A_CODE          MT3351_IRQ_MTVSPI_CODE
#define MT3351_IRQ2B_CODE          MT3351_IRQ_DVFC_CODE

#define MT3351_IRQ2C_CODE          MT3351_IRQ_CHR_DET
#define MT3351_IRQ2D_CODE          MT3351_IRQ_BLS
#define MT3351_IRQ2E_CODE          MT3351_IRQ_LOW_BAT
#define MT3351_IRQ2F_CODE          MT3351_IRQ_TS_CODE
#define MT3351_IRQ30_CODE          MT3351_IRQ_48_CODE
#define MT3351_IRQ31_CODE          MT3351_IRQ_49_CODE
#define MT3351_IRQ32_CODE          MT3351_IRQ_50_CODE
#define MT3351_IRQ33_CODE          MT3351_IRQ_51_CODE
#define MT3351_IRQ34_CODE          MT3351_IRQ_52_CODE
#define MT3351_IRQ35_CODE          MT3351_IRQ_53_CODE
#define MT3351_IRQ36_CODE          MT3351_IRQ_VFE_CODE
#define MT3351_IRQ37_CODE          MT3351_IRQ_ASM_CODE
#define MT3351_IRQ38_CODE		   MT3351_IRQ_56_CODE
#define MT3351_IRQ39_CODE		   MT3351_IRQ_57_CODE
#define MT3351_IRQ3A_CODE		   MT3351_IRQ_58_CODE
#define MT3351_IRQ3B_CODE		   MT3351_IRQ_59_CODE
#define MT3351_IRQ3C_CODE		   MT3351_IRQ_60_CODE
#define MT3351_IRQ3D_CODE		   MT3351_IRQ_61_CODE
#define MT3351_IRQ3E_CODE		   MT3351_IRQ_62_CODE
#define MT3351_IRQ3F_CODE		   MT3351_IRQ_63_CODE

#define MT3351_IRQ6_SEL1          (MT3351_IRQ6_CODE <<  6)
#define MT3351_IRQ7_SEL1          (MT3351_IRQ7_CODE <<  12)
#define MT3351_IRQ8_SEL1          (MT3351_IRQ8_CODE <<  18)
#define MT3351_IRQ9_SEL1          (MT3351_IRQ9_CODE <<  24)
#define MT3351_IRQA_SEL2          (MT3351_IRQA_CODE <<  0)
#define MT3351_IRQB_SEL2          (MT3351_IRQB_CODE <<  6)
#define MT3351_IRQC_SEL2          (MT3351_IRQC_CODE <<  12)
#define MT3351_IRQD_SEL2          (MT3351_IRQD_CODE <<  18)
#define MT3351_IRQE_SEL2          (MT3351_IRQE_CODE <<  24)
#define MT3351_IRQF_SEL3          (MT3351_IRQF_CODE <<  0)

#define MT3351_IRQ10_SEL3         (MT3351_IRQ10_CODE <<  6)
#define MT3351_IRQ11_SEL3         (MT3351_IRQ11_CODE <<  12)
#define MT3351_IRQ12_SEL3         (MT3351_IRQ12_CODE <<  18)
#define MT3351_IRQ13_SEL3         (MT3351_IRQ13_CODE <<  24)
#define MT3351_IRQ14_SEL4         (MT3351_IRQ14_CODE <<  0)
#define MT3351_IRQ15_SEL4         (MT3351_IRQ15_CODE <<  6)
#define MT3351_IRQ16_SEL4         (MT3351_IRQ16_CODE <<  12)
#define MT3351_IRQ17_SEL4         (MT3351_IRQ17_CODE <<  18)
#define MT3351_IRQ18_SEL4         (MT3351_IRQ18_CODE <<  24)
#define MT3351_IRQ19_SEL5         (MT3351_IRQ19_CODE <<  0)
#define MT3351_IRQ1A_SEL5         (MT3351_IRQ1A_CODE <<  6)
#define MT3351_IRQ1B_SEL5         (MT3351_IRQ1B_CODE <<  12)
#define MT3351_IRQ1C_SEL5         (MT3351_IRQ1C_CODE <<  18)
#define MT3351_IRQ1D_SEL5         (MT3351_IRQ1D_CODE <<  24)
#define MT3351_IRQ1E_SEL6         (MT3351_IRQ1E_CODE <<  0)
#define MT3351_IRQ1F_SEL6         (MT3351_IRQ1F_CODE <<  6)

#define MT3351_IRQ20_SEL6         (MT3351_IRQ20_CODE <<  12)
#define MT3351_IRQ21_SEL6         (MT3351_IRQ21_CODE <<  18)
#define MT3351_IRQ22_SEL6         (MT3351_IRQ22_CODE <<  24)
#define MT3351_IRQ23_SEL7         (MT3351_IRQ23_CODE <<  0)
#define MT3351_IRQ24_SEL7         (MT3351_IRQ24_CODE <<  6)
#define MT3351_IRQ25_SEL7         (MT3351_IRQ25_CODE <<  12)
#define MT3351_IRQ26_SEL7         (MT3351_IRQ26_CODE <<  18)
#define MT3351_IRQ27_SEL7         (MT3351_IRQ27_CODE <<  24)
#define MT3351_IRQ28_SEL8         (MT3351_IRQ28_CODE <<  0)
#define MT3351_IRQ29_SEL8         (MT3351_IRQ29_CODE <<  6)
#define MT3351_IRQ2A_SEL8         (MT3351_IRQ2A_CODE <<  12)
#define MT3351_IRQ2B_SEL8         (MT3351_IRQ2B_CODE <<  18)
#define MT3351_IRQ2C_SEL8         (MT3351_IRQ2C_CODE <<  24)
#define MT3351_IRQ2D_SEL9         (MT3351_IRQ2D_CODE <<  0)
#define MT3351_IRQ2E_SEL9         (MT3351_IRQ2E_CODE <<  6)
#define MT3351_IRQ2F_SEL9         (MT3351_IRQ2F_CODE <<  12)

#define MT3351_IRQ30_SEL9         (MT3351_IRQ30_CODE <<  18)
#define MT3351_IRQ31_SEL9         (MT3351_IRQ31_CODE <<  24)
#define MT3351_IRQ32_SEL10         (MT3351_IRQ32_CODE <<  0)
#define MT3351_IRQ33_SEL10         (MT3351_IRQ33_CODE <<  6)
#define MT3351_IRQ34_SEL10         (MT3351_IRQ34_CODE <<  12)
#define MT3351_IRQ35_SEL10         (MT3351_IRQ35_CODE <<  18)
#define MT3351_IRQ36_SEL10         (MT3351_IRQ36_CODE <<  24)
#define MT3351_IRQ37_SEL11         (MT3351_IRQ37_CODE <<  0)
#define MT3351_IRQ38_SEL11         (MT3351_IRQ38_CODE <<  6)
#define MT3351_IRQ39_SEL11         (MT3351_IRQ39_CODE <<  12)
#define MT3351_IRQ3A_SEL11         (MT3351_IRQ3A_CODE <<  18)
#define MT3351_IRQ3B_SEL11         (MT3351_IRQ3B_CODE <<  24)
#endif



#define MT3351_EDGE_SENSITIVE     1
#define MT3351_LEVEL_SENSITIVE    0


#endif /*IRQS_H*/
