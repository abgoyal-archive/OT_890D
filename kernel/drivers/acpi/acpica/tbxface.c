


#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
ACPI_MODULE_NAME("tbxface")

/* Local prototypes */
static acpi_status acpi_tb_load_namespace(void);

static int no_auto_ssdt;


acpi_status acpi_allocate_root_table(u32 initial_table_count)
{

	acpi_gbl_root_table_list.size = initial_table_count;
	acpi_gbl_root_table_list.flags = ACPI_ROOT_ALLOW_RESIZE;

	return (acpi_tb_resize_root_table_list());
}


acpi_status __init
acpi_initialize_tables(struct acpi_table_desc * initial_table_array,
		       u32 initial_table_count, u8 allow_resize)
{
	acpi_physical_address rsdp_address;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_initialize_tables);

	/*
	 * Set up the Root Table Array
	 * Allocate the table array if requested
	 */
	if (!initial_table_array) {
		status = acpi_allocate_root_table(initial_table_count);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	} else {
		/* Root Table Array has been statically allocated by the host */

		ACPI_MEMSET(initial_table_array, 0,
			    (acpi_size) initial_table_count *
			    sizeof(struct acpi_table_desc));

		acpi_gbl_root_table_list.tables = initial_table_array;
		acpi_gbl_root_table_list.size = initial_table_count;
		acpi_gbl_root_table_list.flags = ACPI_ROOT_ORIGIN_UNKNOWN;
		if (allow_resize) {
			acpi_gbl_root_table_list.flags |=
			    ACPI_ROOT_ALLOW_RESIZE;
		}
	}

	/* Get the address of the RSDP */

	rsdp_address = acpi_os_get_root_pointer();
	if (!rsdp_address) {
		return_ACPI_STATUS(AE_NOT_FOUND);
	}

	/*
	 * Get the root table (RSDT or XSDT) and extract all entries to the local
	 * Root Table Array. This array contains the information of the RSDT/XSDT
	 * in a common, more useable format.
	 */
	status =
	    acpi_tb_parse_root_table(rsdp_address, ACPI_TABLE_ORIGIN_MAPPED);
	return_ACPI_STATUS(status);
}

acpi_status acpi_reallocate_root_table(void)
{
	struct acpi_table_desc *tables;
	acpi_size new_size;

	ACPI_FUNCTION_TRACE(acpi_reallocate_root_table);

	/*
	 * Only reallocate the root table if the host provided a static buffer
	 * for the table array in the call to acpi_initialize_tables.
	 */
	if (acpi_gbl_root_table_list.flags & ACPI_ROOT_ORIGIN_ALLOCATED) {
		return_ACPI_STATUS(AE_SUPPORT);
	}

	new_size = ((acpi_size) acpi_gbl_root_table_list.count +
		    ACPI_ROOT_TABLE_SIZE_INCREMENT) *
	    sizeof(struct acpi_table_desc);

	/* Create new array and copy the old array */

	tables = ACPI_ALLOCATE_ZEROED(new_size);
	if (!tables) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	ACPI_MEMCPY(tables, acpi_gbl_root_table_list.tables, new_size);

	acpi_gbl_root_table_list.size = acpi_gbl_root_table_list.count;
	acpi_gbl_root_table_list.tables = tables;
	acpi_gbl_root_table_list.flags =
	    ACPI_ROOT_ORIGIN_ALLOCATED | ACPI_ROOT_ALLOW_RESIZE;

	return_ACPI_STATUS(AE_OK);
}

acpi_status acpi_load_table(struct acpi_table_header *table_ptr)
{
	acpi_status status;
	u32 table_index;
	struct acpi_table_desc table_desc;

	if (!table_ptr)
		return AE_BAD_PARAMETER;

	ACPI_MEMSET(&table_desc, 0, sizeof(struct acpi_table_desc));
	table_desc.pointer = table_ptr;
	table_desc.length = table_ptr->length;
	table_desc.flags = ACPI_TABLE_ORIGIN_UNKNOWN;

	/*
	 * Install the new table into the local data structures
	 */
	status = acpi_tb_add_table(&table_desc, &table_index);
	if (ACPI_FAILURE(status)) {
		return status;
	}
	status = acpi_ns_load_table(table_index, acpi_gbl_root_node);
	return status;
}

ACPI_EXPORT_SYMBOL(acpi_load_table)

