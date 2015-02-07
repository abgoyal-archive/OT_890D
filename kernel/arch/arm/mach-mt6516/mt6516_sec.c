
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>
#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <mach/mt6516_pll.h>
#include <linux/io.h>
#include <mach/mt6516_reg_base.h>
#include <rtc-mt6516.h>

/* RTC registers */
#define RTC_BBPU		(RTC_BASE + 0x0000)
#define RTC_IRQ_STA		(RTC_BASE + 0x0004)
#define RTC_IRQ_EN		(RTC_BASE + 0x0008)
#define RTC_CII_EN		(RTC_BASE + 0x000c)
#define RTC_AL_MASK		(RTC_BASE + 0x0010)
#define RTC_TC_SEC		(RTC_BASE + 0x0014)
#define RTC_TC_MIN		(RTC_BASE + 0x0018)
#define RTC_TC_HOU		(RTC_BASE + 0x001c)
#define RTC_TC_DOM		(RTC_BASE + 0x0020)
#define RTC_TC_DOW		(RTC_BASE + 0x0024)
#define RTC_TC_MTH		(RTC_BASE + 0x0028)
#define RTC_TC_YEA		(RTC_BASE + 0x002c)
#define RTC_AL_SEC		(RTC_BASE + 0x0030)
#define RTC_AL_MIN		(RTC_BASE + 0x0034)
#define RTC_AL_HOU		(RTC_BASE + 0x0038)
#define RTC_AL_DOM		(RTC_BASE + 0x003c)
#define RTC_AL_DOW		(RTC_BASE + 0x0040)
#define RTC_AL_MTH		(RTC_BASE + 0x0044)
#define RTC_AL_YEA		(RTC_BASE + 0x0048)
#define RTC_XOSCCALI		(RTC_BASE + 0x004c)
#define RTC_POWERKEY1		(RTC_BASE + 0x0050)
#define RTC_POWERKEY2		(RTC_BASE + 0x0054)
#define RTC_PDN1		(RTC_BASE + 0x0058)
#define RTC_PDN2		(RTC_BASE + 0x005c)
#define RTC_SPAR1		(RTC_BASE + 0x0064)
#define RTC_PROT		(RTC_BASE + 0x0068)
#define RTC_DIFF		(RTC_BASE + 0x006c)
#define RTC_WRTGR		(RTC_BASE + 0x0074)

#define RTC_BBPU_PWREN		   (1U << 0)	/* BBPU = 1 when alarm occurs */
#define RTC_BBPU_WRITE_EN	   (1U << 1)
#define RTC_BBPU_BBPU		   (1U << 2)	/* 1: power on, 0: power down */
#define RTC_BBPU_AUTO		   (1U << 3)
#define RTC_BBPU_CBUSY		   (1U << 6)
#define RTC_BBPU_KEY		   (0x43 << 8)

#define RTC_IRQ_EN_AL		   (1U << 0)
#define RTC_IRQ_EN_ONESHOT	   (1U << 2)

/* we map HW YEA 0 (2000) to 1968 not 1970 because 2000 is the leap year */
#define RTC_MIN_YEAR		   1968
#define RTC_NUM_YEARS		   128
//#define RTC_MAX_YEAR		   (RTC_MIN_YEAR + RTC_NUM_YEARS - 1)

#define RTC_MIN_YEAR_OFFSET	   (RTC_MIN_YEAR - 1900)

// ===============================================
// MACRO
// ===============================================
#define SEC_DEV_NAME 	   	   "SEC"
#define SEC_SYSFS 	           "SEC"
#define SEC_NAME		   "SEC"
#define SEC_SYSFS_ATTR             "SEC_ATTR"
#define FAKE_NAME		   "RES"
#define SEC_MARK		   (0x40)
#define UPDATED		   	   (0x20)
#define rtc_read(addr)		   (*(volatile u16 *)(addr))
#define rtc_write(addr, val)	   (*(volatile u16 *)(addr) = (u16)(val))
#define CCCI_SEC_MAJOR             182

// ===============================================
// COMMAND
// ===============================================
typedef enum {
	SET_SEC			   = 1,
	UNSET_SEC	           = 2,
} sec_ioctl_cmd;

// ===============================================
// EXTERNAL VARIABLES
// ===============================================
extern unsigned int g_max_bound_addr;

// ===============================================
// GLOBAL VARIABLES
// ===============================================
BOOL g_sec_lock = false;
static dev_t                       sec_dev_num;
static DEFINE_SPINLOCK(sec_rtc_lock);

// ===============================================
// EXTERNAL FUNCTIONS
// ===============================================
extern void rtc_mark_swreset(void);

// ===============================================
// SEC_SHOW
// ===============================================
static ssize_t sec_show(struct kobject *kobj, struct attribute *a, char *buf)
{
    printk("SEC_SHOW\n");
    return sprintf(buf, "%d\n", g_sec_lock);
}

// ===============================================
// SEC_STORE
// ===============================================
static ssize_t sec_store(struct kobject *kobj, struct attribute *a, const char *buf, size_t count)
{   
    printk("SEC_STORE\n");
    return count;
}

// ===============================================
// RTC MANIPULATION
// ===============================================
static void rtc_writeif_unlock(void)
{
	rtc_write(RTC_PROT, 0x9136);
	rtc_write(RTC_WRTGR, 1);
	while (rtc_read(RTC_BBPU) & RTC_BBPU_CBUSY);	/* 120 us */
}
static void rtc_writeif_lock(void)
{
	rtc_write(RTC_PROT, 0);
	rtc_write(RTC_WRTGR, 1);
	while (rtc_read(RTC_BBPU) & RTC_BBPU_CBUSY);	/* 120 us */
}

