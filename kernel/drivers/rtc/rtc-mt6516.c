

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <asm/tcm.h>

#include <mach/mt6516_reg_base.h>
#include <rtc-mt6516.h>
#include <mach/irqs.h>

#define RTC_DEBUG	RTC_NO

#define RTC_NAME	"mt6516-rtc"

/* RTC registers */
#define RTC_BBPU	(RTC_BASE + 0x0000)
#define RTC_IRQ_STA	(RTC_BASE + 0x0004)
#define RTC_IRQ_EN	(RTC_BASE + 0x0008)
#define RTC_CII_EN	(RTC_BASE + 0x000c)
#define RTC_AL_MASK	(RTC_BASE + 0x0010)
#define RTC_TC_SEC	(RTC_BASE + 0x0014)
#define RTC_TC_MIN	(RTC_BASE + 0x0018)
#define RTC_TC_HOU	(RTC_BASE + 0x001c)
#define RTC_TC_DOM	(RTC_BASE + 0x0020)
#define RTC_TC_DOW	(RTC_BASE + 0x0024)
#define RTC_TC_MTH	(RTC_BASE + 0x0028)
#define RTC_TC_YEA	(RTC_BASE + 0x002c)
#define RTC_AL_SEC	(RTC_BASE + 0x0030)
#define RTC_AL_MIN	(RTC_BASE + 0x0034)
#define RTC_AL_HOU	(RTC_BASE + 0x0038)
#define RTC_AL_DOM	(RTC_BASE + 0x003c)
#define RTC_AL_DOW	(RTC_BASE + 0x0040)
#define RTC_AL_MTH	(RTC_BASE + 0x0044)
#define RTC_AL_YEA	(RTC_BASE + 0x0048)
#define RTC_XOSCCALI	(RTC_BASE + 0x004c)
#define RTC_POWERKEY1	(RTC_BASE + 0x0050)
#define RTC_POWERKEY2	(RTC_BASE + 0x0054)
#define RTC_PDN1	(RTC_BASE + 0x0058)
#define RTC_PDN2	(RTC_BASE + 0x005c)
#define RTC_SPAR1	(RTC_BASE + 0x0064)
#define RTC_PROT	(RTC_BASE + 0x0068)
#define RTC_DIFF	(RTC_BASE + 0x006c)
#define RTC_WRTGR	(RTC_BASE + 0x0074)

#define RTC_BBPU_PWREN		(1U << 0)	/* BBPU = 1 when alarm occurs */
#define RTC_BBPU_WRITE_EN	(1U << 1)
#define RTC_BBPU_BBPU		(1U << 2)	/* 1: power on, 0: power down */
#define RTC_BBPU_AUTO		(1U << 3)
#define RTC_BBPU_CBUSY		(1U << 6)
#define RTC_BBPU_KEY		(0x43 << 8)

#define RTC_IRQ_EN_AL		(1U << 0)
#define RTC_IRQ_EN_ONESHOT	(1U << 2)

/* we map HW YEA 0 (2000) to 1968 not 1970 because 2000 is the leap year */
#define RTC_MIN_YEAR		1968
#define RTC_NUM_YEARS		128
//#define RTC_MAX_YEAR		(RTC_MIN_YEAR + RTC_NUM_YEARS - 1)

#define RTC_MIN_YEAR_OFFSET	(RTC_MIN_YEAR - 1900)




#define rtc_read(addr)		(*(volatile u16 *)(addr))
#define rtc_write(addr, val)	(*(volatile u16 *)(addr) = (u16)(val))

#define RTC_SAY		"rtc: "
#if RTC_DEBUG
#define rtc_print(fmt, arg...)	printk(RTC_SAY fmt, ##arg)
#else
#define rtc_print(fmt, arg...)	do {} while (0)
#endif

static struct rtc_device *rtc;
static DEFINE_SPINLOCK(rtc_lock);
static int rtc_show_time = 0;

