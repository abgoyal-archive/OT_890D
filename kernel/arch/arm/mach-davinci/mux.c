
#include <linux/io.h>
#include <linux/spinlock.h>

#include <mach/hardware.h>

#include <mach/mux.h>

/* System control register offsets */
#define PINMUX0         0x00
#define PINMUX1         0x04

static DEFINE_SPINLOCK(mux_lock);

void davinci_mux_peripheral(unsigned int mux, unsigned int enable)
{
	u32 pinmux, muxreg = PINMUX0;

	if (mux >= DAVINCI_MUX_LEVEL2) {
		muxreg = PINMUX1;
		mux -= DAVINCI_MUX_LEVEL2;
	}

	spin_lock(&mux_lock);
	pinmux = davinci_readl(DAVINCI_SYSTEM_MODULE_BASE + muxreg);
	if (enable)
		pinmux |= (1 << mux);
	else
		pinmux &= ~(1 << mux);
	davinci_writel(pinmux, DAVINCI_SYSTEM_MODULE_BASE + muxreg);
	spin_unlock(&mux_lock);
}
