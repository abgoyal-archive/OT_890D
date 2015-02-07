
#include <asm/io.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>
#include <asm/div64.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#include <mach/dma.h>
#include <mach/irqs.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516.h>
#include <mach/mt6516_gpt_sw.h>
#include "mt6516_monitor.h"


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define __raw_readll(a)	(__chk_io_ptr(a), *(volatile u64 __force *)(a)) &0xFFFFFFFF

//#define MT6516_MONITOR_DEBUG
#ifdef MT6516_MONITOR_DEBUG
#define MONITOR_DEBUG printk
#else
#define MONITOR_DEBUG(x,...)
#endif

static spinlock_t MonitorControlLock = SPIN_LOCK_UNLOCKED;

void Monitor_EnableMonControl (MT6516_MON_MODE mode)
{
      spin_lock(&MonitorControlLock);
      DRV_SetReg32(MON_SET, (1 << mode));
      spin_unlock(&MonitorControlLock);
}
EXPORT_SYMBOL(Monitor_EnableMonControl);

void Monitor_DisableMonControl (MT6516_MON_MODE mode)
{
      spin_lock(&MonitorControlLock);
      DRV_SetReg32(MON_CLR, (1 << mode));
      spin_unlock(&MonitorControlLock);
}
EXPORT_SYMBOL(Monitor_DisableMonControl);

BOOL Monitor_GetMonControlStatus (MT6516_MON_MODE mode)
{    	
       spin_lock(&MonitorControlLock);
       if(DRV_Reg32(MON_CON) & (1 << mode))
	{     spin_unlock(&MonitorControlLock);
	       return TRUE;    // this module is POWER DOWN
       }
	else
	{     spin_unlock(&MonitorControlLock);
		return FALSE;   // this module is POWER UP 
	}
}
EXPORT_SYMBOL(Monitor_GetMonControlStatus);

void Monitor_SetDAHB (MT6516_DAHB_SEL sel)
{
      spin_lock(&MonitorControlLock);
      DRV_SetReg32(MON_CLR, (0x11 << 22)); //clear
      DRV_SetReg32(MON_SET, (sel << 22)); //set
      spin_unlock(&MonitorControlLock);
}
EXPORT_SYMBOL(Monitor_SetDAHB);


