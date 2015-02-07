

/* system header files */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/time.h>

#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/mt3351_typedefs.h>
#include <mach/mt3351_wdt.h>
#include <mach/mt3351_pll.h>
#include <mach/mt3351_pmu_sw.h>
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_gpt_sw.h>

#include <asm/mach-types.h>
#include <asm/mach/time.h>

#define MT3351_WRITEREG32(a,b)      (*(volatile unsigned int *)(a) = (unsigned int)b)
#define MT3351_READREG32(a)         (*(volatile unsigned int *)(a))
#define MT3351_WRITEREG16(a,b)      (*(volatile unsigned short *)(a) = (unsigned short)b)
#define MT3351_READREG16(a)         (*(volatile unsigned short *)(a))
#define MT3351_SETREG32(addr, data) ((*(volatile unsigned int *)(addr)) |= (unsigned int)data) 
#define MT3351_CLRREG32(addr, data) ((*(volatile unsigned int *)(addr)) &= ~((unsigned int)data)) 

const unsigned char MT3351_irqCode[] = { 
  MT3351_GPI_FIQ_IRQ_CODE,  MT3351_DMA_IRQ_CODE,    MT3351_UART1_IRQ_CODE,    MT3351_UART2_IRQ_CODE,    MT3351_UART3_IRQ_CODE,          
  MT3351_UART4_IRQ_CODE,    MT3351_UART5_IRQ_CODE,  MT3351_EIT_IRQ_CODE,      MT3351_USB_IRQ_CODE,      MT3351_USB1_IRQ_CODE,           
  MT3351_RTC_IRQ_CODE,      MT3351_MSDC_IRQ_CODE,   MT3351_MSDC2_IRQ_CODE,    MT3351_MSDC3_IRQ_CODE,    MT3351_MSDC_EVENT_IRQ_CODE,
  MT3351_MSDC2_EVENT_IRQ_CODE,    MT3351_MSDC3_EVENT_IRQ_CODE,    MT3351_LCD_IRQ_CODE,    MT3351_GPI_IRQ_CODE,    MT3351_WDT_IRQ_CODE,               
  MT3351_NFI_IRQ_CODE,      MT3351_SPI_IRQ_CODE,    MT3351_IMGDMA_IRQ_CODE,   MT3351_ECC_IRQ_CODE,      MT3351_I2C_IRQ_CODE,
  MT3351_G2D_IRQ_CODE,      MT3351_IMGPROC_IRQ_CODE,MT3351_CAMERA_IRQ_CODE,   MT3351_JPEG_DEC_IRQ_CODE, MT3351_JPEG_ENC_IRQ_CODE,
  MT3351_CRZ_IRQ_CODE,      MT3351_DRZ_IRQ_CODE,    MT3351_PRZ_IRQ_CODE,      MT3351_PWM_IRQ_CODE,      MT3351_DPI_IRQ_CODE,         
  MT3351_DSP2MCU_IRQ_CODE,  MT3351_EMI_IRQ_CODE,    MT3351_SLEEPCTRL_IRQ_CODE,MT3351_KPAD_IRQ_CODE,     MT3351_GPT_IRQ_CODE,     
  MT3351_IFLT_IRQ_CODE,     MT3351_IrDA_IRQ_CODE,   MT3351_MTVSPI_IRQ_CODE,   MT3351_DVFC_IRQ_CODE,     MT3351_CHR_DET_IRQ_CODE,   
  MT3351_BLS_IRQ_CODE,		MT3351_LOW_BAT_IRQ_CODE,MT3351_TS_IRQ_CODE,       MT3351_48_IRQ_CODE,       MT3351_49_IRQ_CODE,
  MT3351_50_IRQ_CODE,       MT3351_51_IRQ_CODE,     MT3351_52_IRQ_CODE,       MT3351_53_IRQ_CODE,       MT3351_VFE_IRQ_CODE,
  MT3351_ASM_IRQ_CODE,      MT3351_56_IRQ_CODE,     MT3351_57_IRQ_CODE,       MT3351_58_IRQ_CODE,       MT3351_59_IRQ_CODE,
  MT3351_60_IRQ_CODE,       MT3351_61_IRQ_CODE,     MT3351_62_IRQ_CODE,       MT3351_63_IRQ_CODE};
