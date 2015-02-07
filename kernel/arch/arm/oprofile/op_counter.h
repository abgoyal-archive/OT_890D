

#ifndef OP_COUNTER_H
#define OP_COUNTER_H

struct op_counter_config {
	unsigned long count;
	unsigned long enabled;
	unsigned long event;
	unsigned long unit_mask;
	unsigned long kernel;
	unsigned long user;
};

extern struct op_counter_config *counter_config;

#endif /* OP_COUNTER_H */
