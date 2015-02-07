


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
#include "me0900_reg.h"
#include "me0900_do.h"



static int me0900_do_io_reset_subdevice(struct me_subdevice *subdevice,
					struct file *filep, int flags)
{
	me0900_do_subdevice_t *instance;

	PDEBUG("executed.\n");

	instance = (me0900_do_subdevice_t *) subdevice;

	if (flags) {
		PERROR("Invalid flag specified.\n");
		return ME_ERRNO_INVALID_FLAGS;
	}

	ME_SUBDEVICE_ENTER;

	spin_lock(&instance->subdevice_lock);
	outb(0xFF, instance->port_reg);
	PDEBUG_REG("port_reg outl(0x%lX+0x%lX)=0x%x\n", instance->reg_base,
		   instance->port_reg - instance->reg_base, 0xff);
	spin_unlock(&instance->subdevice_lock);

	ME_SUBDEVICE_EXIT;

	return ME_ERRNO_SUCCESS;
}

static int me0900_do_io_single_config(me_subdevice_t * subdevice,
				      struct file *filep,
				      int channel,
				      int single_config,
				      int ref,
				      int trig_chan,
				      int trig_type, int trig_edge, int flags)
{
	me0900_do_subdevice_t *instance;
	int err = ME_ERRNO_SUCCESS;

	PDEBUG("executed.\n");

	instance = (me0900_do_subdevice_t *) subdevice;

	ME_SUBDEVICE_ENTER;

	spin_lock(&instance->subdevice_lock);
	switch (flags) {
	case ME_IO_SINGLE_CONFIG_NO_FLAGS:
	case ME_IO_SINGLE_TYPE_DIO_BYTE:
		if (channel == 0) {
			if (single_config == ME_SINGLE_CONFIG_DIO_OUTPUT) {
			} else {
				PERROR("Invalid byte direction specified.\n");
				err = ME_ERRNO_INVALID_SINGLE_CONFIG;
			}
		} else {
			PERROR("Invalid byte number specified.\n");
			err = ME_ERRNO_INVALID_CHANNEL;
		}
		break;

	default:
		PERROR("Invalid flags specified.\n");
		err = ME_ERRNO_INVALID_FLAGS;
	}
	spin_unlock(&instance->subdevice_lock);

	ME_SUBDEVICE_EXIT;

	return err;
}

static int me0900_do_io_single_read(me_subdevice_t * subdevice,
				    struct file *filep,
				    int channel,
				    int *value, int time_out, int flags)
{
	me0900_do_subdevice_t *instance;
	int err = ME_ERRNO_SUCCESS;

	PDEBUG("executed.\n");

	instance = (me0900_do_subdevice_t *) subdevice;

	ME_SUBDEVICE_ENTER;

	spin_lock(&instance->subdevice_lock);
	switch (flags) {
	case ME_IO_SINGLE_TYPE_DIO_BIT:
		if ((channel >= 0) && (channel < 8)) {
			*value = (~inb(instance->port_reg)) & (0x1 << channel);
		} else {
			PERROR("Invalid bit number specified.\n");
			err = ME_ERRNO_INVALID_CHANNEL;
		}
		break;

	case ME_IO_SINGLE_NO_FLAGS:
	case ME_IO_SINGLE_TYPE_DIO_BYTE:
		if (channel == 0) {
			*value = ~inb(instance->port_reg);
		} else {
			PERROR("Invalid byte number specified.\n");
			err = ME_ERRNO_INVALID_CHANNEL;
		}
		break;

	default:
		PERROR("Invalid flags specified.\n");
		err = ME_ERRNO_INVALID_FLAGS;
	}
	spin_unlock(&instance->subdevice_lock);

	ME_SUBDEVICE_EXIT;

	return err;
}