static void rtc_writeif_unlock(void)
{
	rtc_write(RTC_PROT, 0x9136);
	rtc_write(RTC_WRTGR, 1);
	while (rtc_read(RTC_BBPU) & RTC_BBPU_CBUSY);	/* 120 us */
}

static void rtc_writeif_lock(void)
{
	rtc_write(RTC_PROT, 0);
	rtc_write(RTC_WRTGR, 1);
	while (rtc_read(RTC_BBPU) & RTC_BBPU_CBUSY);	/* 120 us */
}

/* used in machine_restart() */
void rtc_mark_recovery(void)
{
	u16 pdn1;

	spin_lock_irq(&rtc_lock);
	pdn1 = rtc_read(RTC_PDN1) & ~0x0030;
	pdn1 |= 0x0010;
	rtc_writeif_unlock();
	rtc_write(RTC_PDN1, pdn1);
	rtc_writeif_lock();
	spin_unlock_irq(&rtc_lock);
}

/* used in machine_restart() */
void rtc_mark_swreset(void)
{
	u16 pdn1;

	spin_lock_irq(&rtc_lock);
	pdn1 = rtc_read(RTC_PDN1) & ~0x0030;
	pdn1 |= 0x0020;
	rtc_writeif_unlock();
	rtc_write(RTC_PDN1, pdn1);
	rtc_writeif_lock();
	spin_unlock_irq(&rtc_lock);
}

void rtc_bbpu_power_down(void)
{
	u16 bbpu;

	bbpu = RTC_BBPU_KEY | RTC_BBPU_AUTO | RTC_BBPU_PWREN;

	spin_lock_irq(&rtc_lock);
	rtc_writeif_unlock();
	rtc_write(RTC_BBPU, bbpu);
	//rtc_writeif_lock();
	rtc_write(RTC_WRTGR, 1);
	while (rtc_read(RTC_BBPU) & RTC_BBPU_CBUSY);	/* 120 us */
	spin_unlock_irq(&rtc_lock);
}

static irqreturn_t __tcmfunc rtc_irq_handler(int irq, void *dev_id)
{
	u16 irqsta, pdn1;
	struct rtc_time tm;

	spin_lock(&rtc_lock);
	tm.tm_sec = rtc_read(RTC_TC_SEC);
	tm.tm_min = rtc_read(RTC_TC_MIN);
	tm.tm_hour = rtc_read(RTC_TC_HOU);
	tm.tm_mday = rtc_read(RTC_TC_DOM);
	tm.tm_mon = rtc_read(RTC_TC_MTH);

	irqsta = rtc_read(RTC_IRQ_STA);		/* read clear */
	pdn1 = rtc_read(RTC_PDN1);
	if (pdn1 & 0x0080) {	/* power-on time is available */
		u16 spar1, min, hou, dom, mth;

		spar1 = rtc_read(RTC_SPAR1);
		min = spar1 & 0x003f;
		hou = (spar1 & 0x07c0) >> 6;
		dom = (spar1 & 0xf800) >> 11;
		mth = rtc_read(RTC_PDN2) & 0x000f;

		if (tm.tm_mon == mth && tm.tm_mday == dom &&
		    tm.tm_hour == hou && tm.tm_min == min &&
		    tm.tm_sec >= (30 - 2) && tm.tm_sec <= (30 + 2)) {
			pdn1 &= ~0x0080;
			rtc_writeif_unlock();
			rtc_write(RTC_PDN1, pdn1);
			rtc_writeif_lock();
			pdn1 = 0;
		} else {
			/* set power-on alarm when power-on time is available */
			rtc_writeif_unlock();
			rtc_write(RTC_AL_MTH, mth);
			rtc_write(RTC_AL_DOM, dom);
			rtc_write(RTC_AL_HOU, hou);
			rtc_write(RTC_AL_MIN, min);
			rtc_write(RTC_AL_SEC, 30);
			rtc_write(RTC_AL_MASK, 0x0050);		/* no YEA and DOW */
			rtc_write(RTC_IRQ_EN, RTC_IRQ_EN_AL | RTC_IRQ_EN_ONESHOT);
			rtc_writeif_lock();
		}
	}
	spin_unlock(&rtc_lock);

	if (rtc->irq_task) {
		tm.tm_mon--;
		rtc->irq_task->private_data = &tm;
	}
	rtc_update_irq(rtc, 1, RTC_IRQF | RTC_AF);

	if (rtc_show_time)
		printk(RTC_SAY "%s time is up\n", pdn1 ? "alarm" : "power-on");

	return IRQ_HANDLED;
}

