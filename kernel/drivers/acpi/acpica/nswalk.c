


#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nswalk")

struct acpi_namespace_node *acpi_ns_get_next_node(acpi_object_type type, struct acpi_namespace_node
						  *parent_node, struct acpi_namespace_node
						  *child_node)
{
	struct acpi_namespace_node *next_node = NULL;

	ACPI_FUNCTION_ENTRY();

	if (!child_node) {

		/* It's really the parent's _scope_ that we want */

		next_node = parent_node->child;
	}

	else {
		/* Start search at the NEXT node */

		next_node = acpi_ns_get_next_valid_node(child_node);
	}

	/* If any type is OK, we are done */

	if (type == ACPI_TYPE_ANY) {

		/* next_node is NULL if we are at the end-of-list */

		return (next_node);
	}

	/* Must search for the node -- but within this scope only */

	while (next_node) {

		/* If type matches, we are done */

		if (next_node->type == type) {
			return (next_node);
		}

		/* Otherwise, move on to the next node */

		next_node = acpi_ns_get_next_valid_node(next_node);
	}

	/* Not found */

	return (NULL);
}


acpi_status
acpi_ns_walk_namespace(acpi_object_type type,
		       acpi_handle start_node,
		       u32 max_depth,
		       u32 flags,
		       acpi_walk_callback user_function,
		       void *context, void **return_value)
{
	acpi_status status;
	acpi_status mutex_status;
	struct acpi_namespace_node *child_node;
	struct acpi_namespace_node *parent_node;
	acpi_object_type child_type;
	u32 level;

	ACPI_FUNCTION_TRACE(ns_walk_namespace);

	/* Special case for the namespace Root Node */

	if (start_node == ACPI_ROOT_OBJECT) {
		start_node = acpi_gbl_root_node;
	}

	/* Null child means "get first node" */

	parent_node = start_node;
	child_node = NULL;
	child_type = ACPI_TYPE_ANY;
	level = 1;

	/*
	 * Traverse the tree of nodes until we bubble back up to where we
	 * started. When Level is zero, the loop is done because we have
	 * bubbled up to (and passed) the original parent handle (start_entry)
	 */
	while (level > 0) {

		/* Get the next node in this scope.  Null if not found */

		status = AE_OK;
		child_node =
		    acpi_ns_get_next_node(ACPI_TYPE_ANY, parent_node,
					  child_node);
		if (child_node) {

			/* Found next child, get the type if we are not searching for ANY */

			if (type != ACPI_TYPE_ANY) {
				child_type = child_node->type;
			}

			/*
			 * Ignore all temporary namespace nodes (created during control
			 * method execution) unless told otherwise. These temporary nodes
			 * can cause a race condition because they can be deleted during the
			 * execution of the user function (if the namespace is unlocked before
			 * invocation of the user function.) Only the debugger namespace dump
			 * will examine the temporary nodes.
			 */
			if ((child_node->flags & ANOBJ_TEMPORARY) &&
			    !(flags & ACPI_NS_WALK_TEMP_NODES)) {
				status = AE_CTRL_DEPTH;
			}

			/* Type must match requested type */

			else if (child_type == type) {
				/*
				 * Found a matching node, invoke the user callback function.
				 * Unlock the namespace if flag is set.
				 */
				if (flags & ACPI_NS_WALK_UNLOCK) {
					mutex_status =
					    acpi_ut_release_mutex
					    (ACPI_MTX_NAMESPACE);
					if (ACPI_FAILURE(mutex_status)) {
						return_ACPI_STATUS
						    (mutex_status);
					}
				}

				status =
				    user_function(child_node, level, context,
						  return_value);

				if (flags & ACPI_NS_WALK_UNLOCK) {
					mutex_status =
					    acpi_ut_acquire_mutex
					    (ACPI_MTX_NAMESPACE);
					if (ACPI_FAILURE(mutex_status)) {
						return_ACPI_STATUS
						    (mutex_status);
					}
				}

				switch (status) {
				case AE_OK:
				case AE_CTRL_DEPTH:

					/* Just keep going */
					break;

				case AE_CTRL_TERMINATE:

					/* Exit now, with OK status */

					return_ACPI_STATUS(AE_OK);

				default:

					/* All others are valid exceptions */

					return_ACPI_STATUS(status);
				}
			}

			/*
			 * Depth first search: Attempt to go down another level in the
			 * namespace if we are allowed to.  Don't go any further if we have
			 * reached the caller specified maximum depth or if the user
			 * function has specified that the maximum depth has been reached.
			 */
			if ((level < max_depth) && (status != AE_CTRL_DEPTH)) {
				if (acpi_ns_get_next_node
				    (ACPI_TYPE_ANY, child_node, NULL)) {

					/* There is at least one child of this node, visit it */

					level++;
					parent_node = child_node;
					child_node = NULL;
				}
			}
		} else {
			/*
			 * No more children of this node (acpi_ns_get_next_node failed), go
			 * back upwards in the namespace tree to the node's parent.
			 */
			level--;
			child_node = parent_node;
			parent_node = acpi_ns_get_parent_node(parent_node);
		}
	}

	/* Complete walk, not terminated by user function */

	return_ACPI_STATUS(AE_OK);
}
