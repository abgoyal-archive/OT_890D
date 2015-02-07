

#ifndef __ET1310_TX_H__
#define __ET1310_TX_H__


/* Typedefs for Tx Descriptor Ring */

typedef union _txdesc_word2_t {
	u32 value;
	struct {
#ifdef _BIT_FIELDS_HTOL
		u32 vlan_prio:3;		// bits 29-31(VLAN priority)
		u32 vlan_cfi:1;		// bit 28(cfi)
		u32 vlan_tag:12;		// bits 16-27(VLAN tag)
		u32 length_in_bytes:16;	// bits  0-15(packet length)
#else
		u32 length_in_bytes:16;	// bits  0-15(packet length)
		u32 vlan_tag:12;		// bits 16-27(VLAN tag)
		u32 vlan_cfi:1;		// bit 28(cfi)
		u32 vlan_prio:3;		// bits 29-31(VLAN priority)
#endif	/* _BIT_FIELDS_HTOL */
	} bits;
} TXDESC_WORD2_t, *PTXDESC_WORD2_t;

typedef union _txdesc_word3_t {
	u32 value;
	struct {
#ifdef _BIT_FIELDS_HTOL
		u32 unused:17;	// bits 15-31
		u32 udpa:1;	// bit 14(UDP checksum assist)
		u32 tcpa:1;	// bit 13(TCP checksum assist)
		u32 ipa:1;		// bit 12(IP checksum assist)
		u32 vlan:1;	// bit 11(append VLAN tag)
		u32 hp:1;		// bit 10(Packet is a Huge packet)
		u32 pp:1;		// bit  9(pad packet)
		u32 mac:1;		// bit  8(MAC override)
		u32 crc:1;		// bit  7(append CRC)
		u32 e:1;		// bit  6(Tx frame has error)
		u32 pf:1;		// bit  5(send pause frame)
		u32 bp:1;		// bit  4(Issue half-duplex backpressure (XON/XOFF)
		u32 cw:1;		// bit  3(Control word - no packet data)
		u32 ir:1;		// bit  2(interrupt the processor when this pkt sent)
		u32 f:1;		// bit  1(first packet in the sequence)
		u32 l:1;		// bit  0(last packet in the sequence)
#else
		u32 l:1;		// bit  0(last packet in the sequence)
		u32 f:1;		// bit  1(first packet in the sequence)
		u32 ir:1;		// bit  2(interrupt the processor when this pkt sent)
		u32 cw:1;		// bit  3(Control word - no packet data)
		u32 bp:1;		// bit  4(Issue half-duplex backpressure (XON/XOFF)
		u32 pf:1;		// bit  5(send pause frame)
		u32 e:1;		// bit  6(Tx frame has error)
		u32 crc:1;		// bit  7(append CRC)
		u32 mac:1;		// bit  8(MAC override)
		u32 pp:1;		// bit  9(pad packet)
		u32 hp:1;		// bit 10(Packet is a Huge packet)
		u32 vlan:1;	// bit 11(append VLAN tag)
		u32 ipa:1;		// bit 12(IP checksum assist)
		u32 tcpa:1;	// bit 13(TCP checksum assist)
		u32 udpa:1;	// bit 14(UDP checksum assist)
		u32 unused:17;	// bits 15-31
#endif	/* _BIT_FIELDS_HTOL */
	} bits;
} TXDESC_WORD3_t, *PTXDESC_WORD3_t;

/* TX_DESC_ENTRY_t is sructure representing each descriptor on the ring */
typedef struct _tx_desc_entry_t {
	u32 DataBufferPtrHigh;
	u32 DataBufferPtrLow;
	TXDESC_WORD2_t word2;	// control words how to xmit the
	TXDESC_WORD3_t word3;	// data (detailed above)
} TX_DESC_ENTRY_t, *PTX_DESC_ENTRY_t;


/* Typedefs for Tx DMA engine status writeback */

