
#ifndef __XFS_SUPPORT_KTRACE_H__
#define __XFS_SUPPORT_KTRACE_H__

typedef struct ktrace_entry {
	void	*val[16];
} ktrace_entry_t;

typedef struct ktrace {
	int		kt_nentries;	/* number of entries in trace buf */
	atomic_t	kt_index;	/* current index in entries */
	unsigned int	kt_index_mask;
	int		kt_rollover;
	ktrace_entry_t	*kt_entries;	/* buffer of entries */
} ktrace_t;

typedef struct ktrace_snap {
	int		ks_start;	/* kt_index at time of snap */
	int		ks_index;	/* current index */
} ktrace_snap_t;


#ifdef CONFIG_XFS_TRACE

extern void ktrace_init(int zentries);
extern void ktrace_uninit(void);

extern ktrace_t *ktrace_alloc(int, unsigned int __nocast);
extern void ktrace_free(ktrace_t *);

extern void ktrace_enter(
	ktrace_t	*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*,
	void		*);

extern ktrace_entry_t   *ktrace_first(ktrace_t *, ktrace_snap_t *);
extern int              ktrace_nentries(ktrace_t *);
extern ktrace_entry_t   *ktrace_next(ktrace_t *, ktrace_snap_t *);
extern ktrace_entry_t   *ktrace_skip(ktrace_t *, int, ktrace_snap_t *);

#else
#define ktrace_init(x)	do { } while (0)
#define ktrace_uninit()	do { } while (0)
#endif	/* CONFIG_XFS_TRACE */

#endif	/* __XFS_SUPPORT_KTRACE_H__ */
