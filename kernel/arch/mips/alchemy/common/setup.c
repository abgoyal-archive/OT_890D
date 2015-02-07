

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/pm.h>

#include <asm/mipsregs.h>
#include <asm/reboot.h>
#include <asm/time.h>

#include <au1000.h>

extern void __init board_setup(void);
extern void au1000_restart(char *);
extern void au1000_halt(void);
extern void au1000_power_off(void);
extern void set_cpuspec(void);

void __init plat_mem_setup(void)
{
	unsigned long est_freq;

	/* determine core clock */
	est_freq = au1xxx_calc_clock();
	est_freq += 5000;    /* round */
	est_freq -= est_freq % 10000;
	printk(KERN_INFO "(PRId %08x) @ %lu.%02lu MHz\n", read_c0_prid(),
	       est_freq / 1000000, ((est_freq % 1000000) * 100) / 1000000);

	_machine_restart = au1000_restart;
	_machine_halt = au1000_halt;
	pm_power_off = au1000_power_off;

	board_setup();  /* board specific setup */

	if (au1xxx_cpu_needs_config_od())
		/* Various early Au1xx0 errata corrected by this */
		set_c0_config(1 << 19); /* Set Config[OD] */
	else
		/* Clear to obtain best system bus performance */
		clear_c0_config(1 << 19); /* Clear Config[OD] */

	/* IO/MEM resources. */
	set_io_port_base(0);
	ioport_resource.start = IOPORT_RESOURCE_START;
	ioport_resource.end = IOPORT_RESOURCE_END;
	iomem_resource.start = IOMEM_RESOURCE_START;
	iomem_resource.end = IOMEM_RESOURCE_END;
}

#if defined(CONFIG_64BIT_PHYS_ADDR)
/* This routine should be valid for all Au1x based boards */
phys_t __fixup_bigphys_addr(phys_t phys_addr, phys_t size)
{
	/* Don't fixup 36-bit addresses */
	if ((phys_addr >> 32) != 0)
		return phys_addr;

#ifdef CONFIG_PCI
	{
		u32 start = (u32)Au1500_PCI_MEM_START;
		u32 end   = (u32)Au1500_PCI_MEM_END;

		/* Check for PCI memory window */
		if (phys_addr >= start && (phys_addr + size - 1) <= end)
			return (phys_t)
			       ((phys_addr - start) + Au1500_PCI_MEM_START);
	}
#endif

	/*
	 * All Au1xx0 SOCs have a PCMCIA controller.
	 * We setup our 32-bit pseudo addresses to be equal to the
	 * 36-bit addr >> 4, to make it easier to check the address
	 * and fix it.
	 * The PCMCIA socket 0 physical attribute address is 0xF 4000 0000.
	 * The pseudo address we use is 0xF400 0000. Any address over
	 * 0xF400 0000 is a PCMCIA pseudo address.
	 */
	if ((phys_addr >= 0xF4000000) && (phys_addr < 0xFFFFFFFF))
		return (phys_t)(phys_addr << 4);

	/* default nop */
	return phys_addr;
}
EXPORT_SYMBOL(__fixup_bigphys_addr);
#endif
