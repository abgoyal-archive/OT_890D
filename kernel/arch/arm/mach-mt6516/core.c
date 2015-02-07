

/* system header files */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/cnt32_to_63.h>

#include <mach/system.h>
#include <mach/hardware.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_gpt_sw.h>

#include <mach/mt6516_ap_config.h>
#include <asm/mach-types.h>
#include <asm/mach/time.h>

#include <linux/kthread.h>
#include <linux/spinlock.h>

#include <linux/clockchips.h>
#ifdef CONFIG_MT6516_SPM
#include <linux/spm.h>
#endif
#include <asm/tcm.h>

extern void MT6516_PLL_init(void);
extern void MT6516_IRQClearInt(unsigned int line);
extern void MT6516_IRQMask(unsigned int line);
extern void MT6516_IRQUnmask(unsigned int line);
extern void MT6516_EINTIRQClearInt(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);
extern void mtk_pm_init(void);
extern UINT32 SetARM9Freq(ARM9_FREQ_DIV_ENUM ARM9_div);
extern void MT6516_EINT_init(void);
#ifdef CONFIG_MT6516_SPM
extern int spm_set_cpu_usage(int id, unsigned usage);
#endif

struct work_struct DVFS_work;
extern unsigned long enter_idle_times;
extern unsigned long total_time;
unsigned long count_timer_tick=0;
spinlock_t DVFS_lock;
unsigned long DVFS_lock_flag;
static int DVFS_thread_timeout = 0;
static DECLARE_WAIT_QUEUE_HEAD(DVFS_thread_wq);

int DVFS_thread_kthread(void *x)
{
	#ifdef CONFIG_MT6516_SPM
	int ret;
	int id = -1;
	struct spm_dp dp;	
	struct sched_param param = { .sched_priority = 1 };    
	sched_setscheduler(current, SCHED_FIFO, &param);
	
    /* Run on a process content */  
    while (1) {           
		spin_lock_irqsave(&DVFS_lock, DVFS_lock_flag);
        if (id < 0) {
			static int count = 0;
			count++;
			if (!(count % 100)) {
				id = spm_get_dp("os_sch", &dp);
				spm_activate_dp(id);
			}			
		} 
		else {
		    ret = spm_set_cpu_usage(id, (total_time/1000)); //idle duration in 100ms => 100000/100 (%)
			if (ret < 0) {
				printk("Warning, SPM is not work!\n");
			}
		}

        total_time = 0;        
        spin_unlock_irqrestore(&DVFS_lock, DVFS_lock_flag);
        wait_event(DVFS_thread_wq, DVFS_thread_timeout);
        DVFS_thread_timeout=0;		
    }
	#endif

    return 0;
}
EXPORT_SYMBOL(DVFS_thread_kthread);

void DVFS_thread_wakeup(void)
{
     DVFS_thread_timeout = 1;
     wake_up(&DVFS_thread_wq);
}

struct irq_chip mt6516_irqchip =
{
    .ack    = MT6516_IRQClearInt,
    .mask   = MT6516_IRQMask,
    .unmask = MT6516_IRQUnmask,
};

/* For external interrupt */
struct irq_chip mt6516_EINTirqchip =
{
    .ack    = MT6516_EINTIRQClearInt,
    .mask   = MT6516_EINTIRQMask,
    .unmask = MT6516_EINTIRQUnmask,
};

