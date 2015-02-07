

#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/stddef.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/highmem.h>

#include <asm/pgalloc.h>
#include <asm/prom.h>
#include <asm/io.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/mmu.h>
#include <asm/uaccess.h>
#include <asm/smp.h>
#include <asm/machdep.h>
#include <asm/setup.h>

#include "mmu_decl.h"

extern void loadcam_entry(unsigned int index);
unsigned int tlbcam_index;
static unsigned long __cam0, __cam1, __cam2;

#define NUM_TLBCAMS	(16)

struct tlbcam TLBCAM[NUM_TLBCAMS];

struct tlbcamrange {
   	unsigned long start;
	unsigned long limit;
	phys_addr_t phys;
} tlbcam_addrs[NUM_TLBCAMS];

extern unsigned int tlbcam_index;

phys_addr_t v_mapped_by_tlbcam(unsigned long va)
{
	int b;
	for (b = 0; b < tlbcam_index; ++b)
		if (va >= tlbcam_addrs[b].start && va < tlbcam_addrs[b].limit)
			return tlbcam_addrs[b].phys + (va - tlbcam_addrs[b].start);
	return 0;
}

unsigned long p_mapped_by_tlbcam(phys_addr_t pa)
{
	int b;
	for (b = 0; b < tlbcam_index; ++b)
		if (pa >= tlbcam_addrs[b].phys
	    	    && pa < (tlbcam_addrs[b].limit-tlbcam_addrs[b].start)
		              +tlbcam_addrs[b].phys)
			return tlbcam_addrs[b].start+(pa-tlbcam_addrs[b].phys);
	return 0;
}

void settlbcam(int index, unsigned long virt, phys_addr_t phys,
		unsigned int size, int flags, unsigned int pid)
{
	unsigned int tsize, lz;

	asm ("cntlzw %0,%1" : "=r" (lz) : "r" (size));
	tsize = (21 - lz) / 2;

#ifdef CONFIG_SMP
	if ((flags & _PAGE_NO_CACHE) == 0)
		flags |= _PAGE_COHERENT;
#endif

	TLBCAM[index].MAS0 = MAS0_TLBSEL(1) | MAS0_ESEL(index) | MAS0_NV(index+1);
	TLBCAM[index].MAS1 = MAS1_VALID | MAS1_IPROT | MAS1_TSIZE(tsize) | MAS1_TID(pid);
	TLBCAM[index].MAS2 = virt & PAGE_MASK;

	TLBCAM[index].MAS2 |= (flags & _PAGE_WRITETHRU) ? MAS2_W : 0;
	TLBCAM[index].MAS2 |= (flags & _PAGE_NO_CACHE) ? MAS2_I : 0;
	TLBCAM[index].MAS2 |= (flags & _PAGE_COHERENT) ? MAS2_M : 0;
	TLBCAM[index].MAS2 |= (flags & _PAGE_GUARDED) ? MAS2_G : 0;
	TLBCAM[index].MAS2 |= (flags & _PAGE_ENDIAN) ? MAS2_E : 0;

	TLBCAM[index].MAS3 = (phys & PAGE_MASK) | MAS3_SX | MAS3_SR;
	TLBCAM[index].MAS3 |= ((flags & _PAGE_RW) ? MAS3_SW : 0);

#ifndef CONFIG_KGDB /* want user access for breakpoints */
	if (flags & _PAGE_USER) {
	   TLBCAM[index].MAS3 |= MAS3_UX | MAS3_UR;
	   TLBCAM[index].MAS3 |= ((flags & _PAGE_RW) ? MAS3_UW : 0);
	}
#else
	TLBCAM[index].MAS3 |= MAS3_UX | MAS3_UR;
	TLBCAM[index].MAS3 |= ((flags & _PAGE_RW) ? MAS3_UW : 0);
#endif

	tlbcam_addrs[index].start = virt;
	tlbcam_addrs[index].limit = virt + size - 1;
	tlbcam_addrs[index].phys = phys;

	loadcam_entry(index);
}

void invalidate_tlbcam_entry(int index)
{
	TLBCAM[index].MAS0 = MAS0_TLBSEL(1) | MAS0_ESEL(index);
	TLBCAM[index].MAS1 = ~MAS1_VALID;

	loadcam_entry(index);
}

void __init cam_mapin_ram(unsigned long cam0, unsigned long cam1,
		unsigned long cam2)
{
	settlbcam(0, PAGE_OFFSET, memstart_addr, cam0, _PAGE_KERNEL, 0);
	tlbcam_index++;
	if (cam1) {
		tlbcam_index++;
		settlbcam(1, PAGE_OFFSET+cam0, memstart_addr+cam0, cam1, _PAGE_KERNEL, 0);
	}
	if (cam2) {
		tlbcam_index++;
		settlbcam(2, PAGE_OFFSET+cam0+cam1, memstart_addr+cam0+cam1, cam2, _PAGE_KERNEL, 0);
	}
}

void __init MMU_init_hw(void)
{
	flush_instruction_cache();
}

unsigned long __init mmu_mapin_ram(void)
{
	cam_mapin_ram(__cam0, __cam1, __cam2);

	return __cam0 + __cam1 + __cam2;
}


void __init
adjust_total_lowmem(void)
{
	phys_addr_t max_lowmem_size = __max_low_memory;
	phys_addr_t cam_max_size = 0x10000000;
	phys_addr_t ram;

	/* adjust CAM size to max_lowmem_size */
	if (max_lowmem_size < cam_max_size)
		cam_max_size = max_lowmem_size;

	/* adjust lowmem size to max_lowmem_size */
	ram = min(max_lowmem_size, total_lowmem);

	/* Calculate CAM values */
	__cam0 = 1UL << 2 * (__ilog2(ram) / 2);
	if (__cam0 > cam_max_size)
		__cam0 = cam_max_size;
	ram -= __cam0;
	if (ram) {
		__cam1 = 1UL << 2 * (__ilog2(ram) / 2);
		if (__cam1 > cam_max_size)
			__cam1 = cam_max_size;
		ram -= __cam1;
	}
	if (ram) {
		__cam2 = 1UL << 2 * (__ilog2(ram) / 2);
		if (__cam2 > cam_max_size)
			__cam2 = cam_max_size;
		ram -= __cam2;
	}

	printk(KERN_INFO "Memory CAM mapping: CAM0=%ldMb, CAM1=%ldMb,"
			" CAM2=%ldMb residual: %ldMb\n",
			__cam0 >> 20, __cam1 >> 20, __cam2 >> 20,
			(long int)((total_lowmem - __cam0 - __cam1 - __cam2)
				   >> 20));
	__max_low_memory = __cam0 + __cam1 + __cam2;
	__initial_memory_limit_addr = memstart_addr + __max_low_memory;
}
