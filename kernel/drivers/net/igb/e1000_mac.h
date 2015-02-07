

#ifndef _E1000_MAC_H_
#define _E1000_MAC_H_

#include "e1000_hw.h"

#include "e1000_phy.h"
#include "e1000_nvm.h"
#include "e1000_defines.h"

s32  igb_blink_led(struct e1000_hw *hw);
s32  igb_check_for_copper_link(struct e1000_hw *hw);
s32  igb_cleanup_led(struct e1000_hw *hw);
s32  igb_config_fc_after_link_up(struct e1000_hw *hw);
s32  igb_disable_pcie_master(struct e1000_hw *hw);
s32  igb_force_mac_fc(struct e1000_hw *hw);
s32  igb_get_auto_rd_done(struct e1000_hw *hw);
s32  igb_get_bus_info_pcie(struct e1000_hw *hw);
s32  igb_get_hw_semaphore(struct e1000_hw *hw);
s32  igb_get_speed_and_duplex_copper(struct e1000_hw *hw, u16 *speed,
				       u16 *duplex);
s32  igb_id_led_init(struct e1000_hw *hw);
s32  igb_led_off(struct e1000_hw *hw);
s32  igb_setup_link(struct e1000_hw *hw);
s32  igb_validate_mdi_setting(struct e1000_hw *hw);
s32  igb_write_8bit_ctrl_reg(struct e1000_hw *hw, u32 reg,
			       u32 offset, u8 data);

void igb_clear_hw_cntrs_base(struct e1000_hw *hw);
void igb_clear_vfta(struct e1000_hw *hw);
void igb_config_collision_dist(struct e1000_hw *hw);
void igb_mta_set(struct e1000_hw *hw, u32 hash_value);
void igb_put_hw_semaphore(struct e1000_hw *hw);
void igb_rar_set(struct e1000_hw *hw, u8 *addr, u32 index);
s32  igb_check_alt_mac_addr(struct e1000_hw *hw);
void igb_remove_device(struct e1000_hw *hw);
void igb_reset_adaptive(struct e1000_hw *hw);
void igb_update_adaptive(struct e1000_hw *hw);
void igb_write_vfta(struct e1000_hw *hw, u32 offset, u32 value);

bool igb_enable_mng_pass_thru(struct e1000_hw *hw);

enum e1000_mng_mode {
	e1000_mng_mode_none = 0,
	e1000_mng_mode_asf,
	e1000_mng_mode_pt,
	e1000_mng_mode_ipmi,
	e1000_mng_mode_host_if_only
};

#define E1000_FACTPS_MNGCG    0x20000000

#define E1000_FWSM_MODE_MASK  0xE
#define E1000_FWSM_MODE_SHIFT 1

#define E1000_MNG_DHCP_COMMAND_TIMEOUT       10
#define E1000_MNG_DHCP_COOKIE_STATUS_VLAN    0x2

#define E1000_HICR_EN              0x01  /* Enable bit - RO */
/* Driver sets this bit when done to put command in RAM */
#define E1000_HICR_C               0x02

extern void e1000_init_function_pointers_82575(struct e1000_hw *hw);
extern u32 igb_hash_mc_addr(struct e1000_hw *hw, u8 *mc_addr);

#endif
