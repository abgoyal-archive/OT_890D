
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>

#include <linux/interrupt.h>
#include <linux/rtc.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/mt3351-rtc.h>
#include <linux/device.h>

#include <linux/time.h>
#include <linux/timex.h>
#include <linux/timer.h>
#include <asm/mach/time.h>

#include <asm/mach/time.h>
#include <linux/android_alarm.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>

#define MTK_WriteReg32(addr,data)   ((*(volatile unsigned int *)(addr)) = (unsigned int)data)
#define MTK_Reg32(addr)             (*(volatile unsigned int *)(addr))

//static struct platform_driver mt3351_rtcdrv;
//static struct miscdevice rtc_dev;
//static const struct rtc_class_ops mt3351_rtcops;
//static struct platform_device mt3351_rtcdev;
static char __initdata mt3351_banner[];

static spinlock_t rtc_lock = SPIN_LOCK_UNLOCKED;
static struct fasync_struct *rtc_async_queue;
static DECLARE_WAIT_QUEUE_HEAD(rtc_wait);
/* frequency */
static int MT3351_rtc_pIRQf   = 32 ; // default value. Its range is from 2HZ to 32HZ in increments of powers of two.
static unsigned long rtc_status = 0;	/* bitmapped status byte.	*/
static unsigned long rtc_irq_data = 0;	/* our output to the world	*/
static unsigned long epoch = 1900;	/* year corresponding to 0x00, default value in the linux for rtc	*/
static const unsigned char days_in_mo[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
/*rtc_al_year'r range is from 0 to 99 and default epoch year in linux is 1900 so need a shift year.*/
//static unsigned long rtc_year_shift = 2000 - epoch; 
#define rtc_year_shift (1970-epoch)

static bool mt3351_rtc_IsHWkeyReady(void){
    return (MTK_Reg32(MT3351_RTC_PROT1)==RTC_PROT1_KEY && MTK_Reg32(MT3351_RTC_PROT2)==RTC_PROT2_KEY
        && MTK_Reg32(MT3351_RTC_PROT3)==RTC_PROT3_KEY && MTK_Reg32(MT3351_RTC_PROT4)==RTC_PROT4_KEY);
}

static bool mt3351_rtc_IsProt_Pass(void)
{    
    return (MTK_Reg32(MT3351_RTC_CTL) & MT3351_PORT_PASS_MASK)? true : false;
}

static bool mt3351_rtc_IsDebounceOk(void)
{
    return (MTK_Reg32(MT3351_RTC_CTL) & MT3351_DEBNCE_OK_MASK )? true : false;
}

static bool mt3351_rtc_IsInhibit_WR(void)
{
    return (MTK_Reg32(MT3351_RTC_CTL) & MT3351_INHIBIT_WR_MASK)? true:false;
}


static void mt3351_rtc_power_inconsistent_init(void)
{
// ---------- can be moved to bootloader

    while(!mt3351_rtc_IsDebounceOk());
    
    do{
        MTK_WriteReg32(MT3351_RTC_PROT1,RTC_PROT1_KEY);
        MTK_WriteReg32(MT3351_RTC_PROT2,RTC_PROT2_KEY);
        MTK_WriteReg32(MT3351_RTC_PROT3,RTC_PROT3_KEY);
        MTK_WriteReg32(MT3351_RTC_PROT4,RTC_PROT4_KEY);
        MTK_WriteReg32(MT3351_RTC_PROT1,RTC_PROT1_KEY);
        MTK_WriteReg32(MT3351_RTC_PROT2,RTC_PROT2_KEY);
        MTK_WriteReg32(MT3351_RTC_PROT3,RTC_PROT3_KEY);
        MTK_WriteReg32(MT3351_RTC_PROT4,RTC_PROT4_KEY);
        
    }while(!mt3351_rtc_IsHWkeyReady()&& !mt3351_rtc_IsProt_Pass());
//----------------------------------------------
    MTK_WriteReg32(MT3351_RTC_CTL,0x91);// stop ripple counter
    MTK_WriteReg32(MT3351_RTC_AL_CTL,0x00); // clear all alarm enable
    MTK_WriteReg32(MT3351_RTC_TIMER_CTL,0x08); // clear timer enable and setup pin RTC_OUT driving
// ---------- can be moved to bootloader
    MTK_WriteReg32(MT3351_RTC_XOSC_CFG, 0x0A);
    MTK_WriteReg32(MT3351_RTC_DEBOUNCE, 0x03);
    MTK_WriteReg32(MT3351_RTC_IRQ_STA, 0x10);
//----------------------------------------------
    // set default time for RTC time
    MTK_WriteReg32(MT3351_RTC_TC_YEA,MT3351_RTC_YEAR);
    MTK_WriteReg32(MT3351_RTC_TC_MON,MT3351_RTC_MON);       
    MTK_WriteReg32(MT3351_RTC_TC_DOM,MT3351_RTC_DOM);
    MTK_WriteReg32(MT3351_RTC_TC_HOU,MT3351_RTC_HOU);
    MTK_WriteReg32(MT3351_RTC_TC_MIN,MT3351_RTC_MIN);
    MTK_WriteReg32(MT3351_RTC_TC_SEC,MT3351_RTC_SEC);
    // set default value to alarm registers
    MTK_WriteReg32(MT3351_RTC_AL_YEAR,0);
    MTK_WriteReg32(MT3351_RTC_AL_MON,0);       
    MTK_WriteReg32(MT3351_RTC_AL_DOM,0);
    MTK_WriteReg32(MT3351_RTC_AL_HOU,0);
    MTK_WriteReg32(MT3351_RTC_AL_MIN,0);
    MTK_WriteReg32(MT3351_RTC_AL_SEC,0);
    // set default value to timer register
    MTK_WriteReg32(MT3351_RTC_TIMER_CNTH,0);       
    MTK_WriteReg32(MT3351_RTC_TIMER_CNTL,0);  
    //write software key 
    MTK_WriteReg32(MT3351_RTC_KEY,MT3351_RTC_KEY_VALUE);
    
}

static int mt3351_rtc_open(struct device *deve)
{
    spin_lock_irq (&rtc_lock);

	if(rtc_status & RTC_IS_OPEN)
		goto out_busy;

	rtc_status |= RTC_IS_OPEN;

	rtc_irq_data = 0;
	spin_unlock_irq (&rtc_lock);
	return 0;

out_busy:
	spin_unlock_irq (&rtc_lock);
	return -EBUSY;
}

static void mt3351_rtc_release(struct device *dev)
{
	/*
	if (file->f_flags & FASYNC) {
		mt3351_rtc_fasync (-1, file, 0);
	}
	*/

	spin_lock_irq (&rtc_lock);
	rtc_irq_data = 0;
	spin_unlock_irq (&rtc_lock);

	rtc_status &= ~RTC_IS_OPEN;
}


static ssize_t mt3351_rtc_read(struct file *file, char *buf,size_t count, loff_t *ppos)
{
    DECLARE_WAITQUEUE(wait, current);
	unsigned long data;
	ssize_t retval;
	
	if (count < sizeof(unsigned long))
		return -EINVAL;

	add_wait_queue(&rtc_wait, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	for (;;) {
		spin_lock_irq (&rtc_lock);
		data = rtc_irq_data;
		if (data != 0) {
			rtc_irq_data = 0;
			break;
		}
		spin_unlock_irq (&rtc_lock);

		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto out;
		}
		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}
		schedule();
	}

	spin_unlock_irq (&rtc_lock);
	retval = put_user(data, (unsigned long *)buf); 
	if (!retval)
		retval = sizeof(unsigned long); 
 out:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&rtc_wait, &wait);

	return retval;
}


