

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
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_gpio.h>
#include <mach/mt3351_pmu_hw.h>
#include <mach/mt3351_pmu_sw.h>
#include <mach/mt3351_pwm.h>
#include <asm/io.h>

#define  VERSION    "v 0.1"


#define 	PWM_ENABLE              (PWM_BASE+0x0000) /* PWM Enable           */
#define 	PWM3_DELAY              (PWM_BASE+0x0004) /* PWM4 Delay Duration */
#define 	PWM4_DELAY              (PWM_BASE+0x0008) /* PWM5 Delay Duration */
#define 	PWM5_DELAY              (PWM_BASE+0x000C) /* PWM6 Delay Duration */

#define 	PWM0_CON                (PWM_BASE+0x0010) /* PWM1 Control        */
#define 	PWM0_HDURATION          (PWM_BASE+0x0014) /* PWM1 High Duration  */
#define 	PWM0_LDURATION          (PWM_BASE+0x0018) /* PWM1 Low Duration  */
#define 	PWM0_GDURATION          (PWM_BASE+0x001C) /* PWM1 Guard Duration  */
#define 	PWM0_BUF0_BASE_ADDR     (PWM_BASE+0x0020) /* PWM1 Buffer0 Base Address  */
#define 	PWM0_BUF0_SIZE          (PWM_BASE+0x0024) /* PWM1 Buffer0 Size  */
#define 	PWM0_SEND_DATA0         (PWM_BASE+0x0030) /* PWM1 Send Data0  */
#define 	PWM0_SEND_DATA1         (PWM_BASE+0x0034) /* PWM1 Send Data1  */
#define 	PWM0_WAVE_NUM           (PWM_BASE+0x0038) /* PWM1 Wave Number  */
#define 	PWM0_DATA_WIDTH         (PWM_BASE+0x003C) /* PWM1 Data Width  */
#define 	PWM0_THRESH             (PWM_BASE+0x0040) /* PWM1 Threshold  */
#define 	PWM0_SEND_WAVENUM       (PWM_BASE+0x0044) /* PWM1 Send Wave Number  */

#define 	PWM1_CON 	            (PWM_BASE+0x0050) /* PWM2 Control        */
#define 	PWM1_HDURATION	        (PWM_BASE+0x0054) /* PWM2 High Duration  */
#define 	PWM1_LDURATION	        (PWM_BASE+0x0058) /* PWM2 Low Duration  */
#define 	PWM1_GDURATION	        (PWM_BASE+0x005C) /* PWM2 Guard Duration  */
#define 	PWM1_BUF0_BASE_ADDR     (PWM_BASE+0x0060) /* PWM2 Buffer0 Base Address  */
#define 	PWM1_BUF0_SIZE          (PWM_BASE+0x0064) /* PWM2 Buffer0 Size  */
#define 	PWM1_SEND_DATA0         (PWM_BASE+0x0070) /* PWM2 Send Data0  */
#define 	PWM1_SEND_DATA1         (PWM_BASE+0x0074) /* PWM2 Send Data1  */
#define 	PWM1_WAVE_NUM           (PWM_BASE+0x0078) /* PWM2 Wave Number  */
#define 	PWM1_DATA_WIDTH         (PWM_BASE+0x007C) /* PWM2 Data Width  */
#define 	PWM1_THRESH             (PWM_BASE+0x0080) /* PWM2 Threshold  */
#define 	PWM1_SEND_WAVENUM       (PWM_BASE+0x0084) /* PWM2 Send Wave Number  */

#define 	PWM2_CON 	            (PWM_BASE+0x0090) /* PWM3 Control        */
#define 	PWM2_HDURATION	        (PWM_BASE+0x0094) /* PWM3 High Duration  */
#define 	PWM2_LDURATION	        (PWM_BASE+0x0098) /* PWM3 Low Duration  */
#define 	PWM2_GDURATION	        (PWM_BASE+0x009C) /* PWM3 Guard Duration  */
#define 	PWM2_BUF0_BASE_ADDR     (PWM_BASE+0x00A0) /* PWM3 Buffer0 Base Address  */
#define 	PWM2_BUF0_SIZE          (PWM_BASE+0x00A4) /* PWM3 Buffer0 Size  */
#define 	PWM2_SEND_DATA0         (PWM_BASE+0x00B0) /* PWM3 Send Data0  */
#define 	PWM2_SEND_DATA1         (PWM_BASE+0x00B4) /* PWM3 Send Data1  */
#define 	PWM2_WAVE_NUM           (PWM_BASE+0x00B8) /* PWM3 Wave Number  */
#define 	PWM2_DATA_WIDTH         (PWM_BASE+0x00BC) /* PWM3 Data Width  */
#define 	PWM2_THRESH             (PWM_BASE+0x00C0) /* PWM3 Threshold  */
#define 	PWM2_SEND_WAVENUM       (PWM_BASE+0x00C4) /* PWM3 Send Wave Number  */

