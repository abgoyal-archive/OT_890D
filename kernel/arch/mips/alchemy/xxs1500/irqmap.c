

#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/mach-au1x00/au1000.h>

struct au1xxx_irqmap __initdata au1xxx_irq_map[] = {
	{ AU1500_GPIO_204, IRQF_TRIGGER_HIGH, 0 },
	{ AU1500_GPIO_201, IRQF_TRIGGER_LOW, 0 },
	{ AU1500_GPIO_202, IRQF_TRIGGER_LOW, 0 },
	{ AU1500_GPIO_203, IRQF_TRIGGER_LOW, 0 },
	{ AU1500_GPIO_205, IRQF_TRIGGER_LOW, 0 },
	{ AU1500_GPIO_207, IRQF_TRIGGER_LOW, 0 },

	{ AU1000_GPIO_0, IRQF_TRIGGER_LOW, 0 },
	{ AU1000_GPIO_1, IRQF_TRIGGER_LOW, 0 },
	{ AU1000_GPIO_2, IRQF_TRIGGER_LOW, 0 },
	{ AU1000_GPIO_3, IRQF_TRIGGER_LOW, 0 },
	{ AU1000_GPIO_4, IRQF_TRIGGER_LOW, 0 }, /* CF interrupt */
	{ AU1000_GPIO_5, IRQF_TRIGGER_LOW, 0 },
};

void __init board_init_irq(void)
{
	au1xxx_setup_irqmap(au1xxx_irq_map, ARRAY_SIZE(au1xxx_irq_map));
}
