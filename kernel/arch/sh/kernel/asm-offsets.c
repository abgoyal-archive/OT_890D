

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/kbuild.h>

#include <asm/thread_info.h>

int main(void)
{
	/* offsets into the thread_info struct */
	DEFINE(TI_TASK,		offsetof(struct thread_info, task));
	DEFINE(TI_EXEC_DOMAIN,	offsetof(struct thread_info, exec_domain));
	DEFINE(TI_FLAGS,	offsetof(struct thread_info, flags));
	DEFINE(TI_CPU,		offsetof(struct thread_info, cpu));
	DEFINE(TI_PRE_COUNT,	offsetof(struct thread_info, preempt_count));
	DEFINE(TI_RESTART_BLOCK,offsetof(struct thread_info, restart_block));

	return 0;
}
