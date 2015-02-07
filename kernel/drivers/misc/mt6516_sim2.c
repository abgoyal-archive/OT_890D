

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_ap_config.h>

#define VERSION					        "v 0.1"
#define SIM2_DEVICE	        			"mt6516_sim2"

int req_sim2_ownership(void)
{
	int read_reg=0;

	//#if defined(CONFIG_MT6516_GEMINI_BOARD)
	read_reg = DRV_Reg32(HW_MISC);
	printk("req_sim2_ownership: %x\n", read_reg);

	DRV_SetReg32(HW_MISC, 0x0080);

	read_reg = DRV_Reg32(HW_MISC);
	printk("req_sim2_ownership: %x\n", read_reg);
	
	if ((read_reg >> 7) & 0x1) {
		printk("req_sim2_ownership: PASS!\n");		
	}
	else {
		printk("req_sim2_ownership: Fail!\n");
		return -1;
	}
	//#endif
	
	return 0;
}

static int mt6516_sim2_probe(struct device *dev)
{
	//int ret;

	printk("MediaTek MT6516 SIM2 driver mt6516_sim2_probe, version %s\n", VERSION);

	#if 0
	/* For local test */
	ret = req_sim2_ownership();
	if (ret) {
		printk("MediaTek MT6516 SIM2 : req_sim2_ownership() FAIL!\n");
		return ret;
	}	
	#endif
	
	return 0;
}

static int mt6516_sim2_remove(struct device *dev)
{
	printk("MediaTek MT6516 SIM2 driver mt6516_sim2_remove, version %s\n", VERSION);

	return 0;
}

static int mt6516_sim2_suspend(struct device *dev, pm_message_t state)
{
	printk("MediaTek MT6516 SIM2 driver mt6516_sim2_suspend, version %s\n", VERSION);
	return 0;
}

static int mt6516_sim2_resume(struct device *dev)
{
	printk("MediaTek MT6516 SIM2 driver mt6516_sim2_resume, version %s\n", VERSION);
	return 0;
}

static struct device_driver mt6516_sim2_driver = {
	.name		= SIM2_DEVICE,
	.bus		= &platform_bus_type,
	.probe		= mt6516_sim2_probe,
	.remove		= mt6516_sim2_remove,
	#ifdef CONFIG_PM
	.suspend	= mt6516_sim2_suspend,
	.resume		= mt6516_sim2_resume,
	#endif
};

struct platform_device MT6516_sim2_device = {
		.name				= SIM2_DEVICE,
		.id					= 0,
		.dev				= {
		}
};

static s32 __devinit sim2_mod_init(void)
{	
	s32 ret;

	printk("MediaTek MT6516 SIM2 driver register, version %s.\n", VERSION);

	ret = platform_device_register(&MT6516_sim2_device);
	if (ret) {
		printk("****[MT6516_sim2_device] Unable to device register(%d)\n", ret);
		return ret;
	}
	
	ret = driver_register(&mt6516_sim2_driver);
	if (ret) {
		printk("****[mt6516_sim2_driver] Unable to register driver (%d)\n", ret);
		return ret;
	}

	printk("Done\n");
 
    return 0;
}

static void __exit sim2_mod_exit(void)
{
	printk("MediaTek MT6516 SIM2 driver unregister, version %s\n", VERSION);
	printk("Done\n");
}

module_init(sim2_mod_init);
module_exit(sim2_mod_exit);
EXPORT_SYMBOL(req_sim2_ownership);
MODULE_DESCRIPTION("MT6516 SIM2 Driver");
MODULE_LICENSE("Proprietary");
