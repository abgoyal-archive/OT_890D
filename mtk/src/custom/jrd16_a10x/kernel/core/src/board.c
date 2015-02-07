
/* system header files */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/setup.h>

#include <mach/system.h>
#include <mach/board.h>
#include <mach/hardware.h>

#include <mach/mt6516_pll.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>

#include <mach/mt_bt.h>
#include <mt6516_kpd.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <cust_eint.h>

extern void mt6516_fixup(struct machine_desc*, struct tag*, char **, struct meminfo*);



extern void MT6516_EnableITCM(UINT32 ITCMAddr);
extern void rtc_bbpu_power_down(void);

extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens);
extern void MT6516_EINT_Registration(kal_uint8 eintno, kal_bool Dbounce_En, 
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void), 
                                     kal_bool auto_umask);

extern ssize_t mt6326_kpled_Disable(void);
extern ssize_t  mt6326_bl_Disable(void);
extern kal_bool pmic_chrdet_status(void);

/*=======================================================================*/
/* Board Specific Devices Power Management                               */
/*=======================================================================*/
/* MT5921 WIFI */
#define MT5921_WIFI_INT_PIN     (GPIO64)
#define MT5921_WIFI_CLK_PIN     (GPIO116)
#define MT5921_WIFI_RST_PIN     (GPIO133)

enum {
    BTWLAN_POWER_WLAN = 0,
    BTWLAN_POWER_BT,
    BTWLAN_POWER_NUM
};

enum _ENUM_FUNC_IDX{
    FUNC_IDX_DS = 0, //Daisy chain
    FUNC_IDX_WIFI,   // Wi-Fi 
    FUNC_IDX_NUM
};
static sdio_irq_handler_t mt5921_wifi_irq_handler = NULL;
static pm_message_t mt5921_wifi_pm_state = { .event = PM_EVENT_HIBERNATE };
static pm_callback_t mt5921_wifi_pm_cb = NULL;
static void *mt5921_wifi_pm_data = NULL;
static void *mt5921_wifi_irq_data = NULL;
static int mt5921_wifi_pm_late_cb = 0;
//static int btwlan_power = 0;
//BT and Wi-Fi default status is on
static int btwlan_module_power[BTWLAN_POWER_NUM]={1,0};
static DEFINE_MUTEX(btwlan_mutex);

typedef void (*btwlan_callback_t)(int funcIdx, int op);
static btwlan_callback_t comodule_status_handler = NULL;

//extern int comodule_status_handler(int funcIdx, int op);
static int mt5921_power_on(void);
static int mt5921_power_off(void);
static int btwlan_module_power_handler( int id, int op );

static void mt5921_wifi_disable_irq(void);

//for test, Add by Lichunhui
//#define MTK_MT6616

/*MT6616 BT+FM co-power support*/
#ifdef MTK_MT6616
enum {
    MT6616_BT = 0,
    MT6616_FM,
    MT6616_MAX
};

void mt6616_poweron(int idx);
void mt6616_poweroff(int idx);
static bool mt6616_module[MT6616_MAX] = {false};
static DEFINE_MUTEX(btfm_mutex);
#endif


int register_comodule_cb(void * cb)
{
    comodule_status_handler = (btwlan_callback_t)cb;
    return 0;
}
EXPORT_SYMBOL(register_comodule_cb);

int deregister_comodule_cb(void)
{
    comodule_status_handler = NULL;
    return 0;
}
EXPORT_SYMBOL(deregister_comodule_cb);

static int mt5921_power_on(void)
{
    printk(KERN_INFO "[mt5921_power_on] ++\n");
    
    printk(KERN_INFO "[mt5921_power_on] RST_PIN output 1\n");
    mt_set_gpio_mode(GPIO_WIFI_RST_PIN, GPIO_WIFI_RST_PIN_M_GPIO); /* GPIO mode */
    mt_set_gpio_dir(GPIO_WIFI_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_WIFI_RST_PIN, GPIO_OUT_ONE);

    printk(KERN_INFO "[mt5921_power_on] config INT_PIN\n");
    mt_set_gpio_mode(GPIO_WIFI_EINT_PIN, GPIO_WIFI_EINT_PIN_M_EINT); /* EINT5 */
    mt5921_wifi_disable_irq();
            
    printk(KERN_INFO "[mt5921_power_on] pmic power on\n");        
    hwPowerOn(MT6516_POWER_VBT, VOL_1800,"BTWLAN");
    hwPowerOn(MT6516_POWER_VWIFI2V8, VOL_3000,"BTWLAN");        
    hwPowerOn(MT6516_POWER_VWIFI3V3, VOL_3300,"BTWLAN");

    printk(KERN_INFO "[mt5921_power_on] enable 32k CLK\n");
    mt_set_clock_output(GPIO_WIFI_CLK_PIN_CLK, GPIO_WIFI_CLK_PIN_FREQ);
    mt_set_gpio_mode(GPIO_WIFI_CLK_PIN, GPIO_WIFI_CLK_PIN_M_CLK); /* CLK_OUT1 mode */
    
    printk(KERN_INFO "[mt5921_power_on] reset chip\n");
    mt_set_gpio_out(GPIO_WIFI_RST_PIN, GPIO_OUT_ZERO);
    msleep(10);
    mt_set_gpio_out(GPIO_WIFI_RST_PIN, GPIO_OUT_ONE);       
    
    return 0;
}

