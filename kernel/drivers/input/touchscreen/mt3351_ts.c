

#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/types.h>
#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_typedefs.h>
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_auxadc_hw.h>
#include <mach/mt3351_pmu_hw.h>
#include <mach/mt3351_pmu_sw.h>
#include <mach/irqs.h>
#include <asm/io.h>

//#define TS_DEBUG

#define AUX_TS_DEBT           (AUXADC_BASE + 0x0038)
#define AUX_TS_CMD            (AUXADC_BASE + 0x003C)
#define AUX_TS_CON            (AUXADC_BASE + 0x0040)
#define AUX_TS_DATA0          (AUXADC_BASE + 0x0044)

#define TS_DEBT_MASK          0x3fff

#define TS_CMD_PD_MASK        0x0003
#define TS_CMD_PD_YDRV_SH     0x0000
#define TS_CMD_PD_IRQ_SH      0x0001
#define TS_CMD_PD_IRQ         0x0003
#define TS_CMD_SE_DF_MASK     0x0004
#define TS_CMD_DIFFERENTIAL   0x0000
#define TS_CMD_SINGLE_END     0x0004
#define TS_CMD_MODE_MASK      0x0008
#define TS_CMD_MODE_10BIT     0x0000
#define TS_CMD_MODE_8BIT      0x0008
#define TS_CMD_ADDR_MASK      0x0070
#define TS_CMD_ADDR_Y         0x0010
#define TS_CMD_ADDR_Z1        0x0030
#define TS_CMD_ADDR_Z2        0x0040
#define TS_CMD_ADDR_X         0x0050

#define TS_CON_SPL_MASK       0x0001
#define TS_CON_SPL_TRIGGER    0x0001
#define TS_CON_STATUS_MASK    0x0002

#define TS_DAT0_DAT_MASK      0x03ff

#define MTK_TS_NAME           "MT3351_TS"
#define MTK_TS_DRIVER_NAME    "MT3351_TS_DRIVER"
#define BUFFER_SIZE           5
#define TDELAY                (2 * HZ/100)
#define DEFAULT_TS_DEBOUNCE_TIME (3*32) /* 20ms */

u32 tspendown;

typedef enum
{
    TS_FALSE = 0,
    TS_TRUE,
}TS_BOOL;

struct ts_event
{
    u16 x;
    u16 y;
    u16 pressure;
};

struct MT3351_TS_DEVICE
{
    struct input_dev *dev;
    struct timer_list timer;
    struct tasklet_struct tasklet;
    struct ts_event event_buffer[BUFFER_SIZE]; /* ring buffer */
    int head, tail; /* head and tail of event_buffer[] */
    TS_BOOL pen_down;
};

static u16 ts_read_adc(u16 pos);
static int __init MT3351_ts_device_init(void);
static void __exit MT3351_ts_device_exit(void);
static irqreturn_t MT3351_ts_interrupt_handler(int irq, void *dev_id);
static void MT3351_ts_tasklet(unsigned long unused);
static void MT3351_ts_timer_fn(unsigned long arg);

static struct MT3351_TS_DEVICE *ts;

static const int sampleSetting = TS_CMD_DIFFERENTIAL | TS_CMD_MODE_10BIT | TS_CMD_PD_YDRV_SH;
#ifndef CONFIG_MT3351_EVZ_BOARD
static const long cal[7] = {32701, 257, -1630528, -155, 19737, -1639754, 65536};
#else
static const long cal[7] = {23481, 51, -1812820, -144, 19708, -2529340, 65536};
#endif

static const unsigned int weight[4][5] =
{
    { 1, 0, 0, 0, 0},
    { 5, 3, 0, 0, 3},
    { 8, 5, 3, 0, 4},
    { 6, 4, 3, 3, 4},
};
#ifdef CONFIG_MT3351_EVK_BOARD
    #define TR_MAX    1200
    #define TR_MIN     300
#else
    #define TR_MAX    1500
    #define TR_MIN     300
#endif