#define 	PWM3_CON 	            (PWM_BASE+0x00D0) /* PWM4 Control        */
#define 	PWM3_HDURATION	        (PWM_BASE+0x00D4) /* PWM4 High Duration  */
#define 	PWM3_LDURATION	        (PWM_BASE+0x00D8) /* PWM4 Low Duration  */
#define 	PWM3_GDURATION	        (PWM_BASE+0x00DC) /* PWM4 Guard Duration  */
#define 	PWM3_BUF0_BASE_ADDR     (PWM_BASE+0x00E0) /* PWM4 Buffer0 Base Address  */
#define 	PWM3_BUF0_SIZE          (PWM_BASE+0x00E4) /* PWM4 Buffer0 Size  */
#define 	PWM3_SEND_DATA0         (PWM_BASE+0x00F0) /* PWM4 Send Data0  */
#define 	PWM3_SEND_DATA1         (PWM_BASE+0x00F4) /* PWM4 Send Data1  */
#define 	PWM3_WAVE_NUM           (PWM_BASE+0x00F8) /* PWM4 Wave Number  */
#define 	PWM3_SEND_WAVENUM       (PWM_BASE+0x00FC) /* PWM4 Send Wave Number  */

#define 	PWM4_CON 	            (PWM_BASE+0x0110) /* PWM5 Control        */
#define 	PWM4_HDURATION	        (PWM_BASE+0x0114) /* PWM5 High Duration  */
#define 	PWM4_LDURATION	        (PWM_BASE+0x0118) /* PWM5 Low Duration  */
#define 	PWM4_GDURATION	        (PWM_BASE+0x011C) /* PWM5 Guard Duration  */
#define 	PWM4_BUF0_BASE_ADDR     (PWM_BASE+0x0120) /* PWM5 Buffer0 Base Address  */
#define 	PWM4_BUF0_SIZE          (PWM_BASE+0x0124) /* PWM5 Buffer0 Size  */
#define 	PWM4_SEND_DATA0         (PWM_BASE+0x0130) /* PWM5 Send Data0  */
#define 	PWM4_SEND_DATA1         (PWM_BASE+0x0134) /* PWM5 Send Data1  */
#define 	PWM4_WAVE_NUM           (PWM_BASE+0x0138) /* PWM5 Wave Number  */
#define 	PWM4_SEND_WAVENUM       (PWM_BASE+0x013C) /* PWM5 Send Wave Number  */

#define 	PWM5_CON 	            (PWM_BASE+0x0150) /* PWM6 Control        */
#define 	PWM5_HDURATION	        (PWM_BASE+0x0154) /* PWM6 High Duration  */
#define 	PWM5_LDURATION	        (PWM_BASE+0x0158) /* PWM6 Low Duration  */
#define 	PWM5_GDURATION	        (PWM_BASE+0x015C) /* PWM6 Guard Duration  */
#define 	PWM5_BUF0_BASE_ADDR     (PWM_BASE+0x0160) /* PWM6 Buffer0 Base Address  */
#define 	PWM5_BUF0_SIZE          (PWM_BASE+0x0164) /* PWM6 Buffer0 Size  */
#define 	PWM5_SEND_DATA0         (PWM_BASE+0x0170) /* PWM6 Send Data0  */
#define 	PWM5_SEND_DATA1         (PWM_BASE+0x0174) /* PWM6 Send Data1  */
#define 	PWM5_WAVE_NUM           (PWM_BASE+0x0178) /* PWM6 Wave Number  */
#define 	PWM5_SEND_WAVENUM       (PWM_BASE+0x017C) /* PWM6 Send Wave Number  */

