

#include <linux/cpu.h>

#include <asm/cacheflush.h>
#include <asm/blackfin.h>
#include <asm/cplb.h>
#include <asm/cplbinit.h>

#if defined(CONFIG_BFIN_ICACHE)
void __cpuinit bfin_icache_init(struct cplb_entry *icplb_tbl)
{
	unsigned long ctrl;
	int i;

	SSYNC();
	for (i = 0; i < MAX_CPLBS; i++) {
		bfin_write32(ICPLB_ADDR0 + i * 4, icplb_tbl[i].addr);
		bfin_write32(ICPLB_DATA0 + i * 4, icplb_tbl[i].data);
	}
	ctrl = bfin_read_IMEM_CONTROL();
	ctrl |= IMC | ENICPLB;
	bfin_write_IMEM_CONTROL(ctrl);
	SSYNC();
}
#endif

#if defined(CONFIG_BFIN_DCACHE)
void __cpuinit bfin_dcache_init(struct cplb_entry *dcplb_tbl)
{
	unsigned long ctrl;
	int i;

	SSYNC();
	for (i = 0; i < MAX_CPLBS; i++) {
		bfin_write32(DCPLB_ADDR0 + i * 4, dcplb_tbl[i].addr);
		bfin_write32(DCPLB_DATA0 + i * 4, dcplb_tbl[i].data);
	}

	ctrl = bfin_read_DMEM_CONTROL();
	ctrl |= DMEM_CNTR;
	bfin_write_DMEM_CONTROL(ctrl);
	SSYNC();
}
#endif
