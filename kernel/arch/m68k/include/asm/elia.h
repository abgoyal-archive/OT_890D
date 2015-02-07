
/****************************************************************************/


/****************************************************************************/
#ifndef	elia_h
#define	elia_h
/****************************************************************************/

#include <asm/coldfire.h>

#ifdef CONFIG_eLIA


#define	eLIA_DCD1		0x0001
#define	eLIA_DCD0		0x0002
#define	eLIA_DTR1		0x0004
#define	eLIA_DTR0		0x0008

#define	eLIA_PCIRESET		0x0020

#ifndef __ASSEMBLY__
extern unsigned short	ppdata;
#endif /* __ASSEMBLY__ */

#endif	/* CONFIG_eLIA */

/****************************************************************************/
#endif	/* elia_h */