#define     PWM_INT_ENABLE          (PWM_BASE+0x0190) /* PWM Interrupt Enable  */
#define     PWM_INT_STATUS          (PWM_BASE+0x0194) /* PWM Interrupt Status  */
#define     PWM_INT_ACK             (PWM_BASE+0x0198) /* PWM Interrupt Acknowledge  */


#define PWM_MAJOR                   251
#define PWM_DEVNAME                 "pwm"

static struct device* pwm_device = NULL;
static unsigned pwm_level = PWM_BACKLIGHT_MAX;

static spinlock_t pwm_lock;


#define FLAG_OPEN			        1
#define FLAG_INFREQ_INVALID		    2
#define FLAG_STARTED			    4

static unsigned int pwm_flags = ~(FLAG_OPEN | FLAG_INFREQ_INVALID | FLAG_STARTED);


void pwm_clk_init(u32 pwm_num, u32 clk_sel, u32 clk_div)
{
	UINT32 tmp;
	UINT32 base_addr;

    base_addr = PWM0_CON + 0x40*pwm_num;
    spin_lock(&pwm_lock);
   	tmp = DRV_Reg32(base_addr);
   	tmp &= ~(PWM_CON_CLKDIV_MASK|PWM_CON_CLKSEL_MASK);
  	tmp |= (PWM_CON_CLKDIV_MASK&(clk_div<<PWM_CON_CLKDIV_SHIFT));
    tmp |= (PWM_CON_CLKSEL_MASK&(clk_sel<<PWM_CON_CLKSEL_SHIFT));    
	DRV_WriteReg32(base_addr,tmp);
	spin_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_clk_init);


void pwm_start(u32 pwm_num)
{
	u32 tmp;

    spin_lock(&pwm_lock);
  	tmp = DRV_Reg32(PWM_ENABLE);
   	tmp &= ~(PWM0_ENABLE_MASK << pwm_num);
   	tmp |= (PWM0_ENABLE << pwm_num);
   	DRV_WriteReg32(PWM_ENABLE, tmp);
   	spin_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_start);


void pwm_stop(u32 pwm_num)
{
	u32 tmp;

    spin_lock(&pwm_lock);
	tmp = DRV_Reg32(PWM_ENABLE);
   	tmp &= ~(PWM0_ENABLE_MASK << pwm_num);
   	DRV_WriteReg32(PWM_ENABLE, tmp);
   	spin_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_stop);


void pwm_seq_start(pwm_seq_en_cnt_e sequence)
{
	u32 tmp;

    spin_lock(&pwm_lock);
  	tmp = DRV_Reg32(PWM_ENABLE);
   	tmp |= (PWM_SEQ_MODE_ON | (sequence<<2));
   	DRV_WriteReg32(PWM_ENABLE, tmp);
   	spin_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_seq_start);


void pwm_seq_stop(void)
{
	u32 tmp;

    spin_lock(&pwm_lock);
	tmp = DRV_Reg32(PWM_ENABLE);
   	tmp &= ~PWM_SEQ_MODE_ON;
   	DRV_WriteReg32(PWM_ENABLE, tmp);
   	spin_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_seq_stop);


void pwm_set_seq_delay(u32 pwm_num, u32 delay)
{
    u32 base_addr;

    base_addr = PWM3_DELAY + 0x0004*(pwm_num - 3);

    DRV_WriteReg32(base_addr, delay); 
}
EXPORT_SYMBOL(pwm_set_seq_delay);


