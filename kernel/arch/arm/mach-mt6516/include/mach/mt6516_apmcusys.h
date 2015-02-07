

#ifndef __MT6516_APMCUSYS__
#define __MT6516_APMCUSYS__

#include    <mach/mt6516_typedefs.h>
#include    <mach/mt6516_reg_base.h>



// clock and PDN register

#define APMCUSYS_PDN_CON0              (AMCONFG_BASE+0x0300)
#define APMCUSYS_PDN_SET0              (AMCONFG_BASE+0x0320)
#define APMCUSYS_PDN_CLR0              (AMCONFG_BASE+0x0340)

#define APMCUSYS_PDN_CON1              (AMCONFG_BASE+0x0360)
#define APMCUSYS_PDN_SET1              (AMCONFG_BASE+0x0380)
#define APMCUSYS_PDN_CLR1              (AMCONFG_BASE+0x03A0)

#define APMCUSYS_DELSEL0              (AMCONFG_BASE+0x0600)
#define APMCUSYS_DELSEL1              (AMCONFG_BASE+0x0604)
#define APMCUSYS_DELSEL2              (AMCONFG_BASE+0x0608)
#define APMCUSYS_DELSEL3              (AMCONFG_BASE+0x060C)

#define APMCUSYS_MON_CON              (AMCONFG_BASE+0x0700)
#define APMCUSYS_MON_SET              (AMCONFG_BASE+0x0704)
#define APMCUSYS_MON_CLR              (AMCONFG_BASE+0x0708)

#define APMCUSYS_MON_PERF1              (AMCONFG_BASE+0x070C)
#define APMCUSYS_MON_PERF2              (AMCONFG_BASE+0x0710)
#define APMCUSYS_MON_PERF3              (AMCONFG_BASE+0x0714)
#define APMCUSYS_MON_PERF4              (AMCONFG_BASE+0x0718)
#define APMCUSYS_MON_PERF5              (AMCONFG_BASE+0x071C)
#define APMCUSYS_MON_PERF6              (AMCONFG_BASE+0x0720)
#define APMCUSYS_MON_PERF7              (AMCONFG_BASE+0x0724)
#define APMCUSYS_MON_PERF8              (AMCONFG_BASE+0x0728)
#define APMCUSYS_MON_PERF9              (AMCONFG_BASE+0x072C)
#define APMCUSYS_MON_PERF10              (AMCONFG_BASE+0x0730)
#define APMCUSYS_MON_PERF11              (AMCONFG_BASE+0x0734)
#define APMCUSYS_MON_PERF12              (AMCONFG_BASE+0x0738)
#define APMCUSYS_MON_PERF13              (AMCONFG_BASE+0x073C)
#define APMCUSYS_MON_PERF14              (AMCONFG_BASE+0x0740)
#define APMCUSYS_MON_PERF15              (AMCONFG_BASE+0x0744)
#define APMCUSYS_MON_PERF16              (AMCONFG_BASE+0x0748)
#define APMCUSYS_MON_PERF17              (AMCONFG_BASE+0x074C)
#define APMCUSYS_MON_PERF18              (AMCONFG_BASE+0x0750)
#define APMCUSYS_MON_PERF19              (AMCONFG_BASE+0x0754)
#define APMCUSYS_MON_PERF20              (AMCONFG_BASE+0x0758)
#define APMCUSYS_MON_PERF21              (AMCONFG_BASE+0x075C)
#define APMCUSYS_MON_PERF22              (AMCONFG_BASE+0x0760)

