

#include <linux/dmi.h>
#include <asm/div64.h>
#include <asm/vmware.h>

#define CPUID_VMWARE_INFO_LEAF	0x40000000
#define VMWARE_HYPERVISOR_MAGIC	0x564D5868
#define VMWARE_HYPERVISOR_PORT	0x5658

#define VMWARE_PORT_CMD_GETVERSION	10
#define VMWARE_PORT_CMD_GETHZ		45

#define VMWARE_PORT(cmd, eax, ebx, ecx, edx)				\
	__asm__("inl (%%dx)" :						\
			"=a"(eax), "=c"(ecx), "=d"(edx), "=b"(ebx) :	\
			"0"(VMWARE_HYPERVISOR_MAGIC),			\
			"1"(VMWARE_PORT_CMD_##cmd),			\
			"2"(VMWARE_HYPERVISOR_PORT), "3"(UINT_MAX) :	\
			"memory");

static inline int __vmware_platform(void)
{
	uint32_t eax, ebx, ecx, edx;
	VMWARE_PORT(GETVERSION, eax, ebx, ecx, edx);
	return eax != (uint32_t)-1 && ebx == VMWARE_HYPERVISOR_MAGIC;
}

static unsigned long __vmware_get_tsc_khz(void)
{
        uint64_t tsc_hz;
        uint32_t eax, ebx, ecx, edx;

        VMWARE_PORT(GETHZ, eax, ebx, ecx, edx);

        if (ebx == UINT_MAX)
                return 0;
        tsc_hz = eax | (((uint64_t)ebx) << 32);
        do_div(tsc_hz, 1000);
        BUG_ON(tsc_hz >> 32);
        return tsc_hz;
}

int vmware_platform(void)
{
	if (cpu_has_hypervisor) {
		unsigned int eax, ebx, ecx, edx;
		char hyper_vendor_id[13];

		cpuid(CPUID_VMWARE_INFO_LEAF, &eax, &ebx, &ecx, &edx);
		memcpy(hyper_vendor_id + 0, &ebx, 4);
		memcpy(hyper_vendor_id + 4, &ecx, 4);
		memcpy(hyper_vendor_id + 8, &edx, 4);
		hyper_vendor_id[12] = '\0';
		if (!strcmp(hyper_vendor_id, "VMwareVMware"))
			return 1;
	} else if (dmi_available && dmi_name_in_serial("VMware") &&
		   __vmware_platform())
		return 1;

	return 0;
}

unsigned long vmware_get_tsc_khz(void)
{
	BUG_ON(!vmware_platform());
	return __vmware_get_tsc_khz();
}

void __cpuinit vmware_set_feature_bits(struct cpuinfo_x86 *c)
{
	set_cpu_cap(c, X86_FEATURE_CONSTANT_TSC);
	set_cpu_cap(c, X86_FEATURE_TSC_RELIABLE);
}
