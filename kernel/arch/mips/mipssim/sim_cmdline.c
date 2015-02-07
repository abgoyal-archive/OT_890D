
#include <linux/init.h>
#include <linux/string.h>
#include <asm/bootinfo.h>

extern char arcs_cmdline[];

char * __init prom_getcmdline(void)
{
	return arcs_cmdline;
}

void  __init prom_init_cmdline(void)
{
	/* XXX: Get boot line from environment? */
}
