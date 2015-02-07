


#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/isdn/capilli.h>

#ifdef KCAPI_DEBUG
#define DBG(format, arg...) do { \
printk(KERN_DEBUG "%s: " format "\n" , __func__ , ## arg); \
} while (0)
#else
#define DBG(format, arg...) /* */
#endif

enum {
	CARD_DETECTED = 1,
	CARD_LOADING  =	2,
	CARD_RUNNING  = 3,
};

extern struct list_head capi_drivers;
extern rwlock_t capi_drivers_list_lock;

extern struct capi20_appl *capi_applications[CAPI_MAXAPPL];
extern struct capi_ctr *capi_cards[CAPI_MAXCONTR];

#ifdef CONFIG_PROC_FS

void kcapi_proc_init(void);
void kcapi_proc_exit(void);

#else

static inline void kcapi_proc_init(void) { };
static inline void kcapi_proc_exit(void) { };

#endif

