
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/i2c-algo-bit.h>
#include "drmP.h"
#include "drm.h"
#include "intel_drv.h"
#include "i915_drm.h"
#include "i915_drv.h"


#define I2C_RISEFALL_TIME 20

static int get_clock(void *data)
{
	struct intel_i2c_chan *chan = data;
	struct drm_i915_private *dev_priv = chan->drm_dev->dev_private;
	u32 val;

	val = I915_READ(chan->reg);
	return ((val & GPIO_CLOCK_VAL_IN) != 0);
}

static int get_data(void *data)
{
	struct intel_i2c_chan *chan = data;
	struct drm_i915_private *dev_priv = chan->drm_dev->dev_private;
	u32 val;

	val = I915_READ(chan->reg);
	return ((val & GPIO_DATA_VAL_IN) != 0);
}

static void set_clock(void *data, int state_high)
{
	struct intel_i2c_chan *chan = data;
	struct drm_device *dev = chan->drm_dev;
	struct drm_i915_private *dev_priv = chan->drm_dev->dev_private;
	u32 reserved = 0, clock_bits;

	/* On most chips, these bits must be preserved in software. */
	if (!IS_I830(dev) && !IS_845G(dev))
		reserved = I915_READ(chan->reg) & (GPIO_DATA_PULLUP_DISABLE |
						   GPIO_CLOCK_PULLUP_DISABLE);

	if (state_high)
		clock_bits = GPIO_CLOCK_DIR_IN | GPIO_CLOCK_DIR_MASK;
	else
		clock_bits = GPIO_CLOCK_DIR_OUT | GPIO_CLOCK_DIR_MASK |
			GPIO_CLOCK_VAL_MASK;
	I915_WRITE(chan->reg, reserved | clock_bits);
	udelay(I2C_RISEFALL_TIME); /* wait for the line to change state */
}

static void set_data(void *data, int state_high)
{
	struct intel_i2c_chan *chan = data;
	struct drm_device *dev = chan->drm_dev;
	struct drm_i915_private *dev_priv = chan->drm_dev->dev_private;
	u32 reserved = 0, data_bits;

	/* On most chips, these bits must be preserved in software. */
	if (!IS_I830(dev) && !IS_845G(dev))
		reserved = I915_READ(chan->reg) & (GPIO_DATA_PULLUP_DISABLE |
						   GPIO_CLOCK_PULLUP_DISABLE);

	if (state_high)
		data_bits = GPIO_DATA_DIR_IN | GPIO_DATA_DIR_MASK;
	else
		data_bits = GPIO_DATA_DIR_OUT | GPIO_DATA_DIR_MASK |
			GPIO_DATA_VAL_MASK;

	I915_WRITE(chan->reg, reserved | data_bits);
	udelay(I2C_RISEFALL_TIME); /* wait for the line to change state */
}

struct intel_i2c_chan *intel_i2c_create(struct drm_device *dev, const u32 reg,
					const char *name)
{
	struct intel_i2c_chan *chan;

	chan = kzalloc(sizeof(struct intel_i2c_chan), GFP_KERNEL);
	if (!chan)
		goto out_free;

	chan->drm_dev = dev;
	chan->reg = reg;
	snprintf(chan->adapter.name, I2C_NAME_SIZE, "intel drm %s", name);
	chan->adapter.owner = THIS_MODULE;
	chan->adapter.algo_data	= &chan->algo;
	chan->adapter.dev.parent = &dev->pdev->dev;
	chan->algo.setsda = set_data;
	chan->algo.setscl = set_clock;
	chan->algo.getsda = get_data;
	chan->algo.getscl = get_clock;
	chan->algo.udelay = 20;
	chan->algo.timeout = usecs_to_jiffies(2200);
	chan->algo.data = chan;

	i2c_set_adapdata(&chan->adapter, chan);

	if(i2c_bit_add_bus(&chan->adapter))
		goto out_free;

	/* JJJ:  raise SCL and SDA? */
	set_data(chan, 1);
	set_clock(chan, 1);
	udelay(20);

	return chan;

out_free:
	kfree(chan);
	return NULL;
}

void intel_i2c_destroy(struct intel_i2c_chan *chan)
{
	if (!chan)
		return;

	i2c_del_adapter(&chan->adapter);
	kfree(chan);
}
