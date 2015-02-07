
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#ifdef CONFIG_HAVE_TCM 
 /*
 * TCM memory whereabouts
 */
#define ITCM_SIZE	0x00004000
#define ITCM_OFFSET	(PAGE_OFFSET - ITCM_SIZE)
#define ITCM_END	(PAGE_OFFSET - 1)
#define DTCM_SIZE	0x00004000
#define DTCM_OFFSET	(ITCM_OFFSET - DTCM_SIZE)
#define DTCM_END	(ITCM_OFFSET - 1)
#endif

#define PHYS_OFFSET	(0x00600000)
#define BUS_OFFSET	(0x00000000)

#define PCI0_BASE           0

#endif
