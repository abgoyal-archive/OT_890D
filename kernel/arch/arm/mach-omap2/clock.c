
#undef DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/bitops.h>

#include <mach/clock.h>
#include <mach/clockdomain.h>
#include <mach/sram.h>
#include <mach/cpu.h>
#include <asm/div64.h>

#include "memory.h"
#include "sdrc.h"
#include "clock.h"
#include "prm.h"
#include "prm-regbits-24xx.h"
#include "cm.h"
#include "cm-regbits-24xx.h"
#include "cm-regbits-34xx.h"

#define MAX_CLOCK_ENABLE_WAIT		100000

/* DPLL rate rounding: minimum DPLL multiplier, divider values */
#define DPLL_MIN_MULTIPLIER		1
#define DPLL_MIN_DIVIDER		1

/* Possible error results from _dpll_test_mult */
#define DPLL_MULT_UNDERFLOW		(1 << 0)

#define DPLL_SCALE_FACTOR		64
#define DPLL_SCALE_BASE			2
#define DPLL_ROUNDING_VAL		((DPLL_SCALE_BASE / 2) * \
					 (DPLL_SCALE_FACTOR / DPLL_SCALE_BASE))

u8 cpu_mask;


void omap2_init_clk_clkdm(struct clk *clk)
{
	struct clockdomain *clkdm;

	if (!clk->clkdm_name)
		return;

	clkdm = clkdm_lookup(clk->clkdm_name);
	if (clkdm) {
		pr_debug("clock: associated clk %s to clkdm %s\n",
			 clk->name, clk->clkdm_name);
		clk->clkdm = clkdm;
	} else {
		pr_debug("clock: could not associate clk %s to "
			 "clkdm %s\n", clk->name, clk->clkdm_name);
	}
}

void omap2_init_clksel_parent(struct clk *clk)
{
	const struct clksel *clks;
	const struct clksel_rate *clkr;
	u32 r, found = 0;

	if (!clk->clksel)
		return;

	r = __raw_readl(clk->clksel_reg) & clk->clksel_mask;
	r >>= __ffs(clk->clksel_mask);

	for (clks = clk->clksel; clks->parent && !found; clks++) {
		for (clkr = clks->rates; clkr->div && !found; clkr++) {
			if ((clkr->flags & cpu_mask) && (clkr->val == r)) {
				if (clk->parent != clks->parent) {
					pr_debug("clock: inited %s parent "
						 "to %s (was %s)\n",
						 clk->name, clks->parent->name,
						 ((clk->parent) ?
						  clk->parent->name : "NULL"));
					clk->parent = clks->parent;
				};
				found = 1;
			}
		}
	}

	if (!found)
		printk(KERN_ERR "clock: init parent: could not find "
		       "regval %0x for clock %s\n", r,  clk->name);

	return;
}

/* Returns the DPLL rate */
u32 omap2_get_dpll_rate(struct clk *clk)
{
	long long dpll_clk;
	u32 dpll_mult, dpll_div, dpll;
	struct dpll_data *dd;

	dd = clk->dpll_data;
	/* REVISIT: What do we return on error? */
	if (!dd)
		return 0;

	dpll = __raw_readl(dd->mult_div1_reg);
	dpll_mult = dpll & dd->mult_mask;
	dpll_mult >>= __ffs(dd->mult_mask);
	dpll_div = dpll & dd->div1_mask;
	dpll_div >>= __ffs(dd->div1_mask);

	dpll_clk = (long long)clk->parent->rate * dpll_mult;
	do_div(dpll_clk, dpll_div + 1);

	return dpll_clk;
}

void omap2_fixed_divisor_recalc(struct clk *clk)
{
	WARN_ON(!clk->fixed_div);

	clk->rate = clk->parent->rate / clk->fixed_div;

	if (clk->flags & RATE_PROPAGATES)
		propagate_rate(clk);
}

