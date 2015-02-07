

#undef DEBUG
#undef DEBUGDATA
#undef DEBUGCCW

#define KMSG_COMPONENT "ctcm"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/bitops.h>

#include <linux/signal.h>
#include <linux/string.h>

#include <linux/ip.h>
#include <linux/if_arp.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ctype.h>
#include <net/dst.h>

#include <linux/io.h>
#include <asm/ccwdev.h>
#include <asm/ccwgroup.h>
#include <linux/uaccess.h>

#include <asm/idals.h>

#include "cu3088.h"
#include "ctcm_fsms.h"
#include "ctcm_main.h"

/* Some common global variables */

struct channel *channels;

void ctcm_unpack_skb(struct channel *ch, struct sk_buff *pskb)
{
	struct net_device *dev = ch->netdev;
	struct ctcm_priv *priv = dev->ml_priv;
	__u16 len = *((__u16 *) pskb->data);

	skb_put(pskb, 2 + LL_HEADER_LENGTH);
	skb_pull(pskb, 2);
	pskb->dev = dev;
	pskb->ip_summed = CHECKSUM_UNNECESSARY;
	while (len > 0) {
		struct sk_buff *skb;
		int skblen;
		struct ll_header *header = (struct ll_header *)pskb->data;

		skb_pull(pskb, LL_HEADER_LENGTH);
		if ((ch->protocol == CTCM_PROTO_S390) &&
		    (header->type != ETH_P_IP)) {
			if (!(ch->logflags & LOG_FLAG_ILLEGALPKT)) {
				ch->logflags |= LOG_FLAG_ILLEGALPKT;
				/*
				 * Check packet type only if we stick strictly
				 * to S/390's protocol of OS390. This only
				 * supports IP. Otherwise allow any packet
				 * type.
				 */
				CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
					"%s(%s): Illegal packet type 0x%04x"
					" - dropping",
					CTCM_FUNTAIL, dev->name, header->type);
			}
			priv->stats.rx_dropped++;
			priv->stats.rx_frame_errors++;
			return;
		}
		pskb->protocol = ntohs(header->type);
		if (header->length <= LL_HEADER_LENGTH) {
			if (!(ch->logflags & LOG_FLAG_ILLEGALSIZE)) {
				CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
					"%s(%s): Illegal packet size %d(%d,%d)"
					"- dropping",
					CTCM_FUNTAIL, dev->name,
					header->length, dev->mtu, len);
				ch->logflags |= LOG_FLAG_ILLEGALSIZE;
			}

			priv->stats.rx_dropped++;
			priv->stats.rx_length_errors++;
			return;
		}
		header->length -= LL_HEADER_LENGTH;
		len -= LL_HEADER_LENGTH;
		if ((header->length > skb_tailroom(pskb)) ||
			(header->length > len)) {
			if (!(ch->logflags & LOG_FLAG_OVERRUN)) {
				CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
					"%s(%s): Packet size %d (overrun)"
					" - dropping", CTCM_FUNTAIL,
						dev->name, header->length);
				ch->logflags |= LOG_FLAG_OVERRUN;
			}

			priv->stats.rx_dropped++;
			priv->stats.rx_length_errors++;
			return;
		}
		skb_put(pskb, header->length);
		skb_reset_mac_header(pskb);
		len -= header->length;
		skb = dev_alloc_skb(pskb->len);
		if (!skb) {
			if (!(ch->logflags & LOG_FLAG_NOMEM)) {
				CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
					"%s(%s): MEMORY allocation error",
						CTCM_FUNTAIL, dev->name);
				ch->logflags |= LOG_FLAG_NOMEM;
			}
			priv->stats.rx_dropped++;
			return;
		}
		skb_copy_from_linear_data(pskb, skb_put(skb, pskb->len),
					  pskb->len);
		skb_reset_mac_header(skb);
		skb->dev = pskb->dev;
		skb->protocol = pskb->protocol;
		pskb->ip_summed = CHECKSUM_UNNECESSARY;
		skblen = skb->len;
		/*
		 * reset logflags
		 */
		ch->logflags = 0;
		priv->stats.rx_packets++;
		priv->stats.rx_bytes += skblen;
		netif_rx_ni(skb);
		dev->last_rx = jiffies;
		if (len > 0) {
			skb_pull(pskb, header->length);
			if (skb_tailroom(pskb) < LL_HEADER_LENGTH) {
				if (!(ch->logflags & LOG_FLAG_OVERRUN)) {
					CTCM_DBF_DEV_NAME(TRACE, dev,
						"Overrun in ctcm_unpack_skb");
					ch->logflags |= LOG_FLAG_OVERRUN;
				}
				return;
			}
			skb_put(pskb, LL_HEADER_LENGTH);
		}
	}
}

static void channel_free(struct channel *ch)
{
	CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO, "%s(%s)", CTCM_FUNTAIL, ch->id);
	ch->flags &= ~CHANNEL_FLAGS_INUSE;
	fsm_newstate(ch->fsm, CTC_STATE_IDLE);
}

static void channel_remove(struct channel *ch)
{
	struct channel **c = &channels;
	char chid[CTCM_ID_SIZE+1];
	int ok = 0;

	if (ch == NULL)
		return;
	else
		strncpy(chid, ch->id, CTCM_ID_SIZE);

	channel_free(ch);
	while (*c) {
		if (*c == ch) {
			*c = ch->next;
			fsm_deltimer(&ch->timer);
			if (IS_MPC(ch))
				fsm_deltimer(&ch->sweep_timer);

			kfree_fsm(ch->fsm);
			clear_normalized_cda(&ch->ccw[4]);
			if (ch->trans_skb != NULL) {
				clear_normalized_cda(&ch->ccw[1]);
				dev_kfree_skb_any(ch->trans_skb);
			}
			if (IS_MPC(ch)) {
				tasklet_kill(&ch->ch_tasklet);
				tasklet_kill(&ch->ch_disc_tasklet);
				kfree(ch->discontact_th);
			}
			kfree(ch->ccw);
			kfree(ch->irb);
			kfree(ch);
			ok = 1;
			break;
		}
		c = &((*c)->next);
	}

	CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO, "%s(%s) %s", CTCM_FUNTAIL,
			chid, ok ? "OK" : "failed");
}

static struct channel *channel_get(enum channel_types type,
					char *id, int direction)
{
	struct channel *ch = channels;

	while (ch && (strncmp(ch->id, id, CTCM_ID_SIZE) || (ch->type != type)))
		ch = ch->next;
	if (!ch) {
		CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
				"%s(%d, %s, %d) not found in channel list\n",
				CTCM_FUNTAIL, type, id, direction);
	} else {
		if (ch->flags & CHANNEL_FLAGS_INUSE)
			ch = NULL;
		else {
			ch->flags |= CHANNEL_FLAGS_INUSE;
			ch->flags &= ~CHANNEL_FLAGS_RWMASK;
			ch->flags |= (direction == WRITE)
			    ? CHANNEL_FLAGS_WRITE : CHANNEL_FLAGS_READ;
			fsm_newstate(ch->fsm, CTC_STATE_STOPPED);
		}
	}
	return ch;
}

