


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

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

#include <linux/kthread.h>

#include <linux/input.h>

#include <linux/wakelock.h>

#define CONFIG_DEBUG_MSG
#ifdef CONFIG_DEBUG_MSG
#define PRINTK(format, args...) printk( KERN_EMERG format,##args )
#else
#define PRINTK(format, args...)
#endif

#define EINT7    (7)
#define EINT6    (6)
#define DEBOUNCE_ENABLE    (1)
#define DEBOUNCE_TIME          (0x3F)

#define NORMAL_EARPHONE_HIIGH (1900)
//#define NORMAL_EARPHONE_LOW (500)
#define NORMAL_EARPHONE_LOW (1)
#define SENDKEY_ADC (300)

int g_headset_first = 1;
int headset_work_flag = 0;

struct wake_lock headset_suspend_lock; 

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
extern int GetOneChannelValue(int dwChannel, int deCount);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINTIRQUnmask(unsigned int line);


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
    HEADSET = 1,
    HEADSET_NO_MIC = 2
};

enum Hook_Switch_result
{
    DO_NOTHING =0,
    ANSWER_CALL = 1,
    REJECT_CALL = 2
};


static  struct switch_dev Headset_Data;
static struct work_struct headset_work;
static struct work_struct hook_switch_work;
static bool Check_state_stable(bool signal);
void headset_irq_handler(void);
#define ENABLE_THRESHOLD (15)
#define GPIO_COUNT_INTERNVAL (20)
//#define HK_GPIO_COUNT_INTERNVAL (50)
#define HK_GPIO_COUNT_INTERNVAL (20)

#define ADC_CHANNEL 5
#define ADC_MEASURE_COUNT 5
DECLARE_WAIT_QUEUE_HEAD(Hook_Switch_Wait_Queue);
static int hk_wait_queue_flag =0;

//#define BUTTON_DETECT_TIMESLIDE (100)
#define BUTTON_DETECT_TIMESLIDE (200)
#define BUTTON_DETECT_INTERNVAL (2000)
int g_call_detect =0;
DECLARE_WAIT_QUEUE_HEAD(Button_Detect_Wait_Queue);
static int bd_wait_queue_flag =0;
int g_button_press = 0;
int V_press = 10; // 10mV
int g_mic_on_off = 0;

static struct input_dev *kpd_headset_dev;
#define KEY_CALL	KEY_SEND
#define KEY_ENDCALL	KEY_END
// test 
//#define KEY_CALL KEY_MENU
//#define KEY_ENDCALL KEY_BACK

#define HEADSET_DEVNAME "MT6516-headset"
#define HEADSET_INIT 0
#define HOOK_SWITCH_CHECK 1
#define GET_CALL_STATE 2
#define GET_Button_Status 3

static struct class *headset_class = NULL;
static int headset_major = 0;
static dev_t headset_devno;
static struct cdev *headset_cdev;

int headset_in_data[1] = {0};
int headset_out_data[1] = {0}; // 0:MIC power off , 1:MICpower on

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

int Check_headset_type(void)
{
	int adc_channel_voltage = 0;

	// MIC power on
	PRINTK("[Headset] Power On MIC bias....\n");
	hk_wait_queue_flag = 1;
	headset_out_data[0] = 1;
	g_mic_on_off = 1;
   	wake_up_interruptible(&Hook_Switch_Wait_Queue);
	
	//PRINTK("[Headset] HK_GPIO_COUNT_INTERNVAL : %d ms \n", HK_GPIO_COUNT_INTERNVAL);
	msleep ((int)HK_GPIO_COUNT_INTERNVAL*20);
	adc_channel_voltage = GetOneChannelValue(ADC_CHANNEL,ADC_MEASURE_COUNT);
	adc_channel_voltage = adc_channel_voltage / ADC_MEASURE_COUNT;
	//PRINTK("[Headset] Check_headset_type:adc_channel_voltage : %d mV \n", adc_channel_voltage);

	// MIC power off
	PRINTK("[Headset] Power Off MIC bias....\n");
	hk_wait_queue_flag = 1;
	headset_out_data[0] = 0;
	g_mic_on_off = 0;
	wake_up_interruptible(&Hook_Switch_Wait_Queue);

	if ( (NORMAL_EARPHONE_LOW <= adc_channel_voltage) && (adc_channel_voltage <= NORMAL_EARPHONE_HIIGH) ) {
		return HEADSET;
	} else {
		return HEADSET_NO_MIC;
	}
}

