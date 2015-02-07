


#include <acpi/acpi.h>
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"
#include "actables.h"

#define _COMPONENT          ACPI_EVENTS
ACPI_MODULE_NAME("evxfevnt")

/* Local prototypes */
acpi_status
acpi_ev_get_gpe_device(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
		       struct acpi_gpe_block_info *gpe_block, void *context);


acpi_status acpi_enable(void)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(acpi_enable);

	/* ACPI tables must be present */

	if (!acpi_tb_tables_loaded()) {
		return_ACPI_STATUS(AE_NO_ACPI_TABLES);
	}

	/* Check current mode */

	if (acpi_hw_get_mode() == ACPI_SYS_MODE_ACPI) {
		ACPI_DEBUG_PRINT((ACPI_DB_INIT,
				  "System is already in ACPI mode\n"));
	} else {
		/* Transition to ACPI mode */

		status = acpi_hw_set_mode(ACPI_SYS_MODE_ACPI);
		if (ACPI_FAILURE(status)) {
			ACPI_ERROR((AE_INFO,
				    "Could not transition to ACPI mode"));
			return_ACPI_STATUS(status);
		}

		ACPI_DEBUG_PRINT((ACPI_DB_INIT,
				  "Transition to ACPI mode successful\n"));
	}

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_enable)

acpi_status acpi_disable(void)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(acpi_disable);

	if (acpi_hw_get_mode() == ACPI_SYS_MODE_LEGACY) {
		ACPI_DEBUG_PRINT((ACPI_DB_INIT,
				  "System is already in legacy (non-ACPI) mode\n"));
	} else {
		/* Transition to LEGACY mode */

		status = acpi_hw_set_mode(ACPI_SYS_MODE_LEGACY);

		if (ACPI_FAILURE(status)) {
			ACPI_ERROR((AE_INFO,
				    "Could not exit ACPI mode to legacy mode"));
			return_ACPI_STATUS(status);
		}

		ACPI_DEBUG_PRINT((ACPI_DB_INIT, "ACPI mode disabled\n"));
	}

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_disable)

acpi_status acpi_enable_event(u32 event, u32 flags)
{
	acpi_status status = AE_OK;
	u32 value;

	ACPI_FUNCTION_TRACE(acpi_enable_event);

	/* Decode the Fixed Event */

	if (event > ACPI_EVENT_MAX) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Enable the requested fixed event (by writing a one to the enable
	 * register bit)
	 */
	status =
	    acpi_set_register(acpi_gbl_fixed_event_info[event].
			      enable_register_id, 1);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Make sure that the hardware responded */

	status =
	    acpi_get_register(acpi_gbl_fixed_event_info[event].
			      enable_register_id, &value);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	if (value != 1) {
		ACPI_ERROR((AE_INFO,
			    "Could not enable %s event",
			    acpi_ut_get_event_name(event)));
		return_ACPI_STATUS(AE_NO_HARDWARE_RESPONSE);
	}

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_enable_event)

acpi_status acpi_set_gpe_type(acpi_handle gpe_device, u32 gpe_number, u8 type)
{
	acpi_status status = AE_OK;
	struct acpi_gpe_event_info *gpe_event_info;

	ACPI_FUNCTION_TRACE(acpi_set_gpe_type);

	/* Ensure that we have a valid GPE number */

	gpe_event_info = acpi_ev_get_gpe_event_info(gpe_device, gpe_number);
	if (!gpe_event_info) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	if ((gpe_event_info->flags & ACPI_GPE_TYPE_MASK) == type) {
		return_ACPI_STATUS(AE_OK);
	}

	/* Set the new type (will disable GPE if currently enabled) */

	status = acpi_ev_set_gpe_type(gpe_event_info, type);

      unlock_and_exit:
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_set_gpe_type)

acpi_status acpi_enable_gpe(acpi_handle gpe_device, u32 gpe_number)
{
	acpi_status status = AE_OK;
	acpi_cpu_flags flags;
	struct acpi_gpe_event_info *gpe_event_info;

	ACPI_FUNCTION_TRACE(acpi_enable_gpe);

	flags = acpi_os_acquire_lock(acpi_gbl_gpe_lock);

	/* Ensure that we have a valid GPE number */

	gpe_event_info = acpi_ev_get_gpe_event_info(gpe_device, gpe_number);
	if (!gpe_event_info) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/* Perform the enable */

	status = acpi_ev_enable_gpe(gpe_event_info, TRUE);

      unlock_and_exit:
	acpi_os_release_lock(acpi_gbl_gpe_lock, flags);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_enable_gpe)