static int mt5921_power_off(void)
{
    printk(KERN_INFO "[mt5921_power_off] ++\n");
    
    mt5921_wifi_disable_irq();    
    
    printk(KERN_INFO "[mt5921_power_on] pmic power off\n");   
    hwPowerDown(MT6516_POWER_VWIFI2V8,"BTWLAN");    
    hwPowerDown(MT6516_POWER_VWIFI3V3,"BTWLAN");
    hwPowerDown(MT6516_POWER_VBT,"BTWLAN");
    
    printk(KERN_INFO "[mt5921_power_on] config CLK, INT, and RST pin\n");   
    mt_set_gpio_mode(GPIO_WIFI_CLK_PIN, GPIO_WIFI_CLK_PIN_M_GPIO);
    //mt_set_gpio_dir(GPIO_WIFI_CLK_PIN, GPIO_DIR_OUT);
    //mt_set_gpio_out(GPIO_WIFI_CLK_PIN, 0);
     
    mt_set_gpio_mode(GPIO_WIFI_EINT_PIN,GPIO_WIFI_EINT_PIN_M_GPIO); /* GPIO mode */
    mt_set_gpio_dir(GPIO_WIFI_RST_PIN, GPIO_DIR_IN);
        
    return 0;
}

static int btwlan_module_power_handler( int id, int op )
{  
    int ret = 0;
    pm_message_t state;
    int i;
    
    if( btwlan_module_power[id] == op ) {
        printk( KERN_INFO "[btwlan_module_power_handler] id %d already in %d\n",id,op );          
        return 0;	
    }    
    
    if( id == BTWLAN_POWER_BT ) {
    	  //BT operation
        if( op == 1 ) {
            // going to turn on BT
            if( btwlan_module_power[BTWLAN_POWER_WLAN] == 0 ) {
            	  // wifi is off
                // 1. power on mt5921
                mt5921_power_on();
                
                //2. call sd callback
                if( mt5921_wifi_pm_cb ) {
                  	state.event = PM_EVENT_USER_RESUME;
                    mt5921_wifi_pm_cb(state, mt5921_wifi_pm_data);
                    
                    //3. enable daisy chain
                    printk( KERN_INFO "[btwlan_module_power_handler] comodule_status_handler = 0x%08x\n",comodule_status_handler );        
                    for(  i=0; i<5; i++) {
                    	  if( comodule_status_handler == NULL )
                            msleep(100);
                        else {
                        	  comodule_status_handler(FUNC_IDX_DS, 1);
                        	  break;
                         }
                         printk( KERN_INFO "[btwlan_module_power_handler] comodule_status_handler = 0x%08x\n",comodule_status_handler );        
                    }
                        
                } else {
                        printk( KERN_ALERT "[btwlan_module_power_handler] no sd callback!!\n");            
                }   
                      
                btwlan_module_power[BTWLAN_POWER_BT] = 1;
                //ret = 0;	

            } else {
                //Wi-Fi is on, do nothing
                btwlan_module_power[BTWLAN_POWER_BT] = 1;
                //ret = 0;	
            }
        } else {
            // going to turn off BT
            if( btwlan_module_power[BTWLAN_POWER_WLAN] == 0 )	{
                //wifi is off
                
                //1. disable daisy chain                    
                if( comodule_status_handler )
                    comodule_status_handler(FUNC_IDX_DS, 0);
                    
                //2. call sd callback                    
                if( mt5921_wifi_pm_cb ) {
                  	state.event = PM_EVENT_USER_SUSPEND;
                    mt5921_wifi_pm_cb(state, mt5921_wifi_pm_data);
                } else {
                    printk( KERN_ALERT "[btwlan_module_power_handler] no sd callback!!\n");                          
                }    
                //3. power off mt5921
                mt5921_power_off();
                    
                btwlan_module_power[BTWLAN_POWER_BT] = 0;
                    
                //ret = 0;
            } else {
                //Wi-Fi is on, do nothing
                btwlan_module_power[BTWLAN_POWER_BT] = 0;
                //ret = 0;            	
            }
        }
    } else {
        //wifi operation
        if( op == 1 ) {
            // going to turn on wifi
            if( btwlan_module_power[BTWLAN_POWER_BT] == 0 ) {
            	  // BT is off
                // 1. power on mt5921
                mt5921_power_on();
                
                //2. call sd callback                
                if( mt5921_wifi_pm_cb ) {

                  	state.event = PM_EVENT_USER_RESUME;
                    mt5921_wifi_pm_cb(state, mt5921_wifi_pm_data);
                    
                    //3. load Wi-Fi function
                    printk( KERN_INFO "[btwlan_module_power_handler] comodule_status_handler = 0x%08x\n",comodule_status_handler );        
                    for(  i=0; i<5; i++) {
                    	  if( comodule_status_handler == NULL )
                            msleep(100);
                        else {
                            comodule_status_handler(FUNC_IDX_WIFI, 1);
                        	  break;
                         }
                         printk( KERN_INFO "[btwlan_module_power_handler] comodule_status_handler = 0x%08x\n",comodule_status_handler );        
                    }                      
                } else {
                    printk( KERN_ALERT "[btwlan_module_power_handler] no sd callback!!\n");                     
                }
                                    
                btwlan_module_power[BTWLAN_POWER_WLAN] = 1;
                //ret = 0;
            } else {
                //BT is on
                //1. disable daisy chain
                if( comodule_status_handler )
                    comodule_status_handler(FUNC_IDX_DS, 0); 
                                    
                //2. enable Wi-Fi function
                if( comodule_status_handler )
                    comodule_status_handler(FUNC_IDX_WIFI, 1); 
                                    
                btwlan_module_power[BTWLAN_POWER_WLAN] = 1;
                //ret = 0;
            }
        } else {
            // going to turn off wifi
            if( btwlan_module_power[BTWLAN_POWER_BT] == 0 )	{
                // BT is off
                
                //1. disable wifi function
                if( comodule_status_handler )
                    comodule_status_handler(FUNC_IDX_WIFI, 0);
                    
                //2. call sd callback                    
                if( mt5921_wifi_pm_cb ) {
                  	state.event = PM_EVENT_USER_SUSPEND;
                    mt5921_wifi_pm_cb(state, mt5921_wifi_pm_data);
                } else {
                    printk( KERN_ALERT "[btwlan_module_power_handler] no sd callback!!\n");                      
                }       
                
                //3. power off mt5921
                mt5921_power_off();
                
                btwlan_module_power[BTWLAN_POWER_WLAN] = 0;
                //ret = 0;	                
            } else {
                //BT is on, 
                //1. disable wifi function
                if( comodule_status_handler )
                    comodule_status_handler(FUNC_IDX_WIFI, 0);
                                 
                //2. enable daisy chain               	  
                if( comodule_status_handler )
                    comodule_status_handler(FUNC_IDX_DS, 1); 
                
                btwlan_module_power[BTWLAN_POWER_WLAN] = 0;
                //ret = 0;            	
            }
        }
    }
    printk(KERN_INFO "[BTWLAN] WLAN(%d), BT(%d)\n",
        btwlan_module_power[BTWLAN_POWER_WLAN], 
        btwlan_module_power[BTWLAN_POWER_BT]);
    
    return ret;
}

