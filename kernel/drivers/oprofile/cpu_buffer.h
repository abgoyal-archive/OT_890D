

#ifndef OPROFILE_CPU_BUFFER_H
#define OPROFILE_CPU_BUFFER_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/cache.h>
#include <linux/sched.h>
#include <linux/ring_buffer.h>

struct task_struct;

int alloc_cpu_buffers(void);
void free_cpu_buffers(void);

void start_cpu_work(void);
void end_cpu_work(void);

struct op_sample {
	unsigned long eip;
	unsigned long event;
	unsigned long data[0];
};

struct op_entry;

struct oprofile_cpu_buffer {
	unsigned long buffer_size;
	struct task_struct *last_task;
	int last_is_kernel;
	int tracing;
	unsigned long sample_received;
	unsigned long sample_lost_overflow;
	unsigned long backtrace_aborted;
	unsigned long sample_invalid_eip;
	int cpu;
	struct delayed_work work;
};

DECLARE_PER_CPU(struct oprofile_cpu_buffer, cpu_buffer);

static inline void op_cpu_buffer_reset(int cpu)
{
	struct oprofile_cpu_buffer *cpu_buf = &per_cpu(cpu_buffer, cpu);

	cpu_buf->last_is_kernel = -1;
	cpu_buf->last_task = NULL;
}


struct op_sample
*op_cpu_buffer_write_reserve(struct op_entry *entry, unsigned long size);
int op_cpu_buffer_write_commit(struct op_entry *entry);
struct op_sample *op_cpu_buffer_read_entry(struct op_entry *entry, int cpu);
unsigned long op_cpu_buffer_entries(int cpu);

/* returns the remaining free size of data in the entry */
static inline
int op_cpu_buffer_add_data(struct op_entry *entry, unsigned long val)
{
	if (!entry->size)
		return 0;
	*entry->data = val;
	entry->size--;
	entry->data++;
	return entry->size;
}

/* returns the size of data in the entry */
static inline
int op_cpu_buffer_get_size(struct op_entry *entry)
{
	return entry->size;
}

/* returns 0 if empty or the size of data including the current value */
static inline
int op_cpu_buffer_get_data(struct op_entry *entry, unsigned long *val)
{
	int size = entry->size;
	if (!size)
		return 0;
	*val = *entry->data;
	entry->size--;
	entry->data++;
	return size;
}

/* extra data flags */
#define KERNEL_CTX_SWITCH	(1UL << 0)
#define IS_KERNEL		(1UL << 1)
#define TRACE_BEGIN		(1UL << 2)
#define USER_CTX_SWITCH		(1UL << 3)

#endif /* OPROFILE_CPU_BUFFER_H */
