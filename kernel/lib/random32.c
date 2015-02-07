

#include <linux/types.h>
#include <linux/percpu.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/random.h>

struct rnd_state {
	u32 s1, s2, s3;
};

static DEFINE_PER_CPU(struct rnd_state, net_rand_state);

static u32 __random32(struct rnd_state *state)
{
#define TAUSWORTHE(s,a,b,c,d) ((s&c)<<d) ^ (((s <<a) ^ s)>>b)

	state->s1 = TAUSWORTHE(state->s1, 13, 19, 4294967294UL, 12);
	state->s2 = TAUSWORTHE(state->s2, 2, 25, 4294967288UL, 4);
	state->s3 = TAUSWORTHE(state->s3, 3, 11, 4294967280UL, 17);

	return (state->s1 ^ state->s2 ^ state->s3);
}

static inline u32 __seed(u32 x, u32 m)
{
	return (x < m) ? x + m : x;
}

u32 random32(void)
{
	unsigned long r;
	struct rnd_state *state = &get_cpu_var(net_rand_state);
	r = __random32(state);
	put_cpu_var(state);
	return r;
}
EXPORT_SYMBOL(random32);

void srandom32(u32 entropy)
{
	int i;
	/*
	 * No locking on the CPUs, but then somewhat random results are, well,
	 * expected.
	 */
	for_each_possible_cpu (i) {
		struct rnd_state *state = &per_cpu(net_rand_state, i);
		state->s1 = __seed(state->s1 ^ entropy, 1);
	}
}
EXPORT_SYMBOL(srandom32);

static int __init random32_init(void)
{
	int i;

	for_each_possible_cpu(i) {
		struct rnd_state *state = &per_cpu(net_rand_state,i);

#define LCG(x)	((x) * 69069)	/* super-duper LCG */
		state->s1 = __seed(LCG(i + jiffies), 1);
		state->s2 = __seed(LCG(state->s1), 7);
		state->s3 = __seed(LCG(state->s2), 15);

		/* "warm it up" */
		__random32(state);
		__random32(state);
		__random32(state);
		__random32(state);
		__random32(state);
		__random32(state);
	}
	return 0;
}
core_initcall(random32_init);

static int __init random32_reseed(void)
{
	int i;

	for_each_possible_cpu(i) {
		struct rnd_state *state = &per_cpu(net_rand_state,i);
		u32 seeds[3];

		get_random_bytes(&seeds, sizeof(seeds));
		state->s1 = __seed(seeds[0], 1);
		state->s2 = __seed(seeds[1], 7);
		state->s3 = __seed(seeds[2], 15);

		/* mix it in */
		__random32(state);
	}
	return 0;
}
late_initcall(random32_reseed);
