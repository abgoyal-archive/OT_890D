

#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/swap.h>

static DEFINE_SPINLOCK(swap_token_lock);
struct mm_struct *swap_token_mm;
static unsigned int global_faults;

void grab_swap_token(void)
{
	int current_interval;

	global_faults++;

	current_interval = global_faults - current->mm->faultstamp;

	if (!spin_trylock(&swap_token_lock))
		return;

	/* First come first served */
	if (swap_token_mm == NULL) {
		current->mm->token_priority = current->mm->token_priority + 2;
		swap_token_mm = current->mm;
		goto out;
	}

	if (current->mm != swap_token_mm) {
		if (current_interval < current->mm->last_interval)
			current->mm->token_priority++;
		else {
			if (likely(current->mm->token_priority > 0))
				current->mm->token_priority--;
		}
		/* Check if we deserve the token */
		if (current->mm->token_priority >
				swap_token_mm->token_priority) {
			current->mm->token_priority += 2;
			swap_token_mm = current->mm;
		}
	} else {
		/* Token holder came in again! */
		current->mm->token_priority += 2;
	}

out:
	current->mm->faultstamp = global_faults;
	current->mm->last_interval = current_interval;
	spin_unlock(&swap_token_lock);
return;
}

/* Called on process exit. */
void __put_swap_token(struct mm_struct *mm)
{
	spin_lock(&swap_token_lock);
	if (likely(mm == swap_token_mm))
		swap_token_mm = NULL;
	spin_unlock(&swap_token_lock);
}
