

#include <linux/kernel.h>

#include <asm/rtc.h>
#include <asm/lv1call.h>
#include <asm/ps3.h>

#include "platform.h"

#define dump_tm(_a) _dump_tm(_a, __func__, __LINE__)
static void _dump_tm(const struct rtc_time *tm, const char* func, int line)
{
	pr_debug("%s:%d tm_sec  %d\n", func, line, tm->tm_sec);
	pr_debug("%s:%d tm_min  %d\n", func, line, tm->tm_min);
	pr_debug("%s:%d tm_hour %d\n", func, line, tm->tm_hour);
	pr_debug("%s:%d tm_mday %d\n", func, line, tm->tm_mday);
	pr_debug("%s:%d tm_mon  %d\n", func, line, tm->tm_mon);
	pr_debug("%s:%d tm_year %d\n", func, line, tm->tm_year);
	pr_debug("%s:%d tm_wday %d\n", func, line, tm->tm_wday);
}

#define dump_time(_a) _dump_time(_a, __func__, __LINE__)
static void __maybe_unused _dump_time(int time, const char *func,
	int line)
{
	struct rtc_time tm;

	to_tm(time, &tm);

	pr_debug("%s:%d time    %d\n", func, line, time);
	_dump_tm(&tm, func, line);
}

void __init ps3_calibrate_decr(void)
{
	int result;
	u64 tmp;

	result = ps3_repository_read_be_tb_freq(0, &tmp);
	BUG_ON(result);

	ppc_tb_freq = tmp;
	ppc_proc_freq = ppc_tb_freq * 40;
}

static u64 read_rtc(void)
{
	int result;
	u64 rtc_val;
	u64 tb_val;

	result = lv1_get_rtc(&rtc_val, &tb_val);
	BUG_ON(result);

	return rtc_val;
}

int ps3_set_rtc_time(struct rtc_time *tm)
{
	u64 now = mktime(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	ps3_os_area_set_rtc_diff(now - read_rtc());
	return 0;
}

void ps3_get_rtc_time(struct rtc_time *tm)
{
	to_tm(read_rtc() + ps3_os_area_get_rtc_diff(), tm);
	tm->tm_year -= 1900;
	tm->tm_mon -= 1;
}

unsigned long __init ps3_get_boot_time(void)
{
	return read_rtc() + ps3_os_area_get_rtc_diff();
}
