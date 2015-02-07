
#ifndef __HPILO_H
#define __HPILO_H

#define ILO_NAME "hpilo"

/* max number of open channel control blocks per device, hw limited to 32 */
#define MAX_CCB		8
/* max number of supported devices */
#define MAX_ILO_DEV	1
/* max number of files */
#define MAX_OPEN	(MAX_CCB * MAX_ILO_DEV)
/* spin counter for open/close delay */
#define MAX_WAIT	10000

struct ilo_hwinfo {
	/* mmio registers on device */
	char __iomem *mmio_vaddr;

	/* doorbell registers on device */
	char __iomem *db_vaddr;

	/* shared memory on device used for channel control blocks */
	char __iomem *ram_vaddr;

	/* files corresponding to this device */
	struct ccb_data *ccb_alloc[MAX_CCB];

	struct pci_dev *ilo_dev;

	spinlock_t alloc_lock;
	spinlock_t fifo_lock;

	struct cdev cdev;
};

/* offset from mmio_vaddr */
#define DB_OUT		0xD4
/* DB_OUT reset bit */
#define DB_RESET	26

#define ILOSW_CCB_SZ	64
#define ILOHW_CCB_SZ 	128
struct ccb {
	union {
		char *send_fifobar;
		u64 padding1;
	} ccb_u1;
	union {
		char *send_desc;
		u64 padding2;
	} ccb_u2;
	u64 send_ctrl;

	union {
		char *recv_fifobar;
		u64 padding3;
	} ccb_u3;
	union {
		char *recv_desc;
		u64 padding4;
	} ccb_u4;
	u64 recv_ctrl;

	union {
		char __iomem *db_base;
		u64 padding5;
	} ccb_u5;

	u64 channel;

	/* unused context area (64 bytes) */
};

/* ccb queue parameters */
#define SENDQ		1
#define RECVQ 		2
#define NR_QENTRY    	4
#define L2_QENTRY_SZ 	12

/* ccb ctrl bitfields */
#define CTRL_BITPOS_L2SZ             0
#define CTRL_BITPOS_FIFOINDEXMASK    4
#define CTRL_BITPOS_DESCLIMIT        18
#define CTRL_BITPOS_A                30
#define CTRL_BITPOS_G                31

/* ccb doorbell macros */
#define L2_DB_SIZE		14
#define ONE_DB_SIZE		(1 << L2_DB_SIZE)

struct ccb_data {
	/* software version of ccb, using virtual addrs */
	struct ccb  driver_ccb;

	/* hardware version of ccb, using physical addrs */
	struct ccb  ilo_ccb;

	/* hardware ccb is written to this shared mapped device memory */
	struct ccb __iomem *mapped_ccb;

	/* dma'able memory used for send/recv queues */
	void       *dma_va;
	dma_addr_t  dma_pa;
	size_t      dma_size;

	/* pointer to hardware device info */
	struct ilo_hwinfo *ilo_hw;

	/* usage count, to allow for shared ccb's */
	int	    ccb_cnt;

	/* open wanted exclusive access to this ccb */
	int	    ccb_excl;
};

#define ILO_START_ALIGN	4096
#define ILO_CACHE_SZ 	 128
struct fifo {
    u64 nrents;	/* user requested number of fifo entries */
    u64 imask;  /* mask to extract valid fifo index */
    u64 merge;	/*  O/C bits to merge in during enqueue operation */
    u64 reset;	/* set to non-zero when the target device resets */
    u8  pad_0[ILO_CACHE_SZ - (sizeof(u64) * 4)];

    u64 head;
    u8  pad_1[ILO_CACHE_SZ - (sizeof(u64))];

    u64 tail;
    u8  pad_2[ILO_CACHE_SZ - (sizeof(u64))];

    u64 fifobar[1];
};

/* convert between struct fifo, and the fifobar, which is saved in the ccb */
#define FIFOHANDLESIZE (sizeof(struct fifo) - sizeof(u64))
#define FIFOBARTOHANDLE(_fifo) \
	((struct fifo *)(((char *)(_fifo)) - FIFOHANDLESIZE))

/* the number of qwords to consume from the entry descriptor */
#define ENTRY_BITPOS_QWORDS      0
/* descriptor index number (within a specified queue) */
#define ENTRY_BITPOS_DESCRIPTOR  10
/* state bit, fifo entry consumed by consumer */
#define ENTRY_BITPOS_C           22
/* state bit, fifo entry is occupied */
#define ENTRY_BITPOS_O           23

#define ENTRY_BITS_QWORDS        10
#define ENTRY_BITS_DESCRIPTOR    12
#define ENTRY_BITS_C             1
#define ENTRY_BITS_O             1
#define ENTRY_BITS_TOTAL	\
	(ENTRY_BITS_C + ENTRY_BITS_O + \
	 ENTRY_BITS_QWORDS + ENTRY_BITS_DESCRIPTOR)

/* extract various entry fields */
#define ENTRY_MASK ((1 << ENTRY_BITS_TOTAL) - 1)
#define ENTRY_MASK_C (((1 << ENTRY_BITS_C) - 1) << ENTRY_BITPOS_C)
#define ENTRY_MASK_O (((1 << ENTRY_BITS_O) - 1) << ENTRY_BITPOS_O)
#define ENTRY_MASK_QWORDS \
	(((1 << ENTRY_BITS_QWORDS) - 1) << ENTRY_BITPOS_QWORDS)
#define ENTRY_MASK_DESCRIPTOR \
	(((1 << ENTRY_BITS_DESCRIPTOR) - 1) << ENTRY_BITPOS_DESCRIPTOR)

#define ENTRY_MASK_NOSTATE (ENTRY_MASK >> (ENTRY_BITS_C + ENTRY_BITS_O))

#endif /* __HPILO_H */
