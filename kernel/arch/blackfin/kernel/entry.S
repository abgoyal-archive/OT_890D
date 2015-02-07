

#include <linux/linkage.h>
#include <asm/thread_info.h>
#include <asm/errno.h>
#include <asm/blackfin.h>
#include <asm/asm-offsets.h>

#include <asm/context.S>

#ifdef CONFIG_EXCPT_IRQ_SYSC_L1
.section .l1.text
#else
.text
#endif

ENTRY(_ret_from_fork)
#ifdef CONFIG_IPIPE
	[--sp] = reti; 		/* IRQs on. */
	SP += 4;
#endif /* CONFIG_IPIPE */
	SP += -12;
	call _schedule_tail;
	SP += 12;
	r0 = [sp + PT_IPEND];
	cc = bittst(r0,1);
	if cc jump .Lin_kernel;
	RESTORE_CONTEXT
	rti;
.Lin_kernel:
	bitclr(r0,1);
	[sp + PT_IPEND] = r0;
	/* do a 'fake' RTI by jumping to [RETI]
	 * to avoid clearing supervisor mode in child
	 */
	r0 = [sp + PT_PC];
	[sp + PT_P0] = r0;

	RESTORE_ALL_SYS
	jump (p0);
ENDPROC(_ret_from_fork)

ENTRY(_sys_fork)
	r0 = -EINVAL;
#if (ANOMALY_05000371)
	nop;
	nop;
	nop;
#endif
	rts;
ENDPROC(_sys_fork)

ENTRY(_sys_vfork)
	r0 = sp;
	r0 += 24;
	[--sp] = rets;
	SP += -12;
	call _bfin_vfork;
	SP += 12;
	rets = [sp++];
	rts;
ENDPROC(_sys_vfork)

ENTRY(_sys_clone)
	r0 = sp;
	r0 += 24;
	[--sp] = rets;
	SP += -12;
	call _bfin_clone;
	SP += 12;
	rets = [sp++];
	rts;
ENDPROC(_sys_clone)

ENTRY(_sys_rt_sigreturn)
	r0 = sp;
	r0 += 24;
	[--sp] = rets;
	SP += -12;
	call _do_rt_sigreturn;
	SP += 12;
	rets = [sp++];
	rts;
ENDPROC(_sys_rt_sigreturn)