int omap2_wait_clock_ready(void __iomem *reg, u32 mask, const char *name)
{
	int i = 0;
	int ena = 0;

	/*
	 * 24xx uses 0 to indicate not ready, and 1 to indicate ready.
	 * 34xx reverses this, just to keep us on our toes
	 */
	if (cpu_mask & (RATE_IN_242X | RATE_IN_243X)) {
		ena = mask;
	} else if (cpu_mask & RATE_IN_343X) {
		ena = 0;
	}

	/* Wait for lock */
	while (((__raw_readl(reg) & mask) != ena) &&
	       (i++ < MAX_CLOCK_ENABLE_WAIT)) {
		udelay(1);
	}

	if (i < MAX_CLOCK_ENABLE_WAIT)
		pr_debug("Clock %s stable after %d loops\n", name, i);
	else
		printk(KERN_ERR "Clock %s didn't enable in %d tries\n",
		       name, MAX_CLOCK_ENABLE_WAIT);


	return (i < MAX_CLOCK_ENABLE_WAIT) ? 1 : 0;
};


static void omap2_clk_wait_ready(struct clk *clk)
{
	void __iomem *reg, *other_reg, *st_reg;
	u32 bit;

	/*
	 * REVISIT: This code is pretty ugly.  It would be nice to generalize
	 * it and pull it into struct clk itself somehow.
	 */
	reg = clk->enable_reg;
	if ((((u32)reg & 0xff) >= CM_FCLKEN1) &&
	    (((u32)reg & 0xff) <= OMAP24XX_CM_FCLKEN2))
		other_reg = (void __iomem *)(((u32)reg & ~0xf0) | 0x10); /* CM_ICLKEN* */
	else if ((((u32)reg & 0xff) >= CM_ICLKEN1) &&
		 (((u32)reg & 0xff) <= OMAP24XX_CM_ICLKEN4))
		other_reg = (void __iomem *)(((u32)reg & ~0xf0) | 0x00); /* CM_FCLKEN* */
	else
		return;

	/* REVISIT: What are the appropriate exclusions for 34XX? */
	/* No check for DSS or cam clocks */
	if (cpu_is_omap24xx() && ((u32)reg & 0x0f) == 0) { /* CM_{F,I}CLKEN1 */
		if (clk->enable_bit == OMAP24XX_EN_DSS2_SHIFT ||
		    clk->enable_bit == OMAP24XX_EN_DSS1_SHIFT ||
		    clk->enable_bit == OMAP24XX_EN_CAM_SHIFT)
			return;
	}

	/* REVISIT: What are the appropriate exclusions for 34XX? */
	/* OMAP3: ignore DSS-mod clocks */
	if (cpu_is_omap34xx() &&
	    (((u32)reg & ~0xff) == (u32)OMAP_CM_REGADDR(OMAP3430_DSS_MOD, 0) ||
	     ((((u32)reg & ~0xff) == (u32)OMAP_CM_REGADDR(CORE_MOD, 0)) &&
	     clk->enable_bit == OMAP3430_EN_SSI_SHIFT)))
		return;

	/* Check if both functional and interface clocks
	 * are running. */
	bit = 1 << clk->enable_bit;
	if (!(__raw_readl(other_reg) & bit))
		return;
	st_reg = (void __iomem *)(((u32)other_reg & ~0xf0) | 0x20); /* CM_IDLEST* */

	omap2_wait_clock_ready(st_reg, bit, clk->name);
}

int _omap2_clk_enable(struct clk *clk)
{
	u32 regval32;

	if (clk->flags & (ALWAYS_ENABLED | PARENT_CONTROLS_CLOCK))
		return 0;

	if (clk->enable)
		return clk->enable(clk);

	if (unlikely(clk->enable_reg == NULL)) {
		printk(KERN_ERR "clock.c: Enable for %s without enable code\n",
		       clk->name);
		return 0; /* REVISIT: -EINVAL */
	}

	regval32 = __raw_readl(clk->enable_reg);
	if (clk->flags & INVERT_ENABLE)
		regval32 &= ~(1 << clk->enable_bit);
	else
		regval32 |= (1 << clk->enable_bit);
	__raw_writel(regval32, clk->enable_reg);
	wmb();

	omap2_clk_wait_ready(clk);

	return 0;
}

