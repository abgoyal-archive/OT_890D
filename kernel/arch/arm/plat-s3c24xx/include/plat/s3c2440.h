

#ifdef CONFIG_CPU_S3C2440
extern  int s3c2440_init(void);
#else
#define s3c2440_init NULL
#endif