typedef union _tx_status_block_t {
	u32 value;
	struct {
#ifdef _BIT_FIELDS_HTOL
		u32 unused:21;		// bits 11-31
		u32 serv_cpl_wrap:1;	// bit 10
		u32 serv_cpl:10;		// bits 0-9
#else
		u32 serv_cpl:10;		// bits 0-9
		u32 serv_cpl_wrap:1;	// bit 10
		u32 unused:21;		// bits 11-31
#endif
	} bits;
} TX_STATUS_BLOCK_t, *PTX_STATUS_BLOCK_t;

/* TCB (Transmit Control Block) */
typedef struct _MP_TCB {
	struct _MP_TCB *Next;
	u32 Flags;
	u32 Count;
	u32 PacketStaleCount;
	struct sk_buff *Packet;
	u32 PacketLength;
	DMA10W_t WrIndex;
	DMA10W_t WrIndexStart;
} MP_TCB, *PMP_TCB;

/* Structure to hold the skb's in a list */
typedef struct tx_skb_list_elem {
	struct list_head skb_list_elem;
	struct sk_buff *skb;
} TX_SKB_LIST_ELEM, *PTX_SKB_LIST_ELEM;

/* TX_RING_t is sructure representing our local reference(s) to the ring */
typedef struct _tx_ring_t {
	/* TCB (Transmit Control Block) memory and lists */
	PMP_TCB MpTcbMem;

	/* List of TCBs that are ready to be used */
	PMP_TCB TCBReadyQueueHead;
	PMP_TCB TCBReadyQueueTail;

	/* list of TCBs that are currently being sent.  NOTE that access to all
	 * three of these (including nBusySend) are controlled via the
	 * TCBSendQLock.  This lock should be secured prior to incementing /
	 * decrementing nBusySend, or any queue manipulation on CurrSendHead /
	 * Tail
	 */
	PMP_TCB CurrSendHead;
	PMP_TCB CurrSendTail;
	int32_t nBusySend;

	/* List of packets (not TCBs) that were queued for lack of resources */
	struct list_head SendWaitQueue;
	int32_t nWaitSend;

	/* The actual descriptor ring */
	PTX_DESC_ENTRY_t pTxDescRingVa;
	dma_addr_t pTxDescRingPa;
	uint64_t pTxDescRingAdjustedPa;
	uint64_t TxDescOffset;

	/* ReadyToSend indicates where we last wrote to in the descriptor ring. */
	DMA10W_t txDmaReadyToSend;

	/* The location of the write-back status block */
	PTX_STATUS_BLOCK_t pTxStatusVa;
	dma_addr_t pTxStatusPa;

	/* A Block of zeroes used to pad packets that are less than 60 bytes */
	void *pTxDummyBlkVa;
	dma_addr_t pTxDummyBlkPa;

	TXMAC_ERR_t TxMacErr;

	/* Variables to track the Tx interrupt coalescing features */
	int32_t TxPacketsSinceLastinterrupt;
} TX_RING_t, *PTX_RING_t;

/* Forward declaration of the frag-list for the following prototypes */
typedef struct _MP_FRAG_LIST MP_FRAG_LIST, *PMP_FRAG_LIST;

/* Forward declaration of the private adapter structure */
struct et131x_adapter;

/* PROTOTYPES for et1310_tx.c */
int et131x_tx_dma_memory_alloc(struct et131x_adapter *adapter);
void et131x_tx_dma_memory_free(struct et131x_adapter *adapter);
void ConfigTxDmaRegs(struct et131x_adapter *pAdapter);
void et131x_init_send(struct et131x_adapter *adapter);
void et131x_tx_dma_disable(struct et131x_adapter *pAdapter);
void et131x_tx_dma_enable(struct et131x_adapter *pAdapter);
void et131x_handle_send_interrupt(struct et131x_adapter *pAdapter);
void et131x_free_busy_send_packets(struct et131x_adapter *pAdapter);
int et131x_send_packets(struct sk_buff *skb, struct net_device *netdev);

#endif /* __ET1310_TX_H__ */
