

#ifndef _SXG_DEBUG_H_
#define _SXG_DEBUG_H_

#define ATKDBG  1
#define ATK_TRACE_ENABLED 1

#define DBG_ERROR(n, args...)	printk(KERN_EMERG n, ##args)

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef SXG_ASSERT_ENABLED
#ifndef ASSERT
#define ASSERT(a)                                                                 \
    {                                                                             \
        if (!(a)) {                                                               \
            DBG_ERROR("ASSERT() Failure: file %s, function %s  line %d\n",\
                __FILE__, __func__, __LINE__);                                \
        }                                                                         \
    }
#endif
#else
#ifndef ASSERT
#define ASSERT(a)
#endif
#endif /* SXG_ASSERT_ENABLED  */


#ifdef ATKDBG

extern ulong ATKTimerDiv;

struct trace_entry_t {
        char      name[8];        /* 8 character name - like 's'i'm'b'a'r'c'v' */
        u32   time;           /* Current clock tic */
        unsigned char     cpu;            /* Current CPU */
        unsigned char     irql;           /* Current IRQL */
        unsigned char     driver;         /* The driver which added the trace call */
        unsigned char     pad2;           /* pad to 4 byte boundary - will probably get used */
        u32   arg1;           /* Caller arg1 */
        u32   arg2;           /* Caller arg2 */
        u32   arg3;           /* Caller arg3 */
        u32   arg4;           /* Caller arg4 */
};

#define TRACE_SXG             1
#define TRACE_VPCI            2
#define TRACE_SLIC            3

#define TRACE_ENTRIES   1024

struct sxg_trace_buffer_t {
        unsigned int                    size;                  /* aid for windbg extension */
        unsigned int                    in;                    /* Where to add */
        unsigned int                    level;                 /* Current Trace level */
	spinlock_t	lock;                  /* For MP tracing */
        struct trace_entry_t           entries[TRACE_ENTRIES];/* The circular buffer */
};

#define TRACE_NONE              0   /* For trace level - if no tracing wanted */
#define TRACE_CRITICAL          1   /* minimal tracing - only critical stuff */
#define TRACE_IMPORTANT         5   /* more tracing - anything important */
#define TRACE_NOISY             10  /* Everything in the world */


#if ATK_TRACE_ENABLED
#define SXG_TRACE_INIT(buffer, tlevel)				\
{								\
	memset((buffer), 0, sizeof(struct sxg_trace_buffer_t));	\
	(buffer)->level = (tlevel);				\
	(buffer)->size = TRACE_ENTRIES;				\
	spin_lock_init(&(buffer)->lock);			\
}
#else
#define SXG_TRACE_INIT(buffer, tlevel)
#endif

#if ATK_TRACE_ENABLED
#define SXG_TRACE(tdriver, buffer, tlevel, tname, a1, a2, a3, a4) {        \
        if ((buffer) && ((buffer)->level >= (tlevel))) {                      \
                unsigned int            trace_irql = 0;    /* ?????? FIX THIS  */    \
                unsigned int            trace_len;                                   \
                struct trace_entry_t	*trace_entry;				\
                struct timeval  timev;                                       \
                                                                             \
                spin_lock(&(buffer)->lock);                       \
                trace_entry = &(buffer)->entries[(buffer)->in];              \
                do_gettimeofday(&timev);                                     \
                                                                             \
                memset(trace_entry->name, 0, 8);                             \
                trace_len = strlen(tname);                                   \
                trace_len = trace_len > 8 ? 8 : trace_len;                   \
                memcpy(trace_entry->name, (tname), trace_len);               \
                trace_entry->time = timev.tv_usec;                           \
                trace_entry->cpu = (unsigned char)(smp_processor_id() & 0xFF);       \
                trace_entry->driver = (tdriver);                             \
                trace_entry->irql = trace_irql;                              \
                trace_entry->arg1 = (ulong)(a1);                             \
                trace_entry->arg2 = (ulong)(a2);                             \
                trace_entry->arg3 = (ulong)(a3);                             \
                trace_entry->arg4 = (ulong)(a4);                             \
                                                                             \
                (buffer)->in++;                                              \
                if ((buffer)->in == TRACE_ENTRIES)                           \
                        (buffer)->in = 0;                                    \
                                                                             \
                spin_unlock(&(buffer)->lock);                       \
        }                                                                    \
}
#else
#define SXG_TRACE(tdriver, buffer, tlevel, tname, a1, a2, a3, a4)
#endif

#endif

#endif  /*  _SXG_DEBUG_H_  */
