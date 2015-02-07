

#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/mach-au1x00/au1000.h>

char irq_tab_alchemy[][5] __initdata = {
	[0] = { -1, INTA, INTA, INTX, INTX }, /* IDSEL 00 - AdapterA-Slot0 (top) */
	[1] = { -1, INTB, INTA, INTX, INTX }, /* IDSEL 01 - AdapterA-Slot1 (bottom) */
	[2] = { -1, INTC, INTD, INTX, INTX }, /* IDSEL 02 - AdapterB-Slot0 (top) */
	[3] = { -1, INTD, INTC, INTX, INTX }, /* IDSEL 03 - AdapterB-Slot1 (bottom) */
	[4] = { -1, INTA, INTB, INTX, INTX }, /* IDSEL 04 - AdapterC-Slot0 (top) */
	[5] = { -1, INTB, INTA, INTX, INTX }, /* IDSEL 05 - AdapterC-Slot1 (bottom) */
	[6] = { -1, INTC, INTD, INTX, INTX }, /* IDSEL 06 - AdapterD-Slot0 (top) */
	[7] = { -1, INTD, INTC, INTX, INTX }, /* IDSEL 07 - AdapterD-Slot1 (bottom) */
};

struct au1xxx_irqmap __initdata au1xxx_irq_map[] = {
       { AU1500_GPIO_204, IRQF_TRIGGER_HIGH, 0 },
       { AU1500_GPIO_201, IRQF_TRIGGER_LOW, 0 },
       { AU1500_GPIO_202, IRQF_TRIGGER_LOW, 0 },
       { AU1500_GPIO_203, IRQF_TRIGGER_LOW, 0 },
       { AU1500_GPIO_205, IRQF_TRIGGER_LOW, 0 },
};


void __init board_init_irq(void)
{
	au1xxx_setup_irqmap(au1xxx_irq_map, ARRAY_SIZE(au1xxx_irq_map));
}
