


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

#ifdef CONFIG_PMIC_DCT
#include 	"pmic_drv.h"
#else
#include 	"../../../drivers/power/pmic6326_hw.h"
#include 	"../../../drivers/power/pmic6326_sw.h"
#include 	<mach/MT6326PMIC_sw.h>
#endif

enum {
	DEBUG_EXIT_SUSPEND = 1U << 0,
	DEBUG_WAKEUP = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_EXPIRE = 1U << 3,
	DEBUG_WAKE_LOCK = 1U << 4,
};

#define  MAX_SET_NO	    4

extern ROOTBUS_HW g_MT6516_BusHW ;
extern int wakelock_debug_mask ;
extern int Userwakelock_debug_mask ;
extern int Earlysuspend_debug_mask ;
extern UINT32 g_GPIO_mask ;
extern int console_suspend_enabled;

static spinlock_t MT6516_csClock_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t MT6516_csPLL_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t MT6516_csSYS_lock = SPIN_LOCK_UNLOCKED;

extern BOOL bPMIC_Init_finish;

static UINT16 s_backupRegVal_GRAPH1SYS_LCD_IO_SEL = 0;


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


#if 1
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

extern ssize_t  mt6326_V3GTX_set_2_5(void);
extern ssize_t  mt6326_V3GTX_set_2_8(void);
extern ssize_t  mt6326_V3GTX_set_3_0(void);
extern ssize_t  mt6326_V3GTX_set_3_3(void);

extern ssize_t  mt6326_V3GRX_set_2_5(void);
extern ssize_t  mt6326_V3GRX_set_2_8(void);
extern ssize_t  mt6326_V3GRX_set_3_0(void);
extern ssize_t  mt6326_V3GRX_set_3_3(void);

extern ssize_t  mt6326_VCAM_A_set_2_8(void);
extern ssize_t  mt6326_VCAM_A_set_2_5(void);
extern ssize_t  mt6326_VCAM_A_set_1_8(void);
extern ssize_t  mt6326_VCAM_A_set_1_5(void);

extern ssize_t  mt6326_VWIFI3V3_set_2_5(void);
extern ssize_t  mt6326_VWIFI3V3_set_2_8(void);
extern ssize_t  mt6326_VWIFI3V3_set_3_0(void);
extern ssize_t  mt6326_VWIFI3V3_set_3_3(void);

extern ssize_t  mt6326_VWIFI2V8_set_2_5(void);
extern ssize_t  mt6326_VWIFI2V8_set_2_8(void);
extern ssize_t  mt6326_VWIFI2V8_set_3_0(void);
extern ssize_t  mt6326_VWIFI2V8_set_3_3(void);

extern ssize_t  mt6326_VSIM_set_1_3(void);
extern ssize_t  mt6326_VSIM_set_1_5(void);
extern ssize_t  mt6326_VSIM_set_1_8(void);
extern ssize_t  mt6326_VSIM_set_2_5(void);
extern ssize_t  mt6326_VSIM_set_2_8(void);
extern ssize_t  mt6326_VSIM_set_3_0(void);
extern ssize_t  mt6326_VSIM_set_3_3(void);

extern ssize_t  mt6326_VBT_set_1_3(void);
extern ssize_t  mt6326_VBT_set_1_5(void);
extern ssize_t  mt6326_VBT_set_1_8(void);
extern ssize_t  mt6326_VBT_set_2_5(void);
extern ssize_t  mt6326_VBT_set_2_8(void);
extern ssize_t  mt6326_VBT_set_3_0(void);
extern ssize_t  mt6326_VBT_set_3_3(void);

extern ssize_t  mt6326_VCAMD_set_1_3(void);
extern ssize_t  mt6326_VCAMD_set_1_5(void);
extern ssize_t  mt6326_VCAMD_set_1_8(void);
extern ssize_t  mt6326_VCAMD_set_2_5(void);
extern ssize_t  mt6326_VCAMD_set_2_8(void);
extern ssize_t  mt6326_VCAMD_set_3_0(void);
extern ssize_t  mt6326_VCAMD_set_3_3(void);

extern ssize_t  mt6326_VGP_set_1_3(void);
extern ssize_t  mt6326_VGP_set_1_5(void);
extern ssize_t  mt6326_VGP_set_1_8(void);
extern ssize_t  mt6326_VGP_set_2_5(void);
extern ssize_t  mt6326_VGP_set_2_8(void);
extern ssize_t  mt6326_VGP_set_3_0(void);
extern ssize_t  mt6326_VGP_set_3_3(void);

extern ssize_t  mt6326_VGP2_set_1_3(void);
extern ssize_t  mt6326_VGP2_set_1_5(void);
extern ssize_t  mt6326_VGP2_set_1_8(void);
extern ssize_t  mt6326_VGP2_set_2_5(void);
extern ssize_t  mt6326_VGP2_set_2_8(void);
extern ssize_t  mt6326_VGP2_set_3_0(void);
extern ssize_t  mt6326_VGP2_set_3_3(void);

extern ssize_t  mt6326_VSDIO_set_2_8(void);
extern ssize_t  mt6326_VSDIO_set_3_0(void);

extern ssize_t  mt6326_VCORE_2_set_0_80(void);
extern ssize_t  mt6326_VCORE_2_set_0_85(void);
extern ssize_t  mt6326_VCORE_2_set_0_90(void);
extern ssize_t  mt6326_VCORE_2_set_0_95(void);
extern ssize_t  mt6326_VCORE_2_set_1_0(void);
extern ssize_t  mt6326_VCORE_2_set_1_05(void);
extern ssize_t  mt6326_VCORE_2_set_1_10(void);
extern ssize_t  mt6326_VCORE_2_set_1_15(void);
extern ssize_t  mt6326_VCORE_2_set_1_2(void);
extern ssize_t  mt6326_VCORE_2_set_1_25(void);
extern ssize_t  mt6326_VCORE_2_set_1_3(void);
extern ssize_t  mt6326_VCORE_2_set_1_35(void);
extern ssize_t  mt6326_VCORE_2_set_1_4(void);
extern ssize_t  mt6326_VCORE_2_set_1_45(void);
extern ssize_t  mt6326_VCORE_2_set_1_5(void);
extern ssize_t  mt6326_VCORE_2_set_1_55(void);


extern void pmic_v3gtx_on_sel(v3gtx_on_sel_enum sel);
extern void pmic_v3grx_on_sel(v3grx_on_sel_enum sel);
extern void pmic_vgp2_on_sel(vgp2_on_sel_enum sel);
extern void pmic_vrf_on_sel(vrf_on_sel_enum sel);
extern void pmic_vtcxo_on_sel(vtcxo_on_sel_enum sel);

extern ssize_t mt6326_read_byte(u8 cmd, u8 *returnData);
extern ssize_t mt6326_write_byte(u8 cmd, u8 writeData);
#endif

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

extern BOOL suspend_lock ;
extern UINT32 bMTCMOS ;


BOOL hwEnableSubsys(MT6516_SUBSYS subsysId);


BOOL hwEnablePLL(MT6516_PLL PllId, char *mode_name)
{    
    UINT32 i;
    //BOOL bStatus, bRegister = FALSE, bLocate = FALSE;
	
	//Error Check
	if ((PllId >= MT6516_PLL_COUNT_END) || (PllId < MT6516_PLL_UPLL))
	{
		MSG(PLL,"[PM LOG]PLL ID is wrong.\r\n");
 		return FALSE;
 	}
	if (suspend_lock)
	{
		//MSG(PLL,"[PM LOG]Enable PLL during suspend flow, PllId = %d\r\n",PllId);
 		//return FALSE;
	}    
	//Lock
	spin_lock(&MT6516_csPLL_lock);

	
	for (i = 0; i< MAX_DEVICE; i++)
	{
		if (!strcmp(g_MT6516_BusHW.Pll[PllId].mod_name[i], NON_OP))
		{
			MSG(PMIC,"[PM LOG][%s] acquire PLLId:%d index:%d mod_name: %s\r\n", 
				__FUNCTION__,PllId, i, mode_name);			
			sprintf(g_MT6516_BusHW.Pll[PllId].mod_name[i] , "%s", mode_name);
			break ;
		}
		//Has already register it
		else if (!strcmp(g_MT6516_BusHW.Pll[PllId].mod_name[i], mode_name))
		{
			//MSG(CG,"[%d] PLL already register just return\r\n",PllId );		
			//spin_unlock(&MT6516_csPLL_lock);
			//return TRUE;
			//break ;
		}

	}
	//Only new request, we add counter
	g_MT6516_BusHW.Pll[PllId].dwPllCount++;
	// Other guy has enable it before, just leave
	if(g_MT6516_BusHW.Pll[PllId].dwPllCount > 1)
	{
		spin_unlock(&MT6516_csPLL_lock);
		return TRUE;
	}
	switch(PllId)
	{
        case MT6516_PLL_UPLL :
            DRV_SetReg32(PDN_CON, (1<<4)); 
            DRV_ClrReg32(SLEEP_CON, 0x80);             
        break;
        case MT6516_PLL_DPLL :
            DRV_SetReg32(PDN_CON, (1<<3)); 
        break;
        case MT6516_PLL_MPLL :
            DRV_SetReg32(PDN_CON, (1<<2)); 
        break;
        case MT6516_PLL_CPLL :
            DRV_SetReg32(CPLL2, (1<<0)); 
        break;
        case MT6516_PLL_TPLL :
            DRV_SetReg32(TPLL2, (1<<0));             
        break;
        case MT6516_PLL_CEVAPLL :
            DRV_SetReg32(CEVAPLL2, (1<<0));                         
        break;
        case MT6516_PLL_MCPLL :
            DRV_SetReg32(MCPLL2, (1<<0));
            // reset MCPLL
            DRV_SetReg32 (MCPLL, 0x800); 
            for(i = 0 ; i < 100 ; i++);
            // releaes reset
            DRV_ClrReg32 (MCPLL, 0x800);
            while (!(DRV_Reg32(MCPLL) & 0x8000));
            
        break;
        case MT6516_PLL_MIPI :
	        DRV_ClrReg32(PLL_RES_CON0, (1<<0));    
        	DRV_SetReg32(MIPITX_CON, (1<<0));                                     
        break;
        case MT6516_PLL_COUNT_END :
		    MSG(PLL,"[PM LOG]PLL ID is wrong\r\n");
        break;
        
	}    
    spin_unlock(&MT6516_csPLL_lock);
    return TRUE;
} 

