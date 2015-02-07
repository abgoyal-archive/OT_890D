

#ifndef _ASM_IA64_XEN_HYPERVISOR_H
#define _ASM_IA64_XEN_HYPERVISOR_H

#ifdef CONFIG_XEN

#include <linux/init.h>
#include <xen/interface/xen.h>
#include <xen/interface/version.h>	/* to compile feature.c */
#include <xen/features.h>		/* to comiple xen-netfront.c */
#include <asm/xen/hypercall.h>

/* xen_domain_type is set before executing any C code by early_xen_setup */
enum xen_domain_type {
	XEN_NATIVE,
	XEN_PV_DOMAIN,
	XEN_HVM_DOMAIN,
};

extern enum xen_domain_type xen_domain_type;

#define xen_domain()		(xen_domain_type != XEN_NATIVE)
#define xen_pv_domain()		(xen_domain_type == XEN_PV_DOMAIN)
#define xen_initial_domain()	(xen_pv_domain() && \
				 (xen_start_info->flags & SIF_INITDOMAIN))
#define xen_hvm_domain()	(xen_domain_type == XEN_HVM_DOMAIN)

/* deprecated. remove this */
#define is_running_on_xen()	(xen_domain_type == XEN_PV_DOMAIN)

extern struct shared_info *HYPERVISOR_shared_info;
extern struct start_info *xen_start_info;

void __init xen_setup_vcpu_info_placement(void);
void force_evtchn_callback(void);

/* for drivers/xen/balloon/balloon.c */
#ifdef CONFIG_XEN_SCRUB_PAGES
#define scrub_pages(_p, _n) memset((void *)(_p), 0, (_n) << PAGE_SHIFT)
#else
#define scrub_pages(_p, _n) ((void)0)
#endif

/* For setup_arch() in arch/ia64/kernel/setup.c */
void xen_ia64_enable_opt_feature(void);

#else /* CONFIG_XEN */

#define xen_domain()		(0)
#define xen_pv_domain()		(0)
#define xen_initial_domain()	(0)
#define xen_hvm_domain()	(0)
#define is_running_on_xen()	(0)	/* deprecated. remove this */
#endif

#define is_initial_xendomain()	(0)	/* deprecated. remove this */

#endif /* _ASM_IA64_XEN_HYPERVISOR_H */
