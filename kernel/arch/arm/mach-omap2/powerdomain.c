
#ifdef CONFIG_OMAP_DEBUG_POWERDOMAIN
# define DEBUG
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/io.h>

#include <asm/atomic.h>

#include "cm.h"
#include "cm-regbits-34xx.h"
#include "prm.h"
#include "prm-regbits-34xx.h"

#include <mach/cpu.h>
#include <mach/powerdomain.h>
#include <mach/clockdomain.h>

/* pwrdm_list contains all registered struct powerdomains */
static LIST_HEAD(pwrdm_list);

static DEFINE_RWLOCK(pwrdm_rwlock);


/* Private functions */

static u32 prm_read_mod_bits_shift(s16 domain, s16 idx, u32 mask)
{
	u32 v;

	v = prm_read_mod_reg(domain, idx);
	v &= mask;
	v >>= __ffs(mask);

	return v;
}

static struct powerdomain *_pwrdm_lookup(const char *name)
{
	struct powerdomain *pwrdm, *temp_pwrdm;

	pwrdm = NULL;

	list_for_each_entry(temp_pwrdm, &pwrdm_list, node) {
		if (!strcmp(name, temp_pwrdm->name)) {
			pwrdm = temp_pwrdm;
			break;
		}
	}

	return pwrdm;
}

/* _pwrdm_deps_lookup - look up the specified powerdomain in a pwrdm list */
static struct powerdomain *_pwrdm_deps_lookup(struct powerdomain *pwrdm,
					      struct pwrdm_dep *deps)
{
	struct pwrdm_dep *pd;

	if (!pwrdm || !deps || !omap_chip_is(pwrdm->omap_chip))
		return ERR_PTR(-EINVAL);

	for (pd = deps; pd; pd++) {

		if (!omap_chip_is(pd->omap_chip))
			continue;

		if (!pd->pwrdm && pd->pwrdm_name)
			pd->pwrdm = pwrdm_lookup(pd->pwrdm_name);

		if (pd->pwrdm == pwrdm)
			break;

	}

	if (!pd)
		return ERR_PTR(-ENOENT);

	return pd->pwrdm;
}


/* Public functions */

void pwrdm_init(struct powerdomain **pwrdm_list)
{
	struct powerdomain **p = NULL;

	if (pwrdm_list)
		for (p = pwrdm_list; *p; p++)
			pwrdm_register(*p);
}

int pwrdm_register(struct powerdomain *pwrdm)
{
	unsigned long flags;
	int ret = -EINVAL;

	if (!pwrdm)
		return -EINVAL;

	if (!omap_chip_is(pwrdm->omap_chip))
		return -EINVAL;

	write_lock_irqsave(&pwrdm_rwlock, flags);
	if (_pwrdm_lookup(pwrdm->name)) {
		ret = -EEXIST;
		goto pr_unlock;
	}

	list_add(&pwrdm->node, &pwrdm_list);

	pr_debug("powerdomain: registered %s\n", pwrdm->name);
	ret = 0;

pr_unlock:
	write_unlock_irqrestore(&pwrdm_rwlock, flags);

	return ret;
}

int pwrdm_unregister(struct powerdomain *pwrdm)
{
	unsigned long flags;

	if (!pwrdm)
		return -EINVAL;

	write_lock_irqsave(&pwrdm_rwlock, flags);
	list_del(&pwrdm->node);
	write_unlock_irqrestore(&pwrdm_rwlock, flags);

	pr_debug("powerdomain: unregistered %s\n", pwrdm->name);

	return 0;
}

struct powerdomain *pwrdm_lookup(const char *name)
{
	struct powerdomain *pwrdm;
	unsigned long flags;

	if (!name)
		return NULL;

	read_lock_irqsave(&pwrdm_rwlock, flags);
	pwrdm = _pwrdm_lookup(name);
	read_unlock_irqrestore(&pwrdm_rwlock, flags);

	return pwrdm;
}

int pwrdm_for_each(int (*fn)(struct powerdomain *pwrdm))
{
	struct powerdomain *temp_pwrdm;
	unsigned long flags;
	int ret = 0;

	if (!fn)
		return -EINVAL;

	read_lock_irqsave(&pwrdm_rwlock, flags);
	list_for_each_entry(temp_pwrdm, &pwrdm_list, node) {
		ret = (*fn)(temp_pwrdm);
		if (ret)
			break;
	}
	read_unlock_irqrestore(&pwrdm_rwlock, flags);

	return ret;
}

