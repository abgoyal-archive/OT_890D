
#include <linux/module.h>
#include <linux/stringify.h>
#include <linux/stddef.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/cpu.h>
#include <linux/freezer.h>
#include <linux/highmem.h>
#include <asm/paravirt.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/poll.h>
#include <asm/asm-offsets.h>
#include "lg.h"


static struct vm_struct *switcher_vma;
static struct page **switcher_page;

/* This One Big lock protects all inter-guest data structures. */
DEFINE_MUTEX(lguest_lock);

static __init int map_switcher(void)
{
	int i, err;
	struct page **pagep;

	/*
	 * Map the Switcher in to high memory.
	 *
	 * It turns out that if we choose the address 0xFFC00000 (4MB under the
	 * top virtual address), it makes setting up the page tables really
	 * easy.
	 */

	/* We allocate an array of struct page pointers.  map_vm_area() wants
	 * this, rather than just an array of pages. */
	switcher_page = kmalloc(sizeof(switcher_page[0])*TOTAL_SWITCHER_PAGES,
				GFP_KERNEL);
	if (!switcher_page) {
		err = -ENOMEM;
		goto out;
	}

	/* Now we actually allocate the pages.  The Guest will see these pages,
	 * so we make sure they're zeroed. */
	for (i = 0; i < TOTAL_SWITCHER_PAGES; i++) {
		unsigned long addr = get_zeroed_page(GFP_KERNEL);
		if (!addr) {
			err = -ENOMEM;
			goto free_some_pages;
		}
		switcher_page[i] = virt_to_page(addr);
	}

	/* First we check that the Switcher won't overlap the fixmap area at
	 * the top of memory.  It's currently nowhere near, but it could have
	 * very strange effects if it ever happened. */
	if (SWITCHER_ADDR + (TOTAL_SWITCHER_PAGES+1)*PAGE_SIZE > FIXADDR_START){
		err = -ENOMEM;
		printk("lguest: mapping switcher would thwack fixmap\n");
		goto free_pages;
	}

	/* Now we reserve the "virtual memory area" we want: 0xFFC00000
	 * (SWITCHER_ADDR).  We might not get it in theory, but in practice
	 * it's worked so far.  The end address needs +1 because __get_vm_area
	 * allocates an extra guard page, so we need space for that. */
	switcher_vma = __get_vm_area(TOTAL_SWITCHER_PAGES * PAGE_SIZE,
				     VM_ALLOC, SWITCHER_ADDR, SWITCHER_ADDR
				     + (TOTAL_SWITCHER_PAGES+1) * PAGE_SIZE);
	if (!switcher_vma) {
		err = -ENOMEM;
		printk("lguest: could not map switcher pages high\n");
		goto free_pages;
	}

	/* This code actually sets up the pages we've allocated to appear at
	 * SWITCHER_ADDR.  map_vm_area() takes the vma we allocated above, the
	 * kind of pages we're mapping (kernel pages), and a pointer to our
	 * array of struct pages.  It increments that pointer, but we don't
	 * care. */
	pagep = switcher_page;
	err = map_vm_area(switcher_vma, PAGE_KERNEL, &pagep);
	if (err) {
		printk("lguest: map_vm_area failed: %i\n", err);
		goto free_vma;
	}

	/* Now the Switcher is mapped at the right address, we can't fail!
	 * Copy in the compiled-in Switcher code (from <arch>_switcher.S). */
	memcpy(switcher_vma->addr, start_switcher_text,
	       end_switcher_text - start_switcher_text);

	printk(KERN_INFO "lguest: mapped switcher at %p\n",
	       switcher_vma->addr);
	/* And we succeeded... */
	return 0;

free_vma:
	vunmap(switcher_vma->addr);
free_pages:
	i = TOTAL_SWITCHER_PAGES;
free_some_pages:
	for (--i; i >= 0; i--)
		__free_pages(switcher_page[i], 0);
	kfree(switcher_page);
out:
	return err;
}
/*:*/

static void unmap_switcher(void)
{
	unsigned int i;

	/* vunmap() undoes *both* map_vm_area() and __get_vm_area(). */
	vunmap(switcher_vma->addr);
	/* Now we just need to free the pages we copied the switcher into */
	for (i = 0; i < TOTAL_SWITCHER_PAGES; i++)
		__free_pages(switcher_page[i], 0);
	kfree(switcher_page);
}

int lguest_address_ok(const struct lguest *lg,
		      unsigned long addr, unsigned long len)
{
	return (addr+len) / PAGE_SIZE < lg->pfn_limit && (addr+len >= addr);
}

