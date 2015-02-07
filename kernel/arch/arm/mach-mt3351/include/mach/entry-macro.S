


#include "mt3351.h"

/* leave disable_fiq as an empty macro */
.macro disable_fiq
.endm

.macro  get_irqnr_preamble, base, tmp
.endm

.macro	arch_ret_to_user, tmp1, tmp2
.endm

.macro get_irqnr_and_base, irqnr, irqstat, base, tmp
    @ set bim base register
    ldr \base, =MT3351_INTH_VIRT 
    ldr \irqnr, [\base, #MT3351_REG_IRQ_STA2]
  
 /* 
  * 2008 09 19
  * Irene Liao
  * give an offset to irqnr by 63 if it is the external interrupt
  */

    mov \irqstat, \irqnr
    teq \irqstat, #MT3351_EIT_IRQ_LINE  @EINT
    bne 8f
    ldr \irqstat, [\base, #MT3351_REG_EINT_STA]
    teq \irqstat, #0
    bne 4f
    mov \irqstat, \irqnr  @invalid External IRQ .
    b 8f

4:  mov \tmp, \irqstat
    and \tmp,\tmp, #0xFF00
    and \irqstat, \irqstat, #0xFF
    orr \irqstat, \irqstat, \tmp
    
    mov \irqnr, #0x3f
3:  mov \irqstat, \irqstat, LSR #1
    add \irqnr, \irqnr, #1
    teq \irqstat,#0
    bne 3b
    
    mov \irqstat, #MT3351_EIT_IRQ_LINE @ EINT 

    @ test irqstat
8: 
    bic \irqstat, \irqstat, #0xFF  
    teq \irqstat, #0x100
    .endm