u32 pwm_configure(u32 pwm_num, u32 mode, void *para)
{
	UINT32 base_addr, tmp;
   
    base_addr = PWM0_CON + 0x40*pwm_num;
    spin_lock(&pwm_lock);
	tmp = DRV_Reg32(base_addr);
    switch(mode)
    {
        case PWM_FIFO_MODE:
        {
        	pwm_fifo_para_s   *fifo_para;
            fifo_para = (pwm_fifo_para_s *)para;
#if 0
            printk("fifo para -> idle_output = 0x%x\n", fifo_para->idle_output);
            printk("fifo para -> guard_output = 0x%x\n", fifo_para->guard_output);
            printk("fifo para -> stop_bitpos = 0x%x\n", fifo_para->stop_bitpos);
            printk("fifo para -> data0 = 0x%x\n", fifo_para->data0);
            printk("fifo para -> data1 = 0x%x\n", fifo_para->data1);
            printk("fifo para -> repeat_count = 0x%x\n", fifo_para->repeat_count);
            printk("fifo para -> high_dur = 0x%x\n", fifo_para->high_dur);
            printk("fifo para -> low_dur = 0x%x\n", fifo_para->low_dur);
            printk("fifo para -> guard_dur = 0x%x\n", fifo_para->guard_dur);
#endif            
        	tmp &= ~(PWM_CON_SRCSEL_MASK|/*PWM_CON_MODE_MASK|*/PWM_CON_IDLE_VALUE_MASK|PWM_CON_GUARD_VALUE_MASK|
          	PWM_CON_STOP_BITPOS_MASK|PWM_CON_CLK_PWM_MODE_MASK);
        	tmp |= (PWM_CON_SRCSEL_FIFO |
         	/*PWM_CON_MODE_PERIODIC |*/
         	((fifo_para->idle_output<<PWM_CON_IDLE_VALUE_SHIFT)&PWM_CON_IDLE_VALUE_MASK) |
         	((fifo_para->guard_output<<PWM_CON_GUARD_VALUE_SHIFT)&PWM_CON_GUARD_VALUE_MASK) |
         	((fifo_para->stop_bitpos<<PWM_CON_STOP_BITPOS_SHIFT)&PWM_CON_STOP_BITPOS_MASK) |
         	PWM_CON_FIFO_MEM_PWM_MODE);
        	DRV_WriteReg32(base_addr, tmp);
        	base_addr = PWM0_SEND_DATA0 + 0x40*pwm_num;
        	DRV_WriteReg32(base_addr, fifo_para->data0);
        	base_addr = PWM0_SEND_DATA1 + 0x40*pwm_num;
        	DRV_WriteReg32(base_addr, fifo_para->data1);
        	base_addr = PWM0_WAVE_NUM + 0x40*pwm_num;
        	DRV_WriteReg32(base_addr, fifo_para->repeat_count);
        	base_addr = PWM0_HDURATION + 0x40*pwm_num;
        	DRV_WriteReg32(base_addr, fifo_para->high_dur);
        	base_addr = PWM0_LDURATION + 0x40*pwm_num;
        	DRV_WriteReg32(base_addr, fifo_para->low_dur);
        	base_addr = PWM0_GDURATION + 0x40*pwm_num;
        	DRV_WriteReg32(base_addr, fifo_para->guard_dur);
        	spin_unlock(&pwm_lock);
        }
        break;

        case PWM_MEMO_MODE:
        {
            pwm_memo_para_s   *mem_para;

            mem_para = (pwm_memo_para_s *)para;
            if ((mem_para->high_dur==0) || (mem_para->low_dur==0)/* || (mem_para->guard_dur==0)*/)
            {
                return -EMEMMODEHLDUR;
            }
            tmp &= ~(PWM_CON_SRCSEL_MASK|/*PWM_CON_MODE_MASK|*/PWM_CON_IDLE_VALUE_MASK|PWM_CON_GUARD_VALUE_MASK|
                  PWM_CON_STOP_BITPOS_MASK|PWM_CON_CLK_PWM_MODE_MASK);
            tmp |= (PWM_CON_SRCSEL_MEM |
                 /*PWM_CON_MODE_PERIODIC |*/
                 ((mem_para->idle_output<<PWM_CON_IDLE_VALUE_SHIFT)&PWM_CON_IDLE_VALUE_MASK) |
                 ((mem_para->guard_output<<PWM_CON_GUARD_VALUE_SHIFT)&PWM_CON_GUARD_VALUE_MASK) |
                 ((mem_para->stop_bitpos<<PWM_CON_STOP_BITPOS_SHIFT)&PWM_CON_STOP_BITPOS_MASK) |
                 PWM_CON_FIFO_MEM_PWM_MODE);
            DRV_WriteReg32(base_addr, tmp);
            base_addr = PWM0_BUF0_BASE_ADDR + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, mem_para->buf_addr);
            base_addr = PWM0_BUF0_SIZE + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, mem_para->buf_size);
            base_addr = PWM0_WAVE_NUM + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, mem_para->repeat_count);
            base_addr = PWM0_HDURATION + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, mem_para->high_dur);
            base_addr = PWM0_LDURATION + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, mem_para->low_dur);
            base_addr = PWM0_GDURATION + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, mem_para->guard_dur);
            spin_unlock(&pwm_lock);
        }
        break;
      
        case PWM_CLK_MODE:
        {
            pwm_clk_para_s   *clk_para;
            clk_para = (pwm_clk_para_s *)para;
            tmp &= ~(PWM_CON_IDLE_VALUE_MASK|PWM_CON_GUARD_VALUE_MASK|PWM_CON_CLK_PWM_MODE_MASK);
            tmp |= (((clk_para->idle_output<<PWM_CON_IDLE_VALUE_SHIFT)&PWM_CON_IDLE_VALUE_MASK) |
                 ((clk_para->guard_output<<PWM_CON_GUARD_VALUE_SHIFT)&PWM_CON_GUARD_VALUE_MASK) |
                 PWM_CON_CLK_PWM_MODE);
            DRV_WriteReg32(base_addr, tmp);
            base_addr = PWM0_DATA_WIDTH + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, clk_para->data_width);
            base_addr = PWM0_THRESH + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, clk_para->threshold);
            base_addr = PWM0_WAVE_NUM + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, clk_para->repeat_count);
            base_addr = PWM0_GDURATION + 0x40*pwm_num;
            DRV_WriteReg32(base_addr, clk_para->guard_dur);
            tmp = DRV_Reg32(PWM_INT_ENABLE);
            DRV_WriteReg32(PWM_INT_ENABLE, tmp);
            spin_unlock(&pwm_lock);
        }
        break;

        default:
            spin_unlock(&pwm_lock);
            break;
    }
    return RSUCCESS;
}
EXPORT_SYMBOL(pwm_configure);


