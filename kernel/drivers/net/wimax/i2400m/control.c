

#include <stdarg.h>
#include "i2400m.h"
#include <linux/kernel.h>
#include <linux/wimax/i2400m.h>


#define D_SUBMODULE control
#include "debug-levels.h"


static
ssize_t i2400m_tlv_match(const struct i2400m_tlv_hdr *tlv,
		     enum i2400m_tlv tlv_type, ssize_t tlv_size)
{
	if (le16_to_cpu(tlv->type) != tlv_type)	/* Not our type? skip */
		return -1;
	if (tlv_size != -1
	    && le16_to_cpu(tlv->length) + sizeof(*tlv) != tlv_size) {
		size_t size = le16_to_cpu(tlv->length) + sizeof(*tlv);
		printk(KERN_WARNING "W: tlv type 0x%x mismatched because of "
		       "size (got %zu vs %zu expected)\n",
		       tlv_type, size, tlv_size);
		return size;
	}
	return 0;
}


static
const struct i2400m_tlv_hdr *i2400m_tlv_buffer_walk(
	struct i2400m *i2400m,
	const void *tlv_buf, size_t buf_size,
	const struct i2400m_tlv_hdr *tlv_pos)
{
	struct device *dev = i2400m_dev(i2400m);
	const struct i2400m_tlv_hdr *tlv_top = tlv_buf + buf_size;
	size_t offset, length, avail_size;
	unsigned type;

	if (tlv_pos == NULL)	/* Take the first one? */
		tlv_pos = tlv_buf;
	else			/* Nope, the next one */
		tlv_pos = (void *) tlv_pos
			+ le16_to_cpu(tlv_pos->length) + sizeof(*tlv_pos);
	if (tlv_pos == tlv_top) {	/* buffer done */
		tlv_pos = NULL;
		goto error_beyond_end;
	}
	if (tlv_pos > tlv_top) {
		tlv_pos = NULL;
		WARN_ON(1);
		goto error_beyond_end;
	}
	offset = (void *) tlv_pos - (void *) tlv_buf;
	avail_size = buf_size - offset;
	if (avail_size < sizeof(*tlv_pos)) {
		dev_err(dev, "HW BUG? tlv_buf %p [%zu bytes], tlv @%zu: "
			"short header\n", tlv_buf, buf_size, offset);
		goto error_short_header;
	}
	type = le16_to_cpu(tlv_pos->type);
	length = le16_to_cpu(tlv_pos->length);
	if (avail_size < sizeof(*tlv_pos) + length) {
		dev_err(dev, "HW BUG? tlv_buf %p [%zu bytes], "
			"tlv type 0x%04x @%zu: "
			"short data (%zu bytes vs %zu needed)\n",
			tlv_buf, buf_size, type, offset, avail_size,
			sizeof(*tlv_pos) + length);
		goto error_short_header;
	}
error_short_header:
error_beyond_end:
	return tlv_pos;
}


static
const struct i2400m_tlv_hdr *i2400m_tlv_find(
	struct i2400m *i2400m,
	const struct i2400m_tlv_hdr *tlv_hdr, size_t size,
	enum i2400m_tlv tlv_type, ssize_t tlv_size)
{
	ssize_t match;
	struct device *dev = i2400m_dev(i2400m);
	const struct i2400m_tlv_hdr *tlv = NULL;
	while ((tlv = i2400m_tlv_buffer_walk(i2400m, tlv_hdr, size, tlv))) {
		match = i2400m_tlv_match(tlv, tlv_type, tlv_size);
		if (match == 0)		/* found it :) */
			break;
		if (match > 0)
			dev_warn(dev, "TLV type 0x%04x found with size "
				 "mismatch (%zu vs %zu needed)\n",
				 tlv_type, match, tlv_size);
	}
	return tlv;
}