#if RTC_OVER_TIME_RESET
static void rtc_reset_to_deftime(struct rtc_time *tm)
{
	tm->tm_year = RTC_DEFAULT_YEA - 1900;
	tm->tm_mon = RTC_DEFAULT_MTH - 1;
	tm->tm_mday = RTC_DEFAULT_DOM;
	tm->tm_wday = RTC_DEFAULT_DOW;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;

	spin_lock_irq(&rtc_lock);
	rtc_writeif_unlock();
	rtc_write(RTC_TC_YEA, RTC_DEFAULT_YEA - RTC_MIN_YEAR);
	rtc_write(RTC_TC_MTH, RTC_DEFAULT_MTH);
	rtc_write(RTC_TC_DOM, RTC_DEFAULT_DOM);
	rtc_write(RTC_TC_DOW, RTC_DEFAULT_DOW == 0 ? 7 : RTC_DEFAULT_DOW);
	rtc_write(RTC_TC_HOU, 0);
	rtc_write(RTC_TC_MIN, 0);
	rtc_write(RTC_TC_SEC, 0);
	rtc_writeif_lock();
	spin_unlock_irq(&rtc_lock);

	printk(RTC_SAY "reset to default date %04d/%02d/%02d (%d)\n",
	       RTC_DEFAULT_YEA, RTC_DEFAULT_MTH, RTC_DEFAULT_DOM, RTC_DEFAULT_DOW);
}
#endif

