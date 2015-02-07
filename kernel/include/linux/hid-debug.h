
#ifndef __HID_DEBUG_H
#define __HID_DEBUG_H



#ifdef CONFIG_HID_DEBUG

void hid_dump_input(struct hid_usage *, __s32);
void hid_dump_device(struct hid_device *);
void hid_dump_field(struct hid_field *, int);
void hid_resolv_usage(unsigned);
void hid_resolv_event(__u8, __u16);

#else

#define hid_dump_input(a,b)     do { } while (0)
#define hid_dump_device(c)      do { } while (0)
#define hid_dump_field(a,b)     do { } while (0)
#define hid_resolv_usage(a)         do { } while (0)
#define hid_resolv_event(a,b)       do { } while (0)

#endif /* CONFIG_HID_DEBUG */


#endif