int pwrdm_add_clkdm(struct powerdomain *pwrdm, struct clockdomain *clkdm)
{
	unsigned long flags;
	int i;
	int ret = -EINVAL;

	if (!pwrdm || !clkdm)
		return -EINVAL;

	pr_debug("powerdomain: associating clockdomain %s with powerdomain "
		 "%s\n", clkdm->name, pwrdm->name);

	write_lock_irqsave(&pwrdm_rwlock, flags);

	for (i = 0; i < PWRDM_MAX_CLKDMS; i++) {
		if (!pwrdm->pwrdm_clkdms[i])
			break;
#ifdef DEBUG
		if (pwrdm->pwrdm_clkdms[i] == clkdm) {
			ret = -EINVAL;
			goto pac_exit;
		}
#endif
	}

	if (i == PWRDM_MAX_CLKDMS) {
		pr_debug("powerdomain: increase PWRDM_MAX_CLKDMS for "
			 "pwrdm %s clkdm %s\n", pwrdm->name, clkdm->name);
		WARN_ON(1);
		ret = -ENOMEM;
		goto pac_exit;
	}

	pwrdm->pwrdm_clkdms[i] = clkdm;

	ret = 0;

pac_exit:
	write_unlock_irqrestore(&pwrdm_rwlock, flags);

	return ret;
}

int pwrdm_del_clkdm(struct powerdomain *pwrdm, struct clockdomain *clkdm)
{
	unsigned long flags;
	int ret = -EINVAL;
	int i;

	if (!pwrdm || !clkdm)
		return -EINVAL;

	pr_debug("powerdomain: dissociating clockdomain %s from powerdomain "
		 "%s\n", clkdm->name, pwrdm->name);

	write_lock_irqsave(&pwrdm_rwlock, flags);

	for (i = 0; i < PWRDM_MAX_CLKDMS; i++)
		if (pwrdm->pwrdm_clkdms[i] == clkdm)
			break;

	if (i == PWRDM_MAX_CLKDMS) {
		pr_debug("powerdomain: clkdm %s not associated with pwrdm "
			 "%s ?!\n", clkdm->name, pwrdm->name);
		ret = -ENOENT;
		goto pdc_exit;
	}

	pwrdm->pwrdm_clkdms[i] = NULL;

	ret = 0;

pdc_exit:
	write_unlock_irqrestore(&pwrdm_rwlock, flags);

	return ret;
}

int pwrdm_for_each_clkdm(struct powerdomain *pwrdm,
			 int (*fn)(struct powerdomain *pwrdm,
				   struct clockdomain *clkdm))
{
	unsigned long flags;
	int ret = 0;
	int i;

	if (!fn)
		return -EINVAL;

	read_lock_irqsave(&pwrdm_rwlock, flags);

	for (i = 0; i < PWRDM_MAX_CLKDMS && !ret; i++)
		ret = (*fn)(pwrdm, pwrdm->pwrdm_clkdms[i]);

	read_unlock_irqrestore(&pwrdm_rwlock, flags);

	return ret;
}


int pwrdm_add_wkdep(struct powerdomain *pwrdm1, struct powerdomain *pwrdm2)
{
	struct powerdomain *p;

	if (!pwrdm1)
		return -EINVAL;

	p = _pwrdm_deps_lookup(pwrdm2, pwrdm1->wkdep_srcs);
	if (IS_ERR(p)) {
		pr_debug("powerdomain: hardware cannot set/clear wake up of "
			 "%s when %s wakes up\n", pwrdm1->name, pwrdm2->name);
		return IS_ERR(p);
	}

	pr_debug("powerdomain: hardware will wake up %s when %s wakes up\n",
		 pwrdm1->name, pwrdm2->name);

	prm_set_mod_reg_bits((1 << pwrdm2->dep_bit),
			     pwrdm1->prcm_offs, PM_WKDEP);

	return 0;
}

int pwrdm_del_wkdep(struct powerdomain *pwrdm1, struct powerdomain *pwrdm2)
{
	struct powerdomain *p;

	if (!pwrdm1)
		return -EINVAL;

	p = _pwrdm_deps_lookup(pwrdm2, pwrdm1->wkdep_srcs);
	if (IS_ERR(p)) {
		pr_debug("powerdomain: hardware cannot set/clear wake up of "
			 "%s when %s wakes up\n", pwrdm1->name, pwrdm2->name);
		return IS_ERR(p);
	}

	pr_debug("powerdomain: hardware will no longer wake up %s after %s "
		 "wakes up\n", pwrdm1->name, pwrdm2->name);

	prm_clear_mod_reg_bits((1 << pwrdm2->dep_bit),
			       pwrdm1->prcm_offs, PM_WKDEP);

	return 0;
}

