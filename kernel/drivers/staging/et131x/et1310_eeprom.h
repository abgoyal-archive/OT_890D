

#ifndef __ET1310_EEPROM_H__
#define __ET1310_EEPROM_H__

#include "et1310_address_map.h"

#ifndef SUCCESS
#define SUCCESS		0
#define FAILURE		1
#endif

#ifndef READ
#define READ		0
#define WRITE		1
#endif

#ifndef SINGLE_BYTE
#define SINGLE_BYTE	0
#define DUAL_BYTE	1
#endif

/* Forward declaration of the private adapter structure */
struct et131x_adapter;

int32_t EepromWriteByte(struct et131x_adapter *adapter, u32 unAddress,
			u8 bData, u32 unEepromId,
			u32 unAddressingMode);
int32_t EepromReadByte(struct et131x_adapter *adapter, u32 unAddress,
		       u8 *pbData, u32 unEepromId,
		       u32 unAddressingMode);

#endif /* _ET1310_EEPROM_H_ */
