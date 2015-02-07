

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/sctp/sctp.h>
#include <net/sctp/sm.h>


/* Initialize datamsg from memory. */
static void sctp_datamsg_init(struct sctp_datamsg *msg)
{
	atomic_set(&msg->refcnt, 1);
	msg->send_failed = 0;
	msg->send_error = 0;
	msg->can_abandon = 0;
	msg->expires_at = 0;
	INIT_LIST_HEAD(&msg->chunks);
}

/* Allocate and initialize datamsg. */
SCTP_STATIC struct sctp_datamsg *sctp_datamsg_new(gfp_t gfp)
{
	struct sctp_datamsg *msg;
	msg = kmalloc(sizeof(struct sctp_datamsg), gfp);
	if (msg) {
		sctp_datamsg_init(msg);
		SCTP_DBG_OBJCNT_INC(datamsg);
	}
	return msg;
}

/* Final destructruction of datamsg memory. */
static void sctp_datamsg_destroy(struct sctp_datamsg *msg)
{
	struct list_head *pos, *temp;
	struct sctp_chunk *chunk;
	struct sctp_sock *sp;
	struct sctp_ulpevent *ev;
	struct sctp_association *asoc = NULL;
	int error = 0, notify;

	/* If we failed, we may need to notify. */
	notify = msg->send_failed ? -1 : 0;

	/* Release all references. */
	list_for_each_safe(pos, temp, &msg->chunks) {
		list_del_init(pos);
		chunk = list_entry(pos, struct sctp_chunk, frag_list);
		/* Check whether we _really_ need to notify. */
		if (notify < 0) {
			asoc = chunk->asoc;
			if (msg->send_error)
				error = msg->send_error;
			else
				error = asoc->outqueue.error;

			sp = sctp_sk(asoc->base.sk);
			notify = sctp_ulpevent_type_enabled(SCTP_SEND_FAILED,
							    &sp->subscribe);
		}

		/* Generate a SEND FAILED event only if enabled. */
		if (notify > 0) {
			int sent;
			if (chunk->has_tsn)
				sent = SCTP_DATA_SENT;
			else
				sent = SCTP_DATA_UNSENT;

			ev = sctp_ulpevent_make_send_failed(asoc, chunk, sent,
							    error, GFP_ATOMIC);
			if (ev)
				sctp_ulpq_tail_event(&asoc->ulpq, ev);
		}

		sctp_chunk_put(chunk);
	}

	SCTP_DBG_OBJCNT_DEC(datamsg);
	kfree(msg);
}

/* Hold a reference. */
static void sctp_datamsg_hold(struct sctp_datamsg *msg)
{
	atomic_inc(&msg->refcnt);
}

/* Release a reference. */
void sctp_datamsg_put(struct sctp_datamsg *msg)
{
	if (atomic_dec_and_test(&msg->refcnt))
		sctp_datamsg_destroy(msg);
}

/* Assign a chunk to this datamsg. */
static void sctp_datamsg_assign(struct sctp_datamsg *msg, struct sctp_chunk *chunk)
{
	sctp_datamsg_hold(msg);
	chunk->msg = msg;
}


struct sctp_datamsg *sctp_datamsg_from_user(struct sctp_association *asoc,
					    struct sctp_sndrcvinfo *sinfo,
					    struct msghdr *msgh, int msg_len)
{
	int max, whole, i, offset, over, err;
	int len, first_len;
	struct sctp_chunk *chunk;
	struct sctp_datamsg *msg;
	struct list_head *pos, *temp;
	__u8 frag;

	msg = sctp_datamsg_new(GFP_KERNEL);
	if (!msg)
		return NULL;

	/* Note: Calculate this outside of the loop, so that all fragments
	 * have the same expiration.
	 */
	if (sinfo->sinfo_timetolive) {
		/* sinfo_timetolive is in milliseconds */
		msg->expires_at = jiffies +
				    msecs_to_jiffies(sinfo->sinfo_timetolive);
		msg->can_abandon = 1;
		SCTP_DEBUG_PRINTK("%s: msg:%p expires_at: %ld jiffies:%ld\n",
				  __func__, msg, msg->expires_at, jiffies);
	}