BOOL hwDisablePLL(MT6516_PLL PllId, BOOL bForce, char *mode_name)
{
	UINT32 i = 0;
	//MSG(PLL,"hwDisablePLL PLL ID [%d]\r\n", PllId);
	
	if ((PllId >= MT6516_PLL_COUNT_END) || (PllId < MT6516_PLL_UPLL))
	{
		MSG(PLL,"[PM LOG]PLL ID is wrong\r\n");
 		return FALSE;
 	}
 	
	spin_lock(&MT6516_csPLL_lock);

	if(!bForce)
	{	
		if(g_MT6516_BusHW.Pll[PllId].dwPllCount == 0)
		{		
			MSG(PLL,"[PM LOG]None use this clock, PllId = %d\r\n",PllId);			
			spin_unlock(&MT6516_csPLL_lock);
			return TRUE;
		}

		for (i = 0; i< MAX_DEVICE; i++)
		{
			//Got it, we release it
			if (!strcmp(g_MT6516_BusHW.Pll[PllId].mod_name[i], mode_name))
			{
				//MSG(PMIC,"[%s] powerId:%d index:%d mod_name: %s\r\n", 
				//	__FUNCTION__,PllId, i, mode_name);			
				sprintf(g_MT6516_BusHW.Pll[PllId].mod_name[i] , "%s", NON_OP);
				break ;
			}
		}

		g_MT6516_BusHW.Pll[PllId].dwPllCount--;

		if(g_MT6516_BusHW.Pll[PllId].dwPllCount > 0)
		{		
			//MSG(PLL,"Someone still use this clock, PllId = %d\r\n",PllId);			
			spin_unlock(&MT6516_csPLL_lock);
			return TRUE;
		}
	}
	// force release
	else
	{
		g_MT6516_BusHW.Pll[PllId].dwPllCount = 0;
		for (i = 0; i< MAX_DEVICE; i++)
		{
			sprintf(g_MT6516_BusHW.Pll[PllId].mod_name[i] , "%s", NON_OP);
		}		
	}
	switch(PllId)
	{
        case MT6516_PLL_UPLL :
            DRV_ClrReg32(PDN_CON, (1<<4)); 
            DRV_SetReg32(SLEEP_CON, 0x80);
        break;
        case MT6516_PLL_DPLL :
            DRV_ClrReg32(PDN_CON, (1<<3));  
        break;
        case MT6516_PLL_MPLL :
            DRV_ClrReg32(PDN_CON, (1<<2)); 
        break;
        case MT6516_PLL_CPLL :
            DRV_ClrReg32(CPLL2, (1<<0)); 
        break;
        case MT6516_PLL_TPLL :
            DRV_ClrReg32(TPLL2, (1<<0));              
        break;
        case MT6516_PLL_CEVAPLL :
            DRV_ClrReg32(CEVAPLL2, (1<<0));                          
        break;
        case MT6516_PLL_MCPLL :
            DRV_ClrReg32(MCPLL2, (1<<0));                                     
        break;
        case MT6516_PLL_MIPI :
        	DRV_ClrReg32(MIPITX_CON, (1<<0));                     
	        DRV_SetReg32(PLL_RES_CON0, (1<<0));            	
        break;
        case MT6516_PLL_COUNT_END :
        break;        
	}    
    spin_unlock(&MT6516_csPLL_lock);
    return TRUE;
}


#define CODEC_DRV_WriteReg32(addr,data)     ((*(volatile UINT32 *)(addr)) = (UINT32)(data))
#define CODEC_DRV_Reg32(addr)               (*(volatile UINT32 *)(addr))
/*Common, MP4_CODEC_COMD*/
#define MP4_CODEC_COMD_CORE_RST                    0x0001
#define MP4_CODEC_COMD_ENC_RST                     0x0002
#define MP4_CODEC_COMD_DEC_RST                     0x0004
#define MP4_base MP4_BASE //extern DWORD MP4_base;
#define MP4_CODEC_COMD                    (MP4_base+0x0) /*RW*/
static void reset_for_mpeg4_hw(void)
{
	UINT32 u4Count = 0;
    //UINT8 index;
    //printk("+reset_for_mpeg4_hw()\n");
    //MSG(SUB,"Enable MP4\r\n");	
		MT6516_GRAPH2SYS_DisablePDN(MT6516_PDN_MM_MP4 - MT6516_GRAPH2SYS_START);			  			
		for (u4Count = 0; u4Count <10000; u4Count++);
		
		CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 7);
		CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 7);
		CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 7);
		CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 7);
		CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 7);
		
    // encode reset
    //printk("WriteReg32(0x%X, 0x%X)\n", MP4_CODEC_COMD, MP4_CODEC_COMD_ENC_RST);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, MP4_CODEC_COMD_ENC_RST);
    //	for (index = 0; index < 20; index++)
    //		;
    for (u4Count = 0; u4Count <10000; u4Count++);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 0);

    //core reset
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, MP4_CODEC_COMD_CORE_RST);
    //	for (index = 0; index < 20; index++)
    //		;
    for (u4Count = 0; u4Count <10000; u4Count++);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 0);
    //printk("enter mpeg4_encoder_reset() done\n");    
    
    // decode reset
    //printk("WriteReg32(0x%X, 0x%X)\n", MP4_CODEC_COMD, MP4_CODEC_COMD_ENC_RST);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, MP4_CODEC_COMD_DEC_RST);
    //	for (index = 0; index < 20; index++)
    //		;
    for (u4Count = 0; u4Count <10000; u4Count++);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 0);

    //core reset
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, MP4_CODEC_COMD_CORE_RST);
    //	for (index = 0; index < 20; index++)
    //		;
    for (u4Count = 0; u4Count <10000; u4Count++);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 0);
    
    //MSG(SUB,"disable MP4\r\n");				
		MT6516_GRAPH2SYS_EnablePDN(MT6516_PDN_MM_MP4 - MT6516_GRAPH2SYS_START);
		for (u4Count = 0; u4Count <10000; u4Count++);
    //printk("-reset_for_mpeg4_hw()\n");
}

BOOL hwEnableSubsys(MT6516_SUBSYS subsysId)
{   
    UINT32 u4Ack = 0, u4Count = 0;

	if ((subsysId >= MT6516_SUBSYS_COUNT_END) || (subsysId < MT6516_SUBSYS_GRAPH1SYS))
	{
		MSG(SUB,"[PM LOG]SubsysId ID is wrong\r\n");
 		return FALSE;
 	}
	if (suspend_lock)
	{
		//MSG(SUB,"[PM LOG]enable subsystem during suspend flow, subsysId = %d\r\n",subsysId);
 		//return FALSE;
	}    
	
	// We've already enable it
    if (g_MT6516_BusHW.dwSubSystem_status & subsysId)
	{
		if(subsysId == MT6516_SUBSYS_GRAPH2SYS)
		{
			//MSG(SUB,"[%d] MTCMOS Already On\r\n",subsysId);				
		}
        return TRUE; 
    }

	spin_lock(&MT6516_csSYS_lock);

        // 1.Enable subsys reset
        // 2.Power on subsys
        // 3.wait power stable
        // 4.disable input ISO
        // 5.enable CLK
        // 6.disable output ISO
        // 7.disable subsys reset

	if(subsysId == MT6516_SUBSYS_GRAPH1SYS)
	{	DRV_WriteReg16(RGU_USRST3, 0xbb80);
	}
	else if(subsysId == MT6516_SUBSYS_GRAPH2SYS)
	{	DRV_WriteReg16(RGU_USRST4, 0xbb80);
	}
	else if(subsysId == MT6516_SUBSYS_CEVASYS)
	{	DRV_WriteReg16(RGU_USRST5, 0xbb80);          
	}

	if(subsysId == MT6516_SUBSYS_GRAPH1SYS)
	{	
		DRV_ClrReg16(PWR_OFF, 1<<3 );
		DRV_ClrReg16(IN_ISO_EN, 1<<3 );
		DRV_ClrReg16(ISO_EN, 1<<3);
		DRV_WriteReg16(RGU_USRST3, 0xbb00);

        // Restore GRAPH1SYS_CONFG register values after power on
        //
        DRV_WriteReg16(GRAPH1SYS_LCD_IO_SEL, s_backupRegVal_GRAPH1SYS_LCD_IO_SEL);
	}
	else if(subsysId == MT6516_SUBSYS_GRAPH2SYS)
	{	
		DRV_ClrReg16(PWR_OFF, 1<<4 );
		DRV_ClrReg16(IN_ISO_EN, 1<<4 );
		DRV_ClrReg16(ISO_EN, 1<<4);
		DRV_WriteReg16(RGU_USRST4, 0xbb00);
	}
	else if(subsysId == MT6516_SUBSYS_CEVASYS)
	{	
		DRV_ClrReg16(PWR_OFF, 1<<5 );
		DRV_ClrReg16(IN_ISO_EN, 1<<5 );
		DRV_ClrReg16(ISO_EN, 1<<5);
		DRV_WriteReg16(RGU_USRST5, 0xbb00);          
	}

	if(subsysId == MT6516_SUBSYS_GRAPH1SYS)
	{	DRV_WriteReg32(GRAPH1SYS_CG_CLR, 1);
	}
	else if(subsysId == MT6516_SUBSYS_GRAPH2SYS)
	{	DRV_WriteReg32(GRAPH2SYS_CG_CLR, 1);
	}	

    u4Ack = 0;

    g_MT6516_BusHW.dwSubSystem_status |= subsysId;

    while(!u4Ack && u4Count < 10000)    
    {
        u4Ack = DRV_Reg32(PWR_ACK);
    	switch(subsysId)
    	{
            case MT6516_SUBSYS_GRAPH1SYS :
                u4Ack &= 0x4;
            break;
            case MT6516_SUBSYS_GRAPH2SYS :
                u4Ack &= 0x2;                
            break;
            case MT6516_SUBSYS_CEVASYS :
                u4Ack &= 0x1;
            break;
            case MT6516_SUBSYS_COUNT_END :
                u4Ack &= 0x1;
            break;                       
    	}
    	u4Count++;
        
    }    
	// When graph1sys is enable
	// 1. auto enable GMC1
	// 2. delay 1ms
	if(subsysId == MT6516_SUBSYS_GRAPH1SYS)
	{
		MT6516_GRAPH1SYS_DisablePDN(MT6516_PDN_MM_GMC1 - MT6516_GRAPH1SYS_START);
		for (u4Count = 0; u4Count <1000; u4Count++);
	}
	// When graph2sys is enable 
	// 1. auto enable GMC2
	// 2. delay 1ms
	// 3.MP4DBLK will be enable automatically, we turn off it
	if(subsysId == MT6516_SUBSYS_GRAPH2SYS)
	{
		MT6516_GRAPH2SYS_EnablePDN(MT6516_PDN_MM_GMC2 - MT6516_GRAPH2SYS_START);			  			
		
		reset_for_mpeg4_hw();
		
		//MSG(SUB,"Enable GMC\r\n");		
		MT6516_GRAPH2SYS_DisablePDN(MT6516_PDN_MM_GMC2 - MT6516_GRAPH2SYS_START);			  			
		for (u4Count = 0; u4Count <10000; u4Count++);
		MT6516_GRAPH2SYS_EnablePDN(MT6516_PDN_MM_MP4DBLK - MT6516_GRAPH2SYS_START);
	}
	spin_unlock(&MT6516_csSYS_lock);
    return TRUE;
} 