acpi_status acpi_disable_gpe(acpi_handle gpe_device, u32 gpe_number)
{
	acpi_status status = AE_OK;
	acpi_cpu_flags flags;
	struct acpi_gpe_event_info *gpe_event_info;

	ACPI_FUNCTION_TRACE(acpi_disable_gpe);

	flags = acpi_os_acquire_lock(acpi_gbl_gpe_lock);
	/* Ensure that we have a valid GPE number */

	gpe_event_info = acpi_ev_get_gpe_event_info(gpe_device, gpe_number);
	if (!gpe_event_info) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	status = acpi_ev_disable_gpe(gpe_event_info);

unlock_and_exit:
	acpi_os_release_lock(acpi_gbl_gpe_lock, flags);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_disable_gpe)

acpi_status acpi_disable_event(u32 event, u32 flags)
{
	acpi_status status = AE_OK;
	u32 value;

	ACPI_FUNCTION_TRACE(acpi_disable_event);

	/* Decode the Fixed Event */

	if (event > ACPI_EVENT_MAX) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Disable the requested fixed event (by writing a zero to the enable
	 * register bit)
	 */
	status =
	    acpi_set_register(acpi_gbl_fixed_event_info[event].
			      enable_register_id, 0);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status =
	    acpi_get_register(acpi_gbl_fixed_event_info[event].
			      enable_register_id, &value);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	if (value != 0) {
		ACPI_ERROR((AE_INFO,
			    "Could not disable %s events",
			    acpi_ut_get_event_name(event)));
		return_ACPI_STATUS(AE_NO_HARDWARE_RESPONSE);
	}

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_disable_event)

acpi_status acpi_clear_event(u32 event)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(acpi_clear_event);

	/* Decode the Fixed Event */

	if (event > ACPI_EVENT_MAX) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Clear the requested fixed event (By writing a one to the status
	 * register bit)
	 */
	status =
	    acpi_set_register(acpi_gbl_fixed_event_info[event].
			      status_register_id, 1);

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_clear_event)

acpi_status acpi_clear_gpe(acpi_handle gpe_device, u32 gpe_number, u32 flags)
{
	acpi_status status = AE_OK;
	struct acpi_gpe_event_info *gpe_event_info;

	ACPI_FUNCTION_TRACE(acpi_clear_gpe);

	/* Use semaphore lock if not executing at interrupt level */

	if (flags & ACPI_NOT_ISR) {
		status = acpi_ut_acquire_mutex(ACPI_MTX_EVENTS);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	}

	/* Ensure that we have a valid GPE number */

	gpe_event_info = acpi_ev_get_gpe_event_info(gpe_device, gpe_number);
	if (!gpe_event_info) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	status = acpi_hw_clear_gpe(gpe_event_info);

      unlock_and_exit:
	if (flags & ACPI_NOT_ISR) {
		(void)acpi_ut_release_mutex(ACPI_MTX_EVENTS);
	}
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_clear_gpe)
acpi_status acpi_get_event_status(u32 event, acpi_event_status * event_status)
{
	acpi_status status = AE_OK;
	u32 value;

	ACPI_FUNCTION_TRACE(acpi_get_event_status);

	if (!event_status) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Decode the Fixed Event */

	if (event > ACPI_EVENT_MAX) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Get the status of the requested fixed event */

	status =
	    acpi_get_register(acpi_gbl_fixed_event_info[event].
			      enable_register_id, &value);
	if (ACPI_FAILURE(status))
		return_ACPI_STATUS(status);

	*event_status = value;

	status =
	    acpi_get_register(acpi_gbl_fixed_event_info[event].
			      status_register_id, &value);
	if (ACPI_FAILURE(status))
		return_ACPI_STATUS(status);

	if (value)
		*event_status |= ACPI_EVENT_FLAG_SET;

	if (acpi_gbl_fixed_event_handlers[event].handler)
		*event_status |= ACPI_EVENT_FLAG_HANDLE;

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_event_status)