/* Interrupt Remapping Table */
unsigned char MT3351_IRQCode2Line[MT3351_NUM_IRQ_SOURCES];
unsigned char MT3351_lineOfirqCode[] = {
  MT3351_GPI_FIQ_IRQ_LINE,  MT3351_DMA_IRQ_LINE,    MT3351_UART1_IRQ_LINE,    MT3351_UART2_IRQ_LINE,    MT3351_UART3_IRQ_LINE,          
  MT3351_UART4_IRQ_LINE,    MT3351_UART5_IRQ_LINE,  MT3351_EIT_IRQ_LINE,      MT3351_USB_IRQ_LINE,      MT3351_USB1_IRQ_LINE,           
  MT3351_RTC_IRQ_LINE,      MT3351_MSDC_IRQ_LINE,   MT3351_MSDC2_IRQ_LINE,    MT3351_MSDC3_IRQ_LINE,    MT3351_MSDC_EVENT_IRQ_LINE,
  MT3351_MSDC2_EVENT_IRQ_LINE,    MT3351_MSDC3_EVENT_IRQ_LINE,    MT3351_LCD_IRQ_LINE,    MT3351_GPI_IRQ_LINE,    MT3351_WDT_IRQ_LINE,            
  MT3351_NFI_IRQ_LINE,      MT3351_SPI_IRQ_LINE,    MT3351_IMGDMA_IRQ_LINE,   MT3351_ECC_IRQ_LINE,      MT3351_I2C_IRQ_LINE,
  MT3351_G2D_IRQ_LINE,      MT3351_IMGPROC_IRQ_LINE,MT3351_CAMERA_IRQ_LINE,   MT3351_JPEG_DEC_IRQ_LINE, MT3351_JPEG_ENC_IRQ_LINE,
  MT3351_CRZ_IRQ_LINE,      MT3351_DRZ_IRQ_LINE,    MT3351_PRZ_IRQ_LINE,      MT3351_PWM_IRQ_LINE,      MT3351_DPI_IRQ_LINE,         
  MT3351_DSP2MCU_IRQ_LINE,  MT3351_EMI_IRQ_LINE,    MT3351_SLEEPCTRL_IRQ_LINE,MT3351_KPAD_IRQ_LINE,     MT3351_GPT_IRQ_LINE,     
  MT3351_IFLT_IRQ_LINE,     MT3351_IrDA_IRQ_LINE,   MT3351_MTVSPI_IRQ_LINE,   MT3351_DVFC_IRQ_LINE,     MT3351_CHR_DET_IRQ_LINE,   
  MT3351_BLS_IRQ_LINE,		MT3351_LOW_BAT_IRQ_LINE,MT3351_TS_IRQ_LINE,      MT3351_48_IRQ_LINE,       MT3351_49_IRQ_LINE,
  MT3351_50_IRQ_LINE,       MT3351_51_IRQ_LINE,     MT3351_52_IRQ_LINE,       MT3351_53_IRQ_LINE,       MT3351_VFE_IRQ_LINE,
  MT3351_ASM_IRQ_LINE,      MT3351_56_IRQ_LINE,     MT3351_57_IRQ_LINE,       MT3351_58_IRQ_LINE,       MT3351_59_IRQ_LINE,
  MT3351_60_IRQ_LINE,       MT3351_61_IRQ_LINE,     MT3351_62_IRQ_LINE,       MT3351_63_IRQ_LINE};

unsigned char MT3351_IRQLineToCode[MT3351_NUM_IRQ_SOURCES];

void MT3351_IRQMask(unsigned int line)
{
    if (line <= 31)
        *MT3351_IRQ_MASK_SETL = (1 << line);
    else
        *MT3351_IRQ_MASK_SETH = (1 << (line - 32));
}

void MT3351_IRQUnmask(unsigned int line)
{
   /*
    * NoteXXX: This should be for MT6238.
    *          For FPGA verification, we use MT6219 temporarily.
    */

   if (line <= 31)
      *MT3351_IRQ_MASK_CLRL = (1 << line);
   else
      *MT3351_IRQ_MASK_CLRH = (1 << (line - 32));
}

void MT3351_IRQClearInt(unsigned int line)
{
    MT3351_IRQMask(line);
    *MT3351_IRQ_EOI2 = line;
}

