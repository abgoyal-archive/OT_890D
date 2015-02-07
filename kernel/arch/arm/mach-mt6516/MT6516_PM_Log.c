


#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_ap_config.h>
#include <mach/mt6516_apmcusys.h>
#include <mach/mt6516_graphsys.h>
#include <mach/ccci_md.h>
#include <mach/ccci.h>

#include <asm/uaccess.h>

#include 	"../../../drivers/power/pmic6326_hw.h"
#include 	"../../../drivers/power/pmic6326_sw.h"
//#include 	"../../../drivers/power/pmic_drv.h"
#include 	<mach/MT6326PMIC_sw.h>

enum {
	DEBUG_EXIT_SUSPEND = 1U << 0,
	DEBUG_WAKEUP = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_EXPIRE = 1U << 3,
	DEBUG_WAKE_LOCK = 1U << 4,
};

#define  MAX_SET_NO	    4

ROOTBUS_HW g_MT6516_BusHW ;
extern int wakelock_debug_mask ;
extern int Userwakelock_debug_mask ;
extern int Earlysuspend_debug_mask ;
extern UINT32 g_GPIO_mask ;
extern int console_suspend_enabled;
extern UINT32 g_Power_optimize_Enable;

UINT32 PM_DBG_FLAG = TRUE;
UINT32 g_USBStatus = 0;

BOOL bPMIC_Init_finish = FALSE;


/* Debug message event */
#define DBG_PMAPI_NONE		    0x00000000	
#define DBG_PMAPI_CG			0x00000001	
#define DBG_PMAPI_PLL			0x00000002	
#define DBG_PMAPI_SUB			0x00000004	
#define DBG_PMAPI_PMIC			0x00000008	
#define DBG_PMAPI_ALL			0xFFFFFFFF	

#define DBG_PMAPI_MASK	   (DBG_PMAPI_ALL)

#if 1
#define MSG(evt, fmt, args...) \
do {	\
	if ((DBG_PMAPI_##evt) & DBG_PMAPI_MASK) { \
		printk(fmt, ##args); \
	} \
} while(0)

#define MSG_FUNC_ENTRY(f)	MSG(ENTER, "<PMAPI FUNC>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)	   do{}while(0)
#endif

#define CLOCK_PERI_PDN0     0x0010
#define CLOCK_PERI_PDN1     0x0020
#define CLOCK_GRAPH_SYS1    0x0030
#define CLOCK_GRAPH_SYS2    0x0040
#define CLOCK_SLEEP         0x0050

#define NON_OP "NOP"


extern ssize_t  mt6326_enable_VRF(void);
extern ssize_t  mt6326_disable_VRF(void);
extern ssize_t  mt6326_enable_VTCXO(void);
extern ssize_t  mt6326_disable_VTCXO(void);
extern ssize_t  mt6326_enable_V3GTX(void);
extern ssize_t  mt6326_disable_V3GTX(void);
extern ssize_t  mt6326_enable_V3GRX(void);
extern ssize_t  mt6326_disable_V3GRX(void);
extern ssize_t  mt6326_enable_VCAM_A(void);
extern ssize_t  mt6326_disable_VCAM_A(void);
extern ssize_t  mt6326_enable_VWIFI3V3(void);
extern ssize_t  mt6326_disable_VWIFI3V3(void);
extern ssize_t  mt6326_enable_VWIFI2V8(void);
extern ssize_t  mt6326_disable_VWIFI2V8(void);
extern ssize_t  mt6326_enable_VSIM(void);
extern ssize_t  mt6326_disable_VSIM(void);
extern ssize_t  mt6326_enable_VUSB(void);
extern ssize_t  mt6326_disable_VUSB(void);
extern ssize_t  mt6326_enable_VBT(void);
extern ssize_t  mt6326_disable_VBT(void);
extern ssize_t  mt6326_enable_VCAM_D(void);
extern ssize_t  mt6326_disable_VCAM_D(void);
extern ssize_t  mt6326_enable_VGP(void);
extern ssize_t  mt6326_disable_VGP(void);
extern ssize_t  mt6326_enable_VGP2(void);
extern ssize_t  mt6326_disable_VGP2(void);
extern ssize_t  mt6326_enable_VSDIO(void);
extern ssize_t  mt6326_disable_VSDIO(void);
extern ssize_t  mt6326_enable_VCORE_2(void);
extern ssize_t  mt6326_disable_VCORE_2(void);
extern void pmic_v3gtx_on_sel(v3gtx_on_sel_enum sel);
extern void pmic_v3grx_on_sel(v3grx_on_sel_enum sel);
extern void pmic_vgp2_on_sel(vgp2_on_sel_enum sel);
extern void pmic_vrf_on_sel(vrf_on_sel_enum sel);
extern void pmic_vtcxo_on_sel(vtcxo_on_sel_enum sel);

extern ssize_t mt6326_read_byte(u8 cmd, u8 *returnData);
extern ssize_t mt6326_write_byte(u8 cmd, u8 writeData);

extern BOOL MT6516_APMCUSYS_GetPDN1Status (APMCUSYS_PDNCONA1_MODE mode);
extern BOOL MT6516_APMCUSYS_GetPDN0Status (APMCUSYS_PDNCONA1_MODE mode);
extern void MT6516_APMCUSYS_DisablePDN0 (APMCUSYS_PDNCONA0_MODE mode);
extern void MT6516_APMCUSYS_DisablePDN1 (APMCUSYS_PDNCONA0_MODE mode);
extern void MT6516_APMCUSYS_EnablePDN0 (APMCUSYS_PDNCONA1_MODE mode);
extern void MT6516_APMCUSYS_EnablePDN1 (APMCUSYS_PDNCONA1_MODE mode);

extern BOOL MT6516_GRAPH1SYS_GetPDNStatus (GRAPH1SYS_PDN_MODE mode);
extern void MT6516_GRAPH1SYS_EnablePDN (GRAPH1SYS_PDN_MODE mode);
extern void MT6516_GRAPH1SYS_DisablePDN (GRAPH1SYS_PDN_MODE mode);
extern BOOL MT6516_GRAPH2SYS_GetPDNStatus (GRAPH2SYS_PDN_MODE mode);
extern void MT6516_GRAPH2SYS_EnablePDN (GRAPH2SYS_PDN_MODE mode);
extern void MT6516_GRAPH2SYS_DisablePDN (GRAPH2SYS_PDN_MODE mode);
extern void MT6516_APMCUSYS_DisableSLEEPCON (UINT32 value);
extern void MT6516_APMCUSYS_EnableSLEEPCON (UINT32 value);
extern INT32 mt6516_EINT_SetMDShareInfo(void);

extern void mt6516_pm_factory_init(void);


extern unsigned char pmic6326_eco_version ;
extern UINT32 g_ChipVer;





void MT6516_hwPMInit(void)
{
    UINT32 i,j,u4Status;
    BOOL bStatus = FALSE;

    //Clear all parameters
    for ( i=0; i < MT6516_CLOCK_COUNT_END; i++ )
    {
        g_MT6516_BusHW.device[i].dwClockCount = 0;		
        g_MT6516_BusHW.device[i].bDefault_on = FALSE;
    }
	for ( i=0; i < MT6516_POWER_COUNT_END; i++ )
        g_MT6516_BusHW.Power[i].dwPowerCount = 0;
    for ( i=0; i < MT6516_PLL_COUNT_END; i++ )
        g_MT6516_BusHW.Pll[i].dwPllCount = 0;

    g_MT6516_BusHW.dwSubSystem_status = 0;
	g_MT6516_BusHW.dwSubSystem_defaultOn = 0;

	for (i = MT6516_PLL_UPLL; i< MT6516_PLL_COUNT_END; i++)
	{
		for (j = 0; j< MAX_DEVICE; j++)
		{
			sprintf(g_MT6516_BusHW.Pll[i].mod_name[j] , "%s", NON_OP);
		}
	}

	for (i = MT6516_PDN_PERI_DMA; i< MT6516_CLOCK_COUNT_END; i++)
	{
		for (j = 0; j< MAX_DEVICE; j++)
		{
			sprintf(g_MT6516_BusHW.device[i].mod_name[j] , "%s", NON_OP);
		}
	}

	//Stop DPLL, Let PIC enable it
	DRV_ClrReg32(PDN_CON,1<<3);

    //PLL
    u4Status = DRV_Reg32(PDN_CON);
    if(u4Status & (1<<4))
    {
        g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].mod_name[0] , "default");
    }
	if(u4Status & (1<<3))
	{
        g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].mod_name[0] , "default");
	}	
    if(u4Status & (1<<2))
    {
        g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].mod_name[0] , "default");
    }
    u4Status = DRV_Reg32(CPLL2);
    if(u4Status & (1<<0))
    {
        g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].mod_name[0] , "default");
	}
    u4Status = DRV_Reg32(MCPLL2);
    if(u4Status & (1<<0))
    {
		g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].mod_name[0] , "default");
	}
    u4Status = DRV_Reg32(CEVAPLL2);
    if(u4Status & (1<<0))
    {
        g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].mod_name[0] , "default");
    }
	u4Status = DRV_Reg32(TPLL2);
    if(u4Status & (1<<0))
    {
        g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].mod_name[0] , "default");
	}
	u4Status = DRV_Reg32(MIPITX_CON);
	if(u4Status & (1<<0))
	{
        g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].bDefault_on = TRUE;
		//g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].dwPllCount++;
		//sprintf(g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].mod_name[0] , "default");
	}
	// 0: SUBSYS ENABLE 1: SUBSYS DISABLE
    u4Status = DRV_Reg32(PWR_OFF);
	if (!(u4Status & (1<<3)))
		g_MT6516_BusHW.dwSubSystem_defaultOn |= MT6516_SUBSYS_GRAPH1SYS;
	if (!(u4Status & (1<<4)))
		g_MT6516_BusHW.dwSubSystem_defaultOn |= MT6516_SUBSYS_GRAPH2SYS;
	if (!(u4Status & (1<<5)))
		g_MT6516_BusHW.dwSubSystem_defaultOn |= MT6516_SUBSYS_CEVASYS;

    // Adjust current subsys status value according to PWR_OFF register value
    g_MT6516_BusHW.dwSubSystem_status = g_MT6516_BusHW.dwSubSystem_defaultOn;

    //Initalize all power management counter    
    //MCU1
    for ( i = MT6516_PDN_PERI_DMA; i <= MT6516_PDN_PERI_ONEWIRE; i++ )
    {
        bStatus = MT6516_APMCUSYS_GetPDN0Status(i);
        if(!bStatus)
        {
            g_MT6516_BusHW.device[i].bDefault_on = TRUE;
			//g_MT6516_BusHW.device[i].dwClockCount++;			
			//sprintf(g_MT6516_BusHW.device[i].mod_name[0] , "default");		
        }
    }    
    //MCU2
    for ( i = MT6516_PDN_PERI_CSDBG; i <= MT6516_PDN_PERI_PWM0; i++ )
    {
        bStatus = MT6516_APMCUSYS_GetPDN1Status(i-MT6516_PDN1_MCU_START);
        if(!bStatus)
        {
            g_MT6516_BusHW.device[i].bDefault_on = TRUE;
			//g_MT6516_BusHW.device[i].dwClockCount++;			
			//sprintf(g_MT6516_BusHW.device[i].mod_name[0] , "default");					
        }
    }
    
    //MM1
	if ((g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH1SYS))
	{
	    for ( i = MT6516_PDN_MM_GMC1; i <= MT6516_PDN_MM_G1FAKE; i++ )
	    {
	        bStatus = MT6516_GRAPH1SYS_GetPDNStatus(i-MT6516_GRAPH1SYS_START);
	        if(!bStatus)
	        {
	            g_MT6516_BusHW.device[i].bDefault_on = TRUE;
				//g_MT6516_BusHW.device[i].dwClockCount++;			
				//sprintf(g_MT6516_BusHW.device[i].mod_name[0] , "default");					
	        }
	    }
	}
    //MM2
	if ((g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH2SYS))
	{
	    for ( i = MT6516_PDN_MM_GMC2; i <= MT6516_PDN_MM_MP4DBLK; i++ )
	    {
	        bStatus = MT6516_GRAPH2SYS_GetPDNStatus(i-MT6516_GRAPH2SYS_START);
	        if(!bStatus)
	        {
				g_MT6516_BusHW.device[i].bDefault_on = TRUE;		
				//g_MT6516_BusHW.device[i].dwClockCount++;			
				//sprintf(g_MT6516_BusHW.device[i].mod_name[0] , "default");					
	        }
		}
	}
   
    for ( i=0; i < MT6516_CLOCK_COUNT_END; i++ )
        g_MT6516_BusHW.device[i].Clockid = -1;
    
    g_MT6516_BusHW.device[MT6516_ID_CEVA].Clockid = MT6516_PLL_CEVAPLL;
    g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].Clockid = MT6516_PLL_UPLL;
    g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].Clockid = MT6516_PLL_TPLL;
    g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].Clockid = MT6516_PLL_CPLL;
    g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].Clockid = MT6516_PLL_MIPI;
    g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].Clockid = MT6516_PLL_MIPI;

	DRV_WriteReg32(0xF00026E0,0x15);
	    
}    

