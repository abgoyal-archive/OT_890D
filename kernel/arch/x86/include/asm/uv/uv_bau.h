

#ifndef _ASM_X86_UV_UV_BAU_H
#define _ASM_X86_UV_UV_BAU_H

#include <linux/bitmap.h>
#define BITSPERBYTE 8


#define UV_ITEMS_PER_DESCRIPTOR		8
#define UV_CPUS_PER_ACT_STATUS		32
#define UV_ACT_STATUS_MASK		0x3
#define UV_ACT_STATUS_SIZE		2
#define UV_ACTIVATION_DESCRIPTOR_SIZE	32
#define UV_DISTRIBUTION_SIZE		256
#define UV_SW_ACK_NPENDING		8
#define UV_NET_ENDPOINT_INTD		0x38
#define UV_DESC_BASE_PNODE_SHIFT	49
#define UV_PAYLOADQ_PNODE_SHIFT		49
#define UV_PTC_BASENAME			"sgi_uv/ptc_statistics"
#define uv_physnodeaddr(x)		((__pa((unsigned long)(x)) & uv_mmask))

#define DESC_STATUS_IDLE		0
#define DESC_STATUS_ACTIVE		1
#define DESC_STATUS_DESTINATION_TIMEOUT	2
#define DESC_STATUS_SOURCE_TIMEOUT	3

#define SOURCE_TIMEOUT_LIMIT		20
#define DESTINATION_TIMEOUT_LIMIT	20

#define DEST_Q_SIZE			17
#define DEST_NUM_RESOURCES		8
#define MAX_CPUS_PER_NODE		32
#define	FLUSH_RETRY			1
#define	FLUSH_GIVEUP			2
#define	FLUSH_COMPLETE			3

struct bau_target_nodemask {
	unsigned long bits[BITS_TO_LONGS(256)];
};

struct bau_local_cpumask {
	unsigned long bits;
};


struct bau_msg_payload {
	unsigned long address;		/* signifies a page or all TLB's
						of the cpu */
	/* 64 bits */
	unsigned short sending_cpu;	/* filled in by sender */
	/* 16 bits */
	unsigned short acknowledge_count;/* filled in by destination */
	/* 16 bits */
	unsigned int reserved1:32;	/* not usable */
};


struct bau_msg_header {
	unsigned int dest_subnodeid:6;	/* must be zero */
	/* bits 5:0 */
	unsigned int base_dest_nodeid:15; /* nasid>>1 (pnode) of */
	/* bits 20:6 */			  /* first bit in node_map */
	unsigned int command:8;	/* message type */
	/* bits 28:21 */
				/* 0x38: SN3net EndPoint Message */
	unsigned int rsvd_1:3;	/* must be zero */
	/* bits 31:29 */
				/* int will align on 32 bits */
	unsigned int rsvd_2:9;	/* must be zero */
	/* bits 40:32 */
				/* Suppl_A is 56-41 */
	unsigned int payload_2a:8;/* becomes byte 16 of msg */
	/* bits 48:41 */	/* not currently using */
	unsigned int payload_2b:8;/* becomes byte 17 of msg */
	/* bits 56:49 */	/* not currently using */
				/* Address field (96:57) is never used as an
				   address (these are address bits 42:3) */
	unsigned int rsvd_3:1;	/* must be zero */
	/* bit 57 */
				/* address bits 27:4 are payload */
				/* these 24 bits become bytes 12-14 of msg */
	unsigned int replied_to:1;/* sent as 0 by the source to byte 12 */
	/* bit 58 */

	unsigned int payload_1a:5;/* not currently used */
	/* bits 63:59 */
	unsigned int payload_1b:8;/* not currently used */
	/* bits 71:64 */
	unsigned int payload_1c:8;/* not currently used */
	/* bits 79:72 */
	unsigned int payload_1d:2;/* not currently used */
	/* bits 81:80 */

