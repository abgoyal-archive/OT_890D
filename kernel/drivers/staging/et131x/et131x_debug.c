

#ifdef CONFIG_ET131X_DEBUG

#include "et131x_version.h"
#include "et131x_debug.h"
#include "et131x_defs.h"

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>

#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/bitops.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/ioport.h>
#include <linux/random.h>

#include "et1310_phy.h"
#include "et1310_pm.h"
#include "et1310_jagcore.h"

#include "et131x_adapter.h"
#include "et131x_netdev.h"
#include "et131x_config.h"
#include "et131x_isr.h"

#include "et1310_address_map.h"
#include "et1310_tx.h"
#include "et1310_rx.h"
#include "et1310_mac.h"

/* Data for debugging facilities */
extern dbg_info_t *et131x_dbginfo;

void DumpTxQueueContents(int dbgLvl, struct et131x_adapter *pAdapter)
{
	MMC_t __iomem *mmc = &pAdapter->CSRAddress->mmc;
	uint32_t TxQueueAddr;

	if (DBG_FLAGS(et131x_dbginfo) & dbgLvl) {
		for (TxQueueAddr = 0x200; TxQueueAddr < 0x3ff; TxQueueAddr++) {
			MMC_SRAM_ACCESS_t sram_access;

			sram_access.value = readl(&mmc->sram_access.value);
			sram_access.bits.req_addr = TxQueueAddr;
			sram_access.bits.req_access = 1;
			writel(sram_access.value, &mmc->sram_access.value);

			DBG_PRINT("Addr 0x%x, Access 0x%08x\t"
				  "Value 1 0x%08x, Value 2 0x%08x, "
				  "Value 3 0x%08x, Value 4 0x%08x, \n",
				  TxQueueAddr,
				  readl(&mmc->sram_access.value),
				  readl(&mmc->sram_word1),
				  readl(&mmc->sram_word2),
				  readl(&mmc->sram_word3),
				  readl(&mmc->sram_word4));
		}

		DBG_PRINT("Shadow Pointers 0x%08x\n",
			  readl(&pAdapter->CSRAddress->txmac.shadow_ptr.value));
	}
}

#define NUM_BLOCKS 8
void DumpDeviceBlock(int dbgLvl, struct et131x_adapter *pAdapter,
		     uint32_t Block)
{
	uint32_t Address1, Address2;
	uint32_t __iomem *BigDevicePointer =
		(uint32_t __iomem *) pAdapter->CSRAddress;
	const char *BlockNames[NUM_BLOCKS] = {
		"Global", "Tx DMA", "Rx DMA", "Tx MAC",
		"Rx MAC", "MAC", "MAC Stat", "MMC"
	};

	/* Output the debug counters to the debug terminal */
	if (DBG_FLAGS(et131x_dbginfo) & dbgLvl) {
		DBG_PRINT("%s block\n", BlockNames[Block]);
		BigDevicePointer += Block * 1024;
		for (Address1 = 0; Address1 < 8; Address1++) {
			for (Address2 = 0; Address2 < 8; Address2++) {
				if (Block == 0 &&
				    (Address1 * 8 + Address2) == 6) {
					DBG_PRINT("  ISR    , ");
				} else {
					DBG_PRINT("0x%08x, ",
						  readl(BigDevicePointer++));
				}
			}
			DBG_PRINT("\n");
		}
		DBG_PRINT("\n");
	}
}

void DumpDeviceReg(int dbgLvl, struct et131x_adapter *pAdapter)
{
	uint32_t Address1, Address2;
	uint32_t Block;
	uint32_t __iomem *BigDevicePointer =
		(uint32_t __iomem *) pAdapter->CSRAddress;
	uint32_t __iomem *Pointer;
	const char *BlockNames[NUM_BLOCKS] = {
		"Global", "Tx DMA", "Rx DMA", "Tx MAC",
		"Rx MAC", "MAC", "MAC Stat", "MMC"
	};

	/* Output the debug counters to the debug terminal */
	if (DBG_FLAGS(et131x_dbginfo) & dbgLvl) {
		for (Block = 0; Block < NUM_BLOCKS; Block++) {
			DBG_PRINT("%s block\n", BlockNames[Block]);
			Pointer = BigDevicePointer + (Block * 1024);

			for (Address1 = 0; Address1 < 8; Address1++) {
				for (Address2 = 0; Address2 < 8; Address2++) {
					DBG_PRINT("0x%08x, ",
						  readl(Pointer++));
				}
				DBG_PRINT("\n");
			}
			DBG_PRINT("\n");
		}
	}
}

#endif // CONFIG_ET131X_DEBUG
