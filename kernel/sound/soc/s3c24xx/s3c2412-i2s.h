

#ifndef __SND_SOC_S3C24XX_S3C2412_I2S_H
#define __SND_SOC_S3C24XX_S3C2412_I2S_H __FILE__

#define S3C2412_DIV_BCLK	(1)
#define S3C2412_DIV_RCLK	(2)
#define S3C2412_DIV_PRESCALER	(3)

#define S3C2412_CLKSRC_PCLK	(0)
#define S3C2412_CLKSRC_I2SCLK	(1)

extern struct clk *s3c2412_get_iisclk(void);

extern struct snd_soc_dai s3c2412_i2s_dai;

struct s3c2412_rate_calc {
	unsigned int	clk_div;	/* for prescaler */
	unsigned int	fs_div;		/* for root frame clock */
};

extern int s3c2412_iis_calc_rate(struct s3c2412_rate_calc *info,
				 unsigned int *fstab,
				 unsigned int rate, struct clk *clk);

#endif /* __SND_SOC_S3C24XX_S3C2412_I2S_H */