// judge headset
void headset_work_callback(struct work_struct *work)
{
    bool Gpio_signal;
    Gpio_signal =  mt_get_gpio_in(GPIO_HEADSET_INSERT_PIN);
    PRINTK("headset work callback function.  Gpio_signal66 = %d \n",Gpio_signal);


    //if(Gpio_signal == NO_DEVICE_GPIO && Headset_Data.state ==HEADSET ){
 	if(Gpio_signal == NO_DEVICE_GPIO && 
		( (Headset_Data.state==HEADSET) || (Headset_Data.state==HEADSET_NO_MIC) ) 
	) {
	PRINTK("Gpio_signal == NO_DEVICE_GPIO && Headset_Data.state ==HEADSET* \n");
         if(Check_state_stable(Gpio_signal) == false){
             PRINTK("Check_state_stable == false\n");
			 headset_work_flag = 0;
             return;
         }
	PRINTK("After check stable, NO Headset ! \n");
    }
    else if(Gpio_signal == HEADSET_GPIO && Headset_Data.state ==NO_DEVICE ){
	PRINTK("Gpio_signal == HEADSET_GPIO && Headset_Data.state ==NO_DEVICE \n");
         if(Check_state_stable(Gpio_signal) == false){
             PRINTK("Check_state_stable == false\n");
			 headset_work_flag = 0;
             return;
         }
	PRINTK("After check stable, Exist Headset ! \n");	 
    }

	headset_work_flag = 1;

	//below set polarity and state
    if(Gpio_signal == NO_DEVICE_GPIO)
    {
    	MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,HEADSET_GPIO);
    	Headset_Data.state = NO_DEVICE;
    	PRINTK("Gpio_signal == NO_DEVICE\n");
    	//switch_set_state((struct switch_dev *)&Headset_Data,NO_DEVICE);
		switch_set_state((struct switch_dev *)Headset_Data.dev,NO_DEVICE);

		//wake_unlock(&headset_suspend_lock);
		//PRINTK("headset_work_callback : wake Unlock\n");
    }
    else if (Gpio_signal == HEADSET_GPIO)
    {		
		MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,NO_DEVICE_GPIO);

		Headset_Data.state = Check_headset_type();
		if (Headset_Data.state == HEADSET) {
	    	PRINTK("Gpio_signal == HEADSET\n");
		} else if(Headset_Data.state == HEADSET_NO_MIC) {
			PRINTK("Gpio_signal == HEADSET_NO_MIC\n");
		} else {
		}
		
   	    //switch_set_state((struct switch_dev *)&Headset_Data,HEADSET);
		switch_set_state((struct switch_dev *)Headset_Data.dev,HEADSET);

		//wake_lock(&headset_suspend_lock);
		//PRINTK("headset_work_callback : wake lock\n");
    }
}

void hook_switch_work_callback(struct work_struct *work)
{
	bool hk_Gpio_signal;
    hk_Gpio_signal =  mt_get_gpio_in(GPIO_HEADSET_INSERT_PIN);
    	
	if(hk_Gpio_signal == HEADSET_GPIO && 
		( (Headset_Data.state==HEADSET) ) 
#if 0		
		( (Headset_Data.state==HEADSET) || (Headset_Data.state==HEADSET_NO_MIC) ) 
#endif		
	) {
	
		//PRINTK("MIC power turn on .............................. \n");
		//hk_wait_queue_flag = 1;
		//headset_out_data[0] = 1;
    	//wake_up_interruptible(&Hook_Switch_Wait_Queue);
    	if (g_call_detect != 0) 
		{
	    	if (g_mic_on_off == 0)
			{
				g_mic_on_off = 1;
				
				PRINTK("MIC power turn on .............................. \n");
		headset_out_data[0] = 1;
				hk_wait_queue_flag = 1;
    	wake_up_interruptible(&Hook_Switch_Wait_Queue);
			}		
    	}
	} 

	if(hk_Gpio_signal == NO_DEVICE_GPIO && Headset_Data.state ==NO_DEVICE ) {

		bd_wait_queue_flag = 0; // stop button detection
		
		PRINTK("MIC power turn off .............................. \n");		
		hk_wait_queue_flag = 1;
		headset_out_data[0] = 0;
		g_mic_on_off = 0;
    	wake_up_interruptible(&Hook_Switch_Wait_Queue);		
	}
	
}