int get_rtc_time(struct device *dev,struct rtc_time *rtc_tm)
{
    while(mt3351_rtc_IsInhibit_WR());
    spin_lock_irq(&rtc_lock);
	rtc_tm->tm_sec = MTK_Reg32(MT3351_RTC_TC_SEC);
	//printk(KERN_INFO "%s: rtc_tm->tm_sec = %d\n", __FUNCTION__,rtc_tm->tm_sec);	
	
	rtc_tm->tm_min = MTK_Reg32(MT3351_RTC_TC_MIN);
	//printk(KERN_INFO "%s: rtc_tm->tm_min = %d\n", __FUNCTION__,rtc_tm->tm_min);		
	
	rtc_tm->tm_hour = MTK_Reg32(MT3351_RTC_TC_HOU);
	//printk(KERN_INFO "%s: rtc_tm->tm_hour = %d\n", __FUNCTION__,rtc_tm->tm_hour);		
	
	rtc_tm->tm_mday = MTK_Reg32(MT3351_RTC_TC_DOM);
	//printk(KERN_INFO "%s: rtc_tm->tm_mday = %d\n", __FUNCTION__,rtc_tm->tm_mday);		
	
	rtc_tm->tm_mon = MTK_Reg32(MT3351_RTC_TC_MON);
	//printk(KERN_INFO "%s: rtc_tm->tm_mon = %d\n", __FUNCTION__,rtc_tm->tm_mon);		
	
	rtc_tm->tm_year = MTK_Reg32(MT3351_RTC_TC_YEA)+rtc_year_shift;	
	//printk(KERN_INFO "%s: rtc_tm->tm_year = %d\n", __FUNCTION__,rtc_tm->tm_year);		
	
	rtc_tm->tm_isdst = -1;
	spin_unlock_irq(&rtc_lock);
	rtc_tm->tm_mon--;

	printk(KERN_INFO "%s: setting RTC time to %04d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		 rtc_tm->tm_year + 1900, rtc_tm->tm_mon + 1, rtc_tm->tm_mday, rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);
    
	return 0;
}

int set_rtc_time(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned char mon, day, hrs, min, sec, leap_yr;
    unsigned int yrs;
    
	    yrs = rtc_tm->tm_year + epoch ;
		mon = rtc_tm->tm_mon+1;   /* tm_mon starts at zero */
		day = rtc_tm->tm_mday;
		hrs = rtc_tm->tm_hour;
		min = rtc_tm->tm_min;
		sec = rtc_tm->tm_sec;

		
		printk(KERN_INFO "%s: setting RTC time to %04d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
			 rtc_tm->tm_year + 1900, rtc_tm->tm_mon + 1, rtc_tm->tm_mday, rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);		

		if (yrs < 1970)
			return -EINVAL;

		leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

		if ((mon > 12) || (day == 0))
			return -EINVAL;

		if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr)))
			return -EINVAL;

		if ((hrs >= 24) || (min >= 60) || (sec >= 60))
			return -EINVAL;

		if ((yrs -= (epoch+rtc_year_shift)) > 99)    /* They are unsigned */
			return -EINVAL;
		while(mt3351_rtc_IsInhibit_WR());
		spin_lock_irq(&rtc_lock);
		MTK_WriteReg32(MT3351_RTC_TC_YEA, yrs);
		MTK_WriteReg32(MT3351_RTC_TC_MON, mon);
		MTK_WriteReg32(MT3351_RTC_TC_DOM, day);
		MTK_WriteReg32(MT3351_RTC_TC_HOU, hrs);
		MTK_WriteReg32(MT3351_RTC_TC_MIN, min);
		MTK_WriteReg32(MT3351_RTC_TC_SEC, sec);
		spin_unlock_irq(&rtc_lock);
		return 0;
}

