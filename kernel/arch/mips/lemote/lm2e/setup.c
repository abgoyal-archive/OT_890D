
#include <linux/bootmem.h>
#include <linux/init.h>
#include <linux/irq.h>

#include <asm/bootinfo.h>
#include <asm/mc146818-time.h>
#include <asm/time.h>
#include <asm/wbflush.h>
#include <asm/mach-lemote/pci.h>

#ifdef CONFIG_VT
#include <linux/console.h>
#include <linux/screen_info.h>
#endif

extern void mips_reboot_setup(void);

unsigned long cpu_clock_freq;
unsigned long bus_clock;
unsigned int memsize;
unsigned int highmemsize = 0;

void __init plat_time_init(void)
{
	/* setup mips r4k timer */
	mips_hpt_frequency = cpu_clock_freq / 2;
}

unsigned long read_persistent_clock(void)
{
	return mc146818_get_cmos_time();
}

void (*__wbflush)(void);
EXPORT_SYMBOL(__wbflush);

static void wbflush_loongson2e(void)
{
	asm(".set\tpush\n\t"
	    ".set\tnoreorder\n\t"
	    ".set mips3\n\t"
	    "sync\n\t"
	    "nop\n\t"
	    ".set\tpop\n\t"
	    ".set mips0\n\t");
}

void __init plat_mem_setup(void)
{
	set_io_port_base((unsigned long)ioremap(LOONGSON2E_IO_PORT_BASE,
				IO_SPACE_LIMIT - LOONGSON2E_PCI_IO_START + 1));
	mips_reboot_setup();

	__wbflush = wbflush_loongson2e;

	add_memory_region(0x0, (memsize << 20), BOOT_MEM_RAM);
#ifdef CONFIG_64BIT
	if (highmemsize > 0) {
		add_memory_region(0x20000000, highmemsize << 20, BOOT_MEM_RAM);
	}
#endif

#ifdef CONFIG_VT
#if defined(CONFIG_VGA_CONSOLE)
	conswitchp = &vga_con;

	screen_info = (struct screen_info) {
		0, 25,		/* orig-x, orig-y */
		    0,		/* unused */
		    0,		/* orig-video-page */
		    0,		/* orig-video-mode */
		    80,		/* orig-video-cols */
		    0, 0, 0,	/* ega_ax, ega_bx, ega_cx */
		    25,		/* orig-video-lines */
		    VIDEO_TYPE_VGAC,	/* orig-video-isVGA */
		    16		/* orig-video-points */
	};
#elif defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
#endif

}