int pwrdm_read_wkdep(struct powerdomain *pwrdm1, struct powerdomain *pwrdm2)
{
	struct powerdomain *p;

	if (!pwrdm1)
		return -EINVAL;

	p = _pwrdm_deps_lookup(pwrdm2, pwrdm1->wkdep_srcs);
	if (IS_ERR(p)) {
		pr_debug("powerdomain: hardware cannot set/clear wake up of "
			 "%s when %s wakes up\n", pwrdm1->name, pwrdm2->name);
		return IS_ERR(p);
	}

	return prm_read_mod_bits_shift(pwrdm1->prcm_offs, PM_WKDEP,
					(1 << pwrdm2->dep_bit));
}

int pwrdm_add_sleepdep(struct powerdomain *pwrdm1, struct powerdomain *pwrdm2)
{
	struct powerdomain *p;

	if (!pwrdm1)
		return -EINVAL;

	if (!cpu_is_omap34xx())
		return -EINVAL;

	p = _pwrdm_deps_lookup(pwrdm2, pwrdm1->sleepdep_srcs);
	if (IS_ERR(p)) {
		pr_debug("powerdomain: hardware cannot set/clear sleep "
			 "dependency affecting %s from %s\n", pwrdm1->name,
			 pwrdm2->name);
		return IS_ERR(p);
	}

	pr_debug("powerdomain: will prevent %s from sleeping if %s is active\n",
		 pwrdm1->name, pwrdm2->name);

	cm_set_mod_reg_bits((1 << pwrdm2->dep_bit),
			    pwrdm1->prcm_offs, OMAP3430_CM_SLEEPDEP);

	return 0;
}

int pwrdm_del_sleepdep(struct powerdomain *pwrdm1, struct powerdomain *pwrdm2)
{
	struct powerdomain *p;

	if (!pwrdm1)
		return -EINVAL;

	if (!cpu_is_omap34xx())
		return -EINVAL;

	p = _pwrdm_deps_lookup(pwrdm2, pwrdm1->sleepdep_srcs);
	if (IS_ERR(p)) {
		pr_debug("powerdomain: hardware cannot set/clear sleep "
			 "dependency affecting %s from %s\n", pwrdm1->name,
			 pwrdm2->name);
		return IS_ERR(p);
	}

	pr_debug("powerdomain: will no longer prevent %s from sleeping if "
		 "%s is active\n", pwrdm1->name, pwrdm2->name);

	cm_clear_mod_reg_bits((1 << pwrdm2->dep_bit),
			      pwrdm1->prcm_offs, OMAP3430_CM_SLEEPDEP);

	return 0;
}

int pwrdm_read_sleepdep(struct powerdomain *pwrdm1, struct powerdomain *pwrdm2)
{
	struct powerdomain *p;

	if (!pwrdm1)
		return -EINVAL;

	if (!cpu_is_omap34xx())
		return -EINVAL;

	p = _pwrdm_deps_lookup(pwrdm2, pwrdm1->sleepdep_srcs);
	if (IS_ERR(p)) {
		pr_debug("powerdomain: hardware cannot set/clear sleep "
			 "dependency affecting %s from %s\n", pwrdm1->name,
			 pwrdm2->name);
		return IS_ERR(p);
	}

	return prm_read_mod_bits_shift(pwrdm1->prcm_offs, OMAP3430_CM_SLEEPDEP,
					(1 << pwrdm2->dep_bit));
}

int pwrdm_get_mem_bank_count(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	return pwrdm->banks;
}

int pwrdm_set_next_pwrst(struct powerdomain *pwrdm, u8 pwrst)
{
	if (!pwrdm)
		return -EINVAL;

	if (!(pwrdm->pwrsts & (1 << pwrst)))
		return -EINVAL;

	pr_debug("powerdomain: setting next powerstate for %s to %0x\n",
		 pwrdm->name, pwrst);

	prm_rmw_mod_reg_bits(OMAP_POWERSTATE_MASK,
			     (pwrst << OMAP_POWERSTATE_SHIFT),
			     pwrdm->prcm_offs, PM_PWSTCTRL);

	return 0;
}

int pwrdm_read_next_pwrst(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	return prm_read_mod_bits_shift(pwrdm->prcm_offs, PM_PWSTCTRL,
					OMAP_POWERSTATE_MASK);
}

int pwrdm_read_pwrst(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	return prm_read_mod_bits_shift(pwrdm->prcm_offs, PM_PWSTST,
					OMAP_POWERSTATEST_MASK);
}

int pwrdm_read_prev_pwrst(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	return prm_read_mod_bits_shift(pwrdm->prcm_offs, OMAP3430_PM_PREPWSTST,
					OMAP3430_LASTPOWERSTATEENTERED_MASK);
}

