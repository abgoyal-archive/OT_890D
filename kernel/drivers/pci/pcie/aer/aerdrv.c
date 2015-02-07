

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/pcieport_if.h>

#include "aerdrv.h"
#include "../../pci.h"

#define DRIVER_VERSION "v1.0"
#define DRIVER_AUTHOR "tom.l.nguyen@intel.com"
#define DRIVER_DESC "Root Port Advanced Error Reporting Driver"
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

static int __devinit aer_probe (struct pcie_device *dev,
	const struct pcie_port_service_id *id );
static void aer_remove(struct pcie_device *dev);
static int aer_suspend(struct pcie_device *dev, pm_message_t state)
{return 0;}
static int aer_resume(struct pcie_device *dev) {return 0;}
static pci_ers_result_t aer_error_detected(struct pci_dev *dev,
	enum pci_channel_state error);
static void aer_error_resume(struct pci_dev *dev);
static pci_ers_result_t aer_root_reset(struct pci_dev *dev);

static struct pcie_port_service_id aer_id[] = {
	{
	.vendor 	= PCI_ANY_ID,
	.device 	= PCI_ANY_ID,
	.port_type 	= PCIE_RC_PORT,
	.service_type 	= PCIE_PORT_SERVICE_AER,
	},
	{ /* end: all zeroes */ }
};

static struct pci_error_handlers aer_error_handlers = {
	.error_detected = aer_error_detected,
	.resume = aer_error_resume,
};

static struct pcie_port_service_driver aerdriver = {
	.name		= "aer",
	.id_table	= &aer_id[0],

	.probe		= aer_probe,
	.remove		= aer_remove,

	.suspend	= aer_suspend,
	.resume		= aer_resume,

	.err_handler	= &aer_error_handlers,

	.reset_link	= aer_root_reset,
};

static int pcie_aer_disable;

void pci_no_aer(void)
{
	pcie_aer_disable = 1;	/* has priority over 'forceload' */
}

static irqreturn_t aer_irq(int irq, void *context)
{
	unsigned int status, id;
	struct pcie_device *pdev = (struct pcie_device *)context;
	struct aer_rpc *rpc = get_service_data(pdev);
	int next_prod_idx;
	unsigned long flags;
	int pos;

	pos = pci_find_ext_capability(pdev->port, PCI_EXT_CAP_ID_ERR);
	/*
	 * Must lock access to Root Error Status Reg, Root Error ID Reg,
	 * and Root error producer/consumer index
	 */
	spin_lock_irqsave(&rpc->e_lock, flags);

	/* Read error status */
	pci_read_config_dword(pdev->port, pos + PCI_ERR_ROOT_STATUS, &status);
	if (!(status & ROOT_ERR_STATUS_MASKS)) {
		spin_unlock_irqrestore(&rpc->e_lock, flags);
		return IRQ_NONE;
	}

	/* Read error source and clear error status */
	pci_read_config_dword(pdev->port, pos + PCI_ERR_ROOT_COR_SRC, &id);
	pci_write_config_dword(pdev->port, pos + PCI_ERR_ROOT_STATUS, status);

	/* Store error source for later DPC handler */
	next_prod_idx = rpc->prod_idx + 1;
	if (next_prod_idx == AER_ERROR_SOURCES_MAX)
		next_prod_idx = 0;
	if (next_prod_idx == rpc->cons_idx) {
		/*
		 * Error Storm Condition - possibly the same error occurred.
		 * Drop the error.
		 */
		spin_unlock_irqrestore(&rpc->e_lock, flags);
		return IRQ_HANDLED;
	}
	rpc->e_sources[rpc->prod_idx].status =  status;
	rpc->e_sources[rpc->prod_idx].id = id;
	rpc->prod_idx = next_prod_idx;
	spin_unlock_irqrestore(&rpc->e_lock, flags);

	/*  Invoke DPC handler */
	schedule_work(&rpc->dpc_handler);

	return IRQ_HANDLED;
}

static struct aer_rpc* aer_alloc_rpc(struct pcie_device *dev)
{
	struct aer_rpc *rpc;

	if (!(rpc = kzalloc(sizeof(struct aer_rpc),
		GFP_KERNEL)))
		return NULL;

	/*
	 * Initialize Root lock access, e_lock, to Root Error Status Reg,
	 * Root Error ID Reg, and Root error producer/consumer index.
	 */
	spin_lock_init(&rpc->e_lock);

	rpc->rpd = dev;
	INIT_WORK(&rpc->dpc_handler, aer_isr);
	rpc->prod_idx = rpc->cons_idx = 0;
	mutex_init(&rpc->rpc_mutex);
	init_waitqueue_head(&rpc->wait_release);

