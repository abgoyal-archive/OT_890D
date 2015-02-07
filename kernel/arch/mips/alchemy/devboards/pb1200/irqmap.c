

#include <linux/init.h>
#include <linux/interrupt.h>

#include <asm/mach-au1x00/au1000.h>

#ifdef CONFIG_MIPS_PB1200
#include <asm/mach-pb1x00/pb1200.h>
#endif

#ifdef CONFIG_MIPS_DB1200
#include <asm/mach-db1x00/db1200.h>
#define PB1200_INT_BEGIN DB1200_INT_BEGIN
#define PB1200_INT_END DB1200_INT_END
#endif

struct au1xxx_irqmap __initdata au1xxx_irq_map[] = {
	/* This is external interrupt cascade */
	{ AU1000_GPIO_7, IRQF_TRIGGER_LOW, 0 },
};



static void pb1200_cascade_handler(unsigned int irq, struct irq_desc *d)
{
	unsigned short bisr = bcsr->int_status;

	for ( ; bisr; bisr &= bisr - 1)
		generic_handle_irq(PB1200_INT_BEGIN + __ffs(bisr));
}

static void pb1200_mask_irq(unsigned int irq_nr)
{
	bcsr->intclr_mask = 1 << (irq_nr - PB1200_INT_BEGIN);
	bcsr->intclr = 1 << (irq_nr - PB1200_INT_BEGIN);
	au_sync();
}

static void pb1200_maskack_irq(unsigned int irq_nr)
{
	bcsr->intclr_mask = 1 << (irq_nr - PB1200_INT_BEGIN);
	bcsr->intclr = 1 << (irq_nr - PB1200_INT_BEGIN);
	bcsr->int_status = 1 << (irq_nr - PB1200_INT_BEGIN);	/* ack */
	au_sync();
}

static void pb1200_unmask_irq(unsigned int irq_nr)
{
	bcsr->intset = 1 << (irq_nr - PB1200_INT_BEGIN);
	bcsr->intset_mask = 1 << (irq_nr - PB1200_INT_BEGIN);
	au_sync();
}

static struct irq_chip pb1200_cpld_irq_type = {
#ifdef CONFIG_MIPS_PB1200
	.name = "Pb1200 Ext",
#endif
#ifdef CONFIG_MIPS_DB1200
	.name = "Db1200 Ext",
#endif
	.mask		= pb1200_mask_irq,
	.mask_ack	= pb1200_maskack_irq,
	.unmask		= pb1200_unmask_irq,
};

void __init board_init_irq(void)
{
	unsigned int irq;

	au1xxx_setup_irqmap(au1xxx_irq_map, ARRAY_SIZE(au1xxx_irq_map));

#ifdef CONFIG_MIPS_PB1200
	/* We have a problem with CPLD rev 3. */
	if (((bcsr->whoami & BCSR_WHOAMI_CPLD) >> 4) <= 3) {
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "Pb1200 must be at CPLD rev 4. Please have Pb1200\n");
		printk(KERN_ERR "updated to latest revision. This software will\n");
		printk(KERN_ERR "not work on anything less than CPLD rev 4.\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		printk(KERN_ERR "WARNING!!!\n");
		panic("Game over.  Your score is 0.");
	}
#endif
	/* mask & disable & ack all */
	bcsr->intclr_mask = 0xffff;
	bcsr->intclr = 0xffff;
	bcsr->int_status = 0xffff;
	au_sync();

	for (irq = PB1200_INT_BEGIN; irq <= PB1200_INT_END; irq++)
		set_irq_chip_and_handler_name(irq, &pb1200_cpld_irq_type,
					 handle_level_irq, "level");

	set_irq_chained_handler(AU1000_GPIO_7, pb1200_cascade_handler);
}
