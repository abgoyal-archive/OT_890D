

#ifndef __ET131X_DBG_H__
#define __ET131X_DBG_H__

/* Define Masks for debugging types/levels */
#define DBG_ERROR_ON        0x00000001L
#define DBG_WARNING_ON      0x00000002L
#define DBG_NOTICE_ON       0x00000004L
#define DBG_TRACE_ON        0x00000008L
#define DBG_VERBOSE_ON      0x00000010L
#define DBG_PARAM_ON        0x00000020L
#define DBG_BREAK_ON        0x00000040L
#define DBG_RX_ON           0x00000100L
#define DBG_TX_ON           0x00000200L

#ifdef CONFIG_ET131X_DEBUG

#ifndef DBG_LVL
#define DBG_LVL	3
#endif /* DBG_LVL */

#define DBG_DEFAULTS		(DBG_ERROR_ON | DBG_WARNING_ON | DBG_BREAK_ON)

#define DBG_FLAGS(A)		((A)->dbgFlags)
#define DBG_NAME(A)		((A)->dbgName)
#define DBG_LEVEL(A)		((A)->dbgLevel)

#ifndef DBG_PRINT
#define DBG_PRINT(S...)		printk(KERN_DEBUG S)
#endif /* DBG_PRINT */

#ifndef DBG_PRINTC
#define DBG_PRINTC(S...)	printk(S)
#endif /* DBG_PRINTC */

#ifndef DBG_TRAP
#define DBG_TRAP		{}	/* BUG() */
#endif /* DBG_TRAP */

#define _ENTER_STR	">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
#define _LEAVE_STR	"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"

#define _DBG_ENTER(A)	printk(KERN_DEBUG "%s:%.*s:%s\n", DBG_NAME(A),	\
				++DBG_LEVEL(A), _ENTER_STR, __func__)
#define _DBG_LEAVE(A)	printk(KERN_DEBUG "%s:%.*s:%s\n", DBG_NAME(A),	\
				DBG_LEVEL(A)--, _LEAVE_STR, __func__)

#define DBG_ENTER(A)							\
	do {								\
		if (DBG_FLAGS(A) & DBG_TRACE_ON)			\
			_DBG_ENTER(A);					\
	} while (0)

#define DBG_LEAVE(A)							\
	do {								\
		if (DBG_FLAGS(A) & DBG_TRACE_ON)			\
			_DBG_LEAVE(A);					\
	} while (0)

#define DBG_PARAM(A, N, F, S...)					\
	do {								\
		if (DBG_FLAGS(A) & DBG_PARAM_ON)			\
			DBG_PRINT("  %s -- "F" ", N, S);		\
	} while (0)

#define DBG_ERROR(A, S...)						 \
	do {								 \
		if (DBG_FLAGS(A) & DBG_ERROR_ON) {			 \
			DBG_PRINT("%s:ERROR:%s ", DBG_NAME(A), __func__);\
			DBG_PRINTC(S);					 \
			DBG_TRAP;					 \
		}							 \
	} while (0)

#define DBG_WARNING(A, S...)						    \
	do {								    \
		if (DBG_FLAGS(A) & DBG_WARNING_ON) {			    \
			DBG_PRINT("%s:WARNING:%s ", DBG_NAME(A), __func__); \
			DBG_PRINTC(S);					    \
		}							    \
	} while (0)

#define DBG_NOTICE(A, S...)						   \
	do {								   \
		if (DBG_FLAGS(A) & DBG_NOTICE_ON) {			   \
			DBG_PRINT("%s:NOTICE:%s ", DBG_NAME(A), __func__); \
			DBG_PRINTC(S);					   \
		}							   \
	} while (0)

#define DBG_TRACE(A, S...)						  \
	do {								  \
		if (DBG_FLAGS(A) & DBG_TRACE_ON) {			  \
			DBG_PRINT("%s:TRACE:%s ", DBG_NAME(A), __func__); \
			DBG_PRINTC(S);					  \
		}							  \
	} while (0)

#define DBG_VERBOSE(A, S...)						    \
	do {								    \
		if (DBG_FLAGS(A) & DBG_VERBOSE_ON) {			    \
			DBG_PRINT("%s:VERBOSE:%s ", DBG_NAME(A), __func__); \
			DBG_PRINTC(S);					    \
		}							    \
	} while (0)

#define DBG_RX(A, S...)				\
	do {					\
		if (DBG_FLAGS(A) & DBG_RX_ON)	\
			DBG_PRINT(S);		\
	} while (0)

#define DBG_RX_ENTER(A)				\
	do {					\
		if (DBG_FLAGS(A) & DBG_RX_ON)	\
			_DBG_ENTER(A);		\
	} while (0)

#define DBG_RX_LEAVE(A)				\
	do {					\
		if (DBG_FLAGS(A) & DBG_RX_ON)	\
			_DBG_LEAVE(A);		\
	} while (0)

#define DBG_TX(A, S...)				\
	do {					\
		if (DBG_FLAGS(A) & DBG_TX_ON)	\
			DBG_PRINT(S);		\
	} while (0)

#define DBG_TX_ENTER(A)				\
	do {					\
		if (DBG_FLAGS(A) & DBG_TX_ON)	\
			_DBG_ENTER(A);		\
	} while (0)

#define DBG_TX_LEAVE(A)				\
	do {					\
		if (DBG_FLAGS(A) & DBG_TX_ON)	\
			_DBG_LEAVE(A);		\
	} while (0)

#define DBG_ASSERT(C)						\
	do {							\
		if (!(C)) {					\
			DBG_PRINT("ASSERT(%s) -- %s#%d (%s) ",  \
			     #C, __FILE__, __LINE__, __func__); \
			DBG_TRAP;				\
		}						\
	} while (0)

#define STATIC

typedef struct {
	char *dbgName;
	int dbgLevel;
	unsigned long dbgFlags;
} dbg_info_t;

#else /* CONFIG_ET131X_DEBUG */

#define DBG_DEFN
#define DBG_TRAP
#define DBG_PRINT(S...)
#define DBG_ENTER(A)
#define DBG_LEAVE(A)
#define DBG_PARAM(A,N,F,S...)
#define DBG_ERROR(A,S...)
#define DBG_WARNING(A,S...)
#define DBG_NOTICE(A,S...)
#define DBG_TRACE(A,S...)
#define DBG_VERBOSE(A,S...)
#define DBG_RX(A,S...)
#define DBG_RX_ENTER(A)
#define DBG_RX_LEAVE(A)
#define DBG_TX(A,S...)
#define DBG_TX_ENTER(A)
#define DBG_TX_LEAVE(A)
#define DBG_ASSERT(C)
#define STATIC static

#endif /* CONFIG_ET131X_DEBUG */

/* Forward declaration of the private adapter structure */
struct et131x_adapter;

void DumpTxQueueContents(int dbgLvl, struct et131x_adapter *adapter);
void DumpDeviceBlock(int dbgLvl, struct et131x_adapter *adapter,
		     unsigned int Block);
void DumpDeviceReg(int dbgLvl, struct et131x_adapter *adapter);

#endif /* __ET131X_DBG_H__ */