void rtc_mark_sec(u16 val)
{
	u16 pdn2;

	spin_lock_irq(&sec_rtc_lock);
	pdn2 = rtc_read(RTC_PDN2) & ~0x0060; // bit 7 & 6
	pdn2 |= val;
	rtc_writeif_unlock();
	rtc_write(RTC_PDN2, pdn2);
	rtc_writeif_lock();
	spin_unlock_irq(&sec_rtc_lock);
}

// ===============================================
// SEC_DEV_IOCTL
// ===============================================
static long sec_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{	
	printk("[%s] +dev_ioctl\n",FAKE_NAME);
	switch (cmd) {

		case SET_SEC:
			printk("[%s] SET_SEC 0x%x\n",FAKE_NAME,rtc_read(RTC_PDN2));
			if(g_sec_lock == false)
			{	
				if((rtc_read(RTC_PDN2) & 0x0060) == UPDATED)
				{	printk("[%s] already update\n",FAKE_NAME);
					return 0xA52C;
				}	
				else			
				{	
					printk("[%s] (rtc_read(RTC_PDN2) & 0x0060) 0x%x\n",FAKE_NAME,(rtc_read(RTC_PDN2) & 0x0060));
					printk("[%s] UPDATED 0x%x\n",FAKE_NAME,UPDATED);
					rtc_mark_sec(SEC_MARK);
					g_sec_lock = true;
					return 0x7856;
				}
			}
			break;
	
		case UNSET_SEC:
			printk("[%s] UNSET_SEC\n",FAKE_NAME);
			//rtc_mark_sec(0x0);
			//g_sec_lock = true;
			return 0xD432;
			break;
	
		default:
			return -EINVAL;
	}
	printk("[%s] -dev_ioctl\n",FAKE_NAME);

	return 0;
}

static int sec_dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] File open\n", FAKE_NAME);    
    return 0;
}

static int sec_dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Release\n",FAKE_NAME);    
    return 0;
}

/* sec object */
static struct kobject sec_kobj;
static struct sysfs_ops sec_sysfs_ops = {
    .show = sec_show,
    .store = sec_store
};

/* sec attribute */
struct attribute sec_attr = {SEC_SYSFS_ATTR, THIS_MODULE, 0644};
static struct attribute *sec_attrs[] = {
    &sec_attr,
    NULL
};

/* sec type */
static struct kobj_type sec_ktype = {
    .sysfs_ops = &sec_sysfs_ops,
    .default_attrs = sec_attrs
};

/* sec device node */
static dev_t sec_dev_num;
static struct cdev sec_cdev;
static struct file_operations sec_fops = {
    .owner = THIS_MODULE,
    .open = sec_dev_open,
    .release = sec_dev_release,
    .write = NULL,
    .read = NULL,
    .unlocked_ioctl = sec_dev_ioctl
};

/* sec device class */
static struct class *sec_class;
static struct device *sec_device;

// ===============================================
// SEC_MODE_PROC
// ===============================================
static int sec_mode_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
  char *p = page;
  int len = 0; 

  p += sprintf(p, "\n\rMT6516 RECOVERY : " );
  switch(g_sec_lock)
  {
  	case true :
		  p += sprintf(p, "LOCKED\n");  		
		  break;
  	case false :
		  p += sprintf(p, "UNLOCKED\n");  		
		  break;
	default :	  	
		  p += sprintf(p, "UNKNOWN STATE\n");  		
		  break;
  }  
  *start = page + off;
  len = p - page;
  if (len > off)
		len -= off;
  else
		len = 0;

  return len < count ? len  : count;     
}

// ===============================================
// SEC_MOD_INIT
// ===============================================
static int __init sec_mod_init(void)
{
    int ret;
    
    printk("[%s] +init mod\n",FAKE_NAME);

    /* allocate device major number */
    sec_dev_num = MKDEV(CCCI_SEC_MAJOR, 0);
    ret         = register_chrdev_region(sec_dev_num, 1, SEC_DEV_NAME);
        
    if (ret)
    {
        printk(KERN_ERR "[%s]: Register character device failed\n",SEC_DEV_NAME);
        return ret;
    }

    /* add character driver */
    cdev_init(&sec_cdev, &sec_fops);
    sec_cdev.owner = THIS_MODULE;
    sec_cdev.ops   = &sec_fops;    
    ret = cdev_add(&sec_cdev, sec_dev_num, 1);
    if (ret < 0) {
        printk("[%s] fail to add cdev\n",SEC_DEV_NAME);
        unregister_chrdev_region(sec_dev_num, 1);
        return ret;
    }

    printk("[%s]: Init complete, device major number = %d\n", FAKE_NAME, MAJOR(sec_dev_num));
    printk("[%s] -init mod\n",FAKE_NAME);

    return 0;
}

// ===============================================
// SEC_MOD_EXIT
// ===============================================
static void __exit sec_mod_exit(void)
{
    cdev_del(&sec_cdev);
}

module_init(sec_mod_init);
module_exit(sec_mod_exit);
MODULE_DESCRIPTION("MT6516 Sec Driver");
MODULE_AUTHOR("MediaTek Inc.");
MODULE_LICENSE("Proprietary");
