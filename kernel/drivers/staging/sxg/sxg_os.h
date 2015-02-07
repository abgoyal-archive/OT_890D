

#ifndef _SLIC_OS_SPECIFIC_H_
#define _SLIC_OS_SPECIFIC_H_

#define FALSE	(0)
#define TRUE	(1)

struct LIST_ENTRY {
	struct LIST_ENTRY *nle_flink;
	struct LIST_ENTRY *nle_blink;
};

#define InitializeListHead(l)                   \
        (l)->nle_flink = (l)->nle_blink = (l)

#define IsListEmpty(h)                          \
        ((h)->nle_flink == (h))

#define RemoveEntryList(e)                      \
        do {                                    \
                list_entry              *b;     \
                list_entry              *f;     \
                                                \
                f = (e)->nle_flink;             \
                b = (e)->nle_blink;             \
                b->nle_flink = f;               \
                f->nle_blink = b;               \
        } while (0)

/* These two have to be inlined since they return things. */

static __inline struct LIST_ENTRY *RemoveHeadList(struct LIST_ENTRY *l)
{
	struct LIST_ENTRY *f;
	struct LIST_ENTRY *e;

	e = l->nle_flink;
	f = e->nle_flink;
	l->nle_flink = f;
	f->nle_blink = l;

	return (e);
}

static __inline struct LIST_ENTRY *RemoveTailList(struct LIST_ENTRY *l)
{
	struct LIST_ENTRY *b;
	struct LIST_ENTRY *e;

	e = l->nle_blink;
	b = e->nle_blink;
	l->nle_blink = b;
	b->nle_flink = l;

	return (e);
}

#define InsertTailList(l, e)                    \
        do {                                    \
                struct LIST_ENTRY       *b;     \
                                                \
                b = (l)->nle_blink;             \
                (e)->nle_flink = (l);           \
                (e)->nle_blink = b;             \
                b->nle_flink = (e);             \
                (l)->nle_blink = (e);           \
        } while (0)

#define InsertHeadList(l, e)                    \
        do {                                    \
                struct LIST_ENTRY       *f;     \
                                                \
                f = (l)->nle_flink;             \
                (e)->nle_flink = f;             \
                (e)->nle_blink = l;             \
                f->nle_blink = (e);             \
                (l)->nle_flink = (e);           \
        } while (0)

#define ATK_DEBUG  1

#if ATK_DEBUG
#define SLIC_TIMESTAMP(value) {                                             \
        struct timeval  timev;                                              \
        do_gettimeofday(&timev);                                            \
        value = timev.tv_sec*1000000 + timev.tv_usec;                       \
}
#else
#define SLIC_TIMESTAMP(value)
#endif

/******************  SXG DEFINES  *****************************************/

#ifdef  ATKDBG
#define SXG_TIMESTAMP(value) {                                             \
        struct timeval  timev;                                              \
        do_gettimeofday(&timev);                                            \
        value = timev.tv_sec*1000000 + timev.tv_usec;                       \
}
#else
#define SXG_TIMESTAMP(value)
#endif

#define WRITE_REG(reg,value,flush)                  sxg_reg32_write((&reg), (value), (flush))
#define WRITE_REG64(a,reg,value,cpu)                sxg_reg64_write((a),(&reg),(value),(cpu))
#define READ_REG(reg,value)   (value) = readl((void __iomem *)(&reg))

#endif /* _SLIC_OS_SPECIFIC_H_  */
