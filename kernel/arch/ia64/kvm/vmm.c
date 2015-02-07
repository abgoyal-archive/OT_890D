


#include<linux/kernel.h>
#include<linux/module.h>
#include<asm/fpswa.h>

#include "vcpu.h"

MODULE_AUTHOR("Intel");
MODULE_LICENSE("GPL");

extern char kvm_ia64_ivt;
extern fpswa_interface_t *vmm_fpswa_interface;

long vmm_sanity = 1;

struct kvm_vmm_info vmm_info = {
	.module	     = THIS_MODULE,
	.vmm_entry   = vmm_entry,
	.tramp_entry = vmm_trampoline,
	.vmm_ivt     = (unsigned long)&kvm_ia64_ivt,
};

static int __init  kvm_vmm_init(void)
{

	vmm_fpswa_interface = fpswa_interface;

	/*Register vmm data to kvm side*/
	return kvm_init(&vmm_info, 1024, THIS_MODULE);
}

static void __exit kvm_vmm_exit(void)
{
	kvm_exit();
	return ;
}

void vmm_spin_lock(spinlock_t *lock)
{
	_vmm_raw_spin_lock(lock);
}

void vmm_spin_unlock(spinlock_t *lock)
{
	_vmm_raw_spin_unlock(lock);
}

static void vcpu_debug_exit(struct kvm_vcpu *vcpu)
{
	struct exit_ctl_data *p = &vcpu->arch.exit_data;
	long psr;

	local_irq_save(psr);
	p->exit_reason = EXIT_REASON_DEBUG;
	vmm_transition(vcpu);
	local_irq_restore(psr);
}

asmlinkage int printk(const char *fmt, ...)
{
	struct kvm_vcpu *vcpu = current_vcpu;
	va_list args;
	int r;

	memset(vcpu->arch.log_buf, 0, VMM_LOG_LEN);
	va_start(args, fmt);
	r = vsnprintf(vcpu->arch.log_buf, VMM_LOG_LEN, fmt, args);
	va_end(args);
	vcpu_debug_exit(vcpu);
	return r;
}

module_init(kvm_vmm_init)
module_exit(kvm_vmm_exit)
