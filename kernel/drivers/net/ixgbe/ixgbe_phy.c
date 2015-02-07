

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "ixgbe_common.h"
#include "ixgbe_phy.h"

static bool ixgbe_validate_phy_addr(struct ixgbe_hw *hw, u32 phy_addr);
static enum ixgbe_phy_type ixgbe_get_phy_type_from_id(u32 phy_id);
static s32 ixgbe_get_phy_id(struct ixgbe_hw *hw);

s32 ixgbe_identify_phy_generic(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_ERR_PHY_ADDR_INVALID;
	u32 phy_addr;

	if (hw->phy.type == ixgbe_phy_unknown) {
		for (phy_addr = 0; phy_addr < IXGBE_MAX_PHY_ADDR; phy_addr++) {
			if (ixgbe_validate_phy_addr(hw, phy_addr)) {
				hw->phy.addr = phy_addr;
				ixgbe_get_phy_id(hw);
				hw->phy.type =
				        ixgbe_get_phy_type_from_id(hw->phy.id);
				status = 0;
				break;
			}
		}
	} else {
		status = 0;
	}

	return status;
}

static bool ixgbe_validate_phy_addr(struct ixgbe_hw *hw, u32 phy_addr)
{
	u16 phy_id = 0;
	bool valid = false;

	hw->phy.addr = phy_addr;
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_HIGH,
	                     IXGBE_MDIO_PMA_PMD_DEV_TYPE, &phy_id);

	if (phy_id != 0xFFFF && phy_id != 0x0)
		valid = true;

	return valid;
}

static s32 ixgbe_get_phy_id(struct ixgbe_hw *hw)
{
	u32 status;
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;

	status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_HIGH,
	                              IXGBE_MDIO_PMA_PMD_DEV_TYPE,
	                              &phy_id_high);

	if (status == 0) {
		hw->phy.id = (u32)(phy_id_high << 16);
		status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_LOW,
		                              IXGBE_MDIO_PMA_PMD_DEV_TYPE,
		                              &phy_id_low);
		hw->phy.id |= (u32)(phy_id_low & IXGBE_PHY_REVISION_MASK);
		hw->phy.revision = (u32)(phy_id_low & ~IXGBE_PHY_REVISION_MASK);
	}
	return status;
}

static enum ixgbe_phy_type ixgbe_get_phy_type_from_id(u32 phy_id)
{
	enum ixgbe_phy_type phy_type;

	switch (phy_id) {
	case TN1010_PHY_ID:
		phy_type = ixgbe_phy_tn;
		break;
	case QT2022_PHY_ID:
		phy_type = ixgbe_phy_qt;
		break;
	case ATH_PHY_ID:
		phy_type = ixgbe_phy_nl;
		break;
	default:
		phy_type = ixgbe_phy_unknown;
		break;
	}

	return phy_type;
}

s32 ixgbe_reset_phy_generic(struct ixgbe_hw *hw)
{
	/*
	 * Perform soft PHY reset to the PHY_XS.
	 * This will cause a soft reset to the PHY
	 */
	return hw->phy.ops.write_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
	                             IXGBE_MDIO_PHY_XS_DEV_TYPE,
	                             IXGBE_MDIO_PHY_XS_RESET);
}