static const struct
{
	char *msg;
	int errno;
} ms_to_errno[I2400M_MS_MAX] = {
	[I2400M_MS_DONE_OK] = { "", 0 },
	[I2400M_MS_DONE_IN_PROGRESS] = { "", 0 },
	[I2400M_MS_INVALID_OP] = { "invalid opcode", -ENOSYS },
	[I2400M_MS_BAD_STATE] = { "invalid state", -EILSEQ },
	[I2400M_MS_ILLEGAL_VALUE] = { "illegal value", -EINVAL },
	[I2400M_MS_MISSING_PARAMS] = { "missing parameters", -ENOMSG },
	[I2400M_MS_VERSION_ERROR] = { "bad version", -EIO },
	[I2400M_MS_ACCESSIBILITY_ERROR] = { "accesibility error", -EIO },
	[I2400M_MS_BUSY] = { "busy", -EBUSY },
	[I2400M_MS_CORRUPTED_TLV] = { "corrupted TLV", -EILSEQ },
	[I2400M_MS_UNINITIALIZED] = { "not unitialized", -EILSEQ },
	[I2400M_MS_UNKNOWN_ERROR] = { "unknown error", -EIO },
	[I2400M_MS_PRODUCTION_ERROR] = { "production error", -EIO },
	[I2400M_MS_NO_RF] = { "no RF", -EIO },
	[I2400M_MS_NOT_READY_FOR_POWERSAVE] =
		{ "not ready for powersave", -EACCES },
	[I2400M_MS_THERMAL_CRITICAL] = { "thermal critical", -EL3HLT },
};


int i2400m_msg_check_status(const struct i2400m_l3l4_hdr *l3l4_hdr,
			    char *strbuf, size_t strbuf_size)
{
	int result;
	enum i2400m_ms status = le16_to_cpu(l3l4_hdr->status);
	const char *str;

	if (status == 0)
		return 0;
	if (status > ARRAY_SIZE(ms_to_errno)) {
		str = "unknown status code";
		result = -EBADR;
	} else {
		str = ms_to_errno[status].msg;
		result = ms_to_errno[status].errno;
	}
	if (strbuf)
		snprintf(strbuf, strbuf_size, "%s (%d)", str, status);
	return result;
}


static
void i2400m_report_tlv_system_state(struct i2400m *i2400m,
				    const struct i2400m_tlv_system_state *ss)
{
	struct device *dev = i2400m_dev(i2400m);
	struct wimax_dev *wimax_dev = &i2400m->wimax_dev;
	enum i2400m_system_state i2400m_state = le32_to_cpu(ss->state);

	d_fnstart(3, dev, "(i2400m %p ss %p [%u])\n", i2400m, ss, i2400m_state);

	if (unlikely(i2400m->ready == 0))	/* act if up */
		goto out;
	if (i2400m->state != i2400m_state) {
		i2400m->state = i2400m_state;
		wake_up_all(&i2400m->state_wq);
	}
	switch (i2400m_state) {
	case I2400M_SS_UNINITIALIZED:
	case I2400M_SS_INIT:
	case I2400M_SS_CONFIG:
	case I2400M_SS_PRODUCTION:
		wimax_state_change(wimax_dev, WIMAX_ST_UNINITIALIZED);
		break;

	case I2400M_SS_RF_OFF:
	case I2400M_SS_RF_SHUTDOWN:
		wimax_state_change(wimax_dev, WIMAX_ST_RADIO_OFF);
		break;

	case I2400M_SS_READY:
	case I2400M_SS_STANDBY:
	case I2400M_SS_SLEEPACTIVE:
		wimax_state_change(wimax_dev, WIMAX_ST_READY);
		break;

	case I2400M_SS_CONNECTING:
	case I2400M_SS_WIMAX_CONNECTED:
		wimax_state_change(wimax_dev, WIMAX_ST_READY);
		break;

	case I2400M_SS_SCAN:
	case I2400M_SS_OUT_OF_ZONE:
		wimax_state_change(wimax_dev, WIMAX_ST_SCANNING);
		break;

	case I2400M_SS_IDLE:
		d_printf(1, dev, "entering BS-negotiated idle mode\n");
	case I2400M_SS_DISCONNECTING:
	case I2400M_SS_DATA_PATH_CONNECTED:
		wimax_state_change(wimax_dev, WIMAX_ST_CONNECTED);
		break;

	default:
		/* Huh? just in case, shut it down */
		dev_err(dev, "HW BUG? unknown state %u: shutting down\n",
			i2400m_state);
		i2400m->bus_reset(i2400m, I2400M_RT_WARM);
		break;
	};
out:
	d_fnend(3, dev, "(i2400m %p ss %p [%u]) = void\n",
		i2400m, ss, i2400m_state);
}


