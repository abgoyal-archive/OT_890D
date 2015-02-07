

#ifndef	DM_PATH_SELECTOR_H
#define	DM_PATH_SELECTOR_H

#include <linux/device-mapper.h>

#include "dm-mpath.h"

struct path_selector_type;
struct path_selector {
	struct path_selector_type *type;
	void *context;
};

/* Information about a path selector type */
struct path_selector_type {
	char *name;
	struct module *module;

	unsigned int table_args;
	unsigned int info_args;

	/*
	 * Constructs a path selector object, takes custom arguments
	 */
	int (*create) (struct path_selector *ps, unsigned argc, char **argv);
	void (*destroy) (struct path_selector *ps);

	/*
	 * Add an opaque path object, along with some selector specific
	 * path args (eg, path priority).
	 */
	int (*add_path) (struct path_selector *ps, struct dm_path *path,
			 int argc, char **argv, char **error);

	/*
	 * Chooses a path for this io, if no paths are available then
	 * NULL will be returned.
	 *
	 * repeat_count is the number of times to use the path before
	 * calling the function again.  0 means don't call it again unless
	 * the path fails.
	 */
	struct dm_path *(*select_path) (struct path_selector *ps,
				     unsigned *repeat_count);

	/*
	 * Notify the selector that a path has failed.
	 */
	void (*fail_path) (struct path_selector *ps, struct dm_path *p);

	/*
	 * Ask selector to reinstate a path.
	 */
	int (*reinstate_path) (struct path_selector *ps, struct dm_path *p);

	/*
	 * Table content based on parameters added in ps_add_path_fn
	 * or path selector status
	 */
	int (*status) (struct path_selector *ps, struct dm_path *path,
		       status_type_t type, char *result, unsigned int maxlen);

	int (*end_io) (struct path_selector *ps, struct dm_path *path);
};

/* Register a path selector */
int dm_register_path_selector(struct path_selector_type *type);

/* Unregister a path selector */
int dm_unregister_path_selector(struct path_selector_type *type);

/* Returns a registered path selector type */
struct path_selector_type *dm_get_path_selector(const char *name);

/* Releases a path selector  */
void dm_put_path_selector(struct path_selector_type *pst);

#endif
