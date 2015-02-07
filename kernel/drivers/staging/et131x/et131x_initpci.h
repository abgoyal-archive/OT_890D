

#ifndef __ET131X_INITPCI_H__
#define __ET131X_INITPCI_H__

/* Function Prototypes */
void et131x_align_allocated_memory(struct et131x_adapter *adapter,
				   u64 *phys_addr,
				   u64 *offset, u64 mask);

int et131x_adapter_setup(struct et131x_adapter *adapter);
int et131x_adapter_memory_alloc(struct et131x_adapter *adapter);
void et131x_adapter_memory_free(struct et131x_adapter *adapter);
void et131x_setup_hardware_properties(struct et131x_adapter *adapter);
void et131x_soft_reset(struct et131x_adapter *adapter);

#endif /* __ET131X_INITPCI_H__ */
