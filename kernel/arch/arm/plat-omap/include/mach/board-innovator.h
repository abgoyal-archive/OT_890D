
#ifndef __ASM_ARCH_OMAP_INNOVATOR_H
#define __ASM_ARCH_OMAP_INNOVATOR_H

#if defined (CONFIG_ARCH_OMAP15XX)

#ifndef OMAP_SDRAM_DEVICE
#define OMAP_SDRAM_DEVICE			D256M_1X16_4B
#endif

#define OMAP1510P1_IMIF_PRI_VALUE		0x00
#define OMAP1510P1_EMIFS_PRI_VALUE		0x00
#define OMAP1510P1_EMIFF_PRI_VALUE		0x00

#ifndef __ASSEMBLY__
void fpga_write(unsigned char val, int reg);
unsigned char fpga_read(int reg);
#endif

#endif /* CONFIG_ARCH_OMAP15XX */

#if defined (CONFIG_ARCH_OMAP16XX)

/* At OMAP1610 Innovator the Ethernet is directly connected to CS1 */
#define INNOVATOR1610_ETHR_START	0x04000300

#endif /* CONFIG_ARCH_OMAP1610 */
#endif /* __ASM_ARCH_OMAP_INNOVATOR_H */