void pwm_enable_interrupt(u32 pwm_num, pwm_int_type_e int_type, bool is_enable)
{
    u16   pwm_int_enable;
    
    spin_lock(&pwm_lock);
    pwm_int_enable = DRV_Reg(PWM_INT_ENABLE);

    if(is_enable)
    {
    	pwm_int_enable |= (1 << ((pwm_num << 1) + int_type));
    }
    else
    {
       pwm_int_enable &= ~(1 << ((pwm_num << 1) + int_type));
    }

    DRV_WriteReg(PWM_INT_ENABLE, pwm_int_enable);
    spin_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_enable_interrupt);


static irqreturn_t pwm_interrupt_handler(int irq, void *dev_id)
{
    u16   PWM_Status;
    
    spin_lock(&pwm_lock);
    PWM_Status = DRV_Reg(PWM_INT_STATUS);

    //process finish interrupt    
    if ( PWM_Status & PWM0_INT_FINISH_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM0_INT_FINISH_ACK);
    }
    if ( PWM_Status & PWM1_INT_FINISH_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM1_INT_FINISH_ACK);      
    }
    if ( PWM_Status & PWM2_INT_FINISH_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM2_INT_FINISH_ACK);            
    }
    if ( PWM_Status & PWM3_INT_FINISH_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM3_INT_FINISH_ACK);            
    }
    if ( PWM_Status & PWM4_INT_FINISH_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM4_INT_FINISH_ACK);            
    }
    if ( PWM_Status & PWM5_INT_FINISH_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM5_INT_FINISH_ACK);            
    }

    //proccess underflow interrupt
    if ( PWM_Status & PWM0_INT_UNDERFLOW_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM0_INT_UNDERFLOW_ACK);            
    }
    if ( PWM_Status & PWM1_INT_UNDERFLOW_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM1_INT_UNDERFLOW_ACK);           
    }
    if ( PWM_Status & PWM2_INT_UNDERFLOW_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM2_INT_UNDERFLOW_ACK);           
    }
    if ( PWM_Status & PWM3_INT_UNDERFLOW_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM3_INT_UNDERFLOW_ACK);           
    }
    if ( PWM_Status & PWM4_INT_UNDERFLOW_EN_ST)
    {
        DRV_WriteReg32(PWM_INT_ACK, PWM4_INT_UNDERFLOW_ACK);           
    }
    if ( PWM_Status & PWM5_INT_UNDERFLOW_EN_ST)
    {   
        DRV_WriteReg32(PWM_INT_ACK, PWM5_INT_UNDERFLOW_ACK);           
    }
    spin_lock(&pwm_lock);
   
    return IRQ_HANDLED;
}


