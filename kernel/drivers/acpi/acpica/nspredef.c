


#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acpredef.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nspredef")

/* Local prototypes */
static acpi_status
acpi_ns_check_package(char *pathname,
		      union acpi_operand_object **return_object_ptr,
		      const union acpi_predefined_info *predefined);

static acpi_status
acpi_ns_check_package_elements(char *pathname,
			       union acpi_operand_object **elements,
			       u8 type1, u32 count1, u8 type2, u32 count2);

static acpi_status
acpi_ns_check_object_type(char *pathname,
			  union acpi_operand_object **return_object_ptr,
			  u32 expected_btypes, u32 package_index);

static acpi_status
acpi_ns_check_reference(char *pathname,
			union acpi_operand_object *return_object);

static acpi_status
acpi_ns_repair_object(u32 expected_btypes,
		      u32 package_index,
		      union acpi_operand_object **return_object_ptr);

static const char *acpi_rtype_names[] = {
	"/Integer",
	"/String",
	"/Buffer",
	"/Package",
	"/Reference",
};

#define ACPI_NOT_PACKAGE    ACPI_UINT32_MAX


acpi_status
acpi_ns_check_predefined_names(struct acpi_namespace_node *node,
			       u32 user_param_count,
			       acpi_status return_status,
			       union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	acpi_status status = AE_OK;
	const union acpi_predefined_info *predefined;
	char *pathname;

	/* Match the name for this method/object against the predefined list */

	predefined = acpi_ns_check_for_predefined_name(node);

	/* Get the full pathname to the object, for use in error messages */

	pathname = acpi_ns_get_external_pathname(node);
	if (!pathname) {
		pathname = ACPI_CAST_PTR(char, predefined->info.name);
	}

	/*
	 * Check that the parameter count for this method matches the ASL
	 * definition. For predefined names, ensure that both the caller and
	 * the method itself are in accordance with the ACPI specification.
	 */
	acpi_ns_check_parameter_count(pathname, node, user_param_count,
				      predefined);

	/* If not a predefined name, we cannot validate the return object */

	if (!predefined) {
		goto exit;
	}

	/* If the method failed, we cannot validate the return object */

	if ((return_status != AE_OK) && (return_status != AE_CTRL_RETURN_VALUE)) {
		goto exit;
	}

	/*
	 * Only validate the return value on the first successful evaluation of
	 * the method. This ensures that any warnings will only be emitted during
	 * the very first evaluation of the method/object.
	 */
	if (node->flags & ANOBJ_EVALUATED) {
		goto exit;
	}

	/* Mark the node as having been successfully evaluated */

	node->flags |= ANOBJ_EVALUATED;

	/*
	 * If there is no return value, check if we require a return value for
	 * this predefined name. Either one return value is expected, or none,
	 * for both methods and other objects.
	 *
	 * Exit now if there is no return object. Warning if one was expected.
	 */
	if (!return_object) {
		if ((predefined->info.expected_btypes) &&
		    (!(predefined->info.expected_btypes & ACPI_RTYPE_NONE))) {
			ACPI_ERROR((AE_INFO,
				    "%s: Missing expected return value",
				    pathname));

			status = AE_AML_NO_RETURN_VALUE;
		}
		goto exit;
	}

	/*
	 * We have a return value, but if one wasn't expected, just exit, this is
	 * not a problem
	 *
	 * For example, if the "Implicit Return" feature is enabled, methods will
	 * always return a value
	 */
	if (!predefined->info.expected_btypes) {
		goto exit;
	}

	/*
	 * Check that the type of the return object is what is expected for
	 * this predefined name
	 */
	status = acpi_ns_check_object_type(pathname, return_object_ptr,
					   predefined->info.expected_btypes,
					   ACPI_NOT_PACKAGE);
	if (ACPI_FAILURE(status)) {
		goto exit;
	}

	/* For returned Package objects, check the type of all sub-objects */

	if (ACPI_GET_OBJECT_TYPE(return_object) == ACPI_TYPE_PACKAGE) {
		status =
		    acpi_ns_check_package(pathname, return_object_ptr,
					  predefined);
	}

      exit:
	if (pathname != predefined->info.name) {
		ACPI_FREE(pathname);
	}

	return (status);
}


