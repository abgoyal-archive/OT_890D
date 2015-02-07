
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
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_typedefs.h>  
#include <mach/mt6516_wdt.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/tcm.h>

#define DRV_WriteReg_16(reg, val) (*(volatile unsigned short * const)(reg)) = (val)
#define DRV_WriteReg_32(reg, val) (*(volatile unsigned int * const)(reg)) = (val)
#define DRV_Reg_16(reg) (*(volatile unsigned short * const)(reg))
#define DRV_Reg_32(reg) (*(volatile unsigned int * const)(reg))

//#define DEBUG_MSG
#undef DEBUG_MSG

#define NO_DEBUG 1


extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);
extern void MT6516_IRQMask(unsigned int line);
extern void MT6516_IRQUnmask(unsigned int line);
extern void MT6516_IRQClearInt(unsigned int line);

#define WDT_DEVNAME "watchdog"

static struct class *wdt_class = NULL;
static int wdt_major = 0;
static dev_t wdt_devno;
static struct cdev *wdt_cdev;

static char expect_close; // Not use
static spinlock_t mt6516_wdt_spinlock = SPIN_LOCK_UNLOCKED;
static unsigned short timeout;
volatile kal_bool  mt6516_wdt_INTR;
static int g_lastTime_TimeOutValue = 0;
static int g_WDT_enable = 0;

enum {
	WDT_NORMAL_MODE,
	WDT_EXP_MODE
} g_wdt_mode = WDT_NORMAL_MODE;

#ifdef CONFIG_WD_KICKER

#include <../../../drivers/watchdog/wdk/wd_kicker.h>


static int mt6516_wk_wdt_config(enum wk_wdt_mode mode, int timeout)
{

	//disable WDT reset, auto-restart disable , disable intr
	mt6516_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
	mt6516_wdt_Restart();



	if (mode == WK_WDT_EXP_MODE) {
		g_wdt_mode = WDT_EXP_MODE;
		mt6516_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE);	        
	}
	else {
		g_wdt_mode = WDT_NORMAL_MODE;
		mt6516_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);
	}

	g_lastTime_TimeOutValue = timeout;
	mt6516_wdt_SetTimeOutValue(timeout);
	g_WDT_enable = 1;
	mt6516_wdt_Restart();

	return 0;
}

static struct wk_wdt mt6516_wk_wdt = {
	.config 	= mt6516_wk_wdt_config,
	.kick_wdt 	= mt6516_wdt_Restart
};
#endif

static int nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, int, 0);
//MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default=CONFIG_WATCHDOG_NOWAYOUT)");


void mt6516_wdt_SetTimeOutValue(unsigned short value)
{
	/*
	 * TimeOut = BitField 15:5
	 * Key	   = BitField  4:0 = 0x08
	 */
	
	spin_lock(&mt6516_wdt_spinlock);
	
	// sec * 32768 / 512 = sec * 64 = sec * 1 << 6
	timeout = (unsigned short)(value * ( 1 << 6) );

	timeout = timeout << 5; 

	DRV_WriteReg_16(MT6516_WDT_LENGTH, (timeout | MT6516_WDT_LENGTH_KEY) );
	
	spin_unlock(&mt6516_wdt_spinlock);
}
EXPORT_SYMBOL(mt6516_wdt_SetTimeOutValue);

void mt6516_wdt_SetResetLength(unsigned short value)
{
	/* 
	 * MT6516_RSTL_MASK	= 0xFFF for BitField 11:0
	 */
	
	spin_lock(&mt6516_wdt_spinlock);
	
	DRV_WriteReg_16(MT6516_WDT_RSTL, (value & MT6516_RSTL_MASK) );

	spin_unlock(&mt6516_wdt_spinlock);
}
EXPORT_SYMBOL(mt6516_wdt_SetResetLength);

void mt6516_wdt_ModeSelection(kal_bool en, kal_bool auto_rstart, kal_bool IRQ )
{
	unsigned short tmp;

    spin_lock(&mt6516_wdt_spinlock);
    
	tmp = DRV_Reg_16(MT6516_WDT_MODE);
	tmp |= MT6516_WDT_MODE_KEY;
	
	// Bit 0 : Whether enable watchdog or not
	if(en == KAL_TRUE)
		tmp |= MT6516_WDT_MODE_ENABLE;
	else
		tmp &= ~MT6516_WDT_MODE_ENABLE;
	
	// Bit 4 : Whether enable auto-restart or not for counting value
	if(auto_rstart == KAL_TRUE)
		tmp |= MT6516_WDT_MODE_AUTORST;
	else
		tmp &= ~MT6516_WDT_MODE_AUTORST;

	// Bit 3 : TRUE for generating Interrupt (False for generating Reset) when WDT timer reaches zero
	if(IRQ == KAL_TRUE)
		tmp |= MT6516_WDT_RESET_IRQ;
	else
		tmp &= ~MT6516_WDT_RESET_IRQ;

	DRV_WriteReg_16(MT6516_WDT_MODE,tmp);

	spin_unlock(&mt6516_wdt_spinlock);
}
EXPORT_SYMBOL(mt6516_wdt_ModeSelection);