s32 ixgbe_read_phy_reg_generic(struct ixgbe_hw *hw, u32 reg_addr,
                               u32 device_type, u16 *phy_data)
{
	u32 command;
	u32 i;
	u32 data;
	s32 status = 0;
	u16 gssr;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		gssr = IXGBE_GSSR_PHY1_SM;
	else
		gssr = IXGBE_GSSR_PHY0_SM;

	if (ixgbe_acquire_swfw_sync(hw, gssr) != 0)
		status = IXGBE_ERR_SWFW_SYNC;

	if (status == 0) {
		/* Setup and write the address cycle command */
		command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
		           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
		           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
		           (IXGBE_MSCA_ADDR_CYCLE | IXGBE_MSCA_MDI_COMMAND));

		IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

		/*
		 * Check every 10 usec to see if the address cycle completed.
		 * The MDI Command bit will clear when the operation is
		 * complete
		 */
		for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
			udelay(10);

			command = IXGBE_READ_REG(hw, IXGBE_MSCA);

			if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
				break;
		}

		if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
			hw_dbg(hw, "PHY address command did not complete.\n");
			status = IXGBE_ERR_PHY;
		}

		if (status == 0) {
			/*
			 * Address cycle complete, setup and write the read
			 * command
			 */
			command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
			           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
			           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
			           (IXGBE_MSCA_READ | IXGBE_MSCA_MDI_COMMAND));

			IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

			/*
			 * Check every 10 usec to see if the address cycle
			 * completed. The MDI Command bit will clear when the
			 * operation is complete
			 */
			for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
				udelay(10);

				command = IXGBE_READ_REG(hw, IXGBE_MSCA);

				if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
					break;
			}

			if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
				hw_dbg(hw, "PHY read command didn't complete\n");
				status = IXGBE_ERR_PHY;
			} else {
				/*
				 * Read operation is complete.  Get the data
				 * from MSRWD
				 */
				data = IXGBE_READ_REG(hw, IXGBE_MSRWD);
				data >>= IXGBE_MSRWD_READ_DATA_SHIFT;
				*phy_data = (u16)(data);
			}
		}

		ixgbe_release_swfw_sync(hw, gssr);
	}

	return status;
}

s32 ixgbe_write_phy_reg_generic(struct ixgbe_hw *hw, u32 reg_addr,
                                u32 device_type, u16 phy_data)
{
	u32 command;
	u32 i;
	s32 status = 0;
	u16 gssr;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		gssr = IXGBE_GSSR_PHY1_SM;
	else
		gssr = IXGBE_GSSR_PHY0_SM;

	if (ixgbe_acquire_swfw_sync(hw, gssr) != 0)
		status = IXGBE_ERR_SWFW_SYNC;

	if (status == 0) {
		/* Put the data in the MDI single read and write data register*/
		IXGBE_WRITE_REG(hw, IXGBE_MSRWD, (u32)phy_data);

		/* Setup and write the address cycle command */
		command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
		           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
		           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
		           (IXGBE_MSCA_ADDR_CYCLE | IXGBE_MSCA_MDI_COMMAND));

		IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

		/*
		 * Check every 10 usec to see if the address cycle completed.
		 * The MDI Command bit will clear when the operation is
		 * complete
		 */
		for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
			udelay(10);

			command = IXGBE_READ_REG(hw, IXGBE_MSCA);

			if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
				break;
		}

		if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
			hw_dbg(hw, "PHY address cmd didn't complete\n");
			status = IXGBE_ERR_PHY;
		}

		if (status == 0) {
			/*
			 * Address cycle complete, setup and write the write
			 * command
			 */
			command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
			           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
			           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
			           (IXGBE_MSCA_WRITE | IXGBE_MSCA_MDI_COMMAND));

			IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

			/*
			 * Check every 10 usec to see if the address cycle
			 * completed. The MDI Command bit will clear when the
			 * operation is complete
			 */
			for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
				udelay(10);

				command = IXGBE_READ_REG(hw, IXGBE_MSCA);

				if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
					break;
			}

			if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
				hw_dbg(hw, "PHY address cmd didn't complete\n");
				status = IXGBE_ERR_PHY;
			}
		}

		ixgbe_release_swfw_sync(hw, gssr);
	}

	return status;
}