int get_rtc_alm_time(struct device *dev, struct rtc_time *rtc_tm)
{
    while(mt3351_rtc_IsInhibit_WR());
    spin_lock_irq(&rtc_lock);
	rtc_tm->tm_sec = MTK_Reg32(MT3351_RTC_AL_SEC);
	rtc_tm->tm_min = MTK_Reg32(MT3351_RTC_AL_MIN);
	rtc_tm->tm_hour = MTK_Reg32(MT3351_RTC_AL_HOU);
	rtc_tm->tm_isdst = -1;
	spin_unlock_irq(&rtc_lock);

   	return 0;
}

int set_rtc_alm_time(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned char min, sec, hrs;
    
		hrs = rtc_tm->tm_hour;
		min = rtc_tm->tm_min;
		sec = rtc_tm->tm_sec;
		if ((hrs >= 24) || (min >= 60) || (sec >= 60))
			return -EINVAL;

		while(mt3351_rtc_IsInhibit_WR());
		spin_lock_irq(&rtc_lock);
		MTK_WriteReg32(MT3351_RTC_AL_HOU, hrs);
		MTK_WriteReg32(MT3351_RTC_AL_MIN, min);
		MTK_WriteReg32(MT3351_RTC_AL_SEC, sec);
		spin_unlock_irq(&rtc_lock);
		return 0;

}

static int get_rtc_wkalm_time(struct device *dev, struct rtc_wkalrm *wkalrm_tm)
{
    struct rtc_time *rtc_tm;
    rtc_tm = &(wkalrm_tm->time);
    while(mt3351_rtc_IsInhibit_WR());
    spin_lock_irq(&rtc_lock);
	rtc_tm->tm_sec = MTK_Reg32(MT3351_RTC_AL_SEC);
	rtc_tm->tm_min = MTK_Reg32(MT3351_RTC_AL_MIN);
	rtc_tm->tm_hour = MTK_Reg32(MT3351_RTC_AL_HOU);
	rtc_tm->tm_mday = MTK_Reg32(MT3351_RTC_AL_DOM);
	rtc_tm->tm_mon = MTK_Reg32(MT3351_RTC_AL_MON);
	rtc_tm->tm_year = MTK_Reg32(MT3351_RTC_AL_YEAR)+ rtc_year_shift + epoch;	
	spin_unlock_irq(&rtc_lock);
	rtc_tm->tm_mon--;

	return 0;

}

