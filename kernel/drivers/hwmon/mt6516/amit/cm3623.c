

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#include <linux/hwmsensor.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "cm3623.h"
#define I2C_DRIVERID_CM3623 3623
/*----------------------------------------------------------------------------*/
#define CM3623_I2C_ADDR_RAR 0   /*!< the index in obj->hw->i2c_addr: alert response address */
#define CM3623_I2C_ADDR_ALS 1   /*!< the index in obj->hw->i2c_addr: ALS address */
#define CM3623_I2C_ADDR_PS  2   /*!< the index in obj->hw->i2c_addr: PS address */
/*----------------------------------------------------------------------------*/
#define CMC_TAG                  "[CM3623] "
#define CMC_FUN(f)               printk(KERN_INFO CMC_TAG"%s\n", __FUNCTION__)
#define CMC_ERR(fmt, args...)    printk(KERN_ERR  CMC_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define CMC_LOG(fmt, args...)    printk(KERN_INFO CMC_TAG fmt, ##args)
#define CMC_DBG(fmt, args...)    printk(KERN_INFO fmt, ##args)                 
extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINT_Set_Polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens);
extern void MT6516_EINT_Registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
/*----------------------------------------------------------------------------*/
#define mt6516_I2C_DATA_PORT        ((base) + 0x0000)
#define mt6516_I2C_SLAVE_ADDR       ((base) + 0x0004)
#define mt6516_I2C_INTR_MASK        ((base) + 0x0008)
#define mt6516_I2C_INTR_STAT        ((base) + 0x000c)
#define mt6516_I2C_CONTROL          ((base) + 0x0010)
#define mt6516_I2C_TRANSFER_LEN     ((base) + 0x0014)
#define mt6516_I2C_TRANSAC_LEN      ((base) + 0x0018)
#define mt6516_I2C_DELAY_LEN        ((base) + 0x001c)
#define mt6516_I2C_TIMING           ((base) + 0x0020)
#define mt6516_I2C_START            ((base) + 0x0024)
#define mt6516_I2C_FIFO_STAT        ((base) + 0x0030)
#define mt6516_I2C_FIFO_THRESH      ((base) + 0x0034)
#define mt6516_I2C_FIFO_ADDR_CLR    ((base) + 0x0038)
#define mt6516_I2C_IO_CONFIG        ((base) + 0x0040)
#define mt6516_I2C_DEBUG            ((base) + 0x0044)
#define mt6516_I2C_HS               ((base) + 0x0048)
#define mt6516_I2C_DEBUGSTAT        ((base) + 0x0064)
#define mt6516_I2C_DEBUGCTRL        ((base) + 0x0068)
/*----------------------------------------------------------------------------*/
static struct i2c_client *cm3623_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static unsigned short normal_i2c[] = {0x00, I2C_CLIENT_END }; 
static unsigned short ignore = I2C_CLIENT_END;
static struct i2c_client_address_data cm3623_addr = {
    .normal_i2c = normal_i2c,
    .probe  = &ignore,
    .ignore = &ignore,
};
/*----------------------------------------------------------------------------*/
static int cm3623_i2c_attach_adapter(struct i2c_adapter *adapter);
static int cm3623_i2c_detect(struct i2c_adapter *adapter, int address, int kind);
static int cm3623_i2c_detach_client(struct i2c_client *client);
/*----------------------------------------------------------------------------*/
static int cm3623_suspend(struct i2c_client *client, pm_message_t msg);
static int cm3623_resume(struct i2c_client *client);
/*----------------------------------------------------------------------------*/
static struct cm3623_priv *g_cm3623_ptr = NULL;
/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_TRC_ALS_DATA= 0x0001,
    CMC_TRC_PS_DATA = 0x0002,
    CMC_TRC_EINT    = 0x0004,
    CMC_TRC_IOCTL   = 0x0008,
    CMC_TRC_I2C     = 0x0010,
    CMC_TRC_CVT_ALS = 0x0020,
    CMC_TRC_CVT_PS  = 0x0040,
    CMC_TRC_DEBUG   = 0x8000,
} CMC_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;
/*----------------------------------------------------------------------------*/
struct cm3623_i2c_addr {    /*define a series of i2c slave address*/
    u8  rar;        /*Alert Response Address*/
    u8  init;       /*device initialization */
    u8  als_cmd;    /*ALS command*/
    u8  als_dat1;   /*ALS MSB*/
    u8  als_dat0;   /*ALS LSB*/
    u8  ps_cmd;     /*PS command*/
    u8  ps_dat;     /*PS data*/
    u8  ps_thd;     /*PS INT threshold*/
};
/*----------------------------------------------------------------------------*/
struct cm3623_priv {
    struct alsps_hw  *hw;
    struct i2c_client client;
    struct work_struct  eint_work;

    /*i2c address group*/
    struct cm3623_i2c_addr  addr;
    
    /*misc*/
    atomic_t    trace;
    atomic_t    i2c_retry;
    atomic_t    als_suspend;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;