static
void i2400m_report_tlv_media_status(struct i2400m *i2400m,
				    const struct i2400m_tlv_media_status *ms)
{
	struct device *dev = i2400m_dev(i2400m);
	struct wimax_dev *wimax_dev = &i2400m->wimax_dev;
	struct net_device *net_dev = wimax_dev->net_dev;
	enum i2400m_media_status status = le32_to_cpu(ms->media_status);

	d_fnstart(3, dev, "(i2400m %p ms %p [%u])\n", i2400m, ms, status);

	if (unlikely(i2400m->ready == 0))	/* act if up */
		goto out;
	switch (status) {
	case I2400M_MEDIA_STATUS_LINK_UP:
		netif_carrier_on(net_dev);
		break;
	case I2400M_MEDIA_STATUS_LINK_DOWN:
		netif_carrier_off(net_dev);
		break;
	/*
	 * This is the network telling us we need to retrain the DHCP
	 * lease -- so far, we are trusting the WiMAX Network Service
	 * in user space to pick this up and poke the DHCP client.
	 */
	case I2400M_MEDIA_STATUS_LINK_RENEW:
		netif_carrier_on(net_dev);
		break;
	default:
		dev_err(dev, "HW BUG? unknown media status %u\n",
			status);
	};
out:
	d_fnend(3, dev, "(i2400m %p ms %p [%u]) = void\n",
		i2400m, ms, status);
}


static
void i2400m_report_state_hook(struct i2400m *i2400m,
			      const struct i2400m_l3l4_hdr *l3l4_hdr,
			      size_t size, const char *tag)
{
	struct device *dev = i2400m_dev(i2400m);
	const struct i2400m_tlv_hdr *tlv;
	const struct i2400m_tlv_system_state *ss;
	const struct i2400m_tlv_rf_switches_status *rfss;
	const struct i2400m_tlv_media_status *ms;
	size_t tlv_size = le16_to_cpu(l3l4_hdr->length);

	d_fnstart(4, dev, "(i2400m %p, l3l4_hdr %p, size %zu, %s)\n",
		  i2400m, l3l4_hdr, size, tag);
	tlv = NULL;

	while ((tlv = i2400m_tlv_buffer_walk(i2400m, &l3l4_hdr->pl,
					     tlv_size, tlv))) {
		if (0 == i2400m_tlv_match(tlv, I2400M_TLV_SYSTEM_STATE,
					  sizeof(*ss))) {
			ss = container_of(tlv, typeof(*ss), hdr);
			d_printf(2, dev, "%s: system state TLV "
				 "found (0x%04x), state 0x%08x\n",
				 tag, I2400M_TLV_SYSTEM_STATE,
				 le32_to_cpu(ss->state));
			i2400m_report_tlv_system_state(i2400m, ss);
		}
		if (0 == i2400m_tlv_match(tlv, I2400M_TLV_RF_STATUS,
					  sizeof(*rfss))) {
			rfss = container_of(tlv, typeof(*rfss), hdr);
			d_printf(2, dev, "%s: RF status TLV "
				 "found (0x%04x), sw 0x%02x hw 0x%02x\n",
				 tag, I2400M_TLV_RF_STATUS,
				 le32_to_cpu(rfss->sw_rf_switch),
				 le32_to_cpu(rfss->hw_rf_switch));
			i2400m_report_tlv_rf_switches_status(i2400m, rfss);
		}
		if (0 == i2400m_tlv_match(tlv, I2400M_TLV_MEDIA_STATUS,
					  sizeof(*ms))) {
			ms = container_of(tlv, typeof(*ms), hdr);
			d_printf(2, dev, "%s: Media Status TLV: %u\n",
				 tag, le32_to_cpu(ms->media_status));
			i2400m_report_tlv_media_status(i2400m, ms);
		}
	}
	d_fnend(4, dev, "(i2400m %p, l3l4_hdr %p, size %zu, %s) = void\n",
		i2400m, l3l4_hdr, size, tag);
}


void i2400m_report_hook(struct i2400m *i2400m,
			const struct i2400m_l3l4_hdr *l3l4_hdr, size_t size)
{
	struct device *dev = i2400m_dev(i2400m);
	unsigned msg_type;

	d_fnstart(3, dev, "(i2400m %p l3l4_hdr %p size %zu)\n",
		  i2400m, l3l4_hdr, size);
	/* Chew on the message, we might need some information from
	 * here */
	msg_type = le16_to_cpu(l3l4_hdr->type);
	switch (msg_type) {
	case I2400M_MT_REPORT_STATE:	/* carrier detection... */
		i2400m_report_state_hook(i2400m,
					 l3l4_hdr, size, "REPORT STATE");
		break;
	/* If the device is ready for power save, then ask it to do
	 * it. */
	case I2400M_MT_REPORT_POWERSAVE_READY:	/* zzzzz */
		if (l3l4_hdr->status == cpu_to_le16(I2400M_MS_DONE_OK)) {
			d_printf(1, dev, "ready for powersave, requesting\n");
			i2400m_cmd_enter_powersave(i2400m);
		}
		break;
	};
	d_fnend(3, dev, "(i2400m %p l3l4_hdr %p size %zu) = void\n",
		i2400m, l3l4_hdr, size);
}


