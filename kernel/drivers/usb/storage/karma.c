

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>

#include "usb.h"
#include "transport.h"
#include "debug.h"
#include "karma.h"

#define RIO_PREFIX "RIOP\x00"
#define RIO_PREFIX_LEN 5
#define RIO_SEND_LEN 40
#define RIO_RECV_LEN 0x200

#define RIO_ENTER_STORAGE 0x1
#define RIO_LEAVE_STORAGE 0x2
#define RIO_RESET 0xC

extern int usb_stor_Bulk_transport(struct scsi_cmnd *, struct us_data *);

struct karma_data {
	int in_storage;
	char *recv;
};

static int rio_karma_send_command(char cmd, struct us_data *us)
{
	int result, partial;
	unsigned long timeout;
	static unsigned char seq = 1;
	struct karma_data *data = (struct karma_data *) us->extra;

	US_DEBUGP("karma: sending command %04x\n", cmd);
	memset(us->iobuf, 0, RIO_SEND_LEN);
	memcpy(us->iobuf, RIO_PREFIX, RIO_PREFIX_LEN);
	us->iobuf[5] = cmd;
	us->iobuf[6] = seq;

	timeout = jiffies + msecs_to_jiffies(6000);
	for (;;) {
		result = usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe,
			us->iobuf, RIO_SEND_LEN, &partial);
		if (result != USB_STOR_XFER_GOOD)
			goto err;

		result = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe,
			data->recv, RIO_RECV_LEN, &partial);
		if (result != USB_STOR_XFER_GOOD)
			goto err;

		if (data->recv[5] == seq)
			break;

		if (time_after(jiffies, timeout))
			goto err;

		us->iobuf[4] = 0x80;
		us->iobuf[5] = 0;
		msleep(50);
	}

	seq++;
	if (seq == 0)
		seq = 1;

	US_DEBUGP("karma: sent command %04x\n", cmd);
	return 0;
err:
	US_DEBUGP("karma: command %04x failed\n", cmd);
	return USB_STOR_TRANSPORT_FAILED;
}

int rio_karma_transport(struct scsi_cmnd *srb, struct us_data *us)
{
	int ret;
	struct karma_data *data = (struct karma_data *) us->extra;

	if (srb->cmnd[0] == READ_10 && !data->in_storage) {
		ret = rio_karma_send_command(RIO_ENTER_STORAGE, us);
		if (ret)
			return ret;

		data->in_storage = 1;
		return usb_stor_Bulk_transport(srb, us);
	} else if (srb->cmnd[0] == START_STOP) {
		ret = rio_karma_send_command(RIO_LEAVE_STORAGE, us);
		if (ret)
			return ret;

		data->in_storage = 0;
		return rio_karma_send_command(RIO_RESET, us);
	}
	return usb_stor_Bulk_transport(srb, us);
}

static void rio_karma_destructor(void *extra)
{
	struct karma_data *data = (struct karma_data *) extra;
	kfree(data->recv);
}

int rio_karma_init(struct us_data *us)
{
	int ret = 0;
	struct karma_data *data = kzalloc(sizeof(struct karma_data), GFP_NOIO);
	if (!data)
		goto out;

	data->recv = kmalloc(RIO_RECV_LEN, GFP_NOIO);
	if (!data->recv) {
		kfree(data);
		goto out;
	}

	us->extra = data;
	us->extra_destructor = rio_karma_destructor;
	ret = rio_karma_send_command(RIO_ENTER_STORAGE, us);
	data->in_storage = (ret == 0);
out:
	return ret;
}
