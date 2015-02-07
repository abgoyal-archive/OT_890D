



#ifndef __ACPIOSXF_H__
#define __ACPIOSXF_H__

#include "platform/acenv.h"
#include "actypes.h"

/* Types for acpi_os_execute */

typedef enum {
	OSL_GLOBAL_LOCK_HANDLER,
	OSL_NOTIFY_HANDLER,
	OSL_GPE_HANDLER,
	OSL_DEBUGGER_THREAD,
	OSL_EC_POLL_HANDLER,
	OSL_EC_BURST_HANDLER
} acpi_execute_type;

#define ACPI_NO_UNIT_LIMIT          ((u32) -1)
#define ACPI_MUTEX_SEM              1

/* Functions for acpi_os_signal */

#define ACPI_SIGNAL_FATAL           0
#define ACPI_SIGNAL_BREAKPOINT      1

struct acpi_signal_fatal_info {
	u32 type;
	u32 code;
	u32 argument;
};

acpi_status __initdata acpi_os_initialize(void);

acpi_status acpi_os_terminate(void);

acpi_physical_address acpi_os_get_root_pointer(void);

acpi_status
acpi_os_predefined_override(const struct acpi_predefined_names *init_val,
			    acpi_string * new_val);

acpi_status
acpi_os_table_override(struct acpi_table_header *existing_table,
		       struct acpi_table_header **new_table);

acpi_status acpi_os_create_lock(acpi_spinlock * out_handle);

void acpi_os_delete_lock(acpi_spinlock handle);

acpi_cpu_flags acpi_os_acquire_lock(acpi_spinlock handle);

void acpi_os_release_lock(acpi_spinlock handle, acpi_cpu_flags flags);

acpi_status
acpi_os_create_semaphore(u32 max_units,
			 u32 initial_units, acpi_semaphore * out_handle);

acpi_status acpi_os_delete_semaphore(acpi_semaphore handle);

acpi_status
acpi_os_wait_semaphore(acpi_semaphore handle, u32 units, u16 timeout);

acpi_status acpi_os_signal_semaphore(acpi_semaphore handle, u32 units);

#if (ACPI_MUTEX_TYPE != ACPI_BINARY_SEMAPHORE)

acpi_status acpi_os_create_mutex(acpi_mutex * out_handle);

void acpi_os_delete_mutex(acpi_mutex handle);

acpi_status acpi_os_acquire_mutex(acpi_mutex handle, u16 timeout);

void acpi_os_release_mutex(acpi_mutex handle);
#endif

void *acpi_os_allocate(acpi_size size);

void __iomem *acpi_os_map_memory(acpi_physical_address where,
				acpi_size length);

void acpi_os_unmap_memory(void __iomem * logical_address, acpi_size size);

#ifdef ACPI_FUTURE_USAGE
acpi_status
acpi_os_get_physical_address(void *logical_address,
			     acpi_physical_address * physical_address);
#endif

acpi_status
acpi_os_create_cache(char *cache_name,
		     u16 object_size,
		     u16 max_depth, acpi_cache_t ** return_cache);

acpi_status acpi_os_delete_cache(acpi_cache_t * cache);

acpi_status acpi_os_purge_cache(acpi_cache_t * cache);

void *acpi_os_acquire_object(acpi_cache_t * cache);

acpi_status acpi_os_release_object(acpi_cache_t * cache, void *object);

acpi_status
acpi_os_install_interrupt_handler(u32 gsi,
				  acpi_osd_handler service_routine,
				  void *context);

acpi_status
acpi_os_remove_interrupt_handler(u32 gsi, acpi_osd_handler service_routine);

void acpi_os_gpe_count(u32 gpe_number);
void acpi_os_fixed_event_count(u32 fixed_event_number);

acpi_thread_id acpi_os_get_thread_id(void);

acpi_status
acpi_os_execute(acpi_execute_type type,
		acpi_osd_exec_callback function, void *context);

acpi_status
acpi_os_hotplug_execute(acpi_osd_exec_callback function, void *context);

void acpi_os_wait_events_complete(void *context);

void acpi_os_sleep(acpi_integer milliseconds);

void acpi_os_stall(u32 microseconds);

acpi_status acpi_os_read_port(acpi_io_address address, u32 * value, u32 width);

acpi_status acpi_os_write_port(acpi_io_address address, u32 value, u32 width);

acpi_status
acpi_os_read_memory(acpi_physical_address address, u32 * value, u32 width);

acpi_status
acpi_os_write_memory(acpi_physical_address address, u32 value, u32 width);

acpi_status
acpi_os_read_pci_configuration(struct acpi_pci_id *pci_id,
			       u32 reg, u32 *value, u32 width);

acpi_status
acpi_os_write_pci_configuration(struct acpi_pci_id *pci_id,
				u32 reg, acpi_integer value, u32 width);

void
acpi_os_derive_pci_id(acpi_handle rhandle,
		      acpi_handle chandle, struct acpi_pci_id **pci_id);

acpi_status acpi_os_validate_interface(char *interface);
acpi_status acpi_osi_invalidate(char* interface);

acpi_status
acpi_os_validate_address(u8 space_id, acpi_physical_address address,
			 acpi_size length, char *name);

u64 acpi_os_get_timer(void);

acpi_status acpi_os_signal(u32 function, void *info);

void ACPI_INTERNAL_VAR_XFACE acpi_os_printf(const char *format, ...);

void acpi_os_vprintf(const char *format, va_list args);

void acpi_os_redirect_output(void *destination);

#ifdef ACPI_FUTURE_USAGE
u32 acpi_os_get_line(char *buffer);
#endif

void *acpi_os_open_directory(char *pathname,
			     char *wildcard_spec, char requested_file_type);

/* requeste_file_type values */

#define REQUEST_FILE_ONLY                   0
#define REQUEST_DIR_ONLY                    1

char *acpi_os_get_next_filename(void *dir_handle);

void acpi_os_close_directory(void *dir_handle);

#endif				/* __ACPIOSXF_H__ */
