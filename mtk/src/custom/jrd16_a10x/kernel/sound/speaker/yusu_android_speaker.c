


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
#include <mach/mt6516_typedefs.h>
#include <linux/pmic6326_sw.h>
#include <linux/delay.h>
#include <mach/mt6516_pll.h>
#include "yusu_android_speaker.h"

//#define CONFIG_DEBUG_MSG
#ifdef CONFIG_DEBUG_MSG
#define PRINTK(format, args...) printk( KERN_EMERG format,##args )
#else
#define PRINTK(format, args...)
#endif

#define EXTERNAL_AMP_GPIO (2)

extern void Yusu_Sound_AMP_Switch(BOOL enable);
extern void pmic_asw_bsel(asw_bsel_enum sel);
extern void pmic_spkr_enable(kal_bool enable);
extern void pmic_spkl_enable(kal_bool enable);
extern void pmic_spkl_vol(kal_uint8 val);
extern void pmic_spkr_vol(kal_uint8 val);

bool Speaker_Init(void)
{
    printk("Speaker Init Success");
    return true;
}

void Sound_SpeakerL_SetVolLevel(int level)
{
    int hw_gain ,i=0;
    int  HW_Value[] = {6, 9, 12, 15, 18, 21, 24, 27};
    PRINTK(" Sound_SpeakerL_SetVolLevel  level = %d\n",level);
    if(level > 27)
        PRINTK("Sound_Speaker_Setlevel with level undefined  = %d\n",level);

    hw_gain = HW_Value[7] - level;
    if(hw_gain < HW_Value[0])
	   hw_gain = HW_Value[0];
    for(i = 0 ; i < 7 ; i++){
        if( HW_Value[i] >= hw_gain )
            break;
    }
    pmic_spkl_vol(i);
    PRINTK("Sound_SpeakerL_SetVolLevel  pmic_spkl_vol[%d]\n",i);

}

void Sound_SpeakerR_SetVolLevel(int level)
{
    int hw_gain ,i=0;
    int  HW_Value[] = {6, 9, 12, 15, 18, 21, 24, 27};
    PRINTK(" Sound_SpeakerR_SetVolLevel  level = %d\n",level);
    if(level > 27)
        PRINTK("Sound_Speaker_Setlevel with level undefined  = %d\n",level);

    hw_gain = HW_Value[7] - level;
    if(hw_gain < HW_Value[0])
	   hw_gain = HW_Value[0];
    for(i = 0 ; i < 7 ; i++){
        if( HW_Value[i] >= hw_gain )
            break;
    }
    pmic_spkr_vol(i);
    PRINTK("Sound_SpeakerR_SetVolLevel  pmic_spkl_vol[%d]\n",i);
}
/////////////Mengli Add///////////////////
#define CUST_INTERNAL_CLASSD_LEFT_CHANNEL_ENABLE 0
#define CUST_INTERNAL_CLASSD_RIGHT_CHANNEL_ENABLE 1
/////////////Mengli End///////////////////

void Sound_Speaker_Turnon(int channel)
{
    PRINTK("Sound_Speaker_Turnon channel = %d\n",channel);
    PRINTK("Sound_Speaker_Turnon  Speaker_Volume = %d\n",Speaker_Volume);

    mt_set_gpio_dir(EXTERNAL_AMP_GPIO,true); // set GPIO 66
    mt_set_gpio_out(EXTERNAL_AMP_GPIO,false);

    
    if(channel == Channel_None){
        return;
    }
    //////Mengli modified///////////////////////////
    //else
    if ((channel == Channel_Right||channel == Channel_Stereo)&&CUST_INTERNAL_CLASSD_RIGHT_CHANNEL_ENABLE){
        Sound_SpeakerR_SetVolLevel(Speaker_Volume);
        pmic_spkr_enable(true);
    }
    //else
    if ((channel == Channel_Left||channel == Channel_Stereo)&&CUST_INTERNAL_CLASSD_LEFT_CHANNEL_ENABLE){
        Sound_SpeakerL_SetVolLevel(Speaker_Volume);
        pmic_spkl_enable(true);
    }
    /*else if ((channel == Channel_Stereo)&&CUST_INTERNAL_CLASSD_LEFT_CHANNEL_ENABLE&&CUST_INTERNAL_CLASSD_RIGHT_CHANNEL_ENABLE){
        Sound_SpeakerL_SetVolLevel(Speaker_Volume);
        pmic_spkl_enable(true);
        Sound_SpeakerR_SetVolLevel(Speaker_Volume);
        pmic_spkr_enable(true);
    }*/
    ///////Mengli modified end///////////////////////
    else{
        PRINTK("Sound_Speaker_Turnon with no define channel = %d\n",channel);
    }
}

void Sound_Speaker_Turnoff(int channel)
{
    PRINTK("Sound_Speaker_Turnoff channel = %d\n",channel);
    
     
    if(channel == Channel_None){
        return;
    }
    else if (channel == Channel_Right){
        pmic_spkr_enable(false);
    }
    else if (channel == Channel_Left){
        pmic_spkl_enable(false);
    }
    else if (channel == Channel_Stereo){
        pmic_spkl_enable(false);
        pmic_spkr_enable(false);
    }
    else{
        PRINTK("Sound_Speaker_Turnoff with no define channel = %d\n",channel);
    }

    mt_set_gpio_dir(EXTERNAL_AMP_GPIO,true); // set GPIO 66
    mt_set_gpio_out(EXTERNAL_AMP_GPIO,false);    
}

void Sound_Speaker_SetVolLevel(int level)
{
    int hw_gain ,i=0;
    int  HW_Value[] = {6, 9, 12, 15, 18, 21, 24, 27};
    if(level > 27)
        PRINTK("Sound_Speaker_Setlevel with level undefined  = %d\n",level);

    hw_gain = HW_Value[7] - level;
    if(hw_gain < HW_Value[0])
	   hw_gain = HW_Value[0];
    for(i = 0 ; i < 7 ; i++){
        if( HW_Value[i] >= hw_gain )
            break;
    }
    pmic_spkl_vol(i);
    pmic_spkr_vol(i);
    Speaker_Volume =level;
    PRINTK("Sound_Speaker_SetVolLevel  pmic_spkl_vol[%d]\n",Speaker_Volume);
}


void Sound_Headset_Turnon(void)
{

}
void Sound_Headset_Turnoff(void)
{

}


