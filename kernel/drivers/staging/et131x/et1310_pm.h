

#ifndef _ET1310_PM_H_
#define _ET1310_PM_H_

#include "et1310_address_map.h"

#define MAX_WOL_PACKET_SIZE    0x80
#define MAX_WOL_MASK_SIZE      ( MAX_WOL_PACKET_SIZE / 8 )
#define NUM_WOL_PATTERNS       0x5
#define CRC16_POLY             0x1021

/* Definition of NDIS_DEVICE_POWER_STATE */
typedef enum {
	NdisDeviceStateUnspecified = 0,
	NdisDeviceStateD0,
	NdisDeviceStateD1,
	NdisDeviceStateD2,
	NdisDeviceStateD3
} NDIS_DEVICE_POWER_STATE;

typedef struct _MP_POWER_MGMT {
	/* variable putting the phy into coma mode when boot up with no cable
	 * plugged in after 5 seconds
	 */
	u8 TransPhyComaModeOnBoot;

	/* Array holding the five CRC values that the device is currently
	 * using for WOL.  This will be queried when a pattern is to be
	 * removed.
	 */
	u32 localWolAndCrc0;
	u16 WOLPatternList[NUM_WOL_PATTERNS];
	u8 WOLMaskList[NUM_WOL_PATTERNS][MAX_WOL_MASK_SIZE];
	u32 WOLMaskSize[NUM_WOL_PATTERNS];

	/* IP address */
	union {
		u32 u32;
		u8 u8[4];
	} IPAddress;

	/* Current Power state of the adapter. */
	NDIS_DEVICE_POWER_STATE PowerState;
	bool WOLState;
	bool WOLEnabled;
	bool Failed10Half;
	bool bFailedStateTransition;

	/* Next two used to save power information at power down. This
	 * information will be used during power up to set up parts of Power
	 * Management in JAGCore
	 */
	u32 tx_en;
	u32 rx_en;
	u16 PowerDownSpeed;
	u8 PowerDownDuplex;
} MP_POWER_MGMT, *PMP_POWER_MGMT;

struct et131x_adapter;

u16 CalculateCCITCRC16(u8 *Pattern, u8 *Mask, u32 MaskSize);
void EnablePhyComa(struct et131x_adapter *adapter);
void DisablePhyComa(struct et131x_adapter *adapter);

#endif /* _ET1310_PM_H_ */
