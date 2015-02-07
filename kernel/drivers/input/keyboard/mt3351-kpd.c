

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/autoconf.h>

#include <linux/types.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_gpio.h>
#include <asm/io.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <mach/irqs.h>

#include <linux/input.h>

#define  VERSION	    "v 0.2"
#define  KPD_DEVICE	    "mt3351-kpd"
#define  MENU_PIN_NO	    13
    
#define  COUNT_DOWN_TIME    1/2

#define 	KP_STA      (KP_BASE+0x0000) /* Keypad status register       */
#define 	KP_MEM1     (KP_BASE+0x0004) /* Keypad scan output register1 */
#define 	KP_MEM2     (KP_BASE+0x0008) /* Keypad scan output register2 */
#define 	KP_MEM3     (KP_BASE+0x000C) /* Keypad scan output register3 */
#define 	KP_MEM4     (KP_BASE+0x0010) /* Keypad scan output register4 */
#define 	KP_MEM5     (KP_BASE+0x0014) /* Keypad scan output register5 */
#define 	KP_DEBOUNCE (KP_BASE+0x0014) /* Keypad debounce period       */

#define DRV_WriteReg32(addr, data)  __raw_writel(data, addr)
#define DRV_Reg32(addr)             __raw_readl(addr)

static volatile unsigned int pwk_pressed = 0;
static volatile unsigned int mnk_pressed = 0;
unsigned long press_stamp;
unsigned long menu_press_stamp;
unsigned int count_down;
struct timer_list timer;

static int *keymap;

extern void MT3351_IRQSensitivity(unsigned char code, unsigned char edge);
extern void MT3351_IRQMask(unsigned int line);
extern void MT3351_IRQUnmask(unsigned int line);
extern void MT3351_IRQClearInt(unsigned int line);

struct mt3351_kp_platform_data 
{
	int rows;
	int cols;
	int *keymap;
	unsigned int keymapsize;
	unsigned int rep:1;
	unsigned long delay;
	unsigned int dbounce:1;
};

struct mt3351_kp
{
    struct input_dev *input;
    int rows;
    int cols;
};

/* Marked for the press powerkey 3 secs shutdown, but reboot automatically */



static irqreturn_t kpd_interrupt_handler(int irq, void *dev_id)
{
    struct mt3351_kp *edev = dev_id;

    MT3351_IRQMask(MT3351_KPAD_IRQ_CODE);

    if(!pwk_pressed)
    {
	if(mnk_pressed)
	{
	    press_stamp = jiffies;
	    input_report_key(edev->input, 0x6b, 1);
	    pwk_pressed = 1;
	}
	else
	{
            press_stamp = jiffies;
	    pwk_pressed = 1;
            //printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	    input_event(edev->input, EV_KEY, 0x9e, 1);
	}
    }
    else
    {   
#if 1
        //if(time_after(jiffies, (press_stamp + 3 * HZ)) && mnk_pressed)
	if(mnk_pressed)
        {
	    input_report_key(edev->input, 0x6b, 0);
        }        
#endif	
        pwk_pressed = 0;
        input_event(edev->input, EV_KEY, 0x9e, 0);
    }

    MT3351_IRQClearInt(MT3351_IRQ_KPAD_CODE);
    MT3351_IRQUnmask(MT3351_IRQ_KPAD_CODE);
    
    return IRQ_HANDLED;
}


static irqreturn_t menu_interrupt_handler(int irq, void *dev_id)
{
    struct mt3351_kp *edev = dev_id;
    unsigned int get_eint;

    get_eint = mt_get_gpio_in(13);

    if(get_eint)
        __raw_writel(0x8004, 0xf0022190);
    else
        __raw_writel(0x8804, 0xf0022190);
    
    if(!mnk_pressed)
    {
	menu_press_stamp = jiffies;
	mnk_pressed = 1;
	//printk("##################################\n");
        input_event(edev->input, EV_KEY, 0xe5, 1);
    }
    else
    {   
	if(time_after(jiffies, (menu_press_stamp + COUNT_DOWN_TIME * HZ)))
        {
            mnk_pressed = 0;             
        }	
        mnk_pressed = 0;
        input_event(edev->input, EV_KEY, 0xe5, 0);
    }  
    return IRQ_HANDLED;
}