static long ctcm_check_irb_error(struct ccw_device *cdev, struct irb *irb)
{
	if (!IS_ERR(irb))
		return 0;

	CTCM_DBF_TEXT_(ERROR, CTC_DBF_WARN,
			"irb error %ld on device %s\n",
				PTR_ERR(irb), dev_name(&cdev->dev));

	switch (PTR_ERR(irb)) {
	case -EIO:
		dev_err(&cdev->dev,
			"An I/O-error occurred on the CTCM device\n");
		break;
	case -ETIMEDOUT:
		dev_err(&cdev->dev,
			"An adapter hardware operation timed out\n");
		break;
	default:
		dev_err(&cdev->dev,
			"An error occurred on the adapter hardware\n");
	}
	return PTR_ERR(irb);
}


static inline void ccw_unit_check(struct channel *ch, __u8 sense)
{
	CTCM_DBF_TEXT_(TRACE, CTC_DBF_DEBUG,
			"%s(%s): %02x",
				CTCM_FUNTAIL, ch->id, sense);

	if (sense & SNS0_INTERVENTION_REQ) {
		if (sense & 0x01) {
			if (ch->sense_rc != 0x01) {
				pr_notice(
					"%s: The communication peer has "
					"disconnected\n", ch->id);
				ch->sense_rc = 0x01;
			}
			fsm_event(ch->fsm, CTC_EVENT_UC_RCRESET, ch);
		} else {
			if (ch->sense_rc != SNS0_INTERVENTION_REQ) {
				pr_notice(
					"%s: The remote operating system is "
					"not available\n", ch->id);
				ch->sense_rc = SNS0_INTERVENTION_REQ;
			}
			fsm_event(ch->fsm, CTC_EVENT_UC_RSRESET, ch);
		}
	} else if (sense & SNS0_EQUIPMENT_CHECK) {
		if (sense & SNS0_BUS_OUT_CHECK) {
			if (ch->sense_rc != SNS0_BUS_OUT_CHECK) {
				CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
					"%s(%s): remote HW error %02x",
						CTCM_FUNTAIL, ch->id, sense);
				ch->sense_rc = SNS0_BUS_OUT_CHECK;
			}
			fsm_event(ch->fsm, CTC_EVENT_UC_HWFAIL, ch);
		} else {
			if (ch->sense_rc != SNS0_EQUIPMENT_CHECK) {
				CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
					"%s(%s): remote read parity error %02x",
						CTCM_FUNTAIL, ch->id, sense);
				ch->sense_rc = SNS0_EQUIPMENT_CHECK;
			}
			fsm_event(ch->fsm, CTC_EVENT_UC_RXPARITY, ch);
		}
	} else if (sense & SNS0_BUS_OUT_CHECK) {
		if (ch->sense_rc != SNS0_BUS_OUT_CHECK) {
			CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
				"%s(%s): BUS OUT error %02x",
					CTCM_FUNTAIL, ch->id, sense);
			ch->sense_rc = SNS0_BUS_OUT_CHECK;
		}
		if (sense & 0x04)	/* data-streaming timeout */
			fsm_event(ch->fsm, CTC_EVENT_UC_TXTIMEOUT, ch);
		else			/* Data-transfer parity error */
			fsm_event(ch->fsm, CTC_EVENT_UC_TXPARITY, ch);
	} else if (sense & SNS0_CMD_REJECT) {
		if (ch->sense_rc != SNS0_CMD_REJECT) {
			CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
				"%s(%s): Command rejected",
						CTCM_FUNTAIL, ch->id);
			ch->sense_rc = SNS0_CMD_REJECT;
		}
	} else if (sense == 0) {
		CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
			"%s(%s): Unit check ZERO",
					CTCM_FUNTAIL, ch->id);
		fsm_event(ch->fsm, CTC_EVENT_UC_ZERO, ch);
	} else {
		CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
			"%s(%s): Unit check code %02x unknown",
					CTCM_FUNTAIL, ch->id, sense);
		fsm_event(ch->fsm, CTC_EVENT_UC_UNKNOWN, ch);
	}
}

int ctcm_ch_alloc_buffer(struct channel *ch)
{
	clear_normalized_cda(&ch->ccw[1]);
	ch->trans_skb = __dev_alloc_skb(ch->max_bufsize, GFP_ATOMIC | GFP_DMA);
	if (ch->trans_skb == NULL) {
		CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
			"%s(%s): %s trans_skb allocation error",
			CTCM_FUNTAIL, ch->id,
			(CHANNEL_DIRECTION(ch->flags) == READ) ? "RX" : "TX");
		return -ENOMEM;
	}

	ch->ccw[1].count = ch->max_bufsize;
	if (set_normalized_cda(&ch->ccw[1], ch->trans_skb->data)) {
		dev_kfree_skb(ch->trans_skb);
		ch->trans_skb = NULL;
		CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
			"%s(%s): %s set norm_cda failed",
			CTCM_FUNTAIL, ch->id,
			(CHANNEL_DIRECTION(ch->flags) == READ) ? "RX" : "TX");
		return -ENOMEM;
	}

	ch->ccw[1].count = 0;
	ch->trans_skb_data = ch->trans_skb->data;
	ch->flags &= ~CHANNEL_FLAGS_BUFSIZE_CHANGED;
	return 0;
}


int ctcm_open(struct net_device *dev)
{
	struct ctcm_priv *priv = dev->ml_priv;

	CTCMY_DBF_DEV_NAME(SETUP, dev, "");
	if (!IS_MPC(priv))
		fsm_event(priv->fsm,	DEV_EVENT_START, dev);
	return 0;
}

int ctcm_close(struct net_device *dev)
{
	struct ctcm_priv *priv = dev->ml_priv;

	CTCMY_DBF_DEV_NAME(SETUP, dev, "");
	if (!IS_MPC(priv))
		fsm_event(priv->fsm, DEV_EVENT_STOP, dev);
	return 0;
}