static int rtc_ops_read_time(struct device *dev, struct rtc_time *tm)
{
	u16 now_sec;

	spin_lock_irq(&rtc_lock);
	tm->tm_sec = rtc_read(RTC_TC_SEC);
	tm->tm_min = rtc_read(RTC_TC_MIN);
	tm->tm_hour = rtc_read(RTC_TC_HOU);
	tm->tm_wday = rtc_read(RTC_TC_DOW);
	tm->tm_mday = rtc_read(RTC_TC_DOM);
	tm->tm_mon = rtc_read(RTC_TC_MTH);
	tm->tm_year = rtc_read(RTC_TC_YEA);

	now_sec = rtc_read(RTC_TC_SEC);
	if (now_sec - tm->tm_sec < 0) {		/* the second has carried */
		tm->tm_sec = rtc_read(RTC_TC_SEC);
		tm->tm_min = rtc_read(RTC_TC_MIN);
		tm->tm_hour = rtc_read(RTC_TC_HOU);
		tm->tm_wday = rtc_read(RTC_TC_DOW);
		tm->tm_mday = rtc_read(RTC_TC_DOM);
		tm->tm_mon = rtc_read(RTC_TC_MTH);
		tm->tm_year = rtc_read(RTC_TC_YEA);
	}
	spin_unlock_irq(&rtc_lock);

	tm->tm_year += RTC_MIN_YEAR_OFFSET;
	tm->tm_mon--;
	if (tm->tm_wday == 7)	/* sunday */
		tm->tm_wday = 0;

#if RTC_OVER_TIME_RESET
	{
		unsigned long time;
		rtc_tm_to_time(tm, &time);
		if (unlikely(time > (unsigned long)LONG_MAX))
			rtc_reset_to_deftime(tm);
	}
#endif

	if (rtc_show_time) {
		printk(RTC_SAY "read tc time = %04d/%02d/%02d (%d) %02d:%02d:%02d\n",
		       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		       tm->tm_wday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}

	return 0;
}

static int rtc_ops_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long time;

	rtc_tm_to_time(tm, &time);
	if (time > (unsigned long)LONG_MAX || (unsigned)tm->tm_wday > 6)
		return -EINVAL;

	tm->tm_year -= RTC_MIN_YEAR_OFFSET;
	tm->tm_mon++;
	if (tm->tm_wday == 0)	/* sunday */
		tm->tm_wday = 7;

	spin_lock_irq(&rtc_lock);
	rtc_writeif_unlock();
	rtc_write(RTC_TC_YEA, tm->tm_year);
	rtc_write(RTC_TC_MTH, tm->tm_mon);
	rtc_write(RTC_TC_DOM, tm->tm_mday);
	rtc_write(RTC_TC_DOW, tm->tm_wday);
	rtc_write(RTC_TC_HOU, tm->tm_hour);
	rtc_write(RTC_TC_MIN, tm->tm_min);
	rtc_write(RTC_TC_SEC, tm->tm_sec);
	rtc_writeif_lock();
	spin_unlock_irq(&rtc_lock);

	if (rtc_show_time) {
		printk(RTC_SAY "set tc time = %04d/%02d/%02d (%d) %02d:%02d:%02d\n",
		       tm->tm_year + RTC_MIN_YEAR, tm->tm_mon, tm->tm_mday,
		       tm->tm_wday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}

	return 0;
}

static int rtc_ops_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	u16 irqen;
	struct rtc_time *tm = &alm->time;

	spin_lock_irq(&rtc_lock);
	irqen = rtc_read(RTC_IRQ_EN);
	tm->tm_sec = rtc_read(RTC_AL_SEC);
	tm->tm_min = rtc_read(RTC_AL_MIN);
	tm->tm_hour = rtc_read(RTC_AL_HOU);
	tm->tm_mday = rtc_read(RTC_AL_DOM);
	tm->tm_mon = rtc_read(RTC_AL_MTH);
	tm->tm_year = rtc_read(RTC_AL_YEA);
	spin_unlock_irq(&rtc_lock);

	alm->enabled = !!(irqen & RTC_IRQ_EN_AL);
	tm->tm_year += RTC_MIN_YEAR_OFFSET;
	tm->tm_mon--;

	rtc_print("read al time = %04d/%02d/%02d %02d:%02d:%02d\n",
	          tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	          tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;
}

static void rtc_save_pwron_time(bool enabled, struct rtc_time *tm)
{
	u16 pdn1, pdn2, spare;

	pdn2 = rtc_read(RTC_PDN2) & 0x00f0;
	pdn2 |= tm->tm_mon;
	spare = (tm->tm_mday << 11) | (tm->tm_hour << 6) | tm->tm_min;

	rtc_writeif_unlock();
	rtc_write(RTC_PDN2, pdn2);
	rtc_write(RTC_SPAR1, spare);
	if (enabled) {
		pdn1 = rtc_read(RTC_PDN1) | 0x0080;
		rtc_write(RTC_PDN1, pdn1);
	} else {
		pdn1 = rtc_read(RTC_PDN1) & ~0x0080;
		rtc_write(RTC_PDN1, pdn1);
	}
	rtc_writeif_lock();
}

static int rtc_ops_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	u16 irqsta;
	unsigned long time;
	struct rtc_time *tm = &alm->time;

	rtc_tm_to_time(tm, &time);
	if (time > (unsigned long)LONG_MAX)
		return -EINVAL;

	tm->tm_year -= RTC_MIN_YEAR_OFFSET;
	tm->tm_mon++;

	if (rtc_show_time) {
		printk(RTC_SAY "set al time = %04d/%02d/%02d %02d:%02d:%02d (%d)\n",
		       tm->tm_year + RTC_MIN_YEAR, tm->tm_mon, tm->tm_mday,
		       tm->tm_hour, tm->tm_min, tm->tm_sec, alm->enabled);
	}

	spin_lock_irq(&rtc_lock);
	if (alm->enabled == 2 || alm->enabled == 3) {	/* for power-on alarm */
		if (alm->enabled == 3)
			alm->enabled = 0;

		rtc_save_pwron_time(alm->enabled, tm);
	}

	irqsta = rtc_read(RTC_IRQ_STA);		/* read clear */
	rtc_writeif_unlock();
	rtc_write(RTC_AL_MTH, tm->tm_mon);
	rtc_write(RTC_AL_DOM, tm->tm_mday);
	rtc_write(RTC_AL_HOU, tm->tm_hour);
	rtc_write(RTC_AL_MIN, tm->tm_min);
	rtc_write(RTC_AL_SEC, tm->tm_sec);
	if (alm->enabled) {
		rtc_write(RTC_AL_MASK, 0x0050);		/* no YEA and DOW */
		rtc_write(RTC_IRQ_EN, RTC_IRQ_EN_AL | RTC_IRQ_EN_ONESHOT);
	} else {
		/*
		 * if RTC_AL_MASK = 0x007f and RTC_BBPU.PWREN = 1, alarm comes
		 * EVERY SECOND, not disabled
		 */
		rtc_write(RTC_AL_MASK, 0);
		rtc_write(RTC_IRQ_EN, 0);
	}
	rtc_writeif_lock();
	spin_unlock_irq(&rtc_lock);

	return 0;
}