static u16 ts_read_adc(u16 pos)
{
   u16  reg;
   
   /* --- 1. Write sample command --- */
   
   reg = pos | sampleSetting;
   
   __raw_writew(reg, AUX_TS_CMD);

    /* --- 2. Trigger sample process --- */
    //PDN_Power_CONA_DOWN(PDN_PERI_TOUCH, KAL_FALSE);
    //2009/02/02, Kelvin modify for power management
    hwEnableClock(MT3351_CLOCK_TOUCHPAD);
   
   do{
	   __raw_writew(TS_CON_SPL_TRIGGER, AUX_TS_CON);
	        
	   /* --- 3. Sample process is done (may need to be modified) --- */
	   
	   /* SPL bit in AUX_TS_CON reset to 0 */
	   
	   while(TS_CON_SPL_MASK & __raw_readw(AUX_TS_CON));
   }while(__raw_readw(PMU_CON28) & ADC_INVALID);

    //2009/02/02, Kelvin modify for power management
    hwDisableClock(MT3351_CLOCK_TOUCHPAD);
   //PDN_Power_CONA_DOWN(PDN_PERI_TOUCH, KAL_TRUE);
   
   /* data is stored at AUX_TS_DATA0 */
   
   reg = __raw_readw(AUX_TS_DATA0); 
                       
   return (reg);
}

u32 mt3351_ts_enable_tasklet(void)
{
    tasklet_enable(&(ts->tasklet));
    return 0;
}
EXPORT_SYMBOL(mt3351_ts_enable_tasklet);

u32 mt3351_ts_disable_tasklet(void)
{
    tasklet_disable(&(ts->tasklet));
    return 0;
}
EXPORT_SYMBOL(mt3351_ts_disable_tasklet);

u32 mt3351_ts_schedule_tasklet(void)
{
    tasklet_schedule(&(ts->tasklet));
    return 0;
}
EXPORT_SYMBOL(mt3351_ts_schedule_tasklet);