static int ctcm_transmit_skb(struct channel *ch, struct sk_buff *skb)
{
	unsigned long saveflags;
	struct ll_header header;
	int rc = 0;
	__u16 block_len;
	int ccw_idx;
	struct sk_buff *nskb;
	unsigned long hi;

	/* we need to acquire the lock for testing the state
	 * otherwise we can have an IRQ changing the state to
	 * TXIDLE after the test but before acquiring the lock.
	 */
	spin_lock_irqsave(&ch->collect_lock, saveflags);
	if (fsm_getstate(ch->fsm) != CTC_STATE_TXIDLE) {
		int l = skb->len + LL_HEADER_LENGTH;

		if (ch->collect_len + l > ch->max_bufsize - 2) {
			spin_unlock_irqrestore(&ch->collect_lock, saveflags);
			return -EBUSY;
		} else {
			atomic_inc(&skb->users);
			header.length = l;
			header.type = skb->protocol;
			header.unused = 0;
			memcpy(skb_push(skb, LL_HEADER_LENGTH), &header,
			       LL_HEADER_LENGTH);
			skb_queue_tail(&ch->collect_queue, skb);
			ch->collect_len += l;
		}
		spin_unlock_irqrestore(&ch->collect_lock, saveflags);
				goto done;
	}
	spin_unlock_irqrestore(&ch->collect_lock, saveflags);
	/*
	 * Protect skb against beeing free'd by upper
	 * layers.
	 */
	atomic_inc(&skb->users);
	ch->prof.txlen += skb->len;
	header.length = skb->len + LL_HEADER_LENGTH;
	header.type = skb->protocol;
	header.unused = 0;
	memcpy(skb_push(skb, LL_HEADER_LENGTH), &header, LL_HEADER_LENGTH);
	block_len = skb->len + 2;
	*((__u16 *)skb_push(skb, 2)) = block_len;

	/*
	 * IDAL support in CTCM is broken, so we have to
	 * care about skb's above 2G ourselves.
	 */
	hi = ((unsigned long)skb_tail_pointer(skb) + LL_HEADER_LENGTH) >> 31;
	if (hi) {
		nskb = alloc_skb(skb->len, GFP_ATOMIC | GFP_DMA);
		if (!nskb) {
			atomic_dec(&skb->users);
			skb_pull(skb, LL_HEADER_LENGTH + 2);
			ctcm_clear_busy(ch->netdev);
			return -ENOMEM;
		} else {
			memcpy(skb_put(nskb, skb->len), skb->data, skb->len);
			atomic_inc(&nskb->users);
			atomic_dec(&skb->users);
			dev_kfree_skb_irq(skb);
			skb = nskb;
		}
	}

	ch->ccw[4].count = block_len;
	if (set_normalized_cda(&ch->ccw[4], skb->data)) {
		/*
		 * idal allocation failed, try via copying to
		 * trans_skb. trans_skb usually has a pre-allocated
		 * idal.
		 */
		if (ctcm_checkalloc_buffer(ch)) {
			/*
			 * Remove our header. It gets added
			 * again on retransmit.
			 */
			atomic_dec(&skb->users);
			skb_pull(skb, LL_HEADER_LENGTH + 2);
			ctcm_clear_busy(ch->netdev);
			return -ENOMEM;
		}

		skb_reset_tail_pointer(ch->trans_skb);
		ch->trans_skb->len = 0;
		ch->ccw[1].count = skb->len;
		skb_copy_from_linear_data(skb,
				skb_put(ch->trans_skb, skb->len), skb->len);
		atomic_dec(&skb->users);
		dev_kfree_skb_irq(skb);
		ccw_idx = 0;
	} else {
		skb_queue_tail(&ch->io_queue, skb);
		ccw_idx = 3;
	}
	ch->retry = 0;
	fsm_newstate(ch->fsm, CTC_STATE_TX);
	fsm_addtimer(&ch->timer, CTCM_TIME_5_SEC, CTC_EVENT_TIMER, ch);
	spin_lock_irqsave(get_ccwdev_lock(ch->cdev), saveflags);
	ch->prof.send_stamp = current_kernel_time(); /* xtime */
	rc = ccw_device_start(ch->cdev, &ch->ccw[ccw_idx],
					(unsigned long)ch, 0xff, 0);
	spin_unlock_irqrestore(get_ccwdev_lock(ch->cdev), saveflags);
	if (ccw_idx == 3)
		ch->prof.doios_single++;
	if (rc != 0) {
		fsm_deltimer(&ch->timer);
		ctcm_ccw_check_rc(ch, rc, "single skb TX");
		if (ccw_idx == 3)
			skb_dequeue_tail(&ch->io_queue);
		/*
		 * Remove our header. It gets added
		 * again on retransmit.
		 */
		skb_pull(skb, LL_HEADER_LENGTH + 2);
	} else if (ccw_idx == 0) {
		struct net_device *dev = ch->netdev;
		struct ctcm_priv *priv = dev->ml_priv;
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += skb->len - LL_HEADER_LENGTH;
	}
done:
	ctcm_clear_busy(ch->netdev);
	return rc;
}

static void ctcmpc_send_sweep_req(struct channel *rch)
{
	struct net_device *dev = rch->netdev;
	struct ctcm_priv *priv;
	struct mpc_group *grp;
	struct th_sweep *header;
	struct sk_buff *sweep_skb;
	struct channel *ch;
	/* int rc = 0; */

	priv = dev->ml_priv;
	grp = priv->mpcg;
	ch = priv->channel[WRITE];

	/* sweep processing is not complete until response and request */
	/* has completed for all read channels in group		       */
	if (grp->in_sweep == 0) {
		grp->in_sweep = 1;
		grp->sweep_rsp_pend_num = grp->active_channels[READ];
		grp->sweep_req_pend_num = grp->active_channels[READ];
	}

	sweep_skb = __dev_alloc_skb(MPC_BUFSIZE_DEFAULT, GFP_ATOMIC|GFP_DMA);

	if (sweep_skb == NULL)	{
		/* rc = -ENOMEM; */
				goto nomem;
	}

	header = kmalloc(TH_SWEEP_LENGTH, gfp_type());

	if (!header) {
		dev_kfree_skb_any(sweep_skb);
		/* rc = -ENOMEM; */
				goto nomem;
	}

	header->th.th_seg	= 0x00 ;
	header->th.th_ch_flag	= TH_SWEEP_REQ;  /* 0x0f */
	header->th.th_blk_flag	= 0x00;
	header->th.th_is_xid	= 0x00;
	header->th.th_seq_num	= 0x00;
	header->sw.th_last_seq	= ch->th_seq_num;

	memcpy(skb_put(sweep_skb, TH_SWEEP_LENGTH), header, TH_SWEEP_LENGTH);

	kfree(header);

	dev->trans_start = jiffies;
	skb_queue_tail(&ch->sweep_queue, sweep_skb);

	fsm_addtimer(&ch->sweep_timer, 100, CTC_EVENT_RSWEEP_TIMER, ch);

	return;

nomem:
	grp->in_sweep = 0;
	ctcm_clear_busy(dev);
	fsm_event(grp->fsm, MPCG_EVENT_INOP, dev);

	return;
}

