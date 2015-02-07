


#ifndef _MEFIRMWARE_H
# define _MEFIRMWARE_H

# ifdef __KERNEL__

#define ME_ERRNO_FIRMWARE		-1

#define ME_XILINX_CS1_REG		0x00C8


#define ME_FIRMWARE_BUSY_FLAG	0x00000020
#define ME_FIRMWARE_DONE_FLAG	0x00000004
#define ME_FIRMWARE_CS_WRITE	0x00000100

#define ME_PLX_PCI_ACTIVATE		0x43

int me_xilinx_download(unsigned long register_base_control,
		       unsigned long register_base_data,
		       struct device *dev, const char *firmware_name);

# endif	//__KERNEL__

#endif //_MEFIRMWARE_H
