

#ifndef _USB_ALAUDA_H
#define _USB_ALAUDA_H

#define ALAUDA_STATUS_ERROR		0x01
#define ALAUDA_STATUS_READY		0x40

#define ALAUDA_GET_XD_MEDIA_STATUS	0x08
#define ALAUDA_GET_SM_MEDIA_STATUS	0x98
#define ALAUDA_ACK_XD_MEDIA_CHANGE	0x0a
#define ALAUDA_ACK_SM_MEDIA_CHANGE	0x9a
#define ALAUDA_GET_XD_MEDIA_SIG		0x86
#define ALAUDA_GET_SM_MEDIA_SIG		0x96

#define ALAUDA_BULK_CMD			0x40

#define ALAUDA_BULK_GET_REDU_DATA	0x85
#define ALAUDA_BULK_READ_BLOCK		0x94
#define ALAUDA_BULK_ERASE_BLOCK		0xa3
#define ALAUDA_BULK_WRITE_BLOCK		0xb4
#define ALAUDA_BULK_GET_STATUS2		0xb7
#define ALAUDA_BULK_RESET_MEDIA		0xe0

#define ALAUDA_PORT_XD			0x00
#define ALAUDA_PORT_SM			0x01

#define UNDEF    0xffff
#define SPARE    0xfffe
#define UNUSABLE 0xfffd

int init_alauda(struct us_data *us);
int alauda_transport(struct scsi_cmnd *srb, struct us_data *us);

struct alauda_media_info {
	unsigned long capacity;		/* total media size in bytes */
	unsigned int pagesize;		/* page size in bytes */
	unsigned int blocksize;		/* number of pages per block */
	unsigned int uzonesize;		/* number of usable blocks per zone */
	unsigned int zonesize;		/* number of blocks per zone */
	unsigned int blockmask;		/* mask to get page from address */

	unsigned char pageshift;
	unsigned char blockshift;
	unsigned char zoneshift;

	u16 **lba_to_pba;		/* logical to physical block map */
	u16 **pba_to_lba;		/* physical to logical block map */
};

struct alauda_info {
	struct alauda_media_info port[2];
	int wr_ep;			/* endpoint to write data out of */

	unsigned char sense_key;
	unsigned long sense_asc;	/* additional sense code */
	unsigned long sense_ascq;	/* additional sense code qualifier */
};

#endif