acpi_status
acpi_get_table_header(char *signature,
		      u32 instance, struct acpi_table_header *out_table_header)
{
       u32 i;
       u32 j;
	struct acpi_table_header *header;

	/* Parameter validation */

	if (!signature || !out_table_header) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * Walk the root table list
	 */
	for (i = 0, j = 0; i < acpi_gbl_root_table_list.count; i++) {
		if (!ACPI_COMPARE_NAME
		    (&(acpi_gbl_root_table_list.tables[i].signature),
		     signature)) {
			continue;
		}

		if (++j < instance) {
			continue;
		}

		if (!acpi_gbl_root_table_list.tables[i].pointer) {
			if ((acpi_gbl_root_table_list.tables[i].
			     flags & ACPI_TABLE_ORIGIN_MASK) ==
			    ACPI_TABLE_ORIGIN_MAPPED) {
				header =
				    acpi_os_map_memory(acpi_gbl_root_table_list.
						       tables[i].address,
						       sizeof(struct
							      acpi_table_header));
				if (!header) {
					return AE_NO_MEMORY;
				}
				ACPI_MEMCPY(out_table_header, header,
					    sizeof(struct acpi_table_header));
				acpi_os_unmap_memory(header,
						     sizeof(struct
							    acpi_table_header));
			} else {
				return AE_NOT_FOUND;
			}
		} else {
			ACPI_MEMCPY(out_table_header,
				    acpi_gbl_root_table_list.tables[i].pointer,
				    sizeof(struct acpi_table_header));
		}
		return (AE_OK);
	}

	return (AE_NOT_FOUND);
}

ACPI_EXPORT_SYMBOL(acpi_get_table_header)

acpi_status acpi_unload_table_id(acpi_owner_id id)
{
	int i;
	acpi_status status = AE_NOT_EXIST;

	ACPI_FUNCTION_TRACE(acpi_unload_table_id);

	/* Find table in the global table list */
	for (i = 0; i < acpi_gbl_root_table_list.count; ++i) {
		if (id != acpi_gbl_root_table_list.tables[i].owner_id) {
			continue;
		}
		/*
		 * Delete all namespace objects owned by this table. Note that these
		 * objects can appear anywhere in the namespace by virtue of the AML
		 * "Scope" operator. Thus, we need to track ownership by an ID, not
		 * simply a position within the hierarchy
		 */
		acpi_tb_delete_namespace_by_owner(i);
		status = acpi_tb_release_owner_id(i);
		acpi_tb_set_table_loaded_flag(i, FALSE);
		break;
	}
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_unload_table_id)

acpi_status
acpi_get_table(char *signature,
	       u32 instance, struct acpi_table_header **out_table)
{
       u32 i;
       u32 j;
	acpi_status status;

	/* Parameter validation */

	if (!signature || !out_table) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * Walk the root table list
	 */
	for (i = 0, j = 0; i < acpi_gbl_root_table_list.count; i++) {
		if (!ACPI_COMPARE_NAME
		    (&(acpi_gbl_root_table_list.tables[i].signature),
		     signature)) {
			continue;
		}

		if (++j < instance) {
			continue;
		}

		status =
		    acpi_tb_verify_table(&acpi_gbl_root_table_list.tables[i]);
		if (ACPI_SUCCESS(status)) {
			*out_table = acpi_gbl_root_table_list.tables[i].pointer;
		}

		if (!acpi_gbl_permanent_mmap) {
			acpi_gbl_root_table_list.tables[i].pointer = NULL;
		}

		return (status);
	}

	return (AE_NOT_FOUND);
}

ACPI_EXPORT_SYMBOL(acpi_get_table)