void  headset_irq_handler(void){
    int ret = schedule_work(&headset_work);	
    if(!ret)
    	PRINTK("headset_irq_handler:headset_work return %d", ret);
    else
    	{}

	/* MIC power management */
	ret = schedule_work(&hook_switch_work);
	if(!ret)
    	PRINTK("headset_irq_handler:hook_switch_work return %d", ret);
    else
    	{}

	//wake_lock(&headset_suspend_lock);
	//PRINTK("headset_irq_handler : wake lock\n");

	//PRINTK("headset_irq_handler:Done\n");

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
  		PRINTK("mt_set_gpio_mode returned \n");
    else{}
    if( mt_set_gpio_dir(GPIO_HEADSET_INSERT_PIN,false)) // set GPIO 66  as input
    	PRINTK("mt_set_gpio_dir returned \n");
    else{}
    if(mt_set_gpio_pull_enable( GPIO_HEADSET_INSERT_PIN,  true)) // set pull high enable
    	PRINTK("mt_set_gpio_pull_enable returned \n");
    else{}
    if(mt_set_gpio_pull_select(GPIO_HEADSET_INSERT_PIN,true)) // set pull up
    	PRINTK("mt_set_gpio_pull_enable returned \n");
    else{}
    if(MT6516_EINT_Set_Sensitivity(CUST_EINT_HEADSET_NUM,CUST_EINT_HEADSET_SENSITIVE)) // set EIN7 to EDGE trigger
    	PRINTK("MT6516_EINT_Set_Sensitivity returned \n");
    else{}
    	MT6516_EINT_Set_HW_Debounce(CUST_EINT_HEADSET_NUM,CUST_EINT_HEADSET_DEBOUNCE_CN); // set debounce time

#if 0	
    if(mt_set_gpio_mode( GPIO_HEADSET_REMOTE_BUTTON_PIN,  0x01)) // set EINT 6 to input;
         PRINTK("mt_set_gpio_mode returned \n");
    else{}
    if(mt_set_gpio_dir(GPIO_HEADSET_REMOTE_BUTTON_PIN,false)) // set GPIO65  as input
         PRINTK("mt_set_gpio_dir returned \n");
    else{}
    if(mt_set_gpio_pull_enable( GPIO_HEADSET_REMOTE_BUTTON_PIN,  true)) // set pull high enable
         PRINTK("mt_set_gpio_pull_enable returned \n");
    else{}
    if(mt_set_gpio_pull_select(GPIO_HEADSET_REMOTE_BUTTON_PIN,false)) // set pull down
         PRINTK("mt_set_gpio_pull_select returned \n");
    else{}
    if( MT6516_EINT_Set_Sensitivity(EINT6,CUST_EINT_HEADSET_SENSITIVE)) // set EIN6 to EDGE trigger
         PRINTK("MT6516_EINT_Set_Sensitivity returned \n");
    else{}
    	MT6516_EINT_Set_HW_Debounce(EINT6,CUST_EINT_HEADSET_DEBOUNCE_CN); // set debounce time
#endif		

}

void Sound_Headset_Unset_Gpio(void) // use for suspend
{
    mt_set_gpio_mode(GPIO_HEADSET_INSERT_PIN, 0);
    mt_set_gpio_dir(GPIO_HEADSET_INSERT_PIN,  GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_HEADSET_INSERT_PIN,  TRUE);
    mt_set_gpio_pull_select(GPIO_HEADSET_INSERT_PIN,  GPIO_PULL_DOWN);
#if 0
    mt_set_gpio_mode(GPIO_HEADSET_REMOTE_BUTTON_PIN, 0);
    mt_set_gpio_dir(GPIO_HEADSET_REMOTE_BUTTON_PIN,  GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_HEADSET_REMOTE_BUTTON_PIN,  TRUE);
    mt_set_gpio_pull_select(GPIO_HEADSET_REMOTE_BUTTON_PIN,  GPIO_PULL_DOWN);
#endif
}

