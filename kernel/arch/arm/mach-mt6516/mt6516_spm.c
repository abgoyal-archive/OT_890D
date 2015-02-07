

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <mach/mt6516_ap_config.h>
#include <linux/delay.h>
#include <linux/spm.h>

#define PFX "mt6516_spm: "

#if (CONFIG_DEBUG_SPM==1)
#define dbgmsg(msg...) printk(PFX msg)
#else
#define dbgmsg(...)
#endif

#define warnmsg(msg...) printk(KERN_WARNING PFX msg);
#define errmsg(msg...) printk(KERN_WARNING PFX msg);

extern UINT32 SetARM9Freq(ARM9_FREQ_DIV_ENUM ARM9_div);
extern ssize_t	mt6326_VCORE_1_set_1_05(void);
extern ssize_t	mt6326_VCORE_1_set_1_10(void);
extern ssize_t	mt6326_VCORE_1_set_1_3(void); 

const struct spm_perf perf_tbl[] = {
		/* E.g. 104 => 104 MHz, 101 => 1.05 v */
		{{104, 105}, {0, 0}},
		{{208, 110}, {0, 0}},
		{{416, 130}, {0, 0}},
		SPM_PERF_TBL_END
};

static int mt6516_spm_init(void)
{
	int ret;
	ret = spm_set_curr_perf(&perf_tbl[2]);
	return ret;
}

int mt6516_set_cpu_voltage(unsigned int voltage)
{
	if (voltage == 130) {
		mt6326_VCORE_1_set_1_3();	
	}
	else if (voltage == 110) {
		mt6326_VCORE_1_set_1_10();
	}
	else if (voltage == 105) {
		mt6326_VCORE_1_set_1_05();
	}
	else {
		return 0;
	}
	udelay(1000);
	return 0;
}

int mt6516_set_cpu_freq(unsigned int freq)
{
	if (freq == 416) {
		SetARM9Freq(DIV_1_416);
	}
	else if (freq == 208) {
		SetARM9Freq(DIV_2_208);
	}
	else if (freq == 104) {
		SetARM9Freq(DIV_4_104);
	}
	else {
		return 0;
	}
	return 0;
}

static const struct spm_perf* mt6516_get_perf_tbl(void)
{	
	return perf_tbl;
}

static int mt6516_spm_change_perf(const struct spm_perf *curr, const struct spm_perf *next)
{
	char spm_log[100];
	int ret = 0; 
	
	SPM_ASSERT(curr);
	SPM_ASSERT(next);

	if (curr->cpu.freq > next->cpu.freq) {

		mt6516_set_cpu_freq(next->cpu.freq);
		mt6516_set_cpu_voltage(next->cpu.voltage);
	} 
	else if (curr->cpu.freq < next->cpu.freq) {

		mt6516_set_cpu_voltage(next->cpu.voltage);
		mt6516_set_cpu_freq(next->cpu.freq);
	}
	else {
		return 0;
	}
	

	return 0;
}

static int mt6516_spm_suspend(void)
{
	return 0;
}

static int mt6516_spm_resume(void)
{
	return 0;
}

static struct spm_driver mt6516_spm_drv = {
			.name = "MT6516_SPM_DRV",
			.init = mt6516_spm_init,
			.get_perf_tbl = mt6516_get_perf_tbl,
			.change_perf = mt6516_spm_change_perf,
			.suspend = mt6516_spm_suspend,
			.resume = mt6516_spm_resume

		};

SPM_REGISTER_DRIVER(mt6516_spm_drv);
