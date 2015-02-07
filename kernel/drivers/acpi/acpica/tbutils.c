


#include <acpi/acpi.h>
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
ACPI_MODULE_NAME("tbutils")

/* Local prototypes */
static acpi_physical_address
acpi_tb_get_root_table_entry(u8 *table_entry, u32 table_entry_size);


static acpi_status
acpi_tb_check_xsdt(acpi_physical_address address)
{
	struct acpi_table_header *table;
	u32 length;
	u64 xsdt_entry_address;
	u8 *table_entry;
	u32 table_count;
	int i;

	table = acpi_os_map_memory(address, sizeof(struct acpi_table_header));
	if (!table)
		return AE_NO_MEMORY;

	length = table->length;
	acpi_os_unmap_memory(table, sizeof(struct acpi_table_header));
	if (length < sizeof(struct acpi_table_header))
		return AE_INVALID_TABLE_LENGTH;

	table = acpi_os_map_memory(address, length);
	if (!table)
		return AE_NO_MEMORY;

	/* Calculate the number of tables described in XSDT */
	table_count =
		(u32) ((table->length -
		sizeof(struct acpi_table_header)) / sizeof(u64));
	table_entry =
		ACPI_CAST_PTR(u8, table) + sizeof(struct acpi_table_header);
	for (i = 0; i < table_count; i++) {
		ACPI_MOVE_64_TO_64(&xsdt_entry_address, table_entry);
		if (!xsdt_entry_address) {
			/* XSDT has NULL entry */
			break;
		}
		table_entry += sizeof(u64);
	}
	acpi_os_unmap_memory(table, length);

	if (i < table_count)
		return AE_NULL_ENTRY;
	else
		return AE_OK;
}


acpi_status acpi_tb_initialize_facs(void)
{
	acpi_status status;

	status = acpi_get_table_by_index(ACPI_TABLE_INDEX_FACS,
					 ACPI_CAST_INDIRECT_PTR(struct
								acpi_table_header,
								&acpi_gbl_FACS));
	return status;
}


u8 acpi_tb_tables_loaded(void)
{

	if (acpi_gbl_root_table_list.count >= 3) {
		return (TRUE);
	}

	return (FALSE);
}


void
acpi_tb_print_table_header(acpi_physical_address address,
			   struct acpi_table_header *header)
{

	if (ACPI_COMPARE_NAME(header->signature, ACPI_SIG_FACS)) {

		/* FACS only has signature and length fields of common table header */

		ACPI_INFO((AE_INFO, "%4.4s %08lX, %04X",
			   header->signature, (unsigned long)address,
			   header->length));
	} else if (ACPI_COMPARE_NAME(header->signature, ACPI_SIG_RSDP)) {

		/* RSDP has no common fields */

		ACPI_INFO((AE_INFO, "RSDP %08lX, %04X (r%d %6.6s)",
			   (unsigned long)address,
			   (ACPI_CAST_PTR(struct acpi_table_rsdp, header)->
			    revision >
			    0) ? ACPI_CAST_PTR(struct acpi_table_rsdp,
					       header)->length : 20,
			   ACPI_CAST_PTR(struct acpi_table_rsdp,
					 header)->revision,
			   ACPI_CAST_PTR(struct acpi_table_rsdp,
					 header)->oem_id));
	} else {
		/* Standard ACPI table with full common header */

		ACPI_INFO((AE_INFO,
			   "%4.4s %08lX, %04X (r%d %6.6s %8.8s %8X %4.4s %8X)",
			   header->signature, (unsigned long)address,
			   header->length, header->revision, header->oem_id,
			   header->oem_table_id, header->oem_revision,
			   header->asl_compiler_id,
			   header->asl_compiler_revision));
	}
}


acpi_status acpi_tb_verify_checksum(struct acpi_table_header *table, u32 length)
{
	u8 checksum;

	/* Compute the checksum on the table */

	checksum = acpi_tb_checksum(ACPI_CAST_PTR(u8, table), length);

	/* Checksum ok? (should be zero) */

	if (checksum) {
		ACPI_WARNING((AE_INFO,
			      "Incorrect checksum in table [%4.4s] - %2.2X, should be %2.2X",
			      table->signature, table->checksum,
			      (u8) (table->checksum - checksum)));

#if (ACPI_CHECKSUM_ABORT)

		return (AE_BAD_CHECKSUM);
#endif
	}

	return (AE_OK);
}