int hook_switch_func(void)
{
	int adc_channel_voltage = 0;
	int i=0;
	int count = 0;
	int bd_count = 0;
	
	count = ((int)BUTTON_DETECT_INTERNVAL) / ((int)BUTTON_DETECT_TIMESLIDE);
	if (count == 0) {
		PRINTK("[Headset] count == 0\n");
		return DO_NOTHING;
	}
	
	for (i=0;i<count;i++)
	{		
		msleep ((int)(BUTTON_DETECT_TIMESLIDE));
		adc_channel_voltage = GetOneChannelValue(ADC_CHANNEL,ADC_MEASURE_COUNT);
		adc_channel_voltage = adc_channel_voltage / ADC_MEASURE_COUNT;
		//PRINTK("[MIC:ON] adc_channel_voltage : %d mV \n", adc_channel_voltage);				

		if (adc_channel_voltage <= V_press) {
			g_button_press = 1;
		} else {
			g_button_press = 0;
		}

		if (i == 0) {
			if (adc_channel_voltage > V_press) {
				//PRINTK("-------1\n");
				return DO_NOTHING;
			}
		}

		if (adc_channel_voltage <= V_press) {
			bd_count++;
		}
	}				

	//PRINTK("[Headset] bd_count = %d, count = %d\n", bd_count, count);

	//PRINTK("-------2\n");
	if ( bd_count == count ) {
		return REJECT_CALL;		
	} else if( bd_count > 1) {
		return ANSWER_CALL;
	} else {
		return DO_NOTHING;	
	}
}

int button_detect_kthread(void *x)
{   
	int hk_result = 0;
	int ret=0;

    while (1) {           
    
    	// bd_wait_queue_flag controlled by ioctl:GET_CALL_STATE 		
		wait_event_interruptible(Button_Detect_Wait_Queue, bd_wait_queue_flag);   
		//PRINTK("[Headset] Button detecting....\n");
		//msleep ((int)GPIO_COUNT_INTERNVAL*20); // 1s
		
		hk_result = hook_switch_func();
		//PRINTK("[Headset] hook_switch_func = %d\n", hk_result);		

		ret =  mt_get_gpio_in(GPIO_HEADSET_INSERT_PIN);	// get GPIO signal

		if ( (Headset_Data.state == HEADSET) && (ret == HEADSET_GPIO) && (g_call_detect != 0) && (headset_work_flag == 1) ) {	
			//PRINTK("(Headset_Data.state == HEADSET) && (ret == HEADSET_GPIO) && (g_call_detect != 0) && (headset_work_flag == 1) +++++++\n");
			if (hk_result == ANSWER_CALL) {
				//if (g_call_detect != 2) {
					//PRINTK("[Headset] Report key event : KEY_CALL(KEY_SEND)\n");
					input_report_key(kpd_headset_dev, KEY_CALL, 1);
					msleep ((int)BUTTON_DETECT_TIMESLIDE); 
					input_report_key(kpd_headset_dev, KEY_CALL, 0);
				//}
			} else if (hk_result == REJECT_CALL) {
				//PRINTK("[Headset] Report key event : KEY_ENDCALL(KEY_END) : %d\n",BUTTON_DETECT_INTERNVAL);
				input_report_key(kpd_headset_dev, KEY_ENDCALL, 1);
				msleep ((int)BUTTON_DETECT_TIMESLIDE); 
				input_report_key(kpd_headset_dev, KEY_ENDCALL, 0);
			} else {
			}		
		}
    }

    return 0;
}