static void MT3351_ts_tasklet(unsigned long unused)
{
    int x, y, pressure;
    u16 sumX, sumY;
    int cur, tmp, i;
    static u16 lastX = 0, lastY = 0, LCMLastX = 0, LCMLastY = 0;
    int xtemp, ytemp;
    
    /* 
     *  if the current status of the touch screen is touched, then add a 
     *  timer which calls MT3351_ts_timer_fn after tdelay jiffies.
     */
     
     static int px = 0, py = 0, dropReleasePt = 1;
     int diffX, diffY, valid, drop = 0;
    #ifdef TS_DEBUG
        printk("tasklet\n");
    #endif

    ts->pen_down = (__raw_readl(AUX_TS_CON) & TS_CON_STATUS_MASK) >> 1;

    tspendown = ts->pen_down;
    
    if(ts->pen_down == TS_TRUE)
     {
         /*
          * modify the expiration time to a latter moment and activate the timer
          * jiffies: current time since system has been booted up
          */
         u16 x, y;
         /* for software workaround (1) */
         u16 z1, z2, pr;
         u16 sumZ1, sumZ2, avgZ1, avgZ2;

         sumZ1 = 0;
         sumZ2 = 0;
         /* for software workaround (1) */

         sumX = 0;
         sumY = 0;
         i = 0;
         valid = 1;

         #ifdef TS_DEBUG
         printk("sample start\n");
         #endif
         while(i < 16)
         {
             x = ts_read_adc(TS_CMD_ADDR_X);
             y = ts_read_adc(TS_CMD_ADDR_Y);
             /* for software workaround (2) */
             z1 = ts_read_adc(TS_CMD_ADDR_Z1);
             z2 = ts_read_adc(TS_CMD_ADDR_Z2);
             /* for software workaround (2) */
             
             if(x == 0 && y == 1023)
             {
                 #ifdef TS_DEBUG
                 printk("invalid point!!\n");
                 #endif
                 valid = 0;
                 break;
             }
             #ifdef TS_DEBUG
             printk("x = %d, y = %d\n", x, y);
             #endif
             sumX += x;
             sumY += y;
             /* for software workaround (3) */
             sumZ1 += z1;
             sumZ2 += z2;
             /* for software workaround (3) */
             i++;
         }
         #ifdef TS_DEBUG
         printk("sample end\n");
         #endif

         x = sumX >> 4;
         y = sumY >> 4;

         /* for software workaround (4) */
         avgZ1 = sumZ1 >> 4;
         avgZ2 = sumZ2 >> 4;

         pr = (x + 1) * (avgZ2 - avgZ1) / (avgZ1 + 1);
         
         #ifdef TS_DEBUG
         printk("touch resistance = %d\n", pr);
         #endif
         
         if( pr < TR_MIN || pr > TR_MAX)
             valid = 0;
         
         /* for software workaround (4) */

         pressure = 255;

         /* Koshi: marked for the ts delay issue */

         mod_timer(&(ts->timer), jiffies + TDELAY);

         diffX = x - px;
         diffY = y - py;

         if(diffX < 0)
             diffX *= -1;
         if(diffY < 0)
             diffY *= -1;
      
         px = x;
         py = y;
         
         /* Koshi: FIXME ! for the noise */
         if(valid)
         {
             ts->event_buffer[ts->head].x = x;
             ts->event_buffer[ts->head].y = y;
             ts->event_buffer[ts->head].pressure = 255;
             dropReleasePt = 0;
         }
         else
         {
             #ifdef TS_DEBUG
	             printk("x = %d, y = %d, pressure = %d, INVALID\n", x, y, pressure);
		     #endif
             return;
         }
     }
     else 
     {
         /* Koshi: marked for the ts delay issue */
         del_timer(&(ts->timer));
         ts->event_buffer[ts->head].x = px;
         ts->event_buffer[ts->head].y = py;
         ts->event_buffer[ts->head].pressure = 0;

         px = 0;
         py = 0;

         if(dropReleasePt)
         {
             #ifdef TS_DEBUG
                 printk("release point dropped\n");
             #endif
             lastX = 0;
	         lastY = 0;
	         ts->tail = ts->head;
             return;
         }

     }  

     ts->head++;
     if(ts->head == BUFFER_SIZE)
         ts->head -= BUFFER_SIZE;
     if(ts->head == ts->tail){
         ts->tail++;
         if(ts->tail == BUFFER_SIZE)
            ts->tail -= BUFFER_SIZE;
     }

     /* original tasklet starts from here */

    cur = ts->head - 1;
    if(cur < 0)
        cur += BUFFER_SIZE;
    pressure = ts->event_buffer[cur].pressure;

    if(pressure == 0)
    {
        xtemp = lastX;
        ytemp = lastY;
        lastX = 0;
        lastY = 0;
        ts->tail = ts->head;
    }
    else
    {
        tmp = ts->tail;

        xtemp = ts->event_buffer[tmp].x;
        ytemp = ts->event_buffer[tmp].y;

        lastX = xtemp;
        lastY = ytemp;
        ts->tail++;
        if(ts->tail == BUFFER_SIZE)
            ts->tail -= BUFFER_SIZE;
    }

    x = (int)((cal[2] + cal[0]*xtemp + cal[1]*ytemp)/(cal[6]));
    y = (int)((cal[5] + cal[3]*xtemp + cal[4]*ytemp)/(cal[6]));

    if(pressure == 0){
        LCMLastX = 0;
        LCMLastY = 0;
    }
    else{
        diffX = x - LCMLastX;
        diffY = y - LCMLastY;

        if(diffX < 0)
            diffX *= -1;
        if(diffY < 0)
            diffY *= -1;

        if(diffX <= 2 && diffY <= 2)
            drop = 1;

        if(!drop){
            LCMLastX = x;
            LCMLastY = y;
        }
    }

    #ifdef CONFIG_MT3351_EVB_BOARD
    x = 479 - x;
    y = 271 - y;
    #endif

    /* clamping based on board type */
    #ifdef CONFIG_MT3351_EVZ_BOARD
    if(x < 0)
        x = 0;
    if(x > 319)
        x = 319;
    if(y < 0)
        y = 0;
    if(y > 239)
        y = 239;
    #else
    if(x < 0)
        x = 0;
    if(x > 479)
        x = 479;
    if(y < 0)
        y = 0;
    if(y > 271)
        y = 271;
    #endif

    if(pressure == 255){
        if(!drop){
            input_report_abs(ts->dev, ABS_X, x);
            input_report_abs(ts->dev, ABS_Y, y);
            input_report_abs(ts->dev, ABS_PRESSURE, pressure);
            input_report_key(ts->dev, BTN_TOUCH, 1);
        }
    }
    else{
        input_report_abs(ts->dev, ABS_PRESSURE, pressure);
        input_report_key(ts->dev, BTN_TOUCH, 0);
    }
    
    input_sync(ts->dev);

    #ifdef TS_DEBUG
        if(!drop){
    	    printk("x = %d, y = %d, pressure = %d\n", x, y, pressure);
    	    if(pressure == 0)
    	        printk("\n");
        }
        else{
            printk("point dropped!!\n");
        }
    #endif

    if(pressure == 0)
        dropReleasePt = 1;

    return;
    
}


