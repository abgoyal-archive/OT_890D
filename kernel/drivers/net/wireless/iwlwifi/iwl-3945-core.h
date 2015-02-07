

#ifndef __iwl_3945_dev_h__
#define __iwl_3945_dev_h__

#define IWL_PCI_DEVICE(dev, subdev, cfg) \
	.vendor = PCI_VENDOR_ID_INTEL,  .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice = (subdev), \
	.driver_data = (kernel_ulong_t)&(cfg)

#define IWL_SKU_G       0x1
#define IWL_SKU_A       0x2

struct iwl_3945_cfg {
	const char *name;
	const char *fw_name_pre;
	const unsigned int ucode_api_max;
	const unsigned int ucode_api_min;
	unsigned int sku;
};

#endif /* __iwl_dev_h__ */
