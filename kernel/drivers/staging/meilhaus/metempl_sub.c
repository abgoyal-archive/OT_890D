


#ifndef __KERNEL__
#  define __KERNEL__
#endif

#include <linux/module.h>

#include <linux/slab.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/types.h>

#include "medefines.h"
#include "meinternal.h"
#include "meerror.h"

#include "medebug.h"
#include "metempl_sub_reg.h"
#include "metempl_sub.h"



static void metempl_sub_destructor(struct me_subdevice *subdevice)
{
	metempl_sub_subdevice_t *instance;

	PDEBUG("executed.\n");
	instance = (metempl_sub_subdevice_t *) subdevice;

	/* Until there this was the things the default constructor does.
	   If you do not have any additional things to do you can wipe it out. */

	me_subdevice_deinit(&instance->base);
	kfree(instance);
}

static int metempl_sub_query_number_channels(me_subdevice_t * subdevice,
					     int *number)
{
	PDEBUG("executed.\n");
	*number = 0;
	return ME_ERRNO_SUCCESS;
}

static int metempl_sub_query_subdevice_type(me_subdevice_t * subdevice,
					    int *type, int *subtype)
{
	PDEBUG("executed.\n");
	*type = 0;
	*subtype = 0;
	return ME_ERRNO_SUCCESS;
}

static int metempl_sub_query_subdevice_caps(me_subdevice_t * subdevice,
					    int *caps)
{
	PDEBUG("executed.\n");
	*caps = 0;
	return ME_ERRNO_SUCCESS;
}

metempl_sub_subdevice_t *metempl_sub_constructor(uint32_t reg_base,
						 unsigned int sub_idx,
						 spinlock_t * ctrl_reg_lock)
{
	metempl_sub_subdevice_t *subdevice;
	int err;

	PDEBUG("executed.\n");

	/* Allocate memory for subdevice instance */
	subdevice = kmalloc(sizeof(metempl_sub_subdevice_t), GFP_KERNEL);

	if (!subdevice) {
		PERROR("Cannot get memory for subdevice instance.\n");
		return NULL;
	}

	memset(subdevice, 0, sizeof(metempl_sub_subdevice_t));

	/* Check if subdevice index is out of range */

	if (sub_idx >= 2) {
		PERROR("Template subdevice index is out of range.\n");
		kfree(subdevice);
		return NULL;
	}

	/* Initialize subdevice base class */
	err = me_subdevice_init(&subdevice->base);

	if (err) {
		PERROR("Cannot initialize subdevice base class instance.\n");
		kfree(subdevice);
		return NULL;
	}
	// Initialize spin locks.
	spin_lock_init(&subdevice->subdevice_lock);

	subdevice->ctrl_reg_lock = ctrl_reg_lock;

	/* Save the subdevice index */
	subdevice->sub_idx = sub_idx;

	/* Override base class methods. */
	subdevice->base.me_subdevice_destructor = metempl_sub_destructor;
	subdevice->base.me_subdevice_query_number_channels =
	    metempl_sub_query_number_channels;
	subdevice->base.me_subdevice_query_subdevice_type =
	    metempl_sub_query_subdevice_type;
	subdevice->base.me_subdevice_query_subdevice_caps =
	    metempl_sub_query_subdevice_caps;

	return subdevice;
}