acpi_status
acpi_get_gpe_status(acpi_handle gpe_device,
		    u32 gpe_number, u32 flags, acpi_event_status * event_status)
{
	acpi_status status = AE_OK;
	struct acpi_gpe_event_info *gpe_event_info;

	ACPI_FUNCTION_TRACE(acpi_get_gpe_status);

	/* Use semaphore lock if not executing at interrupt level */

	if (flags & ACPI_NOT_ISR) {
		status = acpi_ut_acquire_mutex(ACPI_MTX_EVENTS);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	}

	/* Ensure that we have a valid GPE number */

	gpe_event_info = acpi_ev_get_gpe_event_info(gpe_device, gpe_number);
	if (!gpe_event_info) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/* Obtain status on the requested GPE number */

	status = acpi_hw_get_gpe_status(gpe_event_info, event_status);

	if (gpe_event_info->flags & ACPI_GPE_DISPATCH_MASK)
		*event_status |= ACPI_EVENT_FLAG_HANDLE;

      unlock_and_exit:
	if (flags & ACPI_NOT_ISR) {
		(void)acpi_ut_release_mutex(ACPI_MTX_EVENTS);
	}
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_gpe_status)
acpi_status
acpi_install_gpe_block(acpi_handle gpe_device,
		       struct acpi_generic_address *gpe_block_address,
		       u32 register_count, u32 interrupt_number)
{
	acpi_status status;
	union acpi_operand_object *obj_desc;
	struct acpi_namespace_node *node;
	struct acpi_gpe_block_info *gpe_block;

	ACPI_FUNCTION_TRACE(acpi_install_gpe_block);

	if ((!gpe_device) || (!gpe_block_address) || (!register_count)) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	node = acpi_ns_map_handle_to_node(gpe_device);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/*
	 * For user-installed GPE Block Devices, the gpe_block_base_number
	 * is always zero
	 */
	status =
	    acpi_ev_create_gpe_block(node, gpe_block_address, register_count, 0,
				     interrupt_number, &gpe_block);
	if (ACPI_FAILURE(status)) {
		goto unlock_and_exit;
	}

	/* Run the _PRW methods and enable the GPEs */

	status = acpi_ev_initialize_gpe_block(node, gpe_block);
	if (ACPI_FAILURE(status)) {
		goto unlock_and_exit;
	}

	/* Get the device_object attached to the node */

	obj_desc = acpi_ns_get_attached_object(node);
	if (!obj_desc) {

		/* No object, create a new one */

		obj_desc = acpi_ut_create_internal_object(ACPI_TYPE_DEVICE);
		if (!obj_desc) {
			status = AE_NO_MEMORY;
			goto unlock_and_exit;
		}

		status =
		    acpi_ns_attach_object(node, obj_desc, ACPI_TYPE_DEVICE);

		/* Remove local reference to the object */

		acpi_ut_remove_reference(obj_desc);

		if (ACPI_FAILURE(status)) {
			goto unlock_and_exit;
		}
	}

	/* Install the GPE block in the device_object */

	obj_desc->device.gpe_block = gpe_block;

      unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_install_gpe_block)

acpi_status acpi_remove_gpe_block(acpi_handle gpe_device)
{
	union acpi_operand_object *obj_desc;
	acpi_status status;
	struct acpi_namespace_node *node;

	ACPI_FUNCTION_TRACE(acpi_remove_gpe_block);

	if (!gpe_device) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	node = acpi_ns_map_handle_to_node(gpe_device);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/* Get the device_object attached to the node */

	obj_desc = acpi_ns_get_attached_object(node);
	if (!obj_desc || !obj_desc->device.gpe_block) {
		return_ACPI_STATUS(AE_NULL_OBJECT);
	}

	/* Delete the GPE block (but not the device_object) */

	status = acpi_ev_delete_gpe_block(obj_desc->device.gpe_block);
	if (ACPI_SUCCESS(status)) {
		obj_desc->device.gpe_block = NULL;
	}

      unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_remove_gpe_block)

acpi_status
acpi_get_gpe_device(u32 index, acpi_handle *gpe_device)
{
	struct acpi_gpe_device_info info;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_get_gpe_device);

	if (!gpe_device) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (index >= acpi_current_gpe_count) {
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	/* Setup and walk the GPE list */

	info.index = index;
	info.status = AE_NOT_EXIST;
	info.gpe_device = NULL;
	info.next_block_base_index = 0;

	status = acpi_ev_walk_gpe_list(acpi_ev_get_gpe_device, &info);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	*gpe_device = info.gpe_device;
	return_ACPI_STATUS(info.status);
}

ACPI_EXPORT_SYMBOL(acpi_get_gpe_device)

acpi_status
acpi_ev_get_gpe_device(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
		       struct acpi_gpe_block_info *gpe_block, void *context)
{
	struct acpi_gpe_device_info *info = context;

	/* Increment Index by the number of GPEs in this block */

	info->next_block_base_index +=
	    (gpe_block->register_count * ACPI_GPE_REGISTER_WIDTH);

	if (info->index < info->next_block_base_index) {
		/*
		 * The GPE index is within this block, get the node. Leave the node
		 * NULL for the FADT-defined GPEs
		 */
		if ((gpe_block->node)->type == ACPI_TYPE_DEVICE) {
			info->gpe_device = gpe_block->node;
		}

		info->status = AE_OK;
		return (AE_CTRL_END);
	}

	return (AE_OK);
}


acpi_status acpi_disable_all_gpes(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_disable_all_gpes);

	status = acpi_ut_acquire_mutex(ACPI_MTX_EVENTS);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_hw_disable_all_gpes();
	(void)acpi_ut_release_mutex(ACPI_MTX_EVENTS);

	return_ACPI_STATUS(status);
}


acpi_status acpi_enable_all_runtime_gpes(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_enable_all_runtime_gpes);

	status = acpi_ut_acquire_mutex(ACPI_MTX_EVENTS);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_hw_enable_all_runtime_gpes();
	(void)acpi_ut_release_mutex(ACPI_MTX_EVENTS);

	return_ACPI_STATUS(status);
}