/* Disables clock without considering parent dependencies or use count */
void _omap2_clk_disable(struct clk *clk)
{
	u32 regval32;

	if (clk->flags & (ALWAYS_ENABLED | PARENT_CONTROLS_CLOCK))
		return;

	if (clk->disable) {
		clk->disable(clk);
		return;
	}

	if (clk->enable_reg == NULL) {
		/*
		 * 'Independent' here refers to a clock which is not
		 * controlled by its parent.
		 */
		printk(KERN_ERR "clock: clk_disable called on independent "
		       "clock %s which has no enable_reg\n", clk->name);
		return;
	}

	regval32 = __raw_readl(clk->enable_reg);
	if (clk->flags & INVERT_ENABLE)
		regval32 |= (1 << clk->enable_bit);
	else
		regval32 &= ~(1 << clk->enable_bit);
	__raw_writel(regval32, clk->enable_reg);
	wmb();
}

void omap2_clk_disable(struct clk *clk)
{
	if (clk->usecount > 0 && !(--clk->usecount)) {
		_omap2_clk_disable(clk);
		if (likely((u32)clk->parent))
			omap2_clk_disable(clk->parent);
		if (clk->clkdm)
			omap2_clkdm_clk_disable(clk->clkdm, clk);

	}
}

int omap2_clk_enable(struct clk *clk)
{
	int ret = 0;

	if (clk->usecount++ == 0) {
		if (likely((u32)clk->parent))
			ret = omap2_clk_enable(clk->parent);

		if (unlikely(ret != 0)) {
			clk->usecount--;
			return ret;
		}

		if (clk->clkdm)
			omap2_clkdm_clk_enable(clk->clkdm, clk);

		ret = _omap2_clk_enable(clk);

		if (unlikely(ret != 0)) {
			if (clk->clkdm)
				omap2_clkdm_clk_disable(clk->clkdm, clk);

			if (clk->parent) {
				omap2_clk_disable(clk->parent);
				clk->usecount--;
			}
		}
	}

	return ret;
}

void omap2_clksel_recalc(struct clk *clk)
{
	u32 div = 0;

	pr_debug("clock: recalc'ing clksel clk %s\n", clk->name);

	div = omap2_clksel_get_divisor(clk);
	if (div == 0)
		return;

	if (unlikely(clk->rate == clk->parent->rate / div))
		return;
	clk->rate = clk->parent->rate / div;

	pr_debug("clock: new clock rate is %ld (div %d)\n", clk->rate, div);

	if (unlikely(clk->flags & RATE_PROPAGATES))
		propagate_rate(clk);
}

const struct clksel *omap2_get_clksel_by_parent(struct clk *clk,
						struct clk *src_clk)
{
	const struct clksel *clks;

	if (!clk->clksel)
		return NULL;

	for (clks = clk->clksel; clks->parent; clks++) {
		if (clks->parent == src_clk)
			break; /* Found the requested parent */
	}

	if (!clks->parent) {
		printk(KERN_ERR "clock: Could not find parent clock %s in "
		       "clksel array of clock %s\n", src_clk->name,
		       clk->name);
		return NULL;
	}

	return clks;
}

u32 omap2_clksel_round_rate_div(struct clk *clk, unsigned long target_rate,
				u32 *new_div)
{
	unsigned long test_rate;
	const struct clksel *clks;
	const struct clksel_rate *clkr;
	u32 last_div = 0;

	printk(KERN_INFO "clock: clksel_round_rate_div: %s target_rate %ld\n",
	       clk->name, target_rate);

	*new_div = 1;

	clks = omap2_get_clksel_by_parent(clk, clk->parent);
	if (clks == NULL)
		return ~0;

	for (clkr = clks->rates; clkr->div; clkr++) {
		if (!(clkr->flags & cpu_mask))
		    continue;

		/* Sanity check */
		if (clkr->div <= last_div)
			printk(KERN_ERR "clock: clksel_rate table not sorted "
			       "for clock %s", clk->name);

		last_div = clkr->div;

		test_rate = clk->parent->rate / clkr->div;

		if (test_rate <= target_rate)
			break; /* found it */
	}

	if (!clkr->div) {
		printk(KERN_ERR "clock: Could not find divisor for target "
		       "rate %ld for clock %s parent %s\n", target_rate,
		       clk->name, clk->parent->name);
		return ~0;
	}

	*new_div = clkr->div;

	printk(KERN_INFO "clock: new_div = %d, new_rate = %ld\n", *new_div,
	       (clk->parent->rate / clkr->div));

	return (clk->parent->rate / clkr->div);
}