static int me0900_do_io_single_write(me_subdevice_t * subdevice,
				     struct file *filep,
				     int channel,
				     int value, int time_out, int flags)
{
	me0900_do_subdevice_t *instance;
	int err = ME_ERRNO_SUCCESS;
	unsigned long state;

	PDEBUG("executed.\n");

	instance = (me0900_do_subdevice_t *) subdevice;

	ME_SUBDEVICE_ENTER;

	spin_lock(&instance->subdevice_lock);
	switch (flags) {
	case ME_IO_SINGLE_TYPE_DIO_BIT:
		if ((channel >= 0) && (channel < 8)) {
			state = inb(instance->port_reg);
			state =
			    (!value) ? (state | (0x1 << channel)) : (state &
								     ~(0x1 <<
								       channel));
			outb(state, instance->port_reg);
		} else {
			PERROR("Invalid bit number specified.\n");
			err = ME_ERRNO_INVALID_CHANNEL;
		}
		break;

	case ME_IO_SINGLE_NO_FLAGS:
	case ME_IO_SINGLE_TYPE_DIO_BYTE:
		if (channel == 0) {
			outb(~(value), instance->port_reg);
		} else {
			PERROR("Invalid byte number specified.\n");
			err = ME_ERRNO_INVALID_CHANNEL;
		}
		break;

	default:
		PERROR("Invalid flags specified.\n");
		err = ME_ERRNO_INVALID_FLAGS;
	}
	spin_unlock(&instance->subdevice_lock);

	ME_SUBDEVICE_EXIT;

	return err;
}

static int me0900_do_query_number_channels(me_subdevice_t * subdevice,
					   int *number)
{
	PDEBUG("executed.\n");
	*number = 8;
	return ME_ERRNO_SUCCESS;
}

static int me0900_do_query_subdevice_type(me_subdevice_t * subdevice,
					  int *type, int *subtype)
{
	PDEBUG("executed.\n");
	*type = ME_TYPE_DO;
	*subtype = ME_SUBTYPE_SINGLE;
	return ME_ERRNO_SUCCESS;
}

static int me0900_do_query_subdevice_caps(me_subdevice_t * subdevice, int *caps)
{
	PDEBUG("executed.\n");
	*caps = 0;
	return ME_ERRNO_SUCCESS;
}

me0900_do_subdevice_t *me0900_do_constructor(uint32_t reg_base,
					     unsigned int do_idx)
{
	me0900_do_subdevice_t *subdevice;
	int err;

	PDEBUG("executed.\n");

	/* Allocate memory for subdevice instance */
	subdevice = kmalloc(sizeof(me0900_do_subdevice_t), GFP_KERNEL);

	if (!subdevice) {
		PERROR("Cannot get memory for subdevice instance.\n");
		return NULL;
	}

	memset(subdevice, 0, sizeof(me0900_do_subdevice_t));

	/* Initialize subdevice base class */
	err = me_subdevice_init(&subdevice->base);

	if (err) {
		PERROR("Cannot initialize subdevice base class instance.\n");
		kfree(subdevice);
		return NULL;
	}
	// Initialize spin locks.
	spin_lock_init(&subdevice->subdevice_lock);

	/* Save the subdevice index */
	subdevice->do_idx = do_idx;

	/* Initialize registers */
	if (do_idx == 0) {
		subdevice->ctrl_reg = reg_base + ME0900_CTRL_REG;
		subdevice->port_reg = reg_base + ME0900_PORT_A_REG;
		subdevice->enable_reg = reg_base + ME0900_WRITE_ENABLE_REG;
		subdevice->disable_reg = reg_base + ME0900_WRITE_DISABLE_REG;
	} else {
		subdevice->ctrl_reg = reg_base + ME0900_CTRL_REG;
		subdevice->port_reg = reg_base + ME0900_PORT_B_REG;
		subdevice->enable_reg = reg_base + ME0900_WRITE_ENABLE_REG;
		subdevice->disable_reg = reg_base + ME0900_WRITE_DISABLE_REG;
	}
#ifdef MEDEBUG_DEBUG_REG
	subdevice->reg_base = reg_base;
#endif

	/* Overload base class methods. */
	subdevice->base.me_subdevice_io_reset_subdevice =
	    me0900_do_io_reset_subdevice;
	subdevice->base.me_subdevice_io_single_config =
	    me0900_do_io_single_config;
	subdevice->base.me_subdevice_io_single_read = me0900_do_io_single_read;
	subdevice->base.me_subdevice_io_single_write =
	    me0900_do_io_single_write;
	subdevice->base.me_subdevice_query_number_channels =
	    me0900_do_query_number_channels;
	subdevice->base.me_subdevice_query_subdevice_type =
	    me0900_do_query_subdevice_type;
	subdevice->base.me_subdevice_query_subdevice_caps =
	    me0900_do_query_subdevice_caps;

	return subdevice;
}