	unsigned int rsvd_4:7;	/* must be zero */
	/* bits 88:82 */
	unsigned int sw_ack_flag:1;/* software acknowledge flag */
	/* bit 89 */
				/* INTD trasactions at destination are to
				   wait for software acknowledge */
	unsigned int rsvd_5:6;	/* must be zero */
	/* bits 95:90 */
	unsigned int rsvd_6:5;	/* must be zero */
	/* bits 100:96 */
	unsigned int int_both:1;/* if 1, interrupt both sockets on the blade */
	/* bit 101*/
	unsigned int fairness:3;/* usually zero */
	/* bits 104:102 */
	unsigned int multilevel:1;	/* multi-level multicast format */
	/* bit 105 */
				/* 0 for TLB: endpoint multi-unicast messages */
	unsigned int chaining:1;/* next descriptor is part of this activation*/
	/* bit 106 */
	unsigned int rsvd_7:21;	/* must be zero */
	/* bits 127:107 */
};

struct bau_desc {
	struct bau_target_nodemask distribution;
	/*
	 * message template, consisting of header and payload:
	 */
	struct bau_msg_header header;
	struct bau_msg_payload payload;
};

struct bau_payload_queue_entry {
	unsigned long address;		/* signifies a page or all TLB's
						of the cpu */
	/* 64 bits, bytes 0-7 */

	unsigned short sending_cpu;	/* cpu that sent the message */
	/* 16 bits, bytes 8-9 */

	unsigned short acknowledge_count; /* filled in by destination */
	/* 16 bits, bytes 10-11 */

	unsigned short replied_to:1;	/* sent as 0 by the source */
	/* 1 bit */
	unsigned short unused1:7;       /* not currently using */
	/* 7 bits: byte 12) */

	unsigned char unused2[2];	/* not currently using */
	/* bytes 13-14 */

	unsigned char sw_ack_vector;	/* filled in by the hardware */
	/* byte 15 (bits 127:120) */

	unsigned char unused4[3];	/* not currently using bytes 17-19 */
	/* bytes 17-19 */

	int number_of_cpus;		/* filled in at destination */
	/* 32 bits, bytes 20-23 (aligned) */

	unsigned char unused5[8];       /* not using */
	/* bytes 24-31 */
};

struct bau_msg_status {
	struct bau_local_cpumask seen_by;	/* map of cpu's */
};

struct bau_sw_ack_status {
	struct bau_payload_queue_entry *msg;	/* associated message */
	int watcher;				/* cpu monitoring, or -1 */
};

struct bau_control {
	struct bau_desc *descriptor_base;
	struct bau_payload_queue_entry *bau_msg_head;
	struct bau_payload_queue_entry *va_queue_first;
	struct bau_payload_queue_entry *va_queue_last;
	struct bau_msg_status *msg_statuses;
	int *watching; /* pointer to array */
};

struct ptc_stats {
	unsigned long ptc_i;	/* number of IPI-style flushes */
	unsigned long requestor;	/* number of nodes this cpu sent to */
	unsigned long requestee;	/* times cpu was remotely requested */
	unsigned long alltlb;	/* times all tlb's on this cpu were flushed */
	unsigned long onetlb;	/* times just one tlb on this cpu was flushed */
	unsigned long s_retry;	/* retries on source side timeouts */
	unsigned long d_retry;	/* retries on destination side timeouts */
	unsigned long sflush;	/* cycles spent in uv_flush_tlb_others */
	unsigned long dflush;	/* cycles spent on destination side */
	unsigned long retriesok; /* successes on retries */
	unsigned long nomsg;	/* interrupts with no message */
	unsigned long multmsg;	/* interrupts with multiple messages */
	unsigned long ntargeted;/* nodes targeted */
};

static inline int bau_node_isset(int node, struct bau_target_nodemask *dstp)
{
	return constant_test_bit(node, &dstp->bits[0]);
}
static inline void bau_node_set(int node, struct bau_target_nodemask *dstp)
{
	__set_bit(node, &dstp->bits[0]);
}
static inline void bau_nodes_clear(struct bau_target_nodemask *dstp, int nbits)
{
	bitmap_zero(&dstp->bits[0], nbits);
}

static inline void bau_cpubits_clear(struct bau_local_cpumask *dstp, int nbits)
{
	bitmap_zero(&dstp->bits, nbits);
}

#define cpubit_isset(cpu, bau_local_cpumask) \
	test_bit((cpu), (bau_local_cpumask).bits)

extern int uv_flush_tlb_others(cpumask_t *, struct mm_struct *, unsigned long);
extern void uv_bau_message_intr1(void);
extern void uv_bau_timeout_intr1(void);

#endif /* _ASM_X86_UV_UV_BAU_H */