s32 kpd_set_debounce(u16 u2Debounce)
{
    if(u2Debounce > 0x4000)
    {
        printk("Keypad debounce time is too small !!\n");
        return -1;
    }
    else
    {
        DRV_WriteReg32(KP_DEBOUNCE, u2Debounce);
        return 0;
    }
}


s32 kpd_set_gpio(void)
{
    mt_set_gpio_dir(MENU_PIN_NO, GPIO_DIR_IN);
    mt_set_gpio_pulldown(MENU_PIN_NO, GPIO_PD_DISABLE);
    mt_set_gpio_pullup(MENU_PIN_NO, GPIO_PU_ENABLE);
    mt_set_gpio_dir(44, GPIO_DIR_OUT);
    mt_set_gpio_OCFG(GPIO13_OCTL_GPIO, GPIO13_OCTL);
    mt_set_gpio_OCFG(GPIO44_OCTL_GPIO, GPIO44_OCTL);
    mt_set_gpio_ICFG(KPCOL7_GPIO44_INPUT, KPCOL7_SRC);
    mt_set_gpio_ICFG(DSPEINT_GPIO6_INPUT, DSPEINT_SRC);
    mt_set_gpio_ICFG(PWRKEY_DEBOUNCE_INPUT, PWRKEY_SRC);
    mt_set_gpio_ICFG(EINT7_GPIO13_INPUT, EINT7_SRC);
    
#if 0	// For EVB Keypad (NAND IF)
    mt_set_gpio_pullup( 85, 1); // Column 0
    mt_set_gpio_pullup( 86, 1); // Column 1
    mt_set_gpio_pullup( 87, 1); // Column 2
    mt_set_gpio_pullup( 88, 1); // Column 3
    mt_set_gpio_pullup( 89, 1); // Column 4
    mt_set_gpio_pullup( 90, 1); // Column 5
    mt_set_gpio_pullup( 91, 1); // Column 6
    mt_set_gpio_pullup( 44, 1); // Column 7

    mt_set_gpio_PinMux(PIN_MUX_NLD_CTRLL_KEYPAD, NLD_CTLL);
    mt_set_gpio_PinMux(PIN_MUX_NLD_CTRLH_KEYPAD, NLD_CTLH);
    mt_set_gpio_PinMux(PIN_MUX_CAM0_CTRL_GPIO, CAM0_CTL);

    if(mt_get_gpio_PinMux(CAM1_CTL)==PIN_MUX_CAM1_CTRL_KEYPAD)
    {
        mt_set_gpio_PinMux(PIN_MUX_CAM1_CTRL_GPIO, CAM1_CTL);
    }

    mt_set_gpio_ICFG(KPCOL7_GPIO44_INPUT, KPCOL7_SRC);
#endif

    return RSUCCESS;
}


static irqreturn_t mt_kpd_eint_irq(int irq, void *mt, struct pt_regs *regs)
{
    irqreturn_t status;

    /* must exist to unmask uint irq line */
    printk("EXTERNAL INTERRUPT: should not go here, please set debounce time\n");
    
    status = IRQ_HANDLED;

    return status;
}


