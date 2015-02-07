



#include <acpi/acpi.h>
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwsleep")

acpi_status
acpi_set_firmware_waking_vector(u32 physical_address)
{
	ACPI_FUNCTION_TRACE(acpi_set_firmware_waking_vector);


	/*
	 * According to the ACPI specification 2.0c and later, the 64-bit
	 * waking vector should be cleared and the 32-bit waking vector should
	 * be used, unless we want the wake-up code to be called by the BIOS in
	 * Protected Mode.  Some systems (for example HP dv5-1004nr) are known
	 * to fail to resume if the 64-bit vector is used.
	 */

	/* Set the 32-bit vector */

	acpi_gbl_FACS->firmware_waking_vector = physical_address;

	/* Clear the 64-bit vector if it exists */

	if ((acpi_gbl_FACS->length > 32) && (acpi_gbl_FACS->version >= 1)) {
		acpi_gbl_FACS->xfirmware_waking_vector = 0;
	}

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_set_firmware_waking_vector)

acpi_status
acpi_set_firmware_waking_vector64(u64 physical_address)
{
	ACPI_FUNCTION_TRACE(acpi_set_firmware_waking_vector64);


	/* Determine if the 64-bit vector actually exists */

	if ((acpi_gbl_FACS->length <= 32) || (acpi_gbl_FACS->version < 1)) {
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	/* Clear 32-bit vector, set the 64-bit X_ vector */

	acpi_gbl_FACS->firmware_waking_vector = 0;
	acpi_gbl_FACS->xfirmware_waking_vector = physical_address;

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_set_firmware_waking_vector64)

acpi_status acpi_enter_sleep_state_prep(u8 sleep_state)
{
	acpi_status status;
	struct acpi_object_list arg_list;
	union acpi_object arg;

	ACPI_FUNCTION_TRACE(acpi_enter_sleep_state_prep);

	/*
	 * _PSW methods could be run here to enable wake-on keyboard, LAN, etc.
	 */
	status = acpi_get_sleep_type_data(sleep_state,
					  &acpi_gbl_sleep_type_a,
					  &acpi_gbl_sleep_type_b);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Setup parameter object */

	arg_list.count = 1;
	arg_list.pointer = &arg;

	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = sleep_state;

	/* Run the _PTS method */

	status = acpi_evaluate_object(NULL, METHOD_NAME__PTS, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		return_ACPI_STATUS(status);
	}

	/* Setup the argument to _SST */

	switch (sleep_state) {
	case ACPI_STATE_S0:
		arg.integer.value = ACPI_SST_WORKING;
		break;

	case ACPI_STATE_S1:
	case ACPI_STATE_S2:
	case ACPI_STATE_S3:
		arg.integer.value = ACPI_SST_SLEEPING;
		break;

	case ACPI_STATE_S4:
		arg.integer.value = ACPI_SST_SLEEP_CONTEXT;
		break;

	default:
		arg.integer.value = ACPI_SST_INDICATOR_OFF;	/* Default is off */
		break;
	}

	/*
	 * Set the system indicators to show the desired sleep state.
	 * _SST is an optional method (return no error if not found)
	 */
	status = acpi_evaluate_object(NULL, METHOD_NAME__SST, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		ACPI_EXCEPTION((AE_INFO, status,
				"While executing method _SST"));
	}

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_enter_sleep_state_prep)

acpi_status asmlinkage acpi_enter_sleep_state(u8 sleep_state)
{
	u32 PM1Acontrol;
	u32 PM1Bcontrol;
	struct acpi_bit_register_info *sleep_type_reg_info;
	struct acpi_bit_register_info *sleep_enable_reg_info;
	u32 in_value;
	struct acpi_object_list arg_list;
	union acpi_object arg;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_enter_sleep_state);

	if ((acpi_gbl_sleep_type_a > ACPI_SLEEP_TYPE_MAX) ||
	    (acpi_gbl_sleep_type_b > ACPI_SLEEP_TYPE_MAX)) {
		ACPI_ERROR((AE_INFO, "Sleep values out of range: A=%X B=%X",
			    acpi_gbl_sleep_type_a, acpi_gbl_sleep_type_b));
		return_ACPI_STATUS(AE_AML_OPERAND_VALUE);
	}

	sleep_type_reg_info =
	    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_TYPE_A);
	sleep_enable_reg_info =
	    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_ENABLE);

	/* Clear wake status */

	status = acpi_set_register(ACPI_BITREG_WAKE_STATUS, 1);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Clear all fixed and general purpose status bits */

	status = acpi_hw_clear_acpi_status();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * 1) Disable/Clear all GPEs
	 * 2) Enable all wakeup GPEs
	 */
	status = acpi_hw_disable_all_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}
	acpi_gbl_system_awake_and_running = FALSE;

	status = acpi_hw_enable_all_wakeup_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Execute the _GTS method */

	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = sleep_state;

	status = acpi_evaluate_object(NULL, METHOD_NAME__GTS, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		return_ACPI_STATUS(status);
	}

	/* Get current value of PM1A control */

	status = acpi_hw_register_read(ACPI_REGISTER_PM1_CONTROL, &PM1Acontrol);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}
	ACPI_DEBUG_PRINT((ACPI_DB_INIT,
			  "Entering sleep state [S%d]\n", sleep_state));

	/* Clear SLP_EN and SLP_TYP fields */

	PM1Acontrol &= ~(sleep_type_reg_info->access_bit_mask |
			 sleep_enable_reg_info->access_bit_mask);
	PM1Bcontrol = PM1Acontrol;

	/* Insert SLP_TYP bits */

	PM1Acontrol |=
	    (acpi_gbl_sleep_type_a << sleep_type_reg_info->bit_position);
	PM1Bcontrol |=
	    (acpi_gbl_sleep_type_b << sleep_type_reg_info->bit_position);

	/*
	 * We split the writes of SLP_TYP and SLP_EN to workaround
	 * poorly implemented hardware.
	 */

	/* Write #1: fill in SLP_TYP data */

	status = acpi_hw_register_write(ACPI_REGISTER_PM1A_CONTROL,
					PM1Acontrol);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_hw_register_write(ACPI_REGISTER_PM1B_CONTROL,
					PM1Bcontrol);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Insert SLP_ENABLE bit */

	PM1Acontrol |= sleep_enable_reg_info->access_bit_mask;
	PM1Bcontrol |= sleep_enable_reg_info->access_bit_mask;

	/* Write #2: SLP_TYP + SLP_EN */

	ACPI_FLUSH_CPU_CACHE();

	status = acpi_hw_register_write(ACPI_REGISTER_PM1A_CONTROL,
					PM1Acontrol);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_hw_register_write(ACPI_REGISTER_PM1B_CONTROL,
					PM1Bcontrol);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	if (sleep_state > ACPI_STATE_S3) {
		/*
		 * We wanted to sleep > S3, but it didn't happen (by virtue of the
		 * fact that we are still executing!)
		 *
		 * Wait ten seconds, then try again. This is to get S4/S5 to work on
		 * all machines.
		 *
		 * We wait so long to allow chipsets that poll this reg very slowly to
		 * still read the right value. Ideally, this block would go
		 * away entirely.
		 */
		acpi_os_stall(10000000);

		status = acpi_hw_register_write(ACPI_REGISTER_PM1_CONTROL,
						sleep_enable_reg_info->
						access_bit_mask);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	}

	/* Wait until we enter sleep state */

	do {
		status = acpi_get_register_unlocked(ACPI_BITREG_WAKE_STATUS,
						    &in_value);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

		/* Spin until we wake */

	} while (!in_value);

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_enter_sleep_state)

