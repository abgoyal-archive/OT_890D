

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/blkdev.h>
#include <linux/bootmem.h>
#include <linux/smp.h>
#include <linux/initrd.h>
#include <linux/pm.h>

#include <asm/bootinfo.h>
#include <asm/reboot.h>

#define MAX_RAM_SIZE ((CONFIG_SIBYTE_STANDALONE_RAM_SIZE * 1024 * 1024) - 1)

static __init void prom_meminit(void)
{
#ifdef CONFIG_BLK_DEV_INITRD
	unsigned long initrd_pstart;
	unsigned long initrd_pend;

	initrd_pstart = __pa(initrd_start);
	initrd_pend = __pa(initrd_end);
	if (initrd_start &&
	    ((initrd_pstart > MAX_RAM_SIZE)
	     || (initrd_pend > MAX_RAM_SIZE))) {
		panic("initrd out of addressable memory");
	}

	add_memory_region(0, initrd_pstart,
			  BOOT_MEM_RAM);
	add_memory_region(initrd_pstart, initrd_pend - initrd_pstart,
			  BOOT_MEM_RESERVED);
	add_memory_region(initrd_pend,
			  (CONFIG_SIBYTE_STANDALONE_RAM_SIZE * 1024 * 1024) - initrd_pend,
			  BOOT_MEM_RAM);
#else
	add_memory_region(0, CONFIG_SIBYTE_STANDALONE_RAM_SIZE * 1024 * 1024,
			  BOOT_MEM_RAM);
#endif
}

void prom_cpu0_exit(void *unused)
{
        while (1) ;
}

static void prom_linux_exit(void)
{
#ifdef CONFIG_SMP
	if (smp_processor_id()) {
		smp_call_function(prom_cpu0_exit, NULL, 1);
	}
#endif
	while(1);
}

void __init prom_init(void)
{
	_machine_restart   = (void (*)(char *))prom_linux_exit;
	_machine_halt      = prom_linux_exit;
	pm_power_off = prom_linux_exit;

	strcpy(arcs_cmdline, "root=/dev/ram0 ");

	prom_meminit();
}

void __init prom_free_prom_memory(void)
{
	/* Not sure what I'm supposed to do here.  Nothing, I think */
}

void prom_putchar(char c)
{
}
