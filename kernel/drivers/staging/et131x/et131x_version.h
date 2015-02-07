

#ifndef __ET131X_VERSION_H__
#define __ET131X_VERSION_H__

#define DRIVER_AUTHOR		"Victor Soriano (vjsoriano@agere.com)"
#define DRIVER_LICENSE		"Dual BSD/GPL"
#define DRIVER_DEVICE_STRING	"ET1310"
#define DRIVER_NAME		"et131x"
#define DRIVER_MAJOR_VERSION	1
#define DRIVER_MINOR_VERSION	2
#define DRIVER_PATCH_VERSION	3
#define DRIVER_VERSION_STRING	"1.2.3"
#define DRIVER_VENDOR		"Agere Systems, http://www.agere.com"
#define DRIVER_DESC		"10/100/1000 Base-T Ethernet Driver"

#define STRUCT_MODULE		"net"	/* blux: missed by the kernel */

#define DRIVER_INFO		DRIVER_DESC " for the "\
				DRIVER_DEVICE_STRING ", v" \
				DRIVER_VERSION_STRING " by " \
				DRIVER_VENDOR

#define DRIVER_NAME_EXT		"et131x.ko"

#endif /* __ET131X_VERSION_H__ */
