

#ifndef __UBI_USER_H__
#define __UBI_USER_H__


#define UBI_VOL_NUM_AUTO (-1)
#define UBI_DEV_NUM_AUTO (-1)

/* Maximum volume name length */
#define UBI_MAX_VOLUME_NAME 127

/* ioctl commands of UBI character devices */

#define UBI_IOC_MAGIC 'o'

/* Create an UBI volume */
#define UBI_IOCMKVOL _IOW(UBI_IOC_MAGIC, 0, struct ubi_mkvol_req)
/* Remove an UBI volume */
#define UBI_IOCRMVOL _IOW(UBI_IOC_MAGIC, 1, int32_t)
/* Re-size an UBI volume */
#define UBI_IOCRSVOL _IOW(UBI_IOC_MAGIC, 2, struct ubi_rsvol_req)
/* Re-name volumes */
#define UBI_IOCRNVOL _IOW(UBI_IOC_MAGIC, 3, struct ubi_rnvol_req)

/* ioctl commands of the UBI control character device */

#define UBI_CTRL_IOC_MAGIC 'o'

/* Attach an MTD device */
#define UBI_IOCATT _IOW(UBI_CTRL_IOC_MAGIC, 64, struct ubi_attach_req)
/* Detach an MTD device */
#define UBI_IOCDET _IOW(UBI_CTRL_IOC_MAGIC, 65, int32_t)

/* ioctl commands of UBI volume character devices */

#define UBI_VOL_IOC_MAGIC 'O'

/* Start UBI volume update */
#define UBI_IOCVOLUP _IOW(UBI_VOL_IOC_MAGIC, 0, int64_t)
/* LEB erasure command, used for debugging, disabled by default */
#define UBI_IOCEBER _IOW(UBI_VOL_IOC_MAGIC, 1, int32_t)
/* Atomic LEB change command */
#define UBI_IOCEBCH _IOW(UBI_VOL_IOC_MAGIC, 2, int32_t)
/* Map LEB command */
#define UBI_IOCEBMAP _IOW(UBI_VOL_IOC_MAGIC, 3, struct ubi_map_req)
/* Unmap LEB command */
#define UBI_IOCEBUNMAP _IOW(UBI_VOL_IOC_MAGIC, 4, int32_t)
/* Check if LEB is mapped command */
#define UBI_IOCEBISMAP _IOR(UBI_VOL_IOC_MAGIC, 5, int32_t)
/* Set an UBI volume property */
#define UBI_IOCSETPROP _IOW(UBI_VOL_IOC_MAGIC, 6, struct ubi_set_prop_req)

/* Maximum MTD device name length supported by UBI */
#define MAX_UBI_MTD_NAME_LEN 127

/* Maximum amount of UBI volumes that can be re-named at one go */
#define UBI_MAX_RNVOL 32

enum {
	UBI_LONGTERM  = 1,
	UBI_SHORTTERM = 2,
	UBI_UNKNOWN   = 3,
};

enum {
	UBI_DYNAMIC_VOLUME = 3,
	UBI_STATIC_VOLUME  = 4,
};

enum {
       UBI_PROP_DIRECT_WRITE = 1,
};

struct ubi_attach_req {
	int32_t ubi_num;
	int32_t mtd_num;
	int32_t vid_hdr_offset;
	int8_t padding[12];
};

struct ubi_mkvol_req {
	int32_t vol_id;
	int32_t alignment;
	int64_t bytes;
	int8_t vol_type;
	int8_t padding1;
	int16_t name_len;
	int8_t padding2[4];
	char name[UBI_MAX_VOLUME_NAME + 1];
} __attribute__ ((packed));

struct ubi_rsvol_req {
	int64_t bytes;
	int32_t vol_id;
} __attribute__ ((packed));

struct ubi_rnvol_req {
	int32_t count;
	int8_t padding1[12];
	struct {
		int32_t vol_id;
		int16_t name_len;
		int8_t  padding2[2];
		char    name[UBI_MAX_VOLUME_NAME + 1];
	} ents[UBI_MAX_RNVOL];
} __attribute__ ((packed));

struct ubi_leb_change_req {
	int32_t lnum;
	int32_t bytes;
	int8_t  dtype;
	int8_t  padding[7];
} __attribute__ ((packed));

struct ubi_map_req {
	int32_t lnum;
	int8_t  dtype;
	int8_t  padding[3];
} __attribute__ ((packed));


struct ubi_set_prop_req {
       uint8_t  property;
       uint8_t  padding[7];
       uint64_t value;
}  __attribute__ ((packed));

#endif /* __UBI_USER_H__ */