static int headset_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int *user_data_addr;
		
    switch(cmd)
    {
        case HEADSET_INIT :
   		    PRINTK("headset_ioctl : HEADSET_INIT\n");   		    
			if (g_headset_first == 1) { 
								
				int ret=0;				
				/* misc_register and switch_dev_register move to mod_init */
				
				Sound_Headset_Set_Gpio();
			
				ret =  mt_get_gpio_in(GPIO_HEADSET_INSERT_PIN);	// get GPIO signal
				PRINTK("GPIO_HEADSET_INSERT_PIN(66) = %d\n",ret);
			
				if(ret == HEADSET_GPIO)  // plugin
				{		
					Headset_Data.state = Check_headset_type();
					if (Headset_Data.state == HEADSET) {
						PRINTK("Gpio_signal == HEADSET\n");
						headset_work_flag = 1;
					} else if(Headset_Data.state == HEADSET_NO_MIC) {
						PRINTK("Gpio_signal == HEADSET_NO_MIC\n");
					} else {
					}
					//switch_set_state((struct  switch_dev *)&Headset_Data,HEADSET);
					switch_set_state((struct  switch_dev *)Headset_Data.dev,HEADSET);
					MT6516_EINT_Registration(CUST_EINT_HEADSET_NUM,CUST_EINT_DEBOUNCE_ENABLE,NO_DEVICE_GPIO,headset_irq_handler,true);
					MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,NO_DEVICE_GPIO);

					//wake_lock(&headset_suspend_lock);
					//PRINTK("HEADSET_INIT : wake lock\n");
				}
				else if (ret == NO_DEVICE_GPIO) // unplug
				{
				   Headset_Data.state = NO_DEVICE;				   
				   //switch_set_state((struct switch_dev *)&Headset_Data,NO_DEVICE);
				   switch_set_state((struct switch_dev *)Headset_Data.dev,NO_DEVICE);
				   PRINTK("GPIO == NO_DEVICE_GPIO\n");
				   MT6516_EINT_Registration(CUST_EINT_HEADSET_NUM,CUST_EINT_DEBOUNCE_ENABLE,HEADSET_GPIO,headset_irq_handler,true);
				   MT6516_EINT_Set_Polarity(CUST_EINT_HEADSET_NUM,HEADSET_GPIO);

				   //wake_unlock(&headset_suspend_lock);
				   //PRINTK("HEADSET_INIT : wake Unlock\n");
				}
			
				INIT_WORK(&headset_work, headset_work_callback);
				INIT_WORK(&hook_switch_work, hook_switch_work_callback);
			
				//------------------------------------------------------------------
				//							KPD input subsystem
				//------------------------------------------------------------------
				kpd_headset_dev = input_allocate_device();
				if (!kpd_headset_dev) {
					printk("kpd_headset_dev : fail!\n");
					return -ENOMEM;
				}
			
				__set_bit(EV_KEY, kpd_headset_dev->evbit);
			
				__set_bit(KEY_CALL, kpd_headset_dev->keybit);
				__set_bit(KEY_ENDCALL, kpd_headset_dev->keybit);
			
				kpd_headset_dev->id.bustype = BUS_HOST;
				
				if(input_register_device(kpd_headset_dev))
					printk("kpd_headset_dev register : fail!\n");
				else 
					printk("kpd_headset_dev register : success!!\n");
				
				//------------------------------------------------------------------
				//							button detect kthread
				//------------------------------------------------------------------
				kthread_run(button_detect_kthread, NULL, "button_detect_kthread");	
				
				g_headset_first = 0;
				
			}else {
				g_headset_first = 0;
			}
            break;

		case HOOK_SWITCH_CHECK :
			user_data_addr = (int *)arg;
			//-------------------------------------------------------
			// Blocking sound thread before want to turn on / off MIC Bias power 
			//-------------------------------------------------------
			PRINTK("Before bolcking..................\n");
			hk_wait_queue_flag = 0;
			wait_event_interruptible(Hook_Switch_Wait_Queue, hk_wait_queue_flag);
			PRINTK("After bolcking..................\n");	

			if (g_call_detect != 0) { 				
				if (Headset_Data.state == HEADSET) {					
					if (bd_wait_queue_flag == 0) {
						bd_wait_queue_flag = 1; // start button detection once
				    	wake_up_interruptible(&Button_Detect_Wait_Queue);
			    	}
			   	}
			}
            break;

		case GET_CALL_STATE :
			g_call_detect = (int) arg;
			PRINTK("headset_ioctl : CALL_STATE = %d\n", g_call_detect);
			
			if (Headset_Data.state == HEADSET) 
			{
				if (g_call_detect == 0) 
				{
					if (g_mic_on_off == 1)
					{
						g_mic_on_off = 0;
				
						PRINTK("[Headset] Power Off MIC bias....\n");
						headset_out_data[0] = 0;
						hk_wait_queue_flag = 1;						
						wake_up_interruptible(&Hook_Switch_Wait_Queue);						
					}
				}
				else
				{
					if (g_mic_on_off == 0)
					{
						g_mic_on_off = 1;
						
						PRINTK("[Headset] Power On MIC bias....\n");
						headset_out_data[0] = 1;
						hk_wait_queue_flag = 1;
						wake_up_interruptible(&Hook_Switch_Wait_Queue);
					}
				}
			}		
			
			if (g_call_detect == 0) { 
				bd_wait_queue_flag = 0; // stop button detection
				//PRINTK("[Headset] Stop button detection....\n");	

				wake_unlock(&headset_suspend_lock);
				//PRINTK("GET_CALL_STATE : wake Unlock\n");
				
    		} else {
    			if (Headset_Data.state == HEADSET) {
    				if (bd_wait_queue_flag == 0) {
	    				bd_wait_queue_flag = 1; // start button detection once
	    				//PRINTK("[Headset] Start button detection....\n");
						wake_up_interruptible(&Button_Detect_Wait_Queue);
    				}
    			}

				wake_lock(&headset_suspend_lock);
				//PRINTK("GET_CALL_STATE : wake lock\n");
    		}
			break;

		case GET_Button_Status :
			PRINTK("headset_ioctl : Button_Status=%d (state:%d)\n", g_button_press, Headset_Data.state);	
			return g_button_press;
			break;
		  
        default:
   		    PRINTK("headset_ioctl : default\n");
            break;
    }
	
    return headset_out_data[0];
}