BOOL hwDisableSubsys(MT6516_SUBSYS subsysId)
{
    //UINT32 u4Ack = 0, u4Count = 0;

	if ((subsysId >= MT6516_SUBSYS_COUNT_END) || (subsysId < MT6516_SUBSYS_GRAPH1SYS))
	{
		MSG(SUB,"[PM LOG]SubsysId ID is wrong\r\n");
 		return FALSE;
 	}

	// Disable already
    if ((g_MT6516_BusHW.dwSubSystem_status & subsysId) == FALSE)
    {
		MSG(SUB,"[PM LOG][%d] MTCMOS Already off\r\n",subsysId);		
        //return FALSE;  	
    }
    
	spin_lock(&MT6516_csSYS_lock);

	if(subsysId == MT6516_SUBSYS_GRAPH1SYS)
	{	
		DRV_WriteReg32(GRAPH1SYS_CG_SET, 1);
	}
	else if(subsysId == MT6516_SUBSYS_GRAPH2SYS)
	{	
		DRV_WriteReg32(GRAPH2SYS_CG_SET, 1);
	}


	if(subsysId == MT6516_SUBSYS_GRAPH1SYS)
	{	
        // Backup GRAPH1SYS_CONFG register values before power off
        //
        s_backupRegVal_GRAPH1SYS_LCD_IO_SEL = DRV_Reg16(GRAPH1SYS_LCD_IO_SEL);

	    DRV_SetReg16(ISO_EN, 1<<3);
	    DRV_SetReg16(IN_ISO_EN, 1<<3 );    
	    DRV_SetReg16(PWR_OFF, 1<<3 );
		DRV_SetReg16(ACK_CLR, 0x4); 
	}
	else if(subsysId == MT6516_SUBSYS_GRAPH2SYS)
	{	
	    DRV_SetReg16(ISO_EN, 1<<4);
	    DRV_SetReg16(IN_ISO_EN, 1<<4 );    
	    DRV_SetReg16(PWR_OFF, 1<<4 );
		DRV_SetReg16(ACK_CLR, 0x2); 
	}
	else if(subsysId == MT6516_SUBSYS_CEVASYS)
	{	
	    DRV_SetReg16(ISO_EN, 1<<5);
	    DRV_SetReg16(IN_ISO_EN, 1<<5 );    
	    DRV_SetReg16(PWR_OFF, 1<<5 );
		DRV_SetReg16(ACK_CLR, 0x1); 
	}

	g_MT6516_BusHW.dwSubSystem_status &= ~subsysId;
	
	spin_unlock(&MT6516_csSYS_lock);
    return TRUE;	
   
}



BOOL hwEnableClock(MT6516_CLOCK clockId, char *mode_name)
{
    UINT32 clock_catagory = 0, i; 
    BOOL bStatus, bRegister = FALSE, bLocate = FALSE;
	if ((clockId >= MT6516_CLOCK_COUNT_END) || (clockId < MT6516_PDN_PERI_DMA))
	{
		MSG(CG,"[PM LOG]ClockId is wrong\r\n");
 		return FALSE;
 	}
	if (suspend_lock)
	{
		//MSG(CG,"[PM LOG]enable clock during suspend flow, clockId = %d\r\n", clockId);
 		//return FALSE;
	}    

	spin_lock(&MT6516_csClock_lock);
#if 0
	for (i = 0; i< MAX_DEVICE; i++)
	{
		//already register
		if (!strcmp(g_MT6516_BusHW.device[clockId].mod_name[i], mode_name ))
		{
			bRegister = TRUE;
			break ;
			//MSG(CG,"[%d] CG already register just return\r\n",clockId );		
			//spin_unlock(&MT6516_csClock_lock);		
			//return TRUE;
		}
	}
	
	if (!bRegister)
	{
		for (i = 0; i< MAX_DEVICE; i++)
		{
			if (!strcmp(g_MT6516_BusHW.device[clockId].mod_name[i], NON_OP))
			{
				//MSG(PMIC,"[%s] acquire device:%d index:%d mod_name: %s\r\n", 
				//	__FUNCTION__,clockId, i, mode_name);			
				sprintf(g_MT6516_BusHW.device[clockId].mod_name[i] , "%s", mode_name);
				bLocate = TRUE;
				g_MT6516_BusHW.device[clockId].dwClockCount++;
				break ;
			}
		}
		if (!bLocate)
			MSG(CG,"[%d] CG all full \r\n",clockId);			
	}
	
#else
	for (i = 0; i< MAX_DEVICE; i++)
	{
		if (!strcmp(g_MT6516_BusHW.device[clockId].mod_name[i], NON_OP))
		{
			//MSG(PMIC,"[%s] acquire device:%d index:%d mod_name: %s\r\n", 
			//	__FUNCTION__,clockId, i, mode_name);			
			sprintf(g_MT6516_BusHW.device[clockId].mod_name[i] , "%s", mode_name);
			break ;
		}
		//already register just return
		else if (!strcmp(g_MT6516_BusHW.device[clockId].mod_name[i], mode_name ))
		{
			//MSG(CG,"[%d] CG already register just return\r\n",clockId );		
			//spin_unlock(&MT6516_csClock_lock);		
			//return TRUE;
		}

	}

	
	g_MT6516_BusHW.device[clockId].dwClockCount++;
#endif

    // MCU1
    if (clockId >= MT6516_PDN0_MCU_START && clockId <= MT6516_PDN0_MCU_END)
        clock_catagory = CLOCK_PERI_PDN0;

    // MCU2
    if (clockId >= MT6516_PDN1_MCU_START && clockId <= MT6516_PDN1_MCU_END)
        clock_catagory = CLOCK_PERI_PDN1;

    // MMSYS1
    if (clockId >= MT6516_GRAPH1SYS_START && clockId <= MT6516_GRAPH1SYS_END)
    {
        clock_catagory = CLOCK_GRAPH_SYS1;
        bStatus = hwEnableSubsys(MT6516_SUBSYS_GRAPH1SYS);
    }

    // MMSYS2
    if (clockId >= MT6516_GRAPH2SYS_START && clockId <= MT6516_GRAPH2SYS_END)
    {
        clock_catagory = CLOCK_GRAPH_SYS2;
		//printk("@@@@@@ Enable subsystem2 by [%d]\n\r",clockId );
        bStatus = hwEnableSubsys(MT6516_SUBSYS_GRAPH2SYS);
    }

    // CEVA
    if ( clockId == MT6516_ID_CEVA )
    {
		DRV_ClrReg32(SLEEP_CON, (1<<5));
        bStatus = hwEnableSubsys(MT6516_SUBSYS_CEVASYS);
    }

    // MD
    if (clockId == MT6516_IS_MD)
    {
		DRV_ClrReg32(SLEEP_CON, (1<<6));
    }


    // MD and CEVA
    if (clockId >= MT6516_SLEEP_START && clockId <= MT6516_SLEEP_END)
        clock_catagory = CLOCK_SLEEP;

    // AUTO enable PLL
    if (g_MT6516_BusHW.device[clockId].Clockid != -1)
    {
		MSG(PMIC,"[PM LOG][%d] CG auto enable PLL = %d\r\n",clockId ,g_MT6516_BusHW.device[clockId].Clockid);
        hwEnablePLL(g_MT6516_BusHW.device[clockId].Clockid, "AUTO");
    }
    switch (clock_catagory)
    {
        case CLOCK_PERI_PDN0:
            if (MT6516_APMCUSYS_GetPDN0Status(clockId))
                MT6516_APMCUSYS_DisablePDN0(clockId);            
        break;    

        case CLOCK_PERI_PDN1:
            if (MT6516_APMCUSYS_GetPDN1Status(clockId - MT6516_PDN1_MCU_START))
                MT6516_APMCUSYS_DisablePDN1(clockId - MT6516_PDN1_MCU_START);
        break;    

        case CLOCK_GRAPH_SYS1:
            if (MT6516_GRAPH1SYS_GetPDNStatus(clockId - MT6516_GRAPH1SYS_START))
                MT6516_GRAPH1SYS_DisablePDN(clockId - MT6516_GRAPH1SYS_START);            
        break;    

        case CLOCK_GRAPH_SYS2:
            if (MT6516_GRAPH2SYS_GetPDNStatus(clockId - MT6516_GRAPH2SYS_START))
                MT6516_GRAPH2SYS_DisablePDN(clockId - MT6516_GRAPH2SYS_START);
        break;

        case CLOCK_SLEEP:
            if(clockId == MT6516_ID_CEVA)
                MT6516_APMCUSYS_DisableSLEEPCON(0x20);
            else //MD
                MT6516_APMCUSYS_DisableSLEEPCON(0x40);                
        break;        

    }    

    spin_unlock(&MT6516_csClock_lock);
	
   	return TRUE;
}

