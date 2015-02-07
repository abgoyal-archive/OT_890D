



#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acevents.h"

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwregs")

acpi_status acpi_hw_clear_acpi_status(void)
{
	acpi_status status;
	acpi_cpu_flags lock_flags = 0;

	ACPI_FUNCTION_TRACE(hw_clear_acpi_status);

	ACPI_DEBUG_PRINT((ACPI_DB_IO, "About to write %04X to %04X\n",
			  ACPI_BITMASK_ALL_FIXED_STATUS,
			  (u16) acpi_gbl_FADT.xpm1a_event_block.address));

	lock_flags = acpi_os_acquire_lock(acpi_gbl_hardware_lock);

	status = acpi_hw_register_write(ACPI_REGISTER_PM1_STATUS,
					ACPI_BITMASK_ALL_FIXED_STATUS);
	if (ACPI_FAILURE(status)) {
		goto unlock_and_exit;
	}

	/* Clear the fixed events */

	if (acpi_gbl_FADT.xpm1b_event_block.address) {
		status = acpi_write(ACPI_BITMASK_ALL_FIXED_STATUS,
				    &acpi_gbl_FADT.xpm1b_event_block);
		if (ACPI_FAILURE(status)) {
			goto unlock_and_exit;
		}
	}

	/* Clear the GPE Bits in all GPE registers in all GPE blocks */

	status = acpi_ev_walk_gpe_list(acpi_hw_clear_gpe_block, NULL);

      unlock_and_exit:
	acpi_os_release_lock(acpi_gbl_hardware_lock, lock_flags);
	return_ACPI_STATUS(status);
}


struct acpi_bit_register_info *acpi_hw_get_bit_register_info(u32 register_id)
{
	ACPI_FUNCTION_ENTRY();

	if (register_id > ACPI_BITREG_MAX) {
		ACPI_ERROR((AE_INFO, "Invalid BitRegister ID: %X",
			    register_id));
		return (NULL);
	}

	return (&acpi_gbl_bit_register_info[register_id]);
}

acpi_status
acpi_hw_register_read(u32 register_id, u32 * return_value)
{
	u32 value1 = 0;
	u32 value2 = 0;
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_register_read);

	switch (register_id) {
	case ACPI_REGISTER_PM1_STATUS:	/* 16-bit access */

		status = acpi_read(&value1, &acpi_gbl_FADT.xpm1a_event_block);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* PM1B is optional */

		status = acpi_read(&value2, &acpi_gbl_FADT.xpm1b_event_block);
		value1 |= value2;
		break;

	case ACPI_REGISTER_PM1_ENABLE:	/* 16-bit access */

		status = acpi_read(&value1, &acpi_gbl_xpm1a_enable);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* PM1B is optional */

		status = acpi_read(&value2, &acpi_gbl_xpm1b_enable);
		value1 |= value2;
		break;

	case ACPI_REGISTER_PM1_CONTROL:	/* 16-bit access */

		status = acpi_read(&value1, &acpi_gbl_FADT.xpm1a_control_block);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		status = acpi_read(&value2, &acpi_gbl_FADT.xpm1b_control_block);
		value1 |= value2;
		break;

	case ACPI_REGISTER_PM2_CONTROL:	/* 8-bit access */

		status = acpi_read(&value1, &acpi_gbl_FADT.xpm2_control_block);
		break;

	case ACPI_REGISTER_PM_TIMER:	/* 32-bit access */

		status = acpi_read(&value1, &acpi_gbl_FADT.xpm_timer_block);
		break;

	case ACPI_REGISTER_SMI_COMMAND_BLOCK:	/* 8-bit access */

		status =
		    acpi_os_read_port(acpi_gbl_FADT.smi_command, &value1, 8);
		break;

	default:
		ACPI_ERROR((AE_INFO, "Unknown Register ID: %X", register_id));
		status = AE_BAD_PARAMETER;
		break;
	}

      exit:

	if (ACPI_SUCCESS(status)) {
		*return_value = value1;
	}

	return_ACPI_STATUS(status);
}


acpi_status acpi_hw_register_write(u32 register_id, u32 value)
{
	acpi_status status;
	u32 read_value;

	ACPI_FUNCTION_TRACE(hw_register_write);

	switch (register_id) {
	case ACPI_REGISTER_PM1_STATUS:	/* 16-bit access */

		/* Perform a read first to preserve certain bits (per ACPI spec) */

		status = acpi_hw_register_read(ACPI_REGISTER_PM1_STATUS,
					       &read_value);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* Insert the bits to be preserved */

		ACPI_INSERT_BITS(value, ACPI_PM1_STATUS_PRESERVED_BITS,
				 read_value);

		/* Now we can write the data */

		status = acpi_write(value, &acpi_gbl_FADT.xpm1a_event_block);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* PM1B is optional */

		status = acpi_write(value, &acpi_gbl_FADT.xpm1b_event_block);
		break;

	case ACPI_REGISTER_PM1_ENABLE:	/* 16-bit access */

		status = acpi_write(value, &acpi_gbl_xpm1a_enable);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* PM1B is optional */

		status = acpi_write(value, &acpi_gbl_xpm1b_enable);
		break;

	case ACPI_REGISTER_PM1_CONTROL:	/* 16-bit access */

		/*
		 * Perform a read first to preserve certain bits (per ACPI spec)
		 */
		status = acpi_hw_register_read(ACPI_REGISTER_PM1_CONTROL,
					       &read_value);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* Insert the bits to be preserved */

		ACPI_INSERT_BITS(value, ACPI_PM1_CONTROL_PRESERVED_BITS,
				 read_value);

		/* Now we can write the data */

		status = acpi_write(value, &acpi_gbl_FADT.xpm1a_control_block);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		status = acpi_write(value, &acpi_gbl_FADT.xpm1b_control_block);
		break;

	case ACPI_REGISTER_PM1A_CONTROL:	/* 16-bit access */

		status = acpi_write(value, &acpi_gbl_FADT.xpm1a_control_block);
		break;

	case ACPI_REGISTER_PM1B_CONTROL:	/* 16-bit access */

		status = acpi_write(value, &acpi_gbl_FADT.xpm1b_control_block);
		break;

	case ACPI_REGISTER_PM2_CONTROL:	/* 8-bit access */

		status = acpi_write(value, &acpi_gbl_FADT.xpm2_control_block);
		break;

	case ACPI_REGISTER_PM_TIMER:	/* 32-bit access */

		status = acpi_write(value, &acpi_gbl_FADT.xpm_timer_block);
		break;

	case ACPI_REGISTER_SMI_COMMAND_BLOCK:	/* 8-bit access */

		/* SMI_CMD is currently always in IO space */

		status =
		    acpi_os_write_port(acpi_gbl_FADT.smi_command, value, 8);
		break;

	default:
		status = AE_BAD_PARAMETER;
		break;
	}

      exit:
	return_ACPI_STATUS(status);
}