long omap2_clksel_round_rate(struct clk *clk, unsigned long target_rate)
{
	u32 new_div;

	return omap2_clksel_round_rate_div(clk, target_rate, &new_div);
}


/* Given a clock and a rate apply a clock specific rounding function */
long omap2_clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (clk->round_rate != NULL)
		return clk->round_rate(clk, rate);

	if (clk->flags & RATE_FIXED)
		printk(KERN_ERR "clock: generic omap2_clk_round_rate called "
		       "on fixed-rate clock %s\n", clk->name);

	return clk->rate;
}

u32 omap2_clksel_to_divisor(struct clk *clk, u32 field_val)
{
	const struct clksel *clks;
	const struct clksel_rate *clkr;

	clks = omap2_get_clksel_by_parent(clk, clk->parent);
	if (clks == NULL)
		return 0;

	for (clkr = clks->rates; clkr->div; clkr++) {
		if ((clkr->flags & cpu_mask) && (clkr->val == field_val))
			break;
	}

	if (!clkr->div) {
		printk(KERN_ERR "clock: Could not find fieldval %d for "
		       "clock %s parent %s\n", field_val, clk->name,
		       clk->parent->name);
		return 0;
	}

	return clkr->div;
}

u32 omap2_divisor_to_clksel(struct clk *clk, u32 div)
{
	const struct clksel *clks;
	const struct clksel_rate *clkr;

	/* should never happen */
	WARN_ON(div == 0);

	clks = omap2_get_clksel_by_parent(clk, clk->parent);
	if (clks == NULL)
		return ~0;

	for (clkr = clks->rates; clkr->div; clkr++) {
		if ((clkr->flags & cpu_mask) && (clkr->div == div))
			break;
	}

	if (!clkr->div) {
		printk(KERN_ERR "clock: Could not find divisor %d for "
		       "clock %s parent %s\n", div, clk->name,
		       clk->parent->name);
		return ~0;
	}

	return clkr->val;
}

void __iomem *omap2_get_clksel(struct clk *clk, u32 *field_mask)
{
	if (unlikely((clk->clksel_reg == NULL) || (clk->clksel_mask == NULL)))
		return NULL;

	*field_mask = clk->clksel_mask;

	return clk->clksel_reg;
}

u32 omap2_clksel_get_divisor(struct clk *clk)
{
	u32 field_mask, field_val;
	void __iomem *div_addr;

	div_addr = omap2_get_clksel(clk, &field_mask);
	if (div_addr == NULL)
		return 0;

	field_val = __raw_readl(div_addr) & field_mask;
	field_val >>= __ffs(field_mask);

	return omap2_clksel_to_divisor(clk, field_val);
}

int omap2_clksel_set_rate(struct clk *clk, unsigned long rate)
{
	u32 field_mask, field_val, reg_val, validrate, new_div = 0;
	void __iomem *div_addr;

	validrate = omap2_clksel_round_rate_div(clk, rate, &new_div);
	if (validrate != rate)
		return -EINVAL;

	div_addr = omap2_get_clksel(clk, &field_mask);
	if (div_addr == NULL)
		return -EINVAL;

	field_val = omap2_divisor_to_clksel(clk, new_div);
	if (field_val == ~0)
		return -EINVAL;

	reg_val = __raw_readl(div_addr);
	reg_val &= ~field_mask;
	reg_val |= (field_val << __ffs(field_mask));
	__raw_writel(reg_val, div_addr);
	wmb();

	clk->rate = clk->parent->rate / new_div;

	if (clk->flags & DELAYED_APP && cpu_is_omap24xx()) {
		prm_write_mod_reg(OMAP24XX_VALID_CONFIG,
			OMAP24XX_GR_MOD, OMAP24XX_PRCM_CLKCFG_CTRL_OFFSET);
		wmb();
	}

	return 0;
}