BOOL hwDisableClock(MT6516_CLOCK clockId, char *mode_name)
{

    UINT32 clock_catagory = 0, i = 0, u4UsageStatus = 0;
	BOOL bFind = FALSE;

	// MD
    	if (clockId == MT6516_IS_MD)
	{
		DRV_SetReg32(SLEEP_CON, (1<<6));
		return TRUE;
	}

	if ((clockId >= MT6516_CLOCK_COUNT_END) || (clockId < MT6516_PDN_PERI_DMA))
	{
		MSG(CG,"[PM LOG]%s:%s:%d clockId:%d is wrong\r\n",__FILE__,__FUNCTION__,
		    __LINE__ , clockId);
		return FALSE;
	}

	spin_lock(&MT6516_csClock_lock);
	
	if(g_MT6516_BusHW.device[clockId].dwClockCount == 0)
	{
		spin_unlock(&MT6516_csClock_lock);
		MSG(CG,"[PM LOG]Disable clockId:%d which is not enable before\r\n",
		    clockId);
		return TRUE;
	}

	for (i = 0; i< MAX_DEVICE; i++)
	{
		if (!strcmp(g_MT6516_BusHW.device[clockId].mod_name[i], mode_name))
		{
			//MSG(PMIC,"[%s] powerId:%d index:%d mod_name: %s\r\n", 
			//	__FUNCTION__,clockId, i, mode_name);			
			sprintf(g_MT6516_BusHW.device[clockId].mod_name[i] , "%s", NON_OP);
			bFind = TRUE;
			break ;
		}
	}

	if(!bFind)
	{
		//MSG(PMIC,"[PM LOG][%s] Cannot find [%d] master is [%s]\r\n",__FUNCTION__,clockId, mode_name);		
		//spin_unlock(&MT6516_csClock_lock);		
		//return TRUE;
	}		


	g_MT6516_BusHW.device[clockId].dwClockCount --;


	if(g_MT6516_BusHW.device[clockId].dwClockCount > 0)
	{
		spin_unlock(&MT6516_csClock_lock);		
		return TRUE;
	}

    // MCU1
    if (clockId >= MT6516_PDN0_MCU_START && clockId <= MT6516_PDN0_MCU_END)
    {
        clock_catagory = CLOCK_PERI_PDN0;
        if (!MT6516_APMCUSYS_GetPDN0Status(clockId)) 
            MT6516_APMCUSYS_EnablePDN0(clockId);            
    }
    
    // MCU2
    if (clockId >= MT6516_PDN1_MCU_START && clockId <= MT6516_PDN1_MCU_END)
    {
        clock_catagory = CLOCK_PERI_PDN1;
        if (!MT6516_APMCUSYS_GetPDN1Status(clockId - MT6516_PDN1_MCU_START))
            MT6516_APMCUSYS_EnablePDN1(clockId - MT6516_PDN1_MCU_START);

    }
    // MM1, when only GMC1 is alive we disable MTCMOS1
    if (clockId >= (MT6516_GRAPH1SYS_START+1) && clockId <= MT6516_GRAPH1SYS_END)
    {
        clock_catagory = CLOCK_GRAPH_SYS1;
        if (!MT6516_GRAPH1SYS_GetPDNStatus(clockId - MT6516_GRAPH1SYS_START))
            MT6516_GRAPH1SYS_EnablePDN(clockId - MT6516_GRAPH1SYS_START);                    

        for (i = MT6516_GRAPH1SYS_START ; i<= MT6516_GRAPH1SYS_END; i++ )
            u4UsageStatus += g_MT6516_BusHW.device[i].dwClockCount ;
		// Disable MTCMOS1 is dangerous at running, move to suspend
		if(!u4UsageStatus)
        {
      		//MSG(PMIC,"[%d] GRAPH1SYS Disable\r\n",clockId);
            //hwDisableSubsys(MT6516_SUBSYS_GRAPH1SYS);
        }

    }

    // MM2, when only GMC2 is alive we disable MTCMOS2
    if (clockId >= (MT6516_GRAPH2SYS_START) && clockId <= MT6516_GRAPH2SYS_END)
    {
        clock_catagory = CLOCK_GRAPH_SYS2;
        if (!MT6516_GRAPH2SYS_GetPDNStatus(clockId - MT6516_GRAPH2SYS_START))
            MT6516_GRAPH2SYS_EnablePDN(clockId - MT6516_GRAPH2SYS_START);
        
        for (i = (MT6516_GRAPH2SYS_START) ; i<= MT6516_GRAPH2SYS_END; i++ )
            u4UsageStatus += g_MT6516_BusHW.device[i].dwClockCount ;
		// Disable MTCMOS2 is dangerous at running, move to suspend
        if(!u4UsageStatus)
        {
			if (bMTCMOS)
			{
				//printk("@@@@@@ Dis subsystem2 by [%d]\n\r",clockId );
	            hwDisableSubsys(MT6516_SUBSYS_GRAPH2SYS);
			}
		}                
    }

    // AUTO disable PLL
    if (g_MT6516_BusHW.device[clockId].Clockid != -1)
    {
        // Since DPI and DSI share the same MIPI PLL
        // Check if both DPI and DSI has been power down
        //
        BOOL canDisablePLL = TRUE;

        if (MT6516_PDN_MM_DPI == clockId ||
            MT6516_PDN_MM_DSI == clockId)
        {
            if (!MT6516_GRAPH1SYS_GetPDNStatus(MT6516_PDN_MM_DPI - MT6516_PDN_MM_GMC1) ||
                !MT6516_GRAPH1SYS_GetPDNStatus(MT6516_PDN_MM_DSI - MT6516_PDN_MM_GMC1))
            {
                canDisablePLL = FALSE;
            }
        }

        if (canDisablePLL)
        {
            hwDisablePLL(g_MT6516_BusHW.device[clockId].Clockid, FALSE, "AUTO");
        }
    }

    if (clockId >= MT6516_SLEEP_START && clockId <= MT6516_SLEEP_END)
    {
        clock_catagory = CLOCK_SLEEP;        
        MT6516_APMCUSYS_EnableSLEEPCON( (1<<(clockId - MT6516_SLEEP_START)) );

    }

    // CEVA
    if (clockId == MT6516_ID_CEVA)
    {
   		MSG(PMIC,"[PM LOG]CEVA SYS Disable\r\n");
		DRV_SetReg32(SLEEP_CON, (1<<5));
        hwDisableSubsys(MT6516_SUBSYS_CEVASYS);
    }

    
	spin_unlock(&MT6516_csClock_lock);
    return TRUE;
}


#ifdef DCT_PMIC