static void MT3351_ts_timer_fn(unsigned long arg)
{
     #ifdef TS_DEBUG
     printk("timer interrupt\n");
     printk("before schedule timer tasklet, status = %lu\n", ts->tasklet.state);
     #endif
     if(ts->tasklet.state != TASKLET_STATE_RUN){
         tasklet_hi_schedule(&(ts->tasklet));
     }
     #ifdef TS_DEBUG
     else{         
         printk("timer tasklet cancelled\n");
     }
     printk("after schedule timer tasklet\n");
     #endif
    return;
}


static irqreturn_t MT3351_ts_interrupt_handler(int irq, void *dev_id)
{
     #ifdef TS_DEBUG
         printk("PEN IRQ\n");
     #endif
     
     #ifdef TS_DEBUG
	     printk("before schedule pen tasklet, status = %lu\n", ts->tasklet.state);
	 #endif
	 if(ts->tasklet.state != TASKLET_STATE_RUN){
         tasklet_hi_schedule(&(ts->tasklet));
     }
     #ifdef TS_DEBUG
     else{
         printk("pen tasklet cancelled\n");
     }
     printk("after schedule pen tasklet\n");
     #endif
     
     return IRQ_HANDLED;
}


static int __init MT3351_ts_device_init(void)
{
    /* 
     * device initialization
     */
    u16 tmp;
    int err;
  
    __raw_writew(DEFAULT_TS_DEBOUNCE_TIME, AUX_TS_DEBT);
    
    /*
     * initialization of struct MT3351_TS_DEVICE ts
     */
    
    if(!(ts = (struct MT3351_TS_DEVICE*)kmalloc(sizeof(struct MT3351_TS_DEVICE), GFP_KERNEL)))
        return -1;
    
    memset(ts, 0, sizeof(struct MT3351_TS_DEVICE));

    ts->dev = input_allocate_device();
   	
	if (!ts->dev)
		return -ENOMEM;
	
    /*
     * struct input_dev dev initialization and registration
     */

    ts->dev->name = MTK_TS_NAME;
    set_bit(EV_ABS, ts->dev->evbit);
    set_bit(EV_KEY, ts->dev->evbit);
    set_bit(ABS_X, ts->dev->absbit);
    set_bit(ABS_Y, ts->dev->absbit);
    set_bit(ABS_PRESSURE, ts->dev->absbit);
    set_bit(BTN_TOUCH, ts->dev->keybit);
    ts->dev->absmax[ABS_X] = 479;
    ts->dev->absmin[ABS_X] = 0;
    ts->dev->absmax[ABS_Y] = 271;
    ts->dev->absmin[ABS_Y] = 0;
    ts->dev->absmax[ABS_PRESSURE] = 255;
    ts->dev->absmin[ABS_PRESSURE] = 0;

	err = input_register_device(ts->dev);
 
    /*
     * struct timer_list timer initialization
     */

    init_timer(&(ts->timer));
    ts->timer.function = MT3351_ts_timer_fn;

    /*
     * struct tasklet_struct initialization
     */

    tasklet_init(&(ts->tasklet), MT3351_ts_tasklet, 0);

    /* register IRQ line and ISR */

    request_irq(MT3351_IRQ_TS_CODE, MT3351_ts_interrupt_handler, 0, "MT3351_TS", NULL);

    //2009/02/02, Kelvin modify for power management
    hwEnableClock(MT3351_CLOCK_TOUCHPAD);
    //PDN_Power_CONA_DOWN(PDN_PERI_TOUCH, KAL_FALSE);

    tmp = __raw_readl(AUXADC_CON3);
    tmp |= 0x1000;
    __raw_writel(tmp, AUXADC_CON3);

    //2009/02/02, Kelvin modify for power management
    hwDisableClock(MT3351_CLOCK_TOUCHPAD);
    //PDN_Power_CONA_DOWN(PDN_PERI_TOUCH, KAL_TRUE);
    
    ts->pen_down = TS_FALSE;


    printk("MT3351 Touch Screen Initialized\n");
  
    return 0;
}


static void __exit MT3351_ts_device_exit(void)
{
    input_unregister_device(ts->dev);
    return;
}

module_init(MT3351_ts_device_init);
module_exit(MT3351_ts_device_exit);