/* Set the clock rate for a clock source */
int omap2_clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = -EINVAL;

	pr_debug("clock: set_rate for clock %s to rate %ld\n", clk->name, rate);

	/* CONFIG_PARTICIPANT clocks are changed only in sets via the
	   rate table mechanism, driven by mpu_speed  */
	if (clk->flags & CONFIG_PARTICIPANT)
		return -EINVAL;

	/* dpll_ck, core_ck, virt_prcm_set; plus all clksel clocks */
	if (clk->set_rate != NULL)
		ret = clk->set_rate(clk, rate);

	if (unlikely(ret == 0 && (clk->flags & RATE_PROPAGATES)))
		propagate_rate(clk);

	return ret;
}

static u32 omap2_clksel_get_src_field(void __iomem **src_addr,
				      struct clk *src_clk, u32 *field_mask,
				      struct clk *clk, u32 *parent_div)
{
	const struct clksel *clks;
	const struct clksel_rate *clkr;

	*parent_div = 0;
	*src_addr = NULL;

	clks = omap2_get_clksel_by_parent(clk, src_clk);
	if (clks == NULL)
		return 0;

	for (clkr = clks->rates; clkr->div; clkr++) {
		if (clkr->flags & cpu_mask && clkr->flags & DEFAULT_RATE)
			break; /* Found the default rate for this platform */
	}

	if (!clkr->div) {
		printk(KERN_ERR "clock: Could not find default rate for "
		       "clock %s parent %s\n", clk->name,
		       src_clk->parent->name);
		return 0;
	}

	/* Should never happen.  Add a clksel mask to the struct clk. */
	WARN_ON(clk->clksel_mask == 0);

	*field_mask = clk->clksel_mask;
	*src_addr = clk->clksel_reg;
	*parent_div = clkr->div;

	return clkr->val;
}

int omap2_clk_set_parent(struct clk *clk, struct clk *new_parent)
{
	void __iomem *src_addr;
	u32 field_val, field_mask, reg_val, parent_div;

	if (unlikely(clk->flags & CONFIG_PARTICIPANT))
		return -EINVAL;

	if (!clk->clksel)
		return -EINVAL;

	field_val = omap2_clksel_get_src_field(&src_addr, new_parent,
					       &field_mask, clk, &parent_div);
	if (src_addr == NULL)
		return -EINVAL;

	if (clk->usecount > 0)
		omap2_clk_disable(clk);

	/* Set new source value (previous dividers if any in effect) */
	reg_val = __raw_readl(src_addr) & ~field_mask;
	reg_val |= (field_val << __ffs(field_mask));
	__raw_writel(reg_val, src_addr);
	wmb();

	if (clk->flags & DELAYED_APP && cpu_is_omap24xx()) {
		__raw_writel(OMAP24XX_VALID_CONFIG, OMAP24XX_PRCM_CLKCFG_CTRL);
		wmb();
	}

	clk->parent = new_parent;

	if (clk->usecount > 0)
		omap2_clk_enable(clk);

	/* CLKSEL clocks follow their parents' rates, divided by a divisor */
	clk->rate = new_parent->rate;

	if (parent_div > 0)
		clk->rate /= parent_div;

	pr_debug("clock: set parent of %s to %s (new rate %ld)\n",
		 clk->name, clk->parent->name, clk->rate);

	if (unlikely(clk->flags & RATE_PROPAGATES))
		propagate_rate(clk);

	return 0;
}

/* DPLL rate rounding code */

int omap2_dpll_set_rate_tolerance(struct clk *clk, unsigned int tolerance)
{
	if (!clk || !clk->dpll_data)
		return -EINVAL;

	clk->dpll_data->rate_tolerance = tolerance;

	return 0;
}

static unsigned long _dpll_compute_new_rate(unsigned long parent_rate, unsigned int m, unsigned int n)
{
	unsigned long long num;

	num = (unsigned long long)parent_rate * m;
	do_div(num, n);
	return num;
}