void
acpi_ns_check_parameter_count(char *pathname,
			      struct acpi_namespace_node *node,
			      u32 user_param_count,
			      const union acpi_predefined_info *predefined)
{
	u32 param_count;
	u32 required_params_current;
	u32 required_params_old;

	/* Methods have 0-7 parameters. All other types have zero. */

	param_count = 0;
	if (node->type == ACPI_TYPE_METHOD) {
		param_count = node->object->method.param_count;
	}

	/* Argument count check for non-predefined methods/objects */

	if (!predefined) {
		/*
		 * Warning if too few or too many arguments have been passed by the
		 * caller. An incorrect number of arguments may not cause the method
		 * to fail. However, the method will fail if there are too few
		 * arguments and the method attempts to use one of the missing ones.
		 */
		if (user_param_count < param_count) {
			ACPI_WARNING((AE_INFO,
				      "%s: Insufficient arguments - needs %d, found %d",
				      pathname, param_count, user_param_count));
		} else if (user_param_count > param_count) {
			ACPI_WARNING((AE_INFO,
				      "%s: Excess arguments - needs %d, found %d",
				      pathname, param_count, user_param_count));
		}
		return;
	}

	/* Allow two different legal argument counts (_SCP, etc.) */

	required_params_current = predefined->info.param_count & 0x0F;
	required_params_old = predefined->info.param_count >> 4;

	if (user_param_count != ACPI_UINT32_MAX) {

		/* Validate the user-supplied parameter count */

		if ((user_param_count != required_params_current) &&
		    (user_param_count != required_params_old)) {
			ACPI_WARNING((AE_INFO,
				      "%s: Parameter count mismatch - caller passed %d, ACPI requires %d",
				      pathname, user_param_count,
				      required_params_current));
		}
	}

	/*
	 * Only validate the argument count on the first successful evaluation of
	 * the method. This ensures that any warnings will only be emitted during
	 * the very first evaluation of the method/object.
	 */
	if (node->flags & ANOBJ_EVALUATED) {
		return;
	}

	/*
	 * Check that the ASL-defined parameter count is what is expected for
	 * this predefined name.
	 */
	if ((param_count != required_params_current) &&
	    (param_count != required_params_old)) {
		ACPI_WARNING((AE_INFO,
			      "%s: Parameter count mismatch - ASL declared %d, ACPI requires %d",
			      pathname, param_count, required_params_current));
	}
}


const union acpi_predefined_info *acpi_ns_check_for_predefined_name(struct
								    acpi_namespace_node
								    *node)
{
	const union acpi_predefined_info *this_name;

	/* Quick check for a predefined name, first character must be underscore */

	if (node->name.ascii[0] != '_') {
		return (NULL);
	}

	/* Search info table for a predefined method/object name */

	this_name = predefined_names;
	while (this_name->info.name[0]) {
		if (ACPI_COMPARE_NAME(node->name.ascii, this_name->info.name)) {

			/* Return pointer to this table entry */

			return (this_name);
		}

		/*
		 * Skip next entry in the table if this name returns a Package
		 * (next entry contains the package info)
		 */
		if (this_name->info.expected_btypes & ACPI_RTYPE_PACKAGE) {
			this_name++;
		}

		this_name++;
	}

	return (NULL);
}