static void mt5921_wifi_enable_irq(void)
{
    MT6516_EINTIRQUnmask(5);
}

static void mt5921_wifi_disable_irq(void)
{
    MT6516_EINTIRQMask(5);
}

static void mt5921_wifi_eirq_handler(void)
{
    if (mt5921_wifi_irq_handler)
        mt5921_wifi_irq_handler(mt5921_wifi_irq_data);
}

static void mt5921_wifi_request_irq(sdio_irq_handler_t irq_handler, void *data)
{
    MT6516_EINT_Set_Sensitivity(CUST_EINT_MT5921_WIFI_NUM,
		                CUST_EINT_MT5921_WIFI_SENSITIVE);
    MT6516_EINT_Set_HW_Debounce(CUST_EINT_MT5921_WIFI_NUM,
		                CUST_EINT_MT5921_WIFI_DEBOUNCE_CN);
    MT6516_EINT_Registration(CUST_EINT_MT5921_WIFI_NUM,
		             CUST_EINT_MT5921_WIFI_DEBOUNCE_EN,
			     CUST_EINT_MT5921_WIFI_POLARITY,
			     mt5921_wifi_eirq_handler,
			     0);
    MT6516_EINTIRQMask(CUST_EINT_MT5921_WIFI_NUM);
    mt5921_wifi_irq_handler = irq_handler;
    mt5921_wifi_irq_data    = data;
}

void mt5921_wifi_register_pm(pm_callback_t pm_cb, void *data)
{
    /* register pm change callback */
    mt5921_wifi_pm_cb = pm_cb;
    mt5921_wifi_pm_data = data;
}

int mt_wifi_suspend(pm_message_t state)
{
    int evt = state.event;

    if (evt != PM_EVENT_USER_SUSPEND && evt != PM_EVENT_SUSPEND)
        return -1;

    mutex_lock(&btwlan_mutex);
    
    btwlan_module_power_handler( 0,0 );

    mutex_unlock(&btwlan_mutex);
    
    return 0;
}

int mt_wifi_resume(pm_message_t state)
{
    int evt = state.event;

    if (evt != PM_EVENT_USER_RESUME && evt != PM_EVENT_RESUME)
        return -1;

    mutex_lock(&btwlan_mutex);
    
    //printk(KERN_INFO "[WIFI] %s Resume\n", evt == PM_EVENT_RESUME ? "PM":"USR");

    btwlan_module_power_handler( 0,1 );

    mutex_unlock(&btwlan_mutex);

    return 0;
}

void mt_wifi_power_on(void)
{
    pm_message_t state = { .event = PM_EVENT_USER_RESUME };

    (void)mt_wifi_resume(state);
}

void mt_wifi_power_off(void)
{
    pm_message_t state = { .event = PM_EVENT_USER_SUSPEND };
    
    (void)mt_wifi_suspend(state);
}

EXPORT_SYMBOL(mt_wifi_power_on);
EXPORT_SYMBOL(mt_wifi_power_off);

void mt_bt_enable_irq(void)
{
    MT6516_EINTIRQUnmask(1);
}

void mt_bt_disable_irq(void)
{
    MT6516_EINTIRQMask(1);
}

static void bt_eirq_handler(void)
{
    mt_bt_eirq_handler(NULL);
}

static void mt_bt_request_irq(void *data)
{
    MT6516_EINT_Set_Sensitivity(CUST_EINT_BT_NUM, CUST_EINT_BT_SENSITIVE);
    MT6516_EINT_Set_HW_Debounce(CUST_EINT_BT_NUM, CUST_EINT_BT_DEBOUNCE_CN);
    MT6516_EINT_Registration(CUST_EINT_BT_NUM,
		             CUST_EINT_BT_DEBOUNCE_EN,
			     CUST_EINT_BT_POLARITY,
			     bt_eirq_handler,
			     0);
    MT6516_EINTIRQMask(1);
}

