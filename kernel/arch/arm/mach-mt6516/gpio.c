

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <linux/platform_device.h>

#include <linux/types.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/atomic.h>

#include <linux/mtgpio.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_gpio.h>

//#define  GIO_SLFTEST            
#define  GPIO_DEVICE "mt6516-gpio"
#define  VERSION     "$Revision$"

#define  DRV_WriteReg32(addr, data)  __raw_writel(data, addr)
#define  DRV_Reg32(addr)             __raw_readl(addr)

#define  TRUE                   1
#define  FALSE                  0

#define  GIO_TAG                "GPIO> "
#define  GIO_LOG(fmt, arg...)   printk(GIO_TAG fmt, ##arg)
#define  GIO_ERR(fmt, arg...)   printk(KERN_ERR GIO_TAG "%5d: "fmt, __LINE__, ##arg)
#define  GIO_FUC(fmt, arg...)   //printk(GIO_TAG "%s\n", __FUNCTION__)
#define  GIO_INVALID_OBJ(ptr)   ((ptr) != gpio_obj)
#define  GIO_RETERR(res, fmt, args...)                                               \
    do {                                                                             \
        printk(KERN_ERR GIO_TAG "%s:%04d: " fmt"\n", __FUNCTION__, __LINE__, ##args);\
        return res;                                                                  \
    } while(0)

/*GPIO direction control register*/
#define GPIO_DIR_NUM        10
#define GPIO_DIR_BASE       (GPIO_BASE+0x0000)
static u32 mt6516_gpio_dir_reg_addr[GPIO_DIR_NUM] = 
{
    (GPIO_DIR_BASE+0x0000), 
    (GPIO_DIR_BASE+0x0010), 
    (GPIO_DIR_BASE+0x0020),
    (GPIO_DIR_BASE+0x0030), 
    (GPIO_DIR_BASE+0x0040), 
    (GPIO_DIR_BASE+0x0050),
    (GPIO_DIR_BASE+0x0060), 
    (GPIO_DIR_BASE+0x0070), 
    (GPIO_DIR_BASE+0x0080),
    (GPIO_DIR_BASE+0x0090)
};

/*GPIO pull enable register*/
#define GPIO_PULLEN_NUM     10
#define GPIO_PULLEN_BASE    (GPIO_BASE+0x0100)
static u32 mt6516_gpio_pullen_reg_addr[GPIO_PULLEN_NUM] = 
{
    (GPIO_PULLEN_BASE+0x0000), 
    (GPIO_PULLEN_BASE+0x0010), 
    (GPIO_PULLEN_BASE+0x0020),
    (GPIO_PULLEN_BASE+0x0030), 
    (GPIO_PULLEN_BASE+0x0040), 
    (GPIO_PULLEN_BASE+0x0050),
    (GPIO_PULLEN_BASE+0x0060), 
    (GPIO_PULLEN_BASE+0x0070), 
    (GPIO_PULLEN_BASE+0x0080),
    (GPIO_PULLEN_BASE+0x0090)
};

/*GPIO pull selection register*/
#define GPIO_PULLSEL_NUM    10
#define GPIO_PULLSEL_BASE   (GPIO_BASE+0x0200)
static u32 mt6516_gpio_pullsel_reg_addr[GPIO_PULLSEL_NUM] = 
{
    (GPIO_PULLSEL_BASE+0x0000), 
    (GPIO_PULLSEL_BASE+0x0010), 
    (GPIO_PULLSEL_BASE+0x0020),
    (GPIO_PULLSEL_BASE+0x0030), 
    (GPIO_PULLSEL_BASE+0x0040), 
    (GPIO_PULLSEL_BASE+0x0050),
    (GPIO_PULLSEL_BASE+0x0060), 
    (GPIO_PULLSEL_BASE+0x0070), 
    (GPIO_PULLSEL_BASE+0x0080),
    (GPIO_PULLSEL_BASE+0x0090)
};
 
/*GPIO pull-up/pull-down data inversion register*/
#define GPIO_DINV_NUM   10
#define GPIO_DINV_BASE  (GPIO_BASE+0x0300)
static u32 mt6516_gpio_dinv_reg_addr[GPIO_DINV_NUM] = 
{
    (GPIO_DINV_BASE+0x0000), 
    (GPIO_DINV_BASE+0x0010), 
    (GPIO_DINV_BASE+0x0020),
    (GPIO_DINV_BASE+0x0030), 
    (GPIO_DINV_BASE+0x0040), 
    (GPIO_DINV_BASE+0x0050),
    (GPIO_DINV_BASE+0x0060), 
    (GPIO_DINV_BASE+0x0070), 
    (GPIO_DINV_BASE+0x0080),
    (GPIO_DINV_BASE+0x0090)
};

/*GPIO data output register*/
#define GPIO_DOUT_NUM   10
#define GPIO_DOUT_BASE  (GPIO_BASE+0x0400)
static u32 mt6516_gpio_dout_reg_addr[GPIO_DOUT_NUM] = 
{
    (GPIO_DOUT_BASE+0x0000), 
    (GPIO_DOUT_BASE+0x0010), 
    (GPIO_DOUT_BASE+0x0020),
    (GPIO_DOUT_BASE+0x0030), 
    (GPIO_DOUT_BASE+0x0040), 
    (GPIO_DOUT_BASE+0x0050),
    (GPIO_DOUT_BASE+0x0060), 
    (GPIO_DOUT_BASE+0x0070), 
    (GPIO_DOUT_BASE+0x0080),
    (GPIO_DOUT_BASE+0x0090)
};

/*GPIO data input register*/
#define GPIO_DIN_NUM    10
#define GPIO_DIN_BASE   (GPIO_BASE+0x0500)
static u32 mt6516_gpio_din_reg_addr[GPIO_DIN_NUM] = 
{
    (GPIO_DIN_BASE+0x0000), 
    (GPIO_DIN_BASE+0x0010), 
    (GPIO_DIN_BASE+0x0020),
    (GPIO_DIN_BASE+0x0030), 
    (GPIO_DIN_BASE+0x0040), 
    (GPIO_DIN_BASE+0x0050),
    (GPIO_DIN_BASE+0x0060), 
    (GPIO_DIN_BASE+0x0070), 
    (GPIO_DIN_BASE+0x0080),
    (GPIO_DIN_BASE+0x0090)
};

/*GPIO data input register*/
#define GPIO_MODE_NUM   19
#define GPIO_MODE_BASE  (GPIO_BASE+0x0600)
static u32 mt6516_gpio_mode_reg_addr[GPIO_MODE_NUM] = 
{
    (GPIO_MODE_BASE+0x0000), 
    (GPIO_MODE_BASE+0x0010), 
    (GPIO_MODE_BASE+0x0020),
    (GPIO_MODE_BASE+0x0030), 
    (GPIO_MODE_BASE+0x0040), 
    (GPIO_MODE_BASE+0x0050),
    (GPIO_MODE_BASE+0x0060), 
    (GPIO_MODE_BASE+0x0070), 
    (GPIO_MODE_BASE+0x0080),
    (GPIO_MODE_BASE+0x0090),
    (GPIO_MODE_BASE+0x00A0), 
    (GPIO_MODE_BASE+0x00B0), 
    (GPIO_MODE_BASE+0x00C0),
    (GPIO_MODE_BASE+0x00D0), 
    (GPIO_MODE_BASE+0x00E0), 
    (GPIO_MODE_BASE+0x00F0),
    (GPIO_MODE_BASE+0x0100), 
    (GPIO_MODE_BASE+0x0110), 
    (GPIO_MODE_BASE+0x0120), 
};

/*clock output setting*/
#define CLK_OUT_NUM     8
#define CLK_OUT_BASE    (GPIO_BASE+0x0900)
static u32 mt6516_gpio_clkout_reg_addr[CLK_OUT_NUM] = 
{
    (CLK_OUT_BASE+0x0000),  
    (CLK_OUT_BASE+0x0010),  
    (CLK_OUT_BASE+0x0020), 
    (CLK_OUT_BASE+0x0030),  
    (CLK_OUT_BASE+0x0040),  
    (CLK_OUT_BASE+0x0050), 
    (CLK_OUT_BASE+0x0060),  
    (CLK_OUT_BASE+0x0070), 
};
//static spinlock_t obj->lock = SPIN_LOCK_UNLOCKED;
struct gpio_object {
    atomic_t        ref;
    dev_t           devno;
    struct class    *cls;
    struct device   *dev;
    struct cdev     chrdev;
    spinlock_t      lock;
};
static struct gpio_object gpio_dat = {
    .ref  = ATOMIC_INIT(0),
    .cls  = NULL,
    .dev  = NULL,
    .lock = SPIN_LOCK_UNLOCKED,
};
static struct gpio_object *gpio_obj = &gpio_dat;
/*****************************************************************************/
#if defined(GIO_SLFTEST)
/*****************************************************************************/
void mt_gpio_slf_test(void)
{
    int i;
    for (i = 0; i < GPIO_MAX; i++)
    {
        s32 res,old;
        GIO_ERR("GPIO-%3d test\n", i);
        /*direction test*/
        old = mt_get_gpio_dir(i);
        if (old == 0 || old == 1)
        {
            GIO_LOG(" dir old = %d\n", old);
        }
        else
        {
            GIO_ERR(" test dir fail: %d\n", old);
            break;
        }
        if ((res = mt_set_gpio_dir(i, GPIO_DIR_OUT)) != RSUCCESS)
        {
            GIO_ERR(" set dir out fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_dir(i)) != GPIO_DIR_OUT)
        {
            GIO_ERR(" get dir out fail: %d\n", res);
            break;
        }
        else                 
        {
            s32 out = mt_get_gpio_out(i);
            if (out != 0 && out != 1)
            {
                GIO_ERR(" get out fail = %d\n", old);
                break;
            }
            else if ((res = mt_set_gpio_out(i,0)) != RSUCCESS)
            {
                GIO_ERR(" set out as 0 fail: %d\n", res);
                break;
            }
            else if ((res = mt_get_gpio_out(i)) != 0)
            {
                GIO_ERR(" get out fail = %d\n", res);
                break;
            }
            else if ((res = mt_set_gpio_out(i,1)) != RSUCCESS)
            {
                GIO_ERR(" set out as 1 fail: %d\n", res);
                break;
            }
            else if ((res = mt_get_gpio_out(i)) != 1)
            {
                GIO_ERR(" get out fail = %d\n", res);
                break;
            }
            else if ((res = mt_set_gpio_out(i,out)) != RSUCCESS)
            {
                GIO_ERR(" restore out fail: %d\n", res);
                break;
            }
        }
            
        if ((res = mt_set_gpio_dir(i, GPIO_DIR_IN)) != RSUCCESS)
        {
            GIO_ERR(" set dir in fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_dir(i)) != GPIO_DIR_IN)
        {
            GIO_ERR(" get dir in fail: %d\n", res);
            break;
        }
        else
        {
            GIO_LOG(" input data = %d\n", res);
        }
        if ((res = mt_set_gpio_dir(i, old)) != RSUCCESS)
        {
            GIO_ERR(" restore dir fail: %d\n", res);
            break;
        }

        /*pull enable test*/
        old = mt_get_gpio_pull_enable(i);
        if (old == 0 || old == 1)
        {
            GIO_LOG(" pullen old = %d\n", old);
        }
        else
        {
            GIO_ERR(" test pullen fail: %d\n", old);
            break;
        }
        if ((res = mt_set_gpio_pull_enable(i, TRUE)) != RSUCCESS)
        {
            GIO_ERR(" set pullen fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_pull_enable(i)) != TRUE)
        {
            GIO_ERR(" get pullen fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_pull_enable(i, FALSE)) != RSUCCESS)
        {
            GIO_ERR(" set pulldi fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_pull_enable(i)) != FALSE)
        {
            GIO_ERR(" get pulldi fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_pull_enable(i, old)) != RSUCCESS)
        {
            GIO_ERR(" restore pullen fail: %d\n", res);
            break;
        }

        /*pull select test*/
        old = mt_get_gpio_pull_select(i);
        if (old == 0 || old == 1)
        {
            GIO_LOG(" pullsel old = %d\n", old);
        }
        else
        {
            GIO_ERR(" pullsel fail: %d\n", old);
            break;
        }
        if ((res = mt_set_gpio_pull_select(i, GPIO_PULL_UP)) != RSUCCESS)
        {
            GIO_ERR(" set pullup fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_pull_select(i)) != GPIO_PULL_UP)
        {
            GIO_ERR(" get pullup fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_pull_select(i, GPIO_PULL_DOWN)) != RSUCCESS)
        {
            GIO_ERR(" set pulldn fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_pull_select(i)) != GPIO_PULL_DOWN)
        {
            GIO_ERR(" get pulldn fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_pull_select(i, old)) != RSUCCESS)
        {
            GIO_ERR(" restore pullsel fail: %d\n", res);
            break;
        }     

        /*data inversion*/
        old = mt_get_gpio_inversion(i);
        if (old == 0 || old == 1)
        {
            GIO_LOG(" inv old = %d\n", old);
        }
        else
        {
            GIO_ERR(" inv fail: %d\n", old);
            break;
        }
        if ((res = mt_set_gpio_inversion(i, TRUE)) != RSUCCESS)
        {
            GIO_ERR(" set inven fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_inversion(i)) != TRUE)
        {
            GIO_ERR(" get inven fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_inversion(i, FALSE)) != RSUCCESS)
        {
            GIO_ERR(" set invdi fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_inversion(i)) != FALSE)
        {
            GIO_ERR(" get invdi fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_inversion(i, old)) != RSUCCESS)
        {
            GIO_ERR(" restore inv fail: %d\n", res);
            break;
        }     

        /*mode control*/
        old = mt_get_gpio_mode(i);
        if (old == 0 || old == 1 || old == 2 || old == 3)
        {
            GIO_LOG(" mode old = %d\n", old);
        }
        else
        {
            GIO_ERR(" get mode fail: %d\n", old);
            break;
        }
        if ((res = mt_set_gpio_mode(i, GPIO_MODE_00)) != RSUCCESS)
        {
            GIO_ERR(" set mode00 fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_mode(i)) != GPIO_MODE_00)
        {
            GIO_ERR(" get mode00 fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_mode(i, GPIO_MODE_01)) != RSUCCESS)
        {
            GIO_ERR(" set mode01 fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_mode(i)) != GPIO_MODE_01)
        {
            GIO_ERR(" get mode01 fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_mode(i, GPIO_MODE_02)) != RSUCCESS)
        {
            GIO_ERR(" set mode10 fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_mode(i)) != GPIO_MODE_02)
        {
            GIO_ERR(" get mode10 fail: %d\n", res);
            break;
        }
        if ((res = mt_set_gpio_mode(i, GPIO_MODE_03)) != RSUCCESS)
        {
            GIO_ERR(" set mode11 fail: %d\n", res);
            break;
        }
        else if ((res = mt_get_gpio_mode(i)) != GPIO_MODE_03)
        {
            GIO_ERR(" get mode11 fail: %d\n", res);
            break;
        }
        
        if ((res = mt_set_gpio_mode(i,old)) != RSUCCESS)
        {
            GIO_ERR(" restore mode fail: %d\n", res);
            break;
        }   
        
    }
    GIO_LOG("GPIO test done\n");
}
/*****************************************************************************/
#endif
/*****************************************************************************/

s32 mt_set_gpio_dir(u32 u4Pin, u32 u4Dir)
{
    u32 dirNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;

    if (u4Dir > GPIO_DIR_OUT)
        return -EBADDIR;
    
    dirNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (dirNo >= GPIO_DIR_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_dir_reg_addr[dirNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    if (u4Dir == 1)    
        regValue |= (1UL << bitOffset);    
    else     
        regValue &= ~(1UL << bitOffset);                

    DRV_WriteReg32(pinReg, regValue);    
    spin_unlock(&obj->lock);

    return RSUCCESS;
}
EXPORT_SYMBOL(mt_set_gpio_dir);


s32 mt_get_gpio_dir(u32 u4Pin)
{    
    u32 dirNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");
    
    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    
    dirNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (dirNo >= GPIO_DIR_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_dir_reg_addr[dirNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);

    return (((regValue & (1L << bitOffset)) != 0)? 1: 0);
}
EXPORT_SYMBOL(mt_get_gpio_dir);


s32 mt_set_gpio_pull_enable(u32 u4Pin, u8 bPullEn)
{
    u32 pullEnNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");
    
    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;

    pullEnNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (pullEnNo >= GPIO_PULLEN_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_pullen_reg_addr[pullEnNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    
    if (bPullEn == TRUE)
        regValue |= (1UL << bitOffset);
    else 
        regValue &= ~(1UL << bitOffset);        

    DRV_WriteReg32(pinReg, regValue);
    spin_unlock(&obj->lock);

    return RSUCCESS;
}
EXPORT_SYMBOL(mt_set_gpio_pull_enable);

s32 mt_get_gpio_pull_enable(u32 u4Pin)
{
    u32 pullEnNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");
    
    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    
    pullEnNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (pullEnNo >= GPIO_PULLEN_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_pullen_reg_addr[pullEnNo];
    }
    
    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);
    
    return (((regValue & (1L << bitOffset)) != 0)? TRUE: FALSE);
}
EXPORT_SYMBOL(mt_get_gpio_pull_enable);


s32 mt_set_gpio_pull_select(u32 u4Pin, u8 uPullSel)
{
    u32 regNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    else if (uPullSel > GPIO_PULL_UP)
        return -EBADPULLSELECT;

    regNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (regNo >= GPIO_PULLSEL_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_pullsel_reg_addr[regNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    
    if (uPullSel == GPIO_PULL_UP)
        regValue |= (1UL << bitOffset);
    else 
        regValue &= ~(1UL << bitOffset);        

    DRV_WriteReg32(pinReg, regValue);
    spin_unlock(&obj->lock);

    return RSUCCESS;
}
EXPORT_SYMBOL(mt_set_gpio_pull_select);


s32 mt_get_gpio_pull_select(u32 u4Pin)
{
    u32 regNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    
    regNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (regNo >= GPIO_PULLSEL_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_pullsel_reg_addr[regNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);
    
    return (((regValue & (1L << bitOffset)) != 0)? 1: 0);
}
EXPORT_SYMBOL(mt_get_gpio_pull_select);

s32 mt_set_gpio_inversion(u32 u4Pin, u8 bInvEn)
{
    u32 regNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;

    regNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (regNo >= GPIO_DINV_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_dinv_reg_addr[regNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    
    if (bInvEn == TRUE)
        regValue |= (1UL << bitOffset);
    else 
        regValue &= ~(1UL << bitOffset);        

    DRV_WriteReg32(pinReg, regValue);
    spin_unlock(&obj->lock);

    return RSUCCESS;    
}
EXPORT_SYMBOL(mt_set_gpio_inversion);

s32 mt_get_gpio_inversion(u32 u4Pin)
{
    u32 regNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    
    regNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (regNo >= GPIO_DINV_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_dinv_reg_addr[regNo];
    }
    
    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);
    
    return (((regValue & (1L << bitOffset)) != 0)? TRUE: FALSE);
}
EXPORT_SYMBOL(mt_get_gpio_inversion);

s32 mt_set_gpio_out(u32 u4Pin, u32 u4PinOut)
{
    u32 pinOutNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;

    if (u4PinOut > GPIO_OUT_ONE)
        return -EBADINOUTVAL;
    
    pinOutNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (pinOutNo >= GPIO_DOUT_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_dout_reg_addr[pinOutNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    
    if (u4PinOut == 1)
        regValue |= (1UL << bitOffset);
    else 
        regValue &= ~(1UL << bitOffset);        

    DRV_WriteReg32(pinReg, regValue);
    spin_unlock(&obj->lock);

    return RSUCCESS;
}
EXPORT_SYMBOL(mt_set_gpio_out);


s32 mt_get_gpio_out(u32 u4Pin)
{
    u32 pinOutNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    
    pinOutNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (pinOutNo >= GPIO_DOUT_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_dout_reg_addr[pinOutNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);
    
    return (((regValue & (1L << bitOffset)) != 0)? 1: 0);
}
EXPORT_SYMBOL(mt_get_gpio_out);


s32 mt_get_gpio_in(u32 u4Pin)
{
    u32 pinInNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    
    pinInNo = u4Pin / MAX_GPIO_REG_BITS;
    bitOffset = u4Pin % MAX_GPIO_REG_BITS;

    if (pinInNo >= GPIO_DIN_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_din_reg_addr[pinInNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);
    
    return (((regValue & (1L << bitOffset)) != 0)? 1: 0);
}
EXPORT_SYMBOL(mt_get_gpio_in);

s32 mt_set_gpio_mode(u32 u4Pin, u32 u4Mode)
{
    u32 regNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    u32 pinMask = 0x03;    
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;

    if (u4Mode > GPIO_MODE_03)
        return -EBADINOUTVAL;
    
    regNo = u4Pin / MAX_GPIO_MODE_PER_REG;
    bitOffset = u4Pin % MAX_GPIO_MODE_PER_REG;

    if (regNo >= GPIO_MODE_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_mode_reg_addr[regNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);

    regValue &= ~(pinMask << (2*bitOffset));
    regValue |= (u4Mode << (2*bitOffset));
    
    DRV_WriteReg32(pinReg, regValue);
    spin_unlock(&obj->lock);

    return RSUCCESS;
}
EXPORT_SYMBOL(mt_set_gpio_mode);

s32 mt_get_gpio_mode(u32 u4Pin)
{
    u32 regNo;
    u32 bitOffset;
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4Pin >= MAX_GPIO_PIN)
        return -EEXCESSPINNO;
    
    regNo = u4Pin / MAX_GPIO_MODE_PER_REG;
    bitOffset = u4Pin % MAX_GPIO_MODE_PER_REG;

    if (regNo >= GPIO_MODE_NUM)
    {
        return -EEXCESSPINNO;
    }
    else
    {
        pinReg = mt6516_gpio_mode_reg_addr[regNo];
    }

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);
    
    return ((regValue & (0x03 << (2*bitOffset))) >> (2*bitOffset));
}
EXPORT_SYMBOL(mt_get_gpio_mode);

s32 mt_set_clock_output(u32 u4ClkOut, u32 u4Src)
{
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4ClkOut >= CLK_MAX)
        return -EEXCESSCLKOUT;

    if (u4Src >= CLK_SRC_MAX)
        return -EBADCLKSRC;
       
    pinReg = mt6516_gpio_clkout_reg_addr[u4ClkOut];
   
    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);

    regValue &= ~(0x0f);
    regValue |= (u4Src & 0x0f);
    
    DRV_WriteReg32(pinReg, regValue);
    spin_unlock(&obj->lock);

    return RSUCCESS;
}
EXPORT_SYMBOL(mt_set_clock_output);

s32 mt_get_clock_output(u32 u4ClkOut)
{
    u32 regValue;
    u32 pinReg;
    struct gpio_object *obj = gpio_obj;

    if (!obj)
        GIO_RETERR(-EACCES,"");

    if (u4ClkOut >= CLK_MAX)
        return -EEXCESSCLKOUT;
    
    pinReg = mt6516_gpio_clkout_reg_addr[u4ClkOut];

    spin_lock(&obj->lock);
    regValue = DRV_Reg32(pinReg);
    spin_unlock(&obj->lock);
    
    return (regValue & 0x0f);
}
EXPORT_SYMBOL(mt_get_clock_output);

static int mt_gpio_open(struct inode *inode, struct file *file)
{
    struct gpio_object *obj = container_of(inode->i_cdev, struct gpio_object, chrdev);

    GIO_FUC();

    if (!obj) {
        GIO_ERR("NULL pointer");
        return -EFAULT;
    }

    atomic_inc(&obj->ref);
    file->private_data = obj;
    return nonseekable_open(inode, file);
}


static int mt_gpio_release(struct inode *inode, struct file *file)
{
    struct gpio_object *obj = container_of(inode->i_cdev, struct gpio_object, chrdev);

    GIO_FUC();

    if (!obj) {
        GIO_ERR("NULL pointer");
        return -EFAULT;
    }

    atomic_dec(&obj->ref);
    return RSUCCESS;
}

static int mt_gpio_ioctl(struct inode *inode, struct file *file, 
                             unsigned int cmd, unsigned long arg)
{
    struct gpio_object *obj = container_of(inode->i_cdev, struct gpio_object, chrdev);
    int res;
    u32 pin;

    GIO_FUC();

    if (!obj) {
        GIO_ERR("NULL pointer");
        return -EFAULT;
    }

    switch(cmd) 
    {
        case GPIO_IOCQMODE:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_get_gpio_mode(pin);
            break;
        }
        case GPIO_IOCTMODE0:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_mode(pin, GPIO_MODE_00);
            break;
        }
        case GPIO_IOCTMODE1:      
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_mode(pin, GPIO_MODE_01);
            break;
        }
        case GPIO_IOCTMODE2:      
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_mode(pin, GPIO_MODE_02);
            break;
        }
        case GPIO_IOCTMODE3:      
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_mode(pin, GPIO_MODE_03);
            break;
        }
        case GPIO_IOCQDIR:        
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_get_gpio_dir(pin);
            break;
        }
        case GPIO_IOCSDIRIN:      
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_dir(pin, GPIO_DIR_IN);
            break;
        }
        case GPIO_IOCSDIROUT:     
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_dir(pin, GPIO_DIR_OUT);
            break;
        }
        case GPIO_IOCQPULLEN:       
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_get_gpio_pull_enable(pin);
            break;
        }
        case GPIO_IOCSPULLENABLE:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_pull_enable(pin, TRUE);
            break;
        }
        case GPIO_IOCSPULLDISABLE:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_pull_enable(pin, FALSE);
            break;
        }
        case GPIO_IOCQPULL:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_get_gpio_pull_select(pin);
            break;
        }
        case GPIO_IOCSPULLDOWN:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_pull_select(pin, GPIO_PULL_DOWN);
            break;
        }
        case GPIO_IOCSPULLUP:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_pull_select(pin, GPIO_PULL_UP);
            break;
        }
        case GPIO_IOCQINV:        
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_get_gpio_inversion(pin);
            break;
        }
        case GPIO_IOCSINVENABLE:
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_inversion(pin, TRUE);
            break;
        }
        case GPIO_IOCSINVDISABLE: 
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_inversion(pin, FALSE);
            break;
        }
        case GPIO_IOCQDATAIN:     
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EFAULT) : mt_get_gpio_in(pin);
            break;
        }
        case GPIO_IOCQDATAOUT:    
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_get_gpio_out(pin);
            break;
        }
        case GPIO_IOCSDATALOW:    
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_out(pin, GPIO_OUT_ZERO);
            break;
        }
        case GPIO_IOCSDATAHIGH:   
        {
            pin = (u32)arg;
            res = GIO_INVALID_OBJ(obj) ? (-EACCES) : mt_set_gpio_out(pin, GPIO_OUT_ONE);
            break;
        }
        default:
        {
            res = -EPERM;
            break;
        }
    }

    if (res == -EACCES)
        GIO_ERR(" cmd = 0x%8X, invalid pointer\n", cmd);
    else if (res < 0)
        GIO_ERR(" cmd = 0x%8X, err = %d\n", cmd, res);
    return res;
}
/*****************************************************************************/
static struct file_operations mt_gpio_fops = 
{
    .owner=        THIS_MODULE,
    .ioctl=        mt_gpio_ioctl,
    .open=         mt_gpio_open,    
    .release=      mt_gpio_release,
};
/*****************************************************************************/
static int mt_gpio_create_chrdev(struct platform_device *pdev)
{
    int res;
    struct gpio_object *obj = platform_get_drvdata(pdev);

    if (!obj)
        GIO_RETERR(-EACCES,"");

    res = alloc_chrdev_region(&obj->devno, 0, 0 , GPIO_DEVNAME);
    if (res) {
        GIO_ERR("alloc_chrdev_region = %d\n", res);
        goto ERROR;
    }

    GIO_LOG("alloc %s:%d:%d\n", GPIO_DEVNAME, MAJOR(obj->devno), MINOR(obj->devno));

    cdev_init(&obj->chrdev, &mt_gpio_fops);    
    obj->chrdev.owner = THIS_MODULE;

    res = cdev_add(&obj->chrdev, obj->devno, 1);
    if (res) {
        GIO_ERR("cdev_add = %d\n", res);
        goto ERROR;
    }

    obj->cls = class_create(THIS_MODULE, GPIO_CLSNAME);
    if (IS_ERR(obj->cls)) {
        res = PTR_ERR(obj->cls);
        GIO_ERR("class_create = %d\n", res);
        goto ERROR;
    }

    obj->dev = device_create(obj->cls, NULL, obj->devno, NULL, GPIO_DEVNAME);
    if (!obj->dev) {
        res = -ENOMEM;
        GIO_ERR("device_create, no memory");
        goto ERROR;
    }        
    return RSUCCESS;

ERROR:
    return res;
}
/*****************************************************************************/
static int mt_gpio_delete_chrdev(struct gpio_object *obj)
{   
    if (!obj)    
        return RSUCCESS;
    
    device_destroy(obj->cls, obj->devno);
    class_destroy(obj->cls);

    cdev_del(&obj->chrdev);
    unregister_chrdev_region(obj->devno, 1);

    kfree(obj);
    return RSUCCESS;
}
/*****************************************************************************/
static int mt_gpio_probe(struct platform_device *dev)
{
    int ret;
    
    printk("Registering GPIO device\n");
    if (!gpio_obj)
        GIO_RETERR(-EACCES, "");
    platform_set_drvdata(dev, gpio_obj);
    ret = mt_gpio_create_chrdev(dev);
    
    if (ret < 0) 
        return ret;
#if defined(GIO_SLFTEST)        
    mt_gpio_slf_test();
#endif     
    return ret;
}

/*****************************************************************************/
static int mt_gpio_remove(struct platform_device *dev)
{
    struct gpio_object *obj = platform_get_drvdata(dev);

    return mt_gpio_delete_chrdev(obj);
}
/*****************************************************************************/
static void mt_gpio_shutdown(struct platform_device *dev)
{
    printk("GPIO Shut down\n");
}
/*****************************************************************************/
static int mt_gpio_suspend(struct platform_device *dev, pm_message_t state)
{
    printk("GPIO Suspend : %d!\n", state.event);
    return RSUCCESS;
}
/*****************************************************************************/
static int mt_gpio_resume(struct platform_device *dev)
{
    printk("GPIO Resume !\n");
    return RSUCCESS;
}
/*****************************************************************************/
static struct platform_driver gpio_driver = 
{
    .probe          = mt_gpio_probe,
    .remove         = mt_gpio_remove,
    .shutdown       = mt_gpio_shutdown,
    .suspend        = mt_gpio_suspend,
    .resume         = mt_gpio_resume,
    .driver         = {
            .name = "mt6516-gpio",
        },    
};

static int __init mt_gpio_init(void)
{
    int ret = 0;
    printk("MediaTek MT6516 gpio driver, version %s\n", VERSION);
    
    ret = platform_driver_register(&gpio_driver);
    //ret = platform_device_register(&gpio_dev);
    return ret;
}

 
static void __exit mt_gpio_exit(void)
{
    platform_driver_unregister(&gpio_driver);
    return;
}

module_init(mt_gpio_init);
module_exit(mt_gpio_exit);
MODULE_AUTHOR("MingHsien Hsieh <minghsien.hsieh@mediatek.com>");
MODULE_DESCRIPTION("MT6516 General Purpose Driver (GPIO) $Revision$");
MODULE_LICENSE("GPL");