BOOL hwPowerOn(MT6516_POWER powerId, MT6516_POWER_VOLTAGE powervol, char *mode_name)
{
	UINT32 i = 0;
    //BOOL bStatus, bRegister = FALSE, bLocate = FALSE;
	
	if (!bPMIC_Init_finish)
		MSG(PMIC,"[PM LOG][%s] mode:%s PMIC init is not finished!! \r\n", __FUNCTION__, mode_name );
	
    if(powerId >= MT6516_POWER_COUNT_END)
    {
		MSG(PMIC,"[PM LOG] Error!! powerId is wrong\r\n");
        return FALSE;
    }
	if (suspend_lock)
	{
		//MSG(PMIC,"[PM LOG]enable PMIC LDO during suspend flow, powerId = %d\r\n", powerId);
 		//return FALSE;
	}    


	for (i = 0; i< MAX_DEVICE; i++)
	{
		if (!strcmp(g_MT6516_BusHW.Power[powerId].mod_name[i], NON_OP))
		{
			MSG(PMIC,"[PM LOG][%s] acquire powerId:%d index:%d mod_name: %s\r\n", 
				__FUNCTION__,powerId, i, mode_name);			
			sprintf(g_MT6516_BusHW.Power[powerId].mod_name[i] , "%s", mode_name);
			break ;
		}
		//already in it, just return		
		else if (!strcmp(g_MT6516_BusHW.Power[powerId].mod_name[i], mode_name))
		{
			//MSG(CG,"[%d] Power already register just return\r\n",powerId );		
			//spin_unlock(&MT6516_csPower_lock);
			//return TRUE;
		}

	}
	
	g_MT6516_BusHW.Power[powerId].dwPowerCount++ ;

    //We've already enable this LDO before
	if(g_MT6516_BusHW.Power[powerId].dwPowerCount > 1)
	{
		//MSG(PMIC,"[BUS] Error!! (g_MT6516_BusHW.dwPowerCount[powerId] > 1)\r\n");
		return TRUE;
	}
    
    switch (powerId)
    {
    
        // VRF
        case MT6516_POWER_VRF:
            //mt6326_enable_VRF();
        break;
        // VTCXO
        case MT6516_POWER_VTCXO:
            //mt6326_enable_VTCXO();
        break;
        // V3GTX
        case MT6516_POWER_V3GTX:
			//pmic_v3gtx_on_sel(V3GTX_ENABLE_WITH_V3GTX_EN);
            //mt6326_enable_V3GTX();
			GPS_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2800:
					GPS_Set_Volt(GPS_2_8);
                    //mt6326_V3GTX_set_2_8();
                break;
                case VOL_2500:					
					GPS_Set_Volt(GPS_2_5);
                    //mt6326_V3GTX_set_2_5(); 
                break;
                case VOL_3000:
					GPS_Set_Volt(GPS_3_0);
                    //mt6326_V3GTX_set_3_0(); 
                break;
                case VOL_3300:
					GPS_Set_Volt(GPS_3_3);
                    //mt6326_V3GTX_set_3_3();
                break;
                default :
					GPS_Set_Volt(GPS_3_3);
                    //mt6326_V3GTX_set_3_3();                
                break;
            }
        break;
        // V3GRX
        case MT6516_POWER_V3GRX:
			V3GRX_Enable(TRUE);
			//pmic_v3grx_on_sel(V3GRX_ENABLE_WITH_V3GRX_EN);
            //mt6326_enable_V3GRX();
            switch (powervol)
            {
                case VOL_2800:
					V3GRX_Set_Volt(V3GRX_2_8);
                    //mt6326_V3GRX_set_2_8();
                break;
                case VOL_2500:
					V3GRX_Set_Volt(V3GRX_2_5);
                    //mt6326_V3GRX_set_2_5(); 
                break;
                case VOL_3000:
					V3GRX_Set_Volt(V3GRX_3_0);
                    //mt6326_V3GRX_set_3_0(); 
                break;
                case VOL_3300:
					V3GRX_Set_Volt(V3GRX_3_3);
                    //mt6326_V3GRX_set_3_3();
                break;
                default :
					V3GRX_Set_Volt(V3GRX_3_3);
                    //mt6326_V3GRX_set_3_3();                
                break;
            }
        break;
        // VCAM_A
        case MT6516_POWER_VCAM_A:
            //mt6326_enable_VCAM_A();
            CAM1_A_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2500:
                    //mt6326_VCAM_A_set_2_5();
                    CAM1_A_Set_Volt(CAM1_A_2_5);
                break;
                case VOL_2800:
                    //mt6326_VCAM_A_set_2_8(); 
                    CAM1_A_Set_Volt(CAM1_A_2_8); 
                break;
                case VOL_1800:
                    //mt6326_VCAM_A_set_1_8(); 
                    CAM1_A_Set_Volt(CAM1_A_1_8); 
                break;
                case VOL_1500:
                    //mt6326_VCAM_A_set_1_5();
                    CAM1_A_Set_Volt(CAM1_A_1_5);
                break;
                default :
                    //mt6326_VCAM_A_set_2_8();                
                    CAM1_A_Set_Volt(CAM1_A_2_8);                
                break;
            }                                    
        break;
        // VWIFI3V3
        case MT6516_POWER_VWIFI3V3:
            //mt6326_enable_VWIFI3V3();
            WIFI_V1_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2500:
                    //mt6326_VWIFI3V3_set_2_5();
                    WIFI_V1_Set_Volt(WIFI_V1_2_5);
                break;
                case VOL_2800:
                    //mt6326_VWIFI3V3_set_2_8(); 
                    WIFI_V1_Set_Volt(WIFI_V1_2_8); 
                break;
                case VOL_3000:
                    //mt6326_VWIFI3V3_set_3_0(); 
                    WIFI_V1_Set_Volt(WIFI_V1_3_0); 
                break;
                case VOL_3300:
                    //mt6326_VWIFI3V3_set_3_3();
                    WIFI_V1_Set_Volt(WIFI_V1_3_3);
                break;
                default :
                    //mt6326_VWIFI3V3_set_2_5();                
                    WIFI_V1_Set_Volt(WIFI_V1_2_5);                
                break;
            }                                                
        break;
        // VWIFI2V8
        case MT6516_POWER_VWIFI2V8:
            //mt6326_enable_VWIFI2V8();
            WIFI_V2_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2500:
                    //mt6326_VWIFI2V8_set_2_5();
                    WIFI_V2_Set_Volt(WIFI_V2_2_5);
                break;
                case VOL_2800:
                    //mt6326_VWIFI2V8_set_2_8(); 
                    WIFI_V2_Set_Volt(WIFI_V2_2_8);
                break;
                case VOL_3000:
                    //mt6326_VWIFI2V8_set_3_0(); 
                    WIFI_V2_Set_Volt(WIFI_V2_3_0);
                break;
                case VOL_3300:
                    //mt6326_VWIFI2V8_set_3_3();
                    WIFI_V2_Set_Volt(WIFI_V2_3_3);
                break;
                default :
                    //mt6326_VWIFI2V8_set_3_3();                
                    WIFI_V2_Set_Volt(WIFI_V2_3_3);
                break;
            }                                                            
        break;
        // VSIM
        case MT6516_POWER_VSIM:
            //mt6326_enable_VSIM();
            SIM_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
                    //mt6326_VSIM_set_1_3();
                    SIM_Set_Volt(SIM_1_3);
                break;
                case VOL_1500:
                    //mt6326_VSIM_set_1_5(); 
                    SIM_Set_Volt(SIM_1_5);
                break;
                case VOL_1800:
                    //mt6326_VSIM_set_1_8(); 
                    SIM_Set_Volt(SIM_1_8);
                break;
                case VOL_2500:
                    //mt6326_VSIM_set_2_5();
                    SIM_Set_Volt(SIM_2_5);
                break;
                case VOL_2800:
                    //mt6326_VSIM_set_2_8(); 
                    SIM_Set_Volt(SIM_2_8);
                break;
                case VOL_3000:
                    //mt6326_VSIM_set_3_0(); 
                    SIM_Set_Volt(SIM_3_0);
                break;
                case VOL_3300:
                    //mt6326_VSIM_set_3_3();
                    SIM_Set_Volt(SIM_3_3);
                break;                
                default :
                    //mt6326_VSIM_set_3_3();                
                    SIM_Set_Volt(SIM_1_3);
                break;
            }                                                                        
        break;
        // VUSB
        case MT6516_POWER_VUSB:
            //mt6326_enable_VUSB();
            USB_Enable(TRUE);
        break;
        // VBT 
        case MT6516_POWER_VBT:
            //mt6326_enable_VBT();
            BT_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
                    //mt6326_VBT_set_1_3();
                    BT_Set_Volt(BT_1_3);
                break;
                case VOL_1500:
                    //mt6326_VBT_set_1_5(); 
                    BT_Set_Volt(BT_1_5);
                break;
                case VOL_1800:
                    //mt6326_VBT_set_1_8(); 
                    BT_Set_Volt(BT_1_8);
                break;
                case VOL_2500:
                    //mt6326_VBT_set_2_5();
                    BT_Set_Volt(BT_2_5);
                break;
                case VOL_2800:
                    //mt6326_VBT_set_2_8(); 
                    BT_Set_Volt(BT_2_8);
                break;
                case VOL_3000:
                    //mt6326_VBT_set_3_0(); 
                    BT_Set_Volt(BT_3_0);
                break;
                case VOL_3300:
                    //mt6326_VBT_set_3_3();
                    BT_Set_Volt(BT_3_3);
                break;                
                default :
                    //mt6326_VBT_set_3_3();                
                    BT_Set_Volt(BT_3_3);
                break;
            }                                                                                    
        break;
        // VCAM_D
        case MT6516_POWER_VCAM_D:
            //mt6326_enable_VCAM_D();
            CAM1_D_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
                    //mt6326_VCAMD_set_1_3();
                    CAM1_D_Set_Volt(CAM1_D_1_3);
                break;
                case VOL_1500:
                    //mt6326_VCAMD_set_1_5(); 
                    CAM1_D_Set_Volt(CAM1_D_1_5);
                break;
                case VOL_1800:
                    //mt6326_VCAMD_set_1_8(); 
                    CAM1_D_Set_Volt(CAM1_D_1_8);
                break;
                case VOL_2500:
                    //mt6326_VCAMD_set_2_5();
                    CAM1_D_Set_Volt(CAM1_D_2_5);
                break;
                case VOL_2800:
                    //mt6326_VCAMD_set_2_8(); 
                    CAM1_D_Set_Volt(CAM1_D_2_8);
                break;
                case VOL_3000:
                    //mt6326_VCAMD_set_3_0(); 
                    CAM1_D_Set_Volt(CAM1_D_3_0);
                break;
                case VOL_3300:
                    //mt6326_VCAMD_set_3_3();
                    CAM1_D_Set_Volt(CAM1_D_3_3);
                break;                
                default :
                    //mt6326_VCAMD_set_3_3();                
                    CAM1_D_Set_Volt(CAM1_D_3_3);
                break;
            }                                                                                                
        break;
        // VGP
        case MT6516_POWER_VGP:
		case MT6516_POWER_VSIM2:			
            //mt6326_enable_VGP();
            SIM2_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
					SIM2_Set_Volt(SIM2_1_3);
                    //mt6326_VGP_set_1_3();
                break;
                case VOL_1500:
					SIM2_Set_Volt(SIM2_1_5);
                    //mt6326_VGP_set_1_5(); 
                break;
                case VOL_1800:
					SIM2_Set_Volt(SIM2_1_8);
                    //mt6326_VGP_set_1_8(); 
                break;
                case VOL_2500:
					SIM2_Set_Volt(SIM2_2_5);
                    //mt6326_VGP_set_2_5();
                break;
                case VOL_2800:
					SIM2_Set_Volt(SIM2_2_8);
                    //mt6326_VGP_set_2_8(); 
                break;
                case VOL_3000:
					SIM2_Set_Volt(SIM2_3_0);
                    //mt6326_VGP_set_3_0(); 
                break;
                case VOL_3300:
					SIM2_Set_Volt(SIM2_3_3);
                    //mt6326_VGP_set_3_3();
                break;                
                default :
					SIM2_Set_Volt(SIM2_3_3);
                    //mt6326_VGP_set_3_3();                
                break;
            }
        break;
        // VGP2
        case MT6516_POWER_VGP2:
			//pmic_vgp2_on_sel(VGP2_ENABLE_WITH_VGP2_EN);
            //mt6326_enable_VGP2();
            GP2_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
					GP2_Set_Volt(GP2_1_3);
                    //mt6326_VGP2_set_1_3();
                break;
                case VOL_1500:
					GP2_Set_Volt(GP2_1_5);
                    //mt6326_VGP2_set_1_5(); 
                break;
                case VOL_1800:
					GP2_Set_Volt(GP2_1_8);
                    //mt6326_VGP2_set_1_8(); 
                break;
                case VOL_2500:
					GP2_Set_Volt(GP2_2_5);
                    //mt6326_VGP2_set_2_5();
                break;
                case VOL_2800:
					GP2_Set_Volt(GP2_2_8);
                    //mt6326_VGP2_set_2_8(); 
                break;
                case VOL_3000:
					GP2_Set_Volt(GP2_3_0);
                    //mt6326_VGP2_set_3_0(); 
                break;
                case VOL_3300:
					GP2_Set_Volt(GP2_3_3);
                    //mt6326_VGP2_set_3_3();
                break;                
                default :
					GP2_Set_Volt(GP2_3_3);
                    //mt6326_VGP2_set_3_3();                
                break;
            }

        break;
        // VSDIO
        case MT6516_POWER_VSDIO:
            //mt6326_enable_VSDIO();
            MC_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2800:
                    //mt6326_VSDIO_set_2_8(); 
                    MC_Set_Volt(MC_2_8); 
                break;
                case VOL_3000:
                    //mt6326_VSDIO_set_3_0();
                    MC_Set_Volt(MC_3_0); 
                break;                
                default :
                    //mt6326_VSDIO_set_3_0();                
                    MC_Set_Volt(MC_3_0); 
                break;
            }                                                                                                                                    
        break;

        // VCORE2, for WIMAX
        case MT6516_POWER_VCORE2:
            //mt6326_enable_VCORE_2();
            VCORE2_Enable(TRUE);
			#if 0
            switch (powervol)
            {
                case VOL_0800:
                    mt6326_VCORE_2_set_0_80(); 
                break;
                case VOL_0850:
                    mt6326_VCORE_2_set_0_85();
                break;                
                case VOL_0900:
                    mt6326_VCORE_2_set_0_90();
                break;                
                case VOL_0950:
                    mt6326_VCORE_2_set_0_95();
                break;                
                case VOL_1000:
                    mt6326_VCORE_2_set_1_0();
                break;                
                case VOL_1050:
                    mt6326_VCORE_2_set_1_05();
                break;                
                case VOL_1100:
                    mt6326_VCORE_2_set_1_10();
                break;                
                case VOL_1150:
                    mt6326_VCORE_2_set_1_15();
                break;                
                case VOL_1200:
                    mt6326_VCORE_2_set_1_2();
                break;                
                case VOL_1250:
                    mt6326_VCORE_2_set_1_25();
                break;                
                case VOL_1300:
                    mt6326_VCORE_2_set_1_3();
                break;                
                case VOL_1350:
                    mt6326_VCORE_2_set_1_35();
                break;                
                case VOL_1400:
                    mt6326_VCORE_2_set_1_4();
                break;                
                case VOL_1450:
                    mt6326_VCORE_2_set_1_45();
                break;                
                case VOL_1500:
                    mt6326_VCORE_2_set_1_5();
                break;                
                case VOL_1550:
                    mt6326_VCORE_2_set_1_55();
                break;                

                default :
                    mt6326_VCORE_2_set_1_3();                
                break;
            }
			#endif
        break;

        
        default:
            MSG(PMIC,"[PM LOG]PowerOn the power setting can't be recongnize\r\n");
            break;
    }

    return TRUE;    
}


