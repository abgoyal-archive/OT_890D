
/* -*- mode: c; c-basic-offset: 8 -*- */


BUILD_INTERRUPT(vic_sys_interrupt, VIC_SYS_INT)
BUILD_INTERRUPT(vic_cmn_interrupt, VIC_CMN_INT)
BUILD_INTERRUPT(vic_cpi_interrupt, VIC_CPI_LEVEL0);

/* do all the QIC interrupts */
BUILD_INTERRUPT(qic_timer_interrupt, QIC_TIMER_CPI);
BUILD_INTERRUPT(qic_invalidate_interrupt, QIC_INVALIDATE_CPI);
BUILD_INTERRUPT(qic_reschedule_interrupt, QIC_RESCHEDULE_CPI);
BUILD_INTERRUPT(qic_enable_irq_interrupt, QIC_ENABLE_IRQ_CPI);
BUILD_INTERRUPT(qic_call_function_interrupt, QIC_CALL_FUNCTION_CPI);
BUILD_INTERRUPT(qic_call_function_single_interrupt, QIC_CALL_FUNCTION_SINGLE_CPI);