void MT3351_EINTIRQMask(unsigned int line)
{
   char eIntLine = line - MT3351_NUM_IRQ_SOURCES; 
   // Mask MT3351_EINT_MASK
   *MT3351_EINT_MASK_SET = 1 << eIntLine;
}

void MT3351_EINTIRQUnmask(unsigned int line)
{
   /*
    * NoteXXX: This should be for MT6238.
    *          For FPGA verification, we use MT6219 temporarily.
    */
   char eIntLine = line - MT3351_NUM_IRQ_SOURCES;

   *MT3351_EINT_MASK_CLR = 1 << eIntLine;
}


void MT3351_EINTIRQClearInt(unsigned int line)
{
    char eIntLine = line - MT3351_NUM_IRQ_SOURCES;
    // change 09/18
    MT3351_EINTIRQMask(line);
    *MT3351_EINT_INTACK = 1 << eIntLine;
    *MT3351_IRQ_EOI2 = MT3351_EIT_IRQ_LINE; // MT3351_EIT_CODE   
}

void MT3351_FIQMask(void)
{
   *MT3351_FIQ_CON |= 0x01;
}

void MT3351_FIQUnmask(void)
{
   *MT3351_FIQ_CON &= ~(0x01);
}


static struct irq_chip mt3351_irqchip =
{
    .ack    = MT3351_IRQClearInt,
    .mask   = MT3351_IRQMask,
    .unmask = MT3351_IRQUnmask,
};

/* For external interrupt */
static struct irq_chip mt3351_EINTirqchip =
{
    .ack    = MT3351_EINTIRQClearInt,
    .mask   = MT3351_EINTIRQMask,
    .unmask = MT3351_EINTIRQUnmask,
};



void MT3351_IRQSensitivity(unsigned char code, unsigned char edge)
{
   unsigned char line;

   line = MT3351_IRQCode2Line[code];
   
   if (edge != 0) { /* edge sensitive interrupt */
      if (line <= 31)
         *MT3351_IRQ_SENSL &= (~(1 << line));
      else
         *MT3351_IRQ_SENSH &= (~(1 << (line - 32)));
   } else {        /* level sensitive interrupt */
      if (line <= 31)
         *MT3351_IRQ_SENSL |= (1 << line);
      else
         *MT3351_IRQ_SENSH |= (1 << (line - 32));
   }
}



