

#ifndef _SLIC_DEBUG_H_
#define _SLIC_DEBUG_H_

#ifdef SLIC_DEFAULT_LOG_LEVEL
#else
#define SLICLEVEL   KERN_DEBUG
#endif
#define SLIC_DISPLAY              printk
#define DBG_ERROR(n, args...)   SLIC_DISPLAY(KERN_EMERG n, ##args)

#define SLIC_DEBUG_MESSAGE 1
#if SLIC_DEBUG_MESSAGE
/*#define DBG_MSG(n, args...)      SLIC_DISPLAY(SLICLEVEL n, ##args)*/
#define DBG_MSG(n, args...)
#else
#define DBG_MSG(n, args...)
#endif

#ifdef ASSERT
#undef ASSERT
#endif

#if SLIC_ASSERT_ENABLED
#ifdef CONFIG_X86_64
#define VALID_ADDRESS(p)  (1)
#else
#define VALID_ADDRESS(p)  (((u32)(p) & 0x80000000) || ((u32)(p) == 0))
#endif
#ifndef ASSERT
#define ASSERT(a)                                                             \
    {                                                                         \
	if (!(a)) {                                                           \
		DBG_ERROR("ASSERT() Failure: file %s, function %s  line %d\n",\
		__FILE__, __func__, __LINE__);                          \
		slic_assert_fail();                                       \
	}                                                                 \
    }
#endif
#ifndef ASSERTMSG
#define ASSERTMSG(a,msg)                                                  \
    {                                                                     \
	if (!(a)) {                                                       \
		DBG_ERROR("ASSERT() Failure: file %s, function %s"\
			"line %d: %s\n",\
			__FILE__, __func__, __LINE__, (msg));            \
		slic_assert_fail();                                      \
	}                                                                \
    }
#endif
#else
#ifndef ASSERT
#define ASSERT(a)
#endif
#ifndef ASSERTMSG
#define ASSERTMSG(a, msg)
#endif
#endif /* SLIC_ASSERT_ENABLED  */

#endif  /*  _SLIC_DEBUG_H_  */
