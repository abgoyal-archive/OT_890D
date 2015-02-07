

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/ide.h>

#include <asm/macintosh.h>
#include <asm/macints.h>
#include <asm/mac_baboon.h>

#define IDE_BASE 0x50F1A000	/* Base address of IDE controller */


#define IDE_CONTROL	0x38	/* control/altstatus */



#define IDE_IFR		0x101	/* (0x101) IDE interrupt flags on Quadra:
				 *
				 * Bit 0+1: some interrupt flags
				 * Bit 2+3: some interrupt enable
				 * Bit 4:   ??
				 * Bit 5:   IDE interrupt flag (any hwif)
				 * Bit 6:   maybe IDE interrupt enable (any hwif) ??
				 * Bit 7:   Any interrupt condition
				 */

volatile unsigned char *ide_ifr = (unsigned char *) (IDE_BASE + IDE_IFR);

int macide_ack_intr(ide_hwif_t* hwif)
{
	if (*ide_ifr & 0x20) {
		*ide_ifr &= ~0x20;
		return 1;
	}
	return 0;
}

static void __init macide_setup_ports(hw_regs_t *hw, unsigned long base,
				      int irq, ide_ack_intr_t *ack_intr)
{
	int i;

	memset(hw, 0, sizeof(*hw));

	for (i = 0; i < 8; i++)
		hw->io_ports_array[i] = base + i * 4;

	hw->io_ports.ctl_addr = base + IDE_CONTROL;

	hw->irq = irq;
	hw->ack_intr = ack_intr;

	hw->chipset = ide_generic;
}

static const char *mac_ide_name[] =
	{ "Quadra", "Powerbook", "Powerbook Baboon" };


static int __init macide_init(void)
{
	ide_ack_intr_t *ack_intr;
	unsigned long base;
	int irq;
	hw_regs_t hw, *hws[] = { &hw, NULL, NULL, NULL };

	if (!MACH_IS_MAC)
		return -ENODEV;

	switch (macintosh_config->ide_type) {
	case MAC_IDE_QUADRA:
		base = IDE_BASE;
		ack_intr = macide_ack_intr;
		irq = IRQ_NUBUS_F;
		break;
	case MAC_IDE_PB:
		base = IDE_BASE;
		ack_intr = macide_ack_intr;
		irq = IRQ_NUBUS_C;
		break;
	case MAC_IDE_BABOON:
		base = BABOON_BASE;
		ack_intr = NULL;
		irq = IRQ_BABOON_1;
		break;
	default:
		return -ENODEV;
	}

	printk(KERN_INFO "ide: Macintosh %s IDE controller\n",
			 mac_ide_name[macintosh_config->ide_type - 1]);

	macide_setup_ports(&hw, base, irq, ack_intr);

	return ide_host_add(NULL, hws, NULL);
}

module_init(macide_init);

MODULE_LICENSE("GPL");
