


#include "mt6516.h"

/* leave disable_fiq as an empty macro */
.macro disable_fiq
.endm

.macro  get_irqnr_preamble, base, tmp
.endm

.macro	arch_ret_to_user, tmp1, tmp2
.endm

.macro get_irqnr_and_base, irqnr, irqstat, base, tmp
    @ set bim base register
    ldr \base, =MT6516_INTH_VIRT 
    ldr \irqnr, [\base, #MT6516_REG_IRQ_STA2]   
    mov \irqstat, \irqnr

    @ test irqstat
8: 
    bic \irqstat, \irqstat, #0xFF  
    teq \irqstat, #0x100
    .endm