void mt6516_wdt_Restart(void)
{
	// Reset WatchDogTimer's counting value to default value
	// ie., keepalive()
	
	spin_lock(&mt6516_wdt_spinlock);
	
	DRV_WriteReg_16(MT6516_WDT_RESTART, MT6516_WDT_RESTART_KEY);

	spin_unlock(&mt6516_wdt_spinlock);
}
EXPORT_SYMBOL(mt6516_wdt_Restart);

void mt6516_wdt_SWTrigger(void)
{
	// SW trigger WDT reset or IRQ
	
	spin_lock(&mt6516_wdt_spinlock);
	
	DRV_WriteReg_16(MT6516_WDT_STRG, MT6516_WDT_STRG_KEY);

	spin_unlock(&mt6516_wdt_spinlock);
}
EXPORT_SYMBOL(mt6516_wdt_SWTrigger);

unsigned char mt6516_wdt_CheckStatus(void)
{
	unsigned char status;

	spin_lock(&mt6516_wdt_spinlock);
	
	status = DRV_Reg_16(MT6516_WDT_STATUS);

	spin_unlock(&mt6516_wdt_spinlock);
	
	return status;
}
EXPORT_SYMBOL(mt6516_wdt_CheckStatus);

#if 0
/* DUMA not use */
kal_bool mt6516_wdt_SW_MCUPeripheralReset(MT6516_MCU_PERIPH MCU_peri)
{

	if ((MCU_peri > MT6516_MCU_PERI_IRDA) || (MCU_peri < MT6516_MCU_PERI_USB))
		return KAL_FALSE;
	
	// set key and peripheral
	
	spin_lock(&mt6516_wdt_spinlock);
	
	DRV_WriteReg_32(MT6516_MCU_PERIPH_RESET, (MT6516_MCU_PERIPH_RESET_KEY| (1<<MCU_peri) ) );
	
	spin_unlock(&mt6516_wdt_spinlock);
	
	// clear reset

	spin_lock(&mt6516_wdt_spinlock);
	
	DRV_WriteReg_32(MT6516_MCU_PERIPH_RESET, MT6516_MCU_PERIPH_RESET_KEY);
	
	spin_unlock(&mt6516_wdt_spinlock);
	
	return KAL_TRUE;
}
EXPORT_SYMBOL(mt6516_wdt_SW_MCUPeripheralReset);
#endif

#if 0
/* DUMA not use */
kal_bool mt6516_wdt_SW_MMPeripheralReset(MT6516_MM_PERIPH MM_peri)
{

	if ((MM_peri > MT6516_MM_PERI_LCD) || (MM_peri < MT6516_MM_PERI_GBLRST))
		return KAL_FALSE;
	
	// set key and peripheral
	
	spin_lock(&mt6516_wdt_spinlock);
	
	DRV_WriteReg_32(MT6516_MM_PERIPH_RESET, (MT6516_MM_PERIPH_RESET_KEY| (1<<MM_peri) ) );

	spin_unlock(&mt6516_wdt_spinlock);
	
	// clear reset
	
	spin_lock(&mt6516_wdt_spinlock);
	
	DRV_WriteReg_32(MT6516_MM_PERIPH_RESET, MT6516_MM_PERIPH_RESET_KEY);
	
	spin_unlock(&mt6516_wdt_spinlock);
	
	return KAL_TRUE;
}
EXPORT_SYMBOL(mt6516_wdt_SW_MMPeripheralReset);
#endif


void WDT_HW_Reset_test(void)
{
	printk("WDT_HW_Reset_test : System will reset after 5 secs\n");
	
	// 1. set WDT timeout 10 secs, 641*512/32768 = 10sec
	mt6516_wdt_SetTimeOutValue(5);
	
	// 2. enable WDT reset, auto-restart enable ,disable intr
	mt6516_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);
	
	// 3. reset the watch dog timer to the value set in WDT_LENGTH register
	mt6516_wdt_Restart();
	
	// 4. system will reset
	while(1);
}
EXPORT_SYMBOL(WDT_HW_Reset_test);