s32 pwm_set_pinmux(void)
{
    s32 ret;

    ret = mt_set_gpio_PinMux(PIN_MUX_PWM_CTRL_PWM, PWM_CTL);

    if(ret < 0)
    {
        printk("PWM set PWM_CTL pinmux error!!\n");
        return -ESETPWMPINMUX;
    }
    
#if 0
    
    ret = mt_set_gpio_PinMux(PIN_MUX_UT4_CTRL_GPIO, UT4_CTL);

    if(ret < 0)
    {
        printk("PWM set PWM_CTL pinmux error!!\n");
        return -ESETPWMPINMUX;
    }
    
#endif    
    
    if(mt_get_gpio_PinMux(PWM_CTL)!= PIN_MUX_PWM_CTRL_PWM)
    {
        printk("PWM set PWM_CTL pinmux result error!!\n");
        return -ESETPWMPINMUX;
    }

    if(mt_get_gpio_PinMux(UT4_CTL)!= PIN_MUX_UT4_CTRL_GPIO)
    {
        printk("PWM set UT4_CTL pinmux result error!!\n");
        return -ESETPWMPINMUX;
    }

    printk("Successfully setting PWM_CTL & UT4_CTL GPIO!!\n");

#if 0
    ret = mt_set_gpio_dir(12, GPIO_DIR_OUT);

    if(ret < 0)
    {
        printk("PWM set gpio 12 direction error!!\n");
        return -ESETPWMDIR;
    }
#endif    
    
    ret = mt_set_gpio_dir(13, GPIO_DIR_OUT);

    if(ret < 0)
    {
        printk("PWM set gpio 13 direction error!!\n");
        return -ESETPWMDIR;
    }    

#if 0
    if(mt_get_gpio_dir(12)!= GPIO_DIR_OUT)
    {
        printk("PWM set gpio 12 direction result error!!\n");
        return -ESETPWMDIR;
    }
#endif    
    
    if(mt_get_gpio_dir(13)!= GPIO_DIR_OUT)
    {
        printk("PWM set gpio 13 direction result error!!\n");
        return -ESETPWMDIR;
    }

    printk("Successfully setting GPIO12 & GPIO13 Direction!!\n");

#if 0
    ret = mt_set_gpio_OCFG(GPIO12_OCTL_PWM4, GPIO12_OCTL);

    if(ret < 0)
    {
        printk("PWM set gpio 12 OCFG error!!\n");
        return -ESETPWMOCTL;
    }
    
    if(mt_get_gpio_OCFG(GPIO12_OCTL)!=GPIO12_OCTL_PWM4)
    {
        printk("PWM set gpio 12 OCFG result error!!\n");
        return -ESETPWMOCTL;
    }
#endif    

    ret = mt_set_gpio_OCFG(GPIO13_OCTL_PWM5, GPIO13_OCTL);
    
    if(ret < 0)
    {
        printk("PWM set gpio 13 OCFG error!!\n");
        return -ESETPWMOCTL;
    }         

    if(mt_get_gpio_OCFG(GPIO13_OCTL)!=GPIO13_OCTL_PWM5)
    {
        printk("PWM set gpio 13 OCFG result error!!\n");
        return -ESETPWMOCTL;
    }

    printk("Successfully setting GPIO12 & GPIO13 OCFG!!\n");
    return RSUCCESS;
}


