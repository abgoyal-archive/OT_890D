

#ifndef __ASM_ARCH_AUDIO_H
#define __ASM_ARCH_AUDIO_H __FILE__


struct s3c24xx_iis_ops {
	struct module *owner;

	int	(*startup)(struct s3c24xx_iis_ops *me);
	void	(*shutdown)(struct s3c24xx_iis_ops *me);
	int	(*suspend)(struct s3c24xx_iis_ops *me);
	int	(*resume)(struct s3c24xx_iis_ops *me);

	int	(*open)(struct s3c24xx_iis_ops *me, struct snd_pcm_substream *strm);
	int	(*close)(struct s3c24xx_iis_ops *me, struct snd_pcm_substream *strm);
	int	(*prepare)(struct s3c24xx_iis_ops *me, struct snd_pcm_substream *strm, struct snd_pcm_runtime *rt);
};

struct s3c24xx_platdata_iis {
	const char		*codec_clk;
	struct s3c24xx_iis_ops	*ops;
	int			(*match_dev)(struct device *dev);
};

#endif /* __ASM_ARCH_AUDIO_H */
