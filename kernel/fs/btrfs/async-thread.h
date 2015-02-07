

#ifndef __BTRFS_ASYNC_THREAD_
#define __BTRFS_ASYNC_THREAD_

struct btrfs_worker_thread;

struct btrfs_work {
	/*
	 * func should be set to the function you want called
	 * your work struct is passed as the only arg
	 *
	 * ordered_func must be set for work sent to an ordered work queue,
	 * and it is called to complete a given work item in the same
	 * order they were sent to the queue.
	 */
	void (*func)(struct btrfs_work *work);
	void (*ordered_func)(struct btrfs_work *work);
	void (*ordered_free)(struct btrfs_work *work);

	/*
	 * flags should be set to zero.  It is used to make sure the
	 * struct is only inserted once into the list.
	 */
	unsigned long flags;

	/* don't touch these */
	struct btrfs_worker_thread *worker;
	struct list_head list;
	struct list_head order_list;
};

struct btrfs_workers {
	/* current number of running workers */
	int num_workers;

	/* max number of workers allowed.  changed by btrfs_start_workers */
	int max_workers;

	/* once a worker has this many requests or fewer, it is idle */
	int idle_thresh;

	/* force completions in the order they were queued */
	int ordered;

	/* list with all the work threads.  The workers on the idle thread
	 * may be actively servicing jobs, but they haven't yet hit the
	 * idle thresh limit above.
	 */
	struct list_head worker_list;
	struct list_head idle_list;

	/*
	 * when operating in ordered mode, this maintains the list
	 * of work items waiting for completion
	 */
	struct list_head order_list;

	/* lock for finding the next worker thread to queue on */
	spinlock_t lock;

	/* extra name for this worker, used for current->name */
	char *name;
};

int btrfs_queue_worker(struct btrfs_workers *workers, struct btrfs_work *work);
int btrfs_start_workers(struct btrfs_workers *workers, int num_workers);
int btrfs_stop_workers(struct btrfs_workers *workers);
void btrfs_init_workers(struct btrfs_workers *workers, char *name, int max);
int btrfs_requeue_work(struct btrfs_work *work);
#endif