static int headset_open(struct inode *inode, struct file *file)
{ 
   	return 0;
}

static int headset_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations headset_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= headset_ioctl,
	.open		= headset_open,
	.release	= headset_release,	
};

struct platform_device Yusu_device_headset = {
	.name	  ="Yusu_Headset_Driver",		
	.id		  = 0,	
};

static kal_bool headset_Suspen_Flag = false;

static int Yusu_headset_probe(struct platform_device *dev)	
{
	PRINTK("[Audio]Run Yusu_headset_probe \n");
	return 0;
}

static int Yusu_headset_suspend(struct platform_device *dev, pm_message_t state)  // only one suspend mode
{
	if (g_call_detect == 0) 
	{ 
		PRINTK("[Audio]Run headset_suspend \n");
		MT6516_EINTIRQMask(7);
        MT6516_EINTIRQMask(6);
		Sound_Headset_Unset_Gpio();
		*MT6516_EINT_MASK_SET = (1 << 7);
		headset_Suspen_Flag = true;
	} 
	else 
	{
		PRINTK("[Audio]Can not run headset_suspend, keep EINT \n");
	}
	
	return 0;
}

static int Yusu_headset_resume(struct platform_device *dev) // wake up
{
	if( headset_Suspen_Flag == true )
	{
		PRINTK("[Audio]Run headset_resume:%d \n", g_call_detect);
		MT6516_EINTIRQUnmask(7);
	    MT6516_EINTIRQUnmask(6);
	    Sound_Headset_Set_Gpio();
		*MT6516_EINT_INTACK = (1 << 7);
	    *MT6516_EINT_MASK_CLR = (1 << 7);
		headset_Suspen_Flag = false;
	}
	else 
	{
		PRINTK("[Audio]Not need headset_resume\n");
	}
	
	return 0;
}

static struct platform_driver headset_driver = {
	.probe		= Yusu_headset_probe,	
	.suspend	= Yusu_headset_suspend,
	.resume		= Yusu_headset_resume,
	.driver     = {
	.name       = "Yusu_Headset_Driver",
	},
};

static int  sound_headset_mod_init(void)
{
	struct class_device *class_dev = NULL;
	int ret = 0;

	PRINTK("sound_headset_mod_init...\n");

	//------------------------------------------------------------------
	// 							below register Yusu_audio_headset_device
	//------------------------------------------------------------------	
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
	if(ret){
		PRINTK("switch_dev_register returned %d \n", ret);
		return 1;
	}
		
	//------------------------------------------------------------------
	// 							Create ioctl
	//------------------------------------------------------------------
	ret = alloc_chrdev_region(&headset_devno, 0, 1, HEADSET_DEVNAME);
	if (ret) 
		printk("Error: Can't Get Major number for headset \n");
	headset_cdev = cdev_alloc();
    headset_cdev->owner = THIS_MODULE;
    headset_cdev->ops = &headset_fops;
    ret = cdev_add(headset_cdev, headset_devno, 1);
	if(ret)
	    printk("adc_cali Error: cdev_add\n");
	headset_major = MAJOR(headset_devno);
	headset_class = class_create(THIS_MODULE, HEADSET_DEVNAME);
    class_dev = (struct class_device *)device_create(headset_class, 
													NULL, 
													headset_devno, 
													NULL, 
													HEADSET_DEVNAME);

	//------------------------------------------------------------------
	// 							Headset PM
	//------------------------------------------------------------------
	ret = platform_device_register(&Yusu_device_headset);
	if (ret) {
		PRINTK("Unable to headset device register(%d)\n", ret);	
		return ret;
	}
	else
	{
		PRINTK("headset_device register done\n");
	}
	
	ret = platform_driver_register(&headset_driver);
	if (ret) {
		PRINTK("Unable to register headset driver (%d)\n", ret);
		return ret;
	}
	else
	{
		PRINTK("headset_driver register done\n");
	}

	//------------------------------------------------------------------
	//							wake lock
	//------------------------------------------------------------------
	wake_lock_init(&headset_suspend_lock, WAKE_LOCK_SUSPEND, "headset wakelock");
	PRINTK("headset wakelock init\n");

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