static acpi_status
acpi_ns_check_package(char *pathname,
		      union acpi_operand_object **return_object_ptr,
		      const union acpi_predefined_info *predefined)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	const union acpi_predefined_info *package;
	union acpi_operand_object *sub_package;
	union acpi_operand_object **elements;
	union acpi_operand_object **sub_elements;
	acpi_status status;
	u32 expected_count;
	u32 count;
	u32 i;
	u32 j;

	ACPI_FUNCTION_NAME(ns_check_package);

	/* The package info for this name is in the next table entry */

	package = predefined + 1;

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
			  "%s Validating return Package of Type %X, Count %X\n",
			  pathname, package->ret_info.type,
			  return_object->package.count));

	/* Extract package count and elements array */

	elements = return_object->package.elements;
	count = return_object->package.count;

	/* The package must have at least one element, else invalid */

	if (!count) {
		ACPI_WARNING((AE_INFO,
			      "%s: Return Package has no elements (empty)",
			      pathname));

		return (AE_AML_OPERAND_VALUE);
	}

	/*
	 * Decode the type of the expected package contents
	 *
	 * PTYPE1 packages contain no subpackages
	 * PTYPE2 packages contain sub-packages
	 */
	switch (package->ret_info.type) {
	case ACPI_PTYPE1_FIXED:

		/*
		 * The package count is fixed and there are no sub-packages
		 *
		 * If package is too small, exit.
		 * If package is larger than expected, issue warning but continue
		 */
		expected_count =
		    package->ret_info.count1 + package->ret_info.count2;
		if (count < expected_count) {
			goto package_too_small;
		} else if (count > expected_count) {
			ACPI_WARNING((AE_INFO,
				      "%s: Return Package is larger than needed - "
				      "found %u, expected %u", pathname, count,
				      expected_count));
		}

		/* Validate all elements of the returned package */

		status = acpi_ns_check_package_elements(pathname, elements,
							package->ret_info.
							object_type1,
							package->ret_info.
							count1,
							package->ret_info.
							object_type2,
							package->ret_info.
							count2);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
		break;

	case ACPI_PTYPE1_VAR:

		/*
		 * The package count is variable, there are no sub-packages, and all
		 * elements must be of the same type
		 */
		for (i = 0; i < count; i++) {
			status = acpi_ns_check_object_type(pathname, elements,
							   package->ret_info.
							   object_type1, i);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
			elements++;
		}
		break;

	case ACPI_PTYPE1_OPTION:

		/*
		 * The package count is variable, there are no sub-packages. There are
		 * a fixed number of required elements, and a variable number of
		 * optional elements.
		 *
		 * Check if package is at least as large as the minimum required
		 */
		expected_count = package->ret_info3.count;
		if (count < expected_count) {
			goto package_too_small;
		}

		/* Variable number of sub-objects */

		for (i = 0; i < count; i++) {
			if (i < package->ret_info3.count) {

				/* These are the required package elements (0, 1, or 2) */

				status =
				    acpi_ns_check_object_type(pathname,
							      elements,
							      package->
							      ret_info3.
							      object_type[i],
							      i);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
			} else {
				/* These are the optional package elements */

				status =
				    acpi_ns_check_object_type(pathname,
							      elements,
							      package->
							      ret_info3.
							      tail_object_type,
							      i);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
			}
			elements++;
		}
		break;

	case ACPI_PTYPE2_PKG_COUNT:

		/* First element is the (Integer) count of sub-packages to follow */

		status = acpi_ns_check_object_type(pathname, elements,
						   ACPI_RTYPE_INTEGER, 0);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		/*
		 * Count cannot be larger than the parent package length, but allow it
		 * to be smaller. The >= accounts for the Integer above.
		 */
		expected_count = (u32) (*elements)->integer.value;
		if (expected_count >= count) {
			goto package_too_small;
		}

		count = expected_count;
		elements++;

		/* Now we can walk the sub-packages */

		/*lint -fallthrough */

	case ACPI_PTYPE2:
	case ACPI_PTYPE2_FIXED:
	case ACPI_PTYPE2_MIN:
	case ACPI_PTYPE2_COUNT:

		/*
		 * These types all return a single package that consists of a variable
		 * number of sub-packages
		 */
		for (i = 0; i < count; i++) {
			sub_package = *elements;
			sub_elements = sub_package->package.elements;

			/* Each sub-object must be of type Package */

			status =
			    acpi_ns_check_object_type(pathname, &sub_package,
						      ACPI_RTYPE_PACKAGE, i);
			if (ACPI_FAILURE(status)) {
				return (status);
			}

			/* Examine the different types of sub-packages */

			switch (package->ret_info.type) {
			case ACPI_PTYPE2:
			case ACPI_PTYPE2_PKG_COUNT:

				/* Each subpackage has a fixed number of elements */

				expected_count =
				    package->ret_info.count1 +
				    package->ret_info.count2;
				if (sub_package->package.count !=
				    expected_count) {
					count = sub_package->package.count;
					goto package_too_small;
				}

				status =
				    acpi_ns_check_package_elements(pathname,
								   sub_elements,
								   package->
								   ret_info.
								   object_type1,
								   package->
								   ret_info.
								   count1,
								   package->
								   ret_info.
								   object_type2,
								   package->
								   ret_info.
								   count2);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
				break;

			case ACPI_PTYPE2_FIXED:

				/* Each sub-package has a fixed length */

				expected_count = package->ret_info2.count;
				if (sub_package->package.count < expected_count) {
					count = sub_package->package.count;
					goto package_too_small;
				}

				/* Check the type of each sub-package element */

				for (j = 0; j < expected_count; j++) {
					status =
					    acpi_ns_check_object_type(pathname,
						&sub_elements[j],
						package->ret_info2.object_type[j], j);
					if (ACPI_FAILURE(status)) {
						return (status);
					}
				}
				break;

			case ACPI_PTYPE2_MIN:

				/* Each sub-package has a variable but minimum length */

				expected_count = package->ret_info.count1;
				if (sub_package->package.count < expected_count) {
					count = sub_package->package.count;
					goto package_too_small;
				}

				/* Check the type of each sub-package element */

				status =
				    acpi_ns_check_package_elements(pathname,
								   sub_elements,
								   package->
								   ret_info.
								   object_type1,
								   sub_package->
								   package.
								   count, 0, 0);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
				break;

			case ACPI_PTYPE2_COUNT:

				/* First element is the (Integer) count of elements to follow */

				status =
				    acpi_ns_check_object_type(pathname,
							      sub_elements,
							      ACPI_RTYPE_INTEGER,
							      0);
				if (ACPI_FAILURE(status)) {
					return (status);
				}

				/* Make sure package is large enough for the Count */

				expected_count =
				    (u32) (*sub_elements)->integer.value;
				if (sub_package->package.count < expected_count) {
					count = sub_package->package.count;
					goto package_too_small;
				}

				/* Check the type of each sub-package element */

				status =
				    acpi_ns_check_package_elements(pathname,
								   (sub_elements
								    + 1),
								   package->
								   ret_info.
								   object_type1,
								   (expected_count
								    - 1), 0, 0);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
				break;

			default:
				break;
			}

			elements++;
		}
		break;

	default:

		/* Should not get here if predefined info table is correct */

		ACPI_WARNING((AE_INFO,
			      "%s: Invalid internal return type in table entry: %X",
			      pathname, package->ret_info.type));

		return (AE_AML_INTERNAL);
	}

	return (AE_OK);

      package_too_small:

	/* Error exit for the case with an incorrect package count */

	ACPI_WARNING((AE_INFO, "%s: Return Package is too small - "
		      "found %u, expected %u", pathname, count,
		      expected_count));

	return (AE_AML_OPERAND_VALUE);
}


