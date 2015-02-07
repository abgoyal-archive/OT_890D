

#include <linux/module.h>
#include <linux/pci.h>

static int pci_stub_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	return 0;
}

static struct pci_driver stub_driver = {
	.name		= "pci-stub",
	.id_table	= NULL,	/* only dynamic id's */
	.probe		= pci_stub_probe,
};

static int __init pci_stub_init(void)
{
	return pci_register_driver(&stub_driver);
}

static void __exit pci_stub_exit(void)
{
	pci_unregister_driver(&stub_driver);
}

module_init(pci_stub_init);
module_exit(pci_stub_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Wright <chrisw@sous-sol.org>");
