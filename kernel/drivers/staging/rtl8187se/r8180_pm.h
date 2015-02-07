

#ifdef CONFIG_RTL8180_PM

#ifndef R8180_PM_H
#define R8180_PM_H

#include <linux/types.h>
#include <linux/pci.h>

int rtl8180_save_state (struct pci_dev *dev, u32 state);
int rtl8180_suspend (struct pci_dev *pdev, pm_message_t state);
int rtl8180_resume (struct pci_dev *pdev);
int rtl8180_enable_wake (struct pci_dev *dev, u32 state, int enable);

#endif //R8180_PM_H

#endif // CONFIG_RTL8180_PM