static int ctcmpc_transmit_skb(struct channel *ch, struct sk_buff *skb)
{
	struct pdu *p_header;
	struct net_device *dev = ch->netdev;
	struct ctcm_priv *priv = dev->ml_priv;
	struct mpc_group *grp = priv->mpcg;
	struct th_header *header;
	struct sk_buff *nskb;
	int rc = 0;
	int ccw_idx;
	unsigned long hi;
	unsigned long saveflags = 0;	/* avoids compiler warning */
	__u16 block_len;

	CTCM_PR_DEBUG("Enter %s: %s, cp=%i ch=0x%p id=%s state=%s\n",
			__func__, dev->name, smp_processor_id(), ch,
					ch->id, fsm_getstate_str(ch->fsm));

	if ((fsm_getstate(ch->fsm) != CTC_STATE_TXIDLE) || grp->in_sweep) {
		spin_lock_irqsave(&ch->collect_lock, saveflags);
		atomic_inc(&skb->users);
		p_header = kmalloc(PDU_HEADER_LENGTH, gfp_type());

		if (!p_header) {
			spin_unlock_irqrestore(&ch->collect_lock, saveflags);
				goto nomem_exit;
		}

		p_header->pdu_offset = skb->len;
		p_header->pdu_proto = 0x01;
		p_header->pdu_flag = 0x00;
		if (skb->protocol == ntohs(ETH_P_SNAP)) {
			p_header->pdu_flag |= PDU_FIRST | PDU_CNTL;
		} else {
			p_header->pdu_flag |= PDU_FIRST;
		}
		p_header->pdu_seq = 0;
		memcpy(skb_push(skb, PDU_HEADER_LENGTH), p_header,
		       PDU_HEADER_LENGTH);

		CTCM_PR_DEBUG("%s(%s): Put on collect_q - skb len: %04x \n"
				"pdu header and data for up to 32 bytes:\n",
				__func__, dev->name, skb->len);
		CTCM_D3_DUMP((char *)skb->data, min_t(int, 32, skb->len));

		skb_queue_tail(&ch->collect_queue, skb);
		ch->collect_len += skb->len;
		kfree(p_header);

		spin_unlock_irqrestore(&ch->collect_lock, saveflags);
			goto done;
	}

	/*
	 * Protect skb against beeing free'd by upper
	 * layers.
	 */
	atomic_inc(&skb->users);

	block_len = skb->len + TH_HEADER_LENGTH + PDU_HEADER_LENGTH;
	/*
	 * IDAL support in CTCM is broken, so we have to
	 * care about skb's above 2G ourselves.
	 */
	hi = ((unsigned long)skb->tail + TH_HEADER_LENGTH) >> 31;
	if (hi) {
		nskb = __dev_alloc_skb(skb->len, GFP_ATOMIC | GFP_DMA);
		if (!nskb) {
			goto nomem_exit;
		} else {
			memcpy(skb_put(nskb, skb->len), skb->data, skb->len);
			atomic_inc(&nskb->users);
			atomic_dec(&skb->users);
			dev_kfree_skb_irq(skb);
			skb = nskb;
		}
	}

	p_header = kmalloc(PDU_HEADER_LENGTH, gfp_type());

	if (!p_header)
		goto nomem_exit;

	p_header->pdu_offset = skb->len;
	p_header->pdu_proto = 0x01;
	p_header->pdu_flag = 0x00;
	p_header->pdu_seq = 0;
	if (skb->protocol == ntohs(ETH_P_SNAP)) {
		p_header->pdu_flag |= PDU_FIRST | PDU_CNTL;
	} else {
		p_header->pdu_flag |= PDU_FIRST;
	}
	memcpy(skb_push(skb, PDU_HEADER_LENGTH), p_header, PDU_HEADER_LENGTH);

	kfree(p_header);

	if (ch->collect_len > 0) {
		spin_lock_irqsave(&ch->collect_lock, saveflags);
		skb_queue_tail(&ch->collect_queue, skb);
		ch->collect_len += skb->len;
		skb = skb_dequeue(&ch->collect_queue);
		ch->collect_len -= skb->len;
		spin_unlock_irqrestore(&ch->collect_lock, saveflags);
	}

	p_header = (struct pdu *)skb->data;
	p_header->pdu_flag |= PDU_LAST;

	ch->prof.txlen += skb->len - PDU_HEADER_LENGTH;

	header = kmalloc(TH_HEADER_LENGTH, gfp_type());
	if (!header)
		goto nomem_exit;

	header->th_seg = 0x00;
	header->th_ch_flag = TH_HAS_PDU;  /* Normal data */
	header->th_blk_flag = 0x00;
	header->th_is_xid = 0x00;          /* Just data here */
	ch->th_seq_num++;
	header->th_seq_num = ch->th_seq_num;

	CTCM_PR_DBGDATA("%s(%s) ToVTAM_th_seq= %08x\n" ,
		       __func__, dev->name, ch->th_seq_num);

	/* put the TH on the packet */
	memcpy(skb_push(skb, TH_HEADER_LENGTH), header, TH_HEADER_LENGTH);

	kfree(header);

	CTCM_PR_DBGDATA("%s(%s): skb len: %04x\n - pdu header and data for "
			"up to 32 bytes sent to vtam:\n",
				__func__, dev->name, skb->len);
	CTCM_D3_DUMP((char *)skb->data, min_t(int, 32, skb->len));

	ch->ccw[4].count = skb->len;
	if (set_normalized_cda(&ch->ccw[4], skb->data)) {
		/*
		 * idal allocation failed, try via copying to trans_skb.
		 * trans_skb usually has a pre-allocated idal.
		 */
		if (ctcm_checkalloc_buffer(ch)) {
			/*
			 * Remove our header.
			 * It gets added again on retransmit.
			 */
				goto nomem_exit;
		}

		skb_reset_tail_pointer(ch->trans_skb);
		ch->trans_skb->len = 0;
		ch->ccw[1].count = skb->len;
		memcpy(skb_put(ch->trans_skb, skb->len), skb->data, skb->len);
		atomic_dec(&skb->users);
		dev_kfree_skb_irq(skb);
		ccw_idx = 0;
		CTCM_PR_DBGDATA("%s(%s): trans_skb len: %04x\n"
				"up to 32 bytes sent to vtam:\n",
				__func__, dev->name, ch->trans_skb->len);
		CTCM_D3_DUMP((char *)ch->trans_skb->data,
				min_t(int, 32, ch->trans_skb->len));
	} else {
		skb_queue_tail(&ch->io_queue, skb);
		ccw_idx = 3;
	}
	ch->retry = 0;
	fsm_newstate(ch->fsm, CTC_STATE_TX);
	fsm_addtimer(&ch->timer, CTCM_TIME_5_SEC, CTC_EVENT_TIMER, ch);

	if (do_debug_ccw)
		ctcmpc_dumpit((char *)&ch->ccw[ccw_idx],
					sizeof(struct ccw1) * 3);

	spin_lock_irqsave(get_ccwdev_lock(ch->cdev), saveflags);
	ch->prof.send_stamp = current_kernel_time(); /* xtime */
	rc = ccw_device_start(ch->cdev, &ch->ccw[ccw_idx],
					(unsigned long)ch, 0xff, 0);
	spin_unlock_irqrestore(get_ccwdev_lock(ch->cdev), saveflags);
	if (ccw_idx == 3)
		ch->prof.doios_single++;
	if (rc != 0) {
		fsm_deltimer(&ch->timer);
		ctcm_ccw_check_rc(ch, rc, "single skb TX");
		if (ccw_idx == 3)
			skb_dequeue_tail(&ch->io_queue);
	} else if (ccw_idx == 0) {
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += skb->len - TH_HEADER_LENGTH;
	}
	if (ch->th_seq_num > 0xf0000000)	/* Chose at random. */
		ctcmpc_send_sweep_req(ch);

	goto done;
nomem_exit:
	CTCM_DBF_TEXT_(MPC_ERROR, CTC_DBF_CRIT,
			"%s(%s): MEMORY allocation ERROR\n",
			CTCM_FUNTAIL, ch->id);
	rc = -ENOMEM;
	atomic_dec(&skb->users);
	dev_kfree_skb_any(skb);
	fsm_event(priv->mpcg->fsm, MPCG_EVENT_INOP, dev);
done:
	CTCM_PR_DEBUG("Exit %s(%s)\n", __func__, dev->name);
	return rc;
}

