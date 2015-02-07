


#include <acpi/acpi.h>
#include "accommon.h"
#include "acparser.h"
#include "acinterp.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nseval")

acpi_status acpi_ns_evaluate(struct acpi_evaluate_info * info)
{
	acpi_status status;
	struct acpi_namespace_node *node;

	ACPI_FUNCTION_TRACE(ns_evaluate);

	if (!info) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Initialize the return value to an invalid object */

	info->return_object = NULL;
	info->param_count = 0;

	/*
	 * Get the actual namespace node for the target object. Handles these cases:
	 *
	 * 1) Null node, Pathname (absolute path)
	 * 2) Node, Pathname (path relative to Node)
	 * 3) Node, Null Pathname
	 */
	status = acpi_ns_get_node(info->prefix_node, info->pathname,
				  ACPI_NS_NO_UPSEARCH, &info->resolved_node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * For a method alias, we must grab the actual method node so that proper
	 * scoping context will be established before execution.
	 */
	if (acpi_ns_get_type(info->resolved_node) ==
	    ACPI_TYPE_LOCAL_METHOD_ALIAS) {
		info->resolved_node =
		    ACPI_CAST_PTR(struct acpi_namespace_node,
				  info->resolved_node->object);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES, "%s [%p] Value %p\n", info->pathname,
			  info->resolved_node,
			  acpi_ns_get_attached_object(info->resolved_node)));

	node = info->resolved_node;

	/*
	 * Two major cases here:
	 *
	 * 1) The object is a control method -- execute it
	 * 2) The object is not a method -- just return it's current value
	 */
	if (acpi_ns_get_type(info->resolved_node) == ACPI_TYPE_METHOD) {
		/*
		 * 1) Object is a control method - execute it
		 */

		/* Verify that there is a method object associated with this node */

		info->obj_desc =
		    acpi_ns_get_attached_object(info->resolved_node);
		if (!info->obj_desc) {
			ACPI_ERROR((AE_INFO,
				    "Control method has no attached sub-object"));
			return_ACPI_STATUS(AE_NULL_OBJECT);
		}

		/* Count the number of arguments being passed to the method */

		if (info->parameters) {
			while (info->parameters[info->param_count]) {
				if (info->param_count > ACPI_METHOD_MAX_ARG) {
					return_ACPI_STATUS(AE_LIMIT);
				}
				info->param_count++;
			}
		}


		ACPI_DUMP_PATHNAME(info->resolved_node, "Execute Method:",
				   ACPI_LV_INFO, _COMPONENT);

		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "Method at AML address %p Length %X\n",
				  info->obj_desc->method.aml_start + 1,
				  info->obj_desc->method.aml_length - 1));

		/*
		 * Any namespace deletion must acquire both the namespace and
		 * interpreter locks to ensure that no thread is using the portion of
		 * the namespace that is being deleted.
		 *
		 * Execute the method via the interpreter. The interpreter is locked
		 * here before calling into the AML parser
		 */
		acpi_ex_enter_interpreter();
		status = acpi_ps_execute_method(info);
		acpi_ex_exit_interpreter();
	} else {
		/*
		 * 2) Object is not a method, return its current value
		 *
		 * Disallow certain object types. For these, "evaluation" is undefined.
		 */
		switch (info->resolved_node->type) {
		case ACPI_TYPE_DEVICE:
		case ACPI_TYPE_EVENT:
		case ACPI_TYPE_MUTEX:
		case ACPI_TYPE_REGION:
		case ACPI_TYPE_THERMAL:
		case ACPI_TYPE_LOCAL_SCOPE:

			ACPI_ERROR((AE_INFO,
				    "[%4.4s] Evaluation of object type [%s] is not supported",
				    info->resolved_node->name.ascii,
				    acpi_ut_get_type_name(info->resolved_node->
							  type)));

			return_ACPI_STATUS(AE_TYPE);

		default:
			break;
		}

		/*
		 * Objects require additional resolution steps (e.g., the Node may be
		 * a field that must be read, etc.) -- we can't just grab the object
		 * out of the node.
		 *
		 * Use resolve_node_to_value() to get the associated value.
		 *
		 * NOTE: we can get away with passing in NULL for a walk state because
		 * resolved_node is guaranteed to not be a reference to either a method
		 * local or a method argument (because this interface is never called
		 * from a running method.)
		 *
		 * Even though we do not directly invoke the interpreter for object
		 * resolution, we must lock it because we could access an opregion.
		 * The opregion access code assumes that the interpreter is locked.
		 */
		acpi_ex_enter_interpreter();

		/* Function has a strange interface */

		status =
		    acpi_ex_resolve_node_to_value(&info->resolved_node, NULL);
		acpi_ex_exit_interpreter();

		/*
		 * If acpi_ex_resolve_node_to_value() succeeded, the return value was placed
		 * in resolved_node.
		 */
		if (ACPI_SUCCESS(status)) {
			status = AE_CTRL_RETURN_VALUE;
			info->return_object =
			    ACPI_CAST_PTR(union acpi_operand_object,
					  info->resolved_node);

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Returning object %p [%s]\n",
					  info->return_object,
					  acpi_ut_get_object_type_name(info->
								       return_object)));
		}
	}

	/*
	 * Check input argument count against the ASL-defined count for a method.
	 * Also check predefined names: argument count and return value against
	 * the ACPI specification. Some incorrect return value types are repaired.
	 */
	(void)acpi_ns_check_predefined_names(node, info->param_count,
		status, &info->return_object);

	/* Check if there is a return value that must be dealt with */

	if (status == AE_CTRL_RETURN_VALUE) {

		/* If caller does not want the return value, delete it */

		if (info->flags & ACPI_IGNORE_RETURN_VALUE) {
			acpi_ut_remove_reference(info->return_object);
			info->return_object = NULL;
		}

		/* Map AE_CTRL_RETURN_VALUE to AE_OK, we are done with it */

		status = AE_OK;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
			  "*** Completed evaluation of object %s ***\n",
			  info->pathname));

	/*
	 * Namespace was unlocked by the handling acpi_ns* function, so we
	 * just return
	 */
	return_ACPI_STATUS(status);
}
