


#ifndef __ACHWARE_H__
#define __ACHWARE_H__

/* Values for the _SST predefined method */

#define ACPI_SST_INDICATOR_OFF  0
#define ACPI_SST_WORKING        1
#define ACPI_SST_WAKING         2
#define ACPI_SST_SLEEPING       3
#define ACPI_SST_SLEEP_CONTEXT  4

acpi_status acpi_hw_set_mode(u32 mode);

u32 acpi_hw_get_mode(void);

struct acpi_bit_register_info *acpi_hw_get_bit_register_info(u32 register_id);

acpi_status
acpi_hw_register_read(u32 register_id, u32 * return_value);

acpi_status acpi_hw_register_write(u32 register_id, u32 value);

acpi_status acpi_hw_clear_acpi_status(void);

acpi_status acpi_hw_low_disable_gpe(struct acpi_gpe_event_info *gpe_event_info);

acpi_status
acpi_hw_write_gpe_enable_reg(struct acpi_gpe_event_info *gpe_event_info);

acpi_status
acpi_hw_disable_gpe_block(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
			  struct acpi_gpe_block_info *gpe_block, void *context);

acpi_status acpi_hw_clear_gpe(struct acpi_gpe_event_info *gpe_event_info);

acpi_status
acpi_hw_clear_gpe_block(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
			struct acpi_gpe_block_info *gpe_block, void *context);

acpi_status
acpi_hw_get_gpe_status(struct acpi_gpe_event_info *gpe_event_info,
		       acpi_event_status * event_status);

acpi_status acpi_hw_disable_all_gpes(void);

acpi_status acpi_hw_enable_all_runtime_gpes(void);

acpi_status acpi_hw_enable_all_wakeup_gpes(void);

acpi_status
acpi_hw_enable_runtime_gpe_block(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
				 struct acpi_gpe_block_info *gpe_block,
				 void *context);

#ifdef	ACPI_FUTURE_USAGE
acpi_status acpi_get_timer_resolution(u32 * resolution);

acpi_status acpi_get_timer(u32 * ticks);

acpi_status
acpi_get_timer_duration(u32 start_ticks, u32 end_ticks, u32 * time_elapsed);
#endif				/* ACPI_FUTURE_USAGE */

#endif				/* __ACHWARE_H__ */
