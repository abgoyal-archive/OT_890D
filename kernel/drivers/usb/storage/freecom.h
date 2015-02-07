

#ifndef _FREECOM_USB_H
#define _FREECOM_USB_H

extern int freecom_transport(struct scsi_cmnd *srb, struct us_data *us);
extern int usb_stor_freecom_reset(struct us_data *us);
extern int freecom_init (struct us_data *us);

#endif