void pwm_power_up(void)
{
    if (PDN_Get_Peri_Status(PDN_PERI_PWM) == TRUE )
        hwEnableClock(MT3351_CLOCK_PWM);        
        //2009/02/02, Kelvin modify for power management
    	//PDN_Power_CONA_DOWN(PDN_PERI_PWM, KAL_FALSE);	
    // JTAG stop here. Check the PWM0~5: wave output 

    //if (PDN_Get_Peri_Status(PDN_PERI_PWM0CLK) == TRUE )
    //   hwEnableClock(MT3351_CLOCK_PWM0);        
        //2009/02/02, Kelvin modify for power management        
    	//PDN_Power_CONA_DOWN(PDN_PERI_PWM0CLK, KAL_FALSE);      
    // JTAG stop here. Check the PWM0: wave output 

    //if (PDN_Get_Peri_Status(PDN_PERI_PWM1CLK) == TRUE )
    //    hwEnableClock(MT3351_CLOCK_PWM1); 
        //2009/02/02, Kelvin modify for power management                
    	//PDN_Power_CONA_DOWN(PDN_PERI_PWM1CLK, KAL_FALSE);	
    // JTAG stop here. Check the PWM1: wave output

    //if (PDN_Get_Peri_Status(PDN_PERI_PWM2CLK) == TRUE )
    //    hwEnableClock(MT3351_CLOCK_PWM2); 
        //2009/02/02, Kelvin modify for power management
        //PDN_Power_CONA_DOWN(PDN_PERI_PWM2CLK, KAL_FALSE);	
    // JTAG stop here. Check the PWM2: wave output
}


void pwm_power_down(void)
{	
	hwDisableClock(MT3351_CLOCK_PWM0); 
	//PDN_Power_CONA_DOWN(PDN_PERI_PWM0CLK,KAL_TRUE);	
	// JTAG stop here. Check the PWM0: wave output

	//hwDisableClock(MT3351_CLOCK_PWM1); 
	//PDN_Power_CONA_DOWN(PDN_PERI_PWM1CLK,KAL_TRUE);	
	// JTAG stop here. Check the PWM1: wave output
	
	//hwDisableClock(MT3351_CLOCK_PWM2); 
	//PDN_Power_CONA_DOWN(PDN_PERI_PWM2CLK,KAL_TRUE);	
	// JTAG stop here. Check the PWM2: wave output

	//hwDisableClock(MT3351_CLOCK_PWM); 	
	//PDN_Power_CONA_DOWN(PDN_PERI_PWM,KAL_TRUE);			
	// JTAG stop here. Check the PWM0~5: wave output
}


static s32 pwm_open(struct inode *inode, struct file *file)
{
	if( pwm_flags & (FLAG_OPEN | FLAG_INFREQ_INVALID) )
		return -EPWMNODEV;

	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	pwm_flags|=FLAG_OPEN;

	//printk("MT3351-PWM device opened!!\n");
	
	return RSUCCESS;
}


static s32 pwm_release(struct inode *inode, struct file *file)
{
	if((pwm_flags & FLAG_OPEN) == 0)
		return -EPWMNODEV;

	pwm_flags&=~FLAG_OPEN;

	//printk("MT3351-PWM device released!!\n");

	return RSUCCESS;
}


static inline u32 pwm_setlevel(unsigned level)
{
    pwm_fifo_para_s fifo_para;

	if (level < PWM_BACKLIGHT_MIN || level > PWM_BACKLIGHT_MAX) 
	{
		printk("Invalid backlight level %u\n", level);
		return -EINVAL;
	}   

    pwm_stop(PWM5);
    pwm_clk_init(PWM5, PWM_CLK_SEL_52M, PWM_CLK_DIV_NONE);
    fifo_para.data0 = PWM_FIFO_DATA_PATTERN1;
    fifo_para.data1 = PWM_FIFO_DATA_PATTERN1;
    fifo_para.repeat_count = 0;
    fifo_para.stop_bitpos = 63;
    fifo_para.high_dur = (U16)((level*(1<<16))/(PWM_BACKLIGHT_MAX-PWM_BACKLIGHT_MIN));
    fifo_para.low_dur = (U16)((PWM_BACKLIGHT_MAX - level + 1)*(1<<16)/(PWM_BACKLIGHT_MAX-PWM_BACKLIGHT_MIN));
    fifo_para.guard_dur = 0;
    fifo_para.idle_output = 0;
    fifo_para.guard_output = 0;
    pwm_configure(PWM5, PWM_FIFO_MODE, (void *)&fifo_para);
    pwm_enable_interrupt(PWM5, PWM_INT_TYPE_FINISH, TRUE);
    pwm_start(PWM5);    

	pwm_level = level;
	
	return RSUCCESS;
}