void i2400m_msg_ack_hook(struct i2400m *i2400m,
			 const struct i2400m_l3l4_hdr *l3l4_hdr, size_t size)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	unsigned ack_type, ack_status;
	char strerr[32];

	/* Chew on the message, we might need some information from
	 * here */
	ack_type = le16_to_cpu(l3l4_hdr->type);
	ack_status = le16_to_cpu(l3l4_hdr->status);
	switch (ack_type) {
	case I2400M_MT_CMD_ENTER_POWERSAVE:
		/* This is just left here for the sake of example, as
		 * the processing is done somewhere else. */
		if (0) {
			result = i2400m_msg_check_status(
				l3l4_hdr, strerr, sizeof(strerr));
			if (result >= 0)
				d_printf(1, dev, "ready for power save: %zd\n",
					 size);
		}
		break;
	};
	return;
}


int i2400m_msg_size_check(struct i2400m *i2400m,
			  const struct i2400m_l3l4_hdr *l3l4_hdr,
			  size_t msg_size)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	size_t expected_size;
	d_fnstart(4, dev, "(i2400m %p l3l4_hdr %p msg_size %zu)\n",
		  i2400m, l3l4_hdr, msg_size);
	if (msg_size < sizeof(*l3l4_hdr)) {
		dev_err(dev, "bad size for message header "
			"(expected at least %zu, got %zu)\n",
			(size_t) sizeof(*l3l4_hdr), msg_size);
		result = -EIO;
		goto error_hdr_size;
	}
	expected_size = le16_to_cpu(l3l4_hdr->length) + sizeof(*l3l4_hdr);
	if (msg_size < expected_size) {
		dev_err(dev, "bad size for message code 0x%04x (expected %zu, "
			"got %zu)\n", le16_to_cpu(l3l4_hdr->type),
			expected_size, msg_size);
		result = -EIO;
	} else
		result = 0;
error_hdr_size:
	d_fnend(4, dev,
		"(i2400m %p l3l4_hdr %p msg_size %zu) = %d\n",
		i2400m, l3l4_hdr, msg_size, result);
	return result;
}



void i2400m_msg_to_dev_cancel_wait(struct i2400m *i2400m, int code)
{
	struct sk_buff *ack_skb;
	unsigned long flags;

	spin_lock_irqsave(&i2400m->rx_lock, flags);
	ack_skb = i2400m->ack_skb;
	if (ack_skb && !IS_ERR(ack_skb))
		kfree_skb(ack_skb);
	i2400m->ack_skb = ERR_PTR(code);
	spin_unlock_irqrestore(&i2400m->rx_lock, flags);
}


