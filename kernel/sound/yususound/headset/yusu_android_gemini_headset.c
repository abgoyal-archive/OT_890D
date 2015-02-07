


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <mach/mt6516_gpio.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <mach/mt6516_typedefs.h>
#include <linux/switch.h>
#include <linux/delay.h>
#include "../yusu_android_headset.h"

#include <cust_eint.h>

//#define CONFIG_DEBUG_MSG
#ifdef CONFIG_DEBUG_MSG
#define PRINTK(format, args...) printk( KERN_EMERG format,##args )
#else
#define PRINTK(format, args...)
#endif

#define EINT7    (7)
#define EINT6    (6)
#define DEBOUNCE_ENABLE    (1)
#define DEBOUNCE_TIME          (0x3F)
extern void MT6516_EINT_Registration (
    kal_uint8 eintno,
    kal_bool Dbounce_En,
    kal_bool ACT_Polarity,
    void (EINT_FUNC_PTR)(void),
    kal_bool auto_umask
);
extern void MT6516_EINT_Set_Polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens);
extern void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms);


// now plugin in is high , no device is low
#ifdef CONFIG_MT6516_GEMINI_BOARD
enum Headset_state
{
    NO_DEVICE_GPIO = 1,
    HEADSET_GPIO =0
};


#else
enum Headset_state
{
    NO_DEVICE_GPIO = 0,
    HEADSET_GPIO =1
};
#endif

enum Headset_report_state
{
    NO_DEVICE =0,
    HEADSET = 1
};


static  struct switch_dev Headset_Data;
static struct work_struct headset_work;
static bool Check_state_stable(bool signal);
void headset_irq_handler(void);
#define ENABLE_THRESHOLD (20)
#define GPIO_COUNT_INTERNVAL (50)

static bool Check_state_stable(bool signal)
{
    int  count =0, inverseount=0;
    bool Gpio_signal = 0;
    // wait until count to 10 times
    while(count <= (int)ENABLE_THRESHOLD && inverseount < (int)ENABLE_THRESHOLD){
        Gpio_signal =  mt_get_gpio_in(GPIO_HEADSET_INSERT_PIN);  // get signal
        if(Gpio_signal == signal){
            count ++;
            inverseount =0;
        }
        else
        {
           inverseount ++;
           count =0;
        }
        msleep ((int)GPIO_COUNT_INTERNVAL);
   }
   // see the result
    if(count){
        PRINTK("Check_state_stable count = %d return true",count);
        return true;
    }
   return false;
}


// judge headset
void headset_work_callback(struct work_struct *work)
{
    bool Gpio_signal;
    Gpio_signal =  mt_get_gpio_in(GPIO_HEADSET_INSERT_PIN);
    PRINTK("headset work callback function.  Gpio_signal66 = %d \n",Gpio_signal);

    if(Gpio_signal == NO_DEVICE_GPIO && Headset_Data.state ==HEADSET ){
	PRINTK("Gpio_signal == NO_DEVICE_GPIO && Headset_Data.state ==HEADSET \n");
         if(Check_state_stable(Gpio_signal) == false){
             PRINTK("Check_state_stable == false\n");
             return;
         }
    }
    else if(Gpio_signal == HEADSET_GPIO && Headset_Data.state ==NO_DEVICE ){
	PRINTK("Gpio_signal == HEADSET_GPIO && Headset_Data.state ==NO_DEVICE \n");
         if(Check_state_stable(Gpio_signal) == false){
             PRINTK("Check_state_stable == false\n");
             return;
         }
    }

    //below set polarity and state
    if(Gpio_signal == NO_DEVICE_GPIO)
    {
    	MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,HEADSET_GPIO);
    	Headset_Data.state = NO_DEVICE;
    	PRINTK("Gpio_signal == NO_DEVICE\n");
    	switch_set_state((struct switch_dev *)Headset_Data.dev,NO_DEVICE);
    }
    else if (Gpio_signal == HEADSET_GPIO)
    {
    	MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,NO_DEVICE_GPIO);
    	Headset_Data.state = HEADSET;
	PRINTK("Gpio_signal == HEADSET\n");
   	switch_set_state((struct switch_dev *)Headset_Data.dev,HEADSET);
    }
}


void  headset_irq_handler(void){
    int ret = schedule_work(&headset_work);
    if(!ret)
    	PRINTK("headset_irq_handler return %d", ret);
    else
    	{}
    return ;
}

static struct miscdevice Yusu_audio_headset_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "headset",
	.fops = NULL,
};

