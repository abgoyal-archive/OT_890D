

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/gpio-core.h>

#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>
#include <plat/regs-gpio.h>


#define OFF_GPCON	(0x00)
#define OFF_GPDAT	(0x04)

#define con_4bit_shift(__off) ((__off) * 4)

#if 1
#define gpio_dbg(x...) do { } while(0)
#else
#define gpio_dbg(x...) printk(KERN_DEBUG ## x)
#endif


static int s3c64xx_gpiolib_4bit_input(struct gpio_chip *chip, unsigned offset)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	unsigned long con;

	con = __raw_readl(base + OFF_GPCON);
	con &= ~(0xf << con_4bit_shift(offset));
	__raw_writel(con, base + OFF_GPCON);

	gpio_dbg("%s: %p: CON now %08lx\n", __func__, base, con);

	return 0;
}

static int s3c64xx_gpiolib_4bit_output(struct gpio_chip *chip,
				       unsigned offset, int value)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	unsigned long con;
	unsigned long dat;

	con = __raw_readl(base + OFF_GPCON);
	con &= ~(0xf << con_4bit_shift(offset));
	con |= 0x1 << con_4bit_shift(offset);

	dat = __raw_readl(base + OFF_GPDAT);
	if (value)
		dat |= 1 << offset;
	else
		dat &= ~(1 << offset);

	__raw_writel(dat, base + OFF_GPDAT);
	__raw_writel(con, base + OFF_GPCON);
	__raw_writel(dat, base + OFF_GPDAT);

	gpio_dbg("%s: %p: CON %08lx, DAT %08lx\n", __func__, base, con, dat);

	return 0;
}


static int s3c64xx_gpiolib_4bit2_input(struct gpio_chip *chip, unsigned offset)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	void __iomem *regcon = base;
	unsigned long con;

	if (offset > 7)
		offset -= 8;
	else
		regcon -= 4;

	con = __raw_readl(regcon);
	con &= ~(0xf << con_4bit_shift(offset));
	__raw_writel(con, regcon);

	gpio_dbg("%s: %p: CON %08lx\n", __func__, base, con);

	return 0;

}

static int s3c64xx_gpiolib_4bit2_output(struct gpio_chip *chip,
				       unsigned offset, int value)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	void __iomem *regcon = base;
	unsigned long con;
	unsigned long dat;

	if (offset > 7)
		offset -= 8;
	else
		regcon -= 4;

	con = __raw_readl(regcon);
	con &= ~(0xf << con_4bit_shift(offset));
	con |= 0x1 << con_4bit_shift(offset);

	dat = __raw_readl(base + OFF_GPDAT);
	if (value)
		dat |= 1 << offset;
	else
		dat &= ~(1 << offset);

	__raw_writel(dat, base + OFF_GPDAT);
	__raw_writel(con, regcon);
	__raw_writel(dat, base + OFF_GPDAT);

	gpio_dbg("%s: %p: CON %08lx, DAT %08lx\n", __func__, base, con, dat);

	return 0;
}

