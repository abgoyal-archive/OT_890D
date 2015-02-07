
#include <linux/init.h>        /* For init/exit macros */
#include <linux/module.h>      /* For MODULE_ marcros  */

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/platform_device.h>

#include <asm/uaccess.h>
#include <mach/irqs.h>
#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_typedefs.h>  // Jamse add It will be removed from server.
#include <mach/mt3351_wdt.h>

#define DRV_WriteReg_16(reg, val) (*(volatile unsigned short * const)(reg)) = (val)
#define DRV_WriteReg_32(reg, val) (*(volatile unsigned int * const)(reg)) = (val)
#define DRV_Reg_16(reg) (*(volatile unsigned short * const)(reg))
#define DRV_Reg_32(reg) (*(volatile unsigned int * const)(reg))

#define NO_DEBUG 1

extern void MT3351_IRQSensitivity(unsigned char code, unsigned char edge);
extern void MT3351_IRQMask(unsigned int line);
extern void MT3351_IRQUnmask(unsigned int line);
extern void MT3351_IRQClearInt(unsigned int line);

static char expect_close;
static spinlock_t mt3351_wdt_spinlock = SPIN_LOCK_UNLOCKED;
static unsigned short timeout;
//kal_bool  mt3351_wdt_RST;
//kal_bool  mt3351_wdt_SW_RST;
volatile kal_bool  mt3351_wdt_INTR;

static int nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default=CONFIG_WATCHDOG_NOWAYOUT)");


void mt3351_wdt_SetTimeOutValue(unsigned short value)
{
	/*
	 * TimeOut = BitField 15:5
	 * Key	   = BitField  4:0 = 0x08
	 */
	
	spin_lock(&mt3351_wdt_spinlock);
	
	// sec * 32768 / 512 = sec * 64 = sec * 1 << 6
	timeout = (unsigned short)(value * ( 1 << 6) );

	timeout = timeout << 5; 

	DRV_WriteReg_16(MT3351_WDT_LENGTH, (timeout | MT3351_WDT_LENGTH_KEY) );
	
	spin_unlock(&mt3351_wdt_spinlock);
}
EXPORT_SYMBOL(mt3351_wdt_SetTimeOutValue);

void mt3351_wdt_SetResetLength(unsigned short value)
{
	/* 
	 * MT3351_RSTL_MASK	= 0xFFF for BitField 11:0
	 */
	
	spin_lock(&mt3351_wdt_spinlock);
	
	DRV_WriteReg_16(MT3351_WDT_RSTL, (value & MT3351_RSTL_MASK) );

	spin_unlock(&mt3351_wdt_spinlock);
}
EXPORT_SYMBOL(mt3351_wdt_SetResetLength);

void mt3351_wdt_ModeSelection(kal_bool en, kal_bool auto_rstart, kal_bool pwr_key_dis, kal_bool IRQ )
{
	unsigned short tmp;

    spin_lock(&mt3351_wdt_spinlock);
    
	tmp = DRV_Reg_16(MT3351_WDT_MODE);
	tmp |= MT3351_WDT_MODE_KEY;
	
	// Bit 0 : Whether enable watchdog or not
	if(en == KAL_TRUE)
		tmp |= MT3351_WDT_MODE_ENABLE;
	else
		tmp &= ~MT3351_WDT_MODE_ENABLE;
	
	// Bit 4 : Whether enable auto-restart or not for counting value
	if(auto_rstart == KAL_TRUE)
		tmp |= MT3351_WDT_MODE_AUTORST;
	else
		tmp &= ~MT3351_WDT_MODE_AUTORST;
	
	//  Bit 2 : Whether disable power key or not
	if(pwr_key_dis == KAL_TRUE)
		tmp |= MT3351_WDT_PWR_KEY_DIS;
	else
		tmp &= ~MT3351_WDT_PWR_KEY_DIS;

	// Bit 3 : TRUE for generating Interrupt (False for generating Reset) when WDT timer reaches zero
	if(IRQ == KAL_TRUE)
		tmp |= MT3351_WDT_RESET_IRQ;
	else
		tmp &= ~MT3351_WDT_RESET_IRQ;

	DRV_WriteReg_16(MT3351_WDT_MODE,tmp);

	spin_unlock(&mt3351_wdt_spinlock);
}
EXPORT_SYMBOL(mt3351_wdt_ModeSelection);

