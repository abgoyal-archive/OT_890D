
#ifndef __MT6516_MONITOR_H__
#define __MT6516_MONITOR_H__

#include <mach/mt6516.h>

//----------------------------------------------------------------------------
// MT6516 performance monitor register

#define MT6516_MONITOR_VIRT          	 (IO_VIRT + 0x00039700)

#define MON_CON	     	 (MT6516_MONITOR_VIRT+0x00)
#define MON_SET      	 (MT6516_MONITOR_VIRT+0x04)
#define MON_CLR      	 (MT6516_MONITOR_VIRT+0x08)
#define MON_PERF1        (MT6516_MONITOR_VIRT+0x0c)
#define MON_PERF2        (MT6516_MONITOR_VIRT+0x10)
#define MON_PERF3		 (MT6516_MONITOR_VIRT+0x14)
#define MON_PERF4	     (MT6516_MONITOR_VIRT+0x18)
#define MON_PERF5	     (MT6516_MONITOR_VIRT+0x1c)
#define MON_PERF6        (MT6516_MONITOR_VIRT+0x20)
#define MON_PERF7   	 (MT6516_MONITOR_VIRT+0x24)
#define MON_PERF8        (MT6516_MONITOR_VIRT+0x28)
#define MON_PERF9	     (MT6516_MONITOR_VIRT+0x2c)
#define MON_PERF10       (MT6516_MONITOR_VIRT+0x30)
#define MON_PERF11	     (MT6516_MONITOR_VIRT+0x34)
#define MON_PERF12       (MT6516_MONITOR_VIRT+0x38)
#define MON_PERF13       (MT6516_MONITOR_VIRT+0x3c)
#define MON_PERF14       (MT6516_MONITOR_VIRT+0x40)
#define MON_PERF15       (MT6516_MONITOR_VIRT+0x44)
#define MON_PERF16	     (MT6516_MONITOR_VIRT+0x48)
#define MON_PERF17       (MT6516_MONITOR_VIRT+0x4c)
#define MON_PERF18       (MT6516_MONITOR_VIRT+0x50)
#define MON_PERF19	     (MT6516_MONITOR_VIRT+0x54)
#define MON_PERF20       (MT6516_MONITOR_VIRT+0x58)
#define MON_PERF21	     (MT6516_MONITOR_VIRT+0x5c)
#define MON_PERF22       (MT6516_MONITOR_VIRT+0x60)

//----------------------------------------------------------------------------
// MT6516 performance register offset
typedef enum
{
    DCM_EN          = 0,
    ICM_EN 	      = 1, 
    ACTIVE_EN     = 2, 
    DTLB_EN	      = 3, 
    ITLB_EN 	      = 4, 
    DCP_EN 	      = 5, 
    ICP_EN 	      = 6, 
    DEXT_EN 	      = 7, 
    IEXT_EN 	      = 8,
    DAHB_EN 	      = 9,
    IAHB_EN 	      = 10,
    DCM_CLR 	      = 11,
    ICM_CLR 	      = 12,
    ACTIVE_CLR    = 13,
    DTLB_CLR 	      = 14,
    ITLB_CLR 	      = 15,
    DCP_CLR 	      = 16,
    ICP_CLR 		= 17,
    DEXT_CLR 	= 18,
    IEXT_CLR 		= 19,
    DAHB_CLR 	= 20,
    IAHB_CLR 		= 21,
    DAHB_SEL_EXT_MEM = 22,
    DAHB_SEL_INT_MEM = 23,    
    DAHB_SEL_APB_REG = 24        
} MT6516_MON_MODE;


//----------------------------------------------------------------------------
// MT6516 performance monitor command

typedef enum
{
    EXT_MEM = 0,
    INT_MEM = 1,    
    REG = 2        
} MT6516_DAHB_SEL;

//----------------------------------------------------------------------------
// MT6516 performance monitor api

void Monitor_EnableMonControl (MT6516_MON_MODE mode);
void Monitor_DisableMonControl (MT6516_MON_MODE mode);
unsigned char Monitor_GetMonControlStatus (MT6516_MON_MODE mode);
void Monitor_SetDAHB (MT6516_DAHB_SEL sel);
void Monitor_DcacheMissBegin(void);
void Monitor_DcacheMissEnd(void);
void Monitor_IcacheMissBegin(void);
void Monitor_IcacheMissEnd(void);
void Monitor_ARMActiveBegin(void);
void Monitor_ARMActiveEnd(void);
void Monitor_DTLBPenaltyBegin(void);
void Monitor_DTLBPenaltyEnd(void);
void Monitor_ITLBPenaltyBegin(void);
void Monitor_ITLBPenaltyEnd(void);
void Monitor_DcachePenaltyBegin(void);
void Monitor_DcachePenaltyEnd(void);
void Monitor_IcachePenaltyBegin(void);
void Monitor_IcachePenaltyEnd(void);
void Monitor_DEXTPenaltyBegin(void);
void Monitor_DEXTPenaltyEnd(void);
void Monitor_IEXTPenaltyBegin(void);
void Monitor_IEXTPenaltyEnd(void);
void Monitor_DAHBBegin(MT6516_DAHB_SEL sel);
void Monitor_DAHBEnd(void);
void Monitor_IAHBBegin(void);
void Monitor_IAHBEnd(void);


#endif  /* __MT6516_MONITOR_H__ */

