


struct s3c_gpio_cfg;

struct s3c_gpio_chip {
	struct gpio_chip	chip;
	struct s3c_gpio_cfg	*config;
	void __iomem		*base;
};

static inline struct s3c_gpio_chip *to_s3c_gpio(struct gpio_chip *gpc)
{
	return container_of(gpc, struct s3c_gpio_chip, chip);
}

extern void s3c_gpiolib_add(struct s3c_gpio_chip *chip);


#ifdef CONFIG_S3C_GPIO_TRACK
extern struct s3c_gpio_chip *s3c_gpios[S3C_GPIO_END];

static inline struct s3c_gpio_chip *s3c_gpiolib_getchip(unsigned int chip)
{
	return (chip < S3C_GPIO_END) ? s3c_gpios[chip] : NULL;
}
#else
/* machine specific code should provide s3c_gpiolib_getchip */

static inline void s3c_gpiolib_track(struct s3c_gpio_chip *chip) { }
#endif