s32 ixgbe_setup_phy_link_generic(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_NOT_IMPLEMENTED;
	u32 time_out;
	u32 max_time_out = 10;
	u16 autoneg_reg = IXGBE_MII_AUTONEG_REG;

	/*
	 * Set advertisement settings in PHY based on autoneg_advertised
	 * settings. If autoneg_advertised = 0, then advertise default values
	 * tnx devices cannot be "forced" to a autoneg 10G and fail.  But can
	 * for a 1G.
	 */
	hw->phy.ops.read_reg(hw, IXGBE_MII_SPEED_SELECTION_REG,
	                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE, &autoneg_reg);

	if (hw->phy.autoneg_advertised == IXGBE_LINK_SPEED_1GB_FULL)
		autoneg_reg &= 0xEFFF; /* 0 in bit 12 is 1G operation */
	else
		autoneg_reg |= 0x1000; /* 1 in bit 12 is 10G/1G operation */

	hw->phy.ops.write_reg(hw, IXGBE_MII_SPEED_SELECTION_REG,
	                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE, autoneg_reg);

	/* Restart PHY autonegotiation and wait for completion */
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE, &autoneg_reg);

	autoneg_reg |= IXGBE_MII_RESTART;

	hw->phy.ops.write_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE, autoneg_reg);

	/* Wait for autonegotiation to finish */
	for (time_out = 0; time_out < max_time_out; time_out++) {
		udelay(10);
		/* Restart PHY autonegotiation and wait for completion */
		status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_STATUS,
		                              IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                              &autoneg_reg);

		autoneg_reg &= IXGBE_MII_AUTONEG_COMPLETE;
		if (autoneg_reg == IXGBE_MII_AUTONEG_COMPLETE) {
			status = 0;
			break;
		}
	}

	if (time_out == max_time_out)
		status = IXGBE_ERR_LINK_SETUP;

	return status;
}

s32 ixgbe_setup_phy_link_speed_generic(struct ixgbe_hw *hw,
                                       ixgbe_link_speed speed,
                                       bool autoneg,
                                       bool autoneg_wait_to_complete)
{

	/*
	 * Clear autoneg_advertised and set new values based on input link
	 * speed.
	 */
	hw->phy.autoneg_advertised = 0;

	if (speed & IXGBE_LINK_SPEED_10GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_10GB_FULL;

	if (speed & IXGBE_LINK_SPEED_1GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_1GB_FULL;

	/* Setup link based on the new speed settings */
	hw->phy.ops.setup_link(hw);

	return 0;
}

s32 ixgbe_reset_phy_nl(struct ixgbe_hw *hw)
{
	u16 phy_offset, control, eword, edata, block_crc;
	bool end_data = false;
	u16 list_offset, data_offset;
	u16 phy_data = 0;
	s32 ret_val = 0;
	u32 i;

	hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
	                     IXGBE_MDIO_PHY_XS_DEV_TYPE, &phy_data);

	/* reset the PHY and poll for completion */
	hw->phy.ops.write_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
	                      IXGBE_MDIO_PHY_XS_DEV_TYPE,
	                      (phy_data | IXGBE_MDIO_PHY_XS_RESET));

	for (i = 0; i < 100; i++) {
		hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
		                     IXGBE_MDIO_PHY_XS_DEV_TYPE, &phy_data);
		if ((phy_data & IXGBE_MDIO_PHY_XS_RESET) == 0)
			break;
		msleep(10);
	}

	if ((phy_data & IXGBE_MDIO_PHY_XS_RESET) != 0) {
		hw_dbg(hw, "PHY reset did not complete.\n");
		ret_val = IXGBE_ERR_PHY;
		goto out;
	}

	/* Get init offsets */
	ret_val = ixgbe_get_sfp_init_sequence_offsets(hw, &list_offset,
	                                              &data_offset);
	if (ret_val != 0)
		goto out;

	ret_val = hw->eeprom.ops.read(hw, data_offset, &block_crc);
	data_offset++;
	while (!end_data) {
		/*
		 * Read control word from PHY init contents offset
		 */
		ret_val = hw->eeprom.ops.read(hw, data_offset, &eword);
		control = (eword & IXGBE_CONTROL_MASK_NL) >>
		           IXGBE_CONTROL_SHIFT_NL;
		edata = eword & IXGBE_DATA_MASK_NL;
		switch (control) {
		case IXGBE_DELAY_NL:
			data_offset++;
			hw_dbg(hw, "DELAY: %d MS\n", edata);
			msleep(edata);
			break;
		case IXGBE_DATA_NL:
			hw_dbg(hw, "DATA:  \n");
			data_offset++;
			hw->eeprom.ops.read(hw, data_offset++,
			                    &phy_offset);
			for (i = 0; i < edata; i++) {
				hw->eeprom.ops.read(hw, data_offset, &eword);
				hw->phy.ops.write_reg(hw, phy_offset,
				                      IXGBE_TWINAX_DEV, eword);
				hw_dbg(hw, "Wrote %4.4x to %4.4x\n", eword,
				       phy_offset);
				data_offset++;
				phy_offset++;
			}
			break;
		case IXGBE_CONTROL_NL:
			data_offset++;
			hw_dbg(hw, "CONTROL: \n");
			if (edata == IXGBE_CONTROL_EOL_NL) {
				hw_dbg(hw, "EOL\n");
				end_data = true;
			} else if (edata == IXGBE_CONTROL_SOL_NL) {
				hw_dbg(hw, "SOL\n");
			} else {
				hw_dbg(hw, "Bad control value\n");
				ret_val = IXGBE_ERR_PHY;
				goto out;
			}
			break;
		default:
			hw_dbg(hw, "Bad control type\n");
			ret_val = IXGBE_ERR_PHY;
			goto out;
		}
	}

