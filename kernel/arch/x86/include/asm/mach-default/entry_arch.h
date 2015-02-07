

#ifdef CONFIG_X86_SMP
BUILD_INTERRUPT(reschedule_interrupt,RESCHEDULE_VECTOR)
BUILD_INTERRUPT(invalidate_interrupt,INVALIDATE_TLB_VECTOR)
BUILD_INTERRUPT(call_function_interrupt,CALL_FUNCTION_VECTOR)
BUILD_INTERRUPT(call_function_single_interrupt,CALL_FUNCTION_SINGLE_VECTOR)
BUILD_INTERRUPT(irq_move_cleanup_interrupt,IRQ_MOVE_CLEANUP_VECTOR)
#endif

#ifdef CONFIG_X86_LOCAL_APIC
BUILD_INTERRUPT(apic_timer_interrupt,LOCAL_TIMER_VECTOR)
BUILD_INTERRUPT(error_interrupt,ERROR_APIC_VECTOR)
BUILD_INTERRUPT(spurious_interrupt,SPURIOUS_APIC_VECTOR)

#ifdef CONFIG_X86_MCE_P4THERMAL
BUILD_INTERRUPT(thermal_interrupt,THERMAL_APIC_VECTOR)
#endif

#endif