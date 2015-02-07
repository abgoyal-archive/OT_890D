



#include <acpi/acpi.h>
#include "accommon.h"

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwacpi")

acpi_status acpi_hw_set_mode(u32 mode)
{

	acpi_status status;
	u32 retry;

	ACPI_FUNCTION_TRACE(hw_set_mode);

	/*
	 * ACPI 2.0 clarified that if SMI_CMD in FADT is zero,
	 * system does not support mode transition.
	 */
	if (!acpi_gbl_FADT.smi_command) {
		ACPI_ERROR((AE_INFO,
			    "No SMI_CMD in FADT, mode transition failed"));
		return_ACPI_STATUS(AE_NO_HARDWARE_RESPONSE);
	}

	/*
	 * ACPI 2.0 clarified the meaning of ACPI_ENABLE and ACPI_DISABLE
	 * in FADT: If it is zero, enabling or disabling is not supported.
	 * As old systems may have used zero for mode transition,
	 * we make sure both the numbers are zero to determine these
	 * transitions are not supported.
	 */
	if (!acpi_gbl_FADT.acpi_enable && !acpi_gbl_FADT.acpi_disable) {
		ACPI_ERROR((AE_INFO,
			    "No ACPI mode transition supported in this system (enable/disable both zero)"));
		return_ACPI_STATUS(AE_OK);
	}

	switch (mode) {
	case ACPI_SYS_MODE_ACPI:

		/* BIOS should have disabled ALL fixed and GP events */

		status = acpi_os_write_port(acpi_gbl_FADT.smi_command,
					    (u32) acpi_gbl_FADT.acpi_enable, 8);
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				  "Attempting to enable ACPI mode\n"));
		break;

	case ACPI_SYS_MODE_LEGACY:

		/*
		 * BIOS should clear all fixed status bits and restore fixed event
		 * enable bits to default
		 */
		status = acpi_os_write_port(acpi_gbl_FADT.smi_command,
					    (u32) acpi_gbl_FADT.acpi_disable,
					    8);
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				  "Attempting to enable Legacy (non-ACPI) mode\n"));
		break;

	default:
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status,
				"Could not write ACPI mode change"));
		return_ACPI_STATUS(status);
	}

	/*
	 * Some hardware takes a LONG time to switch modes. Give them 3 sec to
	 * do so, but allow faster systems to proceed more quickly.
	 */
	retry = 3000;
	while (retry) {
		if (acpi_hw_get_mode() == mode) {
			ACPI_DEBUG_PRINT((ACPI_DB_INFO,
					  "Mode %X successfully enabled\n",
					  mode));
			return_ACPI_STATUS(AE_OK);
		}
		acpi_os_stall(1000);
		retry--;
	}

	ACPI_ERROR((AE_INFO, "Hardware did not change modes"));
	return_ACPI_STATUS(AE_NO_HARDWARE_RESPONSE);
}


u32 acpi_hw_get_mode(void)
{
	acpi_status status;
	u32 value;

	ACPI_FUNCTION_TRACE(hw_get_mode);

	/*
	 * ACPI 2.0 clarified that if SMI_CMD in FADT is zero,
	 * system does not support mode transition.
	 */
	if (!acpi_gbl_FADT.smi_command) {
		return_UINT32(ACPI_SYS_MODE_ACPI);
	}

	status = acpi_get_register(ACPI_BITREG_SCI_ENABLE, &value);
	if (ACPI_FAILURE(status)) {
		return_UINT32(ACPI_SYS_MODE_LEGACY);
	}

	if (value) {
		return_UINT32(ACPI_SYS_MODE_ACPI);
	} else {
		return_UINT32(ACPI_SYS_MODE_LEGACY);
	}
}
