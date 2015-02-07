

#ifndef _USB_JUMPSHOT_H
#define _USB_JUMPSHOT_H

extern int jumpshot_transport(struct scsi_cmnd *srb, struct us_data *us);

struct jumpshot_info {
   unsigned long   sectors;     // total sector count
   unsigned long   ssize;       // sector size in bytes
 
   // the following aren't used yet
   unsigned char   sense_key;
   unsigned long   sense_asc;   // additional sense code
   unsigned long   sense_ascq;  // additional sense code qualifier
};

#endif
