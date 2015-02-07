


#ifndef _ME1600_AO_H_
#define _ME1600_AO_H_

# include <linux/version.h>
# include "mesubdevice.h"

# ifdef __KERNEL__

#  define ME1600_MAX_RANGES	2	/**< Specifies the maximum number of ranges in me1600_ao_subdevice_t::u_ranges und me1600_ao_subdevice_t::i_ranges. */

typedef struct me1600_ao_range_entry {
	int32_t min;
	int32_t max;
} me1600_ao_range_entry_t;

typedef struct me1600_ao_timeout {
	unsigned long start_time;
	unsigned long delay;
} me1600_ao_timeout_t;

typedef struct me1600_ao_shadow {
	int count;
	unsigned long *registry;
	uint16_t *shadow;
	uint16_t *mirror;
	uint16_t synchronous;									/**< Synchronization list. */
	uint16_t trigger;										/**< Synchronization flag. */
} me1600_ao_shadow_t;

typedef enum ME1600_AO_STATUS {
	ao_status_none = 0,
	ao_status_single_configured,
	ao_status_single_run,
	ao_status_single_end,
	ao_status_last
} ME1600_AO_STATUS;

typedef struct me1600_ao_subdevice {
	/* Inheritance */
	me_subdevice_t base;									/**< The subdevice base class. */

	/* Attributes */
	int ao_idx;												/**< The index of the analog output subdevice on the device. */

	spinlock_t subdevice_lock;								/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *config_regs_lock;							/**< Spin lock to protect configuration registers from concurrent access. */

	int u_ranges_count;										/**< The number of voltage ranges available on this subdevice. */
	me1600_ao_range_entry_t u_ranges[ME1600_MAX_RANGES];	/**< Array holding the voltage ranges on this subdevice. */
	int i_ranges_count;										/**< The number of current ranges available on this subdevice. */
	me1600_ao_range_entry_t i_ranges[ME1600_MAX_RANGES];	/**< Array holding the current ranges on this subdevice. */

	/* Registers */
	unsigned long uni_bi_reg;								/**< Register for switching between unipoar and bipolar output mode. */
	unsigned long i_range_reg;								/**< Register for switching between ranges. */
	unsigned long sim_output_reg;							/**< Register used in order to update all channels simultaneously. */
	unsigned long current_on_reg;							/**< Register enabling current output on the fourth subdevice. */
#   ifdef PDEBUG_REG
	unsigned long reg_base;
#   endif

	ME1600_AO_STATUS status;
	me1600_ao_shadow_t *ao_regs_shadows;					/**< Addresses and shadows of output's registers. */
	spinlock_t *ao_shadows_lock;							/**< Protects the shadow's struct. */
	int mode;												/**< Mode in witch output should works. */
	wait_queue_head_t wait_queue;							/**< Wait queue to put on tasks waiting for data to arrive. */
	me1600_ao_timeout_t timeout;							/**< The timeout for start in blocking and non-blocking mode. */
	struct workqueue_struct *me1600_workqueue;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
	struct work_struct ao_control_task;
#else
	struct delayed_work ao_control_task;
#endif

	volatile int ao_control_task_flag;						/**< Flag controling reexecuting of control task */
} me1600_ao_subdevice_t;

me1600_ao_subdevice_t *me1600_ao_constructor(uint32_t reg_base,
					     unsigned int ao_idx,
					     int curr,
					     spinlock_t * config_regs_lock,
					     spinlock_t * ao_shadows_lock,
					     me1600_ao_shadow_t *
					     ao_regs_shadows,
					     struct workqueue_struct
					     *me1600_wq);

# endif	//__KERNEL__
#endif //_ME1600_AO_H_
