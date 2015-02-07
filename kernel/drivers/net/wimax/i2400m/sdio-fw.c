
#include <linux/mmc/sdio_func.h>
#include "i2400m-sdio.h"


#define D_SUBMODULE fw
#include "sdio-debug-levels.h"

ssize_t i2400ms_bus_bm_cmd_send(struct i2400m *i2400m,
				const struct i2400m_bootrom_header *_cmd,
				size_t cmd_size, int flags)
{
	ssize_t result;
	struct device *dev = i2400m_dev(i2400m);
	struct i2400ms *i2400ms = container_of(i2400m, struct i2400ms, i2400m);
	int opcode = _cmd == NULL ? -1 : i2400m_brh_get_opcode(_cmd);
	struct i2400m_bootrom_header *cmd;
	/* SDIO restriction */
	size_t cmd_size_a = ALIGN(cmd_size, I2400MS_BLK_SIZE);

	d_fnstart(5, dev, "(i2400m %p cmd %p size %zu)\n",
		  i2400m, _cmd, cmd_size);
	result = -E2BIG;
	if (cmd_size > I2400M_BM_CMD_BUF_SIZE)
		goto error_too_big;

	memcpy(i2400m->bm_cmd_buf, _cmd, cmd_size);	/* Prep command */
	cmd = i2400m->bm_cmd_buf;
	if (cmd_size_a > cmd_size)			/* Zero pad space */
		memset(i2400m->bm_cmd_buf + cmd_size, 0, cmd_size_a - cmd_size);
	if ((flags & I2400M_BM_CMD_RAW) == 0) {
		if (WARN_ON(i2400m_brh_get_response_required(cmd) == 0))
			dev_warn(dev, "SW BUG: response_required == 0\n");
		i2400m_bm_cmd_prepare(cmd);
	}
	d_printf(4, dev, "BM cmd %d: %zu bytes (%zu padded)\n",
		 opcode, cmd_size, cmd_size_a);
	d_dump(5, dev, cmd, cmd_size);

	sdio_claim_host(i2400ms->func);			/* Send & check */
	result = sdio_memcpy_toio(i2400ms->func, I2400MS_DATA_ADDR,
				  i2400m->bm_cmd_buf, cmd_size_a);
	sdio_release_host(i2400ms->func);
	if (result < 0) {
		dev_err(dev, "BM cmd %d: cannot send: %ld\n",
			opcode, (long) result);
		goto error_cmd_send;
	}
	result = cmd_size;
error_cmd_send:
error_too_big:
	d_fnend(5, dev, "(i2400m %p cmd %p size %zu) = %d\n",
		i2400m, _cmd, cmd_size, (int) result);
	return result;
}


ssize_t i2400ms_bus_bm_wait_for_ack(struct i2400m *i2400m,
				    struct i2400m_bootrom_header *ack,
				    size_t ack_size)
{
	int result;
	ssize_t rx_size;
	u64 timeout;
	struct i2400ms *i2400ms = container_of(i2400m, struct i2400ms, i2400m);
	struct sdio_func *func = i2400ms->func;
	struct device *dev = &func->dev;

	BUG_ON(sizeof(*ack) > ack_size);

	d_fnstart(5, dev, "(i2400m %p ack %p size %zu)\n",
		  i2400m, ack, ack_size);

	timeout = get_jiffies_64() + 2 * HZ;
	sdio_claim_host(func);
	while (1) {
		if (time_after64(get_jiffies_64(), timeout)) {
			rx_size = -ETIMEDOUT;
			dev_err(dev, "timeout waiting for ack data\n");
			goto error_timedout;
		}

		/* Find the RX size, check if it fits or not -- it if
		 * doesn't fit, fail, as we have no way to dispose of
		 * the extra data. */
		rx_size = __i2400ms_rx_get_size(i2400ms);
		if (rx_size < 0)
			goto error_rx_get_size;
		result = -ENOSPC;		/* Check it fits */
		if (rx_size < sizeof(*ack)) {
			rx_size = -EIO;
			dev_err(dev, "HW BUG? received is too small (%zu vs "
				"%zu needed)\n", sizeof(*ack), rx_size);
			goto error_too_small;
		}
		if (rx_size > I2400M_BM_ACK_BUF_SIZE) {
			dev_err(dev, "SW BUG? BM_ACK_BUF is too small (%u vs "
				"%zu needed)\n", I2400M_BM_ACK_BUF_SIZE,
				rx_size);
			goto error_too_small;
		}

		/* Read it */
		result = sdio_memcpy_fromio(func, i2400m->bm_ack_buf,
					    I2400MS_DATA_ADDR, rx_size);
		if (result == -ETIMEDOUT || result == -ETIME)
			continue;
		if (result < 0) {
			dev_err(dev, "BM SDIO receive (%zu B) failed: %d\n",
				rx_size, result);
			goto error_read;
		} else
			break;
	}
	rx_size = min((ssize_t)ack_size, rx_size);
	memcpy(ack, i2400m->bm_ack_buf, rx_size);
error_read:
error_too_small:
error_rx_get_size:
error_timedout:
	sdio_release_host(func);
	d_fnend(5, dev, "(i2400m %p ack %p size %zu) = %ld\n",
		i2400m, ack, ack_size, (long) rx_size);
	return rx_size;
}