/* first merge version - leaving both functions separated */
static int ctcm_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct ctcm_priv *priv = dev->ml_priv;

	if (skb == NULL) {
		CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
				"%s(%s): NULL sk_buff passed",
					CTCM_FUNTAIL, dev->name);
		priv->stats.tx_dropped++;
		return 0;
	}
	if (skb_headroom(skb) < (LL_HEADER_LENGTH + 2)) {
		CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
			"%s(%s): Got sk_buff with head room < %ld bytes",
			CTCM_FUNTAIL, dev->name, LL_HEADER_LENGTH + 2);
		dev_kfree_skb(skb);
		priv->stats.tx_dropped++;
		return 0;
	}

	/*
	 * If channels are not running, try to restart them
	 * and throw away packet.
	 */
	if (fsm_getstate(priv->fsm) != DEV_STATE_RUNNING) {
		fsm_event(priv->fsm, DEV_EVENT_START, dev);
		dev_kfree_skb(skb);
		priv->stats.tx_dropped++;
		priv->stats.tx_errors++;
		priv->stats.tx_carrier_errors++;
		return 0;
	}

	if (ctcm_test_and_set_busy(dev))
		return -EBUSY;

	dev->trans_start = jiffies;
	if (ctcm_transmit_skb(priv->channel[WRITE], skb) != 0)
		return 1;
	return 0;
}

/* unmerged MPC variant of ctcm_tx */
static int ctcmpc_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len = 0;
	struct ctcm_priv *priv = dev->ml_priv;
	struct mpc_group *grp  = priv->mpcg;
	struct sk_buff *newskb = NULL;

	/*
	 * Some sanity checks ...
	 */
	if (skb == NULL) {
		CTCM_DBF_TEXT_(MPC_ERROR, CTC_DBF_ERROR,
			"%s(%s): NULL sk_buff passed",
					CTCM_FUNTAIL, dev->name);
		priv->stats.tx_dropped++;
					goto done;
	}
	if (skb_headroom(skb) < (TH_HEADER_LENGTH + PDU_HEADER_LENGTH)) {
		CTCM_DBF_TEXT_(MPC_TRACE, CTC_DBF_ERROR,
			"%s(%s): Got sk_buff with head room < %ld bytes",
			CTCM_FUNTAIL, dev->name,
				TH_HEADER_LENGTH + PDU_HEADER_LENGTH);

		CTCM_D3_DUMP((char *)skb->data, min_t(int, 32, skb->len));

		len =  skb->len + TH_HEADER_LENGTH + PDU_HEADER_LENGTH;
		newskb = __dev_alloc_skb(len, gfp_type() | GFP_DMA);

		if (!newskb) {
			CTCM_DBF_TEXT_(MPC_TRACE, CTC_DBF_ERROR,
				"%s: %s: __dev_alloc_skb failed",
						__func__, dev->name);

			dev_kfree_skb_any(skb);
			priv->stats.tx_dropped++;
			priv->stats.tx_errors++;
			priv->stats.tx_carrier_errors++;
			fsm_event(grp->fsm, MPCG_EVENT_INOP, dev);
					goto done;
		}
		newskb->protocol = skb->protocol;
		skb_reserve(newskb, TH_HEADER_LENGTH + PDU_HEADER_LENGTH);
		memcpy(skb_put(newskb, skb->len), skb->data, skb->len);
		dev_kfree_skb_any(skb);
		skb = newskb;
	}

	/*
	 * If channels are not running,
	 * notify anybody about a link failure and throw
	 * away packet.
	 */
	if ((fsm_getstate(priv->fsm) != DEV_STATE_RUNNING) ||
	   (fsm_getstate(grp->fsm) <  MPCG_STATE_XID2INITW)) {
		dev_kfree_skb_any(skb);
		CTCM_DBF_TEXT_(MPC_ERROR, CTC_DBF_ERROR,
			"%s(%s): inactive MPCGROUP - dropped",
					CTCM_FUNTAIL, dev->name);
		priv->stats.tx_dropped++;
		priv->stats.tx_errors++;
		priv->stats.tx_carrier_errors++;
					goto done;
	}

	if (ctcm_test_and_set_busy(dev)) {
		CTCM_DBF_TEXT_(MPC_ERROR, CTC_DBF_ERROR,
			"%s(%s): device busy - dropped",
					CTCM_FUNTAIL, dev->name);
		dev_kfree_skb_any(skb);
		priv->stats.tx_dropped++;
		priv->stats.tx_errors++;
		priv->stats.tx_carrier_errors++;
		fsm_event(grp->fsm, MPCG_EVENT_INOP, dev);
					goto done;
	}

	dev->trans_start = jiffies;
	if (ctcmpc_transmit_skb(priv->channel[WRITE], skb) != 0) {
		CTCM_DBF_TEXT_(MPC_ERROR, CTC_DBF_ERROR,
			"%s(%s): device error - dropped",
					CTCM_FUNTAIL, dev->name);
		dev_kfree_skb_any(skb);
		priv->stats.tx_dropped++;
		priv->stats.tx_errors++;
		priv->stats.tx_carrier_errors++;
		ctcm_clear_busy(dev);
		fsm_event(grp->fsm, MPCG_EVENT_INOP, dev);
					goto done;
	}
	ctcm_clear_busy(dev);
done:
	if (do_debug)
		MPC_DBF_DEV_NAME(TRACE, dev, "exit");

	return 0;	/* handle freeing of skb here */
}


static int ctcm_change_mtu(struct net_device *dev, int new_mtu)
{
	struct ctcm_priv *priv;
	int max_bufsize;

	if (new_mtu < 576 || new_mtu > 65527)
		return -EINVAL;

	priv = dev->ml_priv;
	max_bufsize = priv->channel[READ]->max_bufsize;

	if (IS_MPC(priv)) {
		if (new_mtu > max_bufsize - TH_HEADER_LENGTH)
			return -EINVAL;
		dev->hard_header_len = TH_HEADER_LENGTH + PDU_HEADER_LENGTH;
	} else {
		if (new_mtu > max_bufsize - LL_HEADER_LENGTH - 2)
			return -EINVAL;
		dev->hard_header_len = LL_HEADER_LENGTH + 2;
	}
	dev->mtu = new_mtu;
	return 0;
}

static struct net_device_stats *ctcm_stats(struct net_device *dev)
{
	return &((struct ctcm_priv *)dev->ml_priv)->stats;
}