	max = asoc->frag_point;

	/* If the the peer requested that we authenticate DATA chunks
	 * we need to accound for bundling of the AUTH chunks along with
	 * DATA.
	 */
	if (sctp_auth_send_cid(SCTP_CID_DATA, asoc)) {
		struct sctp_hmac *hmac_desc = sctp_auth_asoc_get_hmac(asoc);

		if (hmac_desc)
			max -= WORD_ROUND(sizeof(sctp_auth_chunk_t) +
					    hmac_desc->hmac_len);
	}

	whole = 0;
	first_len = max;

	/* Encourage Cookie-ECHO bundling. */
	if (asoc->state < SCTP_STATE_COOKIE_ECHOED) {
		whole = msg_len / (max - SCTP_ARBITRARY_COOKIE_ECHO_LEN);

		/* Account for the DATA to be bundled with the COOKIE-ECHO. */
		if (whole) {
			first_len = max - SCTP_ARBITRARY_COOKIE_ECHO_LEN;
			msg_len -= first_len;
			whole = 1;
		}
	}

	/* How many full sized?  How many bytes leftover? */
	whole += msg_len / max;
	over = msg_len % max;
	offset = 0;

	if ((whole > 1) || (whole && over))
		SCTP_INC_STATS_USER(SCTP_MIB_FRAGUSRMSGS);

	/* Create chunks for all the full sized DATA chunks. */
	for (i=0, len=first_len; i < whole; i++) {
		frag = SCTP_DATA_MIDDLE_FRAG;

		if (0 == i)
			frag |= SCTP_DATA_FIRST_FRAG;

		if ((i == (whole - 1)) && !over)
			frag |= SCTP_DATA_LAST_FRAG;

		chunk = sctp_make_datafrag_empty(asoc, sinfo, len, frag, 0);

		if (!chunk)
			goto errout;
		err = sctp_user_addto_chunk(chunk, offset, len, msgh->msg_iov);
		if (err < 0)
			goto errout;

		offset += len;

		/* Put the chunk->skb back into the form expected by send.  */
		__skb_pull(chunk->skb, (__u8 *)chunk->chunk_hdr
			   - (__u8 *)chunk->skb->data);

		sctp_datamsg_assign(msg, chunk);
		list_add_tail(&chunk->frag_list, &msg->chunks);

		/* The first chunk, the first chunk was likely short
		 * to allow bundling, so reset to full size.
		 */
		if (0 == i)
			len = max;
	}

	/* .. now the leftover bytes. */
	if (over) {
		if (!whole)
			frag = SCTP_DATA_NOT_FRAG;
		else
			frag = SCTP_DATA_LAST_FRAG;

		chunk = sctp_make_datafrag_empty(asoc, sinfo, over, frag, 0);

		if (!chunk)
			goto errout;

		err = sctp_user_addto_chunk(chunk, offset, over,msgh->msg_iov);

		/* Put the chunk->skb back into the form expected by send.  */
		__skb_pull(chunk->skb, (__u8 *)chunk->chunk_hdr
			   - (__u8 *)chunk->skb->data);
		if (err < 0)
			goto errout;

		sctp_datamsg_assign(msg, chunk);
		list_add_tail(&chunk->frag_list, &msg->chunks);
	}

	return msg;

errout:
	list_for_each_safe(pos, temp, &msg->chunks) {
		list_del_init(pos);
		chunk = list_entry(pos, struct sctp_chunk, frag_list);
		sctp_chunk_free(chunk);
	}
	sctp_datamsg_put(msg);
	return NULL;
}

/* Check whether this message has expired. */
int sctp_chunk_abandoned(struct sctp_chunk *chunk)
{
	struct sctp_datamsg *msg = chunk->msg;

	if (!msg->can_abandon)
		return 0;

	if (time_after(jiffies, msg->expires_at))
		return 1;

	return 0;
}

/* This chunk (and consequently entire message) has failed in its sending. */
void sctp_chunk_fail(struct sctp_chunk *chunk, int error)
{
	chunk->msg->send_failed = 1;
	chunk->msg->send_error = error;
}
