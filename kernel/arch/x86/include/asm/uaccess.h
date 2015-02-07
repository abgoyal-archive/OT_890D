
#ifndef _ASM_X86_UACCESS_H
#define _ASM_X86_UACCESS_H
#include <linux/errno.h>
#include <linux/compiler.h>
#include <linux/thread_info.h>
#include <linux/prefetch.h>
#include <linux/string.h>
#include <asm/asm.h>
#include <asm/page.h>

#define VERIFY_READ 0
#define VERIFY_WRITE 1


#define MAKE_MM_SEG(s)	((mm_segment_t) { (s) })

#define KERNEL_DS	MAKE_MM_SEG(-1UL)
#define USER_DS		MAKE_MM_SEG(PAGE_OFFSET)

#define get_ds()	(KERNEL_DS)
#define get_fs()	(current_thread_info()->addr_limit)
#define set_fs(x)	(current_thread_info()->addr_limit = (x))

#define segment_eq(a, b)	((a).seg == (b).seg)

#define __addr_ok(addr)					\
	((unsigned long __force)(addr) <		\
	 (current_thread_info()->addr_limit.seg))


#define __range_not_ok(addr, size)					\
({									\
	unsigned long flag, roksum;					\
	__chk_user_ptr(addr);						\
	asm("add %3,%1 ; sbb %0,%0 ; cmp %1,%4 ; sbb $0,%0"		\
	    : "=&r" (flag), "=r" (roksum)				\
	    : "1" (addr), "g" ((long)(size)),				\
	      "rm" (current_thread_info()->addr_limit.seg));		\
	flag;								\
})

#define access_ok(type, addr, size) (likely(__range_not_ok(addr, size) == 0))


struct exception_table_entry {
	unsigned long insn, fixup;
};

extern int fixup_exception(struct pt_regs *regs);


extern int __get_user_1(void);
extern int __get_user_2(void);
extern int __get_user_4(void);
extern int __get_user_8(void);
extern int __get_user_bad(void);

#define __get_user_x(size, ret, x, ptr)		      \
	asm volatile("call __get_user_" #size	      \
		     : "=a" (ret),"=d" (x)	      \
		     : "0" (ptr))		      \


#ifdef CONFIG_X86_32
#define __get_user_8(__ret_gu, __val_gu, ptr)				\
		__get_user_x(X, __ret_gu, __val_gu, ptr)
#else
#define __get_user_8(__ret_gu, __val_gu, ptr)				\
		__get_user_x(8, __ret_gu, __val_gu, ptr)
#endif

#define get_user(x, ptr)						\
({									\
	int __ret_gu;							\
	unsigned long __val_gu;						\
	__chk_user_ptr(ptr);						\
	might_fault();							\
	switch (sizeof(*(ptr))) {					\
	case 1:								\
		__get_user_x(1, __ret_gu, __val_gu, ptr);		\
		break;							\
	case 2:								\
		__get_user_x(2, __ret_gu, __val_gu, ptr);		\
		break;							\
	case 4:								\
		__get_user_x(4, __ret_gu, __val_gu, ptr);		\
		break;							\
	case 8:								\
		__get_user_8(__ret_gu, __val_gu, ptr);			\
		break;							\
	default:							\
		__get_user_x(X, __ret_gu, __val_gu, ptr);		\
		break;							\
	}								\
	(x) = (__typeof__(*(ptr)))__val_gu;				\
	__ret_gu;							\
})

#define __put_user_x(size, x, ptr, __ret_pu)			\
	asm volatile("call __put_user_" #size : "=a" (__ret_pu)	\
		     :"0" ((typeof(*(ptr)))(x)), "c" (ptr) : "ebx")



#ifdef CONFIG_X86_32
#define __put_user_u64(x, addr, err)					\
	asm volatile("1:	movl %%eax,0(%2)\n"			\
		     "2:	movl %%edx,4(%2)\n"			\
		     "3:\n"						\
		     ".section .fixup,\"ax\"\n"				\
		     "4:	movl %3,%0\n"				\
		     "	jmp 3b\n"					\
		     ".previous\n"					\
		     _ASM_EXTABLE(1b, 4b)				\
		     _ASM_EXTABLE(2b, 4b)				\
		     : "=r" (err)					\
		     : "A" (x), "r" (addr), "i" (-EFAULT), "0" (err))

#define __put_user_x8(x, ptr, __ret_pu)				\
	asm volatile("call __put_user_8" : "=a" (__ret_pu)	\
		     : "A" ((typeof(*(ptr)))(x)), "c" (ptr) : "ebx")
#else
#define __put_user_u64(x, ptr, retval) \
	__put_user_asm(x, ptr, retval, "q", "", "Zr", -EFAULT)
#define __put_user_x8(x, ptr, __ret_pu) __put_user_x(8, x, ptr, __ret_pu)
#endif

extern void __put_user_bad(void);

extern void __put_user_1(void);
extern void __put_user_2(void);
extern void __put_user_4(void);
extern void __put_user_8(void);

#ifdef CONFIG_X86_WP_WORKS_OK

#define put_user(x, ptr)					\
({								\
	int __ret_pu;						\
	__typeof__(*(ptr)) __pu_val;				\
	__chk_user_ptr(ptr);					\
	might_fault();						\
	__pu_val = x;						\
	switch (sizeof(*(ptr))) {				\
	case 1:							\
		__put_user_x(1, __pu_val, ptr, __ret_pu);	\
		break;						\
	case 2:							\
		__put_user_x(2, __pu_val, ptr, __ret_pu);	\
		break;						\
	case 4:							\
		__put_user_x(4, __pu_val, ptr, __ret_pu);	\
		break;						\
	case 8:							\
		__put_user_x8(__pu_val, ptr, __ret_pu);		\
		break;						\
	default:						\
		__put_user_x(X, __pu_val, ptr, __ret_pu);	\
		break;						\
	}							\
	__ret_pu;						\
})