void WDT_HW_Reset_kick_6times_test(void)
{
	int kick_times = 6;
	
	printk("WDT_HW_Reset_test : System will reset after 5 secs\n");
	
	// 1. set WDT timeout 10 secs, 641*512/32768 = 10sec
	mt6516_wdt_SetTimeOutValue(5);
	
	// 2. enable WDT reset, auto-restart enable ,disable intr
	mt6516_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);
	
	// 3. reset the watch dog timer to the value set in WDT_LENGTH register
	mt6516_wdt_Restart();

	// 4. kick WDT
	while(kick_times >= 0)
	{
		mdelay(3000);
		printk("WDT_HW_Reset_test : reset after %d times !\n", kick_times);
		mt6516_wdt_Restart();
		kick_times--;
	}

	// 5. system will reset
	printk("WDT_HW_Reset_test : Kick stop !!\n");
	while(1);
	
}
EXPORT_SYMBOL(WDT_HW_Reset_kick_6times_test);


void WDT_SW_Reset_test(void)
{
	printk("WDT_SW_Reset_test : System will reset Immediately\n");
		
	// 1. disable WDT reset, auto-restart disable ,disable intr
	mt6516_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
	
	// 2. prepare for SW reset
	mt6516_wdt_SetTimeOutValue(1);
	mt6516_wdt_Restart();
	
	// 3. system will reset by SW now    
	mt6516_wdt_SWTrigger();
	while(1);
}
EXPORT_SYMBOL(WDT_SW_Reset_test);

void WDT_count_test(void)
{
	/*
	 * Try DVT testsuite : WDT_count_test (Non-reset test)
	 */
	printk("WDT_count_test Start..........\n");

	// 1. set WDT timeout 10 secs
	mt6516_wdt_SetTimeOutValue(10);
	
	// 2. enable WDT reset, auto-restart disable, enable intr
	mt6516_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE);

	// 3. reset the watch dog timer to the value set in WDT_LENGTH register
	mt6516_wdt_Restart();

	printk("1/2 : Waiting 10 sec. for WDT with WDT_status.\n");
	
	// 4. wait WDT status
	while(DRV_Reg_16(MT6516_WDT_STATUS) != MT6516_WDT_STATUS_HWWDT);
	
	printk("WDT_count_test done by WDT_STATUS!!\n");

	mt6516_wdt_INTR = 0; // set to zero before WDT counting down
	
	// 5.reset WDT timer
	mt6516_wdt_Restart();
	
	printk("2/2 : Waiting 10 sec. for WDT with IRQ 15.\n");
	
	while(mt6516_wdt_INTR == 0);

	printk("WDT_count_test done by IRQ 15!!\n");
	
	printk("WDT_count_test Finish !!\n");
	
}
EXPORT_SYMBOL(WDT_count_test);

void MT6516_traceCallStack(void);
void aee_bug(const char *source, const char *msg);

static __tcmfunc irqreturn_t mt6516_wdt_ISR(int irq, void *dev_id)
{
	MT6516_IRQMask(MT6516_WDT_IRQ_LINE);

	mt6516_wdt_INTR = 1;

	//printk("In mt6516_wdt_ISR(), mt6516_wdt_INTR set to 1\n");

	MT6516_traceCallStack();
	aee_bug("WATCHDOG", "Watch Dog Timeout");
	
	MT6516_IRQClearInt(MT6516_WDT_IRQ_LINE);
	MT6516_IRQUnmask(MT6516_WDT_IRQ_LINE);
	
	return IRQ_HANDLED;
}

static int mt6516_wdt_open(struct inode *inode, struct file *file)
{
	#ifdef DEBUG_MSG
	printk(KERN_ALERT "\n******** MT6516 WDT driver open!! ********\n" );
	#endif

	#if 0
	printk(KERN_ALERT "\n[mt6516_wdt_open] Run WDT_HW_Reset_test function\n" );
	printk(KERN_ALERT "\nSystem will reset after 5 secs\n" );
	WDT_HW_Reset_test();
	#endif
	
	#if 0
	printk(KERN_ALERT "\n[mt6516_wdt_open] Run WDT_SW_Reset_test function\n" );
	printk(KERN_ALERT "\nSystem will reset Immediately\n" );
	WDT_SW_Reset_test();
	#endif

	#if NO_DEBUG
	//printk(KERN_ALERT "\n[mt6516_wdt_open] start the HW WDT, Normal mode.\n" );
	/*
	 * start the HW WDT
	 * enable WDT reset, auto-restart enable , disable intr
	 */
	//mt6516_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);

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

	return nonseekable_open(inode, file);
}