static acpi_status
acpi_ns_check_package_elements(char *pathname,
			       union acpi_operand_object **elements,
			       u8 type1, u32 count1, u8 type2, u32 count2)
{
	union acpi_operand_object **this_element = elements;
	acpi_status status;
	u32 i;

	/*
	 * Up to two groups of package elements are supported by the data
	 * structure. All elements in each group must be of the same type.
	 * The second group can have a count of zero.
	 */
	for (i = 0; i < count1; i++) {
		status = acpi_ns_check_object_type(pathname, this_element,
						   type1, i);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
		this_element++;
	}

	for (i = 0; i < count2; i++) {
		status = acpi_ns_check_object_type(pathname, this_element,
						   type2, (i + count1));
		if (ACPI_FAILURE(status)) {
			return (status);
		}
		this_element++;
	}

	return (AE_OK);
}


static acpi_status
acpi_ns_check_object_type(char *pathname,
			  union acpi_operand_object **return_object_ptr,
			  u32 expected_btypes, u32 package_index)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	acpi_status status = AE_OK;
	u32 return_btype;
	char type_buffer[48];	/* Room for 5 types */
	u32 this_rtype;
	u32 i;
	u32 j;

	/*
	 * If we get a NULL return_object here, it is a NULL package element,
	 * and this is always an error.
	 */
	if (!return_object) {
		goto type_error_exit;
	}

	/* A Namespace node should not get here, but make sure */

	if (ACPI_GET_DESCRIPTOR_TYPE(return_object) == ACPI_DESC_TYPE_NAMED) {
		ACPI_WARNING((AE_INFO,
			      "%s: Invalid return type - Found a Namespace node [%4.4s] type %s",
			      pathname, return_object->node.name.ascii,
			      acpi_ut_get_type_name(return_object->node.type)));
		return (AE_AML_OPERAND_TYPE);
	}

	/*
	 * Convert the object type (ACPI_TYPE_xxx) to a bitmapped object type.
	 * The bitmapped type allows multiple possible return types.
	 *
	 * Note, the cases below must handle all of the possible types returned
	 * from all of the predefined names (including elements of returned
	 * packages)
	 */
	switch (ACPI_GET_OBJECT_TYPE(return_object)) {
	case ACPI_TYPE_INTEGER:
		return_btype = ACPI_RTYPE_INTEGER;
		break;

	case ACPI_TYPE_BUFFER:
		return_btype = ACPI_RTYPE_BUFFER;
		break;

	case ACPI_TYPE_STRING:
		return_btype = ACPI_RTYPE_STRING;
		break;

	case ACPI_TYPE_PACKAGE:
		return_btype = ACPI_RTYPE_PACKAGE;
		break;

	case ACPI_TYPE_LOCAL_REFERENCE:
		return_btype = ACPI_RTYPE_REFERENCE;
		break;

	default:
		/* Not one of the supported objects, must be incorrect */

		goto type_error_exit;
	}

	/* Is the object one of the expected types? */

	if (!(return_btype & expected_btypes)) {

		/* Type mismatch -- attempt repair of the returned object */

		status = acpi_ns_repair_object(expected_btypes, package_index,
					       return_object_ptr);
		if (ACPI_SUCCESS(status)) {
			return (status);
		}
		goto type_error_exit;
	}

	/* For reference objects, check that the reference type is correct */

	if (ACPI_GET_OBJECT_TYPE(return_object) == ACPI_TYPE_LOCAL_REFERENCE) {
		status = acpi_ns_check_reference(pathname, return_object);
	}

	return (status);

      type_error_exit:

	/* Create a string with all expected types for this predefined object */

	j = 1;
	type_buffer[0] = 0;
	this_rtype = ACPI_RTYPE_INTEGER;

	for (i = 0; i < ACPI_NUM_RTYPES; i++) {

		/* If one of the expected types, concatenate the name of this type */

		if (expected_btypes & this_rtype) {
			ACPI_STRCAT(type_buffer, &acpi_rtype_names[i][j]);
			j = 0;	/* Use name separator from now on */
		}
		this_rtype <<= 1;	/* Next Rtype */
	}

	if (package_index == ACPI_NOT_PACKAGE) {
		ACPI_WARNING((AE_INFO,
			      "%s: Return type mismatch - found %s, expected %s",
			      pathname,
			      acpi_ut_get_object_type_name(return_object),
			      type_buffer));
	} else {
		ACPI_WARNING((AE_INFO,
			      "%s: Return Package type mismatch at index %u - "
			      "found %s, expected %s", pathname, package_index,
			      acpi_ut_get_object_type_name(return_object),
			      type_buffer));
	}

	return (AE_AML_OPERAND_TYPE);
}


