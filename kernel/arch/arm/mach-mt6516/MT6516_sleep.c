



#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/suspend.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_ap_config.h>
#include <mach/mt6516_apmcusys.h>
#include <mach/mt6516_graphsys.h>
#include <mach/memory.h>
#include <mach/mt6516_gpio.h>
#include <mach/irqs.h>
#include <mach/ccci.h>
#include <mach/ccci_md.h>
#include <mach/mtkpm.h>

#include <linux/leds.h>
#include <mach/mt65xx_leds.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>

#include <mach/mt6516_auxadc_sw.h>
#include <mach/mt6516_auxadc_hw.h>

#include 	"../../../drivers/power/pmic6326_hw.h"
#include 	"../../../drivers/power/pmic6326_sw.h"
#include 	"../../../drivers/video/mtk/disp_drv.h"



/* Debug message event */
#define DBG_PM_NONE		    0x00000000	/* No event */
#define DBG_PM_PMIC			0x00000001	/* PMIC related event */
#define DBG_PM_GPIO 		0x00000002	/* GPIO related event */
#define DBG_PM_MSGS			0x00000004	/* MSG related event */
#define DBG_PM_SUSP         0x00000008	/* Suspend related event */
#define DBG_PM_ENTER      	0x00000010	/* Function Entry */
#define DBG_PM_ALL			0xffffffff

#define DBG_PM_MASK	   (DBG_PM_ALL)

#define GPIO_MASK_CAM		0x1
#define GPIO_MASK_MOTION	0x2
#define GPIO_MASK_I2C1		0x4
#define GPIO_MASK_TP		0x8
#define GPIO_MASK_LED		0x10
#define GPIO_MASK_6326		0x20
#define GPIO_MASK_BTLAN		0x40
#define GPIO_MASK_KPD		0x80
#define GPIO_MASK_EARPHONE	0x100
#define GPIO_MASK_CONNECTOR	0x200
#define GPIO_MASK_FM		0x400
#define GPIO_MASK_3326		0x800
#define GPIO_MASK_AMPLIFIER	0x1000
#define PMIC_MASK_PARAM		0x2000
#define PMIC_REG_SET		0x4000


#if 1
extern UINT32 PM_DBG_FLAG;