void mt_bt_power_on(void)
{   
    mutex_lock(&btwlan_mutex);
    printk(KERN_INFO "+mt_bt_power_on\n");
#ifdef MTK_MT6616
    mt6616_poweron(MT6616_BT);
#else  
    //btwlan_module_power_handler(BTWLAN_POWER_BT,1 );
    
    // UART GPIO Mode
    mt_set_gpio_mode(GPIO67, GPIO_MODE_01);
    mt_set_gpio_mode(GPIO68, GPIO_MODE_01);

    // BT GPIO Mode
    mt_set_gpio_mode(GPIO_PCM_DAICLK_PIN, GPIO_PCM_DAICLK_PIN_M_CLK);
    mt_set_gpio_mode(GPIO_PCM_DAIPCMOUT_PIN, GPIO_PCM_DAIPCMOUT_PIN_M_DAIPCMOUT);
    mt_set_gpio_mode(GPIO_PCM_DAIPCMIN_PIN, GPIO_PCM_DAIPCMIN_PIN_M_DAIPCMIN);
    mt_set_gpio_mode(GPIO_PCM_DAISYNC_PIN, GPIO_PCM_DAISYNC_PIN_M_DAISYNC);
    
    // EINT1
    mt_set_gpio_mode(GPIO_BT_EINT_PIN, GPIO_BT_EINT_PIN_M_GPIO);
    mt_set_gpio_pull_enable(GPIO_BT_EINT_PIN, 1);
    mt_set_gpio_pull_select(GPIO_BT_EINT_PIN, GPIO_PULL_DOWN);
    mt_set_gpio_mode(GPIO_BT_EINT_PIN, GPIO_BT_EINT_PIN_M_EINT);    
    
    mt_set_gpio_mode(GPIO_BT_CLK_PIN , GPIO_BT_CLK_PIN_M_CLK);    
    mt_set_clock_output(GPIO_BT_CLK_PIN_CLK, CLK_SRC_F32K);
    
    // Init BT_PWR_EN and BT_RESET    
    mt_set_gpio_mode(GPIO_BT_POWREN_PIN, GPIO_BT_POWREN_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_POWREN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_POWREN_PIN, 0);
    mt_set_gpio_mode(GPIO_BT_RESET_PIN, GPIO_BT_RESET_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_RESET_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_RESET_PIN, 0);
    
    mt_set_gpio_out(GPIO_BT_POWREN_PIN, 1);
    msleep(200);
    mt_set_gpio_out(GPIO_BT_RESET_PIN, 1);
    msleep(1000);
#endif
    
    printk(KERN_INFO "-mt_bt_power_on\n"); 
    mt_bt_request_irq(NULL);
    mutex_unlock(&btwlan_mutex);
}

void mt_bt_power_off(void)
{
    mutex_lock(&btwlan_mutex);
    printk(KERN_INFO "+mt_bt_power_off\n");
        
#ifdef MTK_MT6616
    mt6616_poweroff(MT6616_BT);
#else  
    mt_set_gpio_out(GPIO_BT_POWREN_PIN, 0);
    mt_set_gpio_out(GPIO_BT_RESET_PIN, 0);

    mt_set_gpio_mode(GPIO_BT_EINT_PIN, GPIO_BT_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_EINT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_EINT_PIN, 0);

    mt_set_gpio_mode(GPIO_BT_CLK_PIN, GPIO_BT_CLK_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_CLK_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_CLK_PIN, 0);

    mt_set_gpio_mode(GPIO67, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO67, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO67, 0);

    mt_set_gpio_mode(GPIO68, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO68, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO68, 0);

    mt_set_gpio_mode(GPIO_PCM_DAICLK_PIN, GPIO_PCM_DAICLK_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAICLK_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAICLK_PIN, 0);

    mt_set_gpio_mode(GPIO_PCM_DAIPCMOUT_PIN, GPIO_PCM_DAIPCMOUT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAIPCMOUT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAIPCMOUT_PIN, 0);

    mt_set_gpio_mode(GPIO_PCM_DAIPCMIN_PIN, GPIO_PCM_DAIPCMIN_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAIPCMIN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAIPCMIN_PIN, 0);

    mt_set_gpio_mode(GPIO_PCM_DAISYNC_PIN, GPIO_PCM_DAISYNC_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAISYNC_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAISYNC_PIN, 0);
    
    //btwlan_module_power_handler(BTWLAN_POWER_BT,0 );
#endif
    printk(KERN_INFO "-mt_bt_power_off\n");
    mutex_unlock(&btwlan_mutex);
}

EXPORT_SYMBOL(mt_bt_power_on);
EXPORT_SYMBOL(mt_bt_power_off);

#ifdef MTK_MT6616
void mt6616_poweron(int idx)
{
    int i = 0;
    mutex_lock(&btfm_mutex);  
    
    for(i = 0; i < MT6616_MAX; i++) {
        if(mt6616_module[i] != false) {
            goto end;
        }
    }
    
    //Daisy chain
    //btwlan_module_power_handler(BTWLAN_POWER_BT,1 );
    
    // UART GPIO Mode
    mt_set_gpio_mode(GPIO67, GPIO_MODE_01);
    mt_set_gpio_mode(GPIO68, GPIO_MODE_01);

    // BT GPIO Mode
    mt_set_gpio_mode(GPIO_PCM_DAICLK_PIN, GPIO_PCM_DAICLK_PIN_M_CLK);
    mt_set_gpio_mode(GPIO_PCM_DAIPCMOUT_PIN, GPIO_PCM_DAIPCMOUT_PIN_M_DAIPCMOUT);
    mt_set_gpio_mode(GPIO_PCM_DAIPCMIN_PIN, GPIO_PCM_DAIPCMIN_PIN_M_DAIPCMIN);
    mt_set_gpio_mode(GPIO_PCM_DAISYNC_PIN, GPIO_PCM_DAISYNC_PIN_M_DAISYNC);
    
    // EINT1
    mt_set_gpio_mode(GPIO_BT_EINT_PIN, GPIO_BT_EINT_PIN_M_GPIO);
    mt_set_gpio_pull_enable(GPIO_BT_EINT_PIN, 1);
    mt_set_gpio_pull_select(GPIO_BT_EINT_PIN, GPIO_PULL_DOWN);
    mt_set_gpio_mode(GPIO_BT_EINT_PIN, GPIO_BT_EINT_PIN_M_EINT);    
    
    mt_set_gpio_mode(GPIO_BT_CLK_PIN , GPIO_BT_CLK_PIN_M_CLK);    
    mt_set_clock_output(GPIO_BT_CLK_PIN_CLK, CLK_SRC_F32K);

    // Init BT_PWR_EN and BT_RESET    
    mt_set_gpio_mode(GPIO_BT_POWREN_PIN, GPIO_BT_POWREN_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_POWREN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_POWREN_PIN, 0);
    mt_set_gpio_mode(GPIO_BT_RESET_PIN, GPIO_BT_RESET_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_RESET_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_RESET_PIN, 0);
    
    mt_set_gpio_out(GPIO_BT_POWREN_PIN, 1);
    msleep(200);
    mt_set_gpio_out(GPIO_BT_RESET_PIN, 1);
    if(MT6616_BT == idx) { //FM first on not need delay
       msleep(1000);
    }else if(MT6616_FM == idx){
        msleep(500);
    }
    
end:
    mt6616_module[idx] = true;
    mutex_unlock(&btfm_mutex);
    printk(KERN_INFO "[BTFM]on, BT(%d), FM(%d)\n", mt6616_module[MT6616_BT], mt6616_module[MT6616_FM]);
}

