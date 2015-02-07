


#ifndef __YAFFS_QSORT_H__
#define __YAFFS_QSORT_H__

extern void yaffs_qsort(void *const base, size_t total_elems, size_t size,
			int (*cmp)(const void *, const void *));

#endif