struct sk_buff *i2400m_msg_to_dev(struct i2400m *i2400m,
				  const void *buf, size_t buf_len)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	const struct i2400m_l3l4_hdr *msg_l3l4_hdr;
	struct sk_buff *ack_skb;
	const struct i2400m_l3l4_hdr *ack_l3l4_hdr;
	size_t ack_len;
	int ack_timeout;
	unsigned msg_type;
	unsigned long flags;

	d_fnstart(3, dev, "(i2400m %p buf %p len %zu)\n",
		  i2400m, buf, buf_len);

	if (i2400m->boot_mode)
		return ERR_PTR(-ENODEV);

	msg_l3l4_hdr = buf;
	/* Check msg & payload consistency */
	result = i2400m_msg_size_check(i2400m, msg_l3l4_hdr, buf_len);
	if (result < 0)
		goto error_bad_msg;
	msg_type = le16_to_cpu(msg_l3l4_hdr->type);
	d_printf(1, dev, "CMD/GET/SET 0x%04x %zu bytes\n",
		 msg_type, buf_len);
	d_dump(2, dev, buf, buf_len);

	/* Setup the completion, ack_skb ("we are waiting") and send
	 * the message to the device */
	mutex_lock(&i2400m->msg_mutex);
	spin_lock_irqsave(&i2400m->rx_lock, flags);
	i2400m->ack_skb = ERR_PTR(-EINPROGRESS);
	spin_unlock_irqrestore(&i2400m->rx_lock, flags);
	init_completion(&i2400m->msg_completion);
	result = i2400m_tx(i2400m, buf, buf_len, I2400M_PT_CTRL);
	if (result < 0) {
		dev_err(dev, "can't send message 0x%04x: %d\n",
			le16_to_cpu(msg_l3l4_hdr->type), result);
		goto error_tx;
	}

	/* Some commands take longer to execute because of crypto ops,
	 * so we give them some more leeway on timeout */
	switch (msg_type) {
	case I2400M_MT_GET_TLS_OPERATION_RESULT:
	case I2400M_MT_CMD_SEND_EAP_RESPONSE:
		ack_timeout = 5 * HZ;
		break;
	default:
		ack_timeout = HZ;
	};

	/* The RX path in rx.c will put any response for this message
	 * in i2400m->ack_skb and wake us up. If we cancel the wait,
	 * we need to change the value of i2400m->ack_skb to something
	 * not -EINPROGRESS so RX knows there is no one waiting. */
	result = wait_for_completion_interruptible_timeout(
		&i2400m->msg_completion, ack_timeout);
	if (result == 0) {
		dev_err(dev, "timeout waiting for reply to message 0x%04x\n",
			msg_type);
		result = -ETIMEDOUT;
		i2400m_msg_to_dev_cancel_wait(i2400m, result);
		goto error_wait_for_completion;
	} else if (result < 0) {
		dev_err(dev, "error waiting for reply to message 0x%04x: %d\n",
			msg_type, result);
		i2400m_msg_to_dev_cancel_wait(i2400m, result);
		goto error_wait_for_completion;
	}

	/* Pull out the ack data from i2400m->ack_skb -- see if it is
	 * an error and act accordingly */
	spin_lock_irqsave(&i2400m->rx_lock, flags);
	ack_skb = i2400m->ack_skb;
	if (IS_ERR(ack_skb))
		result = PTR_ERR(ack_skb);
	else
		result = 0;
	i2400m->ack_skb = NULL;
	spin_unlock_irqrestore(&i2400m->rx_lock, flags);
	if (result < 0)
		goto error_ack_status;
	ack_l3l4_hdr = wimax_msg_data_len(ack_skb, &ack_len);

	/* Check the ack and deliver it if it is ok */
	result = i2400m_msg_size_check(i2400m, ack_l3l4_hdr, ack_len);
	if (result < 0) {
		dev_err(dev, "HW BUG? reply to message 0x%04x: %d\n",
			msg_type, result);
		goto error_bad_ack_len;
	}
	if (msg_type != le16_to_cpu(ack_l3l4_hdr->type)) {
		dev_err(dev, "HW BUG? bad reply 0x%04x to message 0x%04x\n",
			le16_to_cpu(ack_l3l4_hdr->type), msg_type);
		result = -EIO;
		goto error_bad_ack_type;
	}
	i2400m_msg_ack_hook(i2400m, ack_l3l4_hdr, ack_len);
	mutex_unlock(&i2400m->msg_mutex);
	d_fnend(3, dev, "(i2400m %p buf %p len %zu) = %p\n",
		i2400m, buf, buf_len, ack_skb);
	return ack_skb;

error_bad_ack_type:
error_bad_ack_len:
	kfree_skb(ack_skb);
error_ack_status:
error_wait_for_completion:
error_tx:
	mutex_unlock(&i2400m->msg_mutex);
error_bad_msg:
	d_fnend(3, dev, "(i2400m %p buf %p len %zu) = %d\n",
		i2400m, buf, buf_len, result);
	return ERR_PTR(result);
}


enum {
	I2400M_WAKEUP_ENABLED  = 0x01,
	I2400M_WAKEUP_DISABLED = 0x02,
	I2400M_TLV_TYPE_WAKEUP_MODE = 144,
};

struct i2400m_cmd_enter_power_save {
	struct i2400m_l3l4_hdr hdr;
	struct i2400m_tlv_hdr tlv;
	__le32 val;
} __attribute__((packed));