int pwrdm_set_logic_retst(struct powerdomain *pwrdm, u8 pwrst)
{
	if (!pwrdm)
		return -EINVAL;

	if (!(pwrdm->pwrsts_logic_ret & (1 << pwrst)))
		return -EINVAL;

	pr_debug("powerdomain: setting next logic powerstate for %s to %0x\n",
		 pwrdm->name, pwrst);

	/*
	 * The register bit names below may not correspond to the
	 * actual names of the bits in each powerdomain's register,
	 * but the type of value returned is the same for each
	 * powerdomain.
	 */
	prm_rmw_mod_reg_bits(OMAP3430_LOGICL1CACHERETSTATE,
			     (pwrst << __ffs(OMAP3430_LOGICL1CACHERETSTATE)),
			     pwrdm->prcm_offs, PM_PWSTCTRL);

	return 0;
}

int pwrdm_set_mem_onst(struct powerdomain *pwrdm, u8 bank, u8 pwrst)
{
	u32 m;

	if (!pwrdm)
		return -EINVAL;

	if (pwrdm->banks < (bank + 1))
		return -EEXIST;

	if (!(pwrdm->pwrsts_mem_on[bank] & (1 << pwrst)))
		return -EINVAL;

	pr_debug("powerdomain: setting next memory powerstate for domain %s "
		 "bank %0x while pwrdm-ON to %0x\n", pwrdm->name, bank, pwrst);

	/*
	 * The register bit names below may not correspond to the
	 * actual names of the bits in each powerdomain's register,
	 * but the type of value returned is the same for each
	 * powerdomain.
	 */
	switch (bank) {
	case 0:
		m = OMAP3430_SHAREDL1CACHEFLATONSTATE_MASK;
		break;
	case 1:
		m = OMAP3430_L1FLATMEMONSTATE_MASK;
		break;
	case 2:
		m = OMAP3430_SHAREDL2CACHEFLATONSTATE_MASK;
		break;
	case 3:
		m = OMAP3430_L2FLATMEMONSTATE_MASK;
		break;
	default:
		WARN_ON(1); /* should never happen */
		return -EEXIST;
	}

	prm_rmw_mod_reg_bits(m, (pwrst << __ffs(m)),
			     pwrdm->prcm_offs, PM_PWSTCTRL);

	return 0;
}

int pwrdm_set_mem_retst(struct powerdomain *pwrdm, u8 bank, u8 pwrst)
{
	u32 m;

	if (!pwrdm)
		return -EINVAL;

	if (pwrdm->banks < (bank + 1))
		return -EEXIST;

	if (!(pwrdm->pwrsts_mem_ret[bank] & (1 << pwrst)))
		return -EINVAL;

	pr_debug("powerdomain: setting next memory powerstate for domain %s "
		 "bank %0x while pwrdm-RET to %0x\n", pwrdm->name, bank, pwrst);

	/*
	 * The register bit names below may not correspond to the
	 * actual names of the bits in each powerdomain's register,
	 * but the type of value returned is the same for each
	 * powerdomain.
	 */
	switch (bank) {
	case 0:
		m = OMAP3430_SHAREDL1CACHEFLATRETSTATE;
		break;
	case 1:
		m = OMAP3430_L1FLATMEMRETSTATE;
		break;
	case 2:
		m = OMAP3430_SHAREDL2CACHEFLATRETSTATE;
		break;
	case 3:
		m = OMAP3430_L2FLATMEMRETSTATE;
		break;
	default:
		WARN_ON(1); /* should never happen */
		return -EEXIST;
	}

	prm_rmw_mod_reg_bits(m, (pwrst << __ffs(m)), pwrdm->prcm_offs,
			     PM_PWSTCTRL);

	return 0;
}

int pwrdm_read_logic_pwrst(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	return prm_read_mod_bits_shift(pwrdm->prcm_offs, PM_PWSTST,
					OMAP3430_LOGICSTATEST);
}

int pwrdm_read_prev_logic_pwrst(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	/*
	 * The register bit names below may not correspond to the
	 * actual names of the bits in each powerdomain's register,
	 * but the type of value returned is the same for each
	 * powerdomain.
	 */
	return prm_read_mod_bits_shift(pwrdm->prcm_offs, OMAP3430_PM_PREPWSTST,
					OMAP3430_LASTLOGICSTATEENTERED);
}

