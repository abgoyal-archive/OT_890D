
#ifndef _ASM_SGIALIB_H
#define _ASM_SGIALIB_H

#include <asm/sgiarcs.h>

extern struct linux_romvec *romvec;
extern int prom_argc;

extern LONG *_prom_argv, *_prom_envp;

#define prom_argv(index) ((char *) (long) _prom_argv[(index)])
#define prom_argc(index) ((char *) (long) _prom_argc[(index)])

extern int prom_flags;

#define PROM_FLAG_ARCS			1
#define PROM_FLAG_USE_AS_CONSOLE	2
#define PROM_FLAG_DONT_FREE_TEMP	4

/* Simple char-by-char console I/O. */
extern void prom_putchar(char c);
extern char prom_getchar(void);

/* Memory descriptor management. */
#define PROM_MAX_PMEMBLOCKS    32
struct prom_pmemblock {
	LONG	base;		/* Within KSEG0 or XKPHYS. */
	ULONG	size;		/* In bytes. */
	ULONG	type;		/* free or prom memory */
};

extern struct linux_mdesc *prom_getmdesc(struct linux_mdesc *curr);
#define PROM_NULL_MDESC   ((struct linux_mdesc *) 0)

extern void prom_meminit(void);
extern void prom_fixup_mem_map(unsigned long start_mem, unsigned long end_mem);

/* PROM device tree library routines. */
#define PROM_NULL_COMPONENT ((pcomponent *) 0)

/* Get sibling component of THIS. */
extern pcomponent *ArcGetPeer(pcomponent *this);

/* Get child component of THIS. */
extern pcomponent *ArcGetChild(pcomponent *this);

/* Get parent component of CHILD. */
extern pcomponent *prom_getparent(pcomponent *child);

extern long prom_getcdata(void *buffer, pcomponent *this);

/* Other misc. component routines. */
extern pcomponent *prom_childadd(pcomponent *this, pcomponent *tmp, void *data);
extern long prom_delcomponent(pcomponent *this);
extern pcomponent *prom_componentbypath(char *path);

extern void prom_identify_arch(void);

/* Environment variable routines. */
extern PCHAR ArcGetEnvironmentVariable(PCHAR name);
extern LONG ArcSetEnvironmentVariable(PCHAR name, PCHAR value);

/* ARCS command line acquisition and parsing. */
extern char *prom_getcmdline(void);
extern void prom_init_cmdline(void);

/* Acquiring info about the current time, etc. */
extern struct linux_tinfo *prom_gettinfo(void);
extern unsigned long prom_getrtime(void);

/* File operations. */
extern long prom_getvdirent(unsigned long fd, struct linux_vdirent *ent, unsigned long num, unsigned long *cnt);
extern long prom_open(char *name, enum linux_omode md, unsigned long *fd);
extern long prom_close(unsigned long fd);
extern LONG ArcRead(ULONG fd, PVOID buf, ULONG num, PULONG cnt);
extern long prom_getrstatus(unsigned long fd);
extern LONG ArcWrite(ULONG fd, PVOID buf, ULONG num, PULONG cnt);
extern long prom_seek(unsigned long fd, struct linux_bigint *off, enum linux_seekmode sm);
extern long prom_mount(char *name, enum linux_mountops op);
extern long prom_getfinfo(unsigned long fd, struct linux_finfo *buf);
extern long prom_setfinfo(unsigned long fd, unsigned long flags, unsigned long msk);

/* Running stand-along programs. */
extern long prom_load(char *name, unsigned long end, unsigned long *pc, unsigned long *eaddr);
extern long prom_invoke(unsigned long pc, unsigned long sp, long argc, char **argv, char **envp);
extern long prom_exec(char *name, long argc, char **argv, char **envp);

/* Misc. routines. */
extern VOID prom_halt(VOID) __attribute__((noreturn));
extern VOID prom_powerdown(VOID) __attribute__((noreturn));
extern VOID prom_restart(VOID) __attribute__((noreturn));
extern VOID ArcReboot(VOID) __attribute__((noreturn));
extern VOID ArcEnterInteractiveMode(VOID) __attribute__((noreturn));
extern long prom_cfgsave(VOID);
extern struct linux_sysid *prom_getsysid(VOID);
extern VOID ArcFlushAllCaches(VOID);
extern DISPLAY_STATUS *ArcGetDisplayStatus(ULONG FileID);

#endif /* _ASM_SGIALIB_H */
