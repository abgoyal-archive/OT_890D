

#ifndef __iwl_5000_hw_h__
#define __iwl_5000_hw_h__

#define IWL50_RTC_INST_UPPER_BOUND		(0x020000)
#define IWL50_RTC_DATA_UPPER_BOUND		(0x80C000)
#define IWL50_RTC_INST_SIZE (IWL50_RTC_INST_UPPER_BOUND - RTC_INST_LOWER_BOUND)
#define IWL50_RTC_DATA_SIZE (IWL50_RTC_DATA_UPPER_BOUND - RTC_DATA_LOWER_BOUND)

/* EEPROM */
#define IWL_5000_EEPROM_IMG_SIZE			2048

#define IWL50_CMD_FIFO_NUM                 7
#define IWL50_NUM_QUEUES                  20
#define IWL50_NUM_AMPDU_QUEUES		  10
#define IWL50_FIRST_AMPDU_QUEUE		  10

/* Fixed (non-configurable) rx data from phy */

struct iwl5000_scd_bc_tbl {
	__le16 tfd_offset[TFD_QUEUE_BC_SIZE];
} __attribute__ ((packed));


#endif /* __iwl_5000_hw_h__ */