acpi_status asmlinkage acpi_enter_sleep_state_s4bios(void)
{
	u32 in_value;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_enter_sleep_state_s4bios);

	status = acpi_set_register(ACPI_BITREG_WAKE_STATUS, 1);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_hw_clear_acpi_status();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * 1) Disable/Clear all GPEs
	 * 2) Enable all wakeup GPEs
	 */
	status = acpi_hw_disable_all_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}
	acpi_gbl_system_awake_and_running = FALSE;

	status = acpi_hw_enable_all_wakeup_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	ACPI_FLUSH_CPU_CACHE();

	status = acpi_os_write_port(acpi_gbl_FADT.smi_command,
				    (u32) acpi_gbl_FADT.S4bios_request, 8);

	do {
		acpi_os_stall(1000);
		status = acpi_get_register(ACPI_BITREG_WAKE_STATUS, &in_value);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	} while (!in_value);

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_enter_sleep_state_s4bios)

acpi_status acpi_leave_sleep_state_prep(u8 sleep_state)
{
	struct acpi_object_list arg_list;
	union acpi_object arg;
	acpi_status status;
	struct acpi_bit_register_info *sleep_type_reg_info;
	struct acpi_bit_register_info *sleep_enable_reg_info;
	u32 PM1Acontrol;
	u32 PM1Bcontrol;

	ACPI_FUNCTION_TRACE(acpi_leave_sleep_state_prep);

	/*
	 * Set SLP_TYPE and SLP_EN to state S0.
	 * This is unclear from the ACPI Spec, but it is required
	 * by some machines.
	 */
	status = acpi_get_sleep_type_data(ACPI_STATE_S0,
					  &acpi_gbl_sleep_type_a,
					  &acpi_gbl_sleep_type_b);
	if (ACPI_SUCCESS(status)) {
		sleep_type_reg_info =
		    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_TYPE_A);
		sleep_enable_reg_info =
		    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_ENABLE);

		/* Get current value of PM1A control */

		status = acpi_hw_register_read(ACPI_REGISTER_PM1_CONTROL,
					       &PM1Acontrol);
		if (ACPI_SUCCESS(status)) {

			/* Clear SLP_EN and SLP_TYP fields */

			PM1Acontrol &= ~(sleep_type_reg_info->access_bit_mask |
					 sleep_enable_reg_info->
					 access_bit_mask);
			PM1Bcontrol = PM1Acontrol;

			/* Insert SLP_TYP bits */

			PM1Acontrol |=
			    (acpi_gbl_sleep_type_a << sleep_type_reg_info->
			     bit_position);
			PM1Bcontrol |=
			    (acpi_gbl_sleep_type_b << sleep_type_reg_info->
			     bit_position);

			/* Just ignore any errors */

			(void)acpi_hw_register_write(ACPI_REGISTER_PM1A_CONTROL,
						     PM1Acontrol);
			(void)acpi_hw_register_write(ACPI_REGISTER_PM1B_CONTROL,
						     PM1Bcontrol);
		}
	}

	/* Execute the _BFS method */

	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = sleep_state;

	status = acpi_evaluate_object(NULL, METHOD_NAME__BFS, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		ACPI_EXCEPTION((AE_INFO, status, "During Method _BFS"));
	}

	return_ACPI_STATUS(status);
}