    /*data*/
    u16         als;
    u8          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver cm3623_i2c_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = "cm3623 driver",
    },
    .attach_adapter     = cm3623_i2c_attach_adapter,
    .detach_client      = cm3623_i2c_detach_client,
    .suspend            = cm3623_suspend,
    .resume             = cm3623_resume,
    .id                 = I2C_DRIVERID_CM3623,
};
/*----------------------------------------------------------------------------*/
int cm3623_get_addr(struct alsps_hw *hw, struct cm3623_i2c_addr *addr)
{
    if (!hw || !addr)
        return -EFAULT;

    addr->rar       = (hw->i2c_addr[CM3623_I2C_ADDR_RAR] << 1) + 1;      /*R*/
    addr->init      = (hw->i2c_addr[CM3623_I2C_ADDR_ALS] + 1) << 1;      /*W*/
    addr->als_cmd   = (hw->i2c_addr[CM3623_I2C_ADDR_ALS] << 1);          /*W*/
    addr->als_dat1  = (hw->i2c_addr[CM3623_I2C_ADDR_ALS] << 1) + 1;      /*R*/ 
    addr->als_dat0  = ((hw->i2c_addr[CM3623_I2C_ADDR_ALS] + 1) << 1) + 1;/*R*/ 
    addr->ps_cmd    = (hw->i2c_addr[CM3623_I2C_ADDR_PS]) << 1;           /*W*/
    addr->ps_thd    = ((hw->i2c_addr[CM3623_I2C_ADDR_PS] + 1) << 1);     /*W*/
    addr->ps_dat    = (hw->i2c_addr[CM3623_I2C_ADDR_PS] << 1) + 1;       /*R*/
    return 0;    
}
/*----------------------------------------------------------------------------*/
int cm3623_get_timing(void)
{
    u32 base = I2C3_BASE; 
    return (__raw_readw(mt6516_I2C_HS) << 16) | (__raw_readw(mt6516_I2C_TIMING));
}
/*----------------------------------------------------------------------------*/
int cm3623_config_timing(int sample_div, int step_div)
{
    u32 base = I2C3_BASE; 
    unsigned long tmp;

    tmp  = __raw_readw(mt6516_I2C_TIMING) & ~((0x7 << 8) | (0x1f << 0));
    tmp  = (sample_div & 0x7) << 8 | (step_div & 0x1f) << 0 | tmp;

    return (__raw_readw(mt6516_I2C_HS) << 16) | (tmp);
}
/*----------------------------------------------------------------------------*/
int cm3623_master_recv(struct i2c_client *client, u16 addr, char *buf ,int count)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg;
    int ret = 0, retry = 0;
    int trc = atomic_read(&obj->trace);
    int max_try = atomic_read(&obj->i2c_retry);

    msg.addr = addr | I2C_A_FILTER_MSG | I2C_A_CHANGE_TIMING;
    msg.flags = client->flags & I2C_M_TEN;
    msg.flags |= I2C_M_RD;
    msg.len = count;
    msg.buf = buf;
    msg.timing = cm3623_config_timing(5, 6);

    while(retry++ < max_try) {
        ret = i2c_transfer(adap, &msg, 1);
        if (ret == 1)
            break;
        udelay(100);
    }
    
    if (unlikely(trc)) {
        if (trc & CMC_TRC_I2C)
            CMC_LOG("(recv) %x %d %d %p [%02X]\n", msg.addr, msg.flags, msg.len, msg.buf, msg.buf[0]);    
        if ((retry != 1) && (trc & CMC_TRC_DEBUG))
            CMC_LOG("(recv) %d/%d\n", retry-1, max_try); 
    }

    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    return (ret == 1) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int cm3623_master_send(struct i2c_client *client, u16 addr, char *buf ,int count)
{
    int ret = 0, retry = 0;
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    struct i2c_adapter *adap=client->adapter;
    struct i2c_msg msg;
    int trc = atomic_read(&obj->trace);
    int max_try = atomic_read(&obj->i2c_retry);

    msg.addr = addr | I2C_A_FILTER_MSG | I2C_A_CHANGE_TIMING;
    msg.flags = client->flags & I2C_M_TEN;
    msg.len = count;
    msg.buf = (char *)buf;
    msg.timing = cm3623_config_timing(5, 6);

    while(retry++ < max_try) {
        ret = i2c_transfer(adap, &msg, 1);
        if (ret == 1)
            break;
        udelay(100);
    }
    if (unlikely(trc)) {
        if (trc & CMC_TRC_I2C)
            CMC_LOG("(send) %x %d %d %p [%02X]\n", msg.addr, msg.flags, msg.len, msg.buf, msg.buf[0]);    
        if ((retry != 1) && (trc & CMC_TRC_DEBUG))
            CMC_LOG("(send) %d/%d\n", retry-1, max_try); 
    }
    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    return (ret == 1) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int cm3623_read_als(struct i2c_client *client, u16 *data)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);    
    int ret = 0;
    u8 buf[2];
    
    if (1 != (ret = cm3623_master_recv(client, obj->addr.als_dat1, (char*)&buf[1], 1))) {
        CMC_ERR("reads als data1 = %d\n", ret);
        return -EFAULT;
    } else if (1 != (ret = cm3623_master_recv(client, obj->addr.als_dat0, (char*)&buf[0], 1))) {
        CMC_ERR("reads als data2 = %d\n", ret);
        return -EFAULT;
    }
    *data = (buf[1] << 8) | (buf[0]);
    if (atomic_read(&obj->trace) & CMC_TRC_ALS_DATA)
        CMC_DBG("ALS: 0x%04X\n", (u32)(*data));
    return 0;    
}
/*----------------------------------------------------------------------------*/
int cm3623_write_als(struct i2c_client *client, u8 cmd)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);    
    u8 buf = cmd;
    int ret = 0;
    
    if (sizeof(buf) != (ret = cm3623_master_send(client, obj->addr.als_cmd, (char*)&buf, sizeof(buf)))) {
        CMC_ERR("write als = %d\n", ret);
        return -EFAULT;
    } 
    return 0;    
}
/*----------------------------------------------------------------------------*/
int cm3623_read_ps(struct i2c_client *client, u8 *data)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);    
    int ret = 0;
    
    if (sizeof(*data) != (ret = cm3623_master_recv(client, obj->addr.ps_dat, (char*)data, sizeof(*data)))) {
        CMC_ERR("reads ps data = %d\n", ret);
        return -EFAULT;
    } 

    if (atomic_read(&obj->trace) & CMC_TRC_PS_DATA)
        CMC_DBG("PS:  0x%04X\n", (u32)(*data));
    return 0;    
}
/*----------------------------------------------------------------------------*/
int cm3623_write_ps(struct i2c_client *client, u8 cmd)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    u8 buf = cmd;
    int ret = 0;
    
    if (sizeof(buf) != (ret = cm3623_master_send(client, obj->addr.ps_cmd, (char*)&buf, sizeof(buf)))) {
        CMC_ERR("write ps = %d\n", ret);
        return -EFAULT;
    } 
    return 0;    
}
/*----------------------------------------------------------------------------*/
int cm3623_write_ps_thd(struct i2c_client *client, u8 thd)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    u8 buf = thd;
    int ret = 0;
    
    if (sizeof(buf) != (ret = cm3623_master_send(client, obj->addr.ps_thd, (char*)&buf, sizeof(buf)))) {
        CMC_ERR("write thd = %d\n", ret);
        return -EFAULT;
    } 
    return 0;    
}
/*----------------------------------------------------------------------------*/
int cm3623_init_device(struct i2c_client *client)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    u8 buf[] = {0x10};
    int ret = 0;
    
    if (sizeof(buf) != (ret = cm3623_master_send(client, obj->addr.init, (char*)&buf, sizeof(buf)))) {
        CMC_ERR("init = %d\n", ret);
        return -EFAULT;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
int cm3623_read_rar(struct i2c_client *client, u8 *data)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    int ret = 0;

    if (sizeof(*data) != (ret = cm3623_master_recv(client, obj->addr.rar, (char*)data, sizeof(*data)))) {
        CMC_ERR("rar = %d\n", ret);
        return -EFAULT;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static void cm3623_power(struct alsps_hw *hw, unsigned int on) 
{
    static unsigned int power_on = 0;

    //CMC_LOG("power %s\n", on ? "on" : "off");

    if (hw->power_id != MT6516_POWER_NONE) {
        if (power_on == on) {
            CMC_LOG("ignore power control: %d\n", on);
        } else if (on) {
            if (!hwPowerOn(hw->power_id, hw->power_vol, "CM3623")) 
                CMC_ERR("power on fails!!\n");
        } else {
            if (!hwPowerDown(hw->power_id, "CM3623")) 
                CMC_ERR("power off fail!!\n");   
        }
    }
    power_on = on;
}
/*----------------------------------------------------------------------------*/
static int cm3623_enable_als(struct i2c_client *client, int enable)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);
    int err, cur = 0, old = atomic_read(&obj->als_cmd_val);
    int trc = atomic_read(&obj->trace);

    if (enable) 
        cur = old & (~SD_ALS);   
    else
        cur = old | (SD_ALS); 

    if (trc & CMC_TRC_DEBUG)
        CMC_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
    
    if (0 == (cur ^ old))
        return 0;

    if (0 == (err = cm3623_write_als(client, cur))) 
        atomic_set(&obj->als_cmd_val, cur);

    if (enable) {
        atomic_set(&obj->als_deb_on, 1);
        atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
    }

    if (trc & CMC_TRC_DEBUG)
        CMC_LOG("enable als (%d)\n", enable);
    return err;
}
/*----------------------------------------------------------------------------*/
static int cm3623_enable_ps(struct i2c_client *client, int enable)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);
    int err, cur = 0, old = atomic_read(&obj->ps_cmd_val);
    int trc = atomic_read(&obj->trace);

    if (enable)  
        cur = old & (~SD_PS);   
    else
        cur = old | (SD_PS);

    if (trc & CMC_TRC_DEBUG)
        CMC_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
    
    if (0 == (cur ^ old))
        return 0;

    if (0 == (err = cm3623_write_ps(client, cur))) 
        atomic_set(&obj->ps_cmd_val, cur);

    if (enable) {
        atomic_set(&obj->ps_deb_on, 1);
        atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
    }
    
    if (trc & CMC_TRC_DEBUG)
        CMC_LOG("enable ps  (%d)\n", enable);
    return err;
}
/*----------------------------------------------------------------------------*/
static int cm3623_check_and_clear_intr(struct i2c_client *client) 
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);
    int err;
    u8 addr;

    //if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
    //    return 0;
    
    if ((err = cm3623_read_rar(client, &addr))) {
        CMC_ERR("WARNING: read rar: %d\n", err);
        return 0;
    }

    if ((addr & 0xf0) == (obj->addr.als_cmd << 1))
        set_bit(CMC_BIT_ALS, &obj->pending_intr);
    if ((addr & 0xf0) == (obj->addr.ps_cmd << 1))
        set_bit(CMC_BIT_PS,  &obj->pending_intr);

    if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
        CMC_LOG("check intr: 0x%02X => 0x%08lX\n", addr, obj->pending_intr);
    return 0;
}
/*----------------------------------------------------------------------------*/
void cm3623_eint_func(void)
{
    struct cm3623_priv *obj = g_cm3623_ptr;
    if (!obj)
        return;

    schedule_work(&obj->eint_work);
    if (atomic_read(&obj->trace) & CMC_TRC_EINT)
        CMC_LOG("eint: als/ps intrs\n");
}
/*----------------------------------------------------------------------------*/
static void cm3623_eint_work(struct work_struct *work)
{
    struct cm3623_priv *obj = (struct cm3623_priv *)container_of(work, struct cm3623_priv, eint_work);
    int err;

    if ((err = cm3623_check_and_clear_intr(&obj->client)))
        CMC_ERR("check intrs: %d\n", err);
    MT6516_EINTIRQUnmask(CUST_EINT_ALS_NUM);      
}
/*----------------------------------------------------------------------------*/
int cm3623_setup_eint(struct i2c_client *client)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    
    g_cm3623_ptr = obj;
    /*configure to GPIO function, external interrupt*/
    mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
    mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
    mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

    MT6516_EINT_Set_Sensitivity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
    MT6516_EINT_Set_Polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    MT6516_EINT_Set_HW_Debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    MT6516_EINT_Registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, cm3623_eint_func, 0);

    MT6516_EINTIRQUnmask(CUST_EINT_ALS_NUM);  
    return 0;
}
/*----------------------------------------------------------------------------*/
static int cm3623_init_client(struct i2c_client *client)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);
    int err;

    if ((err = cm3623_setup_eint(client))) {
        CMC_ERR("setup eint: %d\n", err);
        return err;
    }
    if ((err = cm3623_check_and_clear_intr(client))) {
        CMC_ERR("check/clear intr: %d\n", err);
    //    return err;
    }
    if ((err = cm3623_init_device(client))) {
        CMC_ERR("init dev: %d\n", err);
        return err;
    }
    if ((err = cm3623_write_als(client, atomic_read(&obj->als_cmd_val)))) {
        CMC_ERR("write als: %d\n", err);
        return err;
    }
    if ((err = cm3623_write_ps(client, atomic_read(&obj->ps_cmd_val)))) {
        CMC_ERR("write ps: %d\n", err);
        return err;        
    }
    if ((err = cm3623_write_ps_thd(client, atomic_read(&obj->ps_thd_val)))) {
        CMC_ERR("write thd: %d\n", err);
        return err;        
    }
    return 0;
}
static ssize_t cm3623_show_config(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct cm3623_priv *obj;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
          atomic_read(&obj->i2c_retry), atomic_read(&obj->als_debounce), 
          atomic_read(&obj->ps_mask), atomic_read(&obj->ps_thd_val), atomic_read(&obj->ps_debounce));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_store_config(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct cm3623_priv *obj;
    int retry, als_deb, ps_deb, mask, thres;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    if (5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb)) { 
        atomic_set(&obj->i2c_retry, retry);
        atomic_set(&obj->als_debounce, als_deb);
        atomic_set(&obj->ps_mask, mask);
        atomic_set(&obj->ps_thd_val, thres);        
        atomic_set(&obj->ps_debounce, ps_deb);
    } else {
        CMC_ERR("invalid content: '%s', length = %d\n", buf, count);
    }
    return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_trace(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct cm3623_priv *obj;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_store_trace(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct cm3623_priv *obj;
    int trace;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    if (1 == sscanf(buf, "0x%x", &trace)) 
        atomic_set(&obj->trace, trace);
    else 
        CMC_ERR("invalid content: '%s', length = %d\n", buf, count);
    return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_als(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    int res;
    struct cm3623_priv *obj;   
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    if ((res = cm3623_read_als(&obj->client, &obj->als)))
        return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
    else
        return snprintf(buf, PAGE_SIZE, "0x%04X\n", obj->als);     
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_ps(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct cm3623_priv *obj;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    if ((res = cm3623_read_ps(&obj->client, &obj->ps)))
        return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
    else
        return snprintf(buf, PAGE_SIZE, "0x%04X\n", obj->ps);     
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_reg(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    struct cm3623_priv *obj;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    /*read*/
    cm3623_check_and_clear_intr(&obj->client);
    cm3623_read_ps(&obj->client, &obj->ps);
    cm3623_read_als(&obj->client, &obj->als);
    /*write*/
    cm3623_write_als(&obj->client, atomic_read(&obj->als_cmd_val));
    cm3623_write_ps(&obj->client, atomic_read(&obj->ps_cmd_val)); 
    cm3623_write_ps_thd(&obj->client, atomic_read(&obj->ps_thd_val));
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_send(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_store_send(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct cm3623_priv *obj;
    int addr, cmd;
    u8 dat;
    
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    } else if (2 != sscanf(buf, "%x %x", &addr, &cmd)) {
        CMC_ERR("invalid format: '%s'\n", buf);
        return 0;
    }
    
    dat = (u8)cmd;
    CMC_LOG("send(%02X, %02X) = %d\n", addr, cmd, 
            cm3623_master_send(&obj->client, (u16)addr, (char*)&dat, sizeof(dat)));
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_recv(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_store_recv(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct cm3623_priv *obj;
    int addr;
    u8 dat;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    } else if (1 != sscanf(buf, "%x", &addr)) {
        CMC_ERR("invalid format: '%s'\n", buf);
        return 0;
    }
    
    CMC_LOG("recv(%02X) = %d, 0x%02X\n", addr, 
            cm3623_master_recv(&obj->client, (u16)addr, (char*)&dat, sizeof(dat)), dat);
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_status(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    struct cm3623_priv *obj;
    ssize_t len = 0;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    if (obj->hw) {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d) (%02X %02X) (%02X %02X %02X) (%02X %02X %02X)\n", 
                obj->hw->i2c_num, obj->hw->power_id, obj->hw->power_vol, obj->addr.init, obj->addr.rar, 
                obj->addr.als_cmd, obj->addr.als_dat0, obj->addr.als_dat1,
                obj->addr.ps_cmd, obj->addr.ps_dat, obj->addr.ps_thd);
    } else {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
    }
    len += snprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02lX %02lX\n", 
           atomic_read(&obj->als_cmd_val), atomic_read(&obj->ps_cmd_val), atomic_read(&obj->ps_thd_val),
           obj->enable, obj->pending_intr);
    len += snprintf(buf+len, PAGE_SIZE-len, "EINT: %d (%d %d %d %d)\n", 
           mt_get_gpio_in(GPIO_ALS_EINT_PIN),
           CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_DEBOUNCE_CN);
    len += snprintf(buf+len, PAGE_SIZE-len, "GPIO: %d (%d %d %d %d)\n", 
           GPIO_ALS_EINT_PIN, mt_get_gpio_dir(GPIO_ALS_EINT_PIN), mt_get_gpio_mode(GPIO_ALS_EINT_PIN), 
           mt_get_gpio_pull_enable(GPIO_ALS_EINT_PIN), mt_get_gpio_pull_select(GPIO_ALS_EINT_PIN));
    len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&obj->als_suspend), atomic_read(&obj->ps_suspend));
    return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_i2c(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct cm3623_priv *obj;    
    ssize_t len = 0;
    u32 base = I2C3_BASE;
    
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    len += snprintf(buf+len, PAGE_SIZE-len, "DATA_PORT      = 0x%08X\n", __raw_readl(mt6516_I2C_DATA_PORT    ));
    len += snprintf(buf+len, PAGE_SIZE-len, "SLAVE_ADDR     = 0x%08X\n", __raw_readl(mt6516_I2C_SLAVE_ADDR));
    len += snprintf(buf+len, PAGE_SIZE-len, "INTR_MASK      = 0x%08X\n", __raw_readl(mt6516_I2C_INTR_MASK));
    len += snprintf(buf+len, PAGE_SIZE-len, "INTR_STAT      = 0x%08X\n", __raw_readl(mt6516_I2C_INTR_STAT));
    len += snprintf(buf+len, PAGE_SIZE-len, "CONTROL        = 0x%08X\n", __raw_readl(mt6516_I2C_CONTROL));
    len += snprintf(buf+len, PAGE_SIZE-len, "TRANSFER_LEN   = 0x%08X\n", __raw_readl(mt6516_I2C_TRANSFER_LEN));
    len += snprintf(buf+len, PAGE_SIZE-len, "TRANSAC_LEN    = 0x%08X\n", __raw_readl(mt6516_I2C_TRANSAC_LEN));
    len += snprintf(buf+len, PAGE_SIZE-len, "DELAY_LEN      = 0x%08X\n", __raw_readl(mt6516_I2C_DELAY_LEN));
    len += snprintf(buf+len, PAGE_SIZE-len, "TIMING         = 0x%08X\n", __raw_readl(mt6516_I2C_TIMING));
    len += snprintf(buf+len, PAGE_SIZE-len, "START          = 0x%08X\n", __raw_readl(mt6516_I2C_START));
    len += snprintf(buf+len, PAGE_SIZE-len, "FIFO_STAT      = 0x%08X\n", __raw_readl(mt6516_I2C_FIFO_STAT));
    len += snprintf(buf+len, PAGE_SIZE-len, "FIFO_THRESH    = 0x%08X\n", __raw_readl(mt6516_I2C_FIFO_THRESH));
    len += snprintf(buf+len, PAGE_SIZE-len, "FIFO_ADDR_CLR  = 0x%08X\n", __raw_readl(mt6516_I2C_FIFO_ADDR_CLR));
    len += snprintf(buf+len, PAGE_SIZE-len, "IO_CONFIG      = 0x%08X\n", __raw_readl(mt6516_I2C_IO_CONFIG));
    len += snprintf(buf+len, PAGE_SIZE-len, "DEBUG          = 0x%08X\n", __raw_readl(mt6516_I2C_DEBUG));
    len += snprintf(buf+len, PAGE_SIZE-len, "HS             = 0x%08X\n", __raw_readl(mt6516_I2C_HS));
    len += snprintf(buf+len, PAGE_SIZE-len, "DEBUGSTAT      = 0x%08X\n", __raw_readl(mt6516_I2C_DEBUGSTAT));
    len += snprintf(buf+len, PAGE_SIZE-len, "DEBUGCTRL      = 0x%08X\n", __raw_readl(mt6516_I2C_DEBUGCTRL));    
    return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_store_i2c(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct cm3623_priv *obj;
    int sample_div, step_div;
    unsigned long tmp;
    u32 base = I2C3_BASE;    

    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    } else if (2 != sscanf(buf, "%d %d", &sample_div, &step_div)) {
        CMC_ERR("invalid format: '%s'\n", buf);
        return 0;
    }
    tmp  = __raw_readw(mt6516_I2C_TIMING) & ~((0x7 << 8) | (0x1f << 0));
    tmp  = (sample_div & 0x7) << 8 | (step_div & 0x1f) << 0 | tmp;
    __raw_writew(tmp, mt6516_I2C_TIMING);        
    return count;
}
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct cm3623_priv *obj, const char* buf, size_t count,
                             u32 data[], int len)
{
    int idx = 0;
    char *cur = (char*)buf, *end = (char*)(buf+count);

    while (idx < len) {
        while ((cur < end) && IS_SPACE(*cur)) cur++;        
        if (1 != sscanf(cur, "%d", &data[idx]))
            break;
        idx++; 
        while ((cur < end) && !IS_SPACE(*cur)) cur++;
    }
    return idx;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_alslv(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    struct cm3623_priv *obj;
    ssize_t len = 0;
    int idx;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    for (idx = 0; idx < obj->als_level_num; idx++)
        len += snprintf(buf+len, PAGE_SIZE-len, "%d ", obj->hw->als_level[idx]);
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");
    return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_store_alslv(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct cm3623_priv *obj;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
    } else if (!strcmp(buf, "def")) {
        memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
    } else if (obj->als_level_num != read_int_from_buf(obj, buf, count, obj->hw->als_level, obj->als_level_num)) {
        CMC_ERR("invalid format: '%s'\n", buf);
    }    
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_show_alsval(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    struct cm3623_priv *obj;
    ssize_t len = 0;
    int idx;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
        return 0;
    }
    for (idx = 0; idx < obj->als_value_num; idx++)
        len += snprintf(buf+len, PAGE_SIZE-len, "%d ", obj->hw->als_value[idx]);
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");
    return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm3623_store_alsval(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct cm3623_priv *obj;
    if (!dev) {
        CMC_ERR("dev is null!!\n");
    } else if (!(obj = (struct cm3623_priv*)dev_get_drvdata(dev))) {
        CMC_ERR("drv data is null!!\n");
    } else if (!strcmp(buf, "def")) {
        memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
    } else if (obj->als_value_num != read_int_from_buf(obj, buf, count, obj->hw->als_value, obj->als_value_num)) {
        CMC_ERR("invalid format: '%s'\n", buf);
    }    
    return count;
}
/*----------------------------------------------------------------------------*/
static DEVICE_ATTR(als,     S_IWUSR | S_IRUGO, cm3623_show_als,   NULL);
static DEVICE_ATTR(ps,      S_IWUSR | S_IRUGO, cm3623_show_ps,    NULL);
static DEVICE_ATTR(config,  S_IWUSR | S_IRUGO, cm3623_show_config,cm3623_store_config);
static DEVICE_ATTR(alslv,   S_IWUSR | S_IRUGO, cm3623_show_alslv, cm3623_store_alslv);
static DEVICE_ATTR(alsval,  S_IWUSR | S_IRUGO, cm3623_show_alsval,cm3623_store_alsval);
static DEVICE_ATTR(trace,   S_IWUSR | S_IRUGO, cm3623_show_trace, cm3623_store_trace);
static DEVICE_ATTR(status,  S_IWUSR | S_IRUGO, cm3623_show_status,  NULL);
static DEVICE_ATTR(send,    S_IWUSR | S_IRUGO, cm3623_show_send,  cm3623_store_send);
static DEVICE_ATTR(recv,    S_IWUSR | S_IRUGO, cm3623_show_recv,  cm3623_store_recv);
static DEVICE_ATTR(reg,     S_IWUSR | S_IRUGO, cm3623_show_reg,   NULL);
static DEVICE_ATTR(i2c,     S_IWUSR | S_IRUGO, cm3623_show_i2c,   cm3623_store_i2c);
/*----------------------------------------------------------------------------*/
static struct device_attribute *cm3623_attr_list[] = {
    &dev_attr_als,
    &dev_attr_ps,    
    &dev_attr_trace,        /*trace log*/
    &dev_attr_config,
    &dev_attr_alslv,
    &dev_attr_alsval,
    &dev_attr_status,
    &dev_attr_send,
    &dev_attr_recv,
    &dev_attr_i2c,
    &dev_attr_reg,
};
/*----------------------------------------------------------------------------*/
static int cm3623_create_attr(struct device *dev) 
{
    int idx, err = 0;
    int num = (int)(sizeof(cm3623_attr_list)/sizeof(cm3623_attr_list[0]));
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) {
        if ((err = device_create_file(dev, cm3623_attr_list[idx]))) {            
            CMC_ERR("device_create_file (%s) = %d\n", cm3623_attr_list[idx]->attr.name, err);        
            break;
        }
    }    
    return err;
}
/*----------------------------------------------------------------------------*/
static int cm3623_delete_attr(struct device *dev)
{
    int idx ,err = 0;
    int num = (int)(sizeof(cm3623_attr_list)/sizeof(cm3623_attr_list[0]));
    
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) 
        device_remove_file(dev, cm3623_attr_list[idx]);

    return err;
}
static int cm3623_get_als_value(struct cm3623_priv *obj, u16 als)
{
    int idx;
    int invalid = 0;
    for (idx = 0; idx < obj->als_level_num; idx++) {
        if (als < obj->hw->als_level[idx])
            break;
    }
    if (idx >= obj->als_value_num) {
        CMC_ERR("exceed range\n"); 
        idx = obj->als_value_num - 1;
    }
    if (1 == atomic_read(&obj->als_deb_on)) {
        unsigned long endt = atomic_read(&obj->als_deb_end);
        if (time_after(jiffies, endt))
            atomic_set(&obj->als_deb_on, 0);
        if (1 == atomic_read(&obj->als_deb_on))
            invalid = 1;
    }

    if (!invalid) {
        if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
            CMC_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
        return obj->hw->als_value[idx];
    } else {
        if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
            CMC_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
        return -1;
    }
}
/*----------------------------------------------------------------------------*/
static int cm3623_get_ps_value(struct cm3623_priv *obj, u16 ps)
{
    int val, mask = atomic_read(&obj->ps_mask);
    int invalid = 0;
    
    if (ps > atomic_read(&obj->ps_thd_val))
        val = 0;  /*close*/
    else
        val = 1;  /*far away*/

    if (atomic_read(&obj->ps_suspend)) {
        invalid = 1;
    } else if (1 == atomic_read(&obj->ps_deb_on)) {
        unsigned long endt = atomic_read(&obj->ps_deb_end);
        if (time_after(jiffies, endt))
            atomic_set(&obj->ps_deb_on, 0);
        if (1 == atomic_read(&obj->ps_deb_on))
            invalid = 1;
    }

    if (!invalid) {
        if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS)) {
            if (mask)
                CMC_DBG("PS:  %05d => %05d [M] \n", ps, val);
            else
                CMC_DBG("PS:  %05d => %05d\n", ps, val);
        }
        return val;
    } else {
        if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
            CMC_DBG("PS:  %05d => %05d (-1)\n", ps, val);    
        return -1;
    }
    /*always return far away if mask*/
    return (mask) ? (1) : (val);
}
static int cm3623_open(struct inode *inode, struct file *file)
{
    file->private_data = cm3623_i2c_client;
    
    if (!file->private_data) {
        CMC_ERR("null pointer!!\n");
        return -EINVAL;
    }
    return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int cm3623_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}
