

#ifndef _ET1310_MAC_H_
#define _ET1310_MAC_H_


#include "et1310_address_map.h"


#define COUNTER_WRAP_28_BIT 0x10000000
#define COUNTER_WRAP_22_BIT 0x400000
#define COUNTER_WRAP_16_BIT 0x10000
#define COUNTER_WRAP_12_BIT 0x1000

#define COUNTER_MASK_28_BIT (COUNTER_WRAP_28_BIT - 1)
#define COUNTER_MASK_22_BIT (COUNTER_WRAP_22_BIT - 1)
#define COUNTER_MASK_16_BIT (COUNTER_WRAP_16_BIT - 1)
#define COUNTER_MASK_12_BIT (COUNTER_WRAP_12_BIT - 1)

#define UPDATE_COUNTER(HostCnt,DevCnt) \
    HostCnt = HostCnt + DevCnt;

/* Forward declaration of the private adapter structure */
struct et131x_adapter;

void ConfigMACRegs1(struct et131x_adapter *adapter);
void ConfigMACRegs2(struct et131x_adapter *adapter);
void ConfigRxMacRegs(struct et131x_adapter *adapter);
void ConfigTxMacRegs(struct et131x_adapter *adapter);
void ConfigMacStatRegs(struct et131x_adapter *adapter);
void ConfigFlowControl(struct et131x_adapter *adapter);
void UpdateMacStatHostCounters(struct et131x_adapter *adapter);
void HandleMacStatInterrupt(struct et131x_adapter *adapter);
void SetupDeviceForMulticast(struct et131x_adapter *adapter);
void SetupDeviceForUnicast(struct et131x_adapter *adapter);

#endif /* _ET1310_MAC_H_ */
