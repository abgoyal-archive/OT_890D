

#ifndef _IXGBE_COMMON_H_
#define _IXGBE_COMMON_H_

#include "ixgbe_type.h"

s32 ixgbe_init_ops_generic(struct ixgbe_hw *hw);
s32 ixgbe_init_hw_generic(struct ixgbe_hw *hw);
s32 ixgbe_start_hw_generic(struct ixgbe_hw *hw);
s32 ixgbe_clear_hw_cntrs_generic(struct ixgbe_hw *hw);
s32 ixgbe_read_pba_num_generic(struct ixgbe_hw *hw, u32 *pba_num);
s32 ixgbe_get_mac_addr_generic(struct ixgbe_hw *hw, u8 *mac_addr);
s32 ixgbe_get_bus_info_generic(struct ixgbe_hw *hw);
s32 ixgbe_stop_adapter_generic(struct ixgbe_hw *hw);

s32 ixgbe_led_on_generic(struct ixgbe_hw *hw, u32 index);
s32 ixgbe_led_off_generic(struct ixgbe_hw *hw, u32 index);

s32 ixgbe_init_eeprom_params_generic(struct ixgbe_hw *hw);
s32 ixgbe_read_eeprom_generic(struct ixgbe_hw *hw, u16 offset, u16 *data);
s32 ixgbe_read_eeprom_bit_bang_generic(struct ixgbe_hw *hw, u16 offset,
                                       u16 *data);
s32 ixgbe_validate_eeprom_checksum_generic(struct ixgbe_hw *hw,
                                           u16 *checksum_val);
s32 ixgbe_update_eeprom_checksum_generic(struct ixgbe_hw *hw);

s32 ixgbe_set_rar_generic(struct ixgbe_hw *hw, u32 index, u8 *addr, u32 vmdq,
                          u32 enable_addr);
s32 ixgbe_clear_rar_generic(struct ixgbe_hw *hw, u32 index);
s32 ixgbe_init_rx_addrs_generic(struct ixgbe_hw *hw);
s32 ixgbe_update_mc_addr_list_generic(struct ixgbe_hw *hw, u8 *mc_addr_list,
                                      u32 mc_addr_count,
                                      ixgbe_mc_addr_itr func);
s32 ixgbe_update_uc_addr_list_generic(struct ixgbe_hw *hw, u8 *addr_list,
                                      u32 addr_count, ixgbe_mc_addr_itr func);
s32 ixgbe_enable_mc_generic(struct ixgbe_hw *hw);
s32 ixgbe_disable_mc_generic(struct ixgbe_hw *hw);

s32 ixgbe_validate_mac_addr(u8 *mac_addr);
s32 ixgbe_acquire_swfw_sync(struct ixgbe_hw *hw, u16 mask);
void ixgbe_release_swfw_sync(struct ixgbe_hw *hw, u16 mask);
s32 ixgbe_disable_pcie_master(struct ixgbe_hw *hw);

s32 ixgbe_read_analog_reg8_generic(struct ixgbe_hw *hw, u32 reg, u8 *val);
s32 ixgbe_write_analog_reg8_generic(struct ixgbe_hw *hw, u32 reg, u8 val);

#define IXGBE_WRITE_REG(a, reg, value) writel((value), ((a)->hw_addr + (reg)))

#define IXGBE_READ_REG(a, reg) readl((a)->hw_addr + (reg))

#define IXGBE_WRITE_REG_ARRAY(a, reg, offset, value) (\
    writel((value), ((a)->hw_addr + (reg) + ((offset) << 2))))

#define IXGBE_READ_REG_ARRAY(a, reg, offset) (\
    readl((a)->hw_addr + (reg) + ((offset) << 2)))

#define IXGBE_WRITE_FLUSH(a) IXGBE_READ_REG(a, IXGBE_STATUS)

#ifdef DEBUG
#define hw_dbg(hw, format, arg...) \
printk(KERN_DEBUG, "%s: " format, ixgbe_get_hw_dev_name(hw), ##arg);
#else
static inline int __attribute__ ((format (printf, 2, 3)))
hw_dbg(struct ixgbe_hw *hw, const char *format, ...)
{
	return 0;
}
#endif

#endif /* IXGBE_COMMON */