int pwrdm_read_mem_pwrst(struct powerdomain *pwrdm, u8 bank)
{
	u32 m;

	if (!pwrdm)
		return -EINVAL;

	if (pwrdm->banks < (bank + 1))
		return -EEXIST;

	/*
	 * The register bit names below may not correspond to the
	 * actual names of the bits in each powerdomain's register,
	 * but the type of value returned is the same for each
	 * powerdomain.
	 */
	switch (bank) {
	case 0:
		m = OMAP3430_SHAREDL1CACHEFLATSTATEST_MASK;
		break;
	case 1:
		m = OMAP3430_L1FLATMEMSTATEST_MASK;
		break;
	case 2:
		m = OMAP3430_SHAREDL2CACHEFLATSTATEST_MASK;
		break;
	case 3:
		m = OMAP3430_L2FLATMEMSTATEST_MASK;
		break;
	default:
		WARN_ON(1); /* should never happen */
		return -EEXIST;
	}

	return prm_read_mod_bits_shift(pwrdm->prcm_offs, PM_PWSTST, m);
}

int pwrdm_read_prev_mem_pwrst(struct powerdomain *pwrdm, u8 bank)
{
	u32 m;

	if (!pwrdm)
		return -EINVAL;

	if (pwrdm->banks < (bank + 1))
		return -EEXIST;

	/*
	 * The register bit names below may not correspond to the
	 * actual names of the bits in each powerdomain's register,
	 * but the type of value returned is the same for each
	 * powerdomain.
	 */
	switch (bank) {
	case 0:
		m = OMAP3430_LASTMEM1STATEENTERED_MASK;
		break;
	case 1:
		m = OMAP3430_LASTMEM2STATEENTERED_MASK;
		break;
	case 2:
		m = OMAP3430_LASTSHAREDL2CACHEFLATSTATEENTERED_MASK;
		break;
	case 3:
		m = OMAP3430_LASTL2FLATMEMSTATEENTERED_MASK;
		break;
	default:
		WARN_ON(1); /* should never happen */
		return -EEXIST;
	}

	return prm_read_mod_bits_shift(pwrdm->prcm_offs,
					OMAP3430_PM_PREPWSTST, m);
}

int pwrdm_clear_all_prev_pwrst(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	/*
	 * XXX should get the powerdomain's current state here;
	 * warn & fail if it is not ON.
	 */

	pr_debug("powerdomain: clearing previous power state reg for %s\n",
		 pwrdm->name);

	prm_write_mod_reg(0, pwrdm->prcm_offs, OMAP3430_PM_PREPWSTST);

	return 0;
}

int pwrdm_enable_hdwr_sar(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	if (!(pwrdm->flags & PWRDM_HAS_HDWR_SAR))
		return -EINVAL;

	pr_debug("powerdomain: %s: setting SAVEANDRESTORE bit\n",
		 pwrdm->name);

	prm_rmw_mod_reg_bits(0, 1 << OMAP3430ES2_SAVEANDRESTORE_SHIFT,
			     pwrdm->prcm_offs, PM_PWSTCTRL);

	return 0;
}

int pwrdm_disable_hdwr_sar(struct powerdomain *pwrdm)
{
	if (!pwrdm)
		return -EINVAL;

	if (!(pwrdm->flags & PWRDM_HAS_HDWR_SAR))
		return -EINVAL;

	pr_debug("powerdomain: %s: clearing SAVEANDRESTORE bit\n",
		 pwrdm->name);

	prm_rmw_mod_reg_bits(1 << OMAP3430ES2_SAVEANDRESTORE_SHIFT, 0,
			     pwrdm->prcm_offs, PM_PWSTCTRL);

	return 0;
}

bool pwrdm_has_hdwr_sar(struct powerdomain *pwrdm)
{
	return (pwrdm && pwrdm->flags & PWRDM_HAS_HDWR_SAR) ? 1 : 0;
}

int pwrdm_wait_transition(struct powerdomain *pwrdm)
{
	u32 c = 0;

	if (!pwrdm)
		return -EINVAL;

	/*
	 * REVISIT: pwrdm_wait_transition() may be better implemented
	 * via a callback and a periodic timer check -- how long do we expect
	 * powerdomain transitions to take?
	 */

	/* XXX Is this udelay() value meaningful? */
	while ((prm_read_mod_reg(pwrdm->prcm_offs, PM_PWSTST) &
		OMAP_INTRANSITION) &&
	       (c++ < PWRDM_TRANSITION_BAILOUT))
		udelay(1);

	if (c >= PWRDM_TRANSITION_BAILOUT) {
		printk(KERN_ERR "powerdomain: waited too long for "
		       "powerdomain %s to complete transition\n", pwrdm->name);
		return -EAGAIN;
	}

	pr_debug("powerdomain: completed transition in %d loops\n", c);

	return 0;
}