void __init mt6516_init_irq(void)
{
    unsigned int irq,eintnum;


    for (irq = 0; irq < MT6516_NUM_IRQ_LINE; irq++)
    {

        set_irq_chip(irq, &mt6516_irqchip);
        set_irq_handler(irq, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }
    eintnum = MT6516_NUM_IRQ_LINE + MT6516_NUM_EINT;
    for (irq = 64; irq < eintnum; irq++)
    {
        set_irq_chip(irq, &mt6516_EINTirqchip);
        set_irq_handler(irq, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }

    SetARM9Freq(DIV_1_416);
    //SetARM9Freq(DIV_2_208);
    //SetARM9Freq(DIV_4_104);

    //EMI_GENA(0x80020070) [3] :PAUSE_STR_EN

    //Kelvin, 2009/07/04, HW self-refresh mode is enabled for suspend
    DRV_SetReg32(0xF0020070, 0x8);

    // Kelvin, 2009/06/15, PLL already set in UBOOT
    //MT6516_PLL_init();

    MT6516_EINT_init();    
    mtk_pm_init();
}


static unsigned long _timer_tick_count;

//#define APXGPT_DEBUG
#ifdef APXGPT_DEBUG
static unsigned long _debug_second_count;
#endif

//===========================================================================
// clock event set mode
//===========================================================================

static void timer_set_mode(enum clock_event_mode mode, struct clock_event_device *clk)
{

	unsigned long ctrl;
	const UINT32 XGPT_CON_ENABLE = 0x0001;
	const UINT32 XGPT_CON_CLEAR = 0x0002;    

	printk("[clock event] timer_set_mode\n");

	ctrl = DRV_Reg32(MT6516_XGPT1_CON);

	switch(mode) {
	case CLOCK_EVT_MODE_PERIODIC:

		printk(" => CLOCK_EVT_MODE_PERIODIC\n");

		// reset counter value
		ctrl |= XGPT_CON_CLEAR;
		
		// clear xgpt mode
		ctrl = ctrl & (~XGPT_FREE_RUN);

		// set xgpt for repeat mode
		ctrl |= XGPT_REPEAT;

		// enable interrupt 
		DRV_SetReg32(MT6516_XGPT_IRQEN, 0x0001);

		// enable xgpt
		ctrl |= XGPT_CON_ENABLE;
		
		break;
	case CLOCK_EVT_MODE_ONESHOT:

		printk(" => CLOCK_EVT_MODE_ONESHOT\n");

		// reset counter value
		ctrl |= XGPT_CON_CLEAR;

		// clear xgpt mode
		ctrl = ctrl & (~XGPT_FREE_RUN);

		// set xgpt for repeat mode
		ctrl |= XGPT_ONE_SHOT;

		// enable interrupt 
		DRV_SetReg32(MT6516_XGPT_IRQEN, 0x0001);		

		// enable xgpt
		ctrl |= XGPT_CON_ENABLE;
		
		break;
	case CLOCK_EVT_MODE_UNUSED:
		printk(" => CLOCK_EVT_MODE_UNUSED\n");	
		
	case CLOCK_EVT_MODE_SHUTDOWN:
	
		printk(" => CLOCK_EVT_MODE_SHUTDOWN\n");	
		// disable xgpt
		//ctrl &= ~XGPT_CON_ENABLE;
		
	default:
		ctrl = 0;
	}	

	__raw_writel(ctrl,MT6516_XGPT1_CON);	
}

//===========================================================================
// clock event set next event
//===========================================================================

static int timer_set_next_event(unsigned long evt,
				struct clock_event_device *unused)
{
	printk("[clock event] timer_set_next_event\n");

	return 0;
}

//===========================================================================
// clock event init
//===========================================================================

static struct clock_event_device system_timer_clockevent =	 {
	.name		= "system_timer_event",
	.shift		= 32,
	.features       = CLOCK_EVT_FEAT_PERIODIC,
	.set_mode	= timer_set_mode,
	.set_next_event	= timer_set_next_event,
	.rating		= 300,
	.cpumask	= cpu_all_mask,
};


static void __init mt6516_clockevents_init(unsigned int timer_irq)
{
	system_timer_clockevent.irq = timer_irq;
	system_timer_clockevent.mult =
		div_sc(1000000, NSEC_PER_SEC, system_timer_clockevent.shift);
	system_timer_clockevent.max_delta_ns =
		clockevent_delta2ns(0xffffffff, &system_timer_clockevent);
	system_timer_clockevent.min_delta_ns =
		clockevent_delta2ns(0xf, &system_timer_clockevent);

	clockevents_register_device(&system_timer_clockevent);
}


static irqreturn_t __tcmfunc
mt6516_timer_interrupt(int irq, void *dev_id)
{

    #ifdef CONFIG_GENERIC_TIME 
                struct clock_event_device *evt = NULL;
    #endif

    if(XGPT_Check_IRQSTA(XGPT1))
    {

		#ifdef CONFIG_GENERIC_TIME    
	    	evt = &system_timer_clockevent;
			evt->event_handler(evt);
		#else
		        timer_tick();
		        _timer_tick_count++;
		#endif	
		
		DRV_WriteReg32(MT6516_XGPT_IRQACK, 1);
	}	
	else
	{	XGPT_LISR();    	   
	}

	#ifdef CONFIG_MT6516_SPM
	count_timer_tick++;  

	if(count_timer_tick >= 10)
	{			
		DVFS_thread_wakeup();
		count_timer_tick=0;							
	}
	#endif
	
	return IRQ_HANDLED;
}

static struct irqaction mt6516_timer_irq =
{
    .name       = "MT6516 Timer Interrupt",
    .flags      = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
    .handler    = mt6516_timer_interrupt,
};

//===========================================================================
// system_timer
//===========================================================================
static cycle_t mt6516_system_timer_get_cycles(void)
{
	return __raw_readl(MT6516_XGPT2_COUNT);
}


// nanosecond = ( clocksource->read * clocksource->mult ) >> clocksource->shift
static struct clocksource clocksource_system_timer_mt6516 = {
	.name	= "system_timer",
	.rating	= 300,
	.read	= mt6516_system_timer_get_cycles,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 20,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init mt6516_system_timer_clocksource_init(void)
{
    // Enable power bit
    DRV_SetReg32(0xF0039340, 0x10000000);

    /*
     *  Disable the XGTP1 to avoid the hardware runing and ack the old interrupt
     *  indication.       
     */
    DRV_WriteReg32(MT6516_XGPT1_CON, 0x12);
    DRV_WriteReg32(MT6516_XGPT_IRQACK, 0x01);
    /*
     *  Enable Interrupt of XGPT1
     */
    DRV_SetReg32(MT6516_XGPT_IRQEN, 0x01);

    if(HZ == 100)
	{
		//  Specify the resolution
	    //DRV_WriteReg32(MT6516_XGPT1_PRESCALE, 0x4);	    	    
	    DRV_WriteReg32(MT6516_XGPT1_PRESCALE, 0x0);	    	    
	    // Set timeout count of XGPT0.
	    //  We expected GPT interrupt arise every 10ms. (HZ = 100Hz)
		//DRV_WriteReg32(MT6516_XGPT1_COMPARE, 8125);  /*MT6516 812500/100=8125 */ 
		DRV_WriteReg32(MT6516_XGPT1_COMPARE, 130000);  /*MT6516 13000000/100=130000 */ 
		printk("System Timer = 100Hz\n");
	}
	else if(HZ == 250)
	{
		//  Specify the resolution
	    //DRV_WriteReg32(MT6516_XGPT1_PRESCALE, 0x4);	    	    
	    DRV_WriteReg32(MT6516_XGPT1_PRESCALE, 0x0);	    	    
	    // Set timeout count of XGPT0.
	    //  We expected GPT interrupt arise every 1000/250 = 4ms (HZ = 250Hz)
		//DRV_WriteReg32(MT6516_XGPT1_COMPARE, 3250);  /*MT6516 812500/250=3250 */
		DRV_WriteReg32(MT6516_XGPT1_COMPARE, 52000);  /*MT6516 13000000/250=52000*/
		printk("System Timer = 250Hz\n");
	}
	else if(HZ == 1000)
	{
		//  Specify the resolution
	    //DRV_WriteReg32(MT6516_XGPT1_PRESCALE, 0x3);	    	    
	    DRV_WriteReg32(MT6516_XGPT1_PRESCALE, 0x0);	    	    
	    // Set timeout count of XGPT0.
	    //  We expected GPT interrupt arise every 1000/1000 = 1ms (HZ = 1000Hz)
		//DRV_WriteReg32(MT6516_XGPT1_COMPARE, 1625);  /*MT6516 1625000/1000=1625 */
		DRV_WriteReg32(MT6516_XGPT1_COMPARE, 13000);  /*MT6516 13000000/1000=13000 */
		printk("System Timer = 1000Hz\n");
	}	
    
    /*
     *  Initial global data of timer and register the GPT interrupt handler to 
     *  associate IRQ.
     */
    _timer_tick_count = 0;


	//printk("System Timer (XGPT1) = free run mode\n");
    /* 
     *  Enable XXGTP1 in Free-Run MODE
     *
     *  [5:4]=0x10=> REPEAT MODE
     *  [1]=0x02=>Clean th counter to 0;
     *  [0]=0x01=>Enabled GPT1
     */
    DRV_WriteReg32(MT6516_XGPT1_CON, 0x13);

	#ifdef CONFIG_GENERIC_TIME    
		clocksource_system_timer_mt6516.mult =
			clocksource_khz2mult(13000, clocksource_system_timer_mt6516.shift);
		clocksource_register(&clocksource_system_timer_mt6516);
	#endif		
	
	
    /*  disable the XGTP2 to avoid the hardware runing and ack the old interrupt indication. */
    DRV_WriteReg32(MT6516_XGPT2_CON, 0x32);
    DRV_WriteReg32(MT6516_XGPT_IRQACK, 0x02);

    /*  specify the resolution 13000000Hz */
    DRV_WriteReg32(MT6516_XGPT2_PRESCALE, 0x0);

    /*  enable XGTP2 in Freerun MODE  */
    DRV_WriteReg32(MT6516_XGPT2_CON, 0x33);   	
}


unsigned long long sched_clock(void)
{
	unsigned long long v = cnt32_to_63(mt6516_system_timer_get_cycles());

	/* the <<1 gets rid of the cnt_32_to_63 top bit saving on a bic insn */	
	// 1000_000_000 (ns) / 13_000_000 (HZ) = 1000 / 13 (ns / HZ)
	v *= 1000<<1;
	do_div(v, 13<<1);

//	printk("sched_clock = %ul\n",v);
	return v;
}




//#define HZ 250

static void __init mt6516_timer_init(void)
{
    #ifndef MT6516_IRQ_APXGPT_CODE
    #define MT6516_IRQ_APXGPT_CODE 0x39
    #endif
    setup_irq(MT6516_IRQ_APXGPT_CODE, &mt6516_timer_irq);
    /*
     *  Set GTP interrupt sensitive as LEVEL sensitive and enable the GTP IRQ
     *  MASK in interrupt controler
     */
    MT6516_IRQSensitivity(MT6516_IRQ_APXGPT_CODE, MT6516_LEVEL_SENSITIVE);
    MT6516_IRQUnmask(MT6516_IRQ_APXGPT_CODE);

	#ifdef CONFIG_GENERIC_TIME    
		printk("mt6516_system_timer_clocksource_init\n");
		mt6516_system_timer_clocksource_init();
		printk("mt6516_clockevents_init\n");
		mt6516_clockevents_init(MT6516_IRQ_APXGPT_CODE);
	#else
		mt6516_system_timer_clocksource_init();
	#endif
	
  
    return;   
}

#define SYS_MODE 0x1F

static void MT6516_load_regs( struct pt_regs *ptr )
{
    asm volatile(
    "stmia %0, {r0 - r15}\n\t"
    :
    : "r" (ptr)
    : "memory"
    );
}

void MT6516_traceCallStack( void )
{
    struct pt_regs *ptr;
    unsigned int fp;
    unsigned long flags;
    
    ptr = kmalloc( sizeof( struct pt_regs ), GFP_KERNEL);
    
    spin_lock_irqsave(1, flags);
    
    printk("MT6516 driver tracking begin");
    MT6516_load_regs( ptr );
    fp = ptr->ARM_fp;
    c_backtrace(fp, SYS_MODE);
    printk("MT6516 driver tracking end\n\n");
    
    spin_unlock_irqrestore(1, flags);
    
    kfree ( ptr );
}
EXPORT_SYMBOL(MT6516_traceCallStack);

#ifndef CONFIG_GENERIC_TIME         
static unsigned long mt6516_gettimeoffset(void)
{
    return 0;
}
#endif

struct sys_timer mt6516_timer =
{
    .init       = mt6516_timer_init,
    #ifndef CONFIG_GENERIC_TIME 
	.offset     = mt6516_gettimeoffset,
	#endif	  
};