static s32 pwm_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	s32	rc;

    switch(cmd) 
    {
        case IOW_BACKLIGHT_OFF:
            //printk("MT3351-PWM device ioctl IOW_BACKLIGHT_OFF!!\n");
            rc = pwm_setlevel(PWM_BACKLIGHT_MIN);
		    if(rc == 0) 
		        pwm_flags&=~FLAG_STARTED;
        	break;
        case IOW_BACKLIGHT_ON:
            //printk("MT3351-PWM device ioctl IOW_BACKLIGHT_ON!!\n");
            rc = pwm_setlevel(PWM_BACKLIGHT_MAX);
		    if(rc == 0) 
		        pwm_flags|=FLAG_STARTED;
        	break;
        case IOW_BACKLIGHT_UPDATE:
            //printk("MT3351-PWM device ioctl IOW_BACKLIGHT_UPDATE %d!!\n", arg);            
            rc = pwm_setlevel(arg);
            if(rc == 0)
            {
                if(arg != PWM_BACKLIGHT_MIN)
                    pwm_flags|=FLAG_STARTED;
                else 
                    pwm_flags&=~FLAG_STARTED;
            }
        	break;
        case IOR_BACKLIGHT_CURRENT:
            //printk("MT3351-PWM device ioctl IOR_BACKLIGHT_CURRENT %d!!\n", pwm_level); 
        	return pwm_level;
        default:
        	printk("Invalid ioctl command %u\n", cmd);
        	return -EPWMINVAL;
    }
    
    return rc; 
}


static struct file_operations pwm_fops = 
{
	.owner		= THIS_MODULE,
	.ioctl		= pwm_ioctl,
	.open		= pwm_open,
	.release	= pwm_release,
};


static int pwm_probe(struct device *dev)
{
    s32 ret;

    ret = register_chrdev(PWM_MAJOR, PWM_DEVNAME, &pwm_fops);

    if (ret < 0) 
    {
        printk("Unable to register PWM device on major=%d (%d)\n", PWM_MAJOR, ret);
        return ret;
    }
    printk("Success to register PWM device on major=%d (%d)\n", PWM_MAJOR, ret);

    pwm_device = dev;

    pwm_level = PWM_BACKLIGHT_MIN;

    /* power up pwm */
    pwm_power_up();
    /* set up pinmux */
    // pwm_set_pinmux();
    /* register IRQ line and ISR */
    request_irq(MT3351_IRQ_PWM_CODE, pwm_interrupt_handler, 0, "MT3351_PWM", NULL);
    
    return RSUCCESS;
}


static int pwm_remove(struct device *dev)
{
    unregister_chrdev(PWM_MAJOR, PWM_DEVNAME);
    pwm_power_down();
    return RSUCCESS;
}


static void pwm_shutdown(struct device *dev)
{
    printk("PWM Shut down\n");
    pwm_power_down();
}


static int pwm_suspend(struct device *pdev, pm_message_t mesg)
{
    printk("PWM Suspend !\n");
    return RSUCCESS;
}


static int pwm_resume(struct device *pdev)
{
    printk("PWM Resume !\n");
    return RSUCCESS;
}


static struct device_driver pwm_driver = 
{
	.name		= "mt3351-pwm",
	.bus		= &platform_bus_type,
	.probe		= pwm_probe,
	.remove		= pwm_remove,
	.shutdown	= pwm_shutdown,
	.suspend	= pwm_suspend,
	.resume		= pwm_resume,	
};



static s32 __init pwm_mod_init(void)
{
    s32 ret;

    spin_lock_init(&pwm_lock);
    
    /* Need to modify as the PMU device driver is ready! */
    DRV_WriteReg32(PMU_CON2, 0x13);
    
    printk("MediaTek MT3351 pwm driver register, version %s\n", VERSION);

    ret = driver_register(&pwm_driver);

	if(ret) 
    {
		printk("Unable to register pwm driver (%d)\n", ret);
		return ret;
	}    

    return RSUCCESS;
}

 
static void __exit pwm_mod_exit(void)
{
    printk("MediaTek MT3351 pwm driver unregister, version %s\n", VERSION);
    driver_unregister(&pwm_driver);
    /* power down pwm */
    pwm_power_down();
    printk("Done\n");
}

module_init(pwm_mod_init);
module_exit(pwm_mod_exit);
MODULE_AUTHOR("Koshi, Chiu <koshi.chiu@mediatek.com>");
MODULE_DESCRIPTION("MT3351 Pulse-Width Modulation Driver (PWM)");
MODULE_LICENSE("GPL");