static int mt6516_wdt_release(struct inode *inode, struct file *file)
{
	#ifdef DEBUG_MSG
	printk(KERN_ALERT "\n******** MT6516 WDT driver release!! ********\n");
	#endif

	//if( expect_close == 42 )
	if( expect_close == 0 )
	{
		#if NO_DEBUG		
		//printk(KERN_ALERT "\nYou can stop watchdog timer!!\n");
		
		//To stop HW WDT
		//disable WDT reset, auto-restart disable, disable intr
		mt6516_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
		#endif
		
		#if 0
		printk(KERN_ALERT "\nDo not stop WDT for debuging!!\n");
		#endif
	}
	else
	{
		#if NO_DEBUG
		//printk(KERN_ALERT "\nUnexpected close, not stopping watchdog!\n");

		//To keepalive HW WDT, and then system will reset.
		mt6516_wdt_Restart();
		#endif
	}

	expect_close = 0;
	
	g_WDT_enable = 0;
		
	return 0;
}

static ssize_t mt6516_wdt_write(struct file *file, const char __user *data,
								size_t len, loff_t *ppos)
{
	#ifdef DEBUG_MSG	
	printk(KERN_ALERT "\n******** MT6516 WDT driver : write ********\n" );	
	#endif

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
	    
	#ifdef DEBUG_MSG
        printk(KERN_ALERT "\nTo keepalive HW WDT, ie., mt6516_wdt_Restart()\n");	
	#endif
	
		//To keepalive HW WDT, ie., mt6516_wdt_Restart()
        mt6516_wdt_Restart();
	}
	return len;
}

#define OPTIONS WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE

static struct watchdog_info mt6516_wdt_ident = {
	.options          = OPTIONS,
	.firmware_version =	0,
	.identity         =	"MT6516 Watchdog",
};

static int mt6516_wdt_ioctl(struct inode *inode, struct file *file,
							unsigned int cmd, unsigned long arg)
{
	#ifdef DEBUG_MSG
	printk("******** MT6516 WDT driver ioctl !! ********\n" );
	#endif

	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int new_TimeOutValue;

	switch (cmd) {
		default:
			return -ENOIOCTLCMD;

		case WDIOC_GETSUPPORT:
			return copy_to_user(argp, &mt6516_wdt_ident, sizeof(mt6516_wdt_ident)) ? -EFAULT : 0;

		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			return put_user(0, p);

		case WDIOC_KEEPALIVE:
			mt6516_wdt_Restart();
			return 0;

		case WDIOC_SETTIMEOUT:
			if( get_user(new_TimeOutValue, p) )
				return -EFAULT;

			#ifdef DEBUG_MSG
			printk(KERN_ALERT "\nWDT driver will set %d sec. to HW WDT.\n", new_TimeOutValue);									
			#endif
			
			//set timeout and keep HW WDT alive
			mt6516_wdt_SetTimeOutValue(new_TimeOutValue);

			g_lastTime_TimeOutValue = new_TimeOutValue;
			g_WDT_enable = 1;

			if (g_wdt_mode == WDT_EXP_MODE)
				mt6516_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE);	        
			else
				mt6516_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);

			mt6516_wdt_Restart();
			
			return put_user(timeout >> 11, p);

		case WDIOC_GETTIMEOUT:
			return put_user(timeout >> 11, p);
	}

	return 0;
}


static struct file_operations mt6516_wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.open		= mt6516_wdt_open,
	.release	= mt6516_wdt_release,
	.write		= mt6516_wdt_write,
	.ioctl		= mt6516_wdt_ioctl,
};

static struct miscdevice mt6516_wdt_miscdev = {
	.minor		= 130,
	.name		= "mt6516-wdt",
	.fops		= &mt6516_wdt_fops,
};


static int mt6516_wdt_probe(struct device *dev)
{
	int ret;
	
	printk("******** MT6516 WDT driver probe!! ********\n" );
	
	/*  
	 * Register IRQ line (15) and ISR
	 */
	
	MT6516_IRQSensitivity(MT6516_WDT_IRQ_LINE, MT6516_EDGE_SENSITIVE);
	#ifdef DEBUG_MSG
	printk("mt6516_wdt_probe : IRQSensitivity = MT6516_EDGE_SENSITIVE\n");
	#endif
		
	ret = request_irq(MT6516_WDT_IRQ_LINE, mt6516_wdt_ISR, 0, "mt6516_watchdog", NULL);
	if(ret != 0)
	{
		printk(KERN_ALERT "mt6516_wdt_probe : Failed to request irq (%d)\n", ret);
		return ret;
	}
	printk("mt6516_wdt_probe : Success to request irq\n");

	#if 0	
	ret = misc_register(&mt6516_wdt_miscdev);
	if(ret)
	{
		printk("Cannot register miscdev on minor=130\n");
		return ret;
	}
	printk("mt6516_wdt_probe : Success to register miscdev\n");
	#endif

#ifdef CONFIG_WD_KICKER
	wk_register_wdt(&mt6516_wk_wdt);
#endif

	return 0;
}