int i2400m_cmd_enter_powersave(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *ack_skb;
	struct i2400m_cmd_enter_power_save *cmd;
	char strerr[32];

	result = -ENOMEM;
	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (cmd == NULL)
		goto error_alloc;
	cmd->hdr.type = cpu_to_le16(I2400M_MT_CMD_ENTER_POWERSAVE);
	cmd->hdr.length = cpu_to_le16(sizeof(*cmd) - sizeof(cmd->hdr));
	cmd->hdr.version = cpu_to_le16(I2400M_L3L4_VERSION);
	cmd->tlv.type = cpu_to_le16(I2400M_TLV_TYPE_WAKEUP_MODE);
	cmd->tlv.length = cpu_to_le16(sizeof(cmd->val));
	cmd->val = cpu_to_le32(I2400M_WAKEUP_ENABLED);

	ack_skb = i2400m_msg_to_dev(i2400m, cmd, sizeof(*cmd));
	result = PTR_ERR(ack_skb);
	if (IS_ERR(ack_skb)) {
		dev_err(dev, "Failed to issue 'Enter power save' command: %d\n",
			result);
		goto error_msg_to_dev;
	}
	result = i2400m_msg_check_status(wimax_msg_data(ack_skb),
					 strerr, sizeof(strerr));
	if (result == -EACCES)
		d_printf(1, dev, "Cannot enter power save mode\n");
	else if (result < 0)
		dev_err(dev, "'Enter power save' (0x%04x) command failed: "
			"%d - %s\n", I2400M_MT_CMD_ENTER_POWERSAVE,
			result, strerr);
	else
		d_printf(1, dev, "device ready to power save\n");
	kfree_skb(ack_skb);
error_msg_to_dev:
	kfree(cmd);
error_alloc:
	return result;
}
EXPORT_SYMBOL_GPL(i2400m_cmd_enter_powersave);


enum {
	I2400M_TLV_DETAILED_DEVICE_INFO = 140
};

struct sk_buff *i2400m_get_device_info(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *ack_skb;
	struct i2400m_l3l4_hdr *cmd;
	const struct i2400m_l3l4_hdr *ack;
	size_t ack_len;
	const struct i2400m_tlv_hdr *tlv;
	const struct i2400m_tlv_detailed_device_info *ddi;
	char strerr[32];

	ack_skb = ERR_PTR(-ENOMEM);
	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (cmd == NULL)
		goto error_alloc;
	cmd->type = cpu_to_le16(I2400M_MT_GET_DEVICE_INFO);
	cmd->length = 0;
	cmd->version = cpu_to_le16(I2400M_L3L4_VERSION);

	ack_skb = i2400m_msg_to_dev(i2400m, cmd, sizeof(*cmd));
	if (IS_ERR(ack_skb)) {
		dev_err(dev, "Failed to issue 'get device info' command: %ld\n",
			PTR_ERR(ack_skb));
		goto error_msg_to_dev;
	}
	ack = wimax_msg_data_len(ack_skb, &ack_len);
	result = i2400m_msg_check_status(ack, strerr, sizeof(strerr));
	if (result < 0) {
		dev_err(dev, "'get device info' (0x%04x) command failed: "
			"%d - %s\n", I2400M_MT_GET_DEVICE_INFO, result,
			strerr);
		goto error_cmd_failed;
	}
	tlv = i2400m_tlv_find(i2400m, ack->pl, ack_len - sizeof(*ack),
			      I2400M_TLV_DETAILED_DEVICE_INFO, sizeof(*ddi));
	if (tlv == NULL) {
		dev_err(dev, "GET DEVICE INFO: "
			"detailed device info TLV not found (0x%04x)\n",
			I2400M_TLV_DETAILED_DEVICE_INFO);
		result = -EIO;
		goto error_no_tlv;
	}
	skb_pull(ack_skb, (void *) tlv - (void *) ack_skb->data);
error_msg_to_dev:
	kfree(cmd);
error_alloc:
	return ack_skb;

error_no_tlv:
error_cmd_failed:
	kfree_skb(ack_skb);
	kfree(cmd);
	return ERR_PTR(result);
}


/* Firmware interface versions we support */
enum {
	I2400M_HDIv_MAJOR = 9,
	I2400M_HDIv_MAJOR_2 = 8,
	I2400M_HDIv_MINOR = 1,
};


