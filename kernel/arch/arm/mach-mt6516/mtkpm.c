


#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/suspend.h>


#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>

#include <mach/mtkpm.h>















static int mtk_pm_begin(suspend_state_t state)
{
	_Chip_pm_begin();
    return 0;
}



static int mtk_pm_prepare(void)
{
	_Chip_pm_prepare();
    return 0;    
}



static int mtk_pm_enter(suspend_state_t state)
{
	_Chip_pm_enter(state);
	return 0;
}


static void mtk_pm_finish(void)
{
	_Chip_pm_finish();    
}

static void mtk_pm_end(void)
{
	_Chip_pm_end();    
}



static int mtk_pm_state_valid(suspend_state_t pm_state)
{
    return pm_state == PM_SUSPEND_MEM ;
}




static struct platform_suspend_ops mtk_pm_ops = {
	.valid      = mtk_pm_state_valid,
	.begin		= mtk_pm_begin,
	.prepare	= mtk_pm_prepare,
	.enter		= mtk_pm_enter,
	.finish		= mtk_pm_finish,
	.end		= mtk_pm_end,
};


void mtk_pm_init(void)
{
	_Chip_PM_init();
    /* Register and set suspend operation */
    suspend_set_ops(&mtk_pm_ops); 
}    




