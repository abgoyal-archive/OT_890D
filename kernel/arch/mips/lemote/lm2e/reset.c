
#include <linux/pm.h>

#include <asm/reboot.h>

static void loongson2e_restart(char *command)
{
#ifdef CONFIG_32BIT
	*(unsigned long *)0xbfe00104 &= ~(1 << 2);
	*(unsigned long *)0xbfe00104 |= (1 << 2);
#else
	*(unsigned long *)0xffffffffbfe00104 &= ~(1 << 2);
	*(unsigned long *)0xffffffffbfe00104 |= (1 << 2);
#endif
	__asm__ __volatile__("jr\t%0"::"r"(0xbfc00000));
}

static void loongson2e_halt(void)
{
	while (1) ;
}

static void loongson2e_power_off(void)
{
	loongson2e_halt();
}

void mips_reboot_setup(void)
{
	_machine_restart = loongson2e_restart;
	_machine_halt = loongson2e_halt;
	pm_power_off = loongson2e_power_off;
}
