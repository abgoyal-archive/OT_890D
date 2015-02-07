

#ifndef __ET1310_JAGCORE_H__
#define __ET1310_JAGCORE_H__

#include "et1310_address_map.h"


#define INTERNAL_MEM_SIZE       0x400	//1024 of internal memory
#define INTERNAL_MEM_RX_OFFSET  0x1FF	//50%   Tx, 50%   Rx

#define REGS_MAX_ARRAY          4096

#define INT_MASK_DISABLE            0xffffffff

// NOTE: Masking out MAC_STAT Interrupt for now...
//#define INT_MASK_ENABLE             0xfff6bf17
//#define INT_MASK_ENABLE_NO_FLOW     0xfff6bfd7
#define INT_MASK_ENABLE             0xfffebf17
#define INT_MASK_ENABLE_NO_FLOW     0xfffebfd7

/* DATA STRUCTURES FOR DIRECT REGISTER ACCESS */

typedef struct {
	u8 bReadWrite;
	u32 nRegCount;
	u32 nData[REGS_MAX_ARRAY];
	u32 nOffsets[REGS_MAX_ARRAY];
} JAGCORE_ACCESS_REGS, *PJAGCORE_ACCESS_REGS;

typedef struct {
	u8 bReadWrite;
	u32 nDataWidth;
	u32 nRegCount;
	u32 nOffsets[REGS_MAX_ARRAY];
	u32 nData[REGS_MAX_ARRAY];
} PCI_CFG_SPACE_REGS, *PPCI_CFG_SPACE_REGS;

/* Forward declaration of the private adapter structure */
struct et131x_adapter;

void ConfigGlobalRegs(struct et131x_adapter *pAdapter);
void ConfigMMCRegs(struct et131x_adapter *pAdapter);
void et131x_enable_interrupts(struct et131x_adapter *adapter);
void et131x_disable_interrupts(struct et131x_adapter *adapter);

#endif /* __ET1310_JAGCORE_H__ */