#define MSG(evt, fmt, args...) \
do {	\
	if (((DBG_PM_##evt) & DBG_PM_MASK) && PM_DBG_FLAG) { \
		printk(fmt, ##args); \
	} \
} while(0)

#define MSG_FUNC_ENTRY(f)	MSG(ENTER, "<PM FUNC>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)	   do{}while(0)
#endif



/*MIPI*/
#define     TVENC               (0xF0089008)
#define     MIPI_PD_B00         (0xF0060b00)
#define     MIPI_PD_B04         (0xF0060b04)
#define     MIPI_PD_B08         (0xF0060b08)
#define     MIPI_PD_B0C         (0xF0060b0c)
#define     MIPI_PD_B10         (0xF0060b10)
#define     MIPI_PD_B14         (0xF0060b14)
#define     MIPI_PD_B18         (0xF0060b18)
#define     MIPI_PD_B1C         (0xF0060b1c)
#define     MIPI_PD_B40         (0xF0060b40)
#define     MIPI_PD_B44         (0xF0060b44)
#define     MIPI_PD_B48         (0xF0060b48)
#define     MIPI_PD_B4C         (0xF0060b4c)
#define 	MIPI_PD_B40         (0xF0060b40)
#define     MIPI_PD_04C         (0xF006004c)
/*AFE_PDN*/
#define     PDN_AFE_AAPDN       (0xF0060208)
#define     PDN_AFE_AAC_NEW     (0xF006020C)
#define     PDN_AFE_AAC_CON1    (0xF0060210)

/*CCIF*/
#define CCIF_START ((volatile unsigned int *)(CCIF_BASE + 0x0008))


/* WAKE UP SOURCE*/
#define WS_NOT_IN_USE           0
#define WS_SM                   1
#define WS_KP                   2
#define WS_EINT                 3
#define WS_RTC                  4
#define WS_MSDC                 5
#define WS_GPT                  6
#define WS_TP                   7
#define WS_LOWBAT               8
#define WS_CCIF                 9
#define WS_END                  10



/* AP SLEEP STATUS*/
#define PAUSE_RQST              (1<<4)
#define PAUSE_INT               (1<<5)
#define PAUSE_CPL               (1<<6)
#define SETTLE_CPL              (1<<7)
#define PAUSE_ABORT             (1<<8)

#define RET_WAKE_REQ			0x1
#define RET_WAKE_INTR			0x2
#define RET_WAKE_TIMEOUT		0x3
#define RET_WAKE_ABORT			0x4
#define RET_WAKE_OTHERS			0x5


#define DANGER_TICK_COUNT       (25)
#define SLEEP_DURATION          (0x7FFFF)
#define AP_CLK_SETTLE_COUNT     (0xA0)
BOOL suspend_lock = FALSE;
UINT32 g_ChipVer;
UINT32 irqMask_L = 0;
UINT32 irqMask_H = 0;
UINT32 dwMissingSleep = 0;
UINT32 g_GPIO_mask = 0;
UINT32 g_GPIO_save[50] = {0};
UINT32 g_MCU_PDN_save = 0;
UINT32 g_MM_PDN_save = 0;
UINT32 g_Sleep_lock = 0;
UINT32 g_Power_optimize_Enable = 1;
UINT32 bMTCMOS = 1;

static UINT32 BK_MIPI_PD_B00;
static UINT32 BK_MIPI_PD_B04;
static UINT32 BK_MIPI_PD_B08;
static UINT32 BK_MIPI_PD_B0C;
static UINT32 BK_MIPI_PD_B10;
static UINT32 BK_MIPI_PD_B14;
static UINT32 BK_MIPI_PD_B18;
static UINT32 BK_MIPI_PD_B1C;
static UINT32 BK_MIPI_PD_B40;
static UINT32 BK_MIPI_PD_04C;


static void (*saved_idle)(void);

#ifdef AP_MD_EINT_SHARE_DATA
static struct cores_sleep_info *pSLP_INFO;
static ccci_cores_sleep_info_base_req sleep_info_func = NULL;
#endif

void mt6516_pm_Maskinterrupt(void);
void mt6516_pm_SetWakeSrc(UINT32 u4Mask);


extern suspend_state_t requested_suspend_state ; 
extern ROOTBUS_HW g_MT6516_BusHW ;

extern int enter_state(suspend_state_t state);
extern UINT32 MT6516_CPUSuspend(UINT32 FuncAddr, UINT32 TCMAddr);
extern void MT6516_EnableITCM(UINT32 ITCMAddr);
extern void mt6516_pm_SuspendEnter(void);
extern void MT6516_IRQUnmask(unsigned int line);
extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);

extern ssize_t  mt6326_bl_Enable(void);
extern ssize_t  mt6326_bl_Disable(void);

extern ssize_t  mt6326_check_power(void);

extern void MT6516_hwPMInit(void);

extern ssize_t  mt6326_disable_VGP(void);
extern ssize_t  mt6326_disable_VGP2(void);
extern ssize_t  mt6326_disable_VUSB(void);
extern ssize_t  mt6326_disable_VSIM(void);
extern ssize_t  mt6326_disable_VRF(void);
extern ssize_t  mt6326_disable_V3GRX(void);
extern ssize_t  mt6326_disable_V3GTX(void);
extern ssize_t  mt6326_disable_VWIFI3V3(void);
extern ssize_t  mt6326_disable_VWIFI2V8(void);
extern ssize_t  mt6326_disable_VBT(void);
extern ssize_t  mt6326_disable_VSDIO(void);
extern ssize_t  mt6326_disable_VCORE_2(void);
extern ssize_t  mt6326_disable_VCAM_A(void);
extern ssize_t  mt6326_disable_VCAM_D(void);
extern ssize_t  mt6326_disable_VPA(void);
extern ssize_t  mt6326_disable_VTCXO(void);


extern ssize_t  mt6326_enable_VGP(void);
extern ssize_t  mt6326_enable_VGP2(void);
extern ssize_t  mt6326_enable_VUSB(void);
extern ssize_t  mt6326_enable_VSIM(void);
extern ssize_t  mt6326_enable_VRF(void);
extern ssize_t  mt6326_enable_V3GRX(void);
extern ssize_t  mt6326_enable_V3GTX(void);
extern ssize_t  mt6326_enable_VWIFI3V3(void);
extern ssize_t  mt6326_enable_VWIFI2V8(void);
extern ssize_t  mt6326_enable_VBT(void);
extern ssize_t  mt6326_enable_VSDIO(void);
extern ssize_t  mt6326_enable_VCORE_2(void);
extern ssize_t  mt6326_enable_VCAM_A(void);
extern ssize_t  mt6326_enable_VCAM_D(void);
extern ssize_t  mt6326_enable_VPA(void);
extern ssize_t  mt6326_enable_VTCXO(void);


extern ssize_t  mt6326_VCORE_1_set_1_0(void);
extern ssize_t  mt6326_VCORE_1_set_1_10(void);
extern ssize_t  mt6326_VCORE_1_set_1_2(void);
extern ssize_t  mt6326_VCORE_1_set_1_3(void);
extern ssize_t  mt6326_VCORE_1_set_0_95(void);
extern ssize_t  mt6326_VCORE_1_set_0_90(void);


extern UINT32 SetARM9Freq(ARM9_FREQ_DIV_ENUM ARM9_div);

extern BOOL hwDisableSubsys(MT6516_SUBSYS subsysId);
extern BOOL hwEnableSubsys(MT6516_SUBSYS subsysId);

extern void pmic_v3gtx_on_sel(v3gtx_on_sel_enum sel);
extern void pmic_v3grx_on_sel(v3grx_on_sel_enum sel);
extern void pmic_vgp2_on_sel(vgp2_on_sel_enum sel);
extern void pmic_vrf_on_sel(vrf_on_sel_enum sel);
extern void pmic_vtcxo_on_sel(vtcxo_on_sel_enum sel);

extern kal_bool pmic_chrdet_status(void);

extern INT32 mt6516_EINT_SetMDShareInfo(void);
extern void MT6516_AP_DisableMCUMEMPDN (MCUMEM_PDNCONA_MODE mode);

extern ssize_t mt6326_kpled_Enable(void);
extern ssize_t mt6326_kpled_Disable(void);
extern ssize_t mt6326_kpled_dim_duty_Full(void);
extern ssize_t mt6326_kpled_dim_duty_0(void);
extern size_t mt6326_vibr_Enable(void);
extern size_t mt6326_vibr_Disable(void);
extern void rtc_bbpu_power_down(void);

extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);

extern void pmic_boost1_enable(kal_bool enable);
extern void pmic_boost2_enable(kal_bool enable);
extern void pmic_vbus_enable(kal_bool enable);
extern void pmic_flash_enable(kal_bool enable);
extern void pmic_spkl_enable(kal_bool enable);

extern ssize_t mt6326_read_byte(u8 cmd, u8 *returnData);
extern ssize_t mt6326_write_byte(u8 cmd, u8 writeData);

extern void pmic_vbus_enable(kal_bool enable);



extern void MT6516_CPUIdle(UINT32 SlowIdle, UINT32 AHB_ADDR);


extern void MT6516_GRAPH1SYS_EnablePDN (GRAPH1SYS_PDN_MODE mode);
extern void MT6516_GRAPH1SYS_DisablePDN (GRAPH1SYS_PDN_MODE mode);

#ifdef AP_MD_EINT_SHARE_DATA
void mt6516_pm_register_sleep_info_func(ccci_cores_sleep_info_base_req func)
{
	printk("mt6516_pm_register_sleep_info_func\n");
	sleep_info_func = func;
}

void mt6516_pm_unregister_sleep_info_func(void)
{
	sleep_info_func = NULL;
}

void mt6516_pm_SleepWorkAround(void)
{
    int iT1 = 0, iT2 = 0;
    //int iTimeDiff = 0;
    int iSleepRTC = 0;
    int iDiff1 = 0, iDiff2 = 0;
    int ienter = 0, ileave = 0;
    int iapwakeup;
    UINT32 i, u4Address = 0;

    if (sleep_info_func == NULL)
	    return;

    u4Address = sleep_info_func((void *)u4Address);

    if (u4Address)
        pSLP_INFO = (struct cores_sleep_info *) u4Address;
    else
        return ; 
            
    /* If we are arround the T1 or T2 within 25 ticks? */
	/* flag for MD to inform AP if MD is sleeping or not ? */
    if (pSLP_INFO->MD_Sleep & 0xFF)
    {
        /* Pass from MD site, T1*/
        iT1 = (int) pSLP_INFO->RTC_MD_Settle_OK;
        /* Pass from MD site, T2*/
        iT2 = (int) pSLP_INFO->RTC_MD_WakeUp;    
        /* When will AP enter sleep*/        
        ienter = iSleepRTC = (int) (( DRV_Reg16(RTCCOUNT_M) & 0xFF) << 16 | DRV_Reg16(RTCCOUNT_L));
        iDiff1 = iT1 - iSleepRTC ; // T1 - T3
        iDiff2 = iT2 - iSleepRTC ; // T2 - T3

        /* Calculate difference*/
        if (iDiff1 < 0)
            iDiff1 = -iDiff1;
        if (iDiff2 < 0)
            iDiff2 = -iDiff2;
		/*not used ?*/
        if (iDiff1 < DANGER_TICK_COUNT)
            dwMissingSleep += DANGER_TICK_COUNT - iDiff1 ;
        if (iDiff2 < DANGER_TICK_COUNT)
            dwMissingSleep += DANGER_TICK_COUNT - iDiff2;
        
        do
        {
            /* When will AP leave sleep*/        
            ileave = iSleepRTC = (int) ((DRV_Reg16(RTCCOUNT_M) & 0xFF) << 16 | 
                DRV_Reg16(RTCCOUNT_L) ) ; 
            iDiff1 = iT1 - iSleepRTC;
            iDiff2 = iT2 - iSleepRTC;
            if (iDiff1 < 0) iDiff1 = -iDiff1;
            if (iDiff2 < 0) iDiff2 = -iDiff2 ;
        } while ((iDiff1<DANGER_TICK_COUNT) || (iDiff2<DANGER_TICK_COUNT)) ;
    }
    else
    {
        ienter = ileave = iSleepRTC = ((DRV_Reg16(RTCCOUNT_M) & 0xFF) << 16 
            | DRV_Reg16(RTCCOUNT_L));
    }

	//printk("pSLP_INFO->AP_Sleep = 0x%x\n\r", pSLP_INFO->AP_Sleep);
	//printk("pSLP_INFO->RTC_AP_WakeUp = 0x%x\n\r", pSLP_INFO->RTC_AP_WakeUp);
	//printk("pSLP_INFO->AP_SettleTime = 0x%x\n\r", pSLP_INFO->AP_SettleTime);
	//printk("pSLP_INFO->MD_Sleep = 0x%x\n\r", pSLP_INFO->MD_Sleep);
	//printk("pSLP_INFO->RTC_MD_WakeUp = 0x%x\n\r", pSLP_INFO->RTC_MD_WakeUp);
	//printk("pSLP_INFO->RTC_MD_Settle_OK = 0x%x\n\r\n\r", pSLP_INFO->RTC_MD_Settle_OK);

	/*check RTC wrape*/
	if((iSleepRTC + SLEEP_DURATION) <= 0xFFFFFF)
    	pSLP_INFO->RTC_AP_WakeUp = (iSleepRTC + SLEEP_DURATION) ;
	else
    	pSLP_INFO->RTC_AP_WakeUp = ((iSleepRTC + SLEEP_DURATION) - 0xFFFFFF) ;
			
	iapwakeup = pSLP_INFO->RTC_AP_WakeUp & 0xFFFFFF;
    pSLP_INFO->AP_SettleTime = (DWORD)AP_CLK_SETTLE_COUNT ;
    for (i=0; i < 1*400*1000; i++)
    {
        if (0 == ( DRV_Reg32(CCIF_START) & 0xff) ) 
            break;
    }	
    if (pSLP_INFO)
        pSLP_INFO->AP_Sleep = TRUE ; 
}

void mt6516_pm_SleepWorkAroundUp(void)
{
    if (pSLP_INFO)
        pSLP_INFO->AP_Sleep = FALSE ; 
}
#endif

void mt6516_GPIO_set(UINT32 gpio_num)
{
	mt_set_gpio_mode(gpio_num, 0);
	mt_set_gpio_dir(gpio_num,  GPIO_DIR_IN);
	mt_set_gpio_pull_enable(gpio_num, TRUE);
	mt_set_gpio_pull_select(gpio_num, GPIO_PULL_DOWN);
}

void mt6516_GPIO_restore(UINT32 gpio_num)
{
	mt_set_gpio_mode(gpio_num, 1);
	mt_set_gpio_dir(gpio_num,  GPIO_DIR_IN);
	mt_set_gpio_pull_select(gpio_num, GPIO_PULL_UP);
	mt_set_gpio_pull_enable(gpio_num, FALSE);
}

void mt6516_pm_PMIC_param(BOOL enable)
{
	pmic_boost1_enable(enable);
	pmic_boost2_enable(enable);
	pmic_vbus_enable(enable);
	pmic_flash_enable(enable);
	pmic_spkl_enable(enable);

}

int Suspend_power_saving(void)
{
    printk("Uboot_power_saving\n");
    /* TV power down*/
    printk("TV power down\n");
    DRV_ClrReg32(TVENC, 0x13E0);

    /* AFE power down*/
    printk("AFE power down\n");	
    DRV_WriteReg32(PDN_AFE_AAPDN, 0); 
    DRV_WriteReg32(PDN_AFE_AAC_NEW, 0);
    DRV_WriteReg32(PDN_AFE_AAC_CON1, 0x0003);


    /* MIPI power down*/
    printk("MIPI power backup\n");
    BK_MIPI_PD_B00 = DRV_Reg32(MIPI_PD_B00);
    BK_MIPI_PD_B04 = DRV_Reg32(MIPI_PD_B04);
    BK_MIPI_PD_B08 = DRV_Reg32(MIPI_PD_B08);
    BK_MIPI_PD_B0C = DRV_Reg32(MIPI_PD_B0C);
    BK_MIPI_PD_B10 = DRV_Reg32(MIPI_PD_B10);
    BK_MIPI_PD_B14 = DRV_Reg32(MIPI_PD_B14);
    BK_MIPI_PD_B18 = DRV_Reg32(MIPI_PD_B18);
    BK_MIPI_PD_B1C = DRV_Reg32(MIPI_PD_B1C);
    BK_MIPI_PD_B40 = DRV_Reg32(MIPI_PD_B40);
    BK_MIPI_PD_04C = DRV_Reg32(MIPI_PD_04C);

	/* MIPI power down*/
	printk("MIPI power down\n");
	DRV_WriteReg32(MIPI_PD_B00, 0x706);
	DRV_WriteReg32(MIPI_PD_B04, 0x450);
	DRV_WriteReg32(MIPI_PD_B08, 0x2400);
	DRV_WriteReg32(MIPI_PD_B0C, 0x402c);
	DRV_WriteReg32(MIPI_PD_B10, 0x666);
	DRV_WriteReg32(MIPI_PD_B14, 0x0);
	DRV_WriteReg32(MIPI_PD_B18, 0x0);
	DRV_WriteReg32(MIPI_PD_B1C, 0x0);
	DRV_WriteReg32(MIPI_PD_B40, 0x0);
	DRV_WriteReg32(MIPI_PD_04C, 0x1);
	return 0;    
}    

void Suspend_power_restore(void)
{
	/* MIPI power restore*/
	printk("MIPI power restore\n");
	DRV_WriteReg32(MIPI_PD_B00, BK_MIPI_PD_B00);
	DRV_WriteReg32(MIPI_PD_B04, BK_MIPI_PD_B04);
	DRV_WriteReg32(MIPI_PD_B08, BK_MIPI_PD_B08);
	DRV_WriteReg32(MIPI_PD_B0C, BK_MIPI_PD_B0C);
	DRV_WriteReg32(MIPI_PD_B10, BK_MIPI_PD_B10);
	DRV_WriteReg32(MIPI_PD_B14, BK_MIPI_PD_B14);
	DRV_WriteReg32(MIPI_PD_B18, BK_MIPI_PD_B18);
	DRV_WriteReg32(MIPI_PD_B1C, BK_MIPI_PD_B1C);
	DRV_WriteReg32(MIPI_PD_B40, BK_MIPI_PD_B40);
	DRV_WriteReg32(MIPI_PD_04C, BK_MIPI_PD_04C);
}


void mt6516_GPIO_suspend_set2(void)
{
	g_GPIO_save[0] = DRV_Reg32(0xF0002000);
	g_GPIO_save[1] = DRV_Reg32(0xF0002010);
	g_GPIO_save[2] = DRV_Reg32(0xF0002020);
	g_GPIO_save[3] = DRV_Reg32(0xF0002030);
	g_GPIO_save[4] = DRV_Reg32(0xF0002040);
	g_GPIO_save[5] = DRV_Reg32(0xF0002050);
	g_GPIO_save[6] = DRV_Reg32(0xF0002060);
	g_GPIO_save[7] = DRV_Reg32(0xF0002070);
	g_GPIO_save[8] = DRV_Reg32(0xF0002080);
	g_GPIO_save[9] = DRV_Reg32(0xF0002090);
	
	g_GPIO_save[10] = DRV_Reg32(0xF0002100);
	g_GPIO_save[11] = DRV_Reg32(0xF0002110);
	g_GPIO_save[12] = DRV_Reg32(0xF0002120);
	g_GPIO_save[13] = DRV_Reg32(0xF0002130);
	g_GPIO_save[14] = DRV_Reg32(0xF0002140);
	g_GPIO_save[15] = DRV_Reg32(0xF0002150);
	g_GPIO_save[16] = DRV_Reg32(0xF0002160);
	g_GPIO_save[17] = DRV_Reg32(0xF0002170);
	g_GPIO_save[18] = DRV_Reg32(0xF0002180);
	g_GPIO_save[19] = DRV_Reg32(0xF0002190);
	
	g_GPIO_save[20] = DRV_Reg32(0xF0002200);
	g_GPIO_save[21] = DRV_Reg32(0xF0002210);
	g_GPIO_save[22] = DRV_Reg32(0xF0002220);
	g_GPIO_save[23] = DRV_Reg32(0xF0002230);
	g_GPIO_save[24] = DRV_Reg32(0xF0002240);
	g_GPIO_save[25] = DRV_Reg32(0xF0002250);
	g_GPIO_save[26] = DRV_Reg32(0xF0002260);
	g_GPIO_save[27] = DRV_Reg32(0xF0002270);
	g_GPIO_save[28] = DRV_Reg32(0xF0002280);
	g_GPIO_save[29] = DRV_Reg32(0xF0002290);
	
	g_GPIO_save[30] = DRV_Reg32(0xF0002600);
	g_GPIO_save[31] = DRV_Reg32(0xF0002610);
	g_GPIO_save[32] = DRV_Reg32(0xF0002620);
	g_GPIO_save[33] = DRV_Reg32(0xF0002630);
	g_GPIO_save[34] = DRV_Reg32(0xF0002640);
	g_GPIO_save[35] = DRV_Reg32(0xF0002650);
	g_GPIO_save[36] = DRV_Reg32(0xF0002660);
	g_GPIO_save[37] = DRV_Reg32(0xF0002670);
	g_GPIO_save[38] = DRV_Reg32(0xF0002680);
	g_GPIO_save[39] = DRV_Reg32(0xF0002690);
	g_GPIO_save[40] = DRV_Reg32(0xF00026A0);
	g_GPIO_save[41] = DRV_Reg32(0xF00026B0);
	g_GPIO_save[42] = DRV_Reg32(0xF00026C0);
	g_GPIO_save[43] = DRV_Reg32(0xF00026D0);
	g_GPIO_save[44] = DRV_Reg32(0xF00026E0);
	g_GPIO_save[45] = DRV_Reg32(0xF00026F0);
	g_GPIO_save[46] = DRV_Reg32(0xF0002700);
	g_GPIO_save[47] = DRV_Reg32(0xF0002710);
	g_GPIO_save[48] = DRV_Reg32(0xF0002720);
#if 0
    MSG(MSGS,"GPIO_DIR1 *0xF0002000 = 0x%x\n\r",DRV_Reg32(0xF0002000));
    MSG(MSGS,"GPIO_DIR2 *0xF0002010 = 0x%x\n\r",DRV_Reg32(0xF0002010));
    MSG(MSGS,"GPIO_DIR3 *0xF0002020 = 0x%x\n\r",DRV_Reg32(0xF0002020));
    MSG(MSGS,"GPIO_DIR4 *0xF0002030 = 0x%x\n\r",DRV_Reg32(0xF0002030));
    MSG(MSGS,"GPIO_DIR5 *0xF0002040 = 0x%x\n\r",DRV_Reg32(0xF0002040));
    MSG(MSGS,"GPIO_DIR6 *0xF0002050 = 0x%x\n\r",DRV_Reg32(0xF0002050));
    MSG(MSGS,"GPIO_DIR7 *0xF0002060 = 0x%x\n\r",DRV_Reg32(0xF0002060));
    MSG(MSGS,"GPIO_DIR8 *0xF0002070 = 0x%x\n\r",DRV_Reg32(0xF0002070));
    MSG(MSGS,"GPIO_DIR9 *0xF0002080 = 0x%x\n\r",DRV_Reg32(0xF0002080));
    MSG(MSGS,"GPIO_DIR10 *0xF0002090 = 0x%x\n\r",DRV_Reg32(0xF0002090));

    MSG(MSGS,"GPIO_PULLEN1 *0xF0002100 = 0x%x\n\r",DRV_Reg32(0xF0002100));
    MSG(MSGS,"GPIO_PULLEN2 *0xF0002110 = 0x%x\n\r",DRV_Reg32(0xF0002110));
    MSG(MSGS,"GPIO_PULLEN3 *0xF0002120 = 0x%x\n\r",DRV_Reg32(0xF0002120));
    MSG(MSGS,"GPIO_PULLEN4 *0xF0002130 = 0x%x\n\r",DRV_Reg32(0xF0002130));
    MSG(MSGS,"GPIO_PULLEN5 *0xF0002140 = 0x%x\n\r",DRV_Reg32(0xF0002140));
    MSG(MSGS,"GPIO_PULLEN6 *0xF0002150 = 0x%x\n\r",DRV_Reg32(0xF0002150));
    MSG(MSGS,"GPIO_PULLEN7 *0xF0002160 = 0x%x\n\r",DRV_Reg32(0xF0002160));
    MSG(MSGS,"GPIO_PULLEN8 *0xF0002170 = 0x%x\n\r",DRV_Reg32(0xF0002170));
    MSG(MSGS,"GPIO_PULLEN9 *0xF0002180 = 0x%x\n\r",DRV_Reg32(0xF0002180));
    MSG(MSGS,"GPIO_PULLEN10 *0xF0002190 = 0x%x\n\r",DRV_Reg32(0xF0002190));

    MSG(MSGS,"GPIO_PULLSEL1 *0xF0002200 = 0x%x\n\r",DRV_Reg32(0xF0002200));
    MSG(MSGS,"GPIO_PULLSEL2 *0xF0002210 = 0x%x\n\r",DRV_Reg32(0xF0002210));
    MSG(MSGS,"GPIO_PULLSEL3 *0xF0002220 = 0x%x\n\r",DRV_Reg32(0xF0002220));
    MSG(MSGS,"GPIO_PULLSEL4 *0xF0002230 = 0x%x\n\r",DRV_Reg32(0xF0002230));
    MSG(MSGS,"GPIO_PULLSEL5 *0xF0002240 = 0x%x\n\r",DRV_Reg32(0xF0002240));
    MSG(MSGS,"GPIO_PULLSEL6 *0xF0002250 = 0x%x\n\r",DRV_Reg32(0xF0002250));
    MSG(MSGS,"GPIO_PULLSEL7 *0xF0002260 = 0x%x\n\r",DRV_Reg32(0xF0002260));
    MSG(MSGS,"GPIO_PULLSEL8 *0xF0002270 = 0x%x\n\r",DRV_Reg32(0xF0002270));
    MSG(MSGS,"GPIO_PULLSEL9 *0xF0002280 = 0x%x\n\r",DRV_Reg32(0xF0002280));
    MSG(MSGS,"GPIO_PULLSEL10 *0xF0002290 = 0x%x\n\r",DRV_Reg32(0xF0002290));

    MSG(MSGS,"GPIO_mode1 *0xF0002600 = 0x%x\n\r",DRV_Reg32(0xF0002600));
    MSG(MSGS,"GPIO_mode2 *0xF0002610 = 0x%x\n\r",DRV_Reg32(0xF0002610));
    MSG(MSGS,"GPIO_mode3 *0xF0002620 = 0x%x\n\r",DRV_Reg32(0xF0002620));
    MSG(MSGS,"GPIO_mode4 *0xF0002630 = 0x%x\n\r",DRV_Reg32(0xF0002630));
    MSG(MSGS,"GPIO_mode5 *0xF0002640 = 0x%x\n\r",DRV_Reg32(0xF0002640));
    MSG(MSGS,"GPIO_mode6 *0xF0002650 = 0x%x\n\r",DRV_Reg32(0xF0002650));
    MSG(MSGS,"GPIO_mode7 *0xF0002660 = 0x%x\n\r",DRV_Reg32(0xF0002660));
    MSG(MSGS,"GPIO_mode8 *0xF0002670 = 0x%x\n\r",DRV_Reg32(0xF0002670));
    MSG(MSGS,"GPIO_mode9 *0xF0002680 = 0x%x\n\r",DRV_Reg32(0xF0002680));
    MSG(MSGS,"GPIO_mode10 *0xF0002690 = 0x%x\n\r",DRV_Reg32(0xF0002690));
    MSG(MSGS,"GPIO_mode11 *0xF00026A0 = 0x%x\n\r",DRV_Reg32(0xF00026A0));
    MSG(MSGS,"GPIO_mode12 *0xF00026B0 = 0x%x\n\r",DRV_Reg32(0xF00026B0));
    MSG(MSGS,"GPIO_mode13 *0xF00026C0 = 0x%x\n\r",DRV_Reg32(0xF00026C0));
    MSG(MSGS,"GPIO_mode14 *0xF00026D0 = 0x%x\n\r",DRV_Reg32(0xF00026D0));
    MSG(MSGS,"GPIO_mode15 *0xF00026E0 = 0x%x\n\r",DRV_Reg32(0xF00026E0));
    MSG(MSGS,"GPIO_mode16 *0xF00026F0 = 0x%x\n\r",DRV_Reg32(0xF00026F0));
    MSG(MSGS,"GPIO_mode17 *0xF0002700 = 0x%x\n\r",DRV_Reg32(0xF0002700));
    MSG(MSGS,"GPIO_mode18 *0xF0002710 = 0x%x\n\r",DRV_Reg32(0xF0002710));
    MSG(MSGS,"GPIO_mode19 *0xF0002720 = 0x%x\n\r",DRV_Reg32(0xF0002720));
#endif
	// disable GPIO CG
	//DRV_WriteReg32(0xF0039340,0x40);


#if 1 // direction
	DRV_WriteReg32(0xF0002000 ,0x0 );
	DRV_WriteReg32(0xF0002010 ,0x0 );
	DRV_WriteReg32(0xF0002020 ,0x30 );

	//DRV_WriteReg32(0xF0002030 ,0x10c0 );

	DRV_WriteReg32(0xF0002040 ,0xb818);
	DRV_WriteReg32(0xF0002050 ,0x0 );
	DRV_WriteReg32(0xF0002060 ,0x8 );
	DRV_WriteReg32(0xF0002070 ,0xe08 );
	DRV_WriteReg32(0xF0002080 ,0x18 );
	DRV_WriteReg32(0xF0002090 ,0x0 );
#endif
#if 1 //pull enable
	DRV_WriteReg32(0xF0002100,0xfffd);
	DRV_WriteReg32(0xF0002110,0xff3f);
	DRV_WriteReg32(0xF0002120,0xffff);

	//DRV_WriteReg32(0xF0002130,0x7fff);

	DRV_WriteReg32(0xF0002140,0xf9fd);
	DRV_WriteReg32(0xF0002150,0xffff);
	DRV_WriteReg32(0xF0002160,0x3fff);
	DRV_WriteReg32(0xF0002170,0xeff8);
	DRV_WriteReg32(0xF0002180,0xfff7);
	DRV_WriteReg32(0xF0002190,0x7);
#endif

#if 1 // pullup or pull down
	DRV_WriteReg32(0xF0002200,0x0);
	DRV_WriteReg32(0xF0002210,0x1000);
	DRV_WriteReg32(0xF0002220,0x34c);

	//DRV_WriteReg32(0xF0002230,0x2400);

	DRV_WriteReg32(0xF0002240,0x1ff);
	DRV_WriteReg32(0xF0002250,0xf8c);
	DRV_WriteReg32(0xF0002260,0x0);
	DRV_WriteReg32(0xF0002270,0x1000);
	DRV_WriteReg32(0xF0002280,0x19e);
	DRV_WriteReg32(0xF0002290,0x0);
#endif	

#if 1 // data output
	DRV_WriteReg32(0xF0002400,0x0);
	DRV_WriteReg32(0xF0002410,0x0);
	DRV_WriteReg32(0xF0002420,0x0);

	//DRV_WriteReg32(0xF0002430,0x0);

	DRV_WriteReg32(0xF0002440,0x0);
	DRV_WriteReg32(0xF0002450,0x0);
	DRV_WriteReg32(0xF0002460,0x0);
	DRV_WriteReg32(0xF0002470,0x0);
	DRV_WriteReg32(0xF0002480,0x20);
	DRV_WriteReg32(0xF0002490,0x0);
#endif	



#if 1 // mode control
	DRV_WriteReg32(0xF0002600,0x5555);
	DRV_WriteReg32(0xF0002610,0x5555);
	DRV_WriteReg32(0xF0002620,0x5555);
	DRV_WriteReg32(0xF0002630,0x5555);
	DRV_WriteReg32(0xF0002640,0x5555);
	DRV_WriteReg32(0xF0002650,0x5555);
	DRV_WriteReg32(0xF0002660,0x5555);

	//DRV_WriteReg32(0xF0002670,0x5455);

	DRV_WriteReg32(0xF0002680,0x5414);
	DRV_WriteReg32(0xF0002690,0x15);
	DRV_WriteReg32(0xF00026A0,0x55f5);
	DRV_WriteReg32(0xF00026B0,0x5555);
	DRV_WriteReg32(0xF00026C0,0x5555);
	DRV_WriteReg32(0xF00026D0,0x5155);
	DRV_WriteReg32(0xF00026E0,0x15);
	DRV_WriteReg32(0xF00026F0,0x5);
	DRV_WriteReg32(0xF0002700,0x503d);
	DRV_WriteReg32(0xF0002710,0x5555);
	DRV_WriteReg32(0xF0002720,0x15);	
#endif										

#if 1 // clock out
		DRV_WriteReg32(0xF0002900,0x5);
		DRV_WriteReg32(0xF0002910,0x5);
		DRV_WriteReg32(0xF0002920,0x0);
		DRV_WriteReg32(0xF0002930,0x0);
		DRV_WriteReg32(0xF0002940,0x0);
		DRV_WriteReg32(0xF0002950,0x0);
		DRV_WriteReg32(0xF0002960,0x0);
		DRV_WriteReg32(0xF0002970,0x0);
#endif	

	// enable GPIO CG
	//DRV_WriteReg32(0xF0039320,0x40);
}

void mt6516_GPIO_suspend_set(void)
{


	UINT32 u4Status;


	
	// CAMIF
	if (g_GPIO_mask & GPIO_MASK_CAM)
	{
		MSG(SUSP,"@@@@@ GPIO_MASK_CAM\n\r");	
		mt6516_GPIO_set(17);
		mt6516_GPIO_set(18);
		mt6516_GPIO_set(19);
		mt6516_GPIO_set(20);
	}
	// MOTION SENSOR
	if (g_GPIO_mask & GPIO_MASK_MOTION)
	{
		MSG(SUSP,"@@@@@ GPIO_MASK_MOTION\n\r");		
		mt6516_GPIO_set(22);
	}
	// CAMIF, SCL1,SDA1
	if (g_GPIO_mask & GPIO_MASK_I2C1)
	{
		MSG(SUSP,"@@@@@ GPIO_MASK_I2C1\n\r");			
		mt6516_GPIO_set(34);
		mt6516_GPIO_set(35);
	}
	// TP
	if (g_GPIO_mask & GPIO_MASK_TP)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_TP\n\r");			
		mt6516_GPIO_set(48);
		mt6516_GPIO_set(49);
		mt6516_GPIO_set(50);
		mt6516_GPIO_set(51);
		mt6516_GPIO_set(52);
		mt6516_GPIO_set(53);

		mt6516_GPIO_set(100);
		mt6516_GPIO_set(101);
		mt6516_GPIO_set(102);
		mt6516_GPIO_set(103);
		mt6516_GPIO_set(104);
		mt6516_GPIO_set(105);

		mt6516_GPIO_set(115);
		mt6516_GPIO_set(116);

		mt6516_GPIO_set(108);
		mt6516_GPIO_set(109);

		mt6516_GPIO_set(129);
		mt6516_GPIO_set(130);
	}	
	// GLED, RLED	
	if (g_GPIO_mask & GPIO_MASK_LED)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_LED\n\r");		
		mt6516_GPIO_set(54);
		mt6516_GPIO_set(55);
	}
	// MT6326
	if (g_GPIO_mask & GPIO_MASK_6326)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_6326\n\r");		
		mt6516_GPIO_set(56);
		mt6516_GPIO_set(59);
	}
	// BT+WLAN
	if (g_GPIO_mask & GPIO_MASK_BTLAN)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_BTLAN\n\r");		
		mt6516_GPIO_set(60);
		mt6516_GPIO_set(64);
		mt6516_GPIO_set(67);
		mt6516_GPIO_set(68);
		mt6516_GPIO_set(75);
		mt6516_GPIO_set(76);
		mt6516_GPIO_set(77);
		mt6516_GPIO_set(79);
		mt6516_GPIO_set(115);
		mt6516_GPIO_set(116);
		mt6516_GPIO_set(122);
		mt6516_GPIO_set(123);
		mt6516_GPIO_set(132);
		mt6516_GPIO_set(133);
	}
	// PWR key + keypad
	
	if (g_GPIO_mask & GPIO_MASK_KPD)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_KPD\n\r");			
		//mt6516_GPIO_set(63);
		mt6516_GPIO_set(73);
		mt6516_GPIO_set(74);
		mt6516_GPIO_set(87);
		mt6516_GPIO_set(88);
		//mt6516_GPIO_set(89);
	}	
	// EarPhone
	if (g_GPIO_mask & GPIO_MASK_EARPHONE)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_EARPHONE\n\r");			
		mt6516_GPIO_set(65);
		mt6516_GPIO_set(66);
	}
	// J301, connector
	if (g_GPIO_mask & GPIO_MASK_CONNECTOR)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_CONNECTOR\n\r");		
		mt6516_GPIO_set(69);
		mt6516_GPIO_set(70);
	}
	// FM
	if (g_GPIO_mask & GPIO_MASK_FM)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_FM\n\r");			
		mt6516_GPIO_set(117);	
		mt6516_GPIO_set(135);
		mt6516_GPIO_set(136);
	}
	// 3326
	if (g_GPIO_mask & GPIO_MASK_3326)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_3326\n\r");			
		mt6516_GPIO_set(118);	
	}
	// amplifier
	if (g_GPIO_mask & GPIO_MASK_AMPLIFIER)
	{	
		MSG(SUSP,"@@@@@ GPIO_MASK_AMPLIFIER\n\r");		
		mt6516_GPIO_set(125);	
	}
	// PMIC
	if (g_GPIO_mask & PMIC_MASK_PARAM)
	{
		mt6516_pm_PMIC_param(FALSE);
	}
	// PMIC
	if (1)
	{
		MSG(SUSP,"@@@@@ PMIC_REG_SET\n\r");		
		mt6326_write_byte(0x2, 0xff );
		mt6326_write_byte(0x3, 0xff );
		mt6326_write_byte(0x6, 0x88 );		
		mt6326_write_byte(0x25, 0x40 );
		mt6326_write_byte(0x31, 0x2  );
		mt6326_write_byte(0x32, 0x4  );
		mt6326_write_byte(0x34, 0x1  );
		mt6326_write_byte(0x35, 0x4  );
		mt6326_write_byte(0x37, 0xd  );
		mt6326_write_byte(0x3a, 0x3  );
		mt6326_write_byte(0x3d, 0x8  );
		mt6326_write_byte(0x40, 0x2  );
		mt6326_write_byte(0x43, 0x2  );
		mt6326_write_byte(0x47, 0x0  );
		mt6326_write_byte(0x49, 0x20 );
//		mt6326_write_byte(0x4F, 0xC8 ); //BT
		mt6326_write_byte(0x53, 0x8  );
		mt6326_write_byte(0x55, 0x0  );
//		mt6326_write_byte(0x57, 0x40  ); //BT
		mt6326_write_byte(0x59, 0x17 );
		mt6326_write_byte(0x5f, 0x40 );
		mt6326_write_byte(0x63, 0x0  );
		mt6326_write_byte(0x64, 0x0  );
		mt6326_write_byte(0x67, 0x2b );
		mt6326_write_byte(0x6e, 0x5  );
		mt6326_write_byte(0x6f, 0x1  );
		mt6326_write_byte(0x70, 0xf  );
		mt6326_write_byte(0x71, 0x14  );
		mt6326_write_byte(0x75, 0xa0 );
		mt6326_write_byte(0x76, 0x2 );

		mt6326_write_byte(0x77, 0x2  );
		mt6326_write_byte(0x7a, 0xa0 );
		mt6326_write_byte(0x7c, 0x2  );
		mt6326_write_byte(0x81, 0x50  );

		mt6326_write_byte(0x83, 0x7  );
		mt6326_write_byte(0x84, 0x80 );
		mt6326_write_byte(0x86, 0x0  );
		mt6326_write_byte(0x89, 0xff );
		mt6326_write_byte(0x8a, 0xff );
		mt6326_write_byte(0x8b, 0x3f );
		mt6326_write_byte(0x8f, 0xf );
        mt6326_write_byte(0x92, 0xf  );  

        mt6326_write_byte(0x96, 0x3  );  

	}

