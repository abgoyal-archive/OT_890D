

#ifndef __COMPAT_LINUX_WRAPPER_H_
#define __COMPAT_LINUX_WRAPPER_H_

#define mem_map_reserve(p)      set_bit(PG_reserved, &((p)->flags))
#define mem_map_unreserve(p)    clear_bit(PG_reserved, &((p)->flags))

#endif /* __COMPAT_LINUX_WRAPPER_H_ */
