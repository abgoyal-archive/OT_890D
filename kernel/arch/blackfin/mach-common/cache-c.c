

#include <asm/blackfin.h>

void blackfin_invalidate_entire_dcache(void)
{
	u32 dmem = bfin_read_DMEM_CONTROL();
	SSYNC();
	bfin_write_DMEM_CONTROL(dmem & ~0xc);
	SSYNC();
	bfin_write_DMEM_CONTROL(dmem);
	SSYNC();
}
