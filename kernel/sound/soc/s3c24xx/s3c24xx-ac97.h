

#ifndef S3C24XXAC97_H_
#define S3C24XXAC97_H_

#define AC_CMD_ADDR(x) (x << 16)
#define AC_CMD_DATA(x) (x & 0xffff)

#ifdef CONFIG_CPU_S3C2440
#define IRQ_S3C244x_AC97 IRQ_S3C2440_AC97
#else
#define IRQ_S3C244x_AC97 IRQ_S3C2443_AC97
#endif

extern struct snd_soc_dai s3c2443_ac97_dai[];

#endif /*S3C24XXAC97_H_*/