static void ctcm_free_netdevice(struct net_device *dev)
{
	struct ctcm_priv *priv;
	struct mpc_group *grp;

	CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO,
			"%s(%s)", CTCM_FUNTAIL, dev->name);
	priv = dev->ml_priv;
	if (priv) {
		grp = priv->mpcg;
		if (grp) {
			if (grp->fsm)
				kfree_fsm(grp->fsm);
			if (grp->xid_skb)
				dev_kfree_skb(grp->xid_skb);
			if (grp->rcvd_xid_skb)
				dev_kfree_skb(grp->rcvd_xid_skb);
			tasklet_kill(&grp->mpc_tasklet2);
			kfree(grp);
			priv->mpcg = NULL;
		}
		if (priv->fsm) {
			kfree_fsm(priv->fsm);
			priv->fsm = NULL;
		}
		kfree(priv->xid);
		priv->xid = NULL;
	/*
	 * Note: kfree(priv); is done in "opposite" function of
	 * allocator function probe_device which is remove_device.
	 */
	}
#ifdef MODULE
	free_netdev(dev);
#endif
}

struct mpc_group *ctcmpc_init_mpc_group(struct ctcm_priv *priv);

void static ctcm_dev_setup(struct net_device *dev)
{
	dev->open = ctcm_open;
	dev->stop = ctcm_close;
	dev->get_stats = ctcm_stats;
	dev->change_mtu = ctcm_change_mtu;
	dev->type = ARPHRD_SLIP;
	dev->tx_queue_len = 100;
	dev->flags = IFF_POINTOPOINT | IFF_NOARP;
}

static struct net_device *ctcm_init_netdevice(struct ctcm_priv *priv)
{
	struct net_device *dev;
	struct mpc_group *grp;
	if (!priv)
		return NULL;

	if (IS_MPC(priv))
		dev = alloc_netdev(0, MPC_DEVICE_GENE, ctcm_dev_setup);
	else
		dev = alloc_netdev(0, CTC_DEVICE_GENE, ctcm_dev_setup);

	if (!dev) {
		CTCM_DBF_TEXT_(ERROR, CTC_DBF_CRIT,
			"%s: MEMORY allocation ERROR",
			CTCM_FUNTAIL);
		return NULL;
	}
	dev->ml_priv = priv;
	priv->fsm = init_fsm("ctcmdev", dev_state_names, dev_event_names,
				CTCM_NR_DEV_STATES, CTCM_NR_DEV_EVENTS,
				dev_fsm, dev_fsm_len, GFP_KERNEL);
	if (priv->fsm == NULL) {
		CTCMY_DBF_DEV(SETUP, dev, "init_fsm error");
		kfree(dev);
		return NULL;
	}
	fsm_newstate(priv->fsm, DEV_STATE_STOPPED);
	fsm_settimer(priv->fsm, &priv->restart_timer);

	if (IS_MPC(priv)) {
		/*  MPC Group Initializations  */
		grp = ctcmpc_init_mpc_group(priv);
		if (grp == NULL) {
			MPC_DBF_DEV(SETUP, dev, "init_mpc_group error");
			kfree(dev);
			return NULL;
		}
		tasklet_init(&grp->mpc_tasklet2,
				mpc_group_ready, (unsigned long)dev);
		dev->mtu = MPC_BUFSIZE_DEFAULT -
				TH_HEADER_LENGTH - PDU_HEADER_LENGTH;

		dev->hard_start_xmit = ctcmpc_tx;
		dev->hard_header_len = TH_HEADER_LENGTH + PDU_HEADER_LENGTH;
		priv->buffer_size = MPC_BUFSIZE_DEFAULT;
	} else {
		dev->mtu = CTCM_BUFSIZE_DEFAULT - LL_HEADER_LENGTH - 2;
		dev->hard_start_xmit = ctcm_tx;
		dev->hard_header_len = LL_HEADER_LENGTH + 2;
	}

	CTCMY_DBF_DEV(SETUP, dev, "finished");

	return dev;
}

static void ctcm_irq_handler(struct ccw_device *cdev,
				unsigned long intparm, struct irb *irb)
{
	struct channel		*ch;
	struct net_device	*dev;
	struct ctcm_priv	*priv;
	struct ccwgroup_device	*cgdev;
	int cstat;
	int dstat;

	CTCM_DBF_TEXT_(TRACE, CTC_DBF_DEBUG,
		"Enter %s(%s)", CTCM_FUNTAIL, dev_name(&cdev->dev));

	if (ctcm_check_irb_error(cdev, irb))
		return;

	cgdev = dev_get_drvdata(&cdev->dev);

	cstat = irb->scsw.cmd.cstat;
	dstat = irb->scsw.cmd.dstat;

	/* Check for unsolicited interrupts. */
	if (cgdev == NULL) {
		CTCM_DBF_TEXT_(TRACE, CTC_DBF_ERROR,
			"%s(%s) unsolicited irq: c-%02x d-%02x\n",
			CTCM_FUNTAIL, dev_name(&cdev->dev), cstat, dstat);
		dev_warn(&cdev->dev,
			"The adapter received a non-specific IRQ\n");
		return;
	}

	priv = dev_get_drvdata(&cgdev->dev);

	/* Try to extract channel from driver data. */
	if (priv->channel[READ]->cdev == cdev)
		ch = priv->channel[READ];
	else if (priv->channel[WRITE]->cdev == cdev)
		ch = priv->channel[WRITE];
	else {
		dev_err(&cdev->dev,
			"%s: Internal error: Can't determine channel for "
			"interrupt device %s\n",
			__func__, dev_name(&cdev->dev));
			/* Explain: inconsistent internal structures */
		return;
	}

	dev = ch->netdev;
	if (dev == NULL) {
		dev_err(&cdev->dev,
			"%s Internal error: net_device is NULL, ch = 0x%p\n",
			__func__, ch);
			/* Explain: inconsistent internal structures */
		return;
	}

	/* Copy interruption response block. */
	memcpy(ch->irb, irb, sizeof(struct irb));

	/* Issue error message and return on subchannel error code */
	if (irb->scsw.cmd.cstat) {
		fsm_event(ch->fsm, CTC_EVENT_SC_UNKNOWN, ch);
		CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
			"%s(%s): sub-ch check %s: cs=%02x ds=%02x",
				CTCM_FUNTAIL, dev->name, ch->id, cstat, dstat);
		dev_warn(&cdev->dev,
				"A check occurred on the subchannel\n");
		return;
	}

	/* Check the reason-code of a unit check */
	if (irb->scsw.cmd.dstat & DEV_STAT_UNIT_CHECK) {
		if ((irb->ecw[0] & ch->sense_rc) == 0)
			/* print it only once */
			CTCM_DBF_TEXT_(TRACE, CTC_DBF_WARN,
				"%s(%s): sense=%02x, ds=%02x",
				CTCM_FUNTAIL, ch->id, irb->ecw[0], dstat);
		ccw_unit_check(ch, irb->ecw[0]);
		return;
	}
	if (irb->scsw.cmd.dstat & DEV_STAT_BUSY) {
		if (irb->scsw.cmd.dstat & DEV_STAT_ATTENTION)
			fsm_event(ch->fsm, CTC_EVENT_ATTNBUSY, ch);
		else
			fsm_event(ch->fsm, CTC_EVENT_BUSY, ch);
		return;
	}
	if (irb->scsw.cmd.dstat & DEV_STAT_ATTENTION) {
		fsm_event(ch->fsm, CTC_EVENT_ATTN, ch);
		return;
	}
	if ((irb->scsw.cmd.stctl & SCSW_STCTL_SEC_STATUS) ||
	    (irb->scsw.cmd.stctl == SCSW_STCTL_STATUS_PEND) ||
	    (irb->scsw.cmd.stctl ==
	     (SCSW_STCTL_ALERT_STATUS | SCSW_STCTL_STATUS_PEND)))
		fsm_event(ch->fsm, CTC_EVENT_FINSTAT, ch);
	else
		fsm_event(ch->fsm, CTC_EVENT_IRQ, ch);

}

