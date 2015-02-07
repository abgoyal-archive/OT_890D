

struct vstusb_args {
	union {
		/* this struct is used for IOCTL_VSTUSB_SEND_PIPE,	*
		 * IOCTL_VSTUSB_RECV_PIPE, and read()/write() fops	*/
		struct {
			void __user	*buffer;
			size_t          count;
			unsigned int    timeout_ms;
			int             pipe;
		};

		/* this one is used for IOCTL_VSTUSB_CONFIG_RW  	*/
		struct {
			int rd_pipe;
			int rd_timeout_ms;
			int wr_pipe;
			int wr_timeout_ms;
		};
	};
};

#define VST_IOC_MAGIC 'L'
#define VST_IOC_FIRST 0x20
#define IOCTL_VSTUSB_SEND_PIPE	_IO(VST_IOC_MAGIC, VST_IOC_FIRST)
#define IOCTL_VSTUSB_RECV_PIPE	_IO(VST_IOC_MAGIC, VST_IOC_FIRST + 1)
#define IOCTL_VSTUSB_CONFIG_RW	_IO(VST_IOC_MAGIC, VST_IOC_FIRST + 2)
