

#include "pmic_drv_nodct.h"


//[BT]
void BT_Enable(kal_bool enable) {
	pmic_vbt_enable(enable);
}
void BT_Set_Volt(BT_sel_enum volt) {
	pmic_vbt_sel((kal_uint32)volt);
}

//[CAM1_D]
void CAM1_D_Enable(kal_bool enable) {
	pmic_vcamd_enable(enable);
}
void CAM1_D_Set_Volt(CAM1_D_sel_enum volt) {
	pmic_vcamd_sel((kal_uint32)volt);
}

//[SIM]
void SIM_Enable(kal_bool enable) {
	pmic_vsim_enable(enable);
}
void SIM_Set_Volt(SIM_sel_enum volt) {
	pmic_vsim_sel((kal_uint32)volt);
}

//[CAM1_A]
void CAM1_A_Enable(kal_bool enable) {
	pmic_vcama_enable(enable);
}
void CAM1_A_Set_Volt(CAM1_A_sel_enum volt) {
	pmic_vcama_sel((kal_uint32)volt);
}

//[RF]
void RF_Enable(kal_bool enable) {
	pmic_vrf_enable(enable);
}
void RF_Set_Volt(RF_sel_enum volt) {
}

//[WIFI_V1]
void WIFI_V1_Enable(kal_bool enable) {
	pmic_vwifi3v3_enable(enable);
}
void WIFI_V1_Set_Volt(WIFI_V1_sel_enum volt) {
	pmic_vwifi3v3_sel((kal_uint32)volt);
}

//[WIFI_V2]
void WIFI_V2_Enable(kal_bool enable) {
	pmic_vwifi2v8_enable(enable);
}
void WIFI_V2_Set_Volt(WIFI_V2_sel_enum volt) {
	pmic_vwifi2v8_sel((kal_uint32)volt);
}

//[GPS]
void GPS_Enable(kal_bool enable) {
	pmic_v3gtx_on_sel(V3GTX_ENABLE_WITH_V3GTX_EN);
	pmic_v3gtx_enable(enable);
}
void GPS_Set_Volt(GPS_sel_enum volt) {
	pmic_v3gtx_sel((kal_uint32)volt);
}

//[3GRX]
void V3GRX_Enable(kal_bool enable) {
	pmic_v3grx_on_sel(V3GRX_ENABLE_WITH_V3GRX_EN);
	pmic_v3grx_enable(enable);
}
void V3GRX_Set_Volt(V3GRX_sel_enum volt) {
	pmic_v3grx_sel((kal_uint32)volt);
}

//[SIM2]
void SIM2_Enable(kal_bool enable) {
	pmic_vgp_enable(enable);
}
void SIM2_Set_Volt(SIM2_sel_enum volt) {
    pmic_vgp_sel((kal_uint32)volt);
}

//[GP2]
void GP2_Enable(kal_bool enable) {
	pmic_vgp2_on_sel(VGP2_ENABLE_WITH_VGP2_EN);
	pmic_vgp2_enable(enable);
}
void GP2_Set_Volt(GP2_sel_enum volt) {
    pmic_vgp2_sel((kal_uint32)volt);
	pmic_vgp2_on_sel(VGP2_ENABLE_WITH_VGP2_EN);
}

//[MC]
void MC_Enable(kal_bool enable) {
	pmic_vsdio_enable(enable);
}
void MC_Set_Volt(MC_sel_enum volt) {
    pmic_vsdio_sel(volt);	
}

//[USB]
void USB_Enable(kal_bool enable) {
	pmic_vusb_enable(enable);
}
void USB_Set_Volt(USB_sel_enum volt) {
}

extern ssize_t  mt6326_VCORE_2_set_1_2(void);

//[VCORE2]
void VCORE2_Enable(kal_bool enable) {
    pmic_vcore2_on_sel(VCORE2_ENABLE_WITH_VCORE2_EN);
    pmic_vcore2_enable(enable);
	mt6326_VCORE_2_set_1_2();
}



//For PMIC driver use
void pmic_init(void) {

	printk("DCT - pmic_init() start\r\n");

    //[BT]
    pmic_vbt_sel(SELECT_BT_VOLTAGE);
	//pmic_vbt_cal(SELECT_BT_CAL_VOLT_STEP);
    //[CAM1_D]
    pmic_vcamd_sel(SELECT_CAM1_D_VOLTAGE);
    //pmic_vcamd_cal(SELECT_CAM1_D_CAL_VOLT_STEP);
    //[SIM]
    pmic_vsim_sel(SELECT_SIM_VOLTAGE);
    //pmic_vsim_cal(SELECT_SIM_CAL_VOLT_STEP);
    //[CAM1_A]
    pmic_vcama_sel(SELECT_CAM1_A_VOLTAGE);
    //pmic_vcama_cal(SELECT_CAM1_A_CAL_VOLT_STEP);
    //[RF]
    pmic_vrf_sel(SELECT_RF_VOLTAGE);
    //pmic_vrf_cal(SELECT_RF_CAL_VOLT_STEP);
    //[WIFI_V1]
    pmic_vwifi3v3_sel(SELECT_WIFI_V1_VOLTAGE);
    //pmic_vwifi3v3_cal(SELECT_WIFI_V1_CAL_VOLT_STEP);
    //[WIFI_V2]
    pmic_vwifi2v8_sel(SELECT_WIFI_V2_VOLTAGE);
    //pmic_vwifi2v8_cal(SELECT_WIFI_V2_CAL_VOLT_STEP);
    //[GPS]
    pmic_v3gtx_sel(SELECT_GPS_VOLTAGE);
    //pmic_v3gtx_cal(SELECT_GPS_CAL_VOLT_STEP);
    //[SIM2]
    pmic_vgp_sel(SELECT_SIM2_VOLTAGE);
    //pmic_vgp_cal(SELECT_SIM2_CAL_VOLT_STEP);
    //[MC]
    pmic_vsdio_sel(SELECT_MC_VOLTAGE);
    //pmic_vsdio_cal(SELECT_MC_CAL_VOLT_STEP);
    //[USB]
    pmic_vusb_sel(SELECT_USB_VOLTAGE);
    //pmic_vusb_cal(SELECT_USB_CAL_VOLT_STEP);

	printk("DCT - pmic_init() end\r\n");
}

/* End of pmic_drv.c */


