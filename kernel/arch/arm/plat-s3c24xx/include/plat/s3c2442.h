

#ifdef CONFIG_CPU_S3C2442
extern  int s3c2442_init(void);
#else
#define s3c2442_init NULL
#endif