acpi_status acpi_leave_sleep_state(u8 sleep_state)
{
	struct acpi_object_list arg_list;
	union acpi_object arg;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_leave_sleep_state);

	/* Ensure enter_sleep_state_prep -> enter_sleep_state ordering */

	acpi_gbl_sleep_type_a = ACPI_SLEEP_TYPE_INVALID;

	/* Setup parameter object */

	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;

	/* Ignore any errors from these methods */

	arg.integer.value = ACPI_SST_WAKING;
	status = acpi_evaluate_object(NULL, METHOD_NAME__SST, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		ACPI_EXCEPTION((AE_INFO, status, "During Method _SST"));
	}

	/*
	 * GPEs must be enabled before _WAK is called as GPEs
	 * might get fired there
	 *
	 * Restore the GPEs:
	 * 1) Disable/Clear all GPEs
	 * 2) Enable all runtime GPEs
	 */
	status = acpi_hw_disable_all_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}
	status = acpi_hw_enable_all_runtime_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	arg.integer.value = sleep_state;
	status = acpi_evaluate_object(NULL, METHOD_NAME__WAK, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		ACPI_EXCEPTION((AE_INFO, status, "During Method _WAK"));
	}
	/* TBD: _WAK "sometimes" returns stuff - do we want to look at it? */

	/*
	 * Some BIOSes assume that WAK_STS will be cleared on resume and use
	 * it to determine whether the system is rebooting or resuming. Clear
	 * it for compatibility.
	 */
	acpi_set_register(ACPI_BITREG_WAKE_STATUS, 1);

	acpi_gbl_system_awake_and_running = TRUE;

	/* Enable power button */

	(void)
	    acpi_set_register(acpi_gbl_fixed_event_info
			      [ACPI_EVENT_POWER_BUTTON].enable_register_id, 1);

	(void)
	    acpi_set_register(acpi_gbl_fixed_event_info
			      [ACPI_EVENT_POWER_BUTTON].status_register_id, 1);

	arg.integer.value = ACPI_SST_WORKING;
	status = acpi_evaluate_object(NULL, METHOD_NAME__SST, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		ACPI_EXCEPTION((AE_INFO, status, "During Method _SST"));
	}

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_leave_sleep_state)