void MT3351_IRQSel(void)
{
    int tmp;
    for(tmp = 0; tmp < MT3351_NUM_IRQ_SOURCES; tmp++){
        MT3351_IRQLineToCode[MT3351_lineOfirqCode[tmp]] = MT3351_irqCode[tmp];
    }
    
  *MT3351_IRQ_SEL0 =  (MT3351_IRQLineToCode[0]) | (MT3351_IRQLineToCode[1]<<6)  | (MT3351_IRQLineToCode[2]<<12)  |  (MT3351_IRQLineToCode[3]<<18)  | (MT3351_IRQLineToCode[4]<<24);
  *MT3351_IRQ_SEL1 =  (MT3351_IRQLineToCode[5]) | (MT3351_IRQLineToCode[6]<<6)  | (MT3351_IRQLineToCode[7]<<12)  |  (MT3351_IRQLineToCode[8]<<18)  | (MT3351_IRQLineToCode[9]<<24);
  *MT3351_IRQ_SEL2 =  (MT3351_IRQLineToCode[10]) | (MT3351_IRQLineToCode[11]<<6) | (MT3351_IRQLineToCode[12]<<12) |  (MT3351_IRQLineToCode[13]<<18) | (MT3351_IRQLineToCode[14]<<24);
  *MT3351_IRQ_SEL3 =  (MT3351_IRQLineToCode[15]) | (MT3351_IRQLineToCode[16]<<6) | (MT3351_IRQLineToCode[17]<<12) |  (MT3351_IRQLineToCode[18]<<18) | (MT3351_IRQLineToCode[19]<<24);
  *MT3351_IRQ_SEL4 =  (MT3351_IRQLineToCode[20]) | (MT3351_IRQLineToCode[21]<<6) | (MT3351_IRQLineToCode[22]<<12) |  (MT3351_IRQLineToCode[23]<<18) | (MT3351_IRQLineToCode[24]<<24);
  *MT3351_IRQ_SEL5 =  (MT3351_IRQLineToCode[25]) | (MT3351_IRQLineToCode[26]<<6) | (MT3351_IRQLineToCode[27]<<12) |  (MT3351_IRQLineToCode[28]<<18) | (MT3351_IRQLineToCode[29]<<24);
  *MT3351_IRQ_SEL6 =  (MT3351_IRQLineToCode[30]) | (MT3351_IRQLineToCode[31]<<6) | (MT3351_IRQLineToCode[32]<<12) |  (MT3351_IRQLineToCode[33]<<18) | (MT3351_IRQLineToCode[34]<<24);
  *MT3351_IRQ_SEL7 =  (MT3351_IRQLineToCode[35]) | (MT3351_IRQLineToCode[36]<<6) | (MT3351_IRQLineToCode[37]<<12) |  (MT3351_IRQLineToCode[38]<<18) | (MT3351_IRQLineToCode[39]<<24);
  *MT3351_IRQ_SEL8 =  (MT3351_IRQLineToCode[40]) | (MT3351_IRQLineToCode[41]<<6) | (MT3351_IRQLineToCode[42]<<12) |  (MT3351_IRQLineToCode[43]<<18) | (MT3351_IRQLineToCode[44]<<24);
  *MT3351_IRQ_SEL9 =  (MT3351_IRQLineToCode[45]) | (MT3351_IRQLineToCode[46]<<6) | (MT3351_IRQLineToCode[47]<<12) |  (MT3351_IRQLineToCode[48]<<18) | (MT3351_IRQLineToCode[49]<<24);
  *MT3351_IRQ_SEL10 = (MT3351_IRQLineToCode[50]) | (MT3351_IRQLineToCode[51]<<6) | (MT3351_IRQLineToCode[52]<<12) |  (MT3351_IRQLineToCode[53]<<18) | (MT3351_IRQLineToCode[54]<<24);
  *MT3351_IRQ_SEL11 = (MT3351_IRQLineToCode[55]) | (MT3351_IRQLineToCode[56]<<6) | (MT3351_IRQLineToCode[57]<<12) |  (MT3351_IRQLineToCode[58]<<18) | (MT3351_IRQLineToCode[59]<<24);
  *MT3351_IRQ_SEL12 = (MT3351_IRQLineToCode[60]) | (MT3351_IRQLineToCode[61]<<6) | (MT3351_IRQLineToCode[62]<<12) |  (MT3351_IRQLineToCode[63]<<18) ;
}



