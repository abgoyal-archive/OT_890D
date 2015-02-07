

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
#include "et1310_mac.h"
#include "et1310_rx.h"

#include "et131x_adapter.h"
#include "et131x_initpci.h"

/* Data for debugging facilities */
#ifdef CONFIG_ET131X_DEBUG
extern dbg_info_t *et131x_dbginfo;
#endif /* CONFIG_ET131X_DEBUG */

void EnablePhyComa(struct et131x_adapter *pAdapter)
{
	unsigned long lockflags;
	PM_CSR_t GlobalPmCSR;
	int32_t LoopCounter = 10;

	DBG_ENTER(et131x_dbginfo);

	GlobalPmCSR.value = readl(&pAdapter->CSRAddress->global.pm_csr.value);

	/* Save the GbE PHY speed and duplex modes. Need to restore this
	 * when cable is plugged back in
	 */
	pAdapter->PoMgmt.PowerDownSpeed = pAdapter->AiForceSpeed;
	pAdapter->PoMgmt.PowerDownDuplex = pAdapter->AiForceDpx;

	/* Stop sending packets. */
	spin_lock_irqsave(&pAdapter->SendHWLock, lockflags);
	MP_SET_FLAG(pAdapter, fMP_ADAPTER_LOWER_POWER);
	spin_unlock_irqrestore(&pAdapter->SendHWLock, lockflags);

	/* Wait for outstanding Receive packets */
	while ((MP_GET_RCV_REF(pAdapter) != 0) && (LoopCounter-- > 0)) {
		mdelay(2);
	}

	/* Gate off JAGCore 3 clock domains */
	GlobalPmCSR.bits.pm_sysclk_gate = 0;
	GlobalPmCSR.bits.pm_txclk_gate = 0;
	GlobalPmCSR.bits.pm_rxclk_gate = 0;
	writel(GlobalPmCSR.value, &pAdapter->CSRAddress->global.pm_csr.value);

	/* Program gigE PHY in to Coma mode */
	GlobalPmCSR.bits.pm_phy_sw_coma = 1;
	writel(GlobalPmCSR.value, &pAdapter->CSRAddress->global.pm_csr.value);

	DBG_LEAVE(et131x_dbginfo);
}

void DisablePhyComa(struct et131x_adapter *pAdapter)
{
	PM_CSR_t GlobalPmCSR;

	DBG_ENTER(et131x_dbginfo);

	GlobalPmCSR.value = readl(&pAdapter->CSRAddress->global.pm_csr.value);

	/* Disable phy_sw_coma register and re-enable JAGCore clocks */
	GlobalPmCSR.bits.pm_sysclk_gate = 1;
	GlobalPmCSR.bits.pm_txclk_gate = 1;
	GlobalPmCSR.bits.pm_rxclk_gate = 1;
	GlobalPmCSR.bits.pm_phy_sw_coma = 0;
	writel(GlobalPmCSR.value, &pAdapter->CSRAddress->global.pm_csr.value);

	/* Restore the GbE PHY speed and duplex modes;
	 * Reset JAGCore; re-configure and initialize JAGCore and gigE PHY
	 */
	pAdapter->AiForceSpeed = pAdapter->PoMgmt.PowerDownSpeed;
	pAdapter->AiForceDpx = pAdapter->PoMgmt.PowerDownDuplex;

	/* Re-initialize the send structures */
	et131x_init_send(pAdapter);

	/* Reset the RFD list and re-start RU  */
	et131x_reset_recv(pAdapter);

	/* Bring the device back to the state it was during init prior to
         * autonegotiation being complete.  This way, when we get the auto-neg
         * complete interrupt, we can complete init by calling ConfigMacREGS2.
         */
	et131x_soft_reset(pAdapter);

	/* setup et1310 as per the documentation ?? */
	et131x_adapter_setup(pAdapter);

	/* Allow Tx to restart */
	MP_CLEAR_FLAG(pAdapter, fMP_ADAPTER_LOWER_POWER);

	/* Need to re-enable Rx. */
	et131x_rx_dma_enable(pAdapter);

	DBG_LEAVE(et131x_dbginfo);
}

