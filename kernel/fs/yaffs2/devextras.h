


#ifndef __EXTRAS_H__
#define __EXTRAS_H__


#if !(defined __KERNEL__)

/* Definition of types */
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned __u32;

#endif


struct ylist_head {
	struct ylist_head *next; /* next in chain */
	struct ylist_head *prev; /* previous in chain */
};


/* Initialise a static list */
#define YLIST_HEAD(name) \
struct ylist_head name = { &(name), &(name)}



/* Initialise a list head to an empty list */
#define YINIT_LIST_HEAD(p) \
do { \
	(p)->next = (p);\
	(p)->prev = (p); \
} while (0)


/* Add an element to a list */
static __inline__ void ylist_add(struct ylist_head *newEntry,
				struct ylist_head *list)
{
	struct ylist_head *listNext = list->next;

	list->next = newEntry;
	newEntry->prev = list;
	newEntry->next = listNext;
	listNext->prev = newEntry;

}

static __inline__ void ylist_add_tail(struct ylist_head *newEntry,
				 struct ylist_head *list)
{
	struct ylist_head *listPrev = list->prev;

	list->prev = newEntry;
	newEntry->next = list;
	newEntry->prev = listPrev;
	listPrev->next = newEntry;

}


static __inline__ void ylist_del(struct ylist_head *entry)
{
	struct ylist_head *listNext = entry->next;
	struct ylist_head *listPrev = entry->prev;

	listNext->prev = listPrev;
	listPrev->next = listNext;

}

static __inline__ void ylist_del_init(struct ylist_head *entry)
{
	ylist_del(entry);
	entry->next = entry->prev = entry;
}


/* Test if the list is empty */
static __inline__ int ylist_empty(struct ylist_head *entry)
{
	return (entry->next == entry);
}




#define ylist_entry(entry, type, member) \
	((type *)((char *)(entry)-(unsigned long)(&((type *)NULL)->member)))



#define ylist_for_each(itervar, list) \
	for (itervar = (list)->next; itervar != (list); itervar = itervar->next)

#define ylist_for_each_safe(itervar, saveVar, list) \
	for (itervar = (list)->next, saveVar = (list)->next->next; \
		itervar != (list); itervar = saveVar, saveVar = saveVar->next)


#if !(defined __KERNEL__)


#ifndef WIN32
#include <sys/stat.h>
#endif


#ifdef CONFIG_YAFFS_PROVIDE_DEFS
/* File types */


#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14


#ifndef WIN32
#include <sys/stat.h>
#endif

#define ATTR_MODE	1
#define ATTR_UID	2
#define ATTR_GID	4
#define ATTR_SIZE	8
#define ATTR_ATIME	16
#define ATTR_MTIME	32
#define ATTR_CTIME	64

struct iattr {
	unsigned int ia_valid;
	unsigned ia_mode;
	unsigned ia_uid;
	unsigned ia_gid;
	unsigned ia_size;
	unsigned ia_atime;
	unsigned ia_mtime;
	unsigned ia_ctime;
	unsigned int ia_attr_flags;
};

#endif

#else

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/stat.h>

#endif


#endif