	/* Use PCIE bus function to store rpc into PCIE device */
	set_service_data(dev, rpc);

	return rpc;
}

static void aer_remove(struct pcie_device *dev)
{
	struct aer_rpc *rpc = get_service_data(dev);

	if (rpc) {
		/* If register interrupt service, it must be free. */
		if (rpc->isr)
			free_irq(dev->irq, dev);

		wait_event(rpc->wait_release, rpc->prod_idx == rpc->cons_idx);

		aer_delete_rootport(rpc);
		set_service_data(dev, NULL);
	}
}

static int __devinit aer_probe (struct pcie_device *dev,
				const struct pcie_port_service_id *id )
{
	int status;
	struct aer_rpc *rpc;
	struct device *device = &dev->device;

	/* Init */
	if ((status = aer_init(dev)))
		return status;

	/* Alloc rpc data structure */
	if (!(rpc = aer_alloc_rpc(dev))) {
		dev_printk(KERN_DEBUG, device, "alloc rpc failed\n");
		aer_remove(dev);
		return -ENOMEM;
	}

	/* Request IRQ ISR */
	if ((status = request_irq(dev->irq, aer_irq, IRQF_SHARED, "aerdrv",
				dev))) {
		dev_printk(KERN_DEBUG, device, "request IRQ failed\n");
		aer_remove(dev);
		return status;
	}

	rpc->isr = 1;

	aer_enable_rootport(rpc);

	return status;
}

static pci_ers_result_t aer_root_reset(struct pci_dev *dev)
{
	u16 p2p_ctrl;
	u32 status;
	int pos;

	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_ERR);

	/* Disable Root's interrupt in response to error messages */
	pci_write_config_dword(dev, pos + PCI_ERR_ROOT_COMMAND, 0);

	/* Assert Secondary Bus Reset */
	pci_read_config_word(dev, PCI_BRIDGE_CONTROL, &p2p_ctrl);
	p2p_ctrl |= PCI_CB_BRIDGE_CTL_CB_RESET;
	pci_write_config_word(dev, PCI_BRIDGE_CONTROL, p2p_ctrl);

	/* De-assert Secondary Bus Reset */
	p2p_ctrl &= ~PCI_CB_BRIDGE_CTL_CB_RESET;
	pci_write_config_word(dev, PCI_BRIDGE_CONTROL, p2p_ctrl);

	/*
	 * System software must wait for at least 100ms from the end
	 * of a reset of one or more device before it is permitted
	 * to issue Configuration Requests to those devices.
	 */
	msleep(200);
	dev_printk(KERN_DEBUG, &dev->dev, "Root Port link has been reset\n");

	/* Enable Root Port's interrupt in response to error messages */
	pci_read_config_dword(dev, pos + PCI_ERR_ROOT_STATUS, &status);
	pci_write_config_dword(dev, pos + PCI_ERR_ROOT_STATUS, status);
	pci_write_config_dword(dev,
		pos + PCI_ERR_ROOT_COMMAND,
		ROOT_PORT_INTR_ON_MESG_MASK);

	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t aer_error_detected(struct pci_dev *dev,
			enum pci_channel_state error)
{
	/* Root Port has no impact. Always recovers. */
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static void aer_error_resume(struct pci_dev *dev)
{
	int pos;
	u32 status, mask;
	u16 reg16;

	/* Clean up Root device status */
	pos = pci_find_capability(dev, PCI_CAP_ID_EXP);
	pci_read_config_word(dev, pos + PCI_EXP_DEVSTA, &reg16);
	pci_write_config_word(dev, pos + PCI_EXP_DEVSTA, reg16);

	/* Clean AER Root Error Status */
	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_ERR);
	pci_read_config_dword(dev, pos + PCI_ERR_UNCOR_STATUS, &status);
	pci_read_config_dword(dev, pos + PCI_ERR_UNCOR_SEVER, &mask);
	if (dev->error_state == pci_channel_io_normal)
		status &= ~mask; /* Clear corresponding nonfatal bits */
	else
		status &= mask; /* Clear corresponding fatal bits */
	pci_write_config_dword(dev, pos + PCI_ERR_UNCOR_STATUS, status);
}

static int __init aer_service_init(void)
{
	if (pcie_aer_disable)
		return -ENXIO;
	return pcie_port_service_register(&aerdriver);
}

static void __exit aer_service_exit(void)
{
	pcie_port_service_unregister(&aerdriver);
}

module_init(aer_service_init);
module_exit(aer_service_exit);