u8 acpi_tb_checksum(u8 *buffer, u32 length)
{
	u8 sum = 0;
	u8 *end = buffer + length;

	while (buffer < end) {
		sum = (u8) (sum + *(buffer++));
	}

	return sum;
}


void
acpi_tb_install_table(acpi_physical_address address,
		      u8 flags, char *signature, u32 table_index)
{
	struct acpi_table_header *table;

	if (!address) {
		ACPI_ERROR((AE_INFO,
			    "Null physical address for ACPI table [%s]",
			    signature));
		return;
	}

	/* Map just the table header */

	table = acpi_os_map_memory(address, sizeof(struct acpi_table_header));
	if (!table) {
		return;
	}

	/* If a particular signature is expected, signature must match */

	if (signature && !ACPI_COMPARE_NAME(table->signature, signature)) {
		ACPI_ERROR((AE_INFO,
			    "Invalid signature 0x%X for ACPI table [%s]",
			    *ACPI_CAST_PTR(u32, table->signature), signature));
		goto unmap_and_exit;
	}

	/* Initialize the table entry */

	acpi_gbl_root_table_list.tables[table_index].address = address;
	acpi_gbl_root_table_list.tables[table_index].length = table->length;
	acpi_gbl_root_table_list.tables[table_index].flags = flags;

	ACPI_MOVE_32_TO_32(&
			   (acpi_gbl_root_table_list.tables[table_index].
			    signature), table->signature);

	acpi_tb_print_table_header(address, table);

	if (table_index == ACPI_TABLE_INDEX_DSDT) {

		/* Global integer width is based upon revision of the DSDT */

		acpi_ut_set_integer_width(table->revision);
	}

      unmap_and_exit:
	acpi_os_unmap_memory(table, sizeof(struct acpi_table_header));
}


static acpi_physical_address
acpi_tb_get_root_table_entry(u8 *table_entry, u32 table_entry_size)
{
	u64 address64;

	/*
	 * Get the table physical address (32-bit for RSDT, 64-bit for XSDT):
	 * Note: Addresses are 32-bit aligned (not 64) in both RSDT and XSDT
	 */
	if (table_entry_size == sizeof(u32)) {
		/*
		 * 32-bit platform, RSDT: Return 32-bit table entry
		 * 64-bit platform, RSDT: Expand 32-bit to 64-bit and return
		 */
		return ((acpi_physical_address)
			(*ACPI_CAST_PTR(u32, table_entry)));
	} else {
		/*
		 * 32-bit platform, XSDT: Truncate 64-bit to 32-bit and return
		 * 64-bit platform, XSDT: Move (unaligned) 64-bit to local, return 64-bit
		 */
		ACPI_MOVE_64_TO_64(&address64, table_entry);

#if ACPI_MACHINE_WIDTH == 32
		if (address64 > ACPI_UINT32_MAX) {

			/* Will truncate 64-bit address to 32 bits, issue warning */

			ACPI_WARNING((AE_INFO,
				      "64-bit Physical Address in XSDT is too large (%8.8X%8.8X), truncating",
				      ACPI_FORMAT_UINT64(address64)));
		}
#endif
		return ((acpi_physical_address) (address64));
	}
}


