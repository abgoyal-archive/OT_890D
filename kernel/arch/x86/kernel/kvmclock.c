

#include <linux/clocksource.h>
#include <linux/kvm_para.h>
#include <asm/pvclock.h>
#include <asm/arch_hooks.h>
#include <asm/msr.h>
#include <asm/apic.h>
#include <linux/percpu.h>
#include <asm/reboot.h>

#define KVM_SCALE 22

static int kvmclock = 1;

static int parse_no_kvmclock(char *arg)
{
	kvmclock = 0;
	return 0;
}
early_param("no-kvmclock", parse_no_kvmclock);

/* The hypervisor will put information about time periodically here */
static DEFINE_PER_CPU_SHARED_ALIGNED(struct pvclock_vcpu_time_info, hv_clock);
static struct pvclock_wall_clock wall_clock;

static unsigned long kvm_get_wallclock(void)
{
	struct pvclock_vcpu_time_info *vcpu_time;
	struct timespec ts;
	int low, high;

	low = (int)__pa(&wall_clock);
	high = ((u64)__pa(&wall_clock) >> 32);
	native_write_msr(MSR_KVM_WALL_CLOCK, low, high);

	vcpu_time = &get_cpu_var(hv_clock);
	pvclock_read_wallclock(&wall_clock, vcpu_time, &ts);
	put_cpu_var(hv_clock);

	return ts.tv_sec;
}

static int kvm_set_wallclock(unsigned long now)
{
	return -1;
}

static cycle_t kvm_clock_read(void)
{
	struct pvclock_vcpu_time_info *src;
	cycle_t ret;

	src = &get_cpu_var(hv_clock);
	ret = pvclock_clocksource_read(src);
	put_cpu_var(hv_clock);
	return ret;
}

static unsigned long kvm_get_tsc_khz(void)
{
	struct pvclock_vcpu_time_info *src;
	src = &per_cpu(hv_clock, 0);
	return pvclock_tsc_khz(src);
}

static void kvm_get_preset_lpj(void)
{
	unsigned long khz;
	u64 lpj;

	khz = kvm_get_tsc_khz();

	lpj = ((u64)khz * 1000);
	do_div(lpj, HZ);
	preset_lpj = lpj;
}

static struct clocksource kvm_clock = {
	.name = "kvm-clock",
	.read = kvm_clock_read,
	.rating = 400,
	.mask = CLOCKSOURCE_MASK(64),
	.mult = 1 << KVM_SCALE,
	.shift = KVM_SCALE,
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
};

static int kvm_register_clock(char *txt)
{
	int cpu = smp_processor_id();
	int low, high;
	low = (int)__pa(&per_cpu(hv_clock, cpu)) | 1;
	high = ((u64)__pa(&per_cpu(hv_clock, cpu)) >> 32);
	printk(KERN_INFO "kvm-clock: cpu %d, msr %x:%x, %s\n",
	       cpu, high, low, txt);
	return native_write_msr_safe(MSR_KVM_SYSTEM_TIME, low, high);
}

#ifdef CONFIG_X86_LOCAL_APIC
static void __cpuinit kvm_setup_secondary_clock(void)
{
	/*
	 * Now that the first cpu already had this clocksource initialized,
	 * we shouldn't fail.
	 */
	WARN_ON(kvm_register_clock("secondary cpu clock"));
	/* ok, done with our trickery, call native */
	setup_secondary_APIC_clock();
}
#endif

#ifdef CONFIG_SMP
static void __init kvm_smp_prepare_boot_cpu(void)
{
	WARN_ON(kvm_register_clock("primary cpu clock"));
	native_smp_prepare_boot_cpu();
}
#endif

#ifdef CONFIG_KEXEC
static void kvm_crash_shutdown(struct pt_regs *regs)
{
	native_write_msr_safe(MSR_KVM_SYSTEM_TIME, 0, 0);
	native_machine_crash_shutdown(regs);
}
#endif

static void kvm_shutdown(void)
{
	native_write_msr_safe(MSR_KVM_SYSTEM_TIME, 0, 0);
	native_machine_shutdown();
}

void __init kvmclock_init(void)
{
	if (!kvm_para_available())
		return;

	if (kvmclock && kvm_para_has_feature(KVM_FEATURE_CLOCKSOURCE)) {
		if (kvm_register_clock("boot clock"))
			return;
		pv_time_ops.get_wallclock = kvm_get_wallclock;
		pv_time_ops.set_wallclock = kvm_set_wallclock;
		pv_time_ops.sched_clock = kvm_clock_read;
		pv_time_ops.get_tsc_khz = kvm_get_tsc_khz;
#ifdef CONFIG_X86_LOCAL_APIC
		pv_apic_ops.setup_secondary_clock = kvm_setup_secondary_clock;
#endif
#ifdef CONFIG_SMP
		smp_ops.smp_prepare_boot_cpu = kvm_smp_prepare_boot_cpu;
#endif
		machine_ops.shutdown  = kvm_shutdown;
#ifdef CONFIG_KEXEC
		machine_ops.crash_shutdown  = kvm_crash_shutdown;
#endif
		kvm_get_preset_lpj();
		clocksource_register(&kvm_clock);
		pv_info.paravirt_enabled = 1;
		pv_info.name = "KVM";
	}
}