static struct s3c_gpio_cfg gpio_4bit_cfg_noint = {
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_cfg gpio_4bit_cfg_eint0111 = {
	.cfg_eint	= 7,
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_cfg gpio_4bit_cfg_eint0011 = {
	.cfg_eint	= 3,
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_chip gpio_4bit[] = {
	{
		.base	= S3C64XX_GPA_BASE,
		.config	= &gpio_4bit_cfg_eint0111,
		.chip	= {
			.base	= S3C64XX_GPA(0),
			.ngpio	= S3C64XX_GPIO_A_NR,
			.label	= "GPA",
		},
	}, {
		.base	= S3C64XX_GPB_BASE,
		.config	= &gpio_4bit_cfg_eint0111,
		.chip	= {
			.base	= S3C64XX_GPB(0),
			.ngpio	= S3C64XX_GPIO_B_NR,
			.label	= "GPB",
		},
	}, {
		.base	= S3C64XX_GPC_BASE,
		.config	= &gpio_4bit_cfg_eint0111,
		.chip	= {
			.base	= S3C64XX_GPC(0),
			.ngpio	= S3C64XX_GPIO_C_NR,
			.label	= "GPC",
		},
	}, {
		.base	= S3C64XX_GPD_BASE,
		.config	= &gpio_4bit_cfg_eint0111,
		.chip	= {
			.base	= S3C64XX_GPD(0),
			.ngpio	= S3C64XX_GPIO_D_NR,
			.label	= "GPD",
		},
	}, {
		.base	= S3C64XX_GPE_BASE,
		.config	= &gpio_4bit_cfg_noint,
		.chip	= {
			.base	= S3C64XX_GPE(0),
			.ngpio	= S3C64XX_GPIO_E_NR,
			.label	= "GPE",
		},
	}, {
		.base	= S3C64XX_GPG_BASE,
		.config	= &gpio_4bit_cfg_eint0111,
		.chip	= {
			.base	= S3C64XX_GPG(0),
			.ngpio	= S3C64XX_GPIO_G_NR,
			.label	= "GPG",
		},
	}, {
		.base	= S3C64XX_GPM_BASE,
		.config	= &gpio_4bit_cfg_eint0011,
		.chip	= {
			.base	= S3C64XX_GPM(0),
			.ngpio	= S3C64XX_GPIO_M_NR,
			.label	= "GPM",
		},
	},
};

static struct s3c_gpio_chip gpio_4bit2[] = {
	{
		.base	= S3C64XX_GPH_BASE + 0x4,
		.config	= &gpio_4bit_cfg_eint0111,
		.chip	= {
			.base	= S3C64XX_GPH(0),
			.ngpio	= S3C64XX_GPIO_H_NR,
			.label	= "GPH",
		},
	}, {
		.base	= S3C64XX_GPK_BASE + 0x4,
		.config	= &gpio_4bit_cfg_noint,
		.chip	= {
			.base	= S3C64XX_GPK(0),
			.ngpio	= S3C64XX_GPIO_K_NR,
			.label	= "GPK",
		},
	}, {
		.base	= S3C64XX_GPL_BASE + 0x4,
		.config	= &gpio_4bit_cfg_eint0011,
		.chip	= {
			.base	= S3C64XX_GPL(0),
			.ngpio	= S3C64XX_GPIO_L_NR,
			.label	= "GPL",
		},
	},
};

static struct s3c_gpio_cfg gpio_2bit_cfg_noint = {
	.set_config	= s3c_gpio_setcfg_s3c24xx,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_cfg gpio_2bit_cfg_eint10 = {
	.cfg_eint	= 2,
	.set_config	= s3c_gpio_setcfg_s3c24xx,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_cfg gpio_2bit_cfg_eint11 = {
	.cfg_eint	= 3,
	.set_config	= s3c_gpio_setcfg_s3c24xx,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_chip gpio_2bit[] = {
	{
		.base	= S3C64XX_GPF_BASE,
		.config	= &gpio_2bit_cfg_eint11,
		.chip	= {
			.base	= S3C64XX_GPF(0),
			.ngpio	= S3C64XX_GPIO_F_NR,
			.label	= "GPF",
		},
	}, {
		.base	= S3C64XX_GPI_BASE,
		.config	= &gpio_2bit_cfg_noint,
		.chip	= {
			.base	= S3C64XX_GPI(0),
			.ngpio	= S3C64XX_GPIO_I_NR,
			.label	= "GPI",
		},
	}, {
		.base	= S3C64XX_GPJ_BASE,
		.config	= &gpio_2bit_cfg_noint,
		.chip	= {
			.base	= S3C64XX_GPJ(0),
			.ngpio	= S3C64XX_GPIO_J_NR,
			.label	= "GPJ",
		},
	}, {
		.base	= S3C64XX_GPN_BASE,
		.config	= &gpio_2bit_cfg_eint10,
		.chip	= {
			.base	= S3C64XX_GPN(0),
			.ngpio	= S3C64XX_GPIO_N_NR,
			.label	= "GPN",
		},
	}, {
		.base	= S3C64XX_GPO_BASE,
		.config	= &gpio_2bit_cfg_eint11,
		.chip	= {
			.base	= S3C64XX_GPO(0),
			.ngpio	= S3C64XX_GPIO_O_NR,
			.label	= "GPO",
		},
	}, {
		.base	= S3C64XX_GPP_BASE,
		.config	= &gpio_2bit_cfg_eint11,
		.chip	= {
			.base	= S3C64XX_GPP(0),
			.ngpio	= S3C64XX_GPIO_P_NR,
			.label	= "GPP",
		},
	}, {
		.base	= S3C64XX_GPQ_BASE,
		.config	= &gpio_2bit_cfg_eint11,
		.chip	= {
			.base	= S3C64XX_GPQ(0),
			.ngpio	= S3C64XX_GPIO_Q_NR,
			.label	= "GPQ",
		},
	},
};

static __init void s3c64xx_gpiolib_add_4bit(struct s3c_gpio_chip *chip)
{
	chip->chip.direction_input = s3c64xx_gpiolib_4bit_input;
	chip->chip.direction_output = s3c64xx_gpiolib_4bit_output;
}

static __init void s3c64xx_gpiolib_add_4bit2(struct s3c_gpio_chip *chip)
{
	chip->chip.direction_input = s3c64xx_gpiolib_4bit2_input;
	chip->chip.direction_output = s3c64xx_gpiolib_4bit2_output;
}

static __init void s3c64xx_gpiolib_add(struct s3c_gpio_chip *chips,
				       int nr_chips,
				       void (*fn)(struct s3c_gpio_chip *))
{
	for (; nr_chips > 0; nr_chips--, chips++) {
		if (fn)
			(fn)(chips);
		s3c_gpiolib_add(chips);
	}
}

static __init int s3c64xx_gpiolib_init(void)
{
	s3c64xx_gpiolib_add(gpio_4bit, ARRAY_SIZE(gpio_4bit),
			    s3c64xx_gpiolib_add_4bit);

	s3c64xx_gpiolib_add(gpio_4bit2, ARRAY_SIZE(gpio_4bit2),
			    s3c64xx_gpiolib_add_4bit2);

	s3c64xx_gpiolib_add(gpio_2bit, ARRAY_SIZE(gpio_2bit), NULL);

	return 0;
}

core_initcall(s3c64xx_gpiolib_init);