EXPORT_SYMBOL(mt6616_poweron);

void mt6616_poweroff(int idx)
{
    int i = 0;    
    mutex_lock(&btfm_mutex);  
    mt6616_module[idx] = false;  
      
    for(i = 0; i < MT6616_MAX; i++) {
        if(mt6616_module[i] != false) {
            goto end;
        }
    }
    
    mt_set_gpio_out(GPIO_BT_POWREN_PIN, 0);
    mt_set_gpio_out(GPIO_BT_RESET_PIN, 0);

    mt_set_gpio_mode(GPIO_BT_EINT_PIN, GPIO_BT_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_EINT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_EINT_PIN, 0);

    mt_set_gpio_mode(GPIO_BT_CLK_PIN, GPIO_BT_CLK_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_BT_CLK_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_BT_CLK_PIN, 0);

    mt_set_gpio_mode(GPIO67, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO67, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO67, 0);

    mt_set_gpio_mode(GPIO68, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO68, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO68, 0);

    mt_set_gpio_mode(GPIO_PCM_DAICLK_PIN, GPIO_PCM_DAICLK_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAICLK_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAICLK_PIN, 0);

    mt_set_gpio_mode(GPIO_PCM_DAIPCMOUT_PIN, GPIO_PCM_DAIPCMOUT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAIPCMOUT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAIPCMOUT_PIN, 0);

    mt_set_gpio_mode(GPIO_PCM_DAIPCMIN_PIN, GPIO_PCM_DAIPCMIN_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAIPCMIN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAIPCMIN_PIN, 0);

    mt_set_gpio_mode(GPIO_PCM_DAISYNC_PIN, GPIO_PCM_DAISYNC_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_PCM_DAISYNC_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_PCM_DAISYNC_PIN, 0);
    
    // Close daisy chain if needed
    //btwlan_module_power_handler(BTWLAN_POWER_BT,0 );
end:
    mutex_unlock(&btfm_mutex);
    printk(KERN_INFO "[BTFM]off, BT(%d), FM(%d)\n", mt6616_module[MT6616_BT], mt6616_module[MT6616_FM]);
}

EXPORT_SYMBOL(mt6616_poweroff);
#endif


int mt_bt_suspend(pm_message_t state)
{
    return MT_BT_OK;
}

int mt_bt_resume(pm_message_t state)
{
    return MT_BT_OK;
}


/*-----------------------------------------------------------------------*/
/* GPS External Power Configuration                                      */
/*-----------------------------------------------------------------------*/
#define DEV_ERR(fmt, arg...)    printk(KERN_ERR "GPS: " "%s: " fmt, __FUNCTION__ ,##arg) 
/*-----------------------------------------------------------------------*/
int mt3326_gps_ext_power_on(int state)
{
    s32 err = 0;  

    if(state == 0)
    {
        /*configure as CLK_OUT4*/
        err = mt_set_gpio_mode(GPIO_GPS_CLK_PIN, GPIO_GPS_CLK_PIN_M_CLK);    
        if (RSUCCESS != err)
            DEV_ERR("set clock fails: %d\n", err);
        /*configure as output*/
        err = mt_set_gpio_dir(GPIO_GPS_CLK_PIN, GPIO_DIR_OUT);    
        if (RSUCCESS != err)
            DEV_ERR("set clock fails: %d\n", err);
        /*configure as 32K*/
        err = mt_set_clock_output(GPIO_GPS_CLK_PIN_CLK, GPIO_GPS_CLK_PIN_FREQ);        
        if (RSUCCESS != err)
            DEV_ERR("set clock fails: %d\n", err);

        /*configure as gpio function*/
        err = mt_set_gpio_mode(GPIO_GPS_RST_PIN, GPIO_GPS_RST_PIN_M_GPIO);
        if (RSUCCESS != err) 
            DEV_ERR("set HRST fails: %d\n", err);
        /*configure as output*/
        err = mt_set_gpio_dir(GPIO_GPS_RST_PIN, GPIO_DIR_OUT);    
        if (RSUCCESS != err)
            DEV_ERR("set HRST fails: %d\n", err);
        /*configure as 0, reset start*/
        err = mt_set_gpio_out(GPIO_GPS_RST_PIN, GPIO_OUT_ZERO);    
        if (RSUCCESS != err)
            DEV_ERR("set HRST fails: %d\n", err);
    }
    else if(state == 1)
    {
        msleep(150);

        /*configure as 1, reset end*/
        err = mt_set_gpio_out(GPIO_GPS_RST_PIN, GPIO_OUT_ONE);    
        if (RSUCCESS != err)
            DEV_ERR("set HRST fails: %d\n", err);

        msleep(10);
    }
    
    return err;
}