static int set_rtc_wkalm_time(struct device *dev, struct rtc_wkalrm *wkalrm_tm)
{

        struct rtc_time rtc_tm;
        unsigned char mon, day, hrs, min, sec, leap_yr;
        unsigned int yrs;
        unsigned tmp;
        
        rtc_tm = wkalrm_tm->time;
         
        yrs = rtc_tm.tm_year + epoch ;
		mon = rtc_tm.tm_mon+1;
		day = rtc_tm.tm_mday;
		hrs = rtc_tm.tm_hour;
		min = rtc_tm.tm_min;
		sec = rtc_tm.tm_sec;

		if (yrs < 1970)
			return -EINVAL;

		leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

		if ((mon > 12) || (day == 0))
			return -EINVAL;

		if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr)))
			return -EINVAL;
			
		if ((hrs >= 24) || (min >= 60) || (sec >= 60))
			return -EINVAL;

		if ((yrs -= (epoch + rtc_year_shift)) > 99)    /* They are unsigned */
			return -EINVAL;
		
		while(mt3351_rtc_IsInhibit_WR());
		spin_lock_irq(&rtc_lock);
		MTK_WriteReg32(MT3351_RTC_AL_YEAR, yrs);
		MTK_WriteReg32(MT3351_RTC_AL_MON, mon);
		MTK_WriteReg32(MT3351_RTC_AL_DOM, day);
		MTK_WriteReg32(MT3351_RTC_AL_HOU, hrs);
		MTK_WriteReg32(MT3351_RTC_AL_MIN, min);
		MTK_WriteReg32(MT3351_RTC_AL_SEC, sec);
	
		tmp = MTK_Reg32(MT3351_RTC_AL_CTL);
		if(wkalrm_tm->enabled)
		{
            tmp = 0xEF; //no day of 
		}
		else
		{
            tmp = 0x00;
		}
		MTK_WriteReg32(MT3351_RTC_AL_CTL,tmp);

		
		spin_unlock_irq(&rtc_lock);

		return 0;


}

