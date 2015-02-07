
#ifndef __ASM_CPU_SH3_TIMER_H
#define __ASM_CPU_SH3_TIMER_H


#if  !defined(CONFIG_CPU_SUBTYPE_SH7720) && !defined(CONFIG_CPU_SUBTYPE_SH7721)
#define TMU_TOCR	0xfffffe90	/* Byte access */
#endif

#if defined(CONFIG_CPU_SUBTYPE_SH7710) || \
    defined(CONFIG_CPU_SUBTYPE_SH7720) || \
    defined(CONFIG_CPU_SUBTYPE_SH7721)
#define TMU_012_TSTR	0xa412fe92	/* Byte access */

#define TMU0_TCOR	0xa412fe94	/* Long access */
#define TMU0_TCNT	0xa412fe98	/* Long access */
#define TMU0_TCR	0xa412fe9c	/* Word access */

#define TMU1_TCOR	0xa412fea0	/* Long access */
#define TMU1_TCNT	0xa412fea4	/* Long access */
#define TMU1_TCR	0xa412fea8	/* Word access */

#define TMU2_TCOR	0xa412feac	/* Long access */
#define TMU2_TCNT	0xa412feb0	/* Long access */
#define TMU2_TCR	0xa412feb4	/* Word access */

#else
#define TMU_012_TSTR	0xfffffe92	/* Byte access */

#define TMU0_TCOR	0xfffffe94	/* Long access */
#define TMU0_TCNT	0xfffffe98	/* Long access */
#define TMU0_TCR	0xfffffe9c	/* Word access */

#define TMU1_TCOR	0xfffffea0	/* Long access */
#define TMU1_TCNT	0xfffffea4	/* Long access */
#define TMU1_TCR	0xfffffea8	/* Word access */

#define TMU2_TCOR	0xfffffeac	/* Long access */
#define TMU2_TCNT	0xfffffeb0	/* Long access */
#define TMU2_TCR	0xfffffeb4	/* Word access */
#if !defined(CONFIG_CPU_SUBTYPE_SH7720) && !defined(CONFIG_CPU_SUBTYPE_SH7721)
#define TMU2_TCPR2	0xfffffeb8	/* Long access */
#endif
#endif

#endif /* __ASM_CPU_SH3_TIMER_H */