int mt3326_gps_ext_power_off(void)
{
    s32 err;
    err = mt_set_gpio_out(GPIO_GPS_RST_PIN, GPIO_OUT_ZERO);    
    if (RSUCCESS != err)
        DEV_ERR("set HRST fails: %d\n", err);
    return err;
}

/*=======================================================================*/
/* Board Specific Devices                                                */
/*=======================================================================*/


/*=======================================================================*/
/* Board Specific Devices Init                                           */
/*=======================================================================*/
__init int board_init(void)
{
    /* MT6516 board device init */
    mt6516_board_init();

    return 0;
}

/*=======================================================================*/
/* Board Devices Capability                                              */
/*=======================================================================*/
#if defined(CFG_DEV_MSDC1)
struct mt6516_sd_host_hw mt6516_sd1_hw = {
    .clk_src        = MSDC_CLKSRC_MCPLL,
    .cmd_edge       = EDGE_RISING,
    .data_edge      = EDGE_FALLING,
    .cmd_odc        = MSDC_ODC_4MA,
    .data_odc       = MSDC_ODC_4MA,
    .cmd_slew_rate  = MSDC_ODC_SLEW_FAST,
    .data_slew_rate = MSDC_ODC_SLEW_FAST,
    .data_pins      = CFG_MSDC1_DATA_PINS,
    .data_offset    = CFG_MSDC1_DATA_OFFSET,
    .flags          = MSDC_SYS_SUSPEND,
};
#endif
#if defined(CFG_DEV_MSDC2)
struct mt6516_sd_host_hw mt6516_sd2_hw = {
    .clk_src         = MSDC_CLKSRC_MCPLL,
    .cmd_edge        = EDGE_RISING,
    .data_edge       = EDGE_FALLING,
    .cmd_odc         = MSDC_ODC_4MA,
    .data_odc        = MSDC_ODC_4MA,
    .cmd_slew_rate   = MSDC_ODC_SLEW_FAST,
    .data_slew_rate  = MSDC_ODC_SLEW_FAST,
    .data_pins       = CFG_MSDC2_DATA_PINS,
    .data_offset     = CFG_MSDC2_DATA_OFFSET,
    .flags           = MSDC_EXT_SDIO_IRQ,
    .request_sdio_eirq = mt5921_wifi_request_irq,
    .enable_sdio_eirq  = mt5921_wifi_enable_irq,
    .disable_sdio_eirq = mt5921_wifi_disable_irq,
    .register_pm       = mt5921_wifi_register_pm,
};
#endif
#if defined(CFG_DEV_MSDC3)
struct mt6516_sd_host_hw mt6516_sd3_hw = {
    .clk_src         = MSDC_CLKSRC_MCPLL,
    .cmd_edge        = EDGE_RISING,
    .data_edge       = EDGE_FALLING,
    .cmd_odc         = MSDC_ODC_4MA,
    .data_odc        = MSDC_ODC_4MA,
    .cmd_slew_rate   = MSDC_ODC_SLEW_FAST,
    .data_slew_rate  = MSDC_ODC_SLEW_FAST,
    .data_pins       = CFG_MSDC3_DATA_PINS,
    .data_offset     = CFG_MSDC3_DATA_OFFSET,
    .flags           = MSDC_SYS_SUSPEND,
};
#endif

/*GPS driver*/
struct mt3326_gps_hardware mt3326_gps_hw = {
    .ext_power_on =  mt3326_gps_ext_power_on,
    .ext_power_off = mt3326_gps_ext_power_off,
};

/* Nand driver */
#if defined(CONFIG_MTD_NAND_MT6516)
struct mt6516_nand_host_hw mt6516_nand_hw = {
    .nfi_bus_width          = NFI_BUS_WIDTH,
	.nfi_access_timing		= NFI_ACCESS_TIMING,     
	.nfi_cs_num				= NFI_CS_NUM,
	.nand_sec_size			= NAND_SECTOR_SIZE,
	.nand_sec_shift			= NAND_SECTOR_SHIFT,
	.nand_ecc_size			= ECC_SIZE,
	.nand_ecc_bytes			= ECC_BYTES,
	.nand_ecc_mode			= NAND_ECC_MODE,
};
#endif

/*=======================================================================*/
/* Board Memory Mapping                                                  */
/*=======================================================================*/
static struct map_desc mt6516_io_desc[] __initdata =
{
	{
		.virtual	= IO_VIRT,
		.pfn		= __phys_to_pfn(IO_PHYS),
		.length		= IO_SIZE,
		.type		= MT_DEVICE
	},
	
	{
		.virtual	= (IO_VIRT+0x100000),
		.pfn		= __phys_to_pfn(IO_PHYS+0x100000),
		.length		= IO_SIZE,
		.type		= MT_DEVICE
	},

	{
        	.virtual    = (IO_VIRT+0x200000),
	        .pfn        = __phys_to_pfn(0x40000000),
        	.length     = IO_SIZE,
	        .type       = MT_DEVICE
	},

	{
        	.virtual    = (IO_VIRT+0x300000),
	        .pfn        = __phys_to_pfn(0x40000000+0x100000),
	        .length     = IO_SIZE,
	        .type       = MT_DEVICE
	},

	{
        	.virtual    = (IO_VIRT+0x400000),
	        .pfn        = __phys_to_pfn(0x50000000),
        	.length     = IO_SIZE,
	        .type       = MT_DEVICE
	},

 	{
	        .virtual    = (IO_VIRT+0x500000),
        	.pfn        = __phys_to_pfn(0x50000000+0x100000),
	        .length     = IO_SIZE,
        	.type       = MT_DEVICE
	},

	{
		.virtual	= CEVA_BASE,
		.pfn		= __phys_to_pfn(0xB1000000),/* CEVA Physical address */
		.length		= IO_SIZE,
		.type		= MT_DEVICE
	}

};

