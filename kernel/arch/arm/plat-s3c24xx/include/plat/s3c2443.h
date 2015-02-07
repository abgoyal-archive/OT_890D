

#ifdef CONFIG_CPU_S3C2443

struct s3c2410_uartcfg;

extern  int s3c2443_init(void);

extern void s3c2443_map_io(void);

extern void s3c2443_init_uarts(struct s3c2410_uartcfg *cfg, int no);

extern void s3c2443_init_clocks(int xtal);

extern  int s3c2443_baseclk_add(void);

#else
#define s3c2443_init_clocks NULL
#define s3c2443_init_uarts NULL
#define s3c2443_map_io NULL
#define s3c2443_init NULL
#endif