int i2400m_firmware_check(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *ack_skb;
	struct i2400m_l3l4_hdr *cmd;
	const struct i2400m_l3l4_hdr *ack;
	size_t ack_len;
	const struct i2400m_tlv_hdr *tlv;
	const struct i2400m_tlv_l4_message_versions *l4mv;
	char strerr[32];
	unsigned major, minor, branch;

	result = -ENOMEM;
	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (cmd == NULL)
		goto error_alloc;
	cmd->type = cpu_to_le16(I2400M_MT_GET_LM_VERSION);
	cmd->length = 0;
	cmd->version = cpu_to_le16(I2400M_L3L4_VERSION);

	ack_skb = i2400m_msg_to_dev(i2400m, cmd, sizeof(*cmd));
	if (IS_ERR(ack_skb)) {
		result = PTR_ERR(ack_skb);
		dev_err(dev, "Failed to issue 'get lm version' command: %-d\n",
			result);
		goto error_msg_to_dev;
	}
	ack = wimax_msg_data_len(ack_skb, &ack_len);
	result = i2400m_msg_check_status(ack, strerr, sizeof(strerr));
	if (result < 0) {
		dev_err(dev, "'get lm version' (0x%04x) command failed: "
			"%d - %s\n", I2400M_MT_GET_LM_VERSION, result,
			strerr);
		goto error_cmd_failed;
	}
	tlv = i2400m_tlv_find(i2400m, ack->pl, ack_len - sizeof(*ack),
			      I2400M_TLV_L4_MESSAGE_VERSIONS, sizeof(*l4mv));
	if (tlv == NULL) {
		dev_err(dev, "get lm version: TLV not found (0x%04x)\n",
			I2400M_TLV_L4_MESSAGE_VERSIONS);
		result = -EIO;
		goto error_no_tlv;
	}
	l4mv = container_of(tlv, typeof(*l4mv), hdr);
	major = le16_to_cpu(l4mv->major);
	minor = le16_to_cpu(l4mv->minor);
	branch = le16_to_cpu(l4mv->branch);
	result = -EINVAL;
	if (major != I2400M_HDIv_MAJOR
	    && major != I2400M_HDIv_MAJOR_2) {
		dev_err(dev, "unsupported major fw interface version "
			"%u.%u.%u\n", major, minor, branch);
		goto error_bad_major;
	}
	if (major == I2400M_HDIv_MAJOR_2)
		dev_err(dev, "deprecated major fw interface version "
			"%u.%u.%u\n", major, minor, branch);
	result = 0;
	if (minor != I2400M_HDIv_MINOR)
		dev_warn(dev, "untested minor fw firmware version %u.%u.%u\n",
			 major, minor, branch);
error_bad_major:
	dev_info(dev, "firmware interface version %u.%u.%u\n",
		 major, minor, branch);
error_no_tlv:
error_cmd_failed:
	kfree_skb(ack_skb);
error_msg_to_dev:
	kfree(cmd);
error_alloc:
	return result;
}


int i2400m_cmd_exit_idle(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *ack_skb;
	struct i2400m_l3l4_hdr *cmd;
	char strerr[32];

	result = -ENOMEM;
	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (cmd == NULL)
		goto error_alloc;
	cmd->type = cpu_to_le16(I2400M_MT_CMD_EXIT_IDLE);
	cmd->length = 0;
	cmd->version = cpu_to_le16(I2400M_L3L4_VERSION);

	ack_skb = i2400m_msg_to_dev(i2400m, cmd, sizeof(*cmd));
	result = PTR_ERR(ack_skb);
	if (IS_ERR(ack_skb)) {
		dev_err(dev, "Failed to issue 'exit idle' command: %d\n",
			result);
		goto error_msg_to_dev;
	}
	result = i2400m_msg_check_status(wimax_msg_data(ack_skb),
					 strerr, sizeof(strerr));
	kfree_skb(ack_skb);
error_msg_to_dev:
	kfree(cmd);
error_alloc:
	return result;

}


int i2400m_cmd_get_state(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *ack_skb;
	struct i2400m_l3l4_hdr *cmd;
	const struct i2400m_l3l4_hdr *ack;
	size_t ack_len;
	char strerr[32];

	result = -ENOMEM;
	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (cmd == NULL)
		goto error_alloc;
	cmd->type = cpu_to_le16(I2400M_MT_GET_STATE);
	cmd->length = 0;
	cmd->version = cpu_to_le16(I2400M_L3L4_VERSION);

	ack_skb = i2400m_msg_to_dev(i2400m, cmd, sizeof(*cmd));
	if (IS_ERR(ack_skb)) {
		dev_err(dev, "Failed to issue 'get state' command: %ld\n",
			PTR_ERR(ack_skb));
		result = PTR_ERR(ack_skb);
		goto error_msg_to_dev;
	}
	ack = wimax_msg_data_len(ack_skb, &ack_len);
	result = i2400m_msg_check_status(ack, strerr, sizeof(strerr));
	if (result < 0) {
		dev_err(dev, "'get state' (0x%04x) command failed: "
			"%d - %s\n", I2400M_MT_GET_STATE, result, strerr);
		goto error_cmd_failed;
	}
	i2400m_report_state_hook(i2400m, ack, ack_len - sizeof(*ack),
				 "GET STATE");
	result = 0;
	kfree_skb(ack_skb);
error_cmd_failed:
error_msg_to_dev:
	kfree(cmd);
error_alloc:
	return result;
}
EXPORT_SYMBOL_GPL(i2400m_cmd_get_state);


