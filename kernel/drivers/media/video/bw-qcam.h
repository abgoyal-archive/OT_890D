


/* One from column A... */
#define QC_NOTSET 0
#define QC_UNIDIR 1
#define QC_BIDIR  2
#define QC_SERIAL 3

/* ... and one from column B */
#define QC_ANY          0x00
#define QC_FORCE_UNIDIR 0x10
#define QC_FORCE_BIDIR  0x20
#define QC_FORCE_SERIAL 0x30
/* in the port_mode member */

#define QC_MODE_MASK    0x07
#define QC_FORCE_MASK   0x70

#define MAX_HEIGHT 243
#define MAX_WIDTH 336

/* Bit fields for status flags */
#define QC_PARAM_CHANGE	0x01 /* Camera status change has occurred */

struct qcam_device {
	struct video_device vdev;
	struct pardevice *pdev;
	struct parport *pport;
	struct mutex lock;
	int width, height;
	int bpp;
	int mode;
	int contrast, brightness, whitebal;
	int port_mode;
	int transfer_scale;
	int top, left;
	int status;
	unsigned int saved_bits;
	unsigned long in_use;
};
