

#include "et131x_version.h"
#include "et131x_debug.h"
#include "et131x_defs.h"

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

#include "et1310_phy.h"
#include "et1310_pm.h"
#include "et1310_jagcore.h"

#include "et131x_adapter.h"
#include "et131x_initpci.h"

/* Data for debugging facilities */
#ifdef CONFIG_ET131X_DEBUG
extern dbg_info_t *et131x_dbginfo;
#endif /* CONFIG_ET131X_DEBUG */

void ConfigGlobalRegs(struct et131x_adapter *pAdapter)
{
	struct _GLOBAL_t __iomem *pGbl = &pAdapter->CSRAddress->global;

	DBG_ENTER(et131x_dbginfo);

	if (pAdapter->RegistryPhyLoopbk == false) {
		if (pAdapter->RegistryJumboPacket < 2048) {
			/* Tx / RxDMA and Tx/Rx MAC interfaces have a 1k word
			 * block of RAM that the driver can split between Tx
			 * and Rx as it desires.  Our default is to split it
			 * 50/50:
			 */
			writel(0, &pGbl->rxq_start_addr.value);
			writel(pAdapter->RegistryRxMemEnd,
			       &pGbl->rxq_end_addr.value);
			writel(pAdapter->RegistryRxMemEnd + 1,
			       &pGbl->txq_start_addr.value);
			writel(INTERNAL_MEM_SIZE - 1,
			       &pGbl->txq_end_addr.value);
		} else if (pAdapter->RegistryJumboPacket < 8192) {
			/* For jumbo packets > 2k but < 8k, split 50-50. */
			writel(0, &pGbl->rxq_start_addr.value);
			writel(INTERNAL_MEM_RX_OFFSET,
			       &pGbl->rxq_end_addr.value);
			writel(INTERNAL_MEM_RX_OFFSET + 1,
			       &pGbl->txq_start_addr.value);
			writel(INTERNAL_MEM_SIZE - 1,
			       &pGbl->txq_end_addr.value);
		} else {
			/* 9216 is the only packet size greater than 8k that
			 * is available. The Tx buffer has to be big enough
			 * for one whole packet on the Tx side. We'll make
			 * the Tx 9408, and give the rest to Rx
			 */
			writel(0x0000, &pGbl->rxq_start_addr.value);
			writel(0x01b3, &pGbl->rxq_end_addr.value);
			writel(0x01b4, &pGbl->txq_start_addr.value);
			writel(INTERNAL_MEM_SIZE - 1,
			       &pGbl->txq_end_addr.value);
		}

		/* Initialize the loopback register. Disable all loopbacks. */
		writel(0, &pGbl->loopback.value);
	} else {
		/* For PHY Line loopback, the memory is configured as if Tx
		 * and Rx both have all the memory.  This is because the
		 * RxMAC will write data into the space, and the TxMAC will
		 * read it out.
		 */
		writel(0, &pGbl->rxq_start_addr.value);
		writel(INTERNAL_MEM_SIZE - 1, &pGbl->rxq_end_addr.value);
		writel(0, &pGbl->txq_start_addr.value);
		writel(INTERNAL_MEM_SIZE - 1, &pGbl->txq_end_addr.value);

		/* Initialize the loopback register (MAC loopback). */
		writel(1, &pGbl->loopback.value);
	}

	/* MSI Register */
	writel(0, &pGbl->msi_config.value);

	/* By default, disable the watchdog timer.  It will be enabled when
	 * a packet is queued.
	 */
	writel(0, &pGbl->watchdog_timer);

	DBG_LEAVE(et131x_dbginfo);
}

void ConfigMMCRegs(struct et131x_adapter *pAdapter)
{
	MMC_CTRL_t mmc_ctrl = { 0 };

	DBG_ENTER(et131x_dbginfo);

	/* All we need to do is initialize the Memory Control Register */
	mmc_ctrl.bits.force_ce = 0x0;
	mmc_ctrl.bits.rxdma_disable = 0x0;
	mmc_ctrl.bits.txdma_disable = 0x0;
	mmc_ctrl.bits.txmac_disable = 0x0;
	mmc_ctrl.bits.rxmac_disable = 0x0;
	mmc_ctrl.bits.arb_disable = 0x0;
	mmc_ctrl.bits.mmc_enable = 0x1;

	writel(mmc_ctrl.value, &pAdapter->CSRAddress->mmc.mmc_ctrl.value);

	DBG_LEAVE(et131x_dbginfo);
}

void et131x_enable_interrupts(struct et131x_adapter *adapter)
{
	uint32_t MaskValue;

	/* Enable all global interrupts */
	if ((adapter->FlowControl == TxOnly) || (adapter->FlowControl == Both)) {
		MaskValue = INT_MASK_ENABLE;
	} else {
		MaskValue = INT_MASK_ENABLE_NO_FLOW;
	}

	if (adapter->DriverNoPhyAccess) {
		MaskValue |= 0x10000;
	}

	adapter->CachedMaskValue.value = MaskValue;
	writel(MaskValue, &adapter->CSRAddress->global.int_mask.value);
}

void et131x_disable_interrupts(struct et131x_adapter * adapter)
{
	/* Disable all global interrupts */
	adapter->CachedMaskValue.value = INT_MASK_DISABLE;
	writel(INT_MASK_DISABLE, &adapter->CSRAddress->global.int_mask.value);
}
