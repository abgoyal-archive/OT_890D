

#if defined(CONFIG_CPU_S3C2440) || defined(CONFIG_CPU_S3C2442)

extern void s3c244x_map_io(void);

extern void s3c244x_init_uarts(struct s3c2410_uartcfg *cfg, int no);

extern void s3c244x_init_clocks(int xtal);

#else
#define s3c244x_init_clocks NULL
#define s3c244x_init_uarts NULL
#define s3c244x_map_io NULL
#endif