void mt3351_wdt_Restart(void)
{
	// Reset WatchDogTimer's counting value to default value
	// ie., keepalive()
	
	spin_lock(&mt3351_wdt_spinlock);
	
	DRV_WriteReg_16(MT3351_WDT_RESTART, MT3351_WDT_RESTART_KEY);

	spin_unlock(&mt3351_wdt_spinlock);
}
EXPORT_SYMBOL(mt3351_wdt_Restart);

void mt3351_wdt_SWTrigger(void)
{
	// SW trigger WDT reset or IRQ
	
	spin_lock(&mt3351_wdt_spinlock);
	
	DRV_WriteReg_16(MT3351_WDT_STRG, MT3351_WDT_STRG_KEY);

	spin_unlock(&mt3351_wdt_spinlock);
}
EXPORT_SYMBOL(mt3351_wdt_SWTrigger);

unsigned char mt3351_wdt_CheckStatus(void)
{
	unsigned char status;

	spin_lock(&mt3351_wdt_spinlock);
	
	status = DRV_Reg_16(MT3351_WDT_STATUS);

	spin_unlock(&mt3351_wdt_spinlock);
	
	return status;
}
EXPORT_SYMBOL(mt3351_wdt_CheckStatus);

kal_bool mt3351_wdt_SW_MCUPeripheralReset(MT3351_MCU_PERIPH MCU_peri)
{

	if ((MCU_peri > MT3351_MCU_PERI_IRDA) || (MCU_peri < MT3351_MCU_PERI_USB))
		return KAL_FALSE;
	
	// set key and peripheral
	
	spin_lock(&mt3351_wdt_spinlock);
	
	DRV_WriteReg_32(MT3351_MCU_PERIPH_RESET, (MT3351_MCU_PERIPH_RESET_KEY| (1<<MCU_peri) ) );
	
	spin_unlock(&mt3351_wdt_spinlock);
	
	// clear reset

	spin_lock(&mt3351_wdt_spinlock);
	
	DRV_WriteReg_32(MT3351_MCU_PERIPH_RESET, MT3351_MCU_PERIPH_RESET_KEY);
	
	spin_unlock(&mt3351_wdt_spinlock);
	
	return KAL_TRUE;
}
EXPORT_SYMBOL(mt3351_wdt_SW_MCUPeripheralReset);

kal_bool mt3351_wdt_SW_MMPeripheralReset(MT3351_MM_PERIPH MM_peri)
{

	if ((MM_peri > MT3351_MM_PERI_LCD) || (MM_peri < MT3351_MM_PERI_GBLRST))
		return KAL_FALSE;
	
	// set key and peripheral
	
	spin_lock(&mt3351_wdt_spinlock);
	
	DRV_WriteReg_32(MT3351_MM_PERIPH_RESET, (MT3351_MM_PERIPH_RESET_KEY| (1<<MM_peri) ) );

	spin_unlock(&mt3351_wdt_spinlock);
	
	// clear reset
	
	spin_lock(&mt3351_wdt_spinlock);
	
	DRV_WriteReg_32(MT3351_MM_PERIPH_RESET, MT3351_MM_PERIPH_RESET_KEY);
	
	spin_unlock(&mt3351_wdt_spinlock);
	
	return KAL_TRUE;
}
EXPORT_SYMBOL(mt3351_wdt_SW_MMPeripheralReset);


void WDT_HW_Reset_test(void)
{
	printk(KERN_ALERT "\nWDT_HW_Reset_test : System will reset after 3 secs\n");
	
	// 1. set WDT timeout 10 secs, 641*512/32768 = 10sec
	mt3351_wdt_SetTimeOutValue(5);
	
	// 2. enable WDT reset, auto-restart enable ,disable PWR key, disable intr
	mt3351_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_TRUE, KAL_FALSE);
	
	// 3. reset the watch dog timer to the value set in WDT_LENGTH register
	mt3351_wdt_Restart();
	
	// 4. system will reset
}

void WDT_SW_Reset_test(void)
{
	printk(KERN_ALERT "\nWDT_SW_Reset_test : System will reset Immediately\n");
	
	// 1. disable WDT reset, auto-restart enable ,disable PWR key, disable intr
    //mt3351_wdt_ModeSelection(KAL_FALSE, KAL_TRUE, KAL_TRUE, KAL_FALSE);

	// 1. disable WDT reset, auto-restart disable ,disable PWR key, disable intr
	mt3351_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_TRUE, KAL_FALSE);
	
	// 2. prepare for SW reset
	mt3351_wdt_SetTimeOutValue(1);
	mt3351_wdt_Restart();
	
	// 3. system will reset by SW now    
	mt3351_wdt_SWTrigger();
}

