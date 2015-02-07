
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#include <asm/openprom.h>
#include <asm/oplib.h>

int
prom_devopen(const char *dstr)
{
	return p1275_cmd ("open", P1275_ARG(0,P1275_ARG_IN_STRING)|
				  P1275_INOUT(1,1),
				  dstr);
}

/* Close the device described by device handle 'dhandle'. */
int
prom_devclose(int dhandle)
{
	p1275_cmd ("close", P1275_INOUT(1,0), dhandle);
	return 0;
}

void
prom_seek(int dhandle, unsigned int seekhi, unsigned int seeklo)
{
	p1275_cmd ("seek", P1275_INOUT(3,1), dhandle, seekhi, seeklo);
}