acpi_status __init
acpi_tb_parse_root_table(acpi_physical_address rsdp_address, u8 flags)
{
	struct acpi_table_rsdp *rsdp;
	u32 table_entry_size;
	u32 i;
	u32 table_count;
	struct acpi_table_header *table;
	acpi_physical_address address;
	acpi_physical_address uninitialized_var(rsdt_address);
	u32 length;
	u8 *table_entry;
	acpi_status status;

	ACPI_FUNCTION_TRACE(tb_parse_root_table);

	/*
	 * Map the entire RSDP and extract the address of the RSDT or XSDT
	 */
	rsdp = acpi_os_map_memory(rsdp_address, sizeof(struct acpi_table_rsdp));
	if (!rsdp) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	acpi_tb_print_table_header(rsdp_address,
				   ACPI_CAST_PTR(struct acpi_table_header,
						 rsdp));

	/* Differentiate between RSDT and XSDT root tables */

	if (rsdp->revision > 1 && rsdp->xsdt_physical_address
			&& !acpi_rsdt_forced) {
		/*
		 * Root table is an XSDT (64-bit physical addresses). We must use the
		 * XSDT if the revision is > 1 and the XSDT pointer is present, as per
		 * the ACPI specification.
		 */
		address = (acpi_physical_address) rsdp->xsdt_physical_address;
		table_entry_size = sizeof(u64);
		rsdt_address = (acpi_physical_address)
					rsdp->rsdt_physical_address;
	} else {
		/* Root table is an RSDT (32-bit physical addresses) */

		address = (acpi_physical_address) rsdp->rsdt_physical_address;
		table_entry_size = sizeof(u32);
	}

	/*
	 * It is not possible to map more than one entry in some environments,
	 * so unmap the RSDP here before mapping other tables
	 */
	acpi_os_unmap_memory(rsdp, sizeof(struct acpi_table_rsdp));

	if (table_entry_size == sizeof(u64)) {
		if (acpi_tb_check_xsdt(address) == AE_NULL_ENTRY) {
			/* XSDT has NULL entry, RSDT is used */
			address = rsdt_address;
			table_entry_size = sizeof(u32);
			ACPI_WARNING((AE_INFO, "BIOS XSDT has NULL entry, "
					"using RSDT"));
		}
	}
	/* Map the RSDT/XSDT table header to get the full table length */

	table = acpi_os_map_memory(address, sizeof(struct acpi_table_header));
	if (!table) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	acpi_tb_print_table_header(address, table);

	/* Get the length of the full table, verify length and map entire table */

	length = table->length;
	acpi_os_unmap_memory(table, sizeof(struct acpi_table_header));

	if (length < sizeof(struct acpi_table_header)) {
		ACPI_ERROR((AE_INFO, "Invalid length 0x%X in RSDT/XSDT",
			    length));
		return_ACPI_STATUS(AE_INVALID_TABLE_LENGTH);
	}

	table = acpi_os_map_memory(address, length);
	if (!table) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	/* Validate the root table checksum */

	status = acpi_tb_verify_checksum(table, length);
	if (ACPI_FAILURE(status)) {
		acpi_os_unmap_memory(table, length);
		return_ACPI_STATUS(status);
	}

	/* Calculate the number of tables described in the root table */

	table_count =
	    (u32) ((table->length -
		    sizeof(struct acpi_table_header)) / table_entry_size);

	/*
	 * First two entries in the table array are reserved for the DSDT and FACS,
	 * which are not actually present in the RSDT/XSDT - they come from the FADT
	 */
	table_entry =
	    ACPI_CAST_PTR(u8, table) + sizeof(struct acpi_table_header);
	acpi_gbl_root_table_list.count = 2;

	/*
	 * Initialize the root table array from the RSDT/XSDT
	 */
	for (i = 0; i < table_count; i++) {
		if (acpi_gbl_root_table_list.count >=
		    acpi_gbl_root_table_list.size) {

			/* There is no more room in the root table array, attempt resize */

			status = acpi_tb_resize_root_table_list();
			if (ACPI_FAILURE(status)) {
				ACPI_WARNING((AE_INFO,
					      "Truncating %u table entries!",
					      (unsigned) (table_count -
					       (acpi_gbl_root_table_list.
					       count - 2))));
				break;
			}
		}

		/* Get the table physical address (32-bit for RSDT, 64-bit for XSDT) */

		acpi_gbl_root_table_list.tables[acpi_gbl_root_table_list.count].
		    address =
		    acpi_tb_get_root_table_entry(table_entry, table_entry_size);

		table_entry += table_entry_size;
		acpi_gbl_root_table_list.count++;
	}

	/*
	 * It is not possible to map more than one entry in some environments,
	 * so unmap the root table here before mapping other tables
	 */
	acpi_os_unmap_memory(table, length);

	/*
	 * Complete the initialization of the root table array by examining
	 * the header of each table
	 */
	for (i = 2; i < acpi_gbl_root_table_list.count; i++) {
		acpi_tb_install_table(acpi_gbl_root_table_list.tables[i].
				      address, flags, NULL, i);

		/* Special case for FADT - get the DSDT and FACS */

		if (ACPI_COMPARE_NAME
		    (&acpi_gbl_root_table_list.tables[i].signature,
		     ACPI_SIG_FADT)) {
			acpi_tb_parse_fadt(i, flags);
		}
	}

	return_ACPI_STATUS(AE_OK);
}