

#ifndef __KVM_TYPES_H__
#define __KVM_TYPES_H__

#include <asm/types.h>


typedef unsigned long  gva_t;
typedef u64            gpa_t;
typedef unsigned long  gfn_t;

typedef unsigned long  hva_t;
typedef u64            hpa_t;
typedef unsigned long  hfn_t;

typedef hfn_t pfn_t;

struct kvm_pio_request {
	unsigned long count;
	int cur_count;
	struct page *guest_pages[2];
	unsigned guest_page_offset;
	int in;
	int port;
	int size;
	int string;
	int down;
	int rep;
};

#endif /* __KVM_TYPES_H__ */
