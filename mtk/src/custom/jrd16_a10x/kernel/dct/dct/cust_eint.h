

#ifndef __CUST_EINTH
#define __CUST_EINTH
#ifdef __cplusplus
extern "C" {
#endif
#define CUST_EINT_POLARITY_LOW              0
#define CUST_EINT_POLARITY_HIGH             1
#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
#define CUST_EINT_EDGE_SENSITIVE            0
#define CUST_EINT_LEVEL_SENSITIVE           1
//////////////////////////////////////////////////////////////////////////////


#define CUST_EINT_MT6326_PMIC_NUM              0
#define CUST_EINT_MT6326_PMIC_DEBOUNCE_CN      0x7ff
#define CUST_EINT_MT6326_PMIC_POLARITY         CUST_EINT_POLARITY_HIGH
#define CUST_EINT_MT6326_PMIC_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_MT6326_PMIC_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define CUST_EINT_BT_NUM              1
#define CUST_EINT_BT_DEBOUNCE_CN      0x0
#define CUST_EINT_BT_POLARITY         CUST_EINT_POLARITY_HIGH
#define CUST_EINT_BT_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_BT_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define CUST_EINT_TOUCH_PANEL_NUM              2
#define CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN      0x0
#define CUST_EINT_TOUCH_PANEL_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_TOUCH_PANEL_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define CUST_EINT_MHALL_NUM              3
#define CUST_EINT_MHALL_DEBOUNCE_CN      0x0
#define CUST_EINT_MHALL_POLARITY         CUST_EINT_POLARITY_HIGH
#define CUST_EINT_MHALL_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_MHALL_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define CUST_EINT_KPD_PWRKEY_NUM              4
#define CUST_EINT_KPD_PWRKEY_DEBOUNCE_CN      0x780
#define CUST_EINT_KPD_PWRKEY_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_KPD_PWRKEY_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_KPD_PWRKEY_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define CUST_EINT_MT5921_WIFI_NUM              5
#define CUST_EINT_MT5921_WIFI_DEBOUNCE_CN      0x0
#define CUST_EINT_MT5921_WIFI_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_MT5921_WIFI_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_MT5921_WIFI_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define CUST_EINT_HEADSET_NUM              7
#define CUST_EINT_HEADSET_DEBOUNCE_CN      0x3f
#define CUST_EINT_HEADSET_POLARITY         CUST_EINT_POLARITY_HIGH
#define CUST_EINT_HEADSET_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_HEADSET_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define CUST_EINT_FM_RDS_NUM              9
#define CUST_EINT_FM_RDS_DEBOUNCE_CN      0x0
#define CUST_EINT_FM_RDS_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_FM_RDS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_FM_RDS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define CUST_EINT_ALS_NUM              16
#define CUST_EINT_ALS_DEBOUNCE_CN      0x0
#define CUST_EINT_ALS_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_ALS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_ALS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define CUST_EINT_OFN_NUM              17
#define CUST_EINT_OFN_DEBOUNCE_CN      0x0
#define CUST_EINT_OFN_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_OFN_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_OFN_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE



//////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
#endif //_CUST_EINT_H