/*----------------------------------------------------------------------------*/
static int cm3623_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
       unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
    struct cm3623_priv *obj = i2c_get_clientdata(client);  
    int err = 0;
    void __user *ptr = (void __user*) arg;
    struct hwmsen_data dat;
    uint32_t enable;
        
    switch (cmd) {
        case HWM_IOCS_PRO_MODE:
            if (copy_from_user(&enable, ptr, sizeof(enable))) {
                err = -EFAULT;
                goto err_out;
            }
            if (enable) {
                if ((err = cm3623_enable_ps(&obj->client, 1))) {
                    CMC_ERR("enable ps fail: %d\n", err); 
                    goto err_out;
                }
                set_bit(CMC_BIT_PS, &obj->enable);
            } else {
                if ((err = cm3623_enable_ps(&obj->client, 0))) {
                    CMC_ERR("disable ps fail: %d\n", err); 
                    goto err_out;
                }
                clear_bit(CMC_BIT_PS, &obj->enable);
            }
            break;

        case HWM_IOCG_PRO_MODE:
            enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
            if (copy_to_user(ptr, &enable, sizeof(enable))) {
                err = -EFAULT;
                goto err_out;
            }
            break;

        case HWM_IOCG_PRO_DATA:    
            if ((err = cm3623_read_ps(&obj->client, &obj->ps)))
                goto err_out;
            dat.raw[HWM_EVT_DISTANCE] = cm3623_get_ps_value(obj, obj->ps);
            dat.num = 0;
            if (copy_to_user(ptr, &dat, sizeof(dat))) {
                err = -EFAULT;
                goto err_out;
            }  
            break;

        case HWM_IOCG_PRO_RAW:    
            if ((err = cm3623_read_ps(&obj->client, &obj->ps)))
                goto err_out;
            dat.raw[HWM_EVT_DISTANCE] = obj->ps;
            dat.num = 0;
            if (copy_to_user(ptr, &dat, sizeof(dat))) {
                err = -EFAULT;
                goto err_out;
            }  
            break;            
                
        case HWM_IOCS_LIG_MODE:
            if (copy_from_user(&enable, ptr, sizeof(enable))) {
                err = -EFAULT;
                goto err_out;
            }
            if (enable) {
                if ((err = cm3623_enable_als(&obj->client, 1))) {
                    CMC_ERR("enable als fail: %d\n", err); 
                    goto err_out;
                }
                set_bit(CMC_BIT_ALS, &obj->enable);
            } else {
                if ((err = cm3623_enable_als(&obj->client, 0))) {
                    CMC_ERR("disable als fail: %d\n", err); 
                    goto err_out;
                }
                clear_bit(CMC_BIT_ALS, &obj->enable);
            }
            break;
            
        case HWM_IOCG_LIG_MODE:
            enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
            if (copy_to_user(ptr, &enable, sizeof(enable))) {
                err = -EFAULT;
                goto err_out;
            }
            break;
            
        case HWM_IOCG_LIG_DATA: 
            if ((err = cm3623_read_als(&obj->client, &obj->als)))
                goto err_out;
            dat.raw[HWM_EVT_LIGHT] = cm3623_get_als_value(obj, obj->als);
            dat.num = 0;
            if (copy_to_user(ptr, &dat, sizeof(dat))) {
                err = -EFAULT;
                goto err_out;
            }              
            break;
            
        case HWM_IOCG_LIG_RAW:    
            if ((err = cm3623_read_als(&obj->client, &obj->als)))
                goto err_out;
            dat.raw[HWM_EVT_LIGHT] = obj->als;
            dat.num = 0;
            if (copy_to_user(ptr, &dat, sizeof(dat))) {
                err = -EFAULT;
                goto err_out;
            }              
            break;
            
        default:
            CMC_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
            err = -ENOIOCTLCMD;
            break;
    }

