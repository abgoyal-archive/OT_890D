

#ifdef CONFIG_EARLY_PRINTK
extern int setup_early_printk(char *);
#else
#define setup_early_printk(fmt) do { } while (0)
#endif /* CONFIG_EARLY_PRINTK */