#define WDT_MODE                    (RGU_BASE+0x000)
#define WDT_LENGTH                  (RGU_BASE+0x004)
#define WDT_RESTART                 (RGU_BASE+0x008) 
#define WDT_STA                     (RGU_BASE+0x00C) 
#define SW_PERIPH_RSTN              (RGU_BASE+0x010) 
#define SW_DSP_RSTN                 (RGU_BASE+0x014)
#define WDT_RSTINTERVAL             (RGU_BASE+0x018)
#define WDT_SWRST                   (RGU_BASE+0x01C)
#define RGU_USRST0                  (RGU_BASE+0x020)
#define RGU_USRST1                  (RGU_BASE+0x024)
#define RGU_USRST2                  (RGU_BASE+0x028)
#define RGU_USRST3                  (RGU_BASE+0x02C)
#define RGU_USRST4                  (RGU_BASE+0x030)
#define RGU_USRST5                  (RGU_BASE+0x034)
#define RGU_USRST6                  (RGU_BASE+0x038)
#define RGU_USRST7                  (RGU_BASE+0x03C)


 //APMCUSYS_PDN_CON0
 //APMCUSYS_PDN_SET0
 //APMCUSYS_PDN_CLR0
typedef enum
{
    PDN_PERI_DMA	=	0,
    PDN_PERI_USB	=	1,
    PDN_PERI_SEJ	=	2,
    PDN_PERI_I2C3	=	3,
    PDN_PERI_GPT	=	4,
    PDN_PERI_KP	    =	5,
    PDN_PERI_GPIO	=	6,
    PDN_PERI_UART1	=	7,
    PDN_PERI_UART2	=	8,
    PDN_PERI_UART3	=	9,
    PDN_PERI_SIM	=	10,
    PDN_PERI_PWM	=	11,
    PDN_PERI_PWM1	=	12,
    PDN_PERI_PWM2	=	13,
    PDN_PERI_PWM3	=	14,
    PDN_PERI_MSDC	=	15,
    PDN_PERI_SWDBG	=	16,
    PDN_PERI_NFI	=	17,
    PDN_PERI_I2C2	=	18,
    PDN_PERI_IRDA	=	19,
    PDN_PERI_I2C	=	21,
    PDN_PERI_SIM2	=	22,
    PDN_PERI_MSDC2	=	23,
    PDN_PERI_ADC	=	26,
    PDN_PERI_TP	    =	27,
    PDN_PERI_XGPT	=	28,
    PDN_PERI_UART4	=	29,
    PDN_PERI_MSDC3	=	30,
    PDN_PERI_ONEWIRE	=	31
    
} APMCUSYS_PDNCONA0_MODE;

 //APMCUSYS_PDN_CON1
 //APMCUSYS_PDN_SET1
 //APMCUSYS_PDN_CLR1
typedef enum
{
    PDN_PERI_CSDBG	=	0,
    PDN_PERI_PWM0	=	1,    
    
} APMCUSYS_PDNCONA1_MODE;


 //APMCUSYS_PDN_CON0
 //APMCUSYS_PDN_SET0
 //APMCUSYS_PDN_CLR0

 //APMCUSYS_PDN_CON1
 //APMCUSYS_PDN_SET1
 //APMCUSYS_PDN_CLR1

 //APMCUSYS_DELSEL0 //need check
 //APMCUSYS_DELSEL1 //need check
 //APMCUSYS_DELSEL2 //need check
 //APMCUSYS_DELSEL3 //need check

 //APMCUSYS_MON_CON
 //APMCUSYS_MON_SET
 //APMCUSYS_MON_CLR

 //APMCUSYS_MON_PERF1
 //APMCUSYS_MON_PERF2
 //APMCUSYS_MON_PERF3
 //APMCUSYS_MON_PERF4
 //APMCUSYS_MON_PERF5
 //APMCUSYS_MON_PERF6
 //APMCUSYS_MON_PERF7
 //APMCUSYS_MON_PERF8
 //APMCUSYS_MON_PERF9
 //APMCUSYS_MON_PERF10
 //APMCUSYS_MON_PERF11
 //APMCUSYS_MON_PERF12
 //APMCUSYS_MON_PERF13
 //APMCUSYS_MON_PERF14
 //APMCUSYS_MON_PERF15
 //APMCUSYS_MON_PERF16
 //APMCUSYS_MON_PERF17
 //APMCUSYS_MON_PERF18
 //APMCUSYS_MON_PERF19
 //APMCUSYS_MON_PERF20
 //APMCUSYS_MON_PERF21
 //APMCUSYS_MON_PERF22

#endif /* __MT6516_APMCUSYS__ */