err_out:
    return err;    
}
/*----------------------------------------------------------------------------*/
static struct file_operations cm3623_fops = {
    .owner = THIS_MODULE,
    .open = cm3623_open,
    .release = cm3623_release,
    .ioctl = cm3623_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice cm3623_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "cm3623",
    .fops = &cm3623_fops,
};
/*----------------------------------------------------------------------------*/
static int cm3623_suspend(struct i2c_client *client, pm_message_t msg) 
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);    
    int err;
    CMC_FUN();    

    if (msg.event == PM_EVENT_SUSPEND) {   
        if (!obj) {
            CMC_ERR("null pointer!!\n");
            return -EINVAL;
        }
        atomic_set(&obj->als_suspend, 1);
        if ((err = cm3623_enable_als(client, 0))) {
            CMC_ERR("disable als: %d\n", err);
            return err;
        }

        atomic_set(&obj->ps_suspend, 1);
        if ((err = cm3623_enable_ps(client, 0))) {
            CMC_ERR("disable ps:  %d\n", err);
            return err;
        }
        cm3623_power(obj->hw, 0);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int cm3623_resume(struct i2c_client *client)
{
    struct cm3623_priv *obj = i2c_get_clientdata(client);        
    int err;
    CMC_FUN();

    if (!obj) {
        CMC_ERR("null pointer!!\n");
        return -EINVAL;
    }
    
    cm3623_power(obj->hw, 1);
    if ((err = cm3623_init_client(client))) {
        CMC_ERR("initialize client fail!!\n");
        return err;        
    }
    atomic_set(&obj->als_suspend, 0);
    if (test_bit(CMC_BIT_ALS, &obj->enable)) {
        if ((err = cm3623_enable_als(client, 1)))
            CMC_ERR("enable als fail: %d\n", err);        
    }
    atomic_set(&obj->ps_suspend, 0);
    if (test_bit(CMC_BIT_PS,  &obj->enable)) {
        if ((err = cm3623_enable_ps(client, 1)))
            CMC_ERR("enable ps fail: %d\n", err);                
    }
        
    return 0;
}
/*----------------------------------------------------------------------------*/
static void cm3623_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
    struct cm3623_priv *obj = container_of(h, struct cm3623_priv, early_drv);   
    int err;
    CMC_FUN();    

    if (!obj) {
        CMC_ERR("null pointer!!\n");
        return;
    }
    atomic_set(&obj->als_suspend, 1);    
    if ((err = cm3623_enable_als(&obj->client, 0))) {
        CMC_ERR("disable als fail: %d\n", err); 
    }
}
/*----------------------------------------------------------------------------*/
static void cm3623_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
    struct cm3623_priv *obj = container_of(h, struct cm3623_priv, early_drv);         
    int err;
    CMC_FUN();

    if (!obj) {
        CMC_ERR("null pointer!!\n");
        return;
    }
    
    atomic_set(&obj->als_suspend, 0);
    if (test_bit(CMC_BIT_ALS, &obj->enable)) {
        if ((err = cm3623_enable_als(&obj->client, 1)))
            CMC_ERR("enable als fail: %d\n", err);        
    }
}
/*----------------------------------------------------------------------------*/
static int cm3623_i2c_attach_adapter(struct i2c_adapter *adapter)
{   
    struct alsps_hw *hw = get_cust_alsps_hw();
    if (adapter->id == hw->i2c_num)         
        return i2c_probe(adapter, &cm3623_addr, cm3623_i2c_detect);
   
    return -1;
}
/*----------------------------------------------------------------------------*/
static int cm3623_i2c_detect(struct i2c_adapter *adapter, int address, int kind)
{
    struct i2c_client *client;
    struct cm3623_priv *obj;
    int err = 0;

    if (!(obj = kzalloc(sizeof(*obj), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }
    memset(obj, 0, sizeof(*obj));

    obj->hw = get_cust_alsps_hw();
    cm3623_get_addr(obj->hw, &obj->addr);

    INIT_WORK(&obj->eint_work, cm3623_eint_work);
    client = &obj->client;
    i2c_set_clientdata(client, obj);
    client->addr = address;
    client->adapter = adapter;
    client->driver = &cm3623_i2c_driver;
    client->flags = 0;
    atomic_set(&obj->als_debounce, 2000);
    atomic_set(&obj->als_deb_on, 0);
    atomic_set(&obj->als_deb_end, 0);
    atomic_set(&obj->ps_debounce, 1000);
    atomic_set(&obj->ps_deb_on, 0);
    atomic_set(&obj->ps_deb_end, 0);
    atomic_set(&obj->ps_mask, 0);
    atomic_set(&obj->trace, 0x00);
    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->als_cmd_val, 0xDF);
    atomic_set(&obj->ps_cmd_val,  0xC1);
    atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
    obj->enable = 0;
    obj->pending_intr = 0;
    obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
    obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
    BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
    memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
    BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
    memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
    atomic_set(&obj->i2c_retry, 3);
    if (!(atomic_read(&obj->als_cmd_val) & SD_ALS))
        set_bit(CMC_BIT_ALS, &obj->enable);
    if (!(atomic_read(&obj->ps_cmd_val) & SD_PS))
        set_bit(CMC_BIT_PS, &obj->enable);

    strlcpy(client->name, "cm3623", I2C_NAME_SIZE);
    cm3623_i2c_client = client;
        
    if ((err = i2c_attach_client(client)))
        goto exit_kfree;

    if ((err = cm3623_init_client(client))) 
        goto exit_init_failed;

    if ((err = misc_register(&cm3623_device))) {
        CMC_ERR("cm3623_device register failed\n");
        goto exit_misc_device_register_failed;
    }

    if ((err = cm3623_create_attr(&client->dev))) {
        CMC_ERR("create attribute err = %d\n", err);
        goto exit_create_attr_failed;
    }


#if defined(CONFIG_HAS_EARLYSUSPEND)
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    obj->early_drv.suspend  = cm3623_early_suspend,
    obj->early_drv.resume   = cm3623_late_resume,    
    register_early_suspend(&obj->early_drv);
#endif 
    CMC_LOG("%s: OK\n", __func__);
    return 0;

exit_create_attr_failed:
    misc_deregister(&cm3623_device);
exit_misc_device_register_failed:
exit_init_failed:
    i2c_detach_client(client);
exit_kfree:
    kfree(obj);
exit:
    cm3623_i2c_client = NULL;           
    MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
    CMC_ERR("%s: err = %d\n", __func__, err);
    return err;
}
/*----------------------------------------------------------------------------*/
static int cm3623_i2c_detach_client(struct i2c_client *client)
{
    int err;

    if ((err = i2c_detach_client(client))) 
        CMC_ERR("i2c_detach_client:%d\n", err);
    if ((err = cm3623_delete_attr(&client->dev))) 
        CMC_ERR("cm3623_delete_attr fail: %d\n", err);
    if ((err = misc_deregister(&cm3623_device)))
        CMC_ERR("misc_deregister fail: %d\n", err);    

    cm3623_i2c_client = NULL;
    kfree(i2c_get_clientdata(client));
    return 0;
}
/*----------------------------------------------------------------------------*/
static int cm3623_probe(struct platform_device *pdev) 
{
    struct alsps_hw *hw = get_cust_alsps_hw();
    struct cm3623_i2c_addr addr;

    cm3623_power(hw, 1);    
    cm3623_get_addr(hw, &addr);
    normal_i2c[0] = addr.init;
    if (i2c_add_driver(&cm3623_i2c_driver)) {
        CMC_ERR("add driver error\n");
        return -1;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static int cm3623_remove(struct platform_device *pdev)
{
    struct alsps_hw *hw = get_cust_alsps_hw();
    CMC_FUN();    
    cm3623_power(hw, 0);    
    i2c_del_driver(&cm3623_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver cm3623_alsps_driver = {
    .probe      = cm3623_probe,
    .remove     = cm3623_remove,    
    .driver     = {
        .name  = "cm3623",
        .owner = THIS_MODULE,
     }
};
/*----------------------------------------------------------------------------*/
static int __init cm3623_init(void)
{
    CMC_FUN();
    if (platform_driver_register(&cm3623_alsps_driver)) {
        CMC_ERR("failed to register driver");
        return -ENODEV;
    }
    return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit cm3623_exit(void)
{
    CMC_FUN();
    platform_driver_unregister(&cm3623_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(cm3623_init);
module_exit(cm3623_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("MingHsien Hsieh");
MODULE_DESCRIPTION("ADXL345 accelerometer driver");
MODULE_LICENSE("GPL");