static int kpd_probe(struct platform_device *pdev)
{
    struct mt3351_kp *mt3351_kp;
    struct input_dev *input_dev;
    int i, ret;

    struct mt3351_kp_platform_data *pdata = pdev->dev.platform_data;
    
    if(!pdata->rows || !pdata->cols || !pdata->keymap)
    {
        printk("No rows, cols or keymap from pdev!\n");
        return -ENOMEM;
    }
    
    mt3351_kp = kzalloc(sizeof(struct mt3351_kp), GFP_KERNEL);
    input_dev = input_allocate_device();

    if (!input_dev || !mt3351_kp)
	return -ENOMEM;

    platform_set_drvdata(pdev, mt3351_kp); 
    
    mt3351_kp->input = input_dev;  
    
    /* set up pinmux */
    kpd_set_gpio();
    
    kpd_set_debounce(0x400);

    keymap = pdata->keymap;

    if(pdata->rep)
        __set_bit(EV_REP, input_dev->evbit);

    mt3351_kp->rows = pdata->rows;        
    mt3351_kp->cols = pdata->cols;

    __set_bit(EV_KEY, input_dev->evbit);
    
    for(i=0; keymap[i] !=0; i++)
        __set_bit(keymap[i] & KEY_MAX, input_dev->keybit);

    input_dev->name = KPD_DEVICE;

    input_dev->dev.parent = &pdev->dev;
    input_dev->id.bustype = BUS_HOST;
    input_dev->id.vendor  = 0x0001;
    input_dev->id.product = 0x0001;
    input_dev->id.version = 0x0100;

    ret = input_register_device(input_dev);

    if(ret < 0)
    {
        printk("Oops unable to register mt3351-keypad input device!\n");
    }
    
    /* register IRQ line and ISR */   
    MT3351_IRQSensitivity(MT3351_KPAD_IRQ_CODE, MT3351_EDGE_SENSITIVE);

    request_irq(MT3351_IRQ_KPAD_CODE, kpd_interrupt_handler, 0, "MT3351_KPD", mt3351_kp);    
    request_irq(MT3351_EIT_IRQ_CODE , mt_kpd_eint_irq, 0, "MT3351_MENU", mt3351_kp);    
    request_irq(71, menu_interrupt_handler, 0, "MT3351_MENU", mt3351_kp);
    
    return RSUCCESS;
}


static int kpd_remove(struct device *dev)
{
    return RSUCCESS;
}


static void kpd_shutdown(struct device *dev)
{
    printk("KPD Shut down\n");
}


static int kpd_suspend(struct device *dev)
{
    printk("KPD Suspend !\n");
    return RSUCCESS;
}


static int kpd_resume(struct device *dev)
{
    printk("KPD Resume !\n");
    return RSUCCESS;
}


static struct platform_driver kpd_driver = 
{
    .probe		= kpd_probe,
    .driver     = {
        .name   = KPD_DEVICE,
    },	
	.remove	    = kpd_remove,
	.shutdown	= kpd_shutdown,
	.suspend	= kpd_suspend,
	.resume	    = kpd_resume,	
};


static void kpd_release_dev(struct device *dev)
{
	/* Nothing to release? */
}


static int mt3351_keymap[] = 
{
    0xe5,
    0xe7,
    0x6c,
    0x6b,
    0x69,
    0xe8,
    0x6a,
    0x66,
    0x67,
    0x9e,
};

static struct resource mt3351_kp_resources[] = 
{
	[0] = 
	{
		.start	= MT3351_KPAD_IRQ_CODE,
		.end	= MT3351_KPAD_IRQ_CODE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mt3351_kp_platform_data mt3351_kp_data = 
{
	.rows		= 8,
	.cols		= 8,
	.keymap		= mt3351_keymap,
	.keymapsize	= ARRAY_SIZE(mt3351_keymap),
	.delay		= 4,
};

static struct platform_device kpd_dev = 
{
    .name = KPD_DEVICE,
    .id   = -1,
    .dev  = 
    {
        .platform_data = &mt3351_kp_data,
        .release  = kpd_release_dev,
    },
    .num_resources	= ARRAY_SIZE(mt3351_kp_resources),
    .resource	= mt3351_kp_resources,
};


static s32 __devinit kpd_mod_init(void)
{
    s32 ret;
 
    printk("MediaTek MT3351 kpd driver register, version %s\n", VERSION);

    ret = platform_driver_register(&kpd_driver);

    if(ret) 
    {
		printk("Unable to register kpd driver (%d)\n", ret);
		return ret;
    }

    ret = platform_device_register(&kpd_dev);

    if(ret) 
    {
        printk("Failed to register kpd device\n");
        platform_driver_unregister(&kpd_driver);
        return ret;
    }   

    return RSUCCESS;
}

 
static void __exit kpd_mod_exit(void)
{
    printk("MediaTek MT3351 keypad driver unregister, version %s\n", VERSION);
    platform_driver_unregister(&kpd_driver);    
    printk("Done\n");
}

module_init(kpd_mod_init);
module_exit(kpd_mod_exit);
MODULE_AUTHOR("Koshi, Chiu <koshi.chiu@mediatek.com>");
MODULE_DESCRIPTION("MT3351 Keypad Driver (KPD)");
MODULE_LICENSE("GPL");
