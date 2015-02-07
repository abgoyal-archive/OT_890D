
struct poch_cbuf_header {
	__s32 group_size_bytes;
	__s32 group_count;
	__s32 group_offsets[0];
};

struct poch_counters {
	__u32 fifo_empty;
	__u32 fifo_overflow;
	__u32 pll_unlock;
};

#define POCH_IOC_NUM			'9'

#define POCH_IOC_TRANSFER_START		_IO(POCH_IOC_NUM, 0)
#define POCH_IOC_TRANSFER_STOP		_IO(POCH_IOC_NUM, 1)
#define POCH_IOC_GET_COUNTERS		_IOR(POCH_IOC_NUM, 2, \
					     struct poch_counters)
#define POCH_IOC_SYNC_GROUP_FOR_USER	_IO(POCH_IOC_NUM, 3)
#define POCH_IOC_SYNC_GROUP_FOR_DEVICE	_IO(POCH_IOC_NUM, 4)