acpi_status
acpi_get_table_by_index(u32 table_index, struct acpi_table_header **table)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_get_table_by_index);

	/* Parameter validation */

	if (!table) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	(void)acpi_ut_acquire_mutex(ACPI_MTX_TABLES);

	/* Validate index */

	if (table_index >= acpi_gbl_root_table_list.count) {
		(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (!acpi_gbl_root_table_list.tables[table_index].pointer) {

		/* Table is not mapped, map it */

		status =
		    acpi_tb_verify_table(&acpi_gbl_root_table_list.
					 tables[table_index]);
		if (ACPI_FAILURE(status)) {
			(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
			return_ACPI_STATUS(status);
		}
	}

	*table = acpi_gbl_root_table_list.tables[table_index].pointer;
	(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_get_table_by_index)

static acpi_status acpi_tb_load_namespace(void)
{
	acpi_status status;
	struct acpi_table_header *table;
	u32 i;

	ACPI_FUNCTION_TRACE(tb_load_namespace);

	(void)acpi_ut_acquire_mutex(ACPI_MTX_TABLES);

	/*
	 * Load the namespace. The DSDT is required, but any SSDT and PSDT tables
	 * are optional.
	 */
	if (!acpi_gbl_root_table_list.count ||
	    !ACPI_COMPARE_NAME(&
			       (acpi_gbl_root_table_list.
				tables[ACPI_TABLE_INDEX_DSDT].signature),
			       ACPI_SIG_DSDT)
	    ||
	    ACPI_FAILURE(acpi_tb_verify_table
			 (&acpi_gbl_root_table_list.
			  tables[ACPI_TABLE_INDEX_DSDT]))) {
		status = AE_NO_ACPI_TABLES;
		goto unlock_and_exit;
	}

	/*
	 * Find DSDT table
	 */
	status =
	    acpi_os_table_override(acpi_gbl_root_table_list.
				   tables[ACPI_TABLE_INDEX_DSDT].pointer,
				   &table);
	if (ACPI_SUCCESS(status) && table) {
		/*
		 * DSDT table has been found
		 */
		acpi_tb_delete_table(&acpi_gbl_root_table_list.
				     tables[ACPI_TABLE_INDEX_DSDT]);
		acpi_gbl_root_table_list.tables[ACPI_TABLE_INDEX_DSDT].pointer =
		    table;
		acpi_gbl_root_table_list.tables[ACPI_TABLE_INDEX_DSDT].length =
		    table->length;
		acpi_gbl_root_table_list.tables[ACPI_TABLE_INDEX_DSDT].flags =
		    ACPI_TABLE_ORIGIN_UNKNOWN;

		ACPI_INFO((AE_INFO, "Table DSDT replaced by host OS"));
		acpi_tb_print_table_header(0, table);

		if (no_auto_ssdt == 0) {
			printk(KERN_WARNING "ACPI: DSDT override uses original SSDTs unless \"acpi_no_auto_ssdt\"\n");
		}
	}

	status =
	    acpi_tb_verify_table(&acpi_gbl_root_table_list.
				 tables[ACPI_TABLE_INDEX_DSDT]);
	if (ACPI_FAILURE(status)) {

		/* A valid DSDT is required */

		status = AE_NO_ACPI_TABLES;
		goto unlock_and_exit;
	}

	(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);

	/*
	 * Load and parse tables.
	 */
	status = acpi_ns_load_table(ACPI_TABLE_INDEX_DSDT, acpi_gbl_root_node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Load any SSDT or PSDT tables. Note: Loop leaves tables locked
	 */
	(void)acpi_ut_acquire_mutex(ACPI_MTX_TABLES);
	for (i = 0; i < acpi_gbl_root_table_list.count; ++i) {
		if ((!ACPI_COMPARE_NAME
		     (&(acpi_gbl_root_table_list.tables[i].signature),
		      ACPI_SIG_SSDT)
		     &&
		     !ACPI_COMPARE_NAME(&
					(acpi_gbl_root_table_list.tables[i].
					 signature), ACPI_SIG_PSDT))
		    ||
		    ACPI_FAILURE(acpi_tb_verify_table
				 (&acpi_gbl_root_table_list.tables[i]))) {
			continue;
		}

		if (no_auto_ssdt) {
			printk(KERN_WARNING "ACPI: SSDT ignored due to \"acpi_no_auto_ssdt\"\n");
			continue;
		}

		/* Ignore errors while loading tables, get as many as possible */

		(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
		(void)acpi_ns_load_table(i, acpi_gbl_root_node);
		(void)acpi_ut_acquire_mutex(ACPI_MTX_TABLES);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INIT, "ACPI Tables successfully acquired\n"));

      unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
	return_ACPI_STATUS(status);
}


acpi_status acpi_load_tables(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_load_tables);

	/*
	 * Load the namespace from the tables
	 */
	status = acpi_tb_load_namespace();
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status,
				"While loading namespace from ACPI tables"));
	}

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_load_tables)


acpi_status
acpi_install_table_handler(acpi_tbl_handler handler, void *context)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_install_table_handler);

	if (!handler) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_EVENTS);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Don't allow more than one handler */

	if (acpi_gbl_table_handler) {
		status = AE_ALREADY_EXISTS;
		goto cleanup;
	}

	/* Install the handler */

	acpi_gbl_table_handler = handler;
	acpi_gbl_table_handler_context = context;

      cleanup:
	(void)acpi_ut_release_mutex(ACPI_MTX_EVENTS);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_install_table_handler)

acpi_status acpi_remove_table_handler(acpi_tbl_handler handler)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_remove_table_handler);

	status = acpi_ut_acquire_mutex(ACPI_MTX_EVENTS);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Make sure that the installed handler is the same */

	if (!handler || handler != acpi_gbl_table_handler) {
		status = AE_BAD_PARAMETER;
		goto cleanup;
	}

	/* Remove the handler */

	acpi_gbl_table_handler = NULL;

      cleanup:
	(void)acpi_ut_release_mutex(ACPI_MTX_EVENTS);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_remove_table_handler)


static int __init acpi_no_auto_ssdt_setup(char *s) {

        printk(KERN_NOTICE "ACPI: SSDT auto-load disabled\n");

        no_auto_ssdt = 1;

        return 1;
}

__setup("acpi_no_auto_ssdt", acpi_no_auto_ssdt_setup);
