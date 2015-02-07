
#if !defined(__AEE_H__)
#define __AEE_H__

#if defined(CONFIG_AEE_IPANIC)

/* Begin starting panic record */
void ipanic_oops_start(const char *str, int err, struct task_struct *task);

void ipanic_oops_end(void);

/* Record stack trace info into current paniclog */
void ipanic_stack_store(unsigned long where, unsigned long from);

#else

#define ipanic_oops_start(str, err, task)

#define ipanic_oops_end()

#define ipanic_stack_store(where, from)

#endif

#endif // __AEE_H__