static void __init mt6516_map_io(void)
{

    MT6516_EnableITCM(0x50000000);

    iotable_init(mt6516_io_desc, ARRAY_SIZE(mt6516_io_desc));
}

/*=======================================================================*/
/* Board Low-Level Init                                                  */
/*=======================================================================*/
static void __init mt6516_init_pinmux(void)
{
#if defined(GPIO_INIT_DEBUG) || defined(GPIO_INIT_PINMUX)
    /* Keypad */
    mt_set_gpio_mode(23, 1);    //KROW5
    mt_set_gpio_mode(28, 1);    //KCOL5
    mt_set_gpio_mode(71, 1);    //KCOL7
    mt_set_gpio_mode(72, 1);    //KCOL6
    mt_set_gpio_mode(73, 1);    //KROW7
    mt_set_gpio_mode(74, 1);    //KROW6
    mt_set_gpio_mode(87, 1);    //KCOL0
    mt_set_gpio_mode(88, 1);    //KCOL1
    mt_set_gpio_mode(89, 1);    //KCOL2
    mt_set_gpio_mode(90, 1);    //KCOL3
    mt_set_gpio_mode(91, 1);    //KCOL4
    mt_set_gpio_mode(110, 1);   //KROW0
    mt_set_gpio_mode(111, 1);   //KROW1
    mt_set_gpio_mode(112, 1);   //KROW2
    mt_set_gpio_mode(113, 1);   //KROW3
    mt_set_gpio_mode(114, 1);   //KROW4

    // MT6516 KEYPAD GPIO Setting
    //      0   1   2   3   4   5   6   7   PULL
    // COL  87  88  89  90  91  28  72  71  H
    // ROW  110 111 112 113 114 23  74  73  L

    mt_set_gpio_pull_enable(87, TRUE);
    mt_set_gpio_pull_enable(88, TRUE);
    mt_set_gpio_pull_enable(89, TRUE);
    mt_set_gpio_pull_enable(90, TRUE);
    mt_set_gpio_pull_enable(91, TRUE);
    mt_set_gpio_pull_enable(28, TRUE);    
    mt_set_gpio_pull_enable(72, TRUE);    
    mt_set_gpio_pull_enable(71, TRUE);      

    mt_set_gpio_pull_enable(110, FALSE);
    mt_set_gpio_pull_enable(111, FALSE);
    mt_set_gpio_pull_enable(112, FALSE);
    mt_set_gpio_pull_enable(113, FALSE);
    mt_set_gpio_pull_enable(114, FALSE);
    mt_set_gpio_pull_enable(23, FALSE);      //NC
    mt_set_gpio_pull_enable(74, FALSE);    
    mt_set_gpio_pull_enable(73, FALSE);

    mt_set_gpio_pull_select(87, GPIO_PULL_UP);
    mt_set_gpio_pull_select(88, GPIO_PULL_UP);
    mt_set_gpio_pull_select(89, GPIO_PULL_UP); //NC
    mt_set_gpio_pull_select(90, GPIO_PULL_UP);
    mt_set_gpio_pull_select(91, GPIO_PULL_UP); //NC
    mt_set_gpio_pull_select(28, GPIO_PULL_UP); //NC   
    mt_set_gpio_pull_select(72, GPIO_PULL_UP); //NC   
    mt_set_gpio_pull_select(71, GPIO_PULL_UP); //NC       

    mt_set_gpio_pull_select(110, GPIO_PULL_DOWN);
    mt_set_gpio_pull_select(111, GPIO_PULL_DOWN);
    mt_set_gpio_pull_select(112, GPIO_PULL_DOWN);
    mt_set_gpio_pull_select(113, GPIO_PULL_DOWN);
    mt_set_gpio_pull_select(114, GPIO_PULL_DOWN);
    mt_set_gpio_pull_select(23, GPIO_PULL_DOWN);    
    mt_set_gpio_pull_select(74, GPIO_PULL_DOWN);    
    mt_set_gpio_pull_select(73, GPIO_PULL_DOWN);      

#if KPD_PWRKEY_USE_EINT
    mt_set_gpio_mode(63, 1);	/* EINT4 */
#endif

#if KPD_HAS_SLIDE_QWERTY
    mt_set_gpio_mode(63, 1);	/* EINT4 */
#endif

    // for capacitive touch panel external interrupt, HW default behavior is pull-down   
    mt_set_gpio_pull_enable(61, FALSE); 
    /* I2C1 */
    mt_set_gpio_mode(34, 1);
    mt_set_gpio_mode(35, 1);

    /* I2C3 */
    mt_set_gpio_mode(135, 1);
    mt_set_gpio_mode(136, 1);

    /* WIFI INT */
    mt_set_gpio_mode(GPIO_WIFI_EINT_PIN, GPIO_WIFI_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_WIFI_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_WIFI_EINT_PIN, 1);
    mt_set_gpio_pull_select(GPIO_WIFI_EINT_PIN, GPIO_PULL_UP);
    mt_set_gpio_mode(GPIO_WIFI_EINT_PIN, GPIO_WIFI_EINT_PIN_M_EINT); /* EINT5 */    

	/*touch panel*/
    mt_set_gpio_pull_enable(61, TRUE);
    mt_set_gpio_pull_select(61, GPIO_PULL_UP);

    /*extra*/
    mt_set_gpio_pull_enable(1, 0);
    mt_set_gpio_pull_select(22, GPIO_PULL_UP);
    mt_set_gpio_pull_select(54, GPIO_PULL_UP);
    mt_set_gpio_pull_select(55, GPIO_PULL_UP);
    mt_set_gpio_pull_select(58, GPIO_PULL_UP);
    mt_set_gpio_pull_select(59, GPIO_PULL_UP);
    mt_set_gpio_pull_select(60, GPIO_PULL_UP);
    mt_set_gpio_pull_select(62, GPIO_PULL_UP);
    mt_set_gpio_pull_select(63, GPIO_PULL_UP);
    mt_set_gpio_pull_select(66, GPIO_PULL_UP);
    mt_set_gpio_pull_select(135, GPIO_PULL_UP);
    mt_set_gpio_pull_select(136, GPIO_PULL_UP);

    mt_set_gpio_pull_enable(67, 1);
    mt_set_gpio_pull_select(67, GPIO_PULL_UP);
    mt_set_gpio_pull_enable(68, 1);
    mt_set_gpio_pull_select(68, GPIO_PULL_UP);
    mt_set_gpio_pull_enable(69, 1);
    mt_set_gpio_pull_select(69, GPIO_PULL_UP);
    mt_set_gpio_pull_enable(70, 1);
    mt_set_gpio_pull_select(70, GPIO_PULL_UP);
#endif    
}

