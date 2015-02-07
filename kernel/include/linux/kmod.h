
#ifndef __LINUX_KMOD_H__
#define __LINUX_KMOD_H__


#include <linux/gfp.h>
#include <linux/stddef.h>
#include <linux/errno.h>
#include <linux/compiler.h>

#define KMOD_PATH_LEN 256

#ifdef CONFIG_MODULES
extern int request_module(const char * name, ...) __attribute__ ((format (printf, 1, 2)));
#define try_then_request_module(x, mod...) ((x) ?: (request_module(mod), (x)))
#else
static inline int request_module(const char * name, ...) { return -ENOSYS; }
#define try_then_request_module(x, mod...) (x)
#endif


struct key;
struct file;
struct subprocess_info;

/* Allocate a subprocess_info structure */
struct subprocess_info *call_usermodehelper_setup(char *path, char **argv,
						  char **envp, gfp_t gfp_mask);

/* Set various pieces of state into the subprocess_info structure */
void call_usermodehelper_setkeys(struct subprocess_info *info,
				 struct key *session_keyring);
int call_usermodehelper_stdinpipe(struct subprocess_info *sub_info,
				  struct file **filp);
void call_usermodehelper_setcleanup(struct subprocess_info *info,
				    void (*cleanup)(char **argv, char **envp));

enum umh_wait {
	UMH_NO_WAIT = -1,	/* don't wait at all */
	UMH_WAIT_EXEC = 0,	/* wait for the exec, but not the process */
	UMH_WAIT_PROC = 1,	/* wait for the process to complete */
};

/* Actually execute the sub-process */
int call_usermodehelper_exec(struct subprocess_info *info, enum umh_wait wait);

void call_usermodehelper_freeinfo(struct subprocess_info *info);

static inline int
call_usermodehelper(char *path, char **argv, char **envp, enum umh_wait wait)
{
	struct subprocess_info *info;
	gfp_t gfp_mask = (wait == UMH_NO_WAIT) ? GFP_ATOMIC : GFP_KERNEL;

	info = call_usermodehelper_setup(path, argv, envp, gfp_mask);
	if (info == NULL)
		return -ENOMEM;
	return call_usermodehelper_exec(info, wait);
}

static inline int
call_usermodehelper_keys(char *path, char **argv, char **envp,
			 struct key *session_keyring, enum umh_wait wait)
{
	struct subprocess_info *info;
	gfp_t gfp_mask = (wait == UMH_NO_WAIT) ? GFP_ATOMIC : GFP_KERNEL;

	info = call_usermodehelper_setup(path, argv, envp, gfp_mask);
	if (info == NULL)
		return -ENOMEM;

	call_usermodehelper_setkeys(info, session_keyring);
	return call_usermodehelper_exec(info, wait);
}

extern void usermodehelper_init(void);

struct file;
extern int call_usermodehelper_pipe(char *path, char *argv[], char *envp[],
				    struct file **filp);

extern int usermodehelper_disable(void);
extern void usermodehelper_enable(void);

#endif /* __LINUX_KMOD_H__ */