#define __put_user_size(x, ptr, size, retval, errret)			\
do {									\
	retval = 0;							\
	__chk_user_ptr(ptr);						\
	switch (size) {							\
	case 1:								\
		__put_user_asm(x, ptr, retval, "b", "b", "iq", errret);	\
		break;							\
	case 2:								\
		__put_user_asm(x, ptr, retval, "w", "w", "ir", errret);	\
		break;							\
	case 4:								\
		__put_user_asm(x, ptr, retval, "l", "k",  "ir", errret);\
		break;							\
	case 8:								\
		__put_user_u64((__typeof__(*ptr))(x), ptr, retval);	\
		break;							\
	default:							\
		__put_user_bad();					\
	}								\
} while (0)

#else

#define __put_user_size(x, ptr, size, retval, errret)			\
do {									\
	__typeof__(*(ptr))__pus_tmp = x;				\
	retval = 0;							\
									\
	if (unlikely(__copy_to_user_ll(ptr, &__pus_tmp, size) != 0))	\
		retval = errret;					\
} while (0)

#define put_user(x, ptr)					\
({								\
	int __ret_pu;						\
	__typeof__(*(ptr))__pus_tmp = x;			\
	__ret_pu = 0;						\
	if (unlikely(__copy_to_user_ll(ptr, &__pus_tmp,		\
				       sizeof(*(ptr))) != 0))	\
		__ret_pu = -EFAULT;				\
	__ret_pu;						\
})
#endif

#ifdef CONFIG_X86_32
#define __get_user_asm_u64(x, ptr, retval, errret)	(x) = __get_user_bad()
#else
#define __get_user_asm_u64(x, ptr, retval, errret) \
	 __get_user_asm(x, ptr, retval, "q", "", "=r", errret)
#endif

#define __get_user_size(x, ptr, size, retval, errret)			\
do {									\
	retval = 0;							\
	__chk_user_ptr(ptr);						\
	switch (size) {							\
	case 1:								\
		__get_user_asm(x, ptr, retval, "b", "b", "=q", errret);	\
		break;							\
	case 2:								\
		__get_user_asm(x, ptr, retval, "w", "w", "=r", errret);	\
		break;							\
	case 4:								\
		__get_user_asm(x, ptr, retval, "l", "k", "=r", errret);	\
		break;							\
	case 8:								\
		__get_user_asm_u64(x, ptr, retval, errret);		\
		break;							\
	default:							\
		(x) = __get_user_bad();					\
	}								\
} while (0)

#define __get_user_asm(x, addr, err, itype, rtype, ltype, errret)	\
	asm volatile("1:	mov"itype" %2,%"rtype"1\n"		\
		     "2:\n"						\
		     ".section .fixup,\"ax\"\n"				\
		     "3:	mov %3,%0\n"				\
		     "	xor"itype" %"rtype"1,%"rtype"1\n"		\
		     "	jmp 2b\n"					\
		     ".previous\n"					\
		     _ASM_EXTABLE(1b, 3b)				\
		     : "=r" (err), ltype(x)				\
		     : "m" (__m(addr)), "i" (errret), "0" (err))

#define __put_user_nocheck(x, ptr, size)			\
({								\
	int __pu_err;						\
	__put_user_size((x), (ptr), (size), __pu_err, -EFAULT);	\
	__pu_err;						\
})

#define __get_user_nocheck(x, ptr, size)				\
({									\
	int __gu_err;							\
	unsigned long __gu_val;						\
	__get_user_size(__gu_val, (ptr), (size), __gu_err, -EFAULT);	\
	(x) = (__force __typeof__(*(ptr)))__gu_val;			\
	__gu_err;							\
})

/* FIXME: this hack is definitely wrong -AK */
struct __large_struct { unsigned long buf[100]; };
#define __m(x) (*(struct __large_struct __user *)(x))

#define __put_user_asm(x, addr, err, itype, rtype, ltype, errret)	\
	asm volatile("1:	mov"itype" %"rtype"1,%2\n"		\
		     "2:\n"						\
		     ".section .fixup,\"ax\"\n"				\
		     "3:	mov %3,%0\n"				\
		     "	jmp 2b\n"					\
		     ".previous\n"					\
		     _ASM_EXTABLE(1b, 3b)				\
		     : "=r"(err)					\
		     : ltype(x), "m" (__m(addr)), "i" (errret), "0" (err))

#define __get_user(x, ptr)						\
	__get_user_nocheck((x), (ptr), sizeof(*(ptr)))

#define __put_user(x, ptr)						\
	__put_user_nocheck((__typeof__(*(ptr)))(x), (ptr), sizeof(*(ptr)))

#define __get_user_unaligned __get_user
#define __put_user_unaligned __put_user

#ifdef CONFIG_X86_INTEL_USERCOPY
extern struct movsl_mask {
	int mask;
} ____cacheline_aligned_in_smp movsl_mask;
#endif

#define ARCH_HAS_NOCACHE_UACCESS 1

#ifdef CONFIG_X86_32
# include "uaccess_32.h"
#else
# define ARCH_HAS_SEARCH_EXTABLE
# include "uaccess_64.h"
#endif

#endif /* _ASM_X86_UACCESS_H */