extern UINT32 bMTCMOS ;

static int hwPMMTCMOSRead(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{

	//struct wake_lock *lock;
	int len = 0;
	char *p = page;


    p += sprintf(p, "Mediatek Power DEBUG flag\n\r" );
    p += sprintf(p, "=========================================\n\r" );
    p += sprintf(p, "bMTCMOS = 0x%x\n\r",bMTCMOS);
    
	*start = page + off;

	len = p - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}    

static int
hwPMMTCMOSWrite (
    struct file *file,
    const char *buffer,
    unsigned long count,
    void *data
    )
{
    char acBuf[32]; 
    UINT32 u4CopySize = 0;
    UINT32 u4NewMTCMOS = 0;

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
    if(copy_from_user(acBuf, buffer, u4CopySize))
		return 0;
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, "%x", &u4NewMTCMOS ) == 1) {
        bMTCMOS = u4NewMTCMOS;
    }
    return count;
}



static int hwPMFlagRead(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{

	//struct wake_lock *lock;
	int len = 0;
	char *p = page;


    p += sprintf(p, "Mediatek Power DEBUG flag\n\r" );
    p += sprintf(p, "=========================================\n\r" );
    p += sprintf(p, "PM_DBG_FLAG = 0x%x\n\r",PM_DBG_FLAG);
    p += sprintf(p, "wakelock_debug_mask = 0x%x\n\r",wakelock_debug_mask);
    p += sprintf(p, "Userwakelock_debug_mask = 0x%x\n\r",Userwakelock_debug_mask);
    p += sprintf(p, "Earlysuspend_debug_mask = 0x%x\n\r",Earlysuspend_debug_mask);
    p += sprintf(p, "MT6516 console suspend enable = 0x%x\n\r",console_suspend_enabled);	

    p += sprintf(p, "MT6516 version = 0x%x\n\r",g_ChipVer);	
    if (pmic6326_eco_version == PMIC6326_ECO_1_VERSION)
        p += sprintf(p, "PMIC6326_ECO_1_VERSION\n\r" );
    else if (pmic6326_eco_version == PMIC6326_ECO_2_VERSION)
        p += sprintf(p, "PMIC6326_ECO_2_VERSION\n\r" );
    else if (pmic6326_eco_version == PMIC6326_ECO_3_VERSION)
        p += sprintf(p, "PMIC6326_ECO_3_VERSION\n\r" );
    else if (pmic6326_eco_version == PMIC6326_ECO_4_VERSION)
        p += sprintf(p, "PMIC6326_ECO_4_VERSION\n\r" );
    else
        p += sprintf(p, "UNKNOWN PMIC version\n\r" );

	p += sprintf(p, "USB Status %d\n\r", g_USBStatus );

			p += sprintf(p, "GPIO_DIR1 *0xF0002000 = 0x%x\n\r",DRV_Reg32(0xF0002000));
			p += sprintf(p, "GPIO_DIR2 *0xF0002010 = 0x%x\n\r",DRV_Reg32(0xF0002010));
			p += sprintf(p, "GPIO_DIR3 *0xF0002020 = 0x%x\n\r",DRV_Reg32(0xF0002020));
			p += sprintf(p, "GPIO_DIR4 *0xF0002030 = 0x%x\n\r",DRV_Reg32(0xF0002030));
			p += sprintf(p, "GPIO_DIR5 *0xF0002040 = 0x%x\n\r",DRV_Reg32(0xF0002040));
			p += sprintf(p, "GPIO_DIR6 *0xF0002050 = 0x%x\n\r",DRV_Reg32(0xF0002050));
			p += sprintf(p, "GPIO_DIR7 *0xF0002060 = 0x%x\n\r",DRV_Reg32(0xF0002060));
			p += sprintf(p, "GPIO_DIR8 *0xF0002070 = 0x%x\n\r",DRV_Reg32(0xF0002070));
			p += sprintf(p, "GPIO_DIR9 *0xF0002080 = 0x%x\n\r",DRV_Reg32(0xF0002080));
			p += sprintf(p, "GPIO_DIR10 *0xF0002090 = 0x%x\n\r",DRV_Reg32(0xF0002090));
			
			p += sprintf(p, "GPIO_PULLEN1 *0xF0002100 = 0x%x\n\r",DRV_Reg32(0xF0002100));
			p += sprintf(p, "GPIO_PULLEN2 *0xF0002110 = 0x%x\n\r",DRV_Reg32(0xF0002110));
			p += sprintf(p, "GPIO_PULLEN3 *0xF0002120 = 0x%x\n\r",DRV_Reg32(0xF0002120));
			p += sprintf(p, "GPIO_PULLEN4 *0xF0002130 = 0x%x\n\r",DRV_Reg32(0xF0002130));
			p += sprintf(p, "GPIO_PULLEN5 *0xF0002140 = 0x%x\n\r",DRV_Reg32(0xF0002140));
			p += sprintf(p, "GPIO_PULLEN6 *0xF0002150 = 0x%x\n\r",DRV_Reg32(0xF0002150));
			p += sprintf(p, "GPIO_PULLEN7 *0xF0002160 = 0x%x\n\r",DRV_Reg32(0xF0002160));
			p += sprintf(p, "GPIO_PULLEN8 *0xF0002170 = 0x%x\n\r",DRV_Reg32(0xF0002170));
			p += sprintf(p, "GPIO_PULLEN9 *0xF0002180 = 0x%x\n\r",DRV_Reg32(0xF0002180));
			p += sprintf(p, "GPIO_PULLEN10 *0xF0002190 = 0x%x\n\r",DRV_Reg32(0xF0002190));
			
			p += sprintf(p, "GPIO_PULLSEL1 *0xF0002200 = 0x%x\n\r",DRV_Reg32(0xF0002200));
			p += sprintf(p, "GPIO_PULLSEL2 *0xF0002210 = 0x%x\n\r",DRV_Reg32(0xF0002210));
			p += sprintf(p, "GPIO_PULLSEL3 *0xF0002220 = 0x%x\n\r",DRV_Reg32(0xF0002220));
			p += sprintf(p, "GPIO_PULLSEL4 *0xF0002230 = 0x%x\n\r",DRV_Reg32(0xF0002230));
			p += sprintf(p, "GPIO_PULLSEL5 *0xF0002240 = 0x%x\n\r",DRV_Reg32(0xF0002240));
			p += sprintf(p, "GPIO_PULLSEL6 *0xF0002250 = 0x%x\n\r",DRV_Reg32(0xF0002250));
			p += sprintf(p, "GPIO_PULLSEL7 *0xF0002260 = 0x%x\n\r",DRV_Reg32(0xF0002260));
			p += sprintf(p, "GPIO_PULLSEL8 *0xF0002270 = 0x%x\n\r",DRV_Reg32(0xF0002270));
			p += sprintf(p, "GPIO_PULLSEL9 *0xF0002280 = 0x%x\n\r",DRV_Reg32(0xF0002280));
			p += sprintf(p, "GPIO_PULLSEL10 *0xF0002290 = 0x%x\n\r",DRV_Reg32(0xF0002290));
			
			p += sprintf(p, "GPIO dataoutput *0xF0002400 = 0x%x\n\r",DRV_Reg32(0xF0002400));
			p += sprintf(p, "GPIO dataoutput *0xF0002410 = 0x%x\n\r",DRV_Reg32(0xF0002410));
			p += sprintf(p, "GPIO dataoutput *0xF0002420 = 0x%x\n\r",DRV_Reg32(0xF0002420));
			p += sprintf(p, "GPIO dataoutput *0xF0002430 = 0x%x\n\r",DRV_Reg32(0xF0002430));
			p += sprintf(p, "GPIO dataoutput *0xF0002440 = 0x%x\n\r",DRV_Reg32(0xF0002440));
			p += sprintf(p, "GPIO dataoutput *0xF0002450 = 0x%x\n\r",DRV_Reg32(0xF0002450));
			p += sprintf(p, "GPIO dataoutput *0xF0002460 = 0x%x\n\r",DRV_Reg32(0xF0002460));
			p += sprintf(p, "GPIO dataoutput *0xF0002470 = 0x%x\n\r",DRV_Reg32(0xF0002470));
			p += sprintf(p, "GPIO dataoutput *0xF0002480 = 0x%x\n\r",DRV_Reg32(0xF0002480));
			p += sprintf(p, "GPIO dataoutput *0xF0002490 = 0x%x\n\r",DRV_Reg32(0xF0002490));
			
			
			p += sprintf(p, "GPIO_mode1 *0xF0002600 = 0x%x\n\r",DRV_Reg32(0xF0002600));
			p += sprintf(p, "GPIO_mode2 *0xF0002610 = 0x%x\n\r",DRV_Reg32(0xF0002610));
			p += sprintf(p, "GPIO_mode3 *0xF0002620 = 0x%x\n\r",DRV_Reg32(0xF0002620));
			p += sprintf(p, "GPIO_mode4 *0xF0002630 = 0x%x\n\r",DRV_Reg32(0xF0002630));
			p += sprintf(p, "GPIO_mode5 *0xF0002640 = 0x%x\n\r",DRV_Reg32(0xF0002640));
			p += sprintf(p, "GPIO_mode6 *0xF0002650 = 0x%x\n\r",DRV_Reg32(0xF0002650));
			p += sprintf(p, "GPIO_mode7 *0xF0002660 = 0x%x\n\r",DRV_Reg32(0xF0002660));
			p += sprintf(p, "GPIO_mode8 *0xF0002670 = 0x%x\n\r",DRV_Reg32(0xF0002670));
			p += sprintf(p, "GPIO_mode9 *0xF0002680 = 0x%x\n\r",DRV_Reg32(0xF0002680));
			p += sprintf(p, "GPIO_mode10 *0xF0002690 = 0x%x\n\r",DRV_Reg32(0xF0002690));
			p += sprintf(p, "GPIO_mode11 *0xF00026A0 = 0x%x\n\r",DRV_Reg32(0xF00026A0));
			p += sprintf(p, "GPIO_mode12 *0xF00026B0 = 0x%x\n\r",DRV_Reg32(0xF00026B0));
			p += sprintf(p, "GPIO_mode13 *0xF00026C0 = 0x%x\n\r",DRV_Reg32(0xF00026C0));
			p += sprintf(p, "GPIO_mode14 *0xF00026D0 = 0x%x\n\r",DRV_Reg32(0xF00026D0));
			p += sprintf(p, "GPIO_mode15 *0xF00026E0 = 0x%x\n\r",DRV_Reg32(0xF00026E0));
			p += sprintf(p, "GPIO_mode16 *0xF00026F0 = 0x%x\n\r",DRV_Reg32(0xF00026F0));
			p += sprintf(p, "GPIO_mode17 *0xF0002700 = 0x%x\n\r",DRV_Reg32(0xF0002700));
			p += sprintf(p, "GPIO_mode18 *0xF0002710 = 0x%x\n\r",DRV_Reg32(0xF0002710));
			p += sprintf(p, "GPIO_mode19 *0xF0002720 = 0x%x\n\r",DRV_Reg32(0xF0002720));
			p += sprintf(p, "GPIO clk out *0xF0002900 = 0x%x\n\r",DRV_Reg32(0xF0002900));
			p += sprintf(p, "GPIO clk out *0xF0002910 = 0x%x\n\r",DRV_Reg32(0xF0002910));
			p += sprintf(p, "GPIO clk out *0xF0002920 = 0x%x\n\r",DRV_Reg32(0xF0002920));
			p += sprintf(p, "GPIO clk out *0xF0002930 = 0x%x\n\r",DRV_Reg32(0xF0002930));
			p += sprintf(p, "GPIO clk out *0xF0002940 = 0x%x\n\r",DRV_Reg32(0xF0002940));
			p += sprintf(p, "GPIO clk out *0xF0002950 = 0x%x\n\r",DRV_Reg32(0xF0002950));
			p += sprintf(p, "GPIO clk out *0xF0002960 = 0x%x\n\r",DRV_Reg32(0xF0002960));
			p += sprintf(p, "GPIO clk out *0xF0002970 = 0x%x\n\r",DRV_Reg32(0xF0002970));

    
	*start = page + off;

	len = p - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}    


static int
hwPMFlagWrite (
    struct file *file,
    const char *buffer,
    unsigned long count,
    void *data
    )
{
    char acBuf[32]; 
    UINT32 u4CopySize = 0;
    UINT32 u4NewPMDbgLevel = 0;
    UINT32 u4NewPMWakelockLevel = 0;
    UINT32 u4NewUserWakelockLevel = 0;
    UINT32 u4NewEarlysuspendLevel = 0;
    UINT32 u4ConsoleSuspend = 0;


    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
    if(copy_from_user(acBuf, buffer, u4CopySize))
		return 0;
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, "%x %x %x %x %x", &u4NewPMDbgLevel, &u4NewPMWakelockLevel, 
        &u4NewUserWakelockLevel, &u4NewEarlysuspendLevel, &u4ConsoleSuspend) == 5) {
        PM_DBG_FLAG = u4NewPMDbgLevel;
        wakelock_debug_mask = u4NewPMWakelockLevel;
        Userwakelock_debug_mask = u4NewUserWakelockLevel;
        Earlysuspend_debug_mask = u4NewEarlysuspendLevel;
		console_suspend_enabled = u4ConsoleSuspend;
    }
    return count;
}


