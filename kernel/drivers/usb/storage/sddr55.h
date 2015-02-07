

#ifndef _USB_SHUTTLE_EUSB_SDDR55_H
#define _USB_SHUTTLE_EUSB_SDDR55_H

/* Sandisk SDDR-55 stuff */

extern int sddr55_transport(struct scsi_cmnd *srb, struct us_data *us);
extern int sddr55_reset(struct us_data *us);

#endif