#if 1 // SD test
	if (g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].bDefault_on)
		hwDisablePLL(MT6516_PLL_DPLL, TRUE, "PM");
#endif
	Suspend_power_saving();

	//PLL
	u4Status = DRV_Reg32(PDN_CON);
	if(u4Status & (1<<4))
		printk("===== MT6516_PLL_UPLL\n\r");
	if(u4Status & (1<<3))
		printk("===== MT6516_PLL_DPLL\n\r");
	if(u4Status & (1<<2))
		printk("===== MT6516_PLL_MPLL\n\r");

	u4Status = DRV_Reg32(CPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_CPLL\n\r");

	u4Status = DRV_Reg32(MCPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_MCPLL\n\r");

	u4Status = DRV_Reg32(CEVAPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_CEVAPLL\n\r");

	u4Status = DRV_Reg32(TPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_TPLL\n\r");

	u4Status = DRV_Reg32(MIPITX_CON);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_MIPI\n\r");
	
}


void mt6516_GPIO_suspend_restore(void)
{

	DRV_WriteReg32(0xF0002000 ,g_GPIO_save[0] );
	DRV_WriteReg32(0xF0002010 ,g_GPIO_save[1] );
	DRV_WriteReg32(0xF0002020 ,g_GPIO_save[2] );
	DRV_WriteReg32(0xF0002030 ,g_GPIO_save[3] );
	DRV_WriteReg32(0xF0002040 ,g_GPIO_save[4] );
	DRV_WriteReg32(0xF0002050 ,g_GPIO_save[5] );
	DRV_WriteReg32(0xF0002060 ,g_GPIO_save[6] );
	DRV_WriteReg32(0xF0002070 ,g_GPIO_save[7] );
	DRV_WriteReg32(0xF0002080 ,g_GPIO_save[8] );
	DRV_WriteReg32(0xF0002090 ,g_GPIO_save[9] );
										
	DRV_WriteReg32(0xF0002100,g_GPIO_save[10]);
	DRV_WriteReg32(0xF0002110,g_GPIO_save[11]);
	DRV_WriteReg32(0xF0002120,g_GPIO_save[12]);
	DRV_WriteReg32(0xF0002130,g_GPIO_save[13]);
	DRV_WriteReg32(0xF0002140,g_GPIO_save[14]);
	DRV_WriteReg32(0xF0002150,g_GPIO_save[15]);
	DRV_WriteReg32(0xF0002160,g_GPIO_save[16]);
	DRV_WriteReg32(0xF0002170,g_GPIO_save[17]);
	DRV_WriteReg32(0xF0002180,g_GPIO_save[18]);
	DRV_WriteReg32(0xF0002190,g_GPIO_save[19]);
										
	DRV_WriteReg32(0xF0002200,g_GPIO_save[20]);
	DRV_WriteReg32(0xF0002210,g_GPIO_save[21]);
	DRV_WriteReg32(0xF0002220,g_GPIO_save[22]);
	DRV_WriteReg32(0xF0002230,g_GPIO_save[23]);
	DRV_WriteReg32(0xF0002240,g_GPIO_save[24]);
	DRV_WriteReg32(0xF0002250,g_GPIO_save[25]);
	DRV_WriteReg32(0xF0002260,g_GPIO_save[26]);
	DRV_WriteReg32(0xF0002270,g_GPIO_save[27]);
	DRV_WriteReg32(0xF0002280,g_GPIO_save[28]);
	DRV_WriteReg32(0xF0002290,g_GPIO_save[29]);
										
	DRV_WriteReg32(0xF0002600,g_GPIO_save[30]);
	DRV_WriteReg32(0xF0002610,g_GPIO_save[31]);
	DRV_WriteReg32(0xF0002620,g_GPIO_save[32]);
	DRV_WriteReg32(0xF0002630,g_GPIO_save[33]);
	DRV_WriteReg32(0xF0002640,g_GPIO_save[34]);
	DRV_WriteReg32(0xF0002650,g_GPIO_save[35]);
	DRV_WriteReg32(0xF0002660,g_GPIO_save[36]);
	DRV_WriteReg32(0xF0002670,g_GPIO_save[37]);
	DRV_WriteReg32(0xF0002680,g_GPIO_save[38]);
	DRV_WriteReg32(0xF0002690,g_GPIO_save[39]);
	DRV_WriteReg32(0xF00026A0,g_GPIO_save[40]);
	DRV_WriteReg32(0xF00026B0,g_GPIO_save[41]);
	DRV_WriteReg32(0xF00026C0,g_GPIO_save[42]);
	DRV_WriteReg32(0xF00026D0,g_GPIO_save[43]);
	DRV_WriteReg32(0xF00026E0,g_GPIO_save[44]);
	DRV_WriteReg32(0xF00026F0,g_GPIO_save[45]);
	DRV_WriteReg32(0xF0002700,g_GPIO_save[46]);
	DRV_WriteReg32(0xF0002710,g_GPIO_save[47]);
	DRV_WriteReg32(0xF0002720,g_GPIO_save[48]);
										



	MSG(MSGS,"GPIO_DIR1 *0xF0002000 = 0x%x\n\r",DRV_Reg32(0xF0002000));
	MSG(MSGS,"GPIO_DIR2 *0xF0002010 = 0x%x\n\r",DRV_Reg32(0xF0002010));
	MSG(MSGS,"GPIO_DIR3 *0xF0002020 = 0x%x\n\r",DRV_Reg32(0xF0002020));
	MSG(MSGS,"GPIO_DIR4 *0xF0002030 = 0x%x\n\r",DRV_Reg32(0xF0002030));
	MSG(MSGS,"GPIO_DIR5 *0xF0002040 = 0x%x\n\r",DRV_Reg32(0xF0002040));
	MSG(MSGS,"GPIO_DIR6 *0xF0002050 = 0x%x\n\r",DRV_Reg32(0xF0002050));
	MSG(MSGS,"GPIO_DIR7 *0xF0002060 = 0x%x\n\r",DRV_Reg32(0xF0002060));
	MSG(MSGS,"GPIO_DIR8 *0xF0002070 = 0x%x\n\r",DRV_Reg32(0xF0002070));
	MSG(MSGS,"GPIO_DIR9 *0xF0002080 = 0x%x\n\r",DRV_Reg32(0xF0002080));
	MSG(MSGS,"GPIO_DIR10 *0xF0002090 = 0x%x\n\r",DRV_Reg32(0xF0002090));

	MSG(MSGS,"GPIO_PULLEN1 *0xF0002100 = 0x%x\n\r",DRV_Reg32(0xF0002100));
	MSG(MSGS,"GPIO_PULLEN2 *0xF0002110 = 0x%x\n\r",DRV_Reg32(0xF0002110));
	MSG(MSGS,"GPIO_PULLEN3 *0xF0002120 = 0x%x\n\r",DRV_Reg32(0xF0002120));
	MSG(MSGS,"GPIO_PULLEN4 *0xF0002130 = 0x%x\n\r",DRV_Reg32(0xF0002130));
	MSG(MSGS,"GPIO_PULLEN5 *0xF0002140 = 0x%x\n\r",DRV_Reg32(0xF0002140));
	MSG(MSGS,"GPIO_PULLEN6 *0xF0002150 = 0x%x\n\r",DRV_Reg32(0xF0002150));
	MSG(MSGS,"GPIO_PULLEN7 *0xF0002160 = 0x%x\n\r",DRV_Reg32(0xF0002160));
	MSG(MSGS,"GPIO_PULLEN8 *0xF0002170 = 0x%x\n\r",DRV_Reg32(0xF0002170));
	MSG(MSGS,"GPIO_PULLEN9 *0xF0002180 = 0x%x\n\r",DRV_Reg32(0xF0002180));
	MSG(MSGS,"GPIO_PULLEN10 *0xF0002190 = 0x%x\n\r",DRV_Reg32(0xF0002190));

	MSG(MSGS,"GPIO_PULLSEL1 *0xF0002200 = 0x%x\n\r",DRV_Reg32(0xF0002200));
	MSG(MSGS,"GPIO_PULLSEL2 *0xF0002210 = 0x%x\n\r",DRV_Reg32(0xF0002210));
	MSG(MSGS,"GPIO_PULLSEL3 *0xF0002220 = 0x%x\n\r",DRV_Reg32(0xF0002220));
	MSG(MSGS,"GPIO_PULLSEL4 *0xF0002230 = 0x%x\n\r",DRV_Reg32(0xF0002230));
	MSG(MSGS,"GPIO_PULLSEL5 *0xF0002240 = 0x%x\n\r",DRV_Reg32(0xF0002240));
	MSG(MSGS,"GPIO_PULLSEL6 *0xF0002250 = 0x%x\n\r",DRV_Reg32(0xF0002250));
	MSG(MSGS,"GPIO_PULLSEL7 *0xF0002260 = 0x%x\n\r",DRV_Reg32(0xF0002260));
	MSG(MSGS,"GPIO_PULLSEL8 *0xF0002270 = 0x%x\n\r",DRV_Reg32(0xF0002270));
	MSG(MSGS,"GPIO_PULLSEL9 *0xF0002280 = 0x%x\n\r",DRV_Reg32(0xF0002280));
	MSG(MSGS,"GPIO_PULLSEL10 *0xF0002290 = 0x%x\n\r",DRV_Reg32(0xF0002290));

	MSG(MSGS,"GPIO_mode1 *0xF0002600 = 0x%x\n\r",DRV_Reg32(0xF0002600));
	MSG(MSGS,"GPIO_mode2 *0xF0002610 = 0x%x\n\r",DRV_Reg32(0xF0002610));
	MSG(MSGS,"GPIO_mode3 *0xF0002620 = 0x%x\n\r",DRV_Reg32(0xF0002620));
	MSG(MSGS,"GPIO_mode4 *0xF0002630 = 0x%x\n\r",DRV_Reg32(0xF0002630));
	MSG(MSGS,"GPIO_mode5 *0xF0002640 = 0x%x\n\r",DRV_Reg32(0xF0002640));
	MSG(MSGS,"GPIO_mode6 *0xF0002650 = 0x%x\n\r",DRV_Reg32(0xF0002650));
	MSG(MSGS,"GPIO_mode7 *0xF0002660 = 0x%x\n\r",DRV_Reg32(0xF0002660));
	MSG(MSGS,"GPIO_mode8 *0xF0002670 = 0x%x\n\r",DRV_Reg32(0xF0002670));
	MSG(MSGS,"GPIO_mode9 *0xF0002680 = 0x%x\n\r",DRV_Reg32(0xF0002680));
	MSG(MSGS,"GPIO_mode10 *0xF0002690 = 0x%x\n\r",DRV_Reg32(0xF0002690));
	MSG(MSGS,"GPIO_mode11 *0xF00026A0 = 0x%x\n\r",DRV_Reg32(0xF00026A0));
	MSG(MSGS,"GPIO_mode12 *0xF00026B0 = 0x%x\n\r",DRV_Reg32(0xF00026B0));
	MSG(MSGS,"GPIO_mode13 *0xF00026C0 = 0x%x\n\r",DRV_Reg32(0xF00026C0));
	MSG(MSGS,"GPIO_mode14 *0xF00026D0 = 0x%x\n\r",DRV_Reg32(0xF00026D0));
	MSG(MSGS,"GPIO_mode15 *0xF00026E0 = 0x%x\n\r",DRV_Reg32(0xF00026E0));
	MSG(MSGS,"GPIO_mode16 *0xF00026F0 = 0x%x\n\r",DRV_Reg32(0xF00026F0));
	MSG(MSGS,"GPIO_mode17 *0xF0002700 = 0x%x\n\r",DRV_Reg32(0xF0002700));
	MSG(MSGS,"GPIO_mode18 *0xF0002710 = 0x%x\n\r",DRV_Reg32(0xF0002710));
	MSG(MSGS,"GPIO_mode19 *0xF0002720 = 0x%x\n\r",DRV_Reg32(0xF0002720));


}


extern void pmic_vcore1_dvfs_1_eco3(kal_uint8 val);
extern void pmic_vcore1_sleep_1_eco3(kal_uint8 val);
extern void pmic_vtcxo_calst(vtcxo_calst_enum sel);
extern void pmic_v3gtx_calst(v3gtx_calst_enum sel);
extern void pmic_spkl_dtip(kal_uint8 val);
extern void pmic_spkl_dtin(kal_uint8 val);
extern void pmic_spkl_dmode(spkl_dmode_enum sel);
extern void pmic_spkr_dtip(kal_uint8 val);
extern void pmic_spkr_dtin(kal_uint8 val);
extern void pmic_spkr_dmode(spkl_dmode_enum sel);


UINT32 mt6516_pm_GetChipVer(void)
{
    UINT32 u4ChipVer;
    u4ChipVer = DRV_Reg32(HW_VER);
    return u4ChipVer ;
}

void mt6516_pm_initTCM(void)
{
    memset ((void *)0xF0400000, 0, 0x800);
    memcpy ((void *)0xF0400000, (UINT32*)MT6516_CPUSuspend , 0x800);
}


void mt6516_pm_SetPLL(void)
{
    /* Set DCM */
    DRV_WriteReg32(MCUCLK_CON, 0);

    /* Because MT6516 HW bug, EMI clock cannot scale down */
    DRV_WriteReg32(EMICLK_CON, 0x1F);
    
}


void mt6516_pm_RegDump(void)
{
	UINT32 index = 0, u4Status = 0; 
	MSG(MSGS,"EMI_CONN		*0xF0020068 = 0x%x\n\r",DRV_Reg32(0xF0020068)); 	
	MSG(MSGS,"RTC FREQ		*0xF002C008 = 0x%x\n\r",DRV_Reg32(0xF002C008));		
    MSG(MSGS,"ARM FREQ      *0xF0001100 = 0x%x\n\r",DRV_Reg32(0xF0001100));
    MSG(MSGS,"HW MISC       *0xF0001020 = 0x%x\n\r",DRV_Reg32(0xF0001020));
    MSG(MSGS,"sleep Control *0xF0001204 = 0x%x\n\r",DRV_Reg32(0xF0001204));
    MSG(MSGS,"MCU CLKCON    *0xF0001208 = 0x%x\n\r",DRV_Reg32(0xF0001208));
    MSG(MSGS,"EMI CLKCON    *0xF000120C = 0x%x\n\r",DRV_Reg32(0xF000120C));
    MSG(MSGS,"Subsys OutIso *0xF0001300 = 0x%x\n\r",DRV_Reg32(0xF0001300));
    MSG(MSGS,"Subsys PowPDN *0xF0001304 = 0x%x\n\r",DRV_Reg32(0xF0001304));
    MSG(MSGS,"MCU MEMPDN    *0xF0001308 = 0x%x\n\r",DRV_Reg32(0xF0001308));
    MSG(MSGS,"GPH1 MEMPDN   *0xF000130c = 0x%x\n\r",DRV_Reg32(0xF000130c));
    MSG(MSGS,"GPH2 MEMPDN   *0xF0001310 = 0x%x\n\r",DRV_Reg32(0xF0001310));
    MSG(MSGS,"CEVA MEMPDN   *0xF0001314 = 0x%x\n\r",DRV_Reg32(0xF0001314));
    MSG(MSGS,"Subsys InIso  *0xF0001318 = 0x%x\n\r",DRV_Reg32(0xF0001318));
    MSG(MSGS,"APMCU PDN0    *0xF0039300 = 0x%x\n\r",DRV_Reg32(0xF0039300));
    MSG(MSGS,"APMCU PDN1    *0xF0039360 = 0x%x\n\r",DRV_Reg32(0xF0039360));
    MSG(MSGS,"GPH1 PDN0     *0xF0092300 = 0x%x\n\r",DRV_Reg32(0xF0092300));
    MSG(MSGS,"GPH2 PDN0     *0xF00A7000 = 0x%x\n\r",DRV_Reg32(0xF00A7000));
    MSG(MSGS,"PLL PDN_CON   *0xF0060010 = 0x%x\n\r",DRV_Reg32(0xF0060010));
    MSG(MSGS,"PLL CLK_CON   *0xF0060014 = 0x%x\n\r",DRV_Reg32(0xF0060014));
    MSG(MSGS,"PLL CPLL2     *0xF006003C = 0x%x\n\r",DRV_Reg32(0xF006003C));
    MSG(MSGS,"PLL TPLL2     *0xF0060044 = 0x%x\n\r",DRV_Reg32(0xF0060044));
    MSG(MSGS,"PLL MCPLL2    *0xF006005C = 0x%x\n\r",DRV_Reg32(0xF006005C));
    MSG(MSGS,"PLL CEVAPLL2  *0xF0060064 = 0x%x\n\r",DRV_Reg32(0xF0060064));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B00 = 0x%x\n\r",DRV_Reg32(0xF0060B00));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B04 = 0x%x\n\r",DRV_Reg32(0xF0060B04));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B08 = 0x%x\n\r",DRV_Reg32(0xF0060B08));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B0C = 0x%x\n\r",DRV_Reg32(0xF0060B0C));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B10 = 0x%x\n\r",DRV_Reg32(0xF0060B10));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B14 = 0x%x\n\r",DRV_Reg32(0xF0060B14));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B18 = 0x%x\n\r",DRV_Reg32(0xF0060B18));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B1C = 0x%x\n\r",DRV_Reg32(0xF0060B1C));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B40 = 0x%x\n\r",DRV_Reg32(0xF0060B40));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B48 = 0x%x\n\r",DRV_Reg32(0xF0060B48));
    MSG(MSGS,"PLL MIPITXPLL *0xF0060B4c = 0x%x\n\r",DRV_Reg32(0xF0060B4C));
    MSG(MSGS,"PLL MIPITXPLL *0xF006004c = 0x%x\n\r",DRV_Reg32(0xF006004C));
    MSG(MSGS,"EMI CONN      *0xF0020068 = 0x%x\n\r",DRV_Reg32(0xF0020068));
    MSG(MSGS,"GPIO_DIR1 *0xF0002000 = 0x%x\n\r",DRV_Reg32(0xF0002000));
    MSG(MSGS,"GPIO_DIR2 *0xF0002010 = 0x%x\n\r",DRV_Reg32(0xF0002010));
    MSG(MSGS,"GPIO_DIR3 *0xF0002020 = 0x%x\n\r",DRV_Reg32(0xF0002020));
    MSG(MSGS,"GPIO_DIR4 *0xF0002030 = 0x%x\n\r",DRV_Reg32(0xF0002030));
    MSG(MSGS,"GPIO_DIR5 *0xF0002040 = 0x%x\n\r",DRV_Reg32(0xF0002040));
    MSG(MSGS,"GPIO_DIR6 *0xF0002050 = 0x%x\n\r",DRV_Reg32(0xF0002050));
    MSG(MSGS,"GPIO_DIR7 *0xF0002060 = 0x%x\n\r",DRV_Reg32(0xF0002060));
    MSG(MSGS,"GPIO_DIR8 *0xF0002070 = 0x%x\n\r",DRV_Reg32(0xF0002070));
    MSG(MSGS,"GPIO_DIR9 *0xF0002080 = 0x%x\n\r",DRV_Reg32(0xF0002080));
    MSG(MSGS,"GPIO_DIR10 *0xF0002090 = 0x%x\n\r",DRV_Reg32(0xF0002090));

    MSG(MSGS,"GPIO_PULLEN1 *0xF0002100 = 0x%x\n\r",DRV_Reg32(0xF0002100));
    MSG(MSGS,"GPIO_PULLEN2 *0xF0002110 = 0x%x\n\r",DRV_Reg32(0xF0002110));
    MSG(MSGS,"GPIO_PULLEN3 *0xF0002120 = 0x%x\n\r",DRV_Reg32(0xF0002120));
    MSG(MSGS,"GPIO_PULLEN4 *0xF0002130 = 0x%x\n\r",DRV_Reg32(0xF0002130));
    MSG(MSGS,"GPIO_PULLEN5 *0xF0002140 = 0x%x\n\r",DRV_Reg32(0xF0002140));
    MSG(MSGS,"GPIO_PULLEN6 *0xF0002150 = 0x%x\n\r",DRV_Reg32(0xF0002150));
    MSG(MSGS,"GPIO_PULLEN7 *0xF0002160 = 0x%x\n\r",DRV_Reg32(0xF0002160));
    MSG(MSGS,"GPIO_PULLEN8 *0xF0002170 = 0x%x\n\r",DRV_Reg32(0xF0002170));
    MSG(MSGS,"GPIO_PULLEN9 *0xF0002180 = 0x%x\n\r",DRV_Reg32(0xF0002180));
    MSG(MSGS,"GPIO_PULLEN10 *0xF0002190 = 0x%x\n\r",DRV_Reg32(0xF0002190));

    MSG(MSGS,"GPIO_PULLSEL1 *0xF0002200 = 0x%x\n\r",DRV_Reg32(0xF0002200));
    MSG(MSGS,"GPIO_PULLSEL2 *0xF0002210 = 0x%x\n\r",DRV_Reg32(0xF0002210));
    MSG(MSGS,"GPIO_PULLSEL3 *0xF0002220 = 0x%x\n\r",DRV_Reg32(0xF0002220));
    MSG(MSGS,"GPIO_PULLSEL4 *0xF0002230 = 0x%x\n\r",DRV_Reg32(0xF0002230));
    MSG(MSGS,"GPIO_PULLSEL5 *0xF0002240 = 0x%x\n\r",DRV_Reg32(0xF0002240));
    MSG(MSGS,"GPIO_PULLSEL6 *0xF0002250 = 0x%x\n\r",DRV_Reg32(0xF0002250));
    MSG(MSGS,"GPIO_PULLSEL7 *0xF0002260 = 0x%x\n\r",DRV_Reg32(0xF0002260));
    MSG(MSGS,"GPIO_PULLSEL8 *0xF0002270 = 0x%x\n\r",DRV_Reg32(0xF0002270));
    MSG(MSGS,"GPIO_PULLSEL9 *0xF0002280 = 0x%x\n\r",DRV_Reg32(0xF0002280));
    MSG(MSGS,"GPIO_PULLSEL10 *0xF0002290 = 0x%x\n\r",DRV_Reg32(0xF0002290));

    MSG(MSGS,"GPIO dataoutput *0xF0002400 = 0x%x\n\r",DRV_Reg32(0xF0002400));
    MSG(MSGS,"GPIO dataoutput *0xF0002410 = 0x%x\n\r",DRV_Reg32(0xF0002410));
    MSG(MSGS,"GPIO dataoutput *0xF0002420 = 0x%x\n\r",DRV_Reg32(0xF0002420));
    MSG(MSGS,"GPIO dataoutput *0xF0002430 = 0x%x\n\r",DRV_Reg32(0xF0002430));
    MSG(MSGS,"GPIO dataoutput *0xF0002440 = 0x%x\n\r",DRV_Reg32(0xF0002440));
    MSG(MSGS,"GPIO dataoutput *0xF0002450 = 0x%x\n\r",DRV_Reg32(0xF0002450));
    MSG(MSGS,"GPIO dataoutput *0xF0002460 = 0x%x\n\r",DRV_Reg32(0xF0002460));
    MSG(MSGS,"GPIO dataoutput *0xF0002470 = 0x%x\n\r",DRV_Reg32(0xF0002470));
    MSG(MSGS,"GPIO dataoutput *0xF0002480 = 0x%x\n\r",DRV_Reg32(0xF0002480));
    MSG(MSGS,"GPIO dataoutput *0xF0002490 = 0x%x\n\r",DRV_Reg32(0xF0002490));


    MSG(MSGS,"GPIO dataInput *0xF0002500 = 0x%x\n\r",DRV_Reg32(0xF0002500));
    MSG(MSGS,"GPIO dataInput *0xF0002510 = 0x%x\n\r",DRV_Reg32(0xF0002510));
    MSG(MSGS,"GPIO dataInput *0xF0002520 = 0x%x\n\r",DRV_Reg32(0xF0002520));
    MSG(MSGS,"GPIO dataInput *0xF0002530 = 0x%x\n\r",DRV_Reg32(0xF0002530));
    MSG(MSGS,"GPIO dataInput *0xF0002540 = 0x%x\n\r",DRV_Reg32(0xF0002540));
    MSG(MSGS,"GPIO dataInput *0xF0002550 = 0x%x\n\r",DRV_Reg32(0xF0002550));
    MSG(MSGS,"GPIO dataInput *0xF0002560 = 0x%x\n\r",DRV_Reg32(0xF0002560));
    MSG(MSGS,"GPIO dataInput *0xF0002570 = 0x%x\n\r",DRV_Reg32(0xF0002570));
    MSG(MSGS,"GPIO dataInput *0xF0002580 = 0x%x\n\r",DRV_Reg32(0xF0002580));
    MSG(MSGS,"GPIO dataInput *0xF0002590 = 0x%x\n\r",DRV_Reg32(0xF0002590));


    MSG(MSGS,"GPIO_mode1 *0xF0002600 = 0x%x\n\r",DRV_Reg32(0xF0002600));
    MSG(MSGS,"GPIO_mode2 *0xF0002610 = 0x%x\n\r",DRV_Reg32(0xF0002610));
    MSG(MSGS,"GPIO_mode3 *0xF0002620 = 0x%x\n\r",DRV_Reg32(0xF0002620));
    MSG(MSGS,"GPIO_mode4 *0xF0002630 = 0x%x\n\r",DRV_Reg32(0xF0002630));
    MSG(MSGS,"GPIO_mode5 *0xF0002640 = 0x%x\n\r",DRV_Reg32(0xF0002640));
    MSG(MSGS,"GPIO_mode6 *0xF0002650 = 0x%x\n\r",DRV_Reg32(0xF0002650));
    MSG(MSGS,"GPIO_mode7 *0xF0002660 = 0x%x\n\r",DRV_Reg32(0xF0002660));
    MSG(MSGS,"GPIO_mode8 *0xF0002670 = 0x%x\n\r",DRV_Reg32(0xF0002670));
    MSG(MSGS,"GPIO_mode9 *0xF0002680 = 0x%x\n\r",DRV_Reg32(0xF0002680));
    MSG(MSGS,"GPIO_mode10 *0xF0002690 = 0x%x\n\r",DRV_Reg32(0xF0002690));
    MSG(MSGS,"GPIO_mode11 *0xF00026A0 = 0x%x\n\r",DRV_Reg32(0xF00026A0));
    MSG(MSGS,"GPIO_mode12 *0xF00026B0 = 0x%x\n\r",DRV_Reg32(0xF00026B0));
    MSG(MSGS,"GPIO_mode13 *0xF00026C0 = 0x%x\n\r",DRV_Reg32(0xF00026C0));
    MSG(MSGS,"GPIO_mode14 *0xF00026D0 = 0x%x\n\r",DRV_Reg32(0xF00026D0));
    MSG(MSGS,"GPIO_mode15 *0xF00026E0 = 0x%x\n\r",DRV_Reg32(0xF00026E0));
    MSG(MSGS,"GPIO_mode16 *0xF00026F0 = 0x%x\n\r",DRV_Reg32(0xF00026F0));
    MSG(MSGS,"GPIO_mode17 *0xF0002700 = 0x%x\n\r",DRV_Reg32(0xF0002700));
    MSG(MSGS,"GPIO_mode18 *0xF0002710 = 0x%x\n\r",DRV_Reg32(0xF0002710));
    MSG(MSGS,"GPIO_mode19 *0xF0002720 = 0x%x\n\r",DRV_Reg32(0xF0002720));

    MSG(MSGS,"GPIO clk out *0xF0002900 = 0x%x\n\r",DRV_Reg32(0xF0002900));
    MSG(MSGS,"GPIO clk out *0xF0002910 = 0x%x\n\r",DRV_Reg32(0xF0002910));
    MSG(MSGS,"GPIO clk out *0xF0002920 = 0x%x\n\r",DRV_Reg32(0xF0002920));
    MSG(MSGS,"GPIO clk out *0xF0002930 = 0x%x\n\r",DRV_Reg32(0xF0002930));
    MSG(MSGS,"GPIO clk out *0xF0002940 = 0x%x\n\r",DRV_Reg32(0xF0002940));
    MSG(MSGS,"GPIO clk out *0xF0002950 = 0x%x\n\r",DRV_Reg32(0xF0002950));
    MSG(MSGS,"GPIO clk out *0xF0002960 = 0x%x\n\r",DRV_Reg32(0xF0002960));
    MSG(MSGS,"GPIO clk out *0xF0002970 = 0x%x\n\r",DRV_Reg32(0xF0002970));

	u4Status = DRV_Reg32(PDN_CON);
	if(u4Status & (1<<4))
		printk("===== MT6516_PLL_UPLL\n\r");
	if(u4Status & (1<<3))
		printk("===== MT6516_PLL_DPLL\n\r");
	if(u4Status & (1<<2))
		printk("===== MT6516_PLL_MPLL\n\r");

	u4Status = DRV_Reg32(CPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_CPLL\n\r");

	u4Status = DRV_Reg32(MCPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_MCPLL\n\r");

	u4Status = DRV_Reg32(CEVAPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_CEVAPLL\n\r");

	u4Status = DRV_Reg32(TPLL2);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_TPLL\n\r");

	u4Status = DRV_Reg32(MIPITX_CON);
	if(u4Status & (1<<0))
		printk("===== MT6516_PLL_MIPI\n\r");


    printk( "\n\rPLL Status\n\r" );
    printk( "=========================================\n\r" );        

	if(g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount)
		printk( "UPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount)
		printk( "DPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount)
		printk( "MPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount)
		printk( "CPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount)
		printk( "TPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount)
		printk( "CEVAPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount)
		printk( "MCPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].dwPllCount)
		printk( "MIPI:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].dwPllCount);

	printk( "\n\r ");

    printk( "\n\rPLL Master\n\r" );
    printk( "=========================================\n\r" );        
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		if(g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount > index )
			printk( "UPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount > index )
			printk( "DPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount > index )
			printk( "MPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount > index )
			printk( "CPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount > index )
			printk( "TPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount > index )
			printk( "CEVAPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount > index )
			printk( "MCPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].dwPllCount > index )
			printk( "MIPI:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].mod_name[index]);
		
		printk( "\n\r ");


	}



    printk( "\n\rPMIC Status\n\r" );
    printk( "=========================================\n\r" );        

	if(g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount)
		printk( "VRF:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount)
		printk( "VTCXO:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount)
		printk( "V3GTX:%d ",g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].dwPowerCount)
		printk( "V3GRX:%d ",g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount)
		printk( "VCAMA:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount)
		printk( "VWIFI3V3:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount)
		printk( "VWIFI2V8:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount)
		printk( "VSIM:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount);

	if(g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount)
		printk( "VUSB:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VBT].dwPowerCount)
		printk( "VBT:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VBT].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount)
		printk( "VCAMD:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount)
		printk( "VGP:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount)
		printk( "VGP2:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount)
		printk( "VSDIO:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount);
	printk( "\n\r ");

    printk( "\n\rPMIC Master\n\r" );
    printk( "=========================================\n\r" );        
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{

		if(g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount > index)
			printk( "VRF:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VRF].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount > index)
			printk( "VTCXO:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount > index)
			printk( "V3GTX:%s ",g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].dwPowerCount > index)
			printk( "V3GRX:%s ",g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount > index)
			printk( "VCAMA:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount > index)
			printk( "VWIFI3V3:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount > index)
			printk( "VWIFI2V8:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount > index)
			printk( "VSIM:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VSIM].mod_name[index]);
		
		if(g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount > index)
			printk( "VUSB:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VUSB].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VBT].dwPowerCount > index)
			printk( "VBT:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VBT].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount > index)
			printk( "VCAMD:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount > index)
			printk( "VGP:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VGP].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount > index)
			printk( "VGP2:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VGP2].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount > index)
			printk( "VSDIO:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].mod_name[index]);
		printk( "\n\r ");

	}


    printk( "\n\rMCU SYS CG Status\n\r" );
    printk( "=========================================\n\r" );    

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].dwClockCount)
		printk( "DMA:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].dwClockCount)
		printk( "USB:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].dwClockCount)
		printk( "SEJ:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].dwClockCount)
		printk( "I2C3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].dwClockCount)
		printk( "GPT:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].dwClockCount)
		printk( "KP:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].dwClockCount)
		printk( "GPIO:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].dwClockCount)
		printk( "UART1:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].dwClockCount)
		printk( "UART2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].dwClockCount)
		printk( "UART3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].dwClockCount)
		printk( "SIM:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].dwClockCount)
		printk( "PWM:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].dwClockCount)
		printk( "PWM1:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].dwClockCount)
		printk( "PWM2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].dwClockCount)
		printk( "PWM3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].dwClockCount)
		printk( "MSDC:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].dwClockCount)
		printk( "SWDBG:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].dwClockCount)
		printk( "NFI:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].dwClockCount)
		printk( "I2C2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].dwClockCount)
		printk( "IRDA:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].dwClockCount)
		printk( "I2C:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].dwClockCount);


	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].dwClockCount)
		printk( "TOUC:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].dwClockCount)
		printk( "SIM2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].dwClockCount)
		printk( "MSDC2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].dwClockCount)
		printk( "ADC:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].dwClockCount)
		printk( "TP:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].dwClockCount)
		printk( "XGPT:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].dwClockCount)
		printk( "UART4:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].dwClockCount);


	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].dwClockCount)
		printk( "MSDC3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].dwClockCount)
		printk( "ONEWIRE:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].dwClockCount)
		printk( "CSDBG:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].dwClockCount)
		printk( "PWM0:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].dwClockCount);
	printk( "\n\r ");



    printk( "\n\rMM SYS CG Status\n\r" );
    printk( "=========================================\n\r" );    
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].dwClockCount)
		printk( "GMC1:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].dwClockCount)
		printk( "G2D:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].dwClockCount)
		printk( "GCMQ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].dwClockCount)
		printk( "BLS:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].dwClockCount)
		printk( "IMGDMA0:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].dwClockCount)
		printk( "PNG:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].dwClockCount)
		printk( "DSI:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].dwClockCount)
		printk( "TVE:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].dwClockCount)
		printk( "TVC:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].dwClockCount)
		printk( "ISP:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].dwClockCount)
		printk( "IPP:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].dwClockCount)
		printk( "PRZ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].dwClockCount)
		printk( "CRZ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].dwClockCount)
		printk( "DRZ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].dwClockCount);


	if(g_MT6516_BusHW.device[MT6516_PDN_MM_WT].dwClockCount)
		printk( "WT:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_WT].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].dwClockCount)
		printk( "AFE:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].dwClockCount)
		printk( "SPI:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].dwClockCount)
		printk( "ASM:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].dwClockCount)
		printk( "RESZLB:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].dwClockCount)
		printk( "LCD:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].dwClockCount)
		printk( "DPI:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].dwClockCount)
		printk( "G1FAKE:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].dwClockCount)
		printk( "GMC2:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].dwClockCount)
		printk( "IMGDMA1:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].dwClockCount)
		printk( "PRZ2:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].dwClockCount)
		printk( "M3D:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_H264].dwClockCount)
		printk( "H264:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_H264].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].dwClockCount)
		printk( "DCT:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].dwClockCount);


	if(g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].dwClockCount)
		printk( "JPEG:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].dwClockCount)
		printk( "MP4:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].dwClockCount)
		printk( "MP4DBLK:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_ID_CEVA].dwClockCount)
		printk( "CEVA:%d ",g_MT6516_BusHW.device[MT6516_ID_CEVA].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_IS_MD].dwClockCount)
		printk( "MD:%d ",g_MT6516_BusHW.device[MT6516_IS_MD].dwClockCount);
	printk( "\n\r ");


	printk( "\n\rMCU SYS Master\n\r" );
	printk( "=========================================\n\r" );		  
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].dwClockCount > index )
			printk( "DMA:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].dwClockCount > index )
			printk( "USB:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].dwClockCount > index )
			printk( "SEJ:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].dwClockCount > index )
			printk( "I2C3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].dwClockCount > index )
			printk( "GPT:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].dwClockCount > index )
			printk( "KP:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].dwClockCount > index )
			printk( "GPIO:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].dwClockCount > index )
			printk( "UART1:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].dwClockCount > index )
			printk( "UART2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].dwClockCount > index )
			printk( "UART3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].dwClockCount > index )
			printk( "SIM:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].dwClockCount > index )
			printk( "PWM:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].dwClockCount > index )
			printk( "PWM1:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].dwClockCount > index )
			printk( "PWM2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].dwClockCount > index )
			printk( "PWM3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].dwClockCount > index )
			printk( "MSDC:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].dwClockCount > index )
			printk( "SWDBG:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].dwClockCount > index )
			printk( "NFI:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].dwClockCount > index )
			printk( "I2C2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].dwClockCount > index )
			printk( "IRDA:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].dwClockCount > index )
			printk( "I2C:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].mod_name[index]);


		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].dwClockCount > index )
			printk( "TOUC:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].dwClockCount > index )
			printk( "SIM2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].dwClockCount > index )
			printk( "MSDC2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].dwClockCount > index )
			printk( "ADC:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].dwClockCount > index )
			printk( "TP:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].dwClockCount > index )
			printk( "XGPT:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].dwClockCount > index )
			printk( "UART4:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].mod_name[index]);


		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].dwClockCount > index )
			printk( "MSDC3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].dwClockCount > index )
			printk( "ONEWIRE:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].dwClockCount > index )
			printk( "CSDBG:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].dwClockCount > index )
			printk( "PWM0:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].mod_name[index]);
		printk( "\n\r ");


	}

	printk( "\n\rMM SYS Master\n\r" );
	printk( "=========================================\n\r" );		  
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].dwClockCount > index )
			printk( "GMC1:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].dwClockCount > index )
			printk( "G2D:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].dwClockCount > index )
			printk( "GCMQ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].dwClockCount > index )
			printk( "BLS:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].dwClockCount > index )
			printk( "IMGDMA0:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].dwClockCount > index )
			printk( "PNG:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].dwClockCount > index )
			printk( "DSI:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].mod_name[index]);


		if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].dwClockCount > index )
			printk( "TVE:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].dwClockCount > index )
			printk( "TVC:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].dwClockCount > index )
			printk( "ISP:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].dwClockCount > index )
			printk( "IPP:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].dwClockCount > index )
			printk( "PRZ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].dwClockCount > index )
			printk( "CRZ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].dwClockCount > index )
			printk( "DRZ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].mod_name[index]);


		if(g_MT6516_BusHW.device[MT6516_PDN_MM_WT].dwClockCount > index )
			printk( "WT:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_WT].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].dwClockCount > index )
			printk( "AFE:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].dwClockCount > index )
			printk( "SPI:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].dwClockCount > index )
			printk( "ASM:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].dwClockCount > index )
			printk( "RESZLB:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].dwClockCount > index )
			printk( "LCD:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].dwClockCount > index )
			printk( "DPI:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].mod_name[index]);


		if(g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].dwClockCount > index )
			printk( "G1FAKE:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].dwClockCount > index )
			printk( "GMC2:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].dwClockCount > index )
			printk( "IMGDMA1:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].dwClockCount > index )
			printk( "PRZ2:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].dwClockCount > index )
			printk( "M3D:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_H264].dwClockCount > index )
			printk( "H264:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_H264].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].dwClockCount > index )
			printk( "DCT:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].mod_name[index]);


		if(g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].dwClockCount > index )
			printk( "JPEG:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].dwClockCount > index )
			printk( "MP4:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].dwClockCount > index )
			printk( "MP4DBLK:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_ID_CEVA].dwClockCount > index )
			printk( "CEVA:%s ",g_MT6516_BusHW.device[MT6516_ID_CEVA].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_IS_MD].dwClockCount > index )
			printk( "MD:%s ",g_MT6516_BusHW.device[MT6516_IS_MD].mod_name[index]);

		printk( "\n\r ");


	}


	#if 0
    printk("\n\rPLL Master\n\r" );
    printk("=========================================\n\r" );        
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		printk("UPLL:%s DPLL:%s MPLL:%s CPLL:%s TPLL:%s CEVAPLL:%s MCPLL:%s\n", 
			g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].mod_name[index],
			g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].mod_name[index],g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].mod_name[index],
			g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].mod_name[index],g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].mod_name[index],
			g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].mod_name[index],g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].mod_name[index]);
	}
    printk("\n\rPMIC Master\n\r" );
    printk("=========================================\n\r" );        
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		printk("VRF:%s VTCXO:%s V3GTX:%s V3GRX:%s VCAMA:%s VWIFI3V3:%s VWIFI2V8:%s VSIM:%s [%d]\n", 
			g_MT6516_BusHW.Power[MT6516_POWER_VRF].mod_name[index],
			g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].mod_name[index],
			g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].mod_name[index],
			g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].mod_name[index],
			g_MT6516_BusHW.Power[MT6516_POWER_VSIM].mod_name[index], index);

		printk("VUSB:%s VBT:%s VCAMD:%s VGP:%s VGP2:%s VSDIO:%s [%d]\n\n", 
			g_MT6516_BusHW.Power[MT6516_POWER_VUSB].mod_name[index],
			g_MT6516_BusHW.Power[MT6516_POWER_VBT].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].mod_name[index],
			g_MT6516_BusHW.Power[MT6516_POWER_VGP].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VGP2].mod_name[index],
			g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].mod_name[index], index);
	}
	printk("\n\rMCU SYS Master\n\r" );
	printk("=========================================\n\r" );		  
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		printk("DMA:%s USB:%s SEJ:%s I2C3:%s GPT:%s KP:%s GPIO:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].mod_name[index]);
		printk("UART1:%s UART2:%s UART3:%s SIM:%s PWM:%s PWM1:%s PWM2:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].mod_name[index]);
		printk("PWM3:%s MSDC:%s SWDBG:%s NFI:%s I2C2:%s IRDA:%s I2C:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].mod_name[index]);
		printk("TOUC:%s SIM2:%s MSDC2:%s ADC:%s TP:%s XGPT:%s UART4:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].mod_name[index]);
		printk("MSDC3:%s ONEWIRE:%s CSDBG:%s PWM0:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].mod_name[index]);
	}

	printk("\n\rMM SYS Master\n\r" );
	printk("=========================================\n\r" );		  
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		printk("GMC1:%s G2D:%s GCMQ:%s BLS:%s IMGDMA0:%s PNG:%s DSI:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].mod_name[index]);
		printk("TVE:%s TVC:%s ISP:%s IPP:%s PRZ:%s CRZ:%s DRZ:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].mod_name[index]);
		printk("WT:%s AFE:%s SPI:%s ASM:%s RESZLB:%s LCD:%s DPI:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_MM_WT].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].mod_name[index]);
		printk("G1FAKE:%s GMC2:%s IMGDMA1:%s PRZ2:%s M3D:%s H264:%s DCT:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_H264].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].mod_name[index]);
		printk("JPEG:%s MP4:%s MP4DBLK:%s CEVA:%s MD:%s\n", 
			g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].mod_name[index],
			g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].mod_name[index],
			g_MT6516_BusHW.device[MT6516_ID_CEVA].mod_name[index],
			g_MT6516_BusHW.device[MT6516_IS_MD].mod_name[index]);
	}		
	#endif

}    

void mt6516_pm_WakeReason(UINT32 u4IRQ)
{
    UINT32 u4WakeupSrc = 0;
    
    u4WakeupSrc = DRV_Reg32(0xF003C22C);
    if ((u4WakeupSrc & (1<<WS_KP)) && (u4IRQ == MT6516_KP_IRQ_LINE))
        MSG(SUSP,"Wake up by Keypad\n\r");
    if ((u4WakeupSrc & (1<<WS_EINT)) && (u4IRQ == MT6516_EIT_IRQ_LINE))
        MSG(SUSP,"Wake up by EINT\n\r");
    if ((u4WakeupSrc & (1<<WS_RTC)) && (u4IRQ == MT6516_RTC_IRQ_LINE))
        MSG(SUSP,"Wake up by RTC\n\r");
    if ((u4WakeupSrc & (1<<WS_LOWBAT)) && (u4IRQ == MT6516_LOWBAT_IRQ_LINE))
    {
        MSG(SUSP,"Wake up by low battery\n\r");
        MSG(SUSP,"****************************************\n\r");        
        MSG(SUSP,"* Low battery => Power down cellphone **\n\r");        
        MSG(SUSP,"****************************************\n\r");        
        //rtc_bbpu_power_down();
    }
    if ((u4WakeupSrc & (1<<WS_CCIF)) && (u4IRQ == MT6516_APCCIF_IRQ_LINE))
        MSG(SUSP,"Wake up by CCIF\n\r");
    if ((u4WakeupSrc & (1<<WS_MSDC)) && (u4IRQ == MT6516_MSDC1EVENT_IRQ_LINE))
        MSG(SUSP,"Wake up by MSDC1\n\r");
    if ((u4WakeupSrc & (1<<WS_MSDC)) && (u4IRQ == MT6516_MSDC2EVENT_IRQ_LINE))
        MSG(SUSP,"Wake up by MSDC2\n\r");
    if ((u4WakeupSrc & (1<<WS_MSDC)) && (u4IRQ == MT6516_MSDC3EVENT_IRQ_LINE))
        MSG(SUSP,"Wake up by MSDC3\n\r");
    if ((u4WakeupSrc & (1<<WS_GPT)) && (u4IRQ == MT6516_GPT_IRQ_LINE))
        MSG(SUSP,"Wake up by GPT\n\r");
    if ((u4WakeupSrc & (1<<WS_TP)) && (u4IRQ == MT6516_TOUCH_IRQ_LINE))
        MSG(SUSP,"Wake up by TP\n\r");
}    

void mt6516_pm_Maskinterrupt(void)
{
    UINT32 u4WakeupSrc = 0, u4Status= 0 ;

    /* STEP1: Set AP_SLEEP_CTRL (IRQ_CODE: 0x36) to level sensitive on CIRQ. */
    MT6516_IRQSensitivity(MT6516_APSLEEP_IRQ_LINE, MT6516_LEVEL_SENSITIVE);
    MT6516_IRQUnmask(MT6516_APSLEEP_IRQ_LINE);

    /* STEP2: Read clear AP_SM_STA (0x8003C21C). */
    u4Status = DRV_Reg32(AP_SM_STA);

    /* STEP3: EOI AP_SLEEP_CTRL interrupt */
    *MT6516_IRQ_EOI2 = MT6516_APSLEEP_IRQ_LINE;

    /* STEP4: Unmask corresponding interrupt of wanted wake-up source */
    u4WakeupSrc = DRV_Reg32(0xF003C22C);

    *MT6516_IRQ_MASKL = 0xFFFFFFFF;
    *MT6516_IRQ_MASKH = 0xFFFFFFFF;

    /* reliquish and refresh interrupt before enter suspend */
    *MT6516_IRQ_EOIL = 0xFFFFFFFF;
    *MT6516_IRQ_EOIH = 0xFFFFFFFF;

	if (u4WakeupSrc & (1<<WS_SM))	
		MT6516_IRQUnmask(MT6516_APSLEEP_IRQ_LINE);	// 20100317 James
		
    if (u4WakeupSrc & (1<<WS_KP))
        MT6516_IRQUnmask(MT6516_KP_IRQ_LINE);

    if (u4WakeupSrc & (1<<WS_EINT))
        MT6516_IRQUnmask(MT6516_EIT_IRQ_LINE);

    if (u4WakeupSrc & (1<<WS_RTC))
        MT6516_IRQUnmask(MT6516_RTC_IRQ_LINE);

    if (u4WakeupSrc & (1<<WS_MSDC))
    {
        MT6516_IRQUnmask(MT6516_MSDC1EVENT_IRQ_LINE);
        MT6516_IRQUnmask(MT6516_MSDC2EVENT_IRQ_LINE);
        MT6516_IRQUnmask(MT6516_MSDC3EVENT_IRQ_LINE);
    }

    if (u4WakeupSrc & (1<<WS_LOWBAT))
        MT6516_IRQUnmask(MT6516_LOWBAT_IRQ_LINE);

    if (u4WakeupSrc & (1<<WS_CCIF))
        MT6516_IRQUnmask(MT6516_APCCIF_IRQ_LINE);

    if (u4WakeupSrc & (1<<WS_GPT))
        MT6516_IRQUnmask(MT6516_GPT_IRQ_LINE);

    if (u4WakeupSrc & (1<<WS_TP))
        MT6516_IRQUnmask(MT6516_TOUCH_IRQ_LINE);
    suspend_lock = TRUE;

    
}    

void mt6516_pm_SetWakeSrc(UINT32 u4Mask)
{
    DRV_WriteReg32(AP_SM_CNF, u4Mask);
}

UINT32 mt6516_pm_CheckStatus(void)
{
    UINT32 u4Status;

    /* STEP10: Polling bit[7:5] of AP_SM_STA (0x8003C21C) register */
    /* Waked up by sleep controller timeout, AP_SM_STA register is 0x00C0 */
    /* Waked up by other interrupt source, AP_SM_STA register is 0x00A0 */

    u4Status = DRV_Reg32(AP_SM_STA);
    //MSG(SUSP,"u4Status = 0x%x\n\r",u4Status);

    if ( u4Status&PAUSE_RQST )
    {
        MSG(SUSP,"Pause mode procedure is request\n\r");
		return RET_WAKE_REQ;
	}    
    if ( u4Status == (SETTLE_CPL | PAUSE_CPL) )
    {
        MSG(SUSP,"Waked up by sleep controller timeout\n\r");
		return RET_WAKE_TIMEOUT;
    }
    if ( u4Status == (SETTLE_CPL | PAUSE_INT) )
    {
        MSG(SUSP,"Waked up by wakeup interrupt source\n\r");
		return RET_WAKE_INTR;
    }
    if ( u4Status&PAUSE_ABORT )    
    {
		MSG(SUSP,"Pause mode is abort because of reception of INT prior entering pause mode\n\r");
		return RET_WAKE_ABORT;
    }
	return RET_WAKE_OTHERS;    
}    

UINT32 mt6516_pm_AcquireSLPclock(void)
{
	g_Sleep_lock ++ ;
	return 0;
}

UINT32 mt6516_pm_ReleaseSLPclock(void)
{
	if (g_Sleep_lock)
		g_Sleep_lock -- ;
	else
	{
		MSG(SUSP,"g_Sleep_lock error, less than zero\n\r");
		return -1;
	}
	return 0;
}

#ifndef CONFIG_MT6516_EVB_BOARD
extern int adc_cali_slop[9] ;
extern int adc_cali_offset[9] ;
extern int adc_cali_cal[1] ;

extern BOOL g_ADC_Cali;
extern BOOL g_ftm_battery_flag ;

#define LOW_BAT_ALARM 3400
#define VBAT_CHANNEL 	1
#define VBAT_COUNT 		3

int mt6516_pm_GetOneChannelValue(int dwChannel, int deCount)
{
    UINT32 u4Sample_times = 0;
    UINT32 dat = 0;
    UINT32 u4channel_0 = 0;    
    UINT32 u4channel_1 = 0;    
    UINT32 u4channel_2 = 0;    
    UINT32 u4channel_3 = 0;    
    UINT32 u4channel_4 = 0;    
    UINT32 u4channel_5 = 0;    
    UINT32 u4channel_6 = 0;    
    UINT32 u4channel_7 = 0;    
    UINT32 u4channel_8 = 0;    
    UINT32 temp_value = 0;

    /* Initialize ADC control register */
    DRV_WriteReg(AUXADC_CON0, 0);
    DRV_WriteReg(AUXADC_CON1, 0);    
    DRV_WriteReg(AUXADC_CON2, 0);    
    DRV_WriteReg(AUXADC_CON3, 0);   

    do
    {

        DRV_WriteReg(AUXADC_CON1, 0);        
        DRV_WriteReg(AUXADC_CON1, 0x1FF);
         
        DVT_DELAYMACRO(1000);

        /* Polling until bit STA = 0 */
        while (0 != (0x01 & DRV_Reg(AUXADC_CON3)));          

        dat = DRV_Reg(AUXADC_DAT0);        
        u4channel_0 += dat;
        dat = DRV_Reg(AUXADC_DAT1);        
        u4channel_1 += dat;   
        dat = DRV_Reg(AUXADC_DAT2);        
        u4channel_2 += dat;   
        dat = DRV_Reg(AUXADC_DAT3);        
        u4channel_3 += dat;   
        dat = DRV_Reg(AUXADC_DAT4);
        u4channel_4 += dat;
        dat = DRV_Reg(AUXADC_DAT5);
        u4channel_5 += dat;
        dat = DRV_Reg(AUXADC_DAT6);
        u4channel_6 += dat;  
        dat = DRV_Reg(AUXADC_DAT7);
        u4channel_7 += dat;
        dat = DRV_Reg(AUXADC_DAT8);
        u4channel_8 += dat;    
        
        u4Sample_times++;
    }
    while (u4Sample_times < deCount);

    
    /* Disable ADC power bit */    
    //mt6516_ADC_power_down();

    /* Gemini Phone */
    if (dwChannel==0) {
        temp_value = ( ( u4channel_0*2800/1024 )*2 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+0)))+(*(adc_cali_offset+0)))/1000;
        }
        return temp_value;
       }
    else if (dwChannel==1) {        
        temp_value = ( ( u4channel_1*2800/1024 )*2 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+1)))+(*(adc_cali_offset+1)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==2) {
        temp_value = ( u4channel_2*2800/1024 ); 
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+2)))+(*(adc_cali_offset+2)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==3) {
        temp_value = ( ( u4channel_3*2800/1024 )*124/24 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+3)))+(*(adc_cali_offset+3)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==4) {
        temp_value = ( u4channel_4*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+4)))+(*(adc_cali_offset+4)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==5) {
        temp_value = ( u4channel_5*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+5)))+(*(adc_cali_offset+5)))/1000;
        }
        return temp_value;        
    }
    else if (dwChannel==6) {
        temp_value = ( u4channel_6*2800/1024 );  
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+6)))+(*(adc_cali_offset+6)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==7) {
        temp_value = ( u4channel_7*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+7)))+(*(adc_cali_offset+7)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==8) {
        temp_value = ( u4channel_8*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+8)))+(*(adc_cali_offset+8)))/1000;
        }
        return temp_value;
    }
    else{        
        return -1;
    }
}
#endif
extern void MT6516_PLL_init(void);

void mt6516_pm_SuspendEnter(void)
{
    UINT32 u32TCM = 0xF0400000;
    UINT32 u4SuspendAddr = 0;
    UINT32 u4Status, u4BATVol;
	UINT32 counter = 0;

    /* Check Chip Version*/
    if (g_ChipVer == CHIP_VER_ECO_1)
        u4SuspendAddr = u32TCM;
    else if(g_ChipVer == CHIP_VER_ECO_2)
        u4SuspendAddr = __virt_to_phys((unsigned long)MT6516_CPUSuspend);

    /* Check PM related register*/
    mt6516_pm_RegDump();
    //mt6326_check_power();

	DRV_WriteReg32(APMCUSYS_PDN_SET0,0x04200000);	

    /* STEP7: Set AP_SM_CNF(DxF003C22C) to wanted wake-up source. */
#if defined(CONFIG_MT6516_EVB_BOARD)	
		mt6516_pm_SetWakeSrc((1<< WS_KP)|(1<<WS_EINT)|(1<<WS_RTC));
#elif defined(CONFIG_MT6516_PHONE_BOARD)
		mt6516_pm_SetWakeSrc((1<< WS_KP)|(1<<WS_EINT)|(1<<WS_CCIF)|(1<<WS_SM)|(1<<WS_RTC));
#else
		mt6516_pm_SetWakeSrc((1<<WS_EINT)|(1<<WS_CCIF)|(1<<WS_SM)|(1<<WS_RTC));
		//mt6516_pm_SetWakeSrc((1<<WS_SM));	
#endif

	/* Save interrupt masks*/
    irqMask_L = *MT6516_IRQ_MASKL;
    irqMask_H = *MT6516_IRQ_MASKH;
    mt6516_pm_Maskinterrupt(); // 20100316 James
	while(1)
	{
#ifdef AP_MD_EINT_SHARE_DATA
    /* Update Sleep flag*/
	mt6516_EINT_SetMDShareInfo();
	mt6516_pm_SleepWorkAround(); 
#endif
    /* Enter suspend mode, mt6516_slpctrl.s */
	if ( g_Sleep_lock <= 0 )
	    u4Status = MT6516_CPUSuspend (u4SuspendAddr, u32TCM); 
	else
        MSG(SUSP,"Someone lock sleep\n\r");
		
#ifdef AP_MD_EINT_SHARE_DATA
	mt6516_pm_SleepWorkAroundUp();
#endif

    /* Check Sleep status*/
    u4Status = mt6516_pm_CheckStatus();
	if (u4Status == RET_WAKE_TIMEOUT)
	{
#ifndef CONFIG_MT6516_EVB_BOARD
		DRV_WriteReg32(APMCUSYS_PDN_CLR0,0x04200000);		
		u4BATVol = (mt6516_pm_GetOneChannelValue(VBAT_CHANNEL,VBAT_COUNT)/VBAT_COUNT);		
		DRV_WriteReg32(APMCUSYS_PDN_SET0,0x04200000);		
		MSG(SUSP,"counter = %d, vbat = %d\n\r",counter++, u4BATVol);
		if(u4BATVol <= LOW_BAT_ALARM)
			goto SLP_EXIT;
#endif
	}
	else
	{
		MSG(SUSP,"leave sleep, wakeup!!\n\r");		
		goto SLP_EXIT;
		//break;
	}
	}
	
SLP_EXIT:	
	/* Restore interrupt mask ;  */   
	*MT6516_IRQ_MASKL = irqMask_L;
	*MT6516_IRQ_MASKH = irqMask_H;
    
} 

static dev_t SLP_FOP_devno;
static struct cdev *SLP_FOP_cdev;
static int SLP_FOP_major = 0;
static struct class *SLP_FOP_class = NULL;
#define SLP_FOP_DEVNAME "MT6516-SLP_FOP"

#define TEST_SLP_FACTORY_MODE 0

extern DISP_STATUS DISP_PanelEnable(BOOL bEnable);
extern DISP_STATUS DISP_PowerEnable(BOOL bEnable);
void mt6516_pm_SuspendEnter(void);


void Factory_Idle(void)
{

	printk("kernel Factory_Idle\n\r");
	MSG(MSGS,"EMI_CONN		*0xF0020068 = 0x%x\n\r",DRV_Reg32(0xF0020068)); 	
	
	//ARM 104
	SetARM9Freq(DIV_4_104) ; 
	//BL
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, 0);
	//CG
    DISP_PanelEnable(FALSE);
    DISP_PowerEnable(FALSE);		
	//MTCMOS
	hwDisableSubsys(MT6516_SUBSYS_GRAPH2SYS);
	hwDisableSubsys(MT6516_SUBSYS_CEVASYS);	
	//PLL
	hwDisablePLL(MT6516_PLL_DPLL, TRUE, "PM");
	//LDO
	mt6326_disable_VGP();
	mt6326_disable_VGP2();
	// Save interrupt status 
    irqMask_L = *MT6516_IRQ_MASKL;
    irqMask_H = *MT6516_IRQ_MASKH;
    *MT6516_IRQ_MASKL = 0xFFFFFFFF;
    *MT6516_IRQ_MASKH = 0xFFFFFFFF;
    *MT6516_IRQ_EOIL = 0xFFFFFFFF;
    *MT6516_IRQ_EOIH = 0xFFFFFFFF;
	MT6516_IRQUnmask(MT6516_KP_IRQ_LINE);
	
	//ARM9 WFI mode
	MT6516_CPUIdle (0, 0xF0001204);
	//mdelay(5000);

	// Restore interrupt mask 
	*MT6516_IRQ_MASKL = irqMask_L;
	*MT6516_IRQ_MASKH = irqMask_H;
	
	//LDO
	mt6326_enable_VGP();
	mt6326_enable_VGP2();
	//PLL
	hwEnablePLL(MT6516_PLL_DPLL,"PM");	
	//MTCMOS
	hwDisableSubsys(MT6516_SUBSYS_CEVASYS);
	hwDisableSubsys(MT6516_SUBSYS_GRAPH2SYS);

	//CG
    DISP_PowerEnable(TRUE);
    DISP_PanelEnable(TRUE);	
	//BL
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_FULL);
	//ARM 416
	SetARM9Freq(DIV_1_416) ; 	
}

static int SLP_FOP_open(struct inode *inode, struct file *file)
{ 
   return 0;
}

static int SLP_FOP_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int SLP_FOP_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{

    switch(cmd)
    {
        case TEST_SLP_FACTORY_MODE :
			printk("SLP_FOP_ioctl\n\r");
    		Factory_Idle();
            break;		  
        default:
            break;
    }	
    return 0;
}

static ssize_t
SLP_FOP_write(struct file *file, const char __user *ubuf,
		    size_t cnt, loff_t *ppos)
{
	Factory_Idle();
	return 0;
}


static struct file_operations SLP_FOP_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= SLP_FOP_ioctl,
	.open		= SLP_FOP_open,
	.release	= SLP_FOP_release,
	.write		= SLP_FOP_write,
};

void mt6516_pm_factory_init(void)
{
	struct class_device *class_dev = NULL;
    int ret = 0;
	
		ret = alloc_chrdev_region(&SLP_FOP_devno, 0, 1, SLP_FOP_DEVNAME);
		if (ret) 
			printk("Error: Can't Get Major number for SLP_FOP \n");
		SLP_FOP_cdev = cdev_alloc();
		SLP_FOP_cdev->owner = THIS_MODULE;
		SLP_FOP_cdev->ops = &SLP_FOP_fops;
		ret = cdev_add(SLP_FOP_cdev, SLP_FOP_devno, 1);
		if(ret)
			printk("SLP_FOP Error: cdev_add\n");
		SLP_FOP_major = MAJOR(SLP_FOP_devno);
		SLP_FOP_class = class_create(THIS_MODULE, SLP_FOP_DEVNAME);
		class_dev = (struct class_device *)device_create(SLP_FOP_class, 
														NULL, 
														SLP_FOP_devno, 
														NULL, 
														SLP_FOP_DEVNAME);
	

}





int _Chip_pm_begin(void)
{
	MSG_FUNC_ENTRY();
	
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_begin @@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

    /* We cannot sleep in idle until we have resumed */
    saved_idle = pm_idle;
    pm_idle = NULL;

	//mt6516_pm_RegDump();
	//mt6326_check_power();

    /* Workaround of AP / MD sleep sync issue*/
    //MT6516_EINTIRQMask(5);

    return 0;
}


int _Chip_pm_prepare(void)
{
#ifndef CONFIG_MT6516_EVB_BOARD
	MSG_FUNC_ENTRY();
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_prepare @@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	//mt6516_pm_RegDump();
	//mt6326_check_power();

//Fix ME !! V1 Layout issue patch, will remove at V2
#if defined CONFIG_MT6516_E1K_BOARD
    printk("E1000 Low power workaround\n\r");
    pmic_boost1_enable(FALSE);

#if 0
	/*GPIO18, Sensor workaround for V1*/
	mt_set_gpio_mode(18, 0);
	mt_set_gpio_dir(18, GPIO_DIR_OUT);
	mt_set_gpio_out(18, GPIO_OUT_ONE);
	printk("GPIO18: %d %d\n", mt_get_gpio_dir(18), mt_get_gpio_out(18));
	mt6326_enable_VCAM_A();
	/*GPIO34,35 I2C1 pull up workaround for V1 */
	mt_set_gpio_pull_enable(34, 1);
	mt_set_gpio_pull_enable(35, 1);
	mt_set_gpio_pull_select(34, GPIO_PULL_UP);
	mt_set_gpio_pull_select(35, GPIO_PULL_UP);
#endif
	g_GPIO_save[0] = DRV_Reg32(0xF0002000);
	g_GPIO_save[1] = DRV_Reg32(0xF0002610);
	g_GPIO_save[2] = DRV_Reg32(0xF0002660);

	DRV_WriteReg32(0xF0002000,0x4);
	DRV_WriteReg32(0xF0002610,0x5555);
	DRV_WriteReg32(0xF0002660,0xe555);

	//DRV_WriteReg32(0xF000130c,0x34c);
	//DRV_WriteReg32(0xF0039300,0xcbfff74c);
#endif

#if 1    
    /* Scale down ARM926 frequency and Vcore voltage*/
    SetARM9Freq(DIV_4_104);

	if(g_Power_optimize_Enable)
	{
		hwDisableSubsys(MT6516_SUBSYS_GRAPH1SYS);
		hwDisableSubsys(MT6516_SUBSYS_GRAPH2SYS);
	}
	// FIX ME !!, delete it when new OPPO v2 come
	//mt6326_disable_VGP2();

    /* LDO control*/
    //mt6326_disable_VUSB();  	
    //mt6326_disable_VSIM();  


	/* Prevent MT5911 external interrupt wake up whole system while in suspend mode */
    //MT6516_EINTIRQMask(5);

#endif
#endif

#ifdef CONFIG_MT6516_PHONE_BOARD
	mt6326_bl_Disable();
#endif

    return 0;
    
}



int _Chip_pm_enter(suspend_state_t state)
{
	MSG_FUNC_ENTRY();
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_enter @@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

	/* ensure the debug is initialised (if enabled) */
	switch (state) 
	{
    	case PM_SUSPEND_ON:
    		MSG(SUSP,"mt6516_pm_enter PM_SUSPEND_ON\n\r");
    		break;
    	case PM_SUSPEND_STANDBY:
    		MSG(SUSP,"mt6516_pm_enter PM_SUSPEND_STANDBY\n\r");        
    		break;
    	case PM_SUSPEND_MEM:
    		MSG(SUSP,"mt6516_pm_enter PM_SUSPEND_MEM\n\r");
            if (g_ChipVer == CHIP_VER_ECO_2)
	            mt6516_pm_SuspendEnter();    		
    		break;
    	case PM_SUSPEND_MAX:
    		MSG(SUSP,"mt6516_pm_enter PM_SUSPEND_MAX\n\r");        
    		MSG(SUSP,"Not support for MT6516\n\r");            		
    		break;
    		
    	default:
    	    MSG(SUSP,"mt6516_pm_enter Error state\n\r");
    		break;
	}
	return 0;
}


void _Chip_pm_finish(void)
{
    BOOL bStatus ;
    suspend_lock = FALSE;    
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_finish @@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
//Fix ME !! V1 Layout issue patch, will remove at V2
#if defined CONFIG_MT6516_E1K_BOARD
    printk("E1000 Low power workaround\n\r");
    pmic_boost1_enable(TRUE);
#if 0
	/*GPIO18*/
	mt_set_gpio_mode(18, 0);
	mt_set_gpio_dir(18, GPIO_DIR_OUT);
	mt_set_gpio_out(18, GPIO_OUT_ZERO);
	mt6326_disable_VCAM_A();
#endif
	DRV_WriteReg32(0xF0002000,g_GPIO_save[0]);
	DRV_WriteReg32(0xF0002610,g_GPIO_save[1]);
	DRV_WriteReg32(0xF0002660,g_GPIO_save[2]);
#endif

#ifndef CONFIG_MT6516_EVB_BOARD	
	//mt6516_pm_RegDump();
	//mt6326_check_power();

#if 1   
	if(g_Power_optimize_Enable)
	{
    	bStatus = hwEnableSubsys(MT6516_SUBSYS_GRAPH1SYS);	
		if (!bMTCMOS)
		{
    		bStatus = hwEnableSubsys(MT6516_SUBSYS_GRAPH2SYS);	
		}
	}
	// FIX ME !!, delete it when new OPPO v2 come
    //mt6326_enable_VGP2();

	// enable GMC
	DRV_WriteReg32(GRAPH1SYS_CG_CLR,0x1);

    /* Restore ARM speed*/    
    SetARM9Freq(DIV_1_416);

    /* LDO control*/
    //mt6326_enable_VUSB();  	
	
#endif
#endif

#ifdef CONFIG_MT6516_PHONE_BOARD
	mt6326_bl_Enable();
#endif


}

void _Chip_pm_end(void)
{
	MSG_FUNC_ENTRY();
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_end @@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

	//mt6516_pm_RegDump();
	//mt6326_check_power();

    pm_idle = saved_idle;    
}



void _Chip_PM_init(void)
{


    g_ChipVer = mt6516_pm_GetChipVer();

    /* Check Chip Version.*/
    /* ECO1 suspend function must execute in TCM */
    /* ECO2 suspend and DRAM self refresh mode is auto switch by HW */   
    if (g_ChipVer == CHIP_VER_ECO_1)
        DRV_SetReg32(HW_MISC, 0xC00);
    else
        DRV_SetReg32(HW_MISC, 0x8C00);

    /* sleep period*/
#if defined(CONFIG_MT6516_EVB_BOARD)	
    DRV_SetReg32(WAKE_APLL_SETTING, 0x4000); 	// infinite sleep
#else    
    printk("Before set 16s wakeup : %x!!\r\n",DRV_Reg32(WAKE_APLL_SETTING));	
    DRV_ClrReg32(WAKE_APLL_SETTING, 0x4000); 	// wake up after timeout
    printk("After set 16s wakeup : %x!!\r\n",DRV_Reg32(WAKE_APLL_SETTING));
    DRV_SetReg32(AP_SM_PAUSE_M, 0x0007); 		// 0x7FFFF * 32k tick ~= 16s
    DRV_SetReg32(AP_SM_PAUSE_L, 0xFFFF); 			
#endif	
    
    /* Init MT6516 power management data structure*/
    MT6516_hwPMInit();
   
    /* Power up CCIF share memory*/
    MT6516_AP_DisableMCUMEMPDN(PDN_MCU_CCIF);

	mt6516_pm_SetPLL();

}    

//Test Only
void mt6516_pm_audio(void)
{	

	   // GPIO
		//mt6516_GPIO_suspend_set2();
	
	
	   // Tail Part : saving 4 mA
	   //MIPI
		//Suspend_power_saving();
	
		//MCU
		DRV_WriteReg32(APMCUSYS_PDN_SET0,0xCFFD7F4E);
		//Graph1
		DRV_WriteReg32(GRAPH1SYS_CG_SET,0xFFEDFFFE);
		
		//hwDisableSubsys(MT6516_SUBSYS_GRAPH2SYS);
#if 0
	
	   //Front Part: 9 mA
	   
		//ARM 104 and voltage
		//SetARM9Freq(DIV_4_104) ; 
		//mt6326_VCORE_1_set_1_0();

		//BL
	        mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, 0);
		//CG
		DISP_PanelEnable(FALSE);
		DISP_PowerEnable(FALSE);  
		//MTCMOS
		 
		hwDisableSubsys(MT6516_SUBSYS_GRAPH2SYS);
		hwDisableSubsys(MT6516_SUBSYS_CEVASYS); 
		
		
		// Middle Part : saving 15mA
		//PLL : 2mA
		hwDisablePLL(MT6516_PLL_DPLL, TRUE, "PM");
		
		//LDO
		mt6326_disable_VGP();
		mt6326_disable_VGP2();
	
	   // GPIO
		mt6516_GPIO_suspend_set2();
	
	
	   // Tail Part : saving 4 mA
	   //MIPI
		Suspend_power_saving();
	
		//MCU
		DRV_WriteReg32(APMCUSYS_PDN_SET0,0xCFFD7F4E);
		//Graph1
		DRV_WriteReg32(GRAPH1SYS_CG_SET,0xFFEDFFFE);
	
		
		//3. SRAM
		// Graph1 MEM PDN
		DRV_WriteReg32(G1_MEM_PDN,0x3E3);
		// Graph2 MEM PDN
		DRV_WriteReg32(G2_MEM_PDN,0x1);
		// CEVA MEM PDN
		DRV_WriteReg32(CEVA_MEM_PDN,0x3F);
#endif	
	
}


EXPORT_SYMBOL(mt6516_pm_AcquireSLPclock);
EXPORT_SYMBOL(mt6516_pm_ReleaseSLPclock);
#ifdef AP_MD_EINT_SHARE_DATA
EXPORT_SYMBOL(mt6516_pm_register_sleep_info_func);
EXPORT_SYMBOL(mt6516_pm_unregister_sleep_info_func);
#endif

