

#ifndef _PMIC_DRV_H
#define _PMIC_DRV_H


#include <mach/MT6326PMIC_sw.h>


//[BT]
typedef enum
{
    BT_1_3 = VBT_1_3,
    BT_1_5 = VBT_1_5,
    BT_1_8 = VBT_1_8,
    BT_2_5 = VBT_2_5,
    BT_2_8 = VBT_2_8,
    BT_3_0 = VBT_3_0,
    BT_3_3 = VBT_3_3
}BT_sel_enum;

extern void BT_Enable(kal_bool enable);
extern void BT_Set_Volt(BT_sel_enum volt);

//[CAM1_D]
typedef enum
{
    CAM1_D_1_3 = VCAMD_1_3,
    CAM1_D_1_5 = VCAMD_1_5,
    CAM1_D_1_8 = VCAMD_1_8,
    CAM1_D_2_5 = VCAMD_2_5,
    CAM1_D_2_8 = VCAMD_2_8,
    CAM1_D_3_0 = VCAMD_3_0,
    CAM1_D_3_3 = VCAMD_3_3
}CAM1_D_sel_enum;

extern void CAM1_D_Enable(kal_bool enable);
extern void CAM1_D_Set_Volt(CAM1_D_sel_enum volt);

//[SIM]
typedef enum
{
    SIM_1_3 = VSIM_1_3,
    SIM_1_5 = VSIM_1_5,
    SIM_1_8 = VSIM_1_8,
    SIM_2_5 = VSIM_2_5,
    SIM_2_8 = VSIM_2_8,
    SIM_3_0 = VSIM_3_0,
    SIM_3_3 = VSIM_3_3
}SIM_sel_enum;

extern void SIM_Enable(kal_bool enable);
extern void SIM_Set_Volt(SIM_sel_enum volt);

//[CAM1_A]
typedef enum
{
    CAM1_A_1_5 = VCAMA_1_5,
    CAM1_A_1_8 = VCAMA_1_8,
    CAM1_A_2_5 = VCAMA_2_5,
    CAM1_A_2_8 = VCAMA_2_8
}CAM1_A_sel_enum;

extern void CAM1_A_Enable(kal_bool enable);
extern void CAM1_A_Set_Volt(CAM1_A_sel_enum volt);

//[RF]
typedef enum
{
    RF_2_8 = VRF_2_8
}RF_sel_enum;

extern void RF_Enable(kal_bool enable);
extern void RF_Set_Volt(RF_sel_enum volt);

//[WIFI_V1]
typedef enum
{
    WIFI_V1_2_5 = VWIFI3V3_2_5,
    WIFI_V1_2_8 = VWIFI3V3_2_8,
    WIFI_V1_3_0 = VWIFI3V3_3_0,
    WIFI_V1_3_3 = VWIFI3V3_3_3
}WIFI_V1_sel_enum;

extern void WIFI_V1_Enable(kal_bool enable);
extern void WIFI_V1_Set_Volt(WIFI_V1_sel_enum volt);

//[WIFI_V2]
typedef enum
{
    WIFI_V2_2_5 = VWIFI2V8_2_5,
    WIFI_V2_2_8 = VWIFI2V8_2_8,
    WIFI_V2_3_0 = VWIFI2V8_3_0,
    WIFI_V2_3_3 = VWIFI2V8_3_3
}WIFI_V2_sel_enum;

extern void WIFI_V2_Enable(kal_bool enable);
extern void WIFI_V2_Set_Volt(WIFI_V2_sel_enum volt);

//[GPS]
typedef enum
{
    GPS_2_5 = V3GTX_2_5,
    GPS_2_8 = V3GTX_2_8,
    GPS_3_0 = V3GTX_3_0,
    GPS_3_3 = V3GTX_3_3
}GPS_sel_enum;


extern void GPS_Enable(kal_bool enable);
extern void GPS_Set_Volt(GPS_sel_enum volt);

//[V3GRX]
typedef enum
{
	V3G_RX_2_5 = V3GRX_2_5,
    V3G_RX_2_8 = V3GRX_2_8,
    V3G_RX_3_0 = V3GRX_3_0,
    V3G_RX_3_3 = V3GRX_3_3,
}V3GRX_sel_enum;

extern void V3GRX_Enable(kal_bool enable);
extern void V3GRX_Set_Volt(V3GRX_sel_enum volt);


//[SIM2]
typedef enum
{
    SIM2_1_3 = VGP_1_3,
    SIM2_1_5 = VGP_1_5,
    SIM2_1_8 = VGP_1_8,
    SIM2_2_5 = VGP_2_5,
    SIM2_2_8 = VGP_2_8,
    SIM2_3_0 = VGP_3_0,
    SIM2_3_3 = VGP_3_3
}SIM2_sel_enum;

extern void SIM2_Enable(kal_bool enable);
extern void SIM2_Set_Volt(SIM2_sel_enum volt);

//[GP2]
typedef enum
{
    GP2_1_3 = VGP2_1_3,
    GP2_1_5 = VGP2_1_5,
    GP2_1_8 = VGP2_1_8,
    GP2_2_5 = VGP2_2_5,
    GP2_2_8 = VGP2_2_8,
    GP2_3_0 = VGP2_3_0,
    GP2_3_3 = VGP2_3_3
}GP2_sel_enum;

extern void GP2_Enable(kal_bool enable);
extern void GP2_Set_Volt(GP2_sel_enum volt);


//[MC]
typedef enum
{
    MC_2_8 = VSDIO_2_8,
    MC_3_0 = VSDIO_3_0
}MC_sel_enum;

extern void MC_Enable(kal_bool enable);
extern void MC_Set_Volt(MC_sel_enum volt);

//[USB]
typedef enum
{
    USB_3_3 = VUSB_3_3
}USB_sel_enum;

extern void USB_Enable(kal_bool enable);
extern void USB_Set_Volt(USB_sel_enum volt);

extern void VCORE2_Enable(kal_bool enable);


//PMIC config result
#define SELECT_BT_VOLTAGE  BT_1_8
#define SELECT_CAM1_D_VOLTAGE  CAM1_D_1_8
#define SELECT_SIM_VOLTAGE  SIM_1_8
#define SELECT_CAM1_A_VOLTAGE  CAM1_A_2_8
#define SELECT_RF_VOLTAGE  RF_2_8
#define SELECT_WIFI_V1_VOLTAGE  WIFI_V1_3_3
#define SELECT_WIFI_V2_VOLTAGE  WIFI_V2_2_8
#define SELECT_GPS_VOLTAGE  GPS_2_8
#define SELECT_SIM2_VOLTAGE  SIM2_1_8
#define SELECT_MC_VOLTAGE  MC_2_8
#define SELECT_USB_VOLTAGE  USB_3_3

//Cal voltage step : 0~15
#define SELECT_BT_CAL_VOLT_STEP  7
#define SELECT_CAM1_D_CAL_VOLT_STEP  7
#define SELECT_SIM_CAL_VOLT_STEP  7
#define SELECT_CAM1_A_CAL_VOLT_STEP  7
#define SELECT_RF_CAL_VOLT_STEP  7
#define SELECT_WIFI_V1_CAL_VOLT_STEP  7
#define SELECT_WIFI_V2_CAL_VOLT_STEP  7
#define SELECT_GPS_CAL_VOLT_STEP  7
#define SELECT_SIM2_CAL_VOLT_STEP  7
#define SELECT_MC_CAL_VOLT_STEP  7
#define SELECT_USB_CAL_VOLT_STEP  7

//For PMIC driver use
void pmic_init(void);


#endif /* _PMIC_DRV_H */