int i2400m_set_init_config(struct i2400m *i2400m,
			   const struct i2400m_tlv_hdr **arg, size_t args)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *ack_skb;
	struct i2400m_l3l4_hdr *cmd;
	char strerr[32];
	unsigned argc, argsize, tlv_size;
	const struct i2400m_tlv_hdr *tlv_hdr;
	void *buf, *itr;

	d_fnstart(3, dev, "(i2400m %p arg %p args %zu)\n", i2400m, arg, args);
	result = 0;
	if (args == 0)
		goto none;
	/* Compute the size of all the TLVs, so we can alloc a
	 * contiguous command block to copy them. */
	argsize = 0;
	for (argc = 0; argc < args; argc++) {
		tlv_hdr = arg[argc];
		argsize += sizeof(*tlv_hdr) + le16_to_cpu(tlv_hdr->length);
	}
	WARN_ON(argc >= 9);	/* As per hw spec */

	/* Alloc the space for the command and TLVs*/
	result = -ENOMEM;
	buf = kzalloc(sizeof(*cmd) + argsize, GFP_KERNEL);
	if (buf == NULL)
		goto error_alloc;
	cmd = buf;
	cmd->type = cpu_to_le16(I2400M_MT_SET_INIT_CONFIG);
	cmd->length = cpu_to_le16(argsize);
	cmd->version = cpu_to_le16(I2400M_L3L4_VERSION);

	/* Copy the TLVs */
	itr = buf + sizeof(*cmd);
	for (argc = 0; argc < args; argc++) {
		tlv_hdr = arg[argc];
		tlv_size = sizeof(*tlv_hdr) + le16_to_cpu(tlv_hdr->length);
		memcpy(itr, tlv_hdr, tlv_size);
		itr += tlv_size;
	}

	/* Send the message! */
	ack_skb = i2400m_msg_to_dev(i2400m, buf, sizeof(*cmd) + argsize);
	result = PTR_ERR(ack_skb);
	if (IS_ERR(ack_skb)) {
		dev_err(dev, "Failed to issue 'init config' command: %d\n",
			result);

		goto error_msg_to_dev;
	}
	result = i2400m_msg_check_status(wimax_msg_data(ack_skb),
					 strerr, sizeof(strerr));
	if (result < 0)
		dev_err(dev, "'init config' (0x%04x) command failed: %d - %s\n",
			I2400M_MT_SET_INIT_CONFIG, result, strerr);
	kfree_skb(ack_skb);
error_msg_to_dev:
	kfree(buf);
error_alloc:
none:
	d_fnend(3, dev, "(i2400m %p arg %p args %zu) = %d\n",
		i2400m, arg, args, result);
	return result;

}
EXPORT_SYMBOL_GPL(i2400m_set_init_config);


int i2400m_dev_initialize(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct i2400m_tlv_config_idle_parameters idle_params;
	const struct i2400m_tlv_hdr *args[9];
	unsigned argc = 0;

	d_fnstart(3, dev, "(i2400m %p)\n", i2400m);
	/* Useless for now...might change */
	if (i2400m_idle_mode_disabled) {
		idle_params.hdr.type =
			cpu_to_le16(I2400M_TLV_CONFIG_IDLE_PARAMETERS);
		idle_params.hdr.length = cpu_to_le16(
			sizeof(idle_params) - sizeof(idle_params.hdr));
		idle_params.idle_timeout = 0;
		idle_params.idle_paging_interval = 0;
		args[argc++] = &idle_params.hdr;
	}
	result = i2400m_set_init_config(i2400m, args, argc);
	if (result < 0)
		goto error;
	result = i2400m_firmware_check(i2400m);	/* fw versions ok? */
	if (result < 0)
		goto error;
	/*
	 * Update state: Here it just calls a get state; parsing the
	 * result (System State TLV and RF Status TLV [done in the rx
	 * path hooks]) will set the hardware and software RF-Kill
	 * status.
	 */
	result = i2400m_cmd_get_state(i2400m);
error:
	d_fnend(3, dev, "(i2400m %p) = %d\n", i2400m, result);
	return result;
}


void i2400m_dev_shutdown(struct i2400m *i2400m)
{
	int result = -ENODEV;
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(3, dev, "(i2400m %p)\n", i2400m);
	result = i2400m->bus_reset(i2400m, I2400M_RT_WARM);
	d_fnend(3, dev, "(i2400m %p) = void [%d]\n", i2400m, result);
	return;
}
