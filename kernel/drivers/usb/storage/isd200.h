

#ifndef _USB_ISD200_H
#define _USB_ISD200_H

extern void isd200_ata_command(struct scsi_cmnd *srb, struct us_data *us);
extern int isd200_Initialization(struct us_data *us);

#endif