void __lgread(struct lg_cpu *cpu, void *b, unsigned long addr, unsigned bytes)
{
	if (!lguest_address_ok(cpu->lg, addr, bytes)
	    || copy_from_user(b, cpu->lg->mem_base + addr, bytes) != 0) {
		/* copy_from_user should do this, but as we rely on it... */
		memset(b, 0, bytes);
		kill_guest(cpu, "bad read address %#lx len %u", addr, bytes);
	}
}

/* This is the write (copy into Guest) version. */
void __lgwrite(struct lg_cpu *cpu, unsigned long addr, const void *b,
	       unsigned bytes)
{
	if (!lguest_address_ok(cpu->lg, addr, bytes)
	    || copy_to_user(cpu->lg->mem_base + addr, b, bytes) != 0)
		kill_guest(cpu, "bad write address %#lx len %u", addr, bytes);
}
/*:*/

int run_guest(struct lg_cpu *cpu, unsigned long __user *user)
{
	/* We stop running once the Guest is dead. */
	while (!cpu->lg->dead) {
		/* First we run any hypercalls the Guest wants done. */
		if (cpu->hcall)
			do_hypercalls(cpu);

		/* It's possible the Guest did a NOTIFY hypercall to the
		 * Launcher, in which case we return from the read() now. */
		if (cpu->pending_notify) {
			if (put_user(cpu->pending_notify, user))
				return -EFAULT;
			return sizeof(cpu->pending_notify);
		}

		/* Check for signals */
		if (signal_pending(current))
			return -ERESTARTSYS;

		/* If Waker set break_out, return to Launcher. */
		if (cpu->break_out)
			return -EAGAIN;

		/* Check if there are any interrupts which can be delivered now:
		 * if so, this sets up the hander to be executed when we next
		 * run the Guest. */
		maybe_do_interrupt(cpu);

		/* All long-lived kernel loops need to check with this horrible
		 * thing called the freezer.  If the Host is trying to suspend,
		 * it stops us. */
		try_to_freeze();

		/* Just make absolutely sure the Guest is still alive.  One of
		 * those hypercalls could have been fatal, for example. */
		if (cpu->lg->dead)
			break;

		/* If the Guest asked to be stopped, we sleep.  The Guest's
		 * clock timer or LHREQ_BREAK from the Waker will wake us. */
		if (cpu->halted) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			continue;
		}

		/* OK, now we're ready to jump into the Guest.  First we put up
		 * the "Do Not Disturb" sign: */
		local_irq_disable();

		/* Actually run the Guest until something happens. */
		lguest_arch_run_guest(cpu);

		/* Now we're ready to be interrupted or moved to other CPUs */
		local_irq_enable();

		/* Now we deal with whatever happened to the Guest. */
		lguest_arch_handle_trap(cpu);
	}

	/* Special case: Guest is 'dead' but wants a reboot. */
	if (cpu->lg->dead == ERR_PTR(-ERESTART))
		return -ERESTART;

	/* The Guest is dead => "No such file or directory" */
	return -ENOENT;
}

static int __init init(void)
{
	int err;

	/* Lguest can't run under Xen, VMI or itself.  It does Tricky Stuff. */
	if (paravirt_enabled()) {
		printk("lguest is afraid of being a guest\n");
		return -EPERM;
	}

	/* First we put the Switcher up in very high virtual memory. */
	err = map_switcher();
	if (err)
		goto out;

	/* Now we set up the pagetable implementation for the Guests. */
	err = init_pagetables(switcher_page, SHARED_SWITCHER_PAGES);
	if (err)
		goto unmap;

	/* We might need to reserve an interrupt vector. */
	err = init_interrupts();
	if (err)
		goto free_pgtables;

	/* /dev/lguest needs to be registered. */
	err = lguest_device_init();
	if (err)
		goto free_interrupts;

	/* Finally we do some architecture-specific setup. */
	lguest_arch_host_init();

	/* All good! */
	return 0;

free_interrupts:
	free_interrupts();
free_pgtables:
	free_pagetables();
unmap:
	unmap_switcher();
out:
	return err;
}

/* Cleaning up is just the same code, backwards.  With a little French. */
static void __exit fini(void)
{
	lguest_device_remove();
	free_interrupts();
	free_pagetables();
	unmap_switcher();

	lguest_arch_host_fini();
}
/*:*/

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rusty Russell <rusty@rustcorp.com.au>");