void __init mt3351_init_irq(void)
{
    unsigned int irq,eintnum;

    mt3351_wdt_SW_MCUPeripheralReset(MT3351_MCU_PERI_IRQ);

    for (irq = 0; irq < MT3351_NUM_IRQ_SOURCES; irq++)
    {
        MT3351_IRQCode2Line[MT3351_irqCode[irq]] = MT3351_lineOfirqCode[irq];
        set_irq_chip(irq, &mt3351_irqchip);
        set_irq_handler(irq, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }
    eintnum = MT3351_NUM_IRQ_SOURCES + MT3351_NUM_EINT;
    for (irq = 64; irq < eintnum; irq++)
    {
        set_irq_chip(irq, &mt3351_EINTirqchip);
        set_irq_handler(irq, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }

    /* pangyen added to enable USB cable in detection */
    MT3351_WRITEREG32(0xf0022130, 0x8804);
    MT3351_WRITEREG32(0xf0022190, 0x8004);
    /* pangyen added to enable USB cable in detection */
    
    MT3351_IRQSel();

    MT3351_pmu_init();
    MT3351_PLL_Init();
}


static unsigned long _timer_tick_count;

#if 0
static irqreturn_t
mt3351_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    write_seqlock(&xtime_lock);
    /* tick system */
    timer_tick(regs);
        /* FIXME:
         *   use a do while loop to tick exact counts
         */
    _timer_tick_count++;
    
    write_sequnlock(&xtime_lock);
    return IRQ_HANDLED;
}
#endif /* #if 0*/

static irqreturn_t
mt3351_timer_interrupt(int irq, void *dev_id)
{
    
    
    // g_u4GPTMask[GPT0] = 0x0001
    // GPT_BASE+0x04GPT_IRQSTA = (GPT_BASE+0x04) = 0xF002A000 + 0x04
    if((0x0001 & DRV_Reg32(0xF002A004)) == true)
    {
    /*
     *  FIX ME :
     *
     *  Should check how to handle for the other GPTs which is not used for 
     *  system timer !
     */
    //write_seqlock(&xtime_lock);
    /* tick system */

    /* Koshi: Modified for porting */
    
    //timer_tick(regs);

    timer_tick();
    
        /* FIXME:
         *   use a do while loop to tick exact counts
         */
    _timer_tick_count++;
    
    //write_sequnlock(&xtime_lock);
    MT3351_SETREG32(MT3351_REG_GPT_IRQACK, 1);
    }
    else
    {
    	#ifdef _DEBUG_GPT
    	int numGPT = GPT_Get_IRQSTA();    	
    	printk("[CORE] GTP%d Interrupt Occur\n",numGPT);
    	#else
    	GPT_Get_IRQSTA();
    	#endif
    	GPT_LISR();    	    	

    }
    
    return IRQ_HANDLED;
}

static struct irqaction mt3351_timer_irq =
{
    .name       = "MT3351 Timer Tick",
    .flags      = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
    .handler    = mt3351_timer_interrupt,
};

static void __init mt3351_timer_init(void)
{
    //2009/02/02, Kelvin modify for power management
    //PDN_Power_CONA_DOWN(4,0);
    hwEnableClock(MT3351_CLOCK_GPT);
    /*
     *  CHECK ME !!!!
     *
     *  Disable the GTP0 to avoid the hardware runing and ack the old interrupt
     *  indication.
     *
     *  QUESTION : should GPT be idle when hardware reset ?
     *  NOTE : Could use watchdog to reset the GPT, do it later .....
     *          
     */
    mt3351_wdt_SW_MCUPeripheralReset(MT3351_MCU_PERI_GPT);
    MT3351_WRITEREG32(MT3351_REG_GPT0_CON, 0x12);
    MT3351_WRITEREG32(MT3351_REG_GPT_IRQACK, 0x01);
    /*
     *  Enable Interrupt of GPT0
     */
    MT3351_SETREG32(MT3351_REG_GPT_IRQEN, 0x01);
    /* Infinity, 20080905, Fix incorrect timer parameter { */
    /*
     *  Set the CLK and DIV for GPT0
     *  [5:4]=0x01=>13000000Hz (System Clock), CLK, [3:0]=0x04=>CLKDIV=16. for A Version IC
     *  [5:4]=0x01=>26000000Hz (System Clock), CLK, [3:0]=0x04=>CLKDIV=16. for BD Version IC
     */
    MT3351_WRITEREG32(MT3351_REG_GPT0_CLK, 0x14);     /* 26000000Hz/16=1625000 */ /* 13000000Hz/16=812500 */
    
    /*
     *  Set timeout count of GPT0.
     *  We expected GPT interrupt arise every 10ms. (HZ = 100Hz)
     */
	MT3351_WRITEREG32(MT3351_REG_GPT0_COMPARE, 16250);  /* 1625000/100=16250 */ /* 812500/100=8125 */	
    /* Infinity, 20080905, Fix incorrect timer parameter } */
    
    /*
     *  Initial global data of timer and register the GPT interrupt handler to 
     *  associate IRQ.
     */
    _timer_tick_count = 0;
    setup_irq(MT3351_IRQ_GPT_CODE, &mt3351_timer_irq);
    /*
     *  Set GTP interrupt sensitive as LEVEL sensitive and enable the GTP IRQ
     *  MASK in interrupt controler
     */
    MT3351_IRQSensitivity(MT3351_IRQ_GPT_CODE, MT3351_LEVEL_SENSITIVE);
    MT3351_IRQUnmask(MT3351_IRQ_GPT_CODE);
    /* 
     *  Enable GTP0 in REPEAT MODE
     *
     *  [5:4]=0x10=> REPEAT MODE
     *  [1]=0x02=>Clean th counter to 0;
     *  [0]=0x01=>Enabled GPT1
     */
    MT3351_WRITEREG32(MT3351_REG_GPT0_CON, 0x13);
    return;   
}
        
static unsigned long mt3351_gettimeoffset(void)
{
    return 0;
}

struct sys_timer mt3351_timer =
{
    .init       = mt3351_timer_init,
    .offset     = mt3351_gettimeoffset,
};