static void mt3351_rtc_gettimeofday(void)
{
	struct timeval time;
	struct rtc_time tm;

	/* set rtc's value to current time of day */

	do_gettimeofday(&time);
	rtc_time_to_tm(time.tv_sec, &tm);
	printk(KERN_INFO "%s: System Time %04d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
			 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	set_rtc_time(NULL,&tm);
}

static void mt3351_rtc_settimeofday(void)
{
	struct rtc_time tm;
	struct timespec time;

	/* set current time of day to rtc's value */

	get_rtc_time(NULL,&tm);
	printk(KERN_INFO "%s: RTC %04d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
			 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	rtc_tm_to_time(&tm, &time.tv_sec);
	time.tv_nsec = 0;
	do_settimeofday(&time);
}

static int mt3351_set_rtc(void)
{
	printk(KERN_INFO "mt3351 set rtc\n");
	mt3351_rtc_gettimeofday();
	return 0;
}


static int mt3351_rtc_ioctl(struct device *dev,unsigned int cmd, unsigned long arg)
{
	struct rtc_time wtime; 
	struct rtc_wkalrm wkalmtm;
    unsigned long tmp;

	printk(KERN_INFO "MT3351 RTC IOCTL is called!!!!!!!!!\n");
    
	switch (cmd) {
	case RTC_AIE_ON:  /* set alarm enab. bit	*/
	{
		//printk(KERN_INFO "CMD: RTC_AIE_ON\n");
        tmp = MTK_Reg32(MT3351_RTC_AL_CTL);
        tmp &= ~0xff ; 
        tmp |= 0x0f ; // enable alram, sec, min, hour
        MTK_WriteReg32(MT3351_RTC_AL_CTL, tmp);
        return 0;
	}
	case RTC_AIE_OFF: /* clear alarm enab. bit	*/
	{    
		//printk(KERN_INFO "CMD: RTC_AIE_OFF\n");
        tmp = MTK_Reg32(MT3351_RTC_AL_CTL);
        tmp &= ~MT3351_Alarm_EN_MASK;
        MTK_WriteReg32(MT3351_RTC_AL_CTL, tmp);
        return 0;
	}
	case RTC_ALM_SET:
	{
		//printk(KERN_INFO "CMD: RTC_ALM_SET\n");	
        struct rtc_time alm_tm;
		if (copy_from_user(&alm_tm, (struct rtc_time*)arg,
				   sizeof(struct rtc_time)))
			return -EFAULT; 			
        return set_rtc_alm_time(NULL,&alm_tm);	        
	}	
    case RTC_ALM_READ: 
    {
		//printk(KERN_INFO "CMD: RTC_ALM_READ\n");	    
        memset(&wtime, 0, sizeof(struct rtc_time));
		get_rtc_alm_time(NULL,&wtime);
		break;
    }
    case RTC_SET_TIME: // revisit
    {
		//printk(KERN_INFO "CMD: RTC_SET_TIME\n");	    
		struct rtc_time rtc_tm;
		if (!capable(CAP_SYS_TIME))
			return -EACCES;

		if (copy_from_user(&rtc_tm, (struct rtc_time*)arg,
				   sizeof(struct rtc_time)))
			return -EFAULT;
        return set_rtc_time(NULL,&rtc_tm);
		
    }
    case RTC_RD_TIME:  
    {	
		//printk(KERN_INFO "CMD: RTC_RD_TIME\n");    
        memset(&wtime, 0, sizeof(struct rtc_time));
        get_rtc_time(NULL,&wtime);
        break;
    }    
    case RTC_EPOCH_READ:
    {
		//printk(KERN_INFO "CMD: RTC_EPOCH_READ\n");     
		return put_user(epoch, (unsigned long *)arg);
	}
    case RTC_EPOCH_SET:  
    {
		//printk(KERN_INFO "CMD: RTC_EPOCH_SET\n");    
		/* 
		 * There were no RTC clocks before 1900.Default epoch year should be 1900.
		 */
		unsigned long year = (*(unsigned long *)arg);
		if (year < 1970)
			return -EINVAL;

		if (!capable(CAP_SYS_TIME))
			return -EACCES;
        /*arg is a pointer address to unsigned long. Can change to copy_from_user*/
		epoch = year ;
		return 0;
	}	
    case RTC_WKALM_SET:
    {
		//printk(KERN_INFO "CMD: RTC_WKALM_SET\n");       
        struct rtc_wkalrm alm_tm;
	    if (copy_from_user(&alm_tm, (struct rtc_wkalrm*)arg,
				   sizeof(struct rtc_wkalrm)))
			return -EFAULT;

        return set_rtc_wkalm_time(NULL,&alm_tm);	
    }
    case RTC_WKALM_RD:
    {
		//printk(KERN_INFO "CMD: RTC_WKALM_RD\n");      
        memset(&wkalmtm, 0, sizeof(struct rtc_time));
		get_rtc_wkalm_time(NULL,&wkalmtm);
		return copy_to_user((void *)arg, &wkalmtm, sizeof (struct rtc_wkalrm)) ? -EFAULT : 0;
    } 
    case RTC_UIE_ON:
    {
    
        unsigned long  status;
		//printk(KERN_INFO "CMD: RTC_UIE_ON\n"); 
        /*more careful, it is better to use spinlock here to protect RTC_TIMER_CTL*/
        status = MTK_Reg32(MT3351_RTC_TIMER_CTL);
        if(!(status&MT3351_INTEN_MASK))
        {   /* if the timer is currently for exteranl interrupt use, RTC don't support PEI*/
            if(status&MT3351_EXTEN_MASK)
                return EINVAL;
            MTK_WriteReg32(MT3351_RTC_TIMER_CNTH, 0);
            MTK_WriteReg32(MT3351_RTC_TIMER_CNTL, 32 - TIMER_SETCNT_TIME);
            /* turn on timer internal interrupt */
            status |= MT3351_INTEN_ON;   
            MTK_WriteReg32(MT3351_RTC_TIMER_CTL,status);
        }
        return 0;
    }
    case RTC_UIE_OFF:  
	{
	
	    unsigned long status;	    
		//printk(KERN_INFO "CMD: RTC_UIE_OFF\n"); 	    
        status = MTK_Reg32(MT3351_RTC_TIMER_CTL);
        if((status&MT3351_INTEN_MASK))
        {
            status &= ~(MT3351_INTEN_MASK);
            MTK_WriteReg32(MT3351_RTC_TIMER_CTL,status);
           // if(!(status & MT3351_EXTEN_MASK))
         
           // MTK_WriteReg32(MT3351_RTC_TIMER_CNTL,0x00);
        }
        return 0;
	}   
    case RTC_PIE_ON:
    {    
        unsigned long  status;
		//printk(KERN_INFO "CMD: RTC_PIE_ON\n");         
        /*more careful, it is better to use spinlock here to protect RTC_TIMER_CTL*/
        status = MTK_Reg32(MT3351_RTC_TIMER_CTL);
        if(!(status&MT3351_INTEN_MASK))
        {   /* if the timer is currently for exteranl interrupt use, RTC don't support PEI*/
            if(status&MT3351_EXTEN_MASK)
                return EINVAL;
            MTK_WriteReg32(MT3351_RTC_TIMER_CNTH, 0);
            MTK_WriteReg32(MT3351_RTC_TIMER_CNTL, 32/MT3351_rtc_pIRQf - TIMER_SETCNT_TIME);
            /* turn on timer internal interrupt */
            status |= MT3351_INTEN_ON;   
            MTK_WriteReg32(MT3351_RTC_TIMER_CTL,status);
        }
        return 0;
    }
	case RTC_PIE_OFF:  
	{
	    unsigned long status;	
	    //printk(KERN_INFO "CMD: RTC_PIE_OFF\n"); 
        status = MTK_Reg32(MT3351_RTC_TIMER_CTL);
        if((status&MT3351_INTEN_MASK))
        {
            status &= ~(MT3351_INTEN_MASK);
            MTK_WriteReg32(MT3351_RTC_TIMER_CTL,status);
           // if(!(status & MT3351_EXTEN_MASK))
         
           // MTK_WriteReg32(MT3351_RTC_TIMER_CNTL,0x00);
        }
        return 0;
	}   
    case RTC_IRQP_SET:
    {
        unsigned long freq,status;
	    //printk(KERN_INFO "CMD: RTC_IRQP_SET\n");        
        if(copy_from_user(&freq, (unsigned long*)arg,sizeof(unsigned long)))
			return -EFAULT;
        //  if(freq<2 || freq>32)
        //  fix me !! freq's range. It can only be 2^i where i's range is 1 to 5
        if(freq<0 || freq>32) //If freq = 1. can simulate UIE,  
            return -EINVAL; 
        if(!capable(CAP_SYS_RESOURCE))
            return -EFAULT;

        spin_lock_irq (&rtc_lock); // protect rtc_timer_ctl
        
        status = MTK_Reg32(MT3351_RTC_TIMER_CTL);
        if(status&MT3351_INTEN_MASK)
        {
            /* 
               Timer can support internal and external interrupt at the same time.
            */
            if(status & MT3351_EXTEN_MASK)
            {
                spin_unlock_irq (&rtc_lock);  
                return -EFAULT;
            }
            // disable internal interrupt
            status &= ~(MT3351_INTEN_MASK);
            MTK_WriteReg32(MT3351_RTC_TIMER_CTL,status);
            MT3351_rtc_pIRQf= freq;
            MTK_WriteReg32(MT3351_RTC_TIMER_CNTH, 0);
            MTK_WriteReg32(MT3351_RTC_TIMER_CNTL,(32/MT3351_rtc_pIRQf));
            status |= MT3351_INTEN_ON;
            MTK_WriteReg32(MT3351_RTC_TIMER_CTL,status);
            spin_unlock_irq (&rtc_lock);  
              
        }
        else
        {
            MT3351_rtc_pIRQf= freq;
            spin_unlock_irq (&rtc_lock);  
        }
        return 0;
    } 
    case RTC_IRQP_READ:  
	    //printk(KERN_INFO "CMD: RTC_IRQP_READ\n");     
	    return put_user(MT3351_rtc_pIRQf, (unsigned long __user *)arg);
	    

    default:
	    //printk(KERN_INFO "CMD: default\n");	    
        return -EINVAL;
	}

    //printk(KERN_INFO "CMD: default\n");	    
	return copy_to_user((void *)arg, &wtime, sizeof (struct rtc_time)) ? -EFAULT : 0;


}

 
//static int mt3351_rtc_proc_output(char *buf)
static int mt3351_rtc_proc_output(struct device *dev, struct seq_file *seq)
{
#define YN(value) ((value) ? "Yes" : "No")
#define ED(value) ((value) ? "Enable" : "Disable")

	
	struct rtc_time tm;
	struct rtc_wkalrm wkalrm_tm;	

	get_rtc_time(NULL,&tm);

	/*rtc time*/
	seq_printf(seq, "rtc_time\t: %02d:%02d:%02d\n"
		     "rtc_date\t: %04d-%02d-%02d\n"
	 	     "rtc_epoch\t: %04lu\n",
		     tm.tm_hour, tm.tm_min, tm.tm_sec,
		     tm.tm_year +(int)epoch, tm.tm_mon + 1, tm.tm_mday, epoch);

	get_rtc_wkalm_time(NULL,&wkalrm_tm);

	/*wkalarm time*/
	seq_printf(seq, "alarm_time\t: %02d:%02d:%02d\n"
		     "alarm_date\t: %04d-%02d-%02d\n",
		     wkalrm_tm.time.tm_hour, wkalrm_tm.time.tm_min, wkalrm_tm.time.tm_sec,
		     wkalrm_tm.time.tm_year, wkalrm_tm.time.tm_mon + 1, wkalrm_tm.time.tm_mday);

	/* show some basic info of RTC supporting features */ 
	seq_printf(seq, "BCD\t\t: %s\n"
		     "24hr\t\t: %s\n"
		     "alarm_IRQ(AIE)\t: %s\n"
		     "uie_IRQ(UIE)\t: No Support\n"
		     "period_IRQ(PIE)\t: %s\n"
		     "update_rate\t: %d HZ\n",
		     YN(0),
		     YN(1),
		     ED(MTK_Reg32(MT3351_RTC_AL_CTL) &
	 		 MT3351_Alarm_EN_MASK),
		     ED(MTK_Reg32(MT3351_RTC_TIMER_CTL) &
			 MT3351_INTEN_MASK),
		     MT3351_rtc_pIRQf);		     

	return 0;		    

#if 0	
	char *p;
	p = buf;

    /*rtc time*/
	p += sprintf(p,
		     "rtc_time\t: %02d:%02d:%02d\n"
		     "rtc_date\t: %04d-%02d-%02d\n"
	 	     "rtc_epoch\t: %04lu\n",
		     tm.tm_hour, tm.tm_min, tm.tm_sec,
		     tm.tm_year +(int)epoch, tm.tm_mon + 1, tm.tm_mday, epoch);

	get_rtc_wkalm_time(NULL,&wkalrm_tm);

    /*wkalarm time*/
	p += sprintf(p,
		     "alarm_time\t: %02d:%02d:%02d\n"
		     "alarm_date\t: %04d-%02d-%02d\n",
		     wkalrm_tm.time.tm_hour, wkalrm_tm.time.tm_min, wkalrm_tm.time.tm_sec,
		     wkalrm_tm.time.tm_year, wkalrm_tm.time.tm_mon + 1, wkalrm_tm.time.tm_mday);
    /* show some basic info of RTC supporting features */ 
	p += sprintf(p,
		     "BCD\t\t: %s\n"
		     "24hr\t\t: %s\n"
		     "alarm_IRQ(AIE)\t: %s\n"
		     "uie_IRQ(UIE)\t: No Support\n"
		     "period_IRQ(PIE)\t: %s\n"
		     "update_rate\t: %d HZ\n",
		     YN(0),
		     YN(1),
		     ED(MTK_Reg32(MT3351_RTC_AL_CTL) &
			MT3351_Alarm_EN_MASK),
		     ED(MTK_Reg32(MT3351_RTC_TIMER_CTL) &
			MT3351_INTEN_MASK),
		    MT3351_rtc_pIRQf);

	return  p - buf;
#endif	
#undef YN
#undef ED
}

#if 0
static int mt3351_rtc_read_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
        int len = mt3351_rtc_proc_output(page);
        *eof = (len <= count);
        *start = page + off;
        len -= off;
        if (len>count) len = count;
        if (len<0) len = 0;
        return len;
}
#endif        

static const struct rtc_class_ops mt3351_rtcops = {
	open:		mt3351_rtc_open,
	release:	mt3351_rtc_release,
	ioctl:		mt3351_rtc_ioctl,
	read_time:	get_rtc_time,
	set_time:	set_rtc_time,
	read_alarm:	get_rtc_wkalm_time,
	set_alarm:	set_rtc_wkalm_time,
	proc:	    mt3351_rtc_proc_output,
};

static int mt3351_rtc_fasync (int fd, struct file *filp, int on)
{
    return fasync_helper (fd, filp, on, &rtc_async_queue);
}

static unsigned int mt3351_rtc_poll(struct file *file, poll_table *wait)
{
    unsigned long l;

	poll_wait(file, &rtc_wait, wait);

	spin_lock_irq (&rtc_lock);
	l = rtc_irq_data;
	spin_unlock_irq (&rtc_lock);

	if (l != 0)
		return POLLIN | POLLRDNORM;
	return 0;
}

//static irqreturn_t mt3351_rtc_interrupt(int irq, void *dev_id, struct pt_regs *regs)
static irqreturn_t mt3351_rtc_interrupt(int irq, void *dev_id)
{
   /*
	 *	Either an alarm interrupt or update complete interrupt.
	 *	We store the status in the low byte and the number ofs
	 *	interrupts received since the last read in the remainder
	 *	of rtc_irq_data.
	 */
    unsigned int tmp,status;
    spin_lock (&rtc_lock);
	rtc_irq_data += 0x100;
	rtc_irq_data &= ~0xff;
	
	while(mt3351_rtc_IsInhibit_WR());
    tmp = MTK_Reg32(MT3351_RTC_IRQ_STA);
    
	if(tmp& MT3351_RTC_STA_ALARM_MASK)
	{
	    rtc_irq_data |= (RTC_AF|RTC_IRQF);
	    tmp &=~(MT3351_RTC_STA_ALARM_MASK);
	}
	if(tmp& MT3351_RTC_STA_TIMER_MASK)
	{
	    rtc_irq_data |= (RTC_PF|RTC_IRQF);
	    /*Periodical interrupt : need to reset timer's counter
          MT3351_RTC_TIMER_CTL.MT3351_INTEN and MT3351_EXTEN
          will be clear when timer count down to zero.
	    */
	  
	    status = MTK_Reg32(MT3351_RTC_TIMER_CTL);
	    #if 0
	    status = status_o & ~(MT3351_INTEN_MASK|MT3351_EXTEN_MASK);
	    MTK_WriteReg32(MT3351_RTC_TIMER_CTL,status);// status
	    #endif
	    
	    MTK_WriteReg32(MT3351_RTC_TIMER_CNTH,0);
	    MTK_WriteReg32(MT3351_RTC_TIMER_CNTL, (32/MT3351_rtc_pIRQf - TIMER_SETCNT_TIME));
	    MTK_WriteReg32(MT3351_RTC_TIMER_CTL,(status|MT3351_INTEN_ON)); //status_o
	    tmp &=~(MT3351_RTC_STA_TIMER_MASK);
	}
	MTK_WriteReg32(MT3351_RTC_IRQ_STA,tmp);
	spin_unlock (&rtc_lock);

	/* Now do the rest of the actions */
	wake_up_interruptible(&rtc_wait);	

	kill_fasync (&rtc_async_queue, SIGIO, POLL_IN);
	return IRQ_HANDLED;
}

static struct platform_device mt3351_rtcdev = {
		.name				= RTC_DEVICE,
		.id					= 0,
		.dev				= {
		}
};


static struct miscdevice rtc_dev=
{
	RTC_MINOR,
	"rtc0",
	&mt3351_rtcops,	
};

static int mt3351_rtc_enable(void)
{
	int ret;

    unsigned int tmp1;

	spin_lock_irq (&rtc_lock);    

	printk(KERN_INFO "%s: sw rtc key = %d\n", RTC_DEVICE,MTK_Reg32(MT3351_RTC_KEY));
	
    //RTC power is not consistent
    if( MTK_Reg32(MT3351_RTC_KEY)!=MT3351_RTC_KEY_VALUE ) 
    {
    	printk(KERN_INFO "%s: is power inconsistent.Need to reset RTC_TC registers.\n", RTC_DEVICE);
        mt3351_rtc_power_inconsistent_init();  
    }

    // Enable ripple counter
    if( MTK_Reg32(MT3351_RTC_CTL)& MT3351_RC_STOP_MASK )
    {
    	printk(KERN_INFO "%s: Enabling RTC.\n", RTC_DEVICE);
        MTK_WriteReg32(MT3351_RTC_CTL,0x90);// Enable ripple counter. RTC can be updated.
   
    }

    printk(KERN_INFO "%s: sw rtc key = %d\n", RTC_DEVICE,MTK_Reg32(MT3351_RTC_KEY));
    
    spin_unlock_irq (&rtc_lock);
  
    // request irq for RTC including timer and alarm	
    
	ret = request_irq(MT3351_RTC_IRQ_CODE, mt3351_rtc_interrupt, 0, rtc_dev.name, NULL);
	
	if(ret != 0)
	{	printk(KERN_ERR "%s: RTC interrupt IRQ is not free.\n",rtc_dev.name);
		return ret;
	}
	else
	{    printk(KERN_INFO "%s: Register IRQ Line %d.\n", rtc_dev.name,MT3351_RTC_IRQ_CODE);
	}
  
    /* reset RTC_IRQ_STA
     * Fix me !! is it good to implement polling here?
     */   
    while(mt3351_rtc_IsInhibit_WR()); 

    // clear timer and alarm hit
    spin_lock_irq (&rtc_lock);
    tmp1 = MTK_Reg32(MT3351_RTC_IRQ_STA);
    tmp1 &= 0xfffc; 
    MTK_WriteReg32(MT3351_RTC_IRQ_STA,tmp1);

	set_rtc = mt3351_set_rtc;

    spin_unlock_irq (&rtc_lock);

    mt3351_rtc_settimeofday();

	printk(KERN_INFO "MT3351 Real Time Clock Driver %s Is Initialized\n",RTC_DEVICE);	


	return 0;

}


static int mt3351_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc = NULL;
	int ret;

	mt3351_rtc_enable();

	platform_set_drvdata(pdev, rtc);

	rtc = rtc_device_register("mt3351-rtc", &pdev->dev, &mt3351_rtcops, THIS_MODULE);

	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		return 0;
	}
	else
	{	printk(KERN_INFO "%s attach device success\n",RTC_DEVICE);
	}

	rtc->max_user_freq = 128;
	
	return 0;
}