BOOL hwPowerDown(MT6516_POWER powerId, char *mode_name)
{

	UINT32 i;
	BOOL bFind = FALSE;

	if (!bPMIC_Init_finish)
		MSG(PMIC,"[PM LOG][%s] mode:%s PMIC init is not finished!! \r\n", __FUNCTION__, mode_name );
	
    if(powerId >= MT6516_POWER_COUNT_END)
    {
		MSG(PMIC,"[PM LOG]%s:%s:%d powerId:%d is wrong\r\n",__FILE__,__FUNCTION__, 
		    __LINE__ , powerId);
        return FALSE;
    }

	
	if(g_MT6516_BusHW.Power[powerId].dwPowerCount == 0)
	{
		MSG(PMIC,"[PM LOG]%s:%s:%d powerId:%d (g_MT6516_BusHW.dwPowerCount[powerId] = 0)\r\n", 
		    __FILE__,__FUNCTION__,__LINE__ ,powerId);
		return FALSE;
	}

	for (i = 0; i< MAX_DEVICE; i++)
	{
		if (!strcmp(g_MT6516_BusHW.Power[powerId].mod_name[i], mode_name))
		{
			MSG(PMIC,"[PM LOG][%s] powerId:%d index:%d mod_name: %s\r\n", 
				__FUNCTION__,powerId, i, mode_name);			
			sprintf(g_MT6516_BusHW.Power[powerId].mod_name[i] , "%s", NON_OP);
			bFind = TRUE;
			break ;
		}
	}
	
	if(!bFind)
	{
		MSG(PMIC,"[PM LOG][%s] Cannot find [%d] master is [%s]\r\n",__FUNCTION__,powerId, mode_name);		
		spin_unlock(&MT6516_csClock_lock);		
		return TRUE;
	}		

	g_MT6516_BusHW.Power[powerId].dwPowerCount--;
	if(g_MT6516_BusHW.Power[powerId].dwPowerCount > 0)
	{
		return TRUE;
	}

    switch (powerId)
    {
        case MT6516_POWER_VRF:
            //mt6326_disable_VRF();
        break;
        case MT6516_POWER_VTCXO:
            //mt6326_disable_VTCXO();
        break;
        case MT6516_POWER_V3GTX:
			//pmic_v3gtx_on_sel(V3GTX_ENABLE_WITH_V3GTX_EN);
            //mt6326_disable_V3GTX();            
			GPS_Enable(FALSE);
        break;
        case MT6516_POWER_V3GRX:
			V3GRX_Enable(FALSE);
			//pmic_v3grx_on_sel(V3GRX_ENABLE_WITH_V3GRX_EN);
            //mt6326_disable_V3GRX();
        break;
        case MT6516_POWER_VCAM_A:
            //mt6326_disable_VCAM_A();
            CAM1_A_Enable(FALSE);
        break;
        case MT6516_POWER_VWIFI3V3:
            //mt6326_disable_VWIFI3V3();
            WIFI_V1_Enable(FALSE);
        break;
        case MT6516_POWER_VWIFI2V8:
            //mt6326_disable_VWIFI2V8();
            WIFI_V2_Enable(FALSE);
        break;
        case MT6516_POWER_VSIM:
            //mt6326_disable_VSIM();
            SIM_Enable(FALSE);
        break;
        case MT6516_POWER_VUSB:
            //mt6326_disable_VUSB();
            USB_Enable(FALSE);
        break;
        case MT6516_POWER_VBT:
            //mt6326_disable_VBT();
            BT_Enable(FALSE);
        break;
        case MT6516_POWER_VCAM_D:
            //mt6326_disable_VCAM_D();
            CAM1_D_Enable(FALSE);
        break;
        case MT6516_POWER_VSIM2:		
        case MT6516_POWER_VGP:
            //mt6326_disable_VGP();
            SIM2_Enable(FALSE);
        break;
        case MT6516_POWER_VGP2:
			GP2_Enable(FALSE);
			//pmic_vgp2_on_sel(VGP2_ENABLE_WITH_VGP2_EN);
            //mt6326_disable_VGP2();
        break;
        case MT6516_POWER_VSDIO:
            //mt6326_disable_VSDIO();
            MC_Enable(FALSE);
        break;
        case MT6516_POWER_VCORE2:
			VCORE2_Enable(FALSE);
            //mt6326_disable_VCORE_2();
        break;

        default:
            MSG(PMIC,"[PM LOG]PowerOFF won't be modified(powerId = %d)\r\n", powerId);
        break;
    }

    return TRUE;    
}


#else