static int mt6516_wdt_remove(struct device *dev)
{
	printk("******** MT6516 WDT driver remove!! ********\n" );

	free_irq(MT6516_WDT_IRQ_LINE, NULL);
	misc_deregister(&mt6516_wdt_miscdev);
	
	return 0;
}

static void mt6516_wdt_shutdown(struct device *dev)
{
	printk("******** MT6516 WDT driver shutdown!! ********\n" );

	//To stop HW WDT or keepalive

	//disable WDT reset, auto-restart disable , disable intr
	mt6516_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);

	mt6516_wdt_Restart();
}

static int mt6516_wdt_suspend(struct device *dev, pm_message_t state)
{
	printk("******** MT6516 WDT driver suspend!! ********\n" );
	
	//To stop HW WDT or keepalive

	//disable WDT reset, auto-restart disable , disable intr
	mt6516_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);

	mt6516_wdt_Restart();
	
	return 0;
}

static int mt6516_wdt_resume(struct device *dev)
{
	printk("******** MT6516 WDT driver resume!! ********\n" );
	
	#if 0	
	//To stop HW WDT or keepalive

	//disable WDT reset, auto-restart disable, disable intr
	mt6516_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);

	mt6516_wdt_Restart();
	#endif

	if ( g_WDT_enable == 1 ) 
	{
		//printk("g_WDT_enable == 1\n" );
	
		//set timeout and keep HW WDT alive
		mt6516_wdt_SetTimeOutValue(g_lastTime_TimeOutValue);
		
		if (g_wdt_mode == WDT_EXP_MODE)
			mt6516_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE);
		else
			mt6516_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);

		mt6516_wdt_Restart();
	}
	
	return 0;
}

static struct device_driver mt6516_wdt_driver = {
	.name		= "mt6516-wdt",
	.bus		= &platform_bus_type,
	.probe		= mt6516_wdt_probe,
	.remove		= mt6516_wdt_remove,
	.shutdown	= mt6516_wdt_shutdown,
	#ifdef CONFIG_PM
	.suspend	= mt6516_wdt_suspend,
	.resume		= mt6516_wdt_resume,
	#endif
};

struct platform_device MT6516_device_wdt = {
		.name				= "mt6516-wdt",
		.id					= 0,
		.dev				= {
		}
};

static int __init mt6516_wdt_init(void)
{
	struct class_device *class_dev = NULL;
	int ret;
	
	ret = platform_device_register(&MT6516_device_wdt);
	if (ret) {
		printk("****[mt6516_wdt_driver] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = driver_register(&mt6516_wdt_driver);
	if (ret) {
		printk("****[mt6516_wdt_driver] Unable to register driver (%d)\n", ret);
		return ret;
	}

	//------------------------------------------------------------------
	// 							Create ioctl
	//------------------------------------------------------------------
	ret = alloc_chrdev_region(&wdt_devno, 0, 1, WDT_DEVNAME);
	if (ret) 
		printk("Error: Can't Get Major number for mt6516 WDT \n");
	wdt_cdev = cdev_alloc();
	wdt_cdev->owner = THIS_MODULE;
	wdt_cdev->ops = &mt6516_wdt_fops;
	ret = cdev_add(wdt_cdev, wdt_devno, 1);
	if(ret)
	    printk("mt6516 wdt Error: cdev_add\n");
	wdt_major = MAJOR(wdt_devno);
	wdt_class = class_create(THIS_MODULE, WDT_DEVNAME);
	class_dev = (struct class_device *)device_create(wdt_class, 
													NULL, 
													wdt_devno, 
													NULL, 
													WDT_DEVNAME);

	printk("mt6516_wdt_probe done\n");

	return 0;	
}

static void __exit mt6516_wdt_exit (void)
{
}

module_init(mt6516_wdt_init);
module_exit(mt6516_wdt_exit);

MODULE_AUTHOR("James Lo");
MODULE_DESCRIPTION("MT6516 Watchdog Device Driver");
MODULE_LICENSE("GPL");
