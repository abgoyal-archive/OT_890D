

#ifndef __REGULATOR_FIXED_H
#define __REGULATOR_FIXED_H

struct fixed_voltage_config {
	const char *supply_name;
	int microvolts;
};

#endif