out:
	return ret_val;
}

s32 ixgbe_identify_sfp_module_generic(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_ERR_PHY_ADDR_INVALID;
	u32 vendor_oui = 0;
	u8 identifier = 0;
	u8 comp_codes_1g = 0;
	u8 comp_codes_10g = 0;
	u8 oui_bytes[4] = {0, 0, 0, 0};
	u8 transmission_media = 0;

	status = hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_IDENTIFIER,
	                                     &identifier);

	if (status == IXGBE_ERR_SFP_NOT_PRESENT) {
		hw->phy.sfp_type = ixgbe_sfp_type_not_present;
		goto out;
	}

	if (identifier == IXGBE_SFF_IDENTIFIER_SFP) {
		hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_1GBE_COMP_CODES,
		                            &comp_codes_1g);
		hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_10GBE_COMP_CODES,
		                            &comp_codes_10g);
		hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_TRANSMISSION_MEDIA,
		                            &transmission_media);

		/* ID Module
		 * =========
		 * 0	SFP_DA_CU
		 * 1	SFP_SR
		 * 2	SFP_LR
		 */
		if (transmission_media & IXGBE_SFF_TWIN_AX_CAPABLE)
			hw->phy.sfp_type = ixgbe_sfp_type_da_cu;
		else if (comp_codes_10g & IXGBE_SFF_10GBASESR_CAPABLE)
			hw->phy.sfp_type = ixgbe_sfp_type_sr;
		else if (comp_codes_10g & IXGBE_SFF_10GBASELR_CAPABLE)
			hw->phy.sfp_type = ixgbe_sfp_type_lr;
		else
			hw->phy.sfp_type = ixgbe_sfp_type_unknown;

		/* Determine PHY vendor */
		if (hw->phy.type == ixgbe_phy_unknown) {
			hw->phy.id = identifier;
			hw->phy.ops.read_i2c_eeprom(hw,
			                            IXGBE_SFF_VENDOR_OUI_BYTE0,
			                            &oui_bytes[0]);
			hw->phy.ops.read_i2c_eeprom(hw,
			                            IXGBE_SFF_VENDOR_OUI_BYTE1,
			                            &oui_bytes[1]);
			hw->phy.ops.read_i2c_eeprom(hw,
			                            IXGBE_SFF_VENDOR_OUI_BYTE2,
			                            &oui_bytes[2]);

			vendor_oui =
			  ((oui_bytes[0] << IXGBE_SFF_VENDOR_OUI_BYTE0_SHIFT) |
			   (oui_bytes[1] << IXGBE_SFF_VENDOR_OUI_BYTE1_SHIFT) |
			   (oui_bytes[2] << IXGBE_SFF_VENDOR_OUI_BYTE2_SHIFT));

			switch (vendor_oui) {
			case IXGBE_SFF_VENDOR_OUI_TYCO:
				if (transmission_media &
				    IXGBE_SFF_TWIN_AX_CAPABLE)
					hw->phy.type = ixgbe_phy_tw_tyco;
				break;
			case IXGBE_SFF_VENDOR_OUI_FTL:
				hw->phy.type = ixgbe_phy_sfp_ftl;
				break;
			case IXGBE_SFF_VENDOR_OUI_AVAGO:
				hw->phy.type = ixgbe_phy_sfp_avago;
				break;
			default:
				if (transmission_media &
				    IXGBE_SFF_TWIN_AX_CAPABLE)
					hw->phy.type = ixgbe_phy_tw_unknown;
				else
					hw->phy.type = ixgbe_phy_sfp_unknown;
				break;
			}
		}
		status = 0;
	}

