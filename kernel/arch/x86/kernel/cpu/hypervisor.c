

#include <asm/processor.h>
#include <asm/vmware.h>
#include <asm/hypervisor.h>

static inline void __cpuinit
detect_hypervisor_vendor(struct cpuinfo_x86 *c)
{
	if (vmware_platform()) {
		c->x86_hyper_vendor = X86_HYPER_VENDOR_VMWARE;
	} else {
		c->x86_hyper_vendor = X86_HYPER_VENDOR_NONE;
	}
}

unsigned long get_hypervisor_tsc_freq(void)
{
	if (boot_cpu_data.x86_hyper_vendor == X86_HYPER_VENDOR_VMWARE)
		return vmware_get_tsc_khz();
	return 0;
}

static inline void __cpuinit
hypervisor_set_feature_bits(struct cpuinfo_x86 *c)
{
	if (boot_cpu_data.x86_hyper_vendor == X86_HYPER_VENDOR_VMWARE) {
		vmware_set_feature_bits(c);
		return;
	}
}

void __cpuinit init_hypervisor(struct cpuinfo_x86 *c)
{
	detect_hypervisor_vendor(c);
	hypervisor_set_feature_bits(c);
}