static int ctcm_probe_device(struct ccwgroup_device *cgdev)
{
	struct ctcm_priv *priv;
	int rc;

	CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO,
			"%s %p",
			__func__, cgdev);

	if (!get_device(&cgdev->dev))
		return -ENODEV;

	priv = kzalloc(sizeof(struct ctcm_priv), GFP_KERNEL);
	if (!priv) {
		CTCM_DBF_TEXT_(ERROR, CTC_DBF_ERROR,
			"%s: memory allocation failure",
			CTCM_FUNTAIL);
		put_device(&cgdev->dev);
		return -ENOMEM;
	}

	rc = ctcm_add_files(&cgdev->dev);
	if (rc) {
		kfree(priv);
		put_device(&cgdev->dev);
		return rc;
	}
	priv->buffer_size = CTCM_BUFSIZE_DEFAULT;
	cgdev->cdev[0]->handler = ctcm_irq_handler;
	cgdev->cdev[1]->handler = ctcm_irq_handler;
	dev_set_drvdata(&cgdev->dev, priv);

	return 0;
}

static int add_channel(struct ccw_device *cdev, enum channel_types type,
				struct ctcm_priv *priv)
{
	struct channel **c = &channels;
	struct channel *ch;
	int ccw_num;
	int rc = 0;

	CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO,
		"%s(%s), type %d, proto %d",
			__func__, dev_name(&cdev->dev),	type, priv->protocol);

	ch = kzalloc(sizeof(struct channel), GFP_KERNEL);
	if (ch == NULL)
		return -ENOMEM;

	ch->protocol = priv->protocol;
	if (IS_MPC(priv)) {
		ch->discontact_th = (struct th_header *)
				kzalloc(TH_HEADER_LENGTH, gfp_type());
		if (ch->discontact_th == NULL)
					goto nomem_return;

		ch->discontact_th->th_blk_flag = TH_DISCONTACT;
		tasklet_init(&ch->ch_disc_tasklet,
			mpc_action_send_discontact, (unsigned long)ch);

		tasklet_init(&ch->ch_tasklet, ctcmpc_bh, (unsigned long)ch);
		ch->max_bufsize = (MPC_BUFSIZE_DEFAULT - 35);
		ccw_num = 17;
	} else
		ccw_num = 8;

	ch->ccw = (struct ccw1 *)
		kzalloc(ccw_num * sizeof(struct ccw1), GFP_KERNEL | GFP_DMA);
	if (ch->ccw == NULL)
					goto nomem_return;

	ch->cdev = cdev;
	snprintf(ch->id, CTCM_ID_SIZE, "ch-%s", dev_name(&cdev->dev));
	ch->type = type;

	/**
	 * "static" ccws are used in the following way:
	 *
	 * ccw[0..2] (Channel program for generic I/O):
	 *           0: prepare
	 *           1: read or write (depending on direction) with fixed
	 *              buffer (idal allocated once when buffer is allocated)
	 *           2: nop
	 * ccw[3..5] (Channel program for direct write of packets)
	 *           3: prepare
	 *           4: write (idal allocated on every write).
	 *           5: nop
	 * ccw[6..7] (Channel program for initial channel setup):
	 *           6: set extended mode
	 *           7: nop
	 *
	 * ch->ccw[0..5] are initialized in ch_action_start because
	 * the channel's direction is yet unknown here.
	 *
	 * ccws used for xid2 negotiations
	 *  ch-ccw[8-14] need to be used for the XID exchange either
	 *    X side XID2 Processing
	 *       8:  write control
	 *       9:  write th
	 *	     10: write XID
	 *	     11: read th from secondary
	 *	     12: read XID   from secondary
	 *	     13: read 4 byte ID
	 *	     14: nop
	 *    Y side XID Processing
	 *	     8:  sense
	 *       9:  read th
	 *	     10: read XID
	 *	     11: write th
	 *	     12: write XID
	 *	     13: write 4 byte ID
	 *	     14: nop
	 *
	 *  ccws used for double noop due to VM timing issues
	 *  which result in unrecoverable Busy on channel
	 *       15: nop
	 *       16: nop
	 */
	ch->ccw[6].cmd_code	= CCW_CMD_SET_EXTENDED;
	ch->ccw[6].flags	= CCW_FLAG_SLI;

	ch->ccw[7].cmd_code	= CCW_CMD_NOOP;
	ch->ccw[7].flags	= CCW_FLAG_SLI;

	if (IS_MPC(priv)) {
		ch->ccw[15].cmd_code = CCW_CMD_WRITE;
		ch->ccw[15].flags    = CCW_FLAG_SLI | CCW_FLAG_CC;
		ch->ccw[15].count    = TH_HEADER_LENGTH;
		ch->ccw[15].cda      = virt_to_phys(ch->discontact_th);

		ch->ccw[16].cmd_code = CCW_CMD_NOOP;
		ch->ccw[16].flags    = CCW_FLAG_SLI;

		ch->fsm = init_fsm(ch->id, ctc_ch_state_names,
				ctc_ch_event_names, CTC_MPC_NR_STATES,
				CTC_MPC_NR_EVENTS, ctcmpc_ch_fsm,
				mpc_ch_fsm_len, GFP_KERNEL);
	} else {
		ch->fsm = init_fsm(ch->id, ctc_ch_state_names,
				ctc_ch_event_names, CTC_NR_STATES,
				CTC_NR_EVENTS, ch_fsm,
				ch_fsm_len, GFP_KERNEL);
	}
	if (ch->fsm == NULL)
				goto free_return;

	fsm_newstate(ch->fsm, CTC_STATE_IDLE);

	ch->irb = kzalloc(sizeof(struct irb), GFP_KERNEL);
	if (ch->irb == NULL)
				goto nomem_return;

	while (*c && ctcm_less_than((*c)->id, ch->id))
		c = &(*c)->next;

	if (*c && (!strncmp((*c)->id, ch->id, CTCM_ID_SIZE))) {
		CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO,
				"%s (%s) already in list, using old entry",
				__func__, (*c)->id);

				goto free_return;
	}

	spin_lock_init(&ch->collect_lock);

	fsm_settimer(ch->fsm, &ch->timer);
	skb_queue_head_init(&ch->io_queue);
	skb_queue_head_init(&ch->collect_queue);

	if (IS_MPC(priv)) {
		fsm_settimer(ch->fsm, &ch->sweep_timer);
		skb_queue_head_init(&ch->sweep_queue);
	}
	ch->next = *c;
	*c = ch;
	return 0;

nomem_return:
	rc = -ENOMEM;