BOOL hwPowerOn(MT6516_POWER powerId, MT6516_POWER_VOLTAGE powervol, char *mode_name)
{
	UINT32 i = 0;
    //BOOL bStatus, bRegister = FALSE, bLocate = FALSE;
	
	if (!bPMIC_Init_finish)
		MSG(PMIC,"[PM LOG][%s] mode:%s PMIC init is not finished!! \r\n", __FUNCTION__, mode_name );
	
    if(powerId >= MT6516_POWER_COUNT_END)
    {
		MSG(PMIC,"[PM LOG] Error!! powerId is wrong\r\n");
        return FALSE;
    }
	if (suspend_lock)
	{
		//MSG(PMIC,"[PM LOG]enable PMIC LDO during suspend flow, powerId = %d\r\n", powerId);
 		//return FALSE;
	}    


	for (i = 0; i< MAX_DEVICE; i++)
	{
		if (!strcmp(g_MT6516_BusHW.Power[powerId].mod_name[i], NON_OP))
		{
			MSG(PMIC,"[PM LOG][%s] acquire powerId:%d index:%d mod_name: %s\r\n", 
				__FUNCTION__,powerId, i, mode_name);			
			sprintf(g_MT6516_BusHW.Power[powerId].mod_name[i] , "%s", mode_name);
			break ;
		}
		//already it, just return		
		else if (!strcmp(g_MT6516_BusHW.Power[powerId].mod_name[i], mode_name))
		{
			//MSG(CG,"[%d] Power already register just return\r\n",powerId );		
			//spin_unlock(&MT6516_csPower_lock);
			//return TRUE;
		}

	}
	
	g_MT6516_BusHW.Power[powerId].dwPowerCount++ ;

    //We've already enable this LDO before
	if(g_MT6516_BusHW.Power[powerId].dwPowerCount > 1)
	{
		//MSG(PMIC,"[BUS] Error!! (g_MT6516_BusHW.dwPowerCount[powerId] > 1)\r\n");
		return TRUE;
	}
    
    switch (powerId)
    {
    
        // VRF
        case MT6516_POWER_VRF:
            //mt6326_enable_VRF();
        break;
        // VTCXO
        case MT6516_POWER_VTCXO:
            //mt6326_enable_VTCXO();
        break;
        // V3GTX
        case MT6516_POWER_V3GTX:
			//GPS_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2800:
					//GPS_Set_Volt(GPS_2_8);
                    mt6326_V3GTX_set_2_8();
                break;
                case VOL_2500:					
					//GPS_Set_Volt(GPS_2_5);
                    mt6326_V3GTX_set_2_5(); 
                break;
                case VOL_3000:
					//GPS_Set_Volt(GPS_3_0);
                    mt6326_V3GTX_set_3_0(); 
                break;
                case VOL_3300:
					//GPS_Set_Volt(GPS_3_3);
                    mt6326_V3GTX_set_3_3();
                break;
                default :
					//GPS_Set_Volt(GPS_3_3);
                    mt6326_V3GTX_set_3_3();                
                break;
            }
			pmic_v3gtx_on_sel(V3GTX_ENABLE_WITH_V3GTX_EN);
            mt6326_enable_V3GTX();
        break;
        // V3GRX
        case MT6516_POWER_V3GRX:
			//V3GRX_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2800:
					//V3GRX_Set_Volt(V3GRX_2_8);
                    mt6326_V3GRX_set_2_8();
                break;
                case VOL_2500:
					//V3GRX_Set_Volt(V3GRX_2_5);
                    mt6326_V3GRX_set_2_5(); 
                break;
                case VOL_3000:
					//V3GRX_Set_Volt(V3GRX_3_0);
                    mt6326_V3GRX_set_3_0(); 
                break;
                case VOL_3300:
					//V3GRX_Set_Volt(V3GRX_3_3);
                    mt6326_V3GRX_set_3_3();
                break;
                default :
					//V3GRX_Set_Volt(V3GRX_3_3);
                    mt6326_V3GRX_set_3_3();                
                break;
            }
			pmic_v3grx_on_sel(V3GRX_ENABLE_WITH_V3GRX_EN);
            mt6326_enable_V3GRX();
        break;
        // VCAM_A
        case MT6516_POWER_VCAM_A:
            //CAM1_A_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2500:
                    mt6326_VCAM_A_set_2_5();
                    //CAM1_A_Set_Volt(CAM1_A_2_5);
                break;
                case VOL_2800:
                    mt6326_VCAM_A_set_2_8(); 
                    //CAM1_A_Set_Volt(CAM1_A_2_8); 
                break;
                case VOL_1800:
                    mt6326_VCAM_A_set_1_8(); 
                    //CAM1_A_Set_Volt(CAM1_A_1_8); 
                break;
                case VOL_1500:
                    mt6326_VCAM_A_set_1_5();
                    //CAM1_A_Set_Volt(CAM1_A_1_5);
                break;
                default :
                    mt6326_VCAM_A_set_2_8();                
                    //CAM1_A_Set_Volt(CAM1_A_1_5);                
                break;
            }                                    
            mt6326_enable_VCAM_A();
        break;
        // VWIFI3V3
        case MT6516_POWER_VWIFI3V3:
            //WIFI_V1_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2500:
                    mt6326_VWIFI3V3_set_2_5();
                    //WIFI_V1_Set_Volt(WIFI_V1_2_5);
                break;
                case VOL_2800:
                    mt6326_VWIFI3V3_set_2_8(); 
                    //WIFI_V1_Set_Volt(WIFI_V1_2_8); 
                break;
                case VOL_3000:
                    mt6326_VWIFI3V3_set_3_0(); 
                    //WIFI_V1_Set_Volt(WIFI_V1_3_0); 
                break;
                case VOL_3300:
                    mt6326_VWIFI3V3_set_3_3();
                    //WIFI_V1_Set_Volt(WIFI_V1_3_3);
                break;
                default :
                    mt6326_VWIFI3V3_set_2_5();                
                    //WIFI_V1_Set_Volt(WIFI_V1_2_5);                
                break;
            }                                                
            mt6326_enable_VWIFI3V3();
        break;
        // VWIFI2V8
        case MT6516_POWER_VWIFI2V8:
            //WIFI_V2_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2500:
                    mt6326_VWIFI2V8_set_2_5();
                    //WIFI_V2_Set_Volt(WIFI_V2_2_5);
                break;
                case VOL_2800:
                    mt6326_VWIFI2V8_set_2_8(); 
                    //WIFI_V2_Set_Volt(WIFI_V2_2_8);
                break;
                case VOL_3000:
                    mt6326_VWIFI2V8_set_3_0(); 
                    //WIFI_V2_Set_Volt(WIFI_V2_3_0);
                break;
                case VOL_3300:
                    mt6326_VWIFI2V8_set_3_3();
                    //WIFI_V2_Set_Volt(WIFI_V2_3_3);
                break;
                default :
                    mt6326_VWIFI2V8_set_3_3();                
                    //WIFI_V2_Set_Volt(WIFI_V2_2_5);
                break;
            }                                                            
            mt6326_enable_VWIFI2V8();
        break;
        // VSIM
        case MT6516_POWER_VSIM:
            //SIM_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
                    mt6326_VSIM_set_1_3();
                    //SIM_Set_Volt(SIM_1_3);
                break;
                case VOL_1500:
                    mt6326_VSIM_set_1_5(); 
                    //SIM_Set_Volt(SIM_1_5);
                break;
                case VOL_1800:
                    mt6326_VSIM_set_1_8(); 
                    //SIM_Set_Volt(SIM_1_8);
                break;
                case VOL_2500:
                    mt6326_VSIM_set_2_5();
                    //SIM_Set_Volt(SIM_2_5);
                break;
                case VOL_2800:
                    mt6326_VSIM_set_2_8(); 
                    //SIM_Set_Volt(SIM_2_8);
                break;
                case VOL_3000:
                    mt6326_VSIM_set_3_0(); 
                    //SIM_Set_Volt(SIM_3_0);
                break;
                case VOL_3300:
                    mt6326_VSIM_set_3_3();
                    //SIM_Set_Volt(SIM_3_3);
                break;                
                default :
                    mt6326_VSIM_set_3_3();                
                    //SIM_Set_Volt(SIM_1_3);
                break;
            }                                                                        
            mt6326_enable_VSIM();
        break;
        // VUSB
        case MT6516_POWER_VUSB:
            mt6326_enable_VUSB();
            //USB_Enable(TRUE);
        break;
        // VBT 
        case MT6516_POWER_VBT:
            //BT_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
                    mt6326_VBT_set_1_3();
                    //BT_Set_Volt(BT_1_3);
                break;
                case VOL_1500:
                    mt6326_VBT_set_1_5(); 
                    //BT_Set_Volt(BT_1_5);
                break;
                case VOL_1800:
                    mt6326_VBT_set_1_8(); 
                    //BT_Set_Volt(BT_1_8);
                break;
                case VOL_2500:
                    mt6326_VBT_set_2_5();
                    //BT_Set_Volt(BT_2_5);
                break;
                case VOL_2800:
                    mt6326_VBT_set_2_8(); 
                    //BT_Set_Volt(BT_2_8);
                break;
                case VOL_3000:
                    mt6326_VBT_set_3_0(); 
                    //BT_Set_Volt(BT_3_0);
                break;
                case VOL_3300:
                    mt6326_VBT_set_3_3();
                    //BT_Set_Volt(BT_3_3);
                break;                
                default :
                    mt6326_VBT_set_3_3();                
                    //BT_Set_Volt(BT_1_3);
                break;
            }                                                                                    
            mt6326_enable_VBT();
        break;
        // VCAM_D
        case MT6516_POWER_VCAM_D:
            //CAM1_D_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
                    mt6326_VCAMD_set_1_3();
                    //CAM1_D_Set_Volt(CAM1_D_1_3);
                break;
                case VOL_1500:
                    mt6326_VCAMD_set_1_5(); 
                    //CAM1_D_Set_Volt(CAM1_D_1_5);
                break;
                case VOL_1800:
                    mt6326_VCAMD_set_1_8(); 
                    //CAM1_D_Set_Volt(CAM1_D_1_8);
                break;
                case VOL_2500:
                    mt6326_VCAMD_set_2_5();
                    //CAM1_D_Set_Volt(CAM1_D_2_5);
                break;
                case VOL_2800:
                    mt6326_VCAMD_set_2_8(); 
                    //CAM1_D_Set_Volt(CAM1_D_2_8);
                break;
                case VOL_3000:
                    mt6326_VCAMD_set_3_0(); 
                    //CAM1_D_Set_Volt(CAM1_D_3_0);
                break;
                case VOL_3300:
                    mt6326_VCAMD_set_3_3();
                    //CAM1_D_Set_Volt(CAM1_D_3_3);
                break;                
                default :
                    mt6326_VCAMD_set_3_3();                
                    //CAM1_D_Set_Volt(CAM1_D_1_3);
                break;
            }                                                                                                
            mt6326_enable_VCAM_D();
        break;
        // VGP, used for SIM2 power in Phone
        case MT6516_POWER_VGP:
		case MT6516_POWER_VSIM2:			
            //SIM2_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
					//SIM2_Set_Volt(SIM2_1_3);
                    mt6326_VGP_set_1_3();
                break;
                case VOL_1500:
					//SIM2_Set_Volt(SIM2_1_5);
                    mt6326_VGP_set_1_5(); 
                break;
                case VOL_1800:
					//SIM2_Set_Volt(SIM2_1_8);
                    mt6326_VGP_set_1_8(); 
                break;
                case VOL_2500:
					//SIM2_Set_Volt(SIM2_2_5);
                    mt6326_VGP_set_2_5();
                break;
                case VOL_2800:
					//SIM2_Set_Volt(SIM2_2_8);
                    mt6326_VGP_set_2_8(); 
                break;
                case VOL_3000:
					//SIM2_Set_Volt(SIM2_3_0);
                    mt6326_VGP_set_3_0(); 
                break;
                case VOL_3300:
					//SIM2_Set_Volt(SIM2_3_3);
                    mt6326_VGP_set_3_3();
                break;                
                default :
					//SIM2_Set_Volt(SIM2_3_3);
                    mt6326_VGP_set_3_3();                
                break;
            }
            mt6326_enable_VGP();
        break;
        // VGP2
        case MT6516_POWER_VGP2:
            //GP2_Enable(TRUE);
            switch (powervol)
            {
                case VOL_1300:
					//GP2_Set_Volt(GP2_1_3);
                    mt6326_VGP2_set_1_3();
                break;
                case VOL_1500:
					//GP2_Set_Volt(GP2_1_5);
                    mt6326_VGP2_set_1_5(); 
                break;
                case VOL_1800:
					//GP2_Set_Volt(GP2_1_8);
                    mt6326_VGP2_set_1_8(); 
                break;
                case VOL_2500:
					//GP2_Set_Volt(GP2_2_5);
                    mt6326_VGP2_set_2_5();
                break;
                case VOL_2800:
					//GP2_Set_Volt(GP2_2_8);
                    mt6326_VGP2_set_2_8(); 
                break;
                case VOL_3000:
					//GP2_Set_Volt(GP2_3_0);
                    mt6326_VGP2_set_3_0(); 
                break;
                case VOL_3300:
					//GP2_Set_Volt(GP2_3_3);
                    mt6326_VGP2_set_3_3();
                break;                
                default :
					//GP2_Set_Volt(GP2_3_3);
                    mt6326_VGP2_set_3_3();                
                break;
            }
			pmic_vgp2_on_sel(VGP2_ENABLE_WITH_VGP2_EN);
            mt6326_enable_VGP2();
        break;
        // VSDIO
        case MT6516_POWER_VSDIO:
            //MC_Enable(TRUE);
            switch (powervol)
            {
                case VOL_2800:
                    mt6326_VSDIO_set_2_8(); 
                    //MC_Set_Volt(MC_2_8); 
                break;
                case VOL_3000:
                    mt6326_VSDIO_set_3_0();
                    //MC_Set_Volt(MC_3_0); 
                break;                
                default :
                    mt6326_VSDIO_set_3_0();                
                    //MC_Set_Volt(MC_2_8); 
                break;
            }                                                                                                                                    
            mt6326_enable_VSDIO();
        break;

        // VCORE2, for WIMAX
        case MT6516_POWER_VCORE2:
            //VCORE2_Enable(TRUE);
			#if 1
            switch (powervol)
            {
                case VOL_0800:
                    mt6326_VCORE_2_set_0_80(); 
                break;
                case VOL_0850:
                    mt6326_VCORE_2_set_0_85();
                break;                
                case VOL_0900:
                    mt6326_VCORE_2_set_0_90();
                break;                
                case VOL_0950:
                    mt6326_VCORE_2_set_0_95();
                break;                
                case VOL_1000:
                    mt6326_VCORE_2_set_1_0();
                break;                
                case VOL_1050:
                    mt6326_VCORE_2_set_1_05();
                break;                
                case VOL_1100:
                    mt6326_VCORE_2_set_1_10();
                break;                
                case VOL_1150:
                    mt6326_VCORE_2_set_1_15();
                break;                
                case VOL_1200:
                    mt6326_VCORE_2_set_1_2();
                break;                
                case VOL_1250:
                    mt6326_VCORE_2_set_1_25();
                break;                
                case VOL_1300:
                    mt6326_VCORE_2_set_1_3();
                break;                
                case VOL_1350:
                    mt6326_VCORE_2_set_1_35();
                break;                
                case VOL_1400:
                    mt6326_VCORE_2_set_1_4();
                break;                
                case VOL_1450:
                    mt6326_VCORE_2_set_1_45();
                break;                
                case VOL_1500:
                    mt6326_VCORE_2_set_1_5();
                break;                
                case VOL_1550:
                    mt6326_VCORE_2_set_1_55();
                break;                

                default :
                    mt6326_VCORE_2_set_1_3();                
                break;
            }
			#endif
            mt6326_enable_VCORE_2();
        break;

        
        default:
            MSG(PMIC,"[PM LOG]PowerOn the power setting can't be recongnize\r\n");
            break;
    }

    return TRUE;    
}