static acpi_status
acpi_ns_check_reference(char *pathname,
			union acpi_operand_object *return_object)
{

	/*
	 * Check the reference object for the correct reference type (opcode).
	 * The only type of reference that can be converted to an union acpi_object is
	 * a reference to a named object (reference class: NAME)
	 */
	if (return_object->reference.class == ACPI_REFCLASS_NAME) {
		return (AE_OK);
	}

	ACPI_WARNING((AE_INFO,
		      "%s: Return type mismatch - unexpected reference object type [%s] %2.2X",
		      pathname, acpi_ut_get_reference_name(return_object),
		      return_object->reference.class));

	return (AE_AML_OPERAND_TYPE);
}


static acpi_status
acpi_ns_repair_object(u32 expected_btypes,
		      u32 package_index,
		      union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object *new_object;
	acpi_size length;

	switch (ACPI_GET_OBJECT_TYPE(return_object)) {
	case ACPI_TYPE_BUFFER:

		if (!(expected_btypes & ACPI_RTYPE_STRING)) {
			return (AE_AML_OPERAND_TYPE);
		}

		/*
		 * Have a Buffer, expected a String, convert. Use a to_string
		 * conversion, no transform performed on the buffer data. The best
		 * example of this is the _BIF method, where the string data from
		 * the battery is often (incorrectly) returned as buffer object(s).
		 */
		length = 0;
		while ((length < return_object->buffer.length) &&
		       (return_object->buffer.pointer[length])) {
			length++;
		}

		/* Allocate a new string object */

		new_object = acpi_ut_create_string_object(length);
		if (!new_object) {
			return (AE_NO_MEMORY);
		}

		/*
		 * Copy the raw buffer data with no transform. String is already NULL
		 * terminated at Length+1.
		 */
		ACPI_MEMCPY(new_object->string.pointer,
			    return_object->buffer.pointer, length);

		/* Install the new return object */

		acpi_ut_remove_reference(return_object);
		*return_object_ptr = new_object;

		/*
		 * If the object is a package element, we need to:
		 * 1. Decrement the reference count of the orignal object, it was
		 *    incremented when building the package
		 * 2. Increment the reference count of the new object, it will be
		 *    decremented when releasing the package
		 */
		if (package_index != ACPI_NOT_PACKAGE) {
			acpi_ut_remove_reference(return_object);
			acpi_ut_add_reference(new_object);
		}
		return (AE_OK);

	default:
		break;
	}

	return (AE_AML_OPERAND_TYPE);
}