void Monitor_DcacheMissBegin(void)
{
	u64 u8_RReq = 0, u8_WReq = 0, u8_RMiss = 0, u8_WMiss = 0;

	Monitor_DisableMonControl(DCM_EN);
	Monitor_DisableMonControl(DCM_CLR);

	// (1) u8_RReq low bit (31~0)
	u8_RReq = ((u64)__raw_readl(MON_PERF1));
	// (1) u8_RReq high bit (39~32)
	u8_RReq |= ((u64)__raw_readl(MON_PERF2)&0xFF)<<32;

	// (2) u8_WReq low bit (23~0)
	u8_WReq = ((u64)__raw_readl(MON_PERF2)>>8);
	// (2) u8_WReq high bit (39~24)
	u8_WReq |= ((u64)__raw_readl(MON_PERF3)&0xFFFF)<<24;

	// (3) u8_RMiss low bit (15~0)
	u8_RMiss = ((u64)__raw_readl(MON_PERF3)&0xFFFF0000)>>16;
	// (3) u8_RMiss high bit (39~16)
	u8_RMiss |= ((u64)__raw_readl(MON_PERF4)&0x00FFFFFF)<<16;
	
	// (4) u8_WMiss low bit (7~0)
	u8_WMiss = ((u64)__raw_readl(MON_PERF4)>>24);
	// (4) u8_WMiss high bit (39~8)
	u8_WMiss |= ((u64)__raw_readl(MON_PERF5)<<8);

	MONITOR_DEBUG("\n[Monitor Data Cache Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("Dcache: total read requests = %llu \n",u8_RReq);
	MONITOR_DEBUG("Dcache: total read miss = %llu \n",u8_RMiss);	
	MONITOR_DEBUG("Dcache: total write requests = %llu \n",u8_WReq);		
	MONITOR_DEBUG("Dcache: total write miss = %llu \n",u8_WMiss);		

	
	if(u8_RReq!=0 || u8_RMiss!=0 || u8_WReq!=0 || u8_WMiss!=0)
	{	printk("Monitor_DcacheMissBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(DCM_CLR);
	Monitor_EnableMonControl(DCM_EN);

}
EXPORT_SYMBOL(Monitor_DcacheMissBegin);

void Monitor_DcacheMissEnd(void)
{

	u64 u8_RReq = 0, u8_WReq = 0, u8_RMiss = 0, u8_WMiss = 0;

	u32 u4_RM1 = 0, u4_RM2 = 0;
	u32 u4_WM1 = 0, u4_WM2 = 0;

	Monitor_DisableMonControl(DCM_EN);

	// (1) u8_RReq low bit (31~0)
	u8_RReq = ((u64)__raw_readl(MON_PERF1));
	// (1) u8_RReq high bit (39~32)
	u8_RReq |= ((u64)__raw_readl(MON_PERF2)&0xFF)<<32;

	// (2) u8_WReq low bit (23~0)
	u8_WReq = ((u64)__raw_readl(MON_PERF2)>>8);
	// (2) u8_WReq high bit (39~24)
	u8_WReq |= ((u64)__raw_readl(MON_PERF3)&0xFFFF)<<24;

	// (3) u8_RMiss low bit (15~0)
	u8_RMiss = ((u64)__raw_readl(MON_PERF3)&0xFFFF0000)>>16;
	// (3) u8_RMiss high bit (39~16)
	u8_RMiss |= ((u64)__raw_readl(MON_PERF4)&0x00FFFFFF)<<16;

	// (4) u8_WMiss low bit (7~0)
	u8_WMiss = ((u64)__raw_readl(MON_PERF4)>>24);
	// (4) u8_WMiss high bit (39~8)
	u8_WMiss |= ((u64)__raw_readl(MON_PERF5)<<8);

	printk("\n[Monitor Data Cache End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PREF1 = %x(hex)\n",__raw_readl(MON_PERF1));
	MONITOR_DEBUG("MON_PREF2 = %x(hex)\n",__raw_readl(MON_PERF2));
	MONITOR_DEBUG("MON_PREF3 = %x(hex)\n",__raw_readl(MON_PERF3));
	MONITOR_DEBUG("MON_PREF4 = %x(hex)\n",__raw_readl(MON_PERF4));
	MONITOR_DEBUG("MON_PREF5 = %x(hex)\n",__raw_readl(MON_PERF5));	
	MONITOR_DEBUG("Dcache: total read requests = %llx(hex)\n",u8_RReq);
	MONITOR_DEBUG("Dcache: total read miss = %llx(hex)\n",u8_RMiss);
	MONITOR_DEBUG("Dcache: total write requests = %llx(hex)\n",u8_WReq);		
	MONITOR_DEBUG("Dcache: total write miss = %llx(hex)\n",u8_WMiss);				

	printk("Dcache: total read requests = %llu\n",u8_RReq);
	printk("Dcache: total read miss = %llu\n",u8_RMiss);

	if(u8_RReq!=0)
	{
		u8_RMiss *= 100;
		u4_RM2 = do_div(u8_RMiss,u8_RReq);
		u4_RM1 = u8_RMiss;

		printk("Dcache: total read miss rate = %u.%4u\n",u4_RM1,u4_RM2);
	}
	else
	{	printk("Dcache: total read miss rate = 0.0\n");
	}
	
	printk("Dcache: total write requests = %llu\n",u8_WReq);		
	printk("Dcache: total write miss = %llu\n",u8_WMiss);					

	if(u8_WReq!=0)
	{
		u8_WMiss *= 100;
		u4_WM2 = do_div(u8_WMiss,u8_RReq);
		u4_WM1 = u8_WMiss;

		printk("Dcache: total write miss rate = %u.%4u\n",u4_WM1,u4_WM2);	
	}
	else
	{	printk("Dcache: total write miss rate = 0.0\n");
	}

	Monitor_DisableMonControl(DCM_CLR);
}
EXPORT_SYMBOL(Monitor_DcacheMissEnd);


void Monitor_IcacheMissBegin(void)
{
	u64 u8_RReq = 0, u8_RMiss = 0;

	Monitor_DisableMonControl(ICM_EN);
	Monitor_DisableMonControl(ICM_CLR);

	// (1) u8_RReq low bit (31~0)
	u8_RReq = ((u64)__raw_readl(MON_PERF6));
	// (1) u8_RReq high bit (39~32)
	u8_RReq |= ((u64)__raw_readl(MON_PERF7)&0xFF)<<32;

	// (2) u8_RMiss low bit (23~0)
	u8_RMiss = ((u64)__raw_readl(MON_PERF7)>>8);
	// (2) u8_RMiss high bit (39~24)
	u8_RMiss |= (((u64)__raw_readl(MON_PERF8)&0xFFFF)<<24);
	
	MONITOR_DEBUG("\n[Monitor Instruction Cache Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("Instruction: total read requests = %llu \n",u8_RReq);
	MONITOR_DEBUG("Instruction: total read miss = %llu \n",u8_RMiss);			
	
	if(u8_RReq!=0 || u8_RMiss!=0)
	{	printk("Monitor_IcacheMissBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(ICM_CLR);
	Monitor_EnableMonControl(ICM_EN);

}
EXPORT_SYMBOL(Monitor_IcacheMissBegin);

void Monitor_IcacheMissEnd(void)
{
	u64 u8_RReq = 0, u8_RMiss = 0;

	u32 u4_RM1 = 0, u4_RM2 = 0;

	Monitor_DisableMonControl(ICM_EN);

	// (1) u8_RReq low bit (31~0)
	u8_RReq = ((u64)__raw_readl(MON_PERF6));
	// (1) u8_RReq high bit (39~32)
	u8_RReq |= ((u64)__raw_readl(MON_PERF7)&0xFF)<<32;

	// (2) u8_RMiss low bit (23~0)
	u8_RMiss = ((u64)__raw_readl(MON_PERF7)>>8);
	// (2) u8_RMiss high bit (39~24)
	u8_RMiss |= (((u64)__raw_readl(MON_PERF8)&0xFFFF)<<24);

	printk("\n[Monitor Instruction Cache End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PREF6 = %x(hex)\n",__raw_readl(MON_PERF6));
	MONITOR_DEBUG("MON_PREF7 = %x(hex)\n",__raw_readl(MON_PERF7));
	MONITOR_DEBUG("MON_PREF8 = %x(hex)\n",__raw_readl(MON_PERF8));		
	MONITOR_DEBUG("Instruction: total read requests = %llx(hex)\n",u8_RReq);
	MONITOR_DEBUG("Instruction: total read miss = %llx(hex)\n",u8_RMiss);			

	printk("Instruction: total read requests = %llu\n",u8_RReq);
	printk("Instruction: total read miss = %llu\n",u8_RMiss);		

	if(u8_RReq!=0)
	{
		u8_RMiss *= 100;
		u4_RM2 = do_div(u8_RMiss,u8_RReq);
		u4_RM1 = u8_RMiss;
		
		printk("Instruction: total read miss rate = %u.%u\n",u4_RM1,u4_RM2);
	}
	else
	{	printk("Instruction: total read miss rate = 0.0\n");
	}

	//Monitor_DisableMonControl(ICM_CLR);
}
EXPORT_SYMBOL(Monitor_IcacheMissEnd);


void Monitor_ARMActiveBegin(void)
{
	u64 u8_Active = 0;

	Monitor_DisableMonControl(ACTIVE_EN);
	Monitor_DisableMonControl(ACTIVE_CLR);

	// (1) u8_Active low bit (15~0)
	u8_Active = (((u64)__raw_readl(MON_PERF8)&0xFFFF0000)>>16);
	// (1) u8_Active high bit (39~16)
	u8_Active |= (((u64)__raw_readl(MON_PERF9)&0x00FFFFFF)<<16);

	MONITOR_DEBUG("\n[Monitor ARM9 Active Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("ARM9: total Active Count = %llu \n",u8_Active);	

	if(u8_Active!=0)
	{	printk("Monitor_ARMActiveBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(ACTIVE_CLR);
	Monitor_EnableMonControl(ACTIVE_EN);

}
EXPORT_SYMBOL(Monitor_ARMActiveBegin);

void Monitor_ARMActiveEnd(void)
{
	u64 u8_Active = 0;

	Monitor_DisableMonControl(ACTIVE_EN);

	// (1) u8_Active low bit (15~0)
	u8_Active = (((u64)__raw_readl(MON_PERF8)&0xFFFF0000)>>16);
	// (1) u8_Active high bit (39~16)
	u8_Active |= (((u64)__raw_readl(MON_PERF9)&0x00FFFFFF)<<16);
	
	printk("\n[Monitor ARM9 Active End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PERF8 = %x(hex)\n",__raw_readl(MON_PERF8));
	MONITOR_DEBUG("MON_PERF9 = %x(hex)\n",__raw_readl(MON_PERF9));		
	MONITOR_DEBUG("ARM9: total Active Count = %llx(hex)\n",u8_Active);			

	printk("ARM9: total Active Count = %llu \n",u8_Active);	

	//Monitor_DisableMonControl(ACTIVE_CLR);
}
EXPORT_SYMBOL(Monitor_ARMActiveEnd);


void Monitor_DTLBPenaltyBegin(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(DTLB_EN);
	Monitor_DisableMonControl(DTLB_CLR);

	// (1) u8_WMiss low bit (7~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF9)>>24);
	// (1) u8_WMiss high bit (39~8)
	u8_Penalty |= ((u64)__raw_readl(MON_PERF10)<<8);

	MONITOR_DEBUG("\n[Monitor DTLB Penalty Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("DTLB: Penalty Count = %llu \n",u8_Penalty);	

	if(u8_Penalty!=0)
	{	printk("Monitor_DTLBPenaltyBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(DTLB_CLR);
	Monitor_EnableMonControl(DTLB_EN);

}
EXPORT_SYMBOL(Monitor_DTLBPenaltyBegin);

void Monitor_DTLBPenaltyEnd(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(DTLB_EN);

	// (1) u8_WMiss low bit (7~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF9)>>24);
	// (1) u8_WMiss high bit (39~8)
	u8_Penalty |= ((u64)__raw_readl(MON_PERF10)<<8);
	
	printk("\n[Monitor DTLB Penalty End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PERF9 = %x(hex)\n",__raw_readl(MON_PERF9));
	MONITOR_DEBUG("MON_PERF10 = %x(hex)\n",__raw_readl(MON_PERF10));		
	MONITOR_DEBUG("DTLB: Penalty Count = %llx(hex)\n",u8_Penalty);			

	printk("DTLB: Penalty Count = %llu \n",u8_Penalty);

	//Monitor_DisableMonControl(DTLB_CLR);
}
EXPORT_SYMBOL(Monitor_DTLBPenaltyEnd);


void Monitor_ITLBPenaltyBegin(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(ITLB_EN);
	Monitor_DisableMonControl(ITLB_CLR);

	// (1) u8_RReq low bit (31~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF11));
	// (1) u8_RReq high bit (39~32)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF12)&0xFF)<<32);	

	MONITOR_DEBUG("\n[Monitor ITLB Penalty Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("ITLB: Penalty Count = %llu \n",u8_Penalty);	

	if(u8_Penalty!=0)
	{	printk("Monitor_ITLBPenaltyBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(ITLB_CLR);
	Monitor_EnableMonControl(ITLB_EN);

}
EXPORT_SYMBOL(Monitor_ITLBPenaltyBegin);

void Monitor_ITLBPenaltyEnd(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(ITLB_EN);

	// (1) u8_RReq low bit (31~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF11));
	// (1) u8_RReq high bit (39~32)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF12)&0xFF)<<32);	
	
	printk("\n[Monitor ITLB Penalty End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PERF11 = %x(hex)\n",__raw_readl(MON_PERF11));
	MONITOR_DEBUG("MON_PERF12 = %x(hex)\n",__raw_readl(MON_PERF12));		
	MONITOR_DEBUG("ITLB: Penalty Count = %llx(hex)\n",u8_Penalty);			

	printk("ITLB: Penalty Count = %llu \n",u8_Penalty);

	//Monitor_DisableMonControl(ITLB_CLR);
}
EXPORT_SYMBOL(Monitor_ITLBPenaltyEnd);


void Monitor_DcachePenaltyBegin(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(DCP_EN);
	Monitor_DisableMonControl(DCP_CLR);

	// (1) u8_WReq low bit (23~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF12)>>8);
	// (1) u8_WReq high bit (39~24)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF13)&0xFFFF)<<24);

	MONITOR_DEBUG("\n[Monitor Dcache Penalty Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("Dcache: Penalty Count = %llu \n",u8_Penalty);	

	if(u8_Penalty!=0)
	{	printk("Monitor_DcachePenaltyBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(DCP_CLR);
	Monitor_EnableMonControl(DCP_EN);

}
EXPORT_SYMBOL(Monitor_DcachePenaltyBegin);

void Monitor_DcachePenaltyEnd(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(DCP_EN);

	// (1) u8_WReq low bit (23~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF12)>>8);
	// (1) u8_WReq high bit (39~24)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF13)&0xFFFF)<<24);
	
	printk("\n[Monitor Dcache Penalty End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PERF12 = %x(hex)\n",__raw_readl(MON_PERF12));
	MONITOR_DEBUG("MON_PERF13 = %x(hex)\n",__raw_readl(MON_PERF13));		
	MONITOR_DEBUG("Dcache: Penalty Count = %llx(hex)\n",u8_Penalty);			

	printk("Dcache: Penalty Count = %llu \n",u8_Penalty);

	Monitor_DisableMonControl(DCP_CLR);
}
EXPORT_SYMBOL(Monitor_DcachePenaltyEnd);


void Monitor_IcachePenaltyBegin(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(ICP_EN);
	Monitor_DisableMonControl(ICP_CLR);

	// (1) u8_Penalty low bit (15~0)
	u8_Penalty = (((u64)__raw_readl(MON_PERF13)&0xFFFF0000)>>16);
	// (1) u8_Penalty high bit (39~16)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF14)&0x00FFFFFF)<<16);

	MONITOR_DEBUG("\n[Monitor Icache Penalty Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("Icache: Penalty Count = %llu \n",u8_Penalty);	

	if(u8_Penalty!=0)
	{	printk("Monitor_IcachePenaltyBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(ICP_CLR);
	Monitor_EnableMonControl(ICP_EN);

}
EXPORT_SYMBOL(Monitor_IcachePenaltyBegin);

void Monitor_IcachePenaltyEnd(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(ICP_EN);

	// (1) u8_Penalty low bit (15~0)
	u8_Penalty = (((u64)__raw_readl(MON_PERF13)&0xFFFF0000)>>16);
	// (1) u8_Penalty high bit (39~16)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF14)&0x00FFFFFF)<<16);
	
	printk("\n[Monitor Icache Penalty End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PERF13 = %x(hex)\n",__raw_readl(MON_PERF13));
	MONITOR_DEBUG("MON_PERF14 = %x(hex)\n",__raw_readl(MON_PERF14));		
	MONITOR_DEBUG("Icache: Penalty Count = %llx(hex)\n",u8_Penalty);			

	printk("Icache: Penalty Count = %llu \n",u8_Penalty);

	//Monitor_DisableMonControl(ICP_CLR);
}
EXPORT_SYMBOL(Monitor_IcachePenaltyEnd);


void Monitor_DEXTPenaltyBegin(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(DEXT_EN);
	Monitor_DisableMonControl(DEXT_CLR);

	// (1) u8_Penalty low bit (7~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF14)>>24);
	// (1) u8_Penalty high bit (39~8)
	u8_Penalty |= ((u64)__raw_readl(MON_PERF15)<<8);
		
	MONITOR_DEBUG("\n[Monitor DEXT Penalty Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("DEXT: Penalty Count = %llu \n",u8_Penalty);

	if(u8_Penalty!=0)
	{	printk("Monitor_DEXTPenaltyBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(DEXT_CLR);
	Monitor_EnableMonControl(DEXT_EN);

}
EXPORT_SYMBOL(Monitor_DEXTPenaltyBegin);

void Monitor_DEXTPenaltyEnd(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(DEXT_EN);

	// (1) u8_Penalty low bit (7~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF14)>>24);
	// (1) u8_Penalty high bit (39~8)
	u8_Penalty |= ((u64)__raw_readl(MON_PERF15)<<8);
	
	printk("\n[Monitor DEXT Penalty End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PERF14 = %x(hex)\n",__raw_readl(MON_PERF14));
	MONITOR_DEBUG("MON_PERF15 = %x(hex)\n",__raw_readl(MON_PERF15));		
	MONITOR_DEBUG("DEXT: Penalty Count = %llx(hex)\n",u8_Penalty);			

	printk("DEXT: Penalty Count = %llu \n",u8_Penalty);

	//Monitor_DisableMonControl(DEXT_CLR);
}
EXPORT_SYMBOL(Monitor_DEXTPenaltyEnd);


void Monitor_IEXTPenaltyBegin(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(IEXT_EN);
	Monitor_DisableMonControl(IEXT_CLR);

	// (1) u8_Penalty low bit (31~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF16));
	// (1) u8_RReq high bit (39~32)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF17)&0xFF)<<32);

	MONITOR_DEBUG("\n[Monitor IEXT Penalty Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("IEXT: Penalty Count = %llu \n",u8_Penalty);	

	if(u8_Penalty!=0)
	{	printk("Monitor_IEXTPenaltyBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(IEXT_CLR);
	Monitor_EnableMonControl(IEXT_EN);

}
EXPORT_SYMBOL(Monitor_IEXTPenaltyBegin);

void Monitor_IEXTPenaltyEnd(void)
{
	u64 u8_Penalty = 0;

	Monitor_DisableMonControl(IEXT_EN);

	// (1) u8_Penalty low bit (31~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF16));
	// (1) u8_RReq high bit (39~32)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF17)&0xFF)<<32);
	
	printk("\n[Monitor IEXT Penalty End]\n");
	printk("---------------------------------\n");	

	MONITOR_DEBUG("MON_PERF16 = %x(hex)\n",__raw_readl(MON_PERF16));
	MONITOR_DEBUG("MON_PERF17 = %x(hex)\n",__raw_readl(MON_PERF17));		
	MONITOR_DEBUG("IEXT: Penalty Count = %llx(hex)\n",u8_Penalty);			

	printk("IEXT: Penalty Count = %llu \n",u8_Penalty);

	//Monitor_DisableMonControl(IEXT_CLR);
}
EXPORT_SYMBOL(Monitor_IEXTPenaltyEnd);


void Monitor_DAHBBegin(MT6516_DAHB_SEL sel)
{
	u64 u8_Penalty = 0;

	u64 u8_Req = 0;	
	
	Monitor_DisableMonControl(DAHB_EN);
	Monitor_DisableMonControl(DAHB_CLR);

	// (1) u8_Penalty low bit (23~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF17)>>8);
	// (1) u8_Penalty high bit (39~24)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF18)&0xFFFF)<<24);

	// (2) u8_Req low bit (15~0)
	u8_Req = (((u64)__raw_readl(MON_PERF18)&0xFFFF0000)>>16);
	// (2) u8_Req high bit (39~16)
	u8_Req |= (((u64)__raw_readl(MON_PERF19)&0x00FFFFFF)<<16);

	MONITOR_DEBUG("\n[Monitor DAHB Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("DAHB: Penalty Count = %llu \n",u8_Penalty);
	MONITOR_DEBUG("DAHB: Total Request Count = %llu \n",u8_Req);			

	if(u8_Penalty!=0 || u8_Req!=0)
	{	printk("Monitor_DAHBBegin: Reset fail!\n");			
	}
	
	Monitor_SetDAHB(sel);
	Monitor_EnableMonControl(DAHB_CLR);
	Monitor_EnableMonControl(DAHB_EN);

}
EXPORT_SYMBOL(Monitor_DAHBBegin);

void Monitor_DAHBEnd(void)
{
	u64 u8_Penalty = 0;

	u64 u8_Req = 0;		

	Monitor_DisableMonControl(DAHB_EN);

	// (1) u8_Penalty low bit (23~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF17)>>8);
	// (1) u8_Penalty high bit (39~24)
	u8_Penalty |= (((u64)__raw_readl(MON_PERF18)&0xFFFF)<<24);

	// (2) u8_Req low bit (15~0)
	u8_Req = (((u64)__raw_readl(MON_PERF18)&0xFFFF0000)>>16);
	// (2) u8_Req high bit (39~16)
	u8_Req |= (((u64)__raw_readl(MON_PERF19)&0x00FFFFFF)<<16);
	
	printk("\n[Monitor DAHB End]\n");
	printk("---------------------------------\n");	
	
	MONITOR_DEBUG("MON_PERF17 = %x(hex)\n",__raw_readl(MON_PERF17));
	MONITOR_DEBUG("MON_PERF18 = %x(hex)\n",__raw_readl(MON_PERF18));	
	MONITOR_DEBUG("MON_PERF19 = %x(hex)\n",__raw_readl(MON_PERF19));			
	MONITOR_DEBUG("DAHB: Penalty Count = %llx(hex)\n",u8_Penalty);			
	MONITOR_DEBUG("DAHB: Total Request Count = %llx(hex)\n",u8_Req);				

	printk("DAHB: Penalty Count = %llu \n",u8_Penalty);
	printk("DAHB: Total Request Count = %llu \n",u8_Req);	

	//Monitor_DisableMonControl(DAHB_CLR);
}
EXPORT_SYMBOL(Monitor_DAHBEnd);


void Monitor_IAHBBegin(void)
{
	u64 u8_Penalty = 0;

	u64 u8_Req = 0;	
	
	Monitor_DisableMonControl(IAHB_EN);
	Monitor_DisableMonControl(IAHB_CLR);

	// (1) u8_Penalty low bit (7~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF19)>>24);
	// (1) u8_Penalty high bit (39~8)
	u8_Penalty |= ((u64)__raw_readl(MON_PERF20)<<8);

	// (2) u8_Req low bit (31~0)
	u8_Req = ((u64)__raw_readl(MON_PERF21));
	// (2) u8_Req high bit (39~32)
	u8_Req |= (((u64)__raw_readl(MON_PERF22)&0xFF)<<32);
	
	MONITOR_DEBUG("\n[Monitor IAHB Begin]\n");
	MONITOR_DEBUG("---------------------------------\n");	
	MONITOR_DEBUG("IAHB: Penalty Count = %llu \n",u8_Penalty);
	MONITOR_DEBUG("IAHB: Total Request Count = %llu \n",u8_Req);	

	if(u8_Penalty!=0 || u8_Req!=0)
	{	printk("Monitor_IAHBBegin: Reset fail!\n");			
	}

	Monitor_EnableMonControl(IAHB_CLR);
	Monitor_EnableMonControl(IAHB_EN);

}
EXPORT_SYMBOL(Monitor_IAHBBegin);

void Monitor_IAHBEnd(void)
{
	u64 u8_Penalty = 0;

	u64 u8_Req = 0;		

	Monitor_DisableMonControl(IAHB_EN);

	// (1) u8_Penalty low bit (7~0)
	u8_Penalty = ((u64)__raw_readl(MON_PERF19)>>24);
	// (1) u8_Penalty high bit (39~8)
	u8_Penalty |= ((u64)__raw_readl(MON_PERF20)<<8);

	// (2) u8_Req low bit (31~0)
	u8_Req = ((u64)__raw_readl(MON_PERF21));
	// (2) u8_Req high bit (39~32)
	u8_Req |= (((u64)__raw_readl(MON_PERF22)&0xFF)<<32);
	
	printk("\n[Monitor IAHB End]\n");
	printk("---------------------------------\n");	
	
	MONITOR_DEBUG("MON_PERF19 = %x(hex)\n",__raw_readl(MON_PERF19));
	MONITOR_DEBUG("MON_PERF20 = %x(hex)\n",__raw_readl(MON_PERF20));	
	MONITOR_DEBUG("MON_PERF21 = %x(hex)\n",__raw_readl(MON_PERF21));		
	MONITOR_DEBUG("MON_PERF22 = %x(hex)\n",__raw_readl(MON_PERF22));			
	MONITOR_DEBUG("IAHB: Penalty Count = %llx(hex)\n",u8_Penalty);			
	MONITOR_DEBUG("IAHB: Total Request Count = %llx(hex)\n",u8_Req);				

	printk("IAHB: Penalty Count = %llu \n",u8_Penalty);
	printk("IAHB: Total Request Count = %llu \n",u8_Req);	

	//Monitor_DisableMonControl(IAHB_CLR);
}
EXPORT_SYMBOL(Monitor_IAHBEnd);



#define  TS_MONITOR_ALL_START                0
#define  TS_MONITOR_ALL_STOP                 1
#define  TS_MONITOR_PERIOD                   3 //(sec)


BOOL g_u8_monitor_begin = TRUE;
void Monitor_Test(void)
{	 	

 	if(g_u8_monitor_begin==FALSE)
	{
		Monitor_DcacheMissBegin();
		Monitor_IcacheMissBegin();
		Monitor_ARMActiveBegin();
		Monitor_DTLBPenaltyBegin();
		Monitor_ITLBPenaltyBegin();
		Monitor_DcachePenaltyBegin();
		Monitor_IcachePenaltyBegin();
		Monitor_DEXTPenaltyBegin();
		Monitor_IEXTPenaltyBegin();
		Monitor_DAHBBegin(EXT_MEM);
		Monitor_IAHBBegin();
		g_u8_monitor_begin=TRUE;
	}
	else
	{ 	g_u8_monitor_begin=FALSE;
		Monitor_DcacheMissEnd();
		Monitor_IcacheMissEnd();
		Monitor_ARMActiveEnd();
		Monitor_DTLBPenaltyEnd();
		Monitor_ITLBPenaltyEnd();
		Monitor_DcachePenaltyEnd();
		Monitor_IcachePenaltyEnd();
		Monitor_DEXTPenaltyEnd();
		Monitor_IEXTPenaltyEnd();
		Monitor_DAHBEnd();
		Monitor_IAHBEnd();
	}	 	 	
	
	printk("\n=>Pass Monitor Period : %d sec!!\n",TS_MONITOR_PERIOD);	 		
}

void Monitor_XGPTConfig(void)
{
    XGPT_CONFIG config;
    XGPT_NUM  gpt_num = XGPT3;    
    XGPT_CLK_DIV clkDiv = XGPT_CLK_DIV_1;
    //unsigned long irq_flag;

    XGPT_Reset(XGPT3);   
    XGPT_Init (gpt_num, Monitor_Test);
    config.num = gpt_num;
    config.mode = XGPT_REPEAT;
    config.clkDiv = clkDiv;
    config.bIrqEnable = TRUE;
    config.u4Compare = 32768*TS_MONITOR_PERIOD;
    
    if (XGPT_Config(config) == FALSE)
        return;                    
        
    XGPT_Start(gpt_num);           

	Monitor_DcacheMissBegin();
	Monitor_IcacheMissBegin();
	Monitor_ARMActiveBegin();
	Monitor_DTLBPenaltyBegin();
	Monitor_ITLBPenaltyBegin();
	Monitor_DcachePenaltyBegin();
	Monitor_IcachePenaltyBegin();
	Monitor_DEXTPenaltyBegin();
	Monitor_IEXTPenaltyBegin();
	Monitor_DAHBBegin(EXT_MEM);
	Monitor_IAHBBegin();
	return ;
}

void Monitor_XGPTStop(void)
{
    XGPT_CONFIG config;
    XGPT_NUM  gpt_num = XGPT3;    
    XGPT_Stop(gpt_num);           
	return ;
}

static int Monitor_Ioctl(struct inode *inode, struct file *file, 
                             unsigned int cmd, unsigned long arg)
{

	printk("MT6516 Monitor IOCTL\n");

	switch (cmd) {
		case TS_MONITOR_ALL_START: 
			    printk("Monitor All START!!!!\n");
			    Monitor_XGPTConfig();
		break;
		case TS_MONITOR_ALL_STOP: 
			    printk("Monitor All STOP!!!!\n");
			    Monitor_XGPTStop();
		break;							
	}								
    
    return 0;
}

static int Monitor_Open(struct inode *inode, struct file *file)
{ 	
    printk("MT6516 Monitor Open\n");
    return 0;
}

static int Monitor_Release(struct inode *inode, struct file *file)
{ 	
    printk("MT6516 Monitor Release\n");
    return 0;
}


static int mt6516_monitor_resume(struct platform_device *pdev)
{
    return 0;
}


static int mt6516_monitor_suspend(struct platform_device *pdev)
{
    return 0;
}


static int mt6516_monitor_remove(struct platform_device *pdev)
{
	return 0;
}


static void mt6516_monitor_shutdown(struct platform_device *pdev)
{
}

static struct file_operations moniotr_fops = 
{
	.owner=		THIS_MODULE,
    .ioctl=		Monitor_Ioctl,
	.open=		Monitor_Open,    
	.release=	Monitor_Release,    
};

static int mt6516_monitor_probe(struct platform_device *pdev)
{

	printk("mt6516 performance monitor probe\n");

    if (register_chrdev(MISC_DYNAMIC_MINOR, "monitor", &moniotr_fops)) {
        printk("Unable to get major %d for monitor driver\n",
               MISC_DYNAMIC_MINOR);
        return -1;
    }

    return 0;
}

static struct platform_driver mt6516_monitor_driver = 
{   .probe		= mt6516_monitor_probe,
	.remove	    = mt6516_monitor_remove,
	.shutdown	= mt6516_monitor_shutdown,
	.suspend	= mt6516_monitor_suspend,
	.resume	    = mt6516_monitor_resume,	
    .driver     = {
    	.name   = "mt6516 monitor",
    	.owner	= THIS_MODULE,
    },
};

static struct platform_device mt6516_monitor_device = {
    .name     = "mt6516 monitor", // platform device's name must sync with platform driver's name
    .id       = 0,
};


static int __init mt_init_monitor(void)
{   			    	
	
	printk("mt6516 performance monitor driver init\n");	

    if (platform_device_register(&mt6516_monitor_device)){
        printk("fail to register monitor device\n");       
        return -ENODEV;
    }
    
    if (platform_driver_register(&mt6516_monitor_driver)){
        printk("fail to register monitor driver\n");       
        platform_device_unregister(&mt6516_monitor_device);
        return -ENODEV;
    }
    
    return 0;
}


static void __exit mt_exit_monitor(void) 
{
	platform_driver_unregister(&mt6516_monitor_driver);    
	printk("mt6516 Performance Monitor is uninitialized!!\n");
}

module_init(mt_init_monitor);
module_exit(mt_exit_monitor);


MODULE_DESCRIPTION("MT6516 performance monitor driver");
MODULE_LICENSE("GPL");

