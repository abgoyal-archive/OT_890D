
#ifndef LINUX_NMI_H
#define LINUX_NMI_H

#include <linux/sched.h>
#include <asm/irq.h>

#ifdef ARCH_HAS_NMI_WATCHDOG
#include <asm/nmi.h>
extern void touch_nmi_watchdog(void);
extern void acpi_nmi_disable(void);
extern void acpi_nmi_enable(void);
#else
static inline void touch_nmi_watchdog(void)
{
	touch_softlockup_watchdog();
}
static inline void acpi_nmi_disable(void) { }
static inline void acpi_nmi_enable(void) { }
#endif

#ifndef trigger_all_cpu_backtrace
#define trigger_all_cpu_backtrace() do { } while (0)
#endif

#endif