void WDT_count_test(void)
{
	/*
	 * Try DVT testsuite : WDT_count_test
	 */
#if 1
	printk(KERN_ALERT "\nWDT_count_test Start..........\n");

	// 1. set WDT timeout 10 secs
	mt3351_wdt_SetTimeOutValue(10);
	
	// 2. enable WDT reset, auto-restart disable ,disable PWR key, enable intr
	mt3351_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE, KAL_TRUE);

	// 3. reset the watch dog timer to the value set in WDT_LENGTH register
	mt3351_wdt_Restart();

	printk(KERN_ALERT "\n1/2 : Waiting 10 sec. for WDT with WDT_status.\n");
	
	// 4. wait WDT status
	while(DRV_Reg_16(MT3351_WDT_STATUS) != MT3351_WDT_STATUS_HWWDT);
	
	printk(KERN_ALERT "\nWDT_count_test done by WDT_STATUS!!\n");

	mt3351_wdt_INTR = 0; // set to zero before WDT counting down
	
	// 5.reset WDT timer
	mt3351_wdt_Restart();
	
	printk(KERN_ALERT "\n2/2 : Waiting 10 sec. for WDT with IRQ 19.\n");
	
	while(mt3351_wdt_INTR == 0);

	printk(KERN_ALERT "\nWDT_count_test done by IRQ 19!!\n");
	
	printk(KERN_ALERT "\nWDT_count_test Finish !!\n");
#endif	
	
}


static irqreturn_t mt3351_wdt_ISR(int irq, void *dev_id)
{
	MT3351_IRQMask(MT3351_WDT_IRQ_CODE);

	mt3351_wdt_INTR = 1;

	printk(KERN_ALERT "\nIn mt3351_wdt_ISR(), mt3351_wdt_INTR set to 1\n");
	
	MT3351_IRQClearInt(MT3351_IRQ_WDT_CODE);

	MT3351_IRQUnmask(MT3351_IRQ_WDT_CODE);
	
	return IRQ_HANDLED;
}


static int mt3351_wdt_open(struct inode *inode, struct file *file)
{
	//printk(KERN_ALERT "\n******** MT3351 WDT driver open!! ********\n" );

#if 0
	printk(KERN_ALERT "\n[mt3351_wdt_open] Run WDT_HW_Reset_test function\n" );
	printk(KERN_ALERT "\nSystem will reset after 5 secs\n" );
	WDT_HW_Reset_test();
#endif
	
#if 0
	printk(KERN_ALERT "\n[mt3351_wdt_open] Run WDT_SW_Reset_test function\n" );
	printk(KERN_ALERT "\nSystem will reset Immediately\n" );
	WDT_SW_Reset_test();
#endif

#if NO_DEBUG
	printk(KERN_ALERT "\n[mt3351_wdt_open] start the HW WDT, Normal mode.\n" );
	/*
	 * start the HW WDT
	 * enable WDT reset, auto-restart enable ,disable PWR key, disable intr
	 */
	mt3351_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_TRUE, KAL_FALSE);

	/*
	 * default : user can not stop WDT
	 *
	 * If the userspace daemon closes the file without sending
	 * this special character "V", the driver will assume that the daemon (and
	 * userspace in general) died, and will stop pinging the watchdog without
	 * disabling it first.  This will then cause a reboot.
	 */
	expect_close = 0;
#endif 
	
	//return 0;
	return nonseekable_open(inode, file);
}

static int mt3351_wdt_release(struct inode *inode, struct file *file)
{
	printk(KERN_ALERT "\n******** MT3351 WDT driver release!! ********\n");

	if( expect_close == 42 )
	{
#if NO_DEBUG		
		printk(KERN_ALERT "\nYou can stop watchdog timer!!\n");
		
		//To stop HW WDT
		//disable WDT reset, auto-restart disable ,enable PWR key, disable intr
		mt3351_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE, KAL_FALSE);
#endif
		
#if 0
		printk(KERN_ALERT "\nDo not stop WDT for debuging!!\n");
#endif
	}
	else
	{
#if NO_DEBUG
		printk(KERN_ALERT "\nUnexpected close, not stopping watchdog!\n");

		//To keepalive HW WDT, and then system will reset.
		mt3351_wdt_Restart();
#endif
	}

	expect_close = 0;
	
	return 0;
}

static ssize_t mt3351_wdt_write(struct file *file, const char __user *data,
								size_t len, loff_t *ppos)
{
	//printk(KERN_ALERT "\n******** MT3351 WDT driver : write ********\n" );	

	if(len) 
	{
		if(!nowayout) 
		{			
			size_t i;
			expect_close = 0;
			for( i = 0 ; i != len ; i++ ) 
			{
				char c;
				if( get_user(c, data + i) )
					return -EFAULT;
				if( c == 'V' )
				{
					expect_close = 42;
					printk(KERN_ALERT "\nnowayout=N and write=V, you can disable HW WDT\n");
				}
			}
		}
		
	    //To keepalive HW WDT, ie., mt3351_wdt_Restart()
        //printk(KERN_ALERT "\nTo keepalive HW WDT, ie., mt3351_wdt_Restart()\n");	
        mt3351_wdt_Restart();
	}
	return len;
}

#define OPTIONS WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE

static struct watchdog_info mt3351_wdt_ident = {
	.options          = OPTIONS,
	.firmware_version =	0,
	.identity         =	"MT3351 Watchdog",
};

static int mt3351_wdt_ioctl(struct inode *inode, struct file *file,
							unsigned int cmd, unsigned long arg)
{
	//printk(KERN_ALERT "\n******** MT3351 WDT driver ioctl something!! ********\n" );

	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int new_TimeOutValue;

	switch (cmd) {
		default:
			return -ENOIOCTLCMD;

		case WDIOC_GETSUPPORT:
			return copy_to_user(argp, &mt3351_wdt_ident, sizeof(mt3351_wdt_ident)) ? -EFAULT : 0;

		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			return put_user(0, p);

		case WDIOC_KEEPALIVE:
			mt3351_wdt_Restart();
			return 0;

		case WDIOC_SETTIMEOUT:
			if( get_user(new_TimeOutValue, p) )
				return -EFAULT;
			
			//printk(KERN_ALERT "\nWDT driver will set %d sec. to HW WDT.\n", new_TimeOutValue);
			
#if 1
			/*
			 * set timeout and keep HW WDT alive
			 */
			mt3351_wdt_SetTimeOutValue(new_TimeOutValue);
			mt3351_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_TRUE, KAL_FALSE);
			mt3351_wdt_Restart();
#endif

#if 0
			/*
			 * WDT_count_test()
			 */
			mt3351_wdt_SetTimeOutValue(new_TimeOutValue);
			
			printk(KERN_ALERT "\nWDT_count_test Start !!\n");

			mt3351_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE, KAL_TRUE);
			
			mt3351_wdt_Restart();

			printk(KERN_ALERT "\n1/2 : Waiting %d sec. for WDT with WDT_status.\n", timeout >> 11);

			while(DRV_Reg_16(MT3351_WDT_STATUS) != MT3351_WDT_STATUS_HWWDT);

			printk(KERN_ALERT "\nWDT_count_test done by WDT_STATUS!!\n");

			mt3351_wdt_INTR = 0; 

			mt3351_wdt_Restart();

			printk(KERN_ALERT "\n2/2 : Waiting %d sec. for WDT with IRQ 19.\n", timeout >> 11);

			while(mt3351_wdt_INTR == 0);

			printk(KERN_ALERT "\nWDT_count_test done by IRQ 19!!\n");

			printk(KERN_ALERT "\nWDT_count_test Finish !!\n");
#endif			
			
			return put_user(timeout >> 11, p);

		case WDIOC_GETTIMEOUT:
			return put_user(timeout >> 11, p);
	}

	return 0;
}

static struct file_operations mt3351_wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.open		= mt3351_wdt_open,
	.release	= mt3351_wdt_release,
	.write		= mt3351_wdt_write,
	.ioctl		= mt3351_wdt_ioctl,
};

static struct miscdevice mt3351_wdt_miscdev = {
	.minor		= 130,
	.name		= "mt3351-wdt",
	.fops		= &mt3351_wdt_fops,
};


