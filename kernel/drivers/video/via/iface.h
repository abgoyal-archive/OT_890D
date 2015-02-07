

#ifndef __IFACE_H__
#define __IFACE_H__

#define Kb  (1024)
#define Mb  (Kb*Kb)

#define VIA_K800_BRIDGE_VID         0x1106
#define VIA_K800_BRIDGE_DID         0x3204

#define VIA_K800_SYSTEM_MEMORY_REG  0x47
#define VIA_K800_VIDEO_MEMORY_REG   0xA1

extern int viafb_memsize;
unsigned int viafb_get_memsize(void);
unsigned long viafb_get_videobuf_addr(void);

#endif /* __IFACE_H__ */
