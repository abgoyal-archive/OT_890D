

#ifndef _USB_SHUTTLE_EUSB_SDDR09_H
#define _USB_SHUTTLE_EUSB_SDDR09_H

/* Sandisk SDDR-09 stuff */

extern int sddr09_transport(struct scsi_cmnd *srb, struct us_data *us);
extern int usb_stor_sddr09_init(struct us_data *us);

/* Microtech DPCM-USB stuff */

extern int dpcm_transport(struct scsi_cmnd *srb, struct us_data *us);
extern int usb_stor_sddr09_dpcm_init(struct us_data *us);

#endif
