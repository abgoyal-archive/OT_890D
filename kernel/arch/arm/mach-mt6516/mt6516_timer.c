
   
#include <mach/irqs.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/irq.h>

#include <mach/mt6516_gpt_sw.h>
#include <mach/mt6516.h>
#include <linux/sched.h>
#include <mach/mt6516_timer.h>

#define GPT_GET_US(x)   ( x / 13 )

/* macro to read the 32 bit timer */
/* the value in this register is updated regularly by HW */
/* every 13000000 equals to 1s */
#define READ_TIMER_US     GPT_GET_US(__raw_readl(MT6516_XGPT2_COUNT))
#define TIMER_LOAD_VAL    0xffffffff
#define MAX_XGPT_REG_US   GPT_GET_US(TIMER_LOAD_VAL)
#define MAX_TIMESTAMP_US  0xffffffff


ulong hw_timer_us (void)
{	return READ_TIMER_US;
}

static ulong timestamp = 0;
/* lastinc is used to record the counter value (us) in xgpt */
static ulong lastinc;

static spinlock_t timer_lock = SPIN_LOCK_UNLOCKED;

void reset_timestamp(void)
{
	spin_lock(&timer_lock);	

	lastinc   = READ_TIMER_US;
	timestamp = 0;

	spin_unlock(&timer_lock);	
}

ulong get_timestamp (void)
{
	ulong current_counter = 0;

	spin_lock(&timer_lock);		
	
	current_counter = READ_TIMER_US;

	if (current_counter >= lastinc) {
		/* lastinc (xgpt) normal */
		timestamp += current_counter - lastinc; 	
	}
	else 
	{ 
		/* lastinc (xgpt) overflow */
		timestamp += MAX_XGPT_REG_US - lastinc + current_counter; 
	}
	lastinc = current_counter;

	spin_unlock(&timer_lock);	
	
	return timestamp;
}

ulong get_timer_us (ulong base)
{
	ulong current_timestamp = get_timestamp ();

	if(current_timestamp >= base)
	{	/* timestamp normal */
		return current_timestamp - base;
	}
	else
	{	/* timestamp overflow */
		return MAX_TIMESTAMP_US - ( base - current_timestamp );
	}
}

static int __init init_us_timer(void)
{

	/*  init lastinc and timestamp */
    reset_timestamp();

    return 0;
}

arch_initcall(init_us_timer);