static int mt3351_wdt_probe(struct device *dev)
{
	int ret;
	
	printk(KERN_ALERT "******** MT3351 WDT driver probe!! ********\n" );
	
#if 1	
	/*  
	 * Register IRQ line (19) and ISR
	 */
	
	MT3351_IRQSensitivity(MT3351_WDT_IRQ_CODE, MT3351_EDGE_SENSITIVE);
	//printk(KERN_ALERT "mt3351_wdt_probe : IRQSensitivity = MT3351_EDGE_SENSITIVE\n");
	
	ret = request_irq(MT3351_WDT_IRQ_CODE, mt3351_wdt_ISR, 0, "mt3351_watchdog", NULL);
	if(ret != 0)
	{
		printk(KERN_ALERT "mt3351_wdt_probe : Failed to request irq (%d)\n", ret);
		return ret;
	}
	//printk(KERN_ALERT "mt3351_wdt_probe : Success to request irq\n");
#endif
	
#if 1	
	ret = misc_register(&mt3351_wdt_miscdev);
	if(ret)
	{
		printk(KERN_ALERT "Cannot register miscdev on minor=130\n");
		return ret;
	}
	//printk(KERN_ALERT "mt3351_wdt_probe : Success to register miscdev\n");
#endif
	
	return 0;
}

static int mt3351_wdt_remove(struct device *dev)
{
	printk(KERN_ALERT "\n******** MT3351 WDT driver remove!! ********\n" );

#if 1
	free_irq(MT3351_WDT_IRQ_CODE, NULL);
	misc_deregister(&mt3351_wdt_miscdev);
#endif	
	
	return 0;
}

static void mt3351_wdt_shutdown(struct device *dev)
{
	printk(KERN_ALERT "\n******** MT3351 WDT driver shutdown!! ********\n" );

#if 0
	//To stop HW WDT or keepalive

	//disable WDT reset, auto-restart disable ,enable PWR key, disable intr
	mt3351_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE, KAL_FALSE);

	mt3351_wdt_Restart();
#endif
}

static int mt3351_wdt_suspend(struct device *dev, pm_message_t state)
{
	printk(KERN_ALERT "\n******** MT3351 WDT driver suspend!! ********\n" );
	
#if 0
	//To stop HW WDT or keepalive

	//disable WDT reset, auto-restart disable ,enable PWR key, disable intr
	mt3351_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE, KAL_FALSE);

	mt3351_wdt_Restart();
#endif
	return 0;
}

static int mt3351_wdt_resume(struct device *dev)
{
	printk(KERN_ALERT "\n******** MT3351 WDT driver resume!! ********\n" );
	
#if 0
	//To stop HW WDT or keepalive

	//disable WDT reset, auto-restart disable ,enable PWR key, disable intr
	mt3351_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE, KAL_FALSE);

	mt3351_wdt_Restart();
#endif
	return 0;
}

static struct device_driver mt3351_wdt_driver = {
	.name		= "mt3351-wdt",
	.bus		= &platform_bus_type,
	.probe		= mt3351_wdt_probe,
	.remove		= mt3351_wdt_remove,
	.shutdown	= mt3351_wdt_shutdown,
#ifdef CONFIG_PM
	.suspend	= mt3351_wdt_suspend,
	.resume		= mt3351_wdt_resume,
#endif
};

struct platform_device MT3351_device_wdt = {
		.name				= "mt3351-wdt",
		.id					= 0,
		.dev				= {
		}
};


static int __init mt3351_wdt_init(void)
{
	int ret;
	
	//printk(KERN_ALERT "\n******** Hello MT3351 WDT driver!! ********\n" );

	//mt3351_wdt_RST = 0;
    //mt3351_wdt_SW_RST = 0;
	//mt3351_wdt_INTR = 0;		  
	
	/*
	 * Judge if WDT timeout happened [TODO]
	 */
	
	
	ret = platform_device_register(&MT3351_device_wdt);
	if (ret) {
		printk("****[mt3351_wdt_driver] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = driver_register(&mt3351_wdt_driver);
	if (ret) {
		printk("****[mt3351_wdt_driver] Unable to register driver (%d)\n", ret);
		return ret;
	}
	return 0;
	
}

static void __exit mt3351_wdt_exit (void)
{
	printk(KERN_ALERT "\n******** Goodbye MT3351 WDT driver!! ********\n");
}

module_init(mt3351_wdt_init);
module_exit(mt3351_wdt_exit);

MODULE_AUTHOR("James Lo");
MODULE_DESCRIPTION("MT3351 Watchdog Device Driver");
MODULE_LICENSE("GPL");
