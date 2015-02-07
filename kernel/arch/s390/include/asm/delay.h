
 
#ifndef _S390_DELAY_H
#define _S390_DELAY_H

extern void __udelay(unsigned long usecs);
extern void udelay_simple(unsigned long usecs);
extern void __delay(unsigned long loops);

#define udelay(n) __udelay(n)

#endif /* defined(_S390_DELAY_H) */