free_return:	/* note that all channel pointers are 0 or valid */
	kfree(ch->ccw);
	kfree(ch->discontact_th);
	kfree_fsm(ch->fsm);
	kfree(ch->irb);
	kfree(ch);
	return rc;
}

static enum channel_types get_channel_type(struct ccw_device_id *id)
{
	enum channel_types type;
	type = (enum channel_types)id->driver_info;

	if (type == channel_type_ficon)
		type = channel_type_escon;

	return type;
}

static int ctcm_new_device(struct ccwgroup_device *cgdev)
{
	char read_id[CTCM_ID_SIZE];
	char write_id[CTCM_ID_SIZE];
	int direction;
	enum channel_types type;
	struct ctcm_priv *priv;
	struct net_device *dev;
	struct ccw_device *cdev0;
	struct ccw_device *cdev1;
	int ret;

	priv = dev_get_drvdata(&cgdev->dev);
	if (!priv)
		return -ENODEV;

	cdev0 = cgdev->cdev[0];
	cdev1 = cgdev->cdev[1];

	type = get_channel_type(&cdev0->id);

	snprintf(read_id, CTCM_ID_SIZE, "ch-%s", dev_name(&cdev0->dev));
	snprintf(write_id, CTCM_ID_SIZE, "ch-%s", dev_name(&cdev1->dev));

	ret = add_channel(cdev0, type, priv);
	if (ret)
		return ret;
	ret = add_channel(cdev1, type, priv);
	if (ret)
		return ret;

	ret = ccw_device_set_online(cdev0);
	if (ret != 0) {
		/* may be ok to fail now - can be done later */
		CTCM_DBF_TEXT_(TRACE, CTC_DBF_NOTICE,
			"%s(%s) set_online rc=%d",
				CTCM_FUNTAIL, read_id, ret);
	}

	ret = ccw_device_set_online(cdev1);
	if (ret != 0) {
		/* may be ok to fail now - can be done later */
		CTCM_DBF_TEXT_(TRACE, CTC_DBF_NOTICE,
			"%s(%s) set_online rc=%d",
				CTCM_FUNTAIL, write_id, ret);
	}

	dev = ctcm_init_netdevice(priv);
	if (dev == NULL)
			goto out;

	for (direction = READ; direction <= WRITE; direction++) {
		priv->channel[direction] =
		    channel_get(type, direction == READ ? read_id : write_id,
				direction);
		if (priv->channel[direction] == NULL) {
			if (direction == WRITE)
				channel_free(priv->channel[READ]);
			goto out_dev;
		}
		priv->channel[direction]->netdev = dev;
		priv->channel[direction]->protocol = priv->protocol;
		priv->channel[direction]->max_bufsize = priv->buffer_size;
	}
	/* sysfs magic */
	SET_NETDEV_DEV(dev, &cgdev->dev);

	if (register_netdev(dev))
			goto out_dev;

	if (ctcm_add_attributes(&cgdev->dev)) {
		unregister_netdev(dev);
			goto out_dev;
	}

	strlcpy(priv->fsm->name, dev->name, sizeof(priv->fsm->name));

	dev_info(&dev->dev,
		"setup OK : r/w = %s/%s, protocol : %d\n",
			priv->channel[READ]->id,
			priv->channel[WRITE]->id, priv->protocol);

	CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO,
		"setup(%s) OK : r/w = %s/%s, protocol : %d", dev->name,
			priv->channel[READ]->id,
			priv->channel[WRITE]->id, priv->protocol);

	return 0;
out_dev:
	ctcm_free_netdevice(dev);
out:
	ccw_device_set_offline(cgdev->cdev[1]);
	ccw_device_set_offline(cgdev->cdev[0]);

	return -ENODEV;
}

static int ctcm_shutdown_device(struct ccwgroup_device *cgdev)
{
	struct ctcm_priv *priv;
	struct net_device *dev;

	priv = dev_get_drvdata(&cgdev->dev);
	if (!priv)
		return -ENODEV;

	if (priv->channel[READ]) {
		dev = priv->channel[READ]->netdev;
		CTCM_DBF_DEV(SETUP, dev, "");
		/* Close the device */
		ctcm_close(dev);
		dev->flags &= ~IFF_RUNNING;
		ctcm_remove_attributes(&cgdev->dev);
		channel_free(priv->channel[READ]);
	} else
		dev = NULL;

	if (priv->channel[WRITE])
		channel_free(priv->channel[WRITE]);

	if (dev) {
		unregister_netdev(dev);
		ctcm_free_netdevice(dev);
	}

	if (priv->fsm)
		kfree_fsm(priv->fsm);

	ccw_device_set_offline(cgdev->cdev[1]);
	ccw_device_set_offline(cgdev->cdev[0]);

	if (priv->channel[READ])
		channel_remove(priv->channel[READ]);
	if (priv->channel[WRITE])
		channel_remove(priv->channel[WRITE]);
	priv->channel[READ] = priv->channel[WRITE] = NULL;

	return 0;

}


static void ctcm_remove_device(struct ccwgroup_device *cgdev)
{
	struct ctcm_priv *priv = dev_get_drvdata(&cgdev->dev);

	BUG_ON(priv == NULL);

	CTCM_DBF_TEXT_(SETUP, CTC_DBF_INFO,
			"removing device %s, r/w = %s/%s, proto : %d",
			priv->channel[READ]->netdev->name,
			priv->channel[READ]->id, priv->channel[WRITE]->id,
			priv->protocol);

	if (cgdev->state == CCWGROUP_ONLINE)
		ctcm_shutdown_device(cgdev);
	ctcm_remove_files(&cgdev->dev);
	dev_set_drvdata(&cgdev->dev, NULL);
	kfree(priv);
	put_device(&cgdev->dev);
}

static struct ccwgroup_driver ctcm_group_driver = {
	.owner       = THIS_MODULE,
	.name        = CTC_DRIVER_NAME,
	.max_slaves  = 2,
	.driver_id   = 0xC3E3C3D4,	/* CTCM */
	.probe       = ctcm_probe_device,
	.remove      = ctcm_remove_device,
	.set_online  = ctcm_new_device,
	.set_offline = ctcm_shutdown_device,
};



static void __exit ctcm_exit(void)
{
	unregister_cu3088_discipline(&ctcm_group_driver);
	ctcm_unregister_dbf_views();
	pr_info("CTCM driver unloaded\n");
}

static void print_banner(void)
{
	pr_info("CTCM driver initialized\n");
}

static int __init ctcm_init(void)
{
	int ret;

	channels = NULL;

	ret = ctcm_register_dbf_views();
	if (ret) {
		return ret;
	}
	ret = register_cu3088_discipline(&ctcm_group_driver);
	if (ret) {
		ctcm_unregister_dbf_views();
		pr_err("%s / register_cu3088_discipline failed, ret = %d\n",
			__func__, ret);
		return ret;
	}
	print_banner();
	return ret;
}

module_init(ctcm_init);
module_exit(ctcm_exit);

MODULE_AUTHOR("Peter Tiedemann <ptiedem@de.ibm.com>");
MODULE_DESCRIPTION("Network driver for S/390 CTC + CTCMPC (SNA)");
MODULE_LICENSE("GPL");