static int mt3351_rtc_remove(struct platform_device *dev)
{
	struct rtc_device *rtc = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);
	rtc_device_unregister(rtc);

	return 0;
}

static int mt3351_rtc_suspend(struct platform_device *pdev, pm_message_t stat)
{
    printk("rtc Suspend !\n");
    return 0;
}

static int mt3351_rtc_resume(struct platform_device *pdev)
{
    printk("rtc Resume !\n");
    return 0;
}

static struct platform_driver mt3351_rtcdrv = {
	.probe		= mt3351_rtc_probe,
	.remove		= mt3351_rtc_remove,
	//.shutdown	= mt3351_rtc_shutdown,
	.suspend	= mt3351_rtc_suspend,
	.resume		= mt3351_rtc_resume,
	.driver		= {
		.name	= RTC_DEVICE,
		.owner	= THIS_MODULE,
	},
};

static int __init mt3351_rtc_init(void)
{
	int ret;

	printk("MediaTek MT3351 RTC driver, version %s\n", MT3351_RTC_VERSION);

    ret = platform_device_register(&mt3351_rtcdev);

    if(ret) 
    {
        printk("Failed to register rtc device\n");
        return ret;
    }   
	
	ret = platform_driver_register(&mt3351_rtcdrv);

    if(ret) 
    {
		printk("Unable to register rtc driver (%d)\n", ret);
		return ret;
    }	
    
	return 0;
}

static void __exit mt3351_rtc_exit(void)
{
	platform_driver_unregister(&mt3351_rtcdrv);
}

module_init(mt3351_rtc_init);
module_exit(mt3351_rtc_exit);

MODULE_AUTHOR("Shu-Hsin Chang <Shu-Hsin.Chang@mediatek.com>");
MODULE_DESCRIPTION("MT3351 Real Time Clock Driver (RTC)");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(get_rtc_time);