static struct rtc_class_ops rtc_ops = {
	.read_time	= rtc_ops_read_time,
	.set_time	= rtc_ops_set_time,
	.read_alarm	= rtc_ops_read_alarm,
	.set_alarm	= rtc_ops_set_alarm,
};

static int rtc_pdrv_probe(struct platform_device *pdev)
{
	int r;

	/* register rtc device (/dev/rtc0) */
	rtc = rtc_device_register(RTC_NAME, &pdev->dev, &rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		printk(RTC_SAY "register rtc device failed (%ld)\n", PTR_ERR(rtc));
		return PTR_ERR(rtc);
	}

	r = request_irq(MT6516_RTC_IRQ_LINE, rtc_irq_handler, 0, RTC_NAME, NULL);
	if (r) {
		printk(RTC_SAY "register IRQ failed (%d)\n", r);
		rtc_device_unregister(rtc);
		return r;
	}

	return 0;
}

/* should never be called */
static int rtc_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rtc_pdrv = {
	.probe		= rtc_pdrv_probe,
	.remove		= rtc_pdrv_remove,
	.driver		= {
		.name	= RTC_NAME,
		.owner	= THIS_MODULE,
	},
};

static struct platform_device rtc_pdev = {
	.name	= RTC_NAME,
	.id	= -1,
};

static int __init rtc_mod_init(void)
{
	int r;

	r = platform_device_register(&rtc_pdev);
	if (r) {
		printk(RTC_SAY "register device failed (%d)\n", r);
		return r;
	}

	r = platform_driver_register(&rtc_pdrv);
	if (r) {
		printk(RTC_SAY "register driver failed (%d)\n", r);
		platform_device_unregister(&rtc_pdev);
		return r;
	}

	return 0;
}

/* should never be called */
static void __exit rtc_mod_exit(void)
{
}

module_init(rtc_mod_init);
module_exit(rtc_mod_exit);

module_param(rtc_show_time, bool, 0644);

MODULE_AUTHOR("Terry Chang <terry.chang@mediatek.com>");
MODULE_DESCRIPTION ("MT6516 RTC Driver v2.2");
MODULE_LICENSE ("GPL");