static void __init mt6516_init_gpio(void)
{
    int idx;
    typedef struct {
        int pin;
        u32 conf; 
    } GPIO_UNLOCK;
    GPIO_UNLOCK items[] = {
        {GPIO1,     GPIO_CONF_PULLSEL},
        {GPIO2,     GPIO_CONF_ALL},
        {GPIO22,    GPIO_CONF_PULLSEL},
        {GPIO54,    GPIO_CONF_PULLSEL},
        {GPIO55,    GPIO_CONF_ALL},
        {GPIO58,    GPIO_CONF_PULLSEL},
        {GPIO59,    GPIO_CONF_PULLSEL},
        {GPIO60,    GPIO_CONF_PULLSEL},
        {GPIO62,    GPIO_CONF_PULLSEL},
        {GPIO66,    GPIO_CONF_PULLSEL},
        {GPIO119,   GPIO_CONF_ALL},
        {GPIO124,   GPIO_CONF_ALL},

        
        /*COL*/
        {GPIO87,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO88,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO89,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO90,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO91,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL}, 
        {GPIO28,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO72,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL}, 
        {GPIO71,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL}, 
        /*ROW*/
        {GPIO110,   GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO111,   GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO112,   GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO113,   GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO114,   GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL}, 
        {GPIO23,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO74,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL}, 
        {GPIO73,    GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL}, 
        /*SLIDE/PWRKEY*/
        {GPIO63,    GPIO_CONF_ALL}, 
        /*TP*/            
        {GPIO61,    GPIO_CONF_ALL},
        /*I2C1*/            
        {GPIO34,    GPIO_CONF_MODE},
        {GPIO35,    GPIO_CONF_MODE},
        /*I2C3*/                        
        {GPIO135,   GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        {GPIO136,   GPIO_CONF_MODE|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
        /*WIFI*/                        
        {GPIO_WIFI_EINT_PIN,   GPIO_CONF_MODE|GPIO_CONF_DIR|GPIO_CONF_PULLEN|GPIO_CONF_PULLSEL},
            
        {GPIO67,    GPIO_CONF_ALL},            
        {GPIO68,    GPIO_CONF_ALL},            
        {GPIO69,    GPIO_CONF_ALL},            
        {GPIO70,    GPIO_CONF_ALL},            
    };
    mt_gpio_unlock_init(0); 
    for (idx = 0; idx < sizeof(items)/sizeof(items[0]); idx++)
        mt_gpio_unlock_pin(items[idx].pin, items[idx].conf);
    mt_gpio_unlock_dump();
    mt_gpio_set_default();
    
    //sunxiaoye GPS Porting begin 
    printk("MediaTek MT6516 GPIO118->32K OUT\n");
    mt_set_gpio_mode(GPIO118, GPIO_MODE_01);
    mt_set_gpio_dir(GPIO118, GPIO_DIR_OUT);
    mt_set_clock_output(CLK_OUT3,CLK_SRC_F32K);
    //sunxiaoye GPS Porting end
}

static void mt6516_power_off(void)
{
    int fChargerIn = 0; /* 1:ChargerIn , 0:ChargerOut */
    
    printk("mt6516_power_off Function Start !\n");

    /* Check Charger In or Out */

    /* RTC Clear Power Key */

    /* RTC Power BB down */
    rtc_bbpu_power_down();

    /*
     * If charger is still plug-in, we cannot power off cell
     * We just turn off LCM and Keypad then reset
     */
    mt6326_kpled_Disable();
    mt6326_bl_Disable();

    fChargerIn = pmic_chrdet_status();

    /* fChargerIn Case */
    if ( fChargerIn )
    {
        /* RGU Reset */
        arch_reset(0);
    }
    else
    {
        /* Delay 30s, than Reset. To prevent user behaiver */
        while(1);
    }    
}

static void __init mt6516_init_machine(void)
{
    /* Setting the Machine power off function */
    pm_power_off = mt6516_power_off;

    mt6516_init_gpio();
    mt_gpio_checkpoint_save();
    mt6516_init_pinmux();
    mt_gpio_checkpoint_compare();    
}


extern struct sys_timer mt6516_timer;
extern void __init mt6516_init_irq(void);



//extern void __init mt6516_init_irq(void);


MACHINE_START(MT6516, "MT6516 E1K")
    .phys_io        = MT6516_PA_UART,
    .io_pg_offst    = (MT6516_VA_UART >> 18) & 0xfffc,
    .boot_params    = PHYS_OFFSET + 0x100,
    .map_io         = mt6516_map_io,
    .init_irq       = mt6516_init_irq,
    .init_machine   = mt6516_init_machine,
    .timer          = &mt6516_timer,
    .fixup          = mt6516_fixup,
MACHINE_END