BOOL hwPowerDown(MT6516_POWER powerId, char *mode_name)
{

	UINT32 i;
	BOOL bFind = FALSE;

	if (!bPMIC_Init_finish)
		MSG(PMIC,"[PM LOG][%s] mode:%s PMIC init is not finished!! \r\n", __FUNCTION__, mode_name );
	
    if(powerId >= MT6516_POWER_COUNT_END)
    {
		MSG(PMIC,"[PM LOG]%s:%s:%d powerId:%d is wrong\r\n",__FILE__,__FUNCTION__, 
		    __LINE__ , powerId);
        return FALSE;
    }

	
	if(g_MT6516_BusHW.Power[powerId].dwPowerCount == 0)
	{
		MSG(PMIC,"[PM LOG]%s:%s:%d powerId:%d (g_MT6516_BusHW.dwPowerCount[powerId] = 0)\r\n", 
		    __FILE__,__FUNCTION__,__LINE__ ,powerId);
		return FALSE;
	}

	for (i = 0; i< MAX_DEVICE; i++)
	{
		if (!strcmp(g_MT6516_BusHW.Power[powerId].mod_name[i], mode_name))
		{
			MSG(PMIC,"[PM LOG][%s] powerId:%d index:%d mod_name: %s\r\n", 
				__FUNCTION__,powerId, i, mode_name);			
			sprintf(g_MT6516_BusHW.Power[powerId].mod_name[i] , "%s", NON_OP);
			bFind = TRUE;
			break ;
		}
	}
	
	if(!bFind)
	{
		//MSG(PMIC,"[%s] Cannot find [%d] master is [%s]\r\n",__FUNCTION__,powerId, mode_name);		
		spin_unlock(&MT6516_csClock_lock);		
		return TRUE;
	}		

	g_MT6516_BusHW.Power[powerId].dwPowerCount--;
	if(g_MT6516_BusHW.Power[powerId].dwPowerCount > 0)
	{
		return TRUE;
	}

    switch (powerId)
    {
        case MT6516_POWER_VRF:
            //mt6326_disable_VRF();
        break;
        case MT6516_POWER_VTCXO:
            //mt6326_disable_VTCXO();
        break;
        case MT6516_POWER_V3GTX:
			pmic_v3gtx_on_sel(V3GTX_ENABLE_WITH_V3GTX_EN);
            mt6326_disable_V3GTX();            
			//GPS_Enable(FALSE);
        break;
        case MT6516_POWER_V3GRX:
			//V3GRX_Enable(FALSE);
			pmic_v3grx_on_sel(V3GRX_ENABLE_WITH_V3GRX_EN);
            mt6326_disable_V3GRX();
        break;
        case MT6516_POWER_VCAM_A:
            mt6326_disable_VCAM_A();
            //CAM1_A_Enable(FALSE);
        break;
        case MT6516_POWER_VWIFI3V3:
            mt6326_disable_VWIFI3V3();
            //WIFI_V1_Enable(FALSE);
        break;
        case MT6516_POWER_VWIFI2V8:
            mt6326_disable_VWIFI2V8();
            //WIFI_V2_Enable(FALSE);
        break;
        case MT6516_POWER_VSIM:
            mt6326_disable_VSIM();
            //SIM_Enable(FALSE);
        break;
        case MT6516_POWER_VUSB:
            mt6326_disable_VUSB();
            //USB_Enable(FALSE);
        break;
        case MT6516_POWER_VBT:
            mt6326_disable_VBT();
            //BT_Enable(FALSE);
        break;
        case MT6516_POWER_VCAM_D:
            mt6326_disable_VCAM_D();
            //CAM1_D_Enable(FALSE);
        break;
        case MT6516_POWER_VSIM2:		
        case MT6516_POWER_VGP:
            mt6326_disable_VGP();
            //SIM2_Enable(FALSE);
        break;
        case MT6516_POWER_VGP2:
			//GP2_Enable(FALSE);
			pmic_vgp2_on_sel(VGP2_ENABLE_WITH_VGP2_EN);
            mt6326_disable_VGP2();
        break;
        case MT6516_POWER_VSDIO:
            mt6326_disable_VSDIO();
            //MC_Enable(FALSE);
        break;
        case MT6516_POWER_VCORE2:
			//VCORE2_Enable(FALSE);
            mt6326_disable_VCORE_2();
        break;

        default:
            MSG(PMIC,"[PM LOG]PowerOFF won't be modified(powerId = %d)\r\n", powerId);
        break;
    }

    return TRUE;    
}
#endif

EXPORT_SYMBOL(hwEnableSubsys);
EXPORT_SYMBOL(hwDisableSubsys);
EXPORT_SYMBOL(hwEnableClock);
EXPORT_SYMBOL(hwDisableClock);
EXPORT_SYMBOL(hwPowerDown);
EXPORT_SYMBOL(hwPowerOn);