out:
	return status;
}

s32 ixgbe_get_sfp_init_sequence_offsets(struct ixgbe_hw *hw,
                                        u16 *list_offset,
                                        u16 *data_offset)
{
	u16 sfp_id;

	if (hw->phy.sfp_type == ixgbe_sfp_type_unknown)
		return IXGBE_ERR_SFP_NOT_SUPPORTED;

	if (hw->phy.sfp_type == ixgbe_sfp_type_not_present)
		return IXGBE_ERR_SFP_NOT_PRESENT;

	if ((hw->device_id == IXGBE_DEV_ID_82598_SR_DUAL_PORT_EM) &&
	    (hw->phy.sfp_type == ixgbe_sfp_type_da_cu))
		return IXGBE_ERR_SFP_NOT_SUPPORTED;

	/* Read offset to PHY init contents */
	hw->eeprom.ops.read(hw, IXGBE_PHY_INIT_OFFSET_NL, list_offset);

	if ((!*list_offset) || (*list_offset == 0xFFFF))
		return IXGBE_ERR_PHY;

	/* Shift offset to first ID word */
	(*list_offset)++;

	/*
	 * Find the matching SFP ID in the EEPROM
	 * and program the init sequence
	 */
	hw->eeprom.ops.read(hw, *list_offset, &sfp_id);

	while (sfp_id != IXGBE_PHY_INIT_END_NL) {
		if (sfp_id == hw->phy.sfp_type) {
			(*list_offset)++;
			hw->eeprom.ops.read(hw, *list_offset, data_offset);
			if ((!*data_offset) || (*data_offset == 0xFFFF)) {
				hw_dbg(hw, "SFP+ module not supported\n");
				return IXGBE_ERR_SFP_NOT_SUPPORTED;
			} else {
				break;
			}
		} else {
			(*list_offset) += 2;
			if (hw->eeprom.ops.read(hw, *list_offset, &sfp_id))
				return IXGBE_ERR_PHY;
		}
	}

	if (sfp_id == IXGBE_PHY_INIT_END_NL) {
		hw_dbg(hw, "No matching SFP+ module found\n");
		return IXGBE_ERR_SFP_NOT_SUPPORTED;
	}

	return 0;
}

s32 ixgbe_check_phy_link_tnx(struct ixgbe_hw *hw, ixgbe_link_speed *speed,
                             bool *link_up)
{
	s32 status = 0;
	u32 time_out;
	u32 max_time_out = 10;
	u16 phy_link = 0;
	u16 phy_speed = 0;
	u16 phy_data = 0;

	/* Initialize speed and link to default case */
	*link_up = false;
	*speed = IXGBE_LINK_SPEED_10GB_FULL;

	/*
	 * Check current speed and link status of the PHY register.
	 * This is a vendor specific register and may have to
	 * be changed for other copper PHYs.
	 */
	for (time_out = 0; time_out < max_time_out; time_out++) {
		udelay(10);
		status = hw->phy.ops.read_reg(hw,
		                        IXGBE_MDIO_VENDOR_SPECIFIC_1_STATUS,
		                        IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
		                        &phy_data);
		phy_link = phy_data &
		           IXGBE_MDIO_VENDOR_SPECIFIC_1_LINK_STATUS;
		phy_speed = phy_data &
		            IXGBE_MDIO_VENDOR_SPECIFIC_1_SPEED_STATUS;
		if (phy_link == IXGBE_MDIO_VENDOR_SPECIFIC_1_LINK_STATUS) {
			*link_up = true;
			if (phy_speed ==
			    IXGBE_MDIO_VENDOR_SPECIFIC_1_SPEED_STATUS)
				*speed = IXGBE_LINK_SPEED_1GB_FULL;
			break;
		}
	}

	return status;
}

s32 ixgbe_get_phy_firmware_version_tnx(struct ixgbe_hw *hw,
                                       u16 *firmware_version)
{
	s32 status = 0;

	status = hw->phy.ops.read_reg(hw, TNX_FW_REV,
	                              IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
	                              firmware_version);

	return status;
}

