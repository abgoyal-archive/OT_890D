

#ifndef _USB_DATAFAB_MDCFE_B_H
#define _USB_DATAFAB_MDCFE_B_H

extern int datafab_transport(struct scsi_cmnd *srb, struct us_data *us);

struct datafab_info {
	unsigned long   sectors;	// total sector count
	unsigned long   ssize;		// sector size in bytes
	signed char	lun;		// used for dual-slot readers
	
	// the following aren't used yet
	unsigned char   sense_key;
	unsigned long   sense_asc;	// additional sense code
	unsigned long   sense_ascq;	// additional sense code qualifier
};

#endif
