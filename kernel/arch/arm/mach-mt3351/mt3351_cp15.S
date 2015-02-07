
#include <linux/autoconf.h>
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>

#define C9_ENABLE_BIT 0x00000001

	.text

ENTRY(apmcu_select_tcm_bank)
    MCR p15,0,r0,c9,c2,0
    BX lr


ENTRY(apmcu_enable_itcm)
    MRC p15,0,r1,c9,c1,1
    ORR r0,r0,#C9_ENABLE_BIT
    ORR r0,r0,r1
    MCR p15,0,r0,c9,c1,1
    BX lr

ENTRY(apmcu_enable_dtcm)
    MRC p15,0,r1,c9,c1,0
    ORR r0,r0,#C9_ENABLE_BIT
    ORR r0,r0,r1
    MCR p15,0,r0,c9,c1,0
    BX lr

ENTRY(apmcu_set_itcm_sec)
	MOV r0, #0
    MCR p15,0,r0,c9,c1,3
    BX lr

ENTRY(apmcu_set_dtcm_sec)
	MOV r0, #0
    MCR p15,0,r0,c9,c1,2
    BX lr
    