static int _dpll_test_mult(int *m, int n, unsigned long *new_rate,
			   unsigned long target_rate,
			   unsigned long parent_rate)
{
	int flags = 0, carry = 0;

	/* Unscale m and round if necessary */
	if (*m % DPLL_SCALE_FACTOR >= DPLL_ROUNDING_VAL)
		carry = 1;
	*m = (*m / DPLL_SCALE_FACTOR) + carry;

	/*
	 * The new rate must be <= the target rate to avoid programming
	 * a rate that is impossible for the hardware to handle
	 */
	*new_rate = _dpll_compute_new_rate(parent_rate, *m, n);
	if (*new_rate > target_rate) {
		(*m)--;
		*new_rate = 0;
	}

	/* Guard against m underflow */
	if (*m < DPLL_MIN_MULTIPLIER) {
		*m = DPLL_MIN_MULTIPLIER;
		*new_rate = 0;
		flags = DPLL_MULT_UNDERFLOW;
	}

	if (*new_rate == 0)
		*new_rate = _dpll_compute_new_rate(parent_rate, *m, n);

	return flags;
}

long omap2_dpll_round_rate(struct clk *clk, unsigned long target_rate)
{
	int m, n, r, e, scaled_max_m;
	unsigned long scaled_rt_rp, new_rate;
	int min_e = -1, min_e_m = -1, min_e_n = -1;

	if (!clk || !clk->dpll_data)
		return ~0;

	pr_debug("clock: starting DPLL round_rate for clock %s, target rate "
		 "%ld\n", clk->name, target_rate);

	scaled_rt_rp = target_rate / (clk->parent->rate / DPLL_SCALE_FACTOR);
	scaled_max_m = clk->dpll_data->max_multiplier * DPLL_SCALE_FACTOR;

	clk->dpll_data->last_rounded_rate = 0;

	for (n = clk->dpll_data->max_divider; n >= DPLL_MIN_DIVIDER; n--) {

		/* Compute the scaled DPLL multiplier, based on the divider */
		m = scaled_rt_rp * n;

		/*
		 * Since we're counting n down, a m overflow means we can
		 * can immediately skip to the next n
		 */
		if (m > scaled_max_m)
			continue;

		r = _dpll_test_mult(&m, n, &new_rate, target_rate,
				    clk->parent->rate);

		e = target_rate - new_rate;
		pr_debug("clock: n = %d: m = %d: rate error is %d "
			 "(new_rate = %ld)\n", n, m, e, new_rate);

		if (min_e == -1 ||
		    min_e >= (int)(abs(e) - clk->dpll_data->rate_tolerance)) {
			min_e = e;
			min_e_m = m;
			min_e_n = n;

			pr_debug("clock: found new least error %d\n", min_e);
		}

		/*
		 * Since we're counting n down, a m underflow means we
		 * can bail out completely (since as n decreases in
		 * the next iteration, there's no way that m can
		 * increase beyond the current m)
		 */
		if (r & DPLL_MULT_UNDERFLOW)
			break;
	}

	if (min_e < 0) {
		pr_debug("clock: error: target rate or tolerance too low\n");
		return ~0;
	}

	clk->dpll_data->last_rounded_m = min_e_m;
	clk->dpll_data->last_rounded_n = min_e_n;
	clk->dpll_data->last_rounded_rate =
		_dpll_compute_new_rate(clk->parent->rate, min_e_m,  min_e_n);

	pr_debug("clock: final least error: e = %d, m = %d, n = %d\n",
		 min_e, min_e_m, min_e_n);
	pr_debug("clock: final rate: %ld  (target rate: %ld)\n",
		 clk->dpll_data->last_rounded_rate, target_rate);

	return clk->dpll_data->last_rounded_rate;
}


#ifdef CONFIG_OMAP_RESET_CLOCKS
void omap2_clk_disable_unused(struct clk *clk)
{
	u32 regval32, v;

	v = (clk->flags & INVERT_ENABLE) ? (1 << clk->enable_bit) : 0;

	regval32 = __raw_readl(clk->enable_reg);
	if ((regval32 & (1 << clk->enable_bit)) == v)
		return;

	printk(KERN_INFO "Disabling unused clock \"%s\"\n", clk->name);
	_omap2_clk_disable(clk);
}
#endif
