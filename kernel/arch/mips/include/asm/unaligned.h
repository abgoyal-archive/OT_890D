
#ifndef _ASM_MIPS_UNALIGNED_H
#define _ASM_MIPS_UNALIGNED_H

#include <linux/compiler.h>
#if defined(__MIPSEB__)
# include <linux/unaligned/be_struct.h>
# include <linux/unaligned/le_byteshift.h>
# include <linux/unaligned/generic.h>
# define get_unaligned	__get_unaligned_be
# define put_unaligned	__put_unaligned_be
#elif defined(__MIPSEL__)
# include <linux/unaligned/le_struct.h>
# include <linux/unaligned/be_byteshift.h>
# include <linux/unaligned/generic.h>
# define get_unaligned	__get_unaligned_le
# define put_unaligned	__put_unaligned_le
#else
#  error "MIPS, but neither __MIPSEB__, nor __MIPSEL__???"
#endif

#endif /* _ASM_MIPS_UNALIGNED_H */
