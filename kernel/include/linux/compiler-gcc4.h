
#ifndef __LINUX_COMPILER_H
#error "Please don't include <linux/compiler-gcc4.h> directly, include <linux/compiler.h> instead."
#endif

/* GCC 4.1.[01] miscompiles __weak */
#if __GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ <= 1
# error Your version of gcc miscompiles the __weak directive
#endif

#define __used			__attribute__((__used__))
#define __must_check 		__attribute__((warn_unused_result))
#define __compiler_offsetof(a,b) __builtin_offsetof(a,b)
#define __always_inline		inline __attribute__((always_inline))

#define uninitialized_var(x) x = x

#if __GNUC_MINOR__ >= 3
#define __cold			__attribute__((__cold__))

#endif