void Sound_Headset_Set_Gpio(void)
{
    if(mt_set_gpio_mode( GPIO_HEADSET_INSERT_PIN,  0x01))// set EINT 7 to input;
  	PRINTK("mt_set_gpio_mode returned %d \n");
    else{}
    if( mt_set_gpio_dir(GPIO_HEADSET_INSERT_PIN,false)) // set GPIO 66  as input
    	PRINTK("mt_set_gpio_dir returned %d \n");
    else{}
    if(mt_set_gpio_pull_enable( GPIO_HEADSET_INSERT_PIN,  true)) // set pull high enable
    	PRINTK("mt_set_gpio_pull_enable returned %d \n");
    else{}
    if(mt_set_gpio_pull_select(GPIO_HEADSET_INSERT_PIN,true)) // set pull up
    	PRINTK("mt_set_gpio_pull_enable returned %d \n");
    else{}
    if(MT6516_EINT_Set_Sensitivity(CUST_EINT_HEADSET_NUM,CUST_EINT_HEADSET_SENSITIVE)) // set EIN7 to EDGE trigger
    	PRINTK("MT6516_EINT_Set_Sensitivity returned %d \n");
    else{}
    MT6516_EINT_Set_HW_Debounce(CUST_EINT_HEADSET_NUM,CUST_EINT_HEADSET_DEBOUNCE_CN); // set debounce time


    if(mt_set_gpio_mode( GPIO_HEADSET_REMOTE_BUTTON_PIN,  0x01)) // set EINT 6 to input;
         PRINTK("mt_set_gpio_mode returned %d \n");
    else{}
    if(mt_set_gpio_dir(GPIO_HEADSET_REMOTE_BUTTON_PIN,false)) // set GPIO65  as input
         PRINTK("mt_set_gpio_dir returned %d \n");
    else{}
    if(mt_set_gpio_pull_enable( GPIO_HEADSET_REMOTE_BUTTON_PIN,  true)) // set pull high enable
    	PRINTK("mt_set_gpio_pull_enable returned %d \n");
    else{}
    if(mt_set_gpio_pull_select(GPIO_HEADSET_REMOTE_BUTTON_PIN,true)) // set pull up
    	PRINTK("mt_set_gpio_pull_enable returned %d \n");
    else{}
    if(MT6516_EINT_Set_Sensitivity(EINT6,false)) // set EIN7 to EDGE trigger
    	PRINTK("MT6516_EINT_Set_Sensitivity returned %d \n");
    else{}
    MT6516_EINT_Set_HW_Debounce(EINT6,CUST_EINT_HEADSET_DEBOUNCE_CN); // set debounce time
}

void Sound_Headset_Unset_Gpio(void) // use for suspend
{
    mt_set_gpio_mode(GPIO_HEADSET_INSERT_PIN, 0);
    mt_set_gpio_dir(GPIO_HEADSET_INSERT_PIN,  GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_HEADSET_INSERT_PIN,  TRUE);
    mt_set_gpio_pull_select(GPIO_HEADSET_INSERT_PIN,  GPIO_PULL_DOWN);

    mt_set_gpio_mode(GPIO_HEADSET_REMOTE_BUTTON_PIN, 0);
    mt_set_gpio_dir(GPIO_HEADSET_REMOTE_BUTTON_PIN,  GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_HEADSET_REMOTE_BUTTON_PIN,  TRUE);
    mt_set_gpio_pull_select(GPIO_HEADSET_REMOTE_BUTTON_PIN,  GPIO_PULL_DOWN);
}

static int  sound_headset_mod_init(void){
    int ret=0;
    PRINTK("sound_headset_mod_init\n");
    // below register Yusu_audio_headset_device
    ret = misc_register(&Yusu_audio_headset_device);
    if(ret) {
	PRINTK("sound_headset_mod_init misc_register returned %d in goldfish_audio_init\n", ret);
	return 1;
    }
    Headset_Data.name = "h2w";
    Headset_Data.dev = Yusu_audio_headset_device.this_device;
    Headset_Data.index = 0;
    Headset_Data.state = NO_DEVICE;

    ret = switch_dev_register(&Headset_Data);
    if(ret ){
    	PRINTK("switch_dev_register returned %d \n", ret);
	return 1;
    }

    Sound_Headset_Set_Gpio();

    ret =  mt_get_gpio_in(GPIO_HEADSET_INSERT_PIN);  // get GPIO signal
    PRINTK("GPIO_HEADSET_INSERT_PIN = %d\n",ret);

    if(ret == HEADSET_GPIO)  // plugin
    {
        Headset_Data.state = HEADSET;
        switch_set_state((struct  switch_dev *)Headset_Data.dev,HEADSET);
        PRINTK("GPIO == HEADSET_GPIO\n");
        MT6516_EINT_Registration(CUST_EINT_HEADSET_NUM,CUST_EINT_DEBOUNCE_ENABLE,NO_DEVICE_GPIO,headset_irq_handler,true);
        MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,NO_DEVICE_GPIO);
    }
    else if (ret == NO_DEVICE_GPIO) // unplug
    {
       Headset_Data.state = NO_DEVICE;
       switch_set_state((struct switch_dev *)Headset_Data.dev,NO_DEVICE);
       PRINTK("GPIO == NO_DEVICE_GPIO\n");
       MT6516_EINT_Registration(CUST_EINT_HEADSET_NUM,CUST_EINT_DEBOUNCE_ENABLE,HEADSET_GPIO,headset_irq_handler,true);
       MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,HEADSET_GPIO);
    }

    INIT_WORK(&headset_work, headset_work_callback);

    PRINTK("sound_headset_mod_init done\n");
    return 0;

}

static void  sound_headset_mod_exit(void){

	PRINTK("sound_headset_mod_exit\n");
}

module_init(sound_headset_mod_init);
module_exit(sound_headset_mod_exit);

MODULE_DESCRIPTION("Yusu_sound_hradset_driver");
MODULE_AUTHOR("ChiPeng <ChiPeng.Chang@mediatek.com>");
MODULE_LICENSE("GPL");