static int hwPMStatus(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{

	//struct wake_lock *lock;
	int len = 0;
	char *p = page;

    kal_uint8 pmic6326_reg_4 = 0;
    kal_uint8 pmic6326_reg_5 = 0;
    kal_uint8 pmic6326_reg_6 = 0;
    kal_uint8 pmic6326_reg_7 = 0;
    kal_uint8 pmic6326_reg_8 = 0;
    kal_uint8 pmic6326_reg_1A = 0;

    int index = 0;	



    mt6326_read_byte(4, &pmic6326_reg_4);
    mt6326_read_byte(5, &pmic6326_reg_5);
    mt6326_read_byte(6, &pmic6326_reg_6);
    mt6326_read_byte(7, &pmic6326_reg_7);
    mt6326_read_byte(8, &pmic6326_reg_8);
    mt6326_read_byte(0x1A, &pmic6326_reg_1A);



    p += sprintf(p, "\n\rPLL Status\n\r" );
    p += sprintf(p, "=========================================\n\r" );        

	if(g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount)
		p += sprintf(p, "UPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount)
		p += sprintf(p, "DPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount)
		p += sprintf(p, "MPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount)
		p += sprintf(p, "CPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount)
		p += sprintf(p, "TPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount)
		p += sprintf(p, "CEVAPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount)
		p += sprintf(p, "MCPLL:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount);
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].dwPllCount)
		p += sprintf(p, "MIPI:%d ",g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].dwPllCount);

	p += sprintf(p, "\n\r ");


    //p += sprintf(p, "UPLL:%d DPLL:%d MPLL:%d CPLL:%d TPLL:%d CEVAPLL:%d MCPLL:%d\n", 
    //    g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount,
    //    g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount,g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount,
    //    g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount,g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount,
    //    g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount,g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount);


    p += sprintf(p, "\n\rPLL Master\n\r" );
    p += sprintf(p, "=========================================\n\r" );        
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		if(g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].dwPllCount > index )
			p += sprintf(p, "UPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].dwPllCount > index )
			p += sprintf(p, "DPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].dwPllCount > index )
			p += sprintf(p, "MPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].dwPllCount > index )
			p += sprintf(p, "CPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].dwPllCount > index )
			p += sprintf(p, "TPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].dwPllCount > index )
			p += sprintf(p, "CEVAPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].dwPllCount > index )
			p += sprintf(p, "MCPLL:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].mod_name[index]);
		if(g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].dwPllCount > index )
			p += sprintf(p, "MIPI:%s ",g_MT6516_BusHW.Pll[MT6516_PLL_MIPI].mod_name[index]);
		
		p += sprintf(p, "\n\r ");

		//p += sprintf(p, "UPLL:%s DPLL:%s MPLL:%s CPLL:%s TPLL:%s CEVAPLL:%s MCPLL:%s\n", 
		//	g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].mod_name[index],
		//	g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].mod_name[index],g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].mod_name[index],
		//	g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].mod_name[index],g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].mod_name[index],
		//	g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].mod_name[index],g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].mod_name[index]);
	}



    p += sprintf(p, "\n\rPMIC Status\n\r" );
    p += sprintf(p, "=========================================\n\r" );        

	if(g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount)
		p += sprintf(p, "VRF:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount)
		p += sprintf(p, "VTCXO:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount)
		p += sprintf(p, "V3GTX:%d ",g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].dwPowerCount)
		p += sprintf(p, "V3GRX:%d ",g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount)
		p += sprintf(p, "VCAMA:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount)
		p += sprintf(p, "VWIFI3V3:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount)
		p += sprintf(p, "VWIFI2V8:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount)
		p += sprintf(p, "VSIM:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount);

	if(g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount)
		p += sprintf(p, "VUSB:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VBT].dwPowerCount)
		p += sprintf(p, "VBT:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VBT].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount)
		p += sprintf(p, "VCAMD:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount)
		p += sprintf(p, "VGP:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount)
		p += sprintf(p, "VGP2:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount);
	if(g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount)
		p += sprintf(p, "VSDIO:%d ",g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount);
	p += sprintf(p, "\n\r ");


    //p += sprintf(p, "VRF:%d VTCXO:%d V3GTX:%d V3GRX:%d VCAMA:%d VWIFI3V3:%d VWIFI2V8:%d VSIM:%d\n", 
    //    g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount,g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount,
    //    g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].dwPowerCount,g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount,g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount);

    //p += sprintf(p, "VUSB:%d VBT:%d VCAMD:%d VGP:%d VGP2:%d VSDIO:%d \n", 
    //    g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VBT].dwPowerCount,g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount,g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount);


    p += sprintf(p, "\n\rPMIC Master\n\r" );
    p += sprintf(p, "=========================================\n\r" );        
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{

		if(g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount > index)
			p += sprintf(p, "VRF:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VRF].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount > index)
			p += sprintf(p, "VTCXO:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount > index)
			p += sprintf(p, "V3GTX:%s ",g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].dwPowerCount > index)
			p += sprintf(p, "V3GRX:%s ",g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount > index)
			p += sprintf(p, "VCAMA:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount > index)
			p += sprintf(p, "VWIFI3V3:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount > index)
			p += sprintf(p, "VWIFI2V8:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount > index)
			p += sprintf(p, "VSIM:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VSIM].mod_name[index]);
		
		if(g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount > index)
			p += sprintf(p, "VUSB:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VUSB].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VBT].dwPowerCount > index)
			p += sprintf(p, "VBT:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VBT].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount > index)
			p += sprintf(p, "VCAMD:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount > index)
			p += sprintf(p, "VGP:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VGP].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount > index)
			p += sprintf(p, "VGP2:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VGP2].mod_name[index]);
		if(g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount > index)
			p += sprintf(p, "VSDIO:%s ",g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].mod_name[index]);
		p += sprintf(p, "\n\r ");



		//p += sprintf(p, "VRF:%s VTCXO:%s V3GTX:%s V3GRX:%s VCAMA:%s VWIFI3V3:%s VWIFI2V8:%s VSIM:%s [%d]\n", 
		//	g_MT6516_BusHW.Power[MT6516_POWER_VRF].mod_name[index],
		//	g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].mod_name[index],
		//	g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].mod_name[index],
		//	g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].mod_name[index],
		//	g_MT6516_BusHW.Power[MT6516_POWER_VSIM].mod_name[index], index);

		//p += sprintf(p, "VUSB:%s VBT:%s VCAMD:%s VGP:%s VGP2:%s VSDIO:%s [%d]\n\n", 
		//	g_MT6516_BusHW.Power[MT6516_POWER_VUSB].mod_name[index],
		//	g_MT6516_BusHW.Power[MT6516_POWER_VBT].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].mod_name[index],
		//	g_MT6516_BusHW.Power[MT6516_POWER_VGP].mod_name[index],g_MT6516_BusHW.Power[MT6516_POWER_VGP2].mod_name[index],
		//	g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].mod_name[index], index);
	}


    p += sprintf(p, "\n\rMCU SYS CG Status\n\r" );
    p += sprintf(p, "=========================================\n\r" );    

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].dwClockCount)
		p += sprintf(p, "DMA:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].dwClockCount)
		p += sprintf(p, "USB:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].dwClockCount)
		p += sprintf(p, "SEJ:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].dwClockCount)
		p += sprintf(p, "I2C3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].dwClockCount)
		p += sprintf(p, "GPT:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].dwClockCount)
		p += sprintf(p, "KP:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].dwClockCount)
		p += sprintf(p, "GPIO:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].dwClockCount);
	//p += sprintf(p, "\n\r ");

//    p += sprintf(p, "DMA:%d USB:%d SEJ:%d I2C3:%d GPT:%d KP:%d GPIO:%d\n", 
//        g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].dwClockCount,
//        g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].dwClockCount,
//        g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].dwClockCount,
//        g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].dwClockCount,
//        g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].dwClockCount,
//        g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].dwClockCount,
//        g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].dwClockCount)
		p += sprintf(p, "UART1:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].dwClockCount)
		p += sprintf(p, "UART2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].dwClockCount)
		p += sprintf(p, "UART3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].dwClockCount)
		p += sprintf(p, "SIM:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].dwClockCount)
		p += sprintf(p, "PWM:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].dwClockCount)
		p += sprintf(p, "PWM1:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].dwClockCount)
		p += sprintf(p, "PWM2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].dwClockCount);
	//p += sprintf(p, "\n\r ");

    //p += sprintf(p, "UART1:%d UART2:%d UART3:%d SIM:%d PWM:%d PWM1:%d PWM2:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].dwClockCount)
		p += sprintf(p, "PWM3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].dwClockCount)
		p += sprintf(p, "MSDC:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].dwClockCount)
		p += sprintf(p, "SWDBG:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].dwClockCount)
		p += sprintf(p, "NFI:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].dwClockCount)
		p += sprintf(p, "I2C2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].dwClockCount)
		p += sprintf(p, "IRDA:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].dwClockCount)
		p += sprintf(p, "I2C:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].dwClockCount);
	//p += sprintf(p, "\n\r ");


    //p += sprintf(p, "PWM3:%d MSDC:%d SWDBG:%d NFI:%d I2C2:%d IRDA:%d I2C:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].dwClockCount)
		p += sprintf(p, "TOUC:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].dwClockCount)
		p += sprintf(p, "SIM2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].dwClockCount)
		p += sprintf(p, "MSDC2:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].dwClockCount)
		p += sprintf(p, "ADC:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].dwClockCount)
		p += sprintf(p, "TP:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].dwClockCount)
		p += sprintf(p, "XGPT:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].dwClockCount)
		p += sprintf(p, "UART4:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].dwClockCount);
	//p += sprintf(p, "\n\r ");


    //p += sprintf(p, "TOUC:%d SIM2:%d MSDC2:%d ADC:%d TP:%d XGPT:%d UART4:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].dwClockCount)
		p += sprintf(p, "MSDC3:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].dwClockCount)
		p += sprintf(p, "ONEWIRE:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].dwClockCount)
		p += sprintf(p, "CSDBG:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].dwClockCount)
		p += sprintf(p, "PWM0:%d ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].dwClockCount);
	p += sprintf(p, "\n\r ");

    //p += sprintf(p, "MSDC3:%d ONEWIRE:%d CSDBG:%d PWM0:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].dwClockCount);

    p += sprintf(p, "\n\rMM SYS CG Status\n\r" );
    p += sprintf(p, "=========================================\n\r" );    
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].dwClockCount)
		p += sprintf(p, "GMC1:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].dwClockCount)
		p += sprintf(p, "G2D:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].dwClockCount)
		p += sprintf(p, "GCMQ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].dwClockCount)
		p += sprintf(p, "BLS:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].dwClockCount)
		p += sprintf(p, "IMGDMA0:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].dwClockCount)
		p += sprintf(p, "PNG:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].dwClockCount)
		p += sprintf(p, "DSI:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].dwClockCount);
	//p += sprintf(p, "\n\r ");

    //p += sprintf(p, "GMC1:%d G2D:%d GCMQ:%d BLS:%d IMGDMA0:%d PNG:%d DSI:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].dwClockCount)
		p += sprintf(p, "TVE:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].dwClockCount)
		p += sprintf(p, "TVC:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].dwClockCount)
		p += sprintf(p, "ISP:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].dwClockCount)
		p += sprintf(p, "IPP:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].dwClockCount)
		p += sprintf(p, "PRZ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].dwClockCount)
		p += sprintf(p, "CRZ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].dwClockCount)
		p += sprintf(p, "DRZ:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].dwClockCount);
	//p += sprintf(p, "\n\r ");

    //p += sprintf(p, "TVE:%d TVC:%d ISP:%d IPP:%d PRZ:%d CRZ:%d DRZ:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_MM_WT].dwClockCount)
		p += sprintf(p, "WT:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_WT].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].dwClockCount)
		p += sprintf(p, "AFE:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].dwClockCount)
		p += sprintf(p, "SPI:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].dwClockCount)
		p += sprintf(p, "ASM:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].dwClockCount)
		p += sprintf(p, "RESZLB:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].dwClockCount)
		p += sprintf(p, "LCD:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].dwClockCount)
		p += sprintf(p, "DPI:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].dwClockCount);
	//p += sprintf(p, "\n\r ");

    //p += sprintf(p, "WT:%d AFE:%d SPI:%d ASM:%d RESZLB:%d LCD:%d DPI:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_WT].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].dwClockCount)
		p += sprintf(p, "G1FAKE:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].dwClockCount)
		p += sprintf(p, "GMC2:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].dwClockCount)
		p += sprintf(p, "IMGDMA1:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].dwClockCount)
		p += sprintf(p, "PRZ2:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].dwClockCount)
		p += sprintf(p, "M3D:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_H264].dwClockCount)
		p += sprintf(p, "H264:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_H264].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].dwClockCount)
		p += sprintf(p, "DCT:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].dwClockCount);
	//p += sprintf(p, "\n\r ");

    //p += sprintf(p, "G1FAKE:%d GMC2:%d IMGDMA1:%d PRZ2:%d M3D:%d H264:%d DCT:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_H264].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].dwClockCount);

	if(g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].dwClockCount)
		p += sprintf(p, "JPEG:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].dwClockCount)
		p += sprintf(p, "MP4:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].dwClockCount)
		p += sprintf(p, "MP4DBLK:%d ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_ID_CEVA].dwClockCount)
		p += sprintf(p, "CEVA:%d ",g_MT6516_BusHW.device[MT6516_ID_CEVA].dwClockCount);
	if(g_MT6516_BusHW.device[MT6516_IS_MD].dwClockCount)
		p += sprintf(p, "MD:%d ",g_MT6516_BusHW.device[MT6516_IS_MD].dwClockCount);
	p += sprintf(p, "\n\r ");

    //p += sprintf(p, "JPEG:%d MP4:%d MP4DBLK:%d CEVA:%d MD:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_ID_CEVA].dwClockCount,
    //    g_MT6516_BusHW.device[MT6516_IS_MD].dwClockCount);


	p += sprintf(p, "\n\rMCU SYS Master\n\r" );
	p += sprintf(p, "=========================================\n\r" );		  
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].dwClockCount > index )
			p += sprintf(p, "DMA:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].dwClockCount > index )
			p += sprintf(p, "USB:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].dwClockCount > index )
			p += sprintf(p, "SEJ:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].dwClockCount > index )
			p += sprintf(p, "I2C3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].dwClockCount > index )
			p += sprintf(p, "GPT:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].dwClockCount > index )
			p += sprintf(p, "KP:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].dwClockCount > index )
			p += sprintf(p, "GPIO:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].mod_name[index]);
		//p += sprintf(p, "\n\r ");

	
		//p += sprintf(p, "DMA:%s USB:%s SEJ:%s I2C3:%s GPT:%s KP:%s GPIO:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].dwClockCount > index )
			p += sprintf(p, "UART1:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].dwClockCount > index )
			p += sprintf(p, "UART2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].dwClockCount > index )
			p += sprintf(p, "UART3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].dwClockCount > index )
			p += sprintf(p, "SIM:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].dwClockCount > index )
			p += sprintf(p, "PWM:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].dwClockCount > index )
			p += sprintf(p, "PWM1:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].dwClockCount > index )
			p += sprintf(p, "PWM2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].mod_name[index]);
		//p += sprintf(p, "\n\r ");

		//p += sprintf(p, "UART1:%s UART2:%s UART3:%s SIM:%s PWM:%s PWM1:%s PWM2:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].dwClockCount > index )
			p += sprintf(p, "PWM3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].dwClockCount > index )
			p += sprintf(p, "MSDC:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].dwClockCount > index )
			p += sprintf(p, "SWDBG:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].dwClockCount > index )
			p += sprintf(p, "NFI:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].dwClockCount > index )
			p += sprintf(p, "I2C2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].dwClockCount > index )
			p += sprintf(p, "IRDA:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].dwClockCount > index )
			p += sprintf(p, "I2C:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].mod_name[index]);
		//p += sprintf(p, "\n\r ");

		//p += sprintf(p, "PWM3:%s MSDC:%s SWDBG:%s NFI:%s I2C2:%s IRDA:%s I2C:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].dwClockCount > index )
			p += sprintf(p, "TOUC:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].dwClockCount > index )
			p += sprintf(p, "SIM2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].dwClockCount > index )
			p += sprintf(p, "MSDC2:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].dwClockCount > index )
			p += sprintf(p, "ADC:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].dwClockCount > index )
			p += sprintf(p, "TP:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].dwClockCount > index )
			p += sprintf(p, "XGPT:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].dwClockCount > index )
			p += sprintf(p, "UART4:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].mod_name[index]);
		//p += sprintf(p, "\n\r ");


		//p += sprintf(p, "TOUC:%s SIM2:%s MSDC2:%s ADC:%s TP:%s XGPT:%s UART4:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].dwClockCount > index )
			p += sprintf(p, "MSDC3:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].dwClockCount > index )
			p += sprintf(p, "ONEWIRE:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].dwClockCount > index )
			p += sprintf(p, "CSDBG:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].dwClockCount > index )
			p += sprintf(p, "PWM0:%s ",g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].mod_name[index]);
		p += sprintf(p, "\n\r ");

		//p += sprintf(p, "MSDC3:%s ONEWIRE:%s CSDBG:%s PWM0:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].mod_name[index]);
	}

	p += sprintf(p, "\n\rMM SYS Master\n\r" );
	p += sprintf(p, "=========================================\n\r" );		  
	for (index = 0 ; index < MAX_DEVICE ; index++)
	{
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].dwClockCount > index )
			p += sprintf(p, "GMC1:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].dwClockCount > index )
			p += sprintf(p, "G2D:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].dwClockCount > index )
			p += sprintf(p, "GCMQ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].dwClockCount > index )
			p += sprintf(p, "BLS:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].dwClockCount > index )
			p += sprintf(p, "IMGDMA0:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].dwClockCount > index )
			p += sprintf(p, "PNG:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].dwClockCount > index )
			p += sprintf(p, "DSI:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].mod_name[index]);
		//p += sprintf(p, "\n\r ");

		//p += sprintf(p, "GMC1:%s G2D:%s GCMQ:%s BLS:%s IMGDMA0:%s PNG:%s DSI:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].dwClockCount > index )
			p += sprintf(p, "TVE:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].dwClockCount > index )
			p += sprintf(p, "TVC:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].dwClockCount > index )
			p += sprintf(p, "ISP:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].dwClockCount > index )
			p += sprintf(p, "IPP:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].dwClockCount > index )
			p += sprintf(p, "PRZ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].dwClockCount > index )
			p += sprintf(p, "CRZ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].dwClockCount > index )
			p += sprintf(p, "DRZ:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].mod_name[index]);
		//p += sprintf(p, "\n\r ");

		//p += sprintf(p, "TVE:%s TVC:%s ISP:%s IPP:%s PRZ:%s CRZ:%s DRZ:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_MM_WT].dwClockCount > index )
			p += sprintf(p, "WT:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_WT].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].dwClockCount > index )
			p += sprintf(p, "AFE:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].dwClockCount > index )
			p += sprintf(p, "SPI:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].dwClockCount > index )
			p += sprintf(p, "ASM:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].dwClockCount > index )
			p += sprintf(p, "RESZLB:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].dwClockCount > index )
			p += sprintf(p, "LCD:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].dwClockCount > index )
			p += sprintf(p, "DPI:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].mod_name[index]);
		//p += sprintf(p, "\n\r ");

		//p += sprintf(p, "WT:%s AFE:%s SPI:%s ASM:%s RESZLB:%s LCD:%s DPI:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_WT].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].dwClockCount > index )
			p += sprintf(p, "G1FAKE:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].dwClockCount > index )
			p += sprintf(p, "GMC2:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].dwClockCount > index )
			p += sprintf(p, "IMGDMA1:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].dwClockCount > index )
			p += sprintf(p, "PRZ2:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].dwClockCount > index )
			p += sprintf(p, "M3D:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_H264].dwClockCount > index )
			p += sprintf(p, "H264:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_H264].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].dwClockCount > index )
			p += sprintf(p, "DCT:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].mod_name[index]);
		//p += sprintf(p, "\n\r ");

		//p += sprintf(p, "G1FAKE:%s GMC2:%s IMGDMA1:%s PRZ2:%s M3D:%s H264:%s DCT:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_H264].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].mod_name[index]);

		if(g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].dwClockCount > index )
			p += sprintf(p, "JPEG:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].dwClockCount > index )
			p += sprintf(p, "MP4:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].dwClockCount > index )
			p += sprintf(p, "MP4DBLK:%s ",g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_ID_CEVA].dwClockCount > index )
			p += sprintf(p, "CEVA:%s ",g_MT6516_BusHW.device[MT6516_ID_CEVA].mod_name[index]);
		if(g_MT6516_BusHW.device[MT6516_IS_MD].dwClockCount > index )
			p += sprintf(p, "MD:%s ",g_MT6516_BusHW.device[MT6516_IS_MD].mod_name[index]);

		p += sprintf(p, "\n\r ");

		//p += sprintf(p, "JPEG:%s MP4:%s MP4DBLK:%s CEVA:%s MD:%s\n", 
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_ID_CEVA].mod_name[index],
		//	g_MT6516_BusHW.device[MT6516_IS_MD].mod_name[index]);
	}


	*start = page + off;

	len = p - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}



static int hwPMDefault(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{

	//struct wake_lock *lock;
	int len = 0;
	char *p = page;

    kal_uint8 pmic6326_reg_4 = 0;
    kal_uint8 pmic6326_reg_5 = 0;
    kal_uint8 pmic6326_reg_6 = 0;
    kal_uint8 pmic6326_reg_7 = 0;
    kal_uint8 pmic6326_reg_8 = 0;
    kal_uint8 pmic6326_reg_1A = 0;

    int index = 0;	
    kal_uint8 pmic6326_reg_ALL = 0;
	UINT32 u4Status = 0;

    mt6326_read_byte(4, &pmic6326_reg_4);
    mt6326_read_byte(5, &pmic6326_reg_5);
    mt6326_read_byte(6, &pmic6326_reg_6);
    mt6326_read_byte(7, &pmic6326_reg_7);
    mt6326_read_byte(8, &pmic6326_reg_8);
    mt6326_read_byte(0x1A, &pmic6326_reg_1A);

#if 1
		p += sprintf(p, "Power management status\n\r" );
		p += sprintf(p, "=========================================\n\r" );
		p += sprintf(p, "ARM9 FREQ     *0xF0001100 = 0x%x\n\r",DRV_Reg32(0xF0001100));
		p += sprintf(p, "sleep Control *0xF0001204 = 0x%x\n\r",DRV_Reg32(0xF0001204));
		p += sprintf(p, "Subsys OutIso *0xF0001300 = 0x%x\n\r",DRV_Reg32(0xF0001300));
		p += sprintf(p, "Subsys PowPDN *0xF0001304 = 0x%x\n\r",DRV_Reg32(0xF0001304));
		p += sprintf(p, "MCU MEMPDN    *0xF0001308 = 0x%x\n\r",DRV_Reg32(0xF0001308));
		p += sprintf(p, "GPH1 MEMPDN   *0xF000130c = 0x%x\n\r",DRV_Reg32(0xF000130c));
		p += sprintf(p, "GPH2 MEMPDN   *0xF0001310 = 0x%x\n\r",DRV_Reg32(0xF0001310));
		p += sprintf(p, "CEVA MEMPDN   *0xF0001314 = 0x%x\n\r",DRV_Reg32(0xF0001314));
		p += sprintf(p, "Subsys InIso  *0xF0001318 = 0x%x\n\r",DRV_Reg32(0xF0001318));
		p += sprintf(p, "APMCU PDN0    *0xF0039300 = 0x%x\n\r",DRV_Reg32(0xF0039300));
		p += sprintf(p, "APMCU PDN1    *0xF0039360 = 0x%x\n\r",DRV_Reg32(0xF0039360));
		p += sprintf(p, "GPH1 PDN0	   *0xF0092300 = 0x%x\n\r",DRV_Reg32(0xF0092300));
		p += sprintf(p, "GPH2 PDN0	   *0xF00A7000 = 0x%x\n\r",DRV_Reg32(0xF00A7000));
		p += sprintf(p, "PLL PDN_CON   *0xF0060010 = 0x%x\n\r",DRV_Reg32(0xF0060010));
		p += sprintf(p, "PLL CLK_CON   *0xF0060014 = 0x%x\n\r",DRV_Reg32(0xF0060014));
		p += sprintf(p, "PLL CPLL2	   *0xF006003C = 0x%x\n\r",DRV_Reg32(0xF006003C));
		p += sprintf(p, "PLL TPLL2	   *0xF0060044 = 0x%x\n\r",DRV_Reg32(0xF0060044));
		p += sprintf(p, "PLL MCPLL2    *0xF006005C = 0x%x\n\r",DRV_Reg32(0xF006005C));
		p += sprintf(p, "PLL CEVAPLL2  *0xF0060064 = 0x%x\n\r",DRV_Reg32(0xF0060064));
		p += sprintf(p, "PLL MIPITXPLL *0xF0060B00 = 0x%x\n\r",DRV_Reg32(0xF0060B00));	  
	
		p += sprintf(p, "\n\rPMIC LDO status\n\r" );
		p += sprintf(p, "=========================================\n\r" );	  
		p += sprintf(p, "VCORE 1 : %d\n", (pmic6326_reg_7>>7)&(0x01));
		p += sprintf(p, "VCORE 2 : %d\n", (pmic6326_reg_8>>3)&(0x01));
		p += sprintf(p, "VPA : %d\n", (pmic6326_reg_8>>5)&(0x01));	  
		p += sprintf(p, "VM : %d\n", (pmic6326_reg_8>>1)&(0x01));	 
		p += sprintf(p, "V3GTX : %d\n", (pmic6326_reg_4>>4)&(0x01));	
		p += sprintf(p, "V3GRX : %d\n", (pmic6326_reg_4>>6)&(0x01));
		p += sprintf(p, "VRF : %d\n", (pmic6326_reg_4>>0)&(0x01));	  
		p += sprintf(p, "VTCXO : %d\n", (pmic6326_reg_4>>2)&(0x01));
		p += sprintf(p, "VA : %d\n", (pmic6326_reg_5>>0)&(0x01));
		p += sprintf(p, "VCAMA : %d\n", (pmic6326_reg_5>>5)&(0x01));
		p += sprintf(p, "VWIFI3V3 : %d\n", (pmic6326_reg_5>>7)&(0x01));
		p += sprintf(p, "VWIFI2V8 : %d\n", (pmic6326_reg_6>>1)&(0x01));
		p += sprintf(p, "VIO : %d\n", (pmic6326_reg_5>>2)&(0x01));
		p += sprintf(p, "VSIM : %d\n", (pmic6326_reg_6>>3)&(0x01));
		p += sprintf(p, "VUSB : %d\n", (pmic6326_reg_6>>7)&(0x01));
		p += sprintf(p, "VBT : %d\n", (pmic6326_reg_6>>5)&(0x01));	  
		p += sprintf(p, "VCAMD : %d\n", (pmic6326_reg_7>>1)&(0x01));
		p += sprintf(p, "VSDIO : %d\n", (pmic6326_reg_7>>5)&(0x01));
		p += sprintf(p, "VGP : %d\n", (pmic6326_reg_7>>3)&(0x01));
		p += sprintf(p, "VGP2 : %d\n", (pmic6326_reg_1A>>2)&(0x01));	
		p += sprintf(p, "VRTC : %d\n", (pmic6326_reg_5>>4)&(0x01));
	
		p += sprintf(p, "\n\rSubsystem Status\n\r" );
		p += sprintf(p, "=========================================\n\r" );	  
		if ((g_MT6516_BusHW.dwSubSystem_status & MT6516_SUBSYS_GRAPH1SYS))
			p += sprintf(p, "GRAPH1SYS ");
		if ((g_MT6516_BusHW.dwSubSystem_status & MT6516_SUBSYS_GRAPH2SYS))
			p += sprintf(p, "GRAPH2SYS ");
		if ((g_MT6516_BusHW.dwSubSystem_status & MT6516_SUBSYS_CEVASYS))
			p += sprintf(p, "CEVASYS ");
		p += sprintf(p, "\n\r ");


		//p += sprintf(p, "GRAPH1SYS : %d\n", (g_MT6516_BusHW.dwSubSystem_status & MT6516_SUBSYS_GRAPH1SYS)? 1:0);
		//p += sprintf(p, "GRAPH2SYS : %d\n", (g_MT6516_BusHW.dwSubSystem_status & MT6516_SUBSYS_GRAPH2SYS)? 1:0);
		//p += sprintf(p, "CEVASYS : %d\n", (g_MT6516_BusHW.dwSubSystem_status & MT6516_SUBSYS_CEVASYS)? 1:0);

		p += sprintf(p, "\n\rSubsystem Default On\n\r" );
		p += sprintf(p, "=========================================\n\r" );	  
		if ((g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH1SYS))
			p += sprintf(p, "GRAPH1SYS ");
		if ((g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH2SYS))
			p += sprintf(p, "GRAPH2SYS ");
		if ((g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_CEVASYS))
			p += sprintf(p, "CEVASYS ");
		p += sprintf(p, "\n\r ");


		//p += sprintf(p, "GRAPH1SYS  : %d\n", (g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH1SYS)? 1:0);
		//p += sprintf(p, "GRAPH2SYS : %d\n", (g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH2SYS)? 1:0);
		//p += sprintf(p, "CEVASYS : %d\n", (g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_CEVASYS)? 1:0);

#endif


    p += sprintf(p, "\n\rPLL Default On\n\r" );
    p += sprintf(p, "=========================================\n\r" );        
	if(g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].bDefault_on)
		p += sprintf(p, "UPLL ");
	if(g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].bDefault_on)
		p += sprintf(p, "DPLL ");
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].bDefault_on)
		p += sprintf(p, "MPLL ");
	if(g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].bDefault_on)
		p += sprintf(p, "CPLL ");
	if(g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].bDefault_on)
		p += sprintf(p, "TPLL ");
	if(g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].bDefault_on)
		p += sprintf(p, "CEVAPLL ");
	if(g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].bDefault_on)
		p += sprintf(p, "MCPLL ");

	p += sprintf(p, "\n\r ");

    //p += sprintf(p, "UPLL:%d DPLL:%d MPLL:%d CPLL:%d TPLL:%d CEVAPLL:%d MCPLL:%d\n", 
      //  g_MT6516_BusHW.Pll[MT6516_PLL_UPLL].bDefault_on,
      //  g_MT6516_BusHW.Pll[MT6516_PLL_DPLL].bDefault_on,g_MT6516_BusHW.Pll[MT6516_PLL_MPLL].bDefault_on,
      //  g_MT6516_BusHW.Pll[MT6516_PLL_CPLL].bDefault_on,g_MT6516_BusHW.Pll[MT6516_PLL_TPLL].bDefault_on,
      //  g_MT6516_BusHW.Pll[MT6516_PLL_CEVAPLL].bDefault_on,g_MT6516_BusHW.Pll[MT6516_PLL_MCPLL].bDefault_on);


    p += sprintf(p, "\n\rPMIC LDO Default on\n\r" );
    p += sprintf(p, "=========================================\n\r" );        
	if(g_MT6516_BusHW.Power[MT6516_POWER_VRF].bDefault_on)
		p += sprintf(p, "VRF ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].bDefault_on)
		p += sprintf(p, "VTCXO ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].bDefault_on)
		p += sprintf(p, "V3GTX ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].bDefault_on)
		p += sprintf(p, "V3GRX ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].bDefault_on)
		p += sprintf(p, "VCAMA ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].bDefault_on)
		p += sprintf(p, "VWIFI3V3 ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].bDefault_on)
		p += sprintf(p, "VWIFI2V8 ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VSIM].bDefault_on)
		p += sprintf(p, "VSIM ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VUSB].bDefault_on)
		p += sprintf(p, "VUSB ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VBT].bDefault_on)
		p += sprintf(p, "VBT ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].bDefault_on)
		p += sprintf(p, "VCAMD ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VGP].bDefault_on)
		p += sprintf(p, "VGP ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VGP2].bDefault_on)
		p += sprintf(p, "VGP2 ");
	if(g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].bDefault_on)
		p += sprintf(p, "VSDIO ");
	p += sprintf(p, "\n\r ");
		


    //p += sprintf(p, "VRF:%d VTCXO:%d V3GTX:%d V3GRX:%d VCAMA:%d VWIFI3V3:%d VWIFI2V8:%d VSIM:%d\n", 
    //    g_MT6516_BusHW.Power[MT6516_POWER_VRF].bDefault_on,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].bDefault_on,g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].bDefault_on,
    //    g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].bDefault_on,g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].bDefault_on,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].bDefault_on,g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].bDefault_on,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VSIM].bDefault_on);
    //p += sprintf(p, "VUSB:%d VBT:%d VCAMD:%d VGP:%d VGP2:%d VSDIO:%d \n", 
    //    g_MT6516_BusHW.Power[MT6516_POWER_VUSB].bDefault_on,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VBT].bDefault_on,g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].bDefault_on,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VGP].bDefault_on,g_MT6516_BusHW.Power[MT6516_POWER_VGP2].bDefault_on,
    //    g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].bDefault_on);


    p += sprintf(p, "\n\rMCU SYS CG Default On\n\r" );
    p += sprintf(p, "=========================================\n\r" );    
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].bDefault_on)
		p += sprintf(p, "DMA ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].bDefault_on)
		p += sprintf(p, "USB ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].bDefault_on)
		p += sprintf(p, "SEJ ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].bDefault_on)
		p += sprintf(p, "I2C3 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].bDefault_on)
		p += sprintf(p, "GPT ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].bDefault_on)
		p += sprintf(p, "KP ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].bDefault_on)
		p += sprintf(p, "GPIO ");
	//p += sprintf(p, "\n\r ");


    //p += sprintf(p, "DMA:%d USB:%d SEJ:%d I2C3:%d GPT:%d KP:%d GPIO:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_DMA].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_USB].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_SEJ].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C3].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_GPT].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_KP].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_GPIO].bDefault_on);

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].bDefault_on)
		p += sprintf(p, "UART1 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].bDefault_on)
		p += sprintf(p, "UART2 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].bDefault_on)
		p += sprintf(p, "UART3 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].bDefault_on)
		p += sprintf(p, "SIM ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].bDefault_on)
		p += sprintf(p, "PWM ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].bDefault_on)
		p += sprintf(p, "PWM1 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].bDefault_on)
		p += sprintf(p, "PWM2 ");
	//p += sprintf(p, "\n\r ");


    //p += sprintf(p, "UART1:%d UART2:%d UART3:%d SIM:%d PWM:%d PWM1:%d PWM2:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART1].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART2].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART3].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM1].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM2].bDefault_on);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].bDefault_on)
		p += sprintf(p, "PWM3 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].bDefault_on)
		p += sprintf(p, "MSDC ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].bDefault_on)
		p += sprintf(p, "SWDBG ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].bDefault_on)
		p += sprintf(p, "NFI ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].bDefault_on)
		p += sprintf(p, "I2C2 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].bDefault_on)
		p += sprintf(p, "IRDA ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].bDefault_on)
		p += sprintf(p, "I2C ");
	//p += sprintf(p, "\n\r ");


    //p += sprintf(p, "PWM3:%d MSDC:%d SWDBG:%d NFI:%d I2C2:%d IRDA:%d I2C:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM3].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_SWDBG].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_NFI].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C2].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_IRDA].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_I2C].bDefault_on);

	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].bDefault_on)
		p += sprintf(p, "TOUC ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].bDefault_on)
		p += sprintf(p, "SIM2 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].bDefault_on)
		p += sprintf(p, "MSDC2 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].bDefault_on)
		p += sprintf(p, "ADC ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].bDefault_on)
		p += sprintf(p, "TP ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].bDefault_on)
		p += sprintf(p, "XGPT ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].bDefault_on)
		p += sprintf(p, "UART4 ");
	//p += sprintf(p, "\n\r ");

    //p += sprintf(p, "TOUC:%d SIM2:%d MSDC2:%d ADC:%d TP:%d XGPT:%d UART4:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_TOUC].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_SIM2].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC2].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_ADC].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_TP].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_XGPT].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_UART4].bDefault_on);
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].bDefault_on)
		p += sprintf(p, "MSDC3 ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].bDefault_on)
		p += sprintf(p, "ONEWIRE ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].bDefault_on)
		p += sprintf(p, "CSDBG ");
	if(g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].bDefault_on)
		p += sprintf(p, "PWM0 ");
	p += sprintf(p, "\n\r ");


    //p += sprintf(p, "MSDC3:%d ONEWIRE:%d CSDBG:%d PWM0:%d\n", 
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_MSDC3].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_ONEWIRE].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_CSDBG].bDefault_on,
    //    g_MT6516_BusHW.device[MT6516_PDN_PERI_PWM0].bDefault_on);

    p += sprintf(p, "\n\rMM SYS CG Default On\n\r" );
    p += sprintf(p, "=========================================\n\r" );  
    u4Status = DRV_Reg32(PWR_OFF);
    p += sprintf(p, "GRAPH1SYS : \n\r" );  	
	if (!(g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH1SYS))
	{
	    p += sprintf(p, "MTCMOS1 is off\n"); 
	}
	else	
	{
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].bDefault_on)
			p += sprintf(p, "GMC1 ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].bDefault_on)
			p += sprintf(p, "G2D ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].bDefault_on)
			p += sprintf(p, "GCMQ ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].bDefault_on)
			p += sprintf(p, "BLS ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].bDefault_on)
			p += sprintf(p, "IMGDMA0 ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].bDefault_on)
			p += sprintf(p, "PNG ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].bDefault_on)
			p += sprintf(p, "DSI ");
		//p += sprintf(p, "\n\r");
			
	    //p += sprintf(p, "GMC1:%d G2D:%d GCMQ:%d BLS:%d IMGDMA0:%d PNG:%d DSI:%d\n", 
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_GMC1].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_G2D].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_GCMQ].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_BLS].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA0].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_PNG].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DSI].bDefault_on);

		if (g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].bDefault_on)
			p += sprintf(p, "TVE ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].bDefault_on)
			p += sprintf(p, "TVC ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].bDefault_on)
			p += sprintf(p, "ISP ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].bDefault_on)
			p += sprintf(p, "IPP ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].bDefault_on)
			p += sprintf(p, "PRZ ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].bDefault_on)
			p += sprintf(p, "CRZ ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].bDefault_on)
			p += sprintf(p, "DRZ ");
		//p += sprintf(p, "\n\r");

		//p += sprintf(p, "TVE:%d TVC:%d ISP:%d IPP:%d PRZ:%d CRZ:%d DRZ:%d\n", 
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_TVE].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_TVC].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_ISP].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_IPP].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_CRZ].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DRZ].bDefault_on);

		if (g_MT6516_BusHW.device[MT6516_PDN_MM_WT].bDefault_on)
			p += sprintf(p, "WT ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].bDefault_on)
			p += sprintf(p, "AFE ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].bDefault_on)
			p += sprintf(p, "SPI ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].bDefault_on)
			p += sprintf(p, "ASM ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].bDefault_on)
			p += sprintf(p, "RESZLB ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].bDefault_on)
			p += sprintf(p, "LCD ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].bDefault_on)
			p += sprintf(p, "DPI ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].bDefault_on)
			p += sprintf(p, "G1FAKE ");
		p += sprintf(p, "\n\r");

	    //p += sprintf(p, "WT:%d AFE:%d SPI:%d ASM:%d RESZLB:%d LCD:%d DPI:%d G1FAKE:%d \n", 
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_WT].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_AFE].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_SPI].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_ASM].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_RESZLB].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_LCD].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DPI].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_G1FAKE].bDefault_on);
	}
    p += sprintf(p, "GRAPH2SYS : \n\r" );  		
	if (!(g_MT6516_BusHW.dwSubSystem_defaultOn & MT6516_SUBSYS_GRAPH2SYS))
	{
	    p += sprintf(p, "MTCMOS2 is off\n"); 
	}
	else
	{
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].bDefault_on)
			p += sprintf(p, "GMC2 ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].bDefault_on)
			p += sprintf(p, "IMGDMA1 ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].bDefault_on)
			p += sprintf(p, "PRZ2 ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].bDefault_on)
			p += sprintf(p, "M3D ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_H264].bDefault_on)
			p += sprintf(p, "H264 ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].bDefault_on)
			p += sprintf(p, "DCT ");
		p += sprintf(p, "\n\r ");
	
		//p += sprintf(p, "GMC2:%d IMGDMA1:%d PRZ2:%d M3D:%d H264:%d DCT:%d\n", 
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_GMC2].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_IMGDMA1].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_PRZ2].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_M3D].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_H264].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_DCT].bDefault_on);

		if (g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].bDefault_on)
			p += sprintf(p, "JPEG ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].bDefault_on)
			p += sprintf(p, "MP4 ");
		if (g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].bDefault_on)
			p += sprintf(p, "MP4DBLK ");
		if (g_MT6516_BusHW.device[MT6516_ID_CEVA].bDefault_on)
			p += sprintf(p, "CEVA ");
		if (g_MT6516_BusHW.device[MT6516_IS_MD].bDefault_on)
			p += sprintf(p, "MD ");

	    //p += sprintf(p, "JPEG:%d MP4:%d MP4DBLK:%d CEVA:%d MD:%d\n", 
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_JPEG].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_MP4].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_PDN_MM_MP4DBLK].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_ID_CEVA].bDefault_on,
	    //    g_MT6516_BusHW.device[MT6516_IS_MD].bDefault_on);
	}
	p += sprintf(p, "\n\rPMIC register Status\n\r" );
	p += sprintf(p, "=========================================\n\r" );	  
	for (index = 0 ; index < 0x97 ; index++)
	{
		mt6326_read_byte(index, &pmic6326_reg_ALL);
		p += sprintf(p, "reg%x: %x\n", index ,pmic6326_reg_ALL);
	}

	*start = page + off;

	len = p - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}
#if 1
BYTE g_pmic_default_setting[][2]={
#if 0 // no use
		{0x1B,0x08},
		{0x1C,0x02},            //VRF:  ON_SEL
		{0x1F,0x0},	  //VTCXO: ON_SEL
		{0x21,0x0},
		{0x22,0x40},//V3GTX: ON_SEL,EN,SEL
		{0x24,0x0},
		{0x25,0x40},//V3GRX: ON_SEL,EN,SEL
		{0x2E,0x0},
		{0x2F,0x0},//VCAMA:SEL,EN
		{0x31,0x2},
		{0x32,0x4},//VWIFI3V3:SEL,EN
		{0x34,0x0},
		{0x35,0x4},//VWFIF2V8:SEL,EN
		{0x37,0x0},//VSIM:SEL,EN
		{0x3A,0x0},//VBT:EN!!!!!!!!!!!!!!!!!!!
		{0x3D,0x8},//VUSB:SEL,EN!!!!!!!!!!!!!!
		{0x40,0x0},//VCAMA:SEL,EN
		{0x43,0x0},//VGP:SEL,EN
		{0x47,0x0},
		{0x46,0x0},//VSDIO:SEL,EN
		{0x84,0x80},
		{0x1A,0x4},
		{0x49,0x20},
		{0x5E,0x10},//VGP2:ON_SEL,SEL,EN
		{0x59,0x17},
		{0x58,0x0},//VPA:EN,SEL
		{0x52,0x00},
		{0x53,0x8},//VCORE2:ON_SEL,EN
		{0x5D,0x0},//VBOOTST1:EN
#endif
#if 0 
		//{0x60,0x0},//VBOOST2:EN
		{0x70,0x0},//VIB:EN
		{0x72,0x1}, //pls do not modify this value
		{0x6E,0x0},//KPAD:EN
		//{0x67,0x0},//VBL:EN
#endif
//		{0x65,0xF},
//		{0x66,0x0},//VFLASH:EN
		{0x63,0x0},//VBUS:EN
#if 0 
		{0x74,0x0},//SPKL:EN
		{0x77,0x02},
		{0x7c,0x02},
#endif
#if 0 // no use
		{0x75,0xA0},
		{0x7A,0xA0},
		{0x79,0x0},//SPKR:EN
		{0x89,0xFF},
		{0x8A,0xFF},
		{0x8B,0x3F},
		{0x8F,0xF},
		{0x92,0xF},
#endif		
		{0x0,0x0}  //END
};
#endif
void hwPMMonitorInit(void)
{
    struct proc_dir_entry *prEntry;
	UINT32 i, j, index;
	BOOL bRet = TRUE;
    
	kal_uint8 pmic6326_reg_ALL = 0;
    kal_uint8 pmic6326_reg_4 = 0;
    kal_uint8 pmic6326_reg_5 = 0;
    kal_uint8 pmic6326_reg_6 = 0;
    kal_uint8 pmic6326_reg_7 = 0;
    kal_uint8 pmic6326_reg_8 = 0;
    kal_uint8 pmic6326_reg_1A = 0;	

	if(g_Power_optimize_Enable)
	{
		for(i=0;g_pmic_default_setting[i][0x0]!=0;i++){
			bRet = mt6326_write_byte(g_pmic_default_setting[i][0x0],g_pmic_default_setting[i][0x1]);
		}
	}

	for (index = 0 ; index < 0x97 ; index++)
	{
		mt6326_read_byte(index, &pmic6326_reg_ALL);
		printk("reg%x: %x\n", index ,pmic6326_reg_ALL);
	}


    mt6326_read_byte(4, &pmic6326_reg_4);
    mt6326_read_byte(5, &pmic6326_reg_5);
    mt6326_read_byte(6, &pmic6326_reg_6);
    mt6326_read_byte(7, &pmic6326_reg_7);
    mt6326_read_byte(8, &pmic6326_reg_8);
    mt6326_read_byte(0x1A, &pmic6326_reg_1A);
    
	pmic_vtcxo_on_sel(VTCXO_ENABLE_WITH_VTCXO_EN); 

	pmic_v3gtx_on_sel(V3GTX_ENABLE_WITH_V3GTX_EN);
    mt6326_disable_V3GTX();  

    //mt6326_disable_VUSB();  	

	pmic_v3grx_on_sel(V3GRX_ENABLE_WITH_V3GRX_EN); 
    mt6326_disable_V3GRX();  

	for (i = MT6516_POWER_VRF; i< MT6516_POWER_COUNT_END; i++)
	{
		for (j = 0; j< MAX_DEVICE; j++)
		{
			sprintf(g_MT6516_BusHW.Power[i].mod_name[j] , "%s", NON_OP);
		}
	}

	if ((pmic6326_reg_4>>4)&(0x01))
	{
		g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].mod_name[0] , "default");
	}	
	if ((pmic6326_reg_4>>6)&(0x01))
	{		
		g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_V3GRX].mod_name[0] , "default");
	}
	if ((pmic6326_reg_4>>0)&(0x01))	                                 
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VRF].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_VRF].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VRF].mod_name[0] , "default");
	}
	if ((pmic6326_reg_4>>2)&(0x01))
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].bDefault_on = TRUE;				
		//g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VTCXO].mod_name[0] , "default");
	}
	if ((pmic6326_reg_5>>5)&(0x01))
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].bDefault_on = TRUE;				
		//g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_A].mod_name[0] , "default");
	}
	if ((pmic6326_reg_5>>7)&(0x01))                               
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].bDefault_on = TRUE;				
		//g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI3V3].mod_name[0] , "default");
	}
	if ((pmic6326_reg_6>>1)&(0x01))
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].bDefault_on = TRUE;				
		//g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VWIFI2V8].mod_name[0] , "default");
	}
	if ((pmic6326_reg_6>>3)&(0x01))                               
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VSIM].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_VSIM].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VSIM].mod_name[0] , "default");
	}
	if ((pmic6326_reg_6>>7)&(0x01))                               
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VUSB].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_VUSB].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VUSB].mod_name[0] , "default");
	}
	if ((pmic6326_reg_6>>5)&(0x01))	                                 
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VBT].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_V3GTX].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VBT].mod_name[0] , "default");
	}
	if ((pmic6326_reg_7>>1)&(0x01))                               
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VCAM_D].mod_name[0] , "default");
	}
	if ((pmic6326_reg_7>>5)&(0x01))                               
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VSDIO].mod_name[0] , "default");
	}
	if ((pmic6326_reg_7>>3)&(0x01))                               
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VGP].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_VGP].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VGP].mod_name[0] , "default");
	}
	if ((pmic6326_reg_1A>>2)&(0x01))	                               
	{
		g_MT6516_BusHW.Power[MT6516_POWER_VGP2].bDefault_on = TRUE;		
		//g_MT6516_BusHW.Power[MT6516_POWER_VGP2].dwPowerCount++ ;
		//sprintf(g_MT6516_BusHW.Power[MT6516_POWER_VGP2].mod_name[0] , "default");
	}

    // Crate proc entry at /proc/pm_stat, all PM messages
    prEntry = create_proc_read_entry("pm_stat", S_IRUGO, NULL, hwPMStatus, NULL);

    prEntry = create_proc_read_entry("pm_default", S_IRUGO, NULL, hwPMDefault, NULL);

    prEntry = create_proc_entry("pm_flag", 0, NULL);
    if (prEntry) {
        prEntry->read_proc = hwPMFlagRead;
        prEntry->write_proc = hwPMFlagWrite;
    }
    prEntry = create_proc_entry("pm_mtcmos", 0, NULL);
    if (prEntry) {
        prEntry->read_proc = hwPMMTCMOSRead;
        prEntry->write_proc = hwPMMTCMOSWrite;
    }

	mt6516_pm_factory_init();


#ifdef AP_MD_EINT_SHARE_DATA
    mt6516_EINT_SetMDShareInfo();
#endif
	bPMIC_Init_finish = TRUE;

#if defined(CONFIG_MT6516_E1K_BOARD)	
    hwPowerOn(MT6516_POWER_VGP2, VOL_1800, "Test");
#endif

}


EXPORT_SYMBOL(hwPMMonitorInit);


