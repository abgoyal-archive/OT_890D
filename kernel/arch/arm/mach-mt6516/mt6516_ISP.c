
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <asm/atomic.h>
//Arch dependent files
//#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_ISP.h>

#include <asm/tcm.h>

#define MT6516ISP_DEBUG
#ifdef MT6516ISP_DEBUG
#define ISPDB printk
#else
#define ISPDB(x,...)
#endif

//#define MT6516ISP_PARM
#ifdef MT6516ISP_PARM
#define ISPPARMDB printk
#else
#define ISPPARMDB(x,...)
#endif

static spinlock_t g_MT6516ISPLock;
static u32 u4MT6516ISPRes = 0;
static wait_queue_head_t g_MT6516ISPWQ;
static u32 g_u4MT6516ISPIRQ = 0 ,g_u4MT6516ISPIRQMSK = 0;
static dev_t g_MT6516ISPdevno = MKDEV(ISP_DEV_MAJOR_NUMBER,0);
static struct cdev * g_pMT6516ISP_CharDrv = NULL;
static struct class *isp_class = NULL;
static int g_MT6516ISPIRQNum;

static unsigned long g_MT6516ISPCLKSTATUS = 0;
static atomic_t g_MT6516ISPAtomic;
static int g_u4ColorOrder = 3;

//#define MT6516ISP_DBUFFREGCNT 52
#define MT6516ISP_DBUFFREGCNT 208
typedef struct {
    unsigned long u4Addr[MT6516ISP_DBUFFREGCNT];
    unsigned long u4Value[MT6516ISP_DBUFFREGCNT];
    unsigned long u4Counter;
} stMT6516ISPDBuffRegMap;
static stMT6516ISPDBuffRegMap * g_pMT6516ISPDBuff = NULL;

static void GetMT6516ISPDBuff(stMT6516ISPDBuffRegMap ** a_ppDBuff)
{
    *a_ppDBuff = g_pMT6516ISPDBuff;
}
static void SetMT6516ISPDBuff(unsigned long a_u4Value , unsigned long a_u4Addr)
{
    if(NULL == g_pMT6516ISPDBuff)
    {
        ISPDB("[MT6516ISP] ISP double buff is null\n");
        return;
    }

    if(MT6516ISP_DBUFFREGCNT <= g_pMT6516ISPDBuff->u4Counter)
    {
        ISPDB("[MT6516ISP] ISP double buff is full\n");
        return;
    }

    g_pMT6516ISPDBuff->u4Addr[g_pMT6516ISPDBuff->u4Counter] = a_u4Addr;
    g_pMT6516ISPDBuff->u4Value[g_pMT6516ISPDBuff->u4Counter] = a_u4Value;
    g_pMT6516ISPDBuff->u4Counter += 1;
}
////////////////User space driver
//TODO : Move to user space.

#define MT6516CAM_VERSION (CAM_BASE + 0x274)
//TODO : Find a good place
#define MT6516CAM_VFCON (CAM_BASE + 0x18)
#define MT6516CAM_INTSTA (CAM_BASE + 0x20)
#define MT6516CAM_RESET (CAM_BASE + 0x1D8)
#define MT6516CAM_TGSTATUS (CAM_BASE + 0x1DC)
#define MT6516CAM_PHSCNT (CAM_BASE)
#define MT6516CAM_SHADING1 (CAM_BASE + 0x214)
#define MT6516CAM_G1MEMPDN (CONFIG_BASE + 0x30C)
#define MT6516CAM_PATH (CAM_BASE + 0x24)
static int MT6516ISP_Run(u32 a_bRun)
{
    u32 u4RegValue = 0;

//Clean int
    spin_lock_irq(&g_MT6516ISPLock);
    g_u4MT6516ISPIRQ = ioread32(MT6516CAM_INTSTA);
    g_u4MT6516ISPIRQ = 0;
    atomic_set(&g_MT6516ISPAtomic , 0);
    spin_unlock_irq(&g_MT6516ISPLock);

    if(a_bRun)
    {
        //! Sean, Move the reset mechanism to setIOIF(), not here
#if 0        
         //Reset ISP frame counter
        //! Sean, Move this to reset ISP only at run. and when stop, don' 
        //! reset the ISP, because sensor may still work (YUV sensor)
        iowrite32(1 , MT6516CAM_RESET);
        mdelay(1);//Delay at least 100us for reseting the ISP
        iowrite32(0 , MT6516CAM_RESET);
#endif 

        //ISP related clock
        u4RegValue = ioread32(MT6516CAM_G1MEMPDN);
        if((ioread32(MT6516CAM_PATH) & (0x3 << 20)) == 0)
        {
            //Turn on SRAM clock, 1us later, start run
            hwEnableClock(MT6516_PDN_MM_RESZLB,"ISP");
            u4RegValue &= (~(1<<8));
        }
        u4RegValue &= (~(1<<9));
        iowrite32(u4RegValue , MT6516CAM_G1MEMPDN);

        //DRAM in case, to trigger another process, got to disable then enable again.
        u4RegValue = ioread32(MT6516CAM_VFCON);
        u4RegValue &= (~(1<<6));
        iowrite32(u4RegValue , MT6516CAM_VFCON);
        u4RegValue |= (1<<6);
        iowrite32(u4RegValue , MT6516CAM_VFCON);

        strobe_StillExpCfgStart();//config capture flash

        enable_irq(g_MT6516ISPIRQNum);
    }
    else
    {
        //Vignetting must be off --> reset in Vignetting setting
        //iowrite32(0,MT6516CAM_SHADING1);

        u4RegValue = ioread32(MT6516CAM_VFCON);
        u4RegValue &= (~(1<<6));
        iowrite32(u4RegValue , MT6516CAM_VFCON);

        //ISP related clock
        u4RegValue = ioread32(MT6516CAM_G1MEMPDN);
        u4RegValue |= (1<<9);
        if((ioread32(MT6516CAM_PATH) & (0x3 << 20)) == 0)
        {
            //Turn off SRAM clock
            hwDisableClock(MT6516_PDN_MM_RESZLB,"ISP");
            u4RegValue |= (1<<8);
        }
        iowrite32(u4RegValue , MT6516CAM_G1MEMPDN);

        disable_irq(g_MT6516ISPIRQNum);
    }

    return 0;
}

inline static unsigned long MT6516ISP_QueryRun(void)
{
    return ((ioread32(MT6516CAM_VFCON) & (1<<6))>>6);
}

#define MT6516CAM_GRABCOL (CAM_BASE + 0x8)
#define MT6516CAM_GRABROW (CAM_BASE + 0xC)
#define MT6516CAM_RWINV_SEL (CAM_BASE + 0x174)
#define MT6516CAM_RWINH_SEL (CAM_BASE + 0x178)

//#define MT6516CAM_PATH (CAM_BASE + 0x24)
#define MT6516CAM_NR1_CON (CAM_BASE + 0x550)//
//The function is to adjust the grab window according to the demosaic and NR setting.
//Demosaic w+4 h+6, Demosaic+NR w+6 h+8.
#if 0
static void MT6516ISP_AdjGrabWin(unsigned long a_u4Set , unsigned long a_u4Width , unsigned long a_u4Height)
{
    static unsigned long u4Width, u4Height;
    unsigned long u4ExtraWidth, u4ExtraHeight;
    unsigned long u4StartX, u4StartY;
    unsigned long u4RegVal = 0;

    if(a_u4Set)
    {
        u4Width = a_u4Width;
        u4Height = a_u4Height;
    }

    //Check current status
    u4RegVal = ioread32(MT6516CAM_PATH);
    if(u4RegVal & (1<<16))// output to DRAM, no demosaic
    {
        u4RegVal = ioread32(MT6516CAM_NR1_CON);
        if(u4RegVal & 0x7)
        {
            u4ExtraWidth = 2;
            u4ExtraHeight = 2;
        }
        else
        {
            u4ExtraWidth = 0;
            u4ExtraHeight = 0;
        }
    }else//output to CRZ
    {
        u4RegVal = ioread32(MT6516CAM_NR1_CON);
        if(u4RegVal & 0x7)// NR + demosaic
        {
            u4ExtraWidth = 6;
            u4ExtraHeight = 8;
        }
        else // demosaic only
        {
            u4ExtraWidth = 4;
            u4ExtraHeight = 6;
        }
    }

    //demosaic w+4, h+6
    //NR w + 2 h + 2
    u4RegVal = ioread32(MT6516CAM_GRABCOL);
    u4StartX = ((u4RegVal & 0xFFFF0000) >> 16) - 1;
    u4RegVal &= (~0xFFFF);
    u4RegVal += (u4Width + u4ExtraWidth + u4StartX);
    iowrite32(u4RegVal , MT6516CAM_GRABCOL);

    u4RegVal = ioread32(MT6516CAM_GRABROW);
    u4StartY = ((u4RegVal & 0xFFFF0000) >> 16) - 1;
    u4RegVal &= (~0xFFFF);
    u4RegVal += (u4Height + u4ExtraHeight + u4StartY);
    iowrite32(u4RegVal , MT6516CAM_GRABROW);
}
#endif

/* macros of TG grab start/end pixel configuration register */
#define SET_TG_GRAB_PIXEL(start,pixel)		    (iowrite32(((u32)(start) << 16) + (u32)(pixel + (start) - 1),   MT6516CAM_GRABCOL))
#define SET_TG_GRAB_LINE(start,line)		           (iowrite32(((u32)(start) << 16) +  (u32)(line +(start) - 1), MT6516CAM_GRABROW))
#define SET_RESULT_WINDOW_GRAB_PIXEL(start,pixel) (iowrite32( ((u32)(start) << 16) + (u32)(start + pixel) , MT6516CAM_RWINH_SEL))
#define SET_RESULT_WINDOW_GRAB_LINE(start, pixel) (iowrite32((1 << 28) +((u32)(start) << 16) + (u32)(start+ pixel), MT6516CAM_RWINV_SEL))


//I/O interface
//When memory in, bayer raw out is not supported.
#define MT6516CAM_CAMWIN (CAM_BASE + 0x4)
//#define MT6516CAM_GRABCOL (CAM_BASE + 0x8)
//#define MT6516CAM_GRABROW (CAM_BASE + 0xC)
#define MT6516CAM_CSMODE (CAM_BASE + 0x10)
#define MT6516CAM_INTEN (CAM_BASE + 0x1C)
//#define MT6516CAM_PATH (CAM_BASE + 0x24)
#define MT6516CAM_INADDR (CAM_BASE + 0x28)
#define MT6516CAM_OUTADDR (CAM_BASE + 0x2C)
#define MT6516CAM_CSISTA1 (CAM_BASE + 0xE4)
#define MT6516CAM_CSISTA2 (CAM_BASE + 0xE8)
#define MT6516CAM_VSUB (CAM_BASE + 0x128)
#define MT6516CAM_HSUB (CAM_BASE + 0x12C)
#define MT6516CAM_CAMCRZ_CTRL (CAM_BASE + 0x1A0)
#define MT6516CAM_MEMINDB_CTRL (CAM_BASE + 0x180)//Debug
#define MT6516CAM_MEMINDB1 (CAM_BASE + 0x184)//Debug
#define MT6516CAM_LASTADDR (CAM_BASE + 0x188)//Debug
#define MT6516CAM_XFERCNT (CAM_BASE + 0x18C)//Debug
#define MT6516CAM_MDLCFG1 (CAM_BASE + 0x190)//Debug
#define MT6516CAM_MDLCFG2 (CAM_BASE + 0x194)//Debug
#define REG_ISP_LPF_CTRL  (CAM_BASE + 0x011C)
#define MT6516CAM_DRIVING_CURRENT   (CONFIG_BASE+0x0500)
//Demosaic w+4 h+6, Demosaic+NR w+6 h+8.
static int MT6516ISP_SetIOIF(stMT6516ISPIFCFG * a_pstCfg)
{
    u32 u4RegValue = 0;
    u32 u4CLKCNT = 0;

    //! Reset the ISP, 
    //! before reset, ensure the take pic is set to 0 (HW Suggest) 
    iowrite32(0, MT6516CAM_VFCON); 
    iowrite32(1 , MT6516CAM_RESET);
    mdelay(1);//Delay at least 100us for reseting the ISP
    iowrite32(0 , MT6516CAM_RESET);

    //Source from DRAM, don't need to check the clock
    if( 1 == a_pstCfg->bSource &&
    	((a_pstCfg->u4OutputClkInkHz < 3250) || (a_pstCfg->u4OutputClkInkHz >= 104000)))
    {
        ISPDB("[mt6516_ISP] Input clock rate error !\n");
        return -EINVAL;
    }

    //Check the input dimension
    if((a_pstCfg->u2TotalWidthInPixel + a_pstCfg->u2HOffsetInPixel > 4095) ||
    	 (a_pstCfg->u2TotalHeightInLine + a_pstCfg->u2VOffsetInLine > 4095) ||
    	 (a_pstCfg->u2CropWidth + a_pstCfg->u2CropWinXStart > 4095) ||
        (a_pstCfg->u2CropHeight  + a_pstCfg->u2CropWinYStart > 4095) ||
        (0 == a_pstCfg->u2TotalWidthInPixel) || (0 == a_pstCfg->u2TotalHeightInLine) ||
        (0 == a_pstCfg->u2CropWidth) || (0 == a_pstCfg->u2CropHeight))
    {
        ISPDB("[mt6516_ISP] Input dimension error !\n");
        return -EINVAL;
    }

    //out put to CRZ , format must be YUV444
    if((0 == a_pstCfg->bDestination ) && (MT6516OUTFMT_YUV444 != a_pstCfg->eOutFormat))
    {
        ISPDB("[mt6516_ISP] Output to CRZ should be YUV444 !\n");
        return -EINVAL;
    }

//Debug message
    ISPPARMDB("ISP grab win x : %d y: %d w: %d h: %d \n", a_pstCfg->u2CropWinXStart ,a_pstCfg->u2CropWinYStart ,
        a_pstCfg->u2CropWidth, a_pstCfg->u2CropHeight);

    ISPPARMDB("ISP freq %d kHz \n", a_pstCfg->u4OutputClkInkHz);

    ISPPARMDB("In PhyAddr : 0x%x \n",a_pstCfg->u4InAddr);
    ISPPARMDB("Out PhyAddr : 0x%x \n",a_pstCfg->u4OutAddr);
//
    //Master clock switch 0:52Mhz, 1:48Mhz 
    if (a_pstCfg->bMasterClockSwtich)
    {
        u4CLKCNT = (96000 + (a_pstCfg->u4OutputClkInkHz >> 1)) / a_pstCfg->u4OutputClkInkHz;
        u4CLKCNT = (u4CLKCNT > 15 ? 15 : u4CLKCNT);
    }
    else 
    {
        u4CLKCNT = (104000 + (a_pstCfg->u4OutputClkInkHz >> 1)) / a_pstCfg->u4OutputClkInkHz;
        u4CLKCNT = (u4CLKCNT > 15 ? 15 : u4CLKCNT);
    }

    u4RegValue = (1 << 31) + (1 << 29) + ((u4CLKCNT-1) << 24) + ((u4CLKCNT >> 1) << 16) +
        + (1<<12) + ((u4CLKCNT-1) << 4) + ((u4CLKCNT >> 1));
    u4RegValue += ((u4CLKCNT & 0x1 ? 1 : 0 ) << 11);
    u4RegValue += (a_pstCfg->bMasterClockSwtich << 8); 

    iowrite32(u4RegValue , MT6516CAM_PHSCNT);

    u4RegValue = (0xFFF << 16) + 0xFFF;
//    u4RegValue = (((u32)a_pstCfg->u2CropWidth + 4) << 16) + (u32)a_pstCfg->u2CropHeight + 7;
    iowrite32(u4RegValue , MT6516CAM_CAMWIN);

   //set grab window
   SET_TG_GRAB_PIXEL(a_pstCfg->u2HOffsetInPixel, a_pstCfg->u2TotalWidthInPixel );
   SET_TG_GRAB_LINE(a_pstCfg->u2VOffsetInLine, a_pstCfg->u2TotalHeightInLine);


    if(1 == a_pstCfg->bSourceIF) //serial interface
    {
        u4RegValue = ((u32)a_pstCfg->stMipiIF.bCSI2Header << 12) + ((u32)a_pstCfg->stMipiIF.bECC << 11) +
        ((u32)a_pstCfg->stMipiIF.bEnDLane4 << 10) + ((u32)a_pstCfg->stMipiIF.bEnDLane2 << 9) + (1<<8) + 1;
    }
    else
    {    //parallel, set sync signal polarity
        u4RegValue = ((u32)a_pstCfg->stParallelIF.bVsyncPolarity << 7) + ((u32)a_pstCfg->stParallelIF.bHsyncPolarity << 6) + 1;
    }
    iowrite32(u4RegValue , MT6516CAM_CSMODE);

    u4RegValue = ioread32(MT6516CAM_VFCON);
    u4RegValue &= (~0xC000FFBF);
    u4RegValue += ((u32)a_pstCfg->bCapMode << 7);
    //If single capture mode , always skip first frame,
    if(a_pstCfg->bCapMode)
    {
        u4RegValue += (a_pstCfg->u2CapDelay << 8);
    }
    if(0 == a_pstCfg->bSourceIF)// parallel interface
    {
        u4RegValue += ((u32)a_pstCfg->stParallelIF.bVsyncPolarity << 30);
    }

    iowrite32(u4RegValue , MT6516CAM_VFCON);

    //If raw out, the format is 0 2b' + R[0:9] 10b' + G[0:9] 10b' + B[0:9] 10b'
    //By experience, CLKCNT is propotional to D zoom ratio. 1x : 3, 2x : 5 .etc...
    //TODO : Simply it.
    u4CLKCNT = a_pstCfg->u2TotalWidthInPixel/a_pstCfg->u2CropWidth;
    u4CLKCNT += 2;
    u4CLKCNT = (u4CLKCNT > 15 ? 15 : u4CLKCNT);
//    u4CLKCNT = 15;
    if(0 == a_pstCfg->bDestination)// To CRZ
    {
        u4RegValue = (3 << 25) + (1 << 20) + (a_pstCfg->bSwapY << 13) + (a_pstCfg->bSwapCbCr << 12) + (1 << 11) + (a_pstCfg->eInFormat << 8 ) +
        (u4CLKCNT << 4) + (1 << 3) + a_pstCfg->bSource;
    }
    else// To DRAM
    {
    	//TODO : 8 bit Bayer
        u4RegValue = (3 << 25) + (1<<24) + (1 << 23) + (a_pstCfg->eOutFormat << 20) + (1 << 16) +
        	(1 << 11) + (a_pstCfg->eInFormat << 8 ) + (u4CLKCNT << 4) + (1 << 3) + a_pstCfg->bSource;
    }

    if(MT6516OUTFMT_Bayer == a_pstCfg->eOutFormat)
    {
        u4RegValue += (7 << 17);
    }

    iowrite32(u4RegValue,MT6516CAM_PATH);

    iowrite32(a_pstCfg->u4InAddr ,MT6516CAM_INADDR);

    iowrite32(a_pstCfg->u4OutAddr ,MT6516CAM_OUTADDR);

#if 0
    if(0 == a_pstCfg->bDestination)// To CRZ
    {
        u4RegValue = (1 << 28) + ((u32)(a_pstCfg->u2CropWinYStart) << 16)
        + (u32)(a_pstCfg->u2CropWinYStart + a_pstCfg->u2CropHeight);
        iowrite32(u4RegValue,MT6516CAM_RWINV_SEL);

        u4RegValue = ((u32)(a_pstCfg->u2CropWinXStart) << 16) +
            (u32)(a_pstCfg->u2CropWinXStart + a_pstCfg->u2CropWidth);
        iowrite32(u4RegValue,MT6516CAM_RWINH_SEL);

    }
    else
    {
        iowrite32(0,MT6516CAM_RWINV_SEL);
        iowrite32(0,MT6516CAM_RWINH_SEL);
    }
#endif
    //set result win
    SET_RESULT_WINDOW_GRAB_PIXEL(a_pstCfg->u2CropWinXStart, a_pstCfg->u2CropWidth);
    SET_RESULT_WINDOW_GRAB_LINE(a_pstCfg->u2CropWinYStart , a_pstCfg->u2CropHeight);

//    iowrite32(,MT6516CAM_CAMCRZ_CTRL);

    u4RegValue = ioread32(MT6516CAM_INTEN);
    if(MT6516OUTFMT_Bayer == a_pstCfg->eOutFormat)
    {
        u4RegValue |= (1<<24);
    }
    else
    {
        u4RegValue &= (~(1<<24));
    }
    iowrite32(u4RegValue , MT6516CAM_INTEN);

    //!
    //! YUV sensor, control this register to increase performance,
    //! Only for YUV sensor
    if (MT6516INFMT_YUV422 == a_pstCfg->eInFormat || MT6516INFMT_YCBCR == a_pstCfg->eInFormat)
    {
        u4RegValue = ioread32(REG_ISP_LPF_CTRL);
        iowrite32(u4RegValue | 0x00100000, REG_ISP_LPF_CTRL);
    }

    //! Set Cam I/O Driving Current 
    u4RegValue = ioread32(MT6516CAM_DRIVING_CURRENT);     
    
    switch (a_pstCfg->uDrivingCurrent)
    {
        case 0:   // 2MA
        	iowrite32(u4RegValue & (~((u32)0x3 << 12)), MT6516CAM_DRIVING_CURRENT); 
        	break; 
        case 1: // 4MA
        	iowrite32(u4RegValue & (~((u32)0x3 << 12)) | (0x2 << 12), MT6516CAM_DRIVING_CURRENT); 
        	break; 
        case 2: // 6MA
        	iowrite32(u4RegValue & (~((u32)0x3 << 12)) | (0x1 << 12), MT6516CAM_DRIVING_CURRENT); 
        	break; 
        case 3: // 8MA
   	       iowrite32(u4RegValue & (~((u32)0x3 << 12)) | (0x3 << 12), MT6516CAM_DRIVING_CURRENT); 
        	break; 
    }
    //printk("Driving Current:0x%08x\n", ioread32(MT6516CAM_DRIVING_CURRENT)); 
    //printk("Grab StartX:%d\n", a_pstCfg->u2HOffsetInPixel); 
    //printk("Grab StartY:%d\n",  a_pstCfg->u2VOffsetInLine);
    //printk("Driving Current:%d\n", a_pstCfg->uDrivingCurrent); 
    //printk("Master Clock switch:%d\n", a_pstCfg->bMasterClockSwtich); 

    g_u4ColorOrder = a_pstCfg->u4ColorOrder;
    return 0;
}

//TODO : get I/O IF configuration

//#define MT6516CAM_INTSTA (CAM_BASE + 0x20)
static int MT6516ISP_SetIRQ(stMT6516ISPIRQCfg * a_pstCfg)
{
    u32 u4RegValue = 0;

    //Check parameters
    if((a_pstCfg->u2AVSyncLine > 4095))
    {
        ISPDB("[MT6516ISP] Set AVsync line count exceeds \n");
        return -EINVAL;
    }

    if((a_pstCfg->u2IRQEnMask > 0x1FF))
    {
        ISPDB("[MT6516ISP] Set IRQ error \n");
        return -EINVAL;
    }

    u4RegValue = ioread32(MT6516CAM_VFCON);

    u4RegValue &= (~(0xFFF << 16));

    u4RegValue += a_pstCfg->u2AVSyncLine << 16;

    iowrite32(u4RegValue , MT6516CAM_VFCON);

    u4RegValue = ioread32(MT6516CAM_INTEN);

    u4RegValue &= (~0x1FF);

    u4RegValue += (u32)a_pstCfg->u2IRQEnMask;

    u4RegValue |= 0x7;

    iowrite32(u4RegValue , MT6516CAM_INTEN);

    g_u4MT6516ISPIRQMSK = a_pstCfg->u2IRQEnMask;

    return 0;
}

static int MT6516ISP_WaitIRQ(u32 * u4IRQMask)
{
    spin_lock_irq(&g_MT6516ISPLock);
    wait_event_interruptible(g_MT6516ISPWQ, (g_u4MT6516ISPIRQMSK & g_u4MT6516ISPIRQ) );

    *u4IRQMask = g_u4MT6516ISPIRQ;

    ISPPARMDB("[ISP] IRQ : 0x%4x \n",g_u4MT6516ISPIRQ);

    g_u4MT6516ISPIRQ = 0;
    spin_unlock_irq(&g_MT6516ISPLock);

    return 0;
}

static int MT6516ISP_CheckIRQ(u32 * u4IRQMask)
{

    *u4IRQMask = ioread32(MT6516CAM_INTSTA);

    return 0;
}

#define MT6516CAM_RAWGAIN0 (CAM_BASE + 0x16C)
#define MT6516CAM_RAWGAIN1 (CAM_BASE + 0x170)
inline static int MT6516ISP_SetRawGain(stMT6516ISPRawGainCFG * a_pstCfg)
{
    u32 u4RegValue = 0;

    //raw gain
    if(MT6516ISP_QueryRun()){
        u4RegValue = ((u32)a_pstCfg->u2RawPreRgain << 16) + (u32)a_pstCfg->u2RawPreGrgain;
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_RAWGAIN0);

        u4RegValue = ((u32)a_pstCfg->u2RawPreBgain << 16) + (u32)a_pstCfg->u2RawPreGbgain;
        SetMT6516ISPDBuff(u4RegValue,MT6516CAM_RAWGAIN1);
    }
    else
    {
        u4RegValue = ((u32)a_pstCfg->u2RawPreRgain << 16) + (u32)a_pstCfg->u2RawPreGrgain;
        iowrite32(u4RegValue,MT6516CAM_RAWGAIN0);

        u4RegValue = ((u32)a_pstCfg->u2RawPreBgain << 16) + (u32)a_pstCfg->u2RawPreGbgain;
        iowrite32(u4RegValue,MT6516CAM_RAWGAIN1);
    }

    return 0;
}

#define MT6516CAM_CTRL2 (CAM_BASE + 0x44)
#define MT6516CAM_AEWINH (CAM_BASE + 0x48)
#define MT6516CAM_AEHISWIN (CAM_BASE + 0x4C)
#define MT6516CAM_AEHISGAIN (CAM_BASE + 0x50)
#define MT6516CAM_AFWIN0 (CAM_BASE + 0x24C)
#define MT6516CAM_AFWIN1 (CAM_BASE + 0x250)
#define MT6516CAM_AFWIN2 (CAM_BASE + 0x254)
#define MT6516CAM_AFWIN3 (CAM_BASE + 0x258)
#define MT6516CAM_AFWIN4 (CAM_BASE + 0x25C)
#define MT6516CAM_AFWIN5 (CAM_BASE + 0x268)
#define MT6516CAM_AFWIN6 (CAM_BASE + 0x26C)
#define MT6516CAM_AFWIN7 (CAM_BASE + 0x270)
#define MT6516CAM_AFTH0 (CAM_BASE + 0x260)
#define MT6516CAM_AFTH1 (CAM_BASE + 0x264)
#define MT6516CAM_AWBSUM_WIN (CAM_BASE + 0x27C)
#define MT6516CAM_AWB_CTRL (CAM_BASE + 0x280)
#define MT6516CAM_AWBTH (CAM_BASE + 0x284)
#define MT6516CAM_AWBXYH1 (CAM_BASE + 0x288)
#define MT6516CAM_AWBXYH2 (CAM_BASE + 0x28C)
#define MT6516CAM_AWBCE_WINH (CAM_BASE + 0x290)
#define MT6516CAM_AWBCE_WINV (CAM_BASE + 0x294)
#define MT6516CAM_AWBXY_WINH0 (CAM_BASE + 0x298)
#define MT6516CAM_AWBXY_WINV0 (CAM_BASE + 0x29C)
//....
#define MT6516CAM_AWBXY_WINHB (CAM_BASE + 0x2F0)
#define MT6516CAM_AWBXY_WINVB (CAM_BASE + 0x2F4)
#define MT6516CAM_HST_CON (CAM_BASE + 0x540)
#define MT6516CAM_HST_CFG1 (CAM_BASE + 0x544)
#define MT6516CAM_HST_CFG2 (CAM_BASE + 0x548)
#define MT6516CAM_HST_CFG3 (CAM_BASE + 0x54C)
inline static int MT6516ISP_Set3AStatistic(stMT6516ISP3AStatisticCFG * a_pstCfg)
{
    u32 u4RegValue = 0;
    u32 u4Index = 0;

    //switch
    u4RegValue = ((u32)a_pstCfg->bAEHistEn << 25) + (1 << 24) + (1 << 23) + ((u32)a_pstCfg->bAELuminanceWinEn << 22) +
        (1 << 21) + ((u32)a_pstCfg->bFlareHistEn << 19) + ((u32)a_pstCfg->bAFWinEn << 15) +
        ((u32)a_pstCfg->bAFFilter << 13) + (1 << 8) + (1 << 6);
    iowrite32(u4RegValue,MT6516CAM_CTRL2);

    //AE statistic window
    u4RegValue = ((u32)a_pstCfg->u2AEWinXStart << 25) + ((u32)a_pstCfg->u2AEWinWidth << 16) +
        ((u32)a_pstCfg->u2AEWinYStart << 9) + (u32)a_pstCfg->u2AEWinHeight;
    iowrite32(u4RegValue,MT6516CAM_AEWINH);

    //Histogram window
    u4RegValue = ((u32)a_pstCfg->u2HistWinXStart << 24) +
        ((u32)(a_pstCfg->u2HistWinXStart + a_pstCfg->u2HistWinWidth) << 16) +
        ((u32)a_pstCfg->u2HistWinYStart << 8) + (u32)(a_pstCfg->u2HistWinYStart + a_pstCfg->u2HistWinHeight);
    iowrite32(u4RegValue,MT6516CAM_AEHISWIN);

    //AE histogram gain
    iowrite32((1 << 8),MT6516CAM_AEHISGAIN);

    //Histogram configuration
    iowrite32(a_pstCfg->bAEHistEn , MT6516CAM_HST_CON);

    u4RegValue = ((u32)a_pstCfg->u2HistWinXStart << 16) + a_pstCfg->u2HistWinXStart + a_pstCfg->u2HistWinWidth;
    iowrite32(u4RegValue , MT6516CAM_HST_CFG1);

    u4RegValue = ((u32)a_pstCfg->u2HistWinYStart << 16) + a_pstCfg->u2HistWinYStart + a_pstCfg->u2HistWinHeight;
    iowrite32(u4RegValue , MT6516CAM_HST_CFG2);

    //AF window
    iowrite32(a_pstCfg->stAFWin[0].u4RegValue, MT6516CAM_AFWIN0);
    iowrite32(a_pstCfg->stAFWin[1].u4RegValue , MT6516CAM_AFWIN1);
    iowrite32(a_pstCfg->stAFWin[2].u4RegValue , MT6516CAM_AFWIN2);
    iowrite32(a_pstCfg->stAFWin[3].u4RegValue , MT6516CAM_AFWIN3);
    iowrite32(a_pstCfg->stAFWin[4].u4RegValue , MT6516CAM_AFWIN4);
    iowrite32(a_pstCfg->stAFWin[5].u4RegValue , MT6516CAM_AFWIN5);
    iowrite32(a_pstCfg->stAFWin[6].u4RegValue , MT6516CAM_AFWIN6);
    iowrite32(a_pstCfg->stAFWin[7].u4RegValue , MT6516CAM_AFWIN7);

    //AF configuration
    u4RegValue = a_pstCfg->uAFThresholdCFG[3] + (a_pstCfg->uAFThresholdCFG[2] << 8) +
    (a_pstCfg->uAFThresholdCFG[1] << 16) + (a_pstCfg->uAFThresholdCFG[0] << 24);
    iowrite32(u4RegValue , MT6516CAM_AFTH0);
    iowrite32(a_pstCfg->uAFThresholdCFG[4] , MT6516CAM_AFTH1);

    //AWB configuration
    iowrite32(a_pstCfg->stAWBSumWin.u4RegValue , MT6516CAM_AWBSUM_WIN);

    iowrite32(a_pstCfg->stAWBCtl.u4RegValue , MT6516CAM_AWB_CTRL);

    iowrite32(a_pstCfg->stAWBThreshold.u4RegValue , MT6516CAM_AWBTH);

    u4RegValue = (u32)a_pstCfg->stAWBColorSpace.uAWBH12 +
        ((u32)a_pstCfg->stAWBColorSpace.bAWBH12_SIGN << 8) +
        ((u32)a_pstCfg->stAWBColorSpace.uAWBH11 << 16) +
        ((u32)a_pstCfg->stAWBColorSpace.bAWBH11_SIGN << 24);
    iowrite32(u4RegValue , MT6516CAM_AWBXYH1);

    u4RegValue = (u32)a_pstCfg->stAWBColorSpace.uAWBH22 +
        ((u32)a_pstCfg->stAWBColorSpace.bAWBH22_SIGN << 8) +
        ((u32)a_pstCfg->stAWBColorSpace.uAWBH21 << 16) +
        ((u32)a_pstCfg->stAWBColorSpace.bAWBH21_SIGN << 24);
    iowrite32(u4RegValue , MT6516CAM_AWBXYH2);

    for(u4Index = 0 ; u4Index < MT6516ISP_AWBWINCNT; u4Index += 1){
        u4RegValue = (u32)a_pstCfg->stAWBWin[u4Index].u2WinR + ((u32)a_pstCfg->stAWBWin[u4Index].u2WinL << 16);
        iowrite32(u4RegValue , (MT6516CAM_AWBCE_WINH + (u4Index << 3)));
        u4RegValue = (u32)a_pstCfg->stAWBWin[u4Index].u2WinD + ((u32)a_pstCfg->stAWBWin[u4Index].u2WinU << 16);
        iowrite32(u4RegValue , (MT6516CAM_AWBCE_WINV + (u4Index << 3)));
    }

    return 0;
}

//TODO : get 3A statistic window configuration

//
#define MT6516CAM_AEMEM0 (CAM_BASE + 0x1000)
//...0x1000~0x1054
#define MT6516CAM_FLAREMEM0 (CAM_BASE + 0x1060)
//...0x1060~0x1084
#define MT6516CAM_AFFILTER0 (CAM_BASE + 0x1088)
//...0x1088~0x1124
#define MT6516CAM_AFMEAN0 (CAM_BASE + 0x1128)
//...0x1128~0x1144
#define MT6516CAM_AWBXYWIN0 (CAM_BASE + 0x1148)
//...0x1148~0x1204
#define MT6516CAM_AWBSUMWIN0 (CAM_BASE + 0x1208)
//...0x1208~0x1214
#define MT6516CAM_AWBCEWIN0 (CAM_BASE + 0x1218)
//...0x1218~0x1224
#define MT6516CAM_AEHIS0 (CAM_BASE + 0x1228)
//...0x1228~0x1324
inline static int MT6516ISP_Update3AStatistic(stMT6516ISP3AStat * a_pstCfg)
{

    memcpy(a_pstCfg, (u32 *)MT6516CAM_AEMEM0, 808);

    return 0;
}

#define MT6516CAM_DEFECT0 (CAM_BASE + 0x154)
#define MT6516CAM_DEFECT1 (CAM_BASE + 0x158)
#define MT6516CAM_DEFECT2 (CAM_BASE + 0x15C)// Read only , for debug purpose
inline static int MT6516ISP_SetBadPixelComp(stMT6516ISPBadPixelCFG * a_pstCfg)
{
    u32 u4RegValue = 0;

    u4RegValue = ((u32)a_pstCfg->uDefectCorrectEn << 24) + (4 << 16);
    iowrite32(u4RegValue,MT6516CAM_DEFECT0);

    iowrite32(a_pstCfg->u4DefectTblAddr ,MT6516CAM_DEFECT1);

    return 0;
}

//TODO : get bad pixel compensation configuration

//No double buffer
#define MT6516CAM_RGBOFF (CAM_BASE + 0x14)
#define MT6516CAM_FLREGAIN (CAM_BASE + 0xB0)
#define MT6516CAM_FLREOFF (CAM_BASE + 0xB4)
#define MT6516CAM_YCCGO_CON (CAM_BASE + 0x600)
#define MT6516CAM_YCCGO_CFG2 (CAM_BASE + 0x608)
#define MT6516CAM_YCCGO_CFG3 (CAM_BASE + 0x60C)
#define MT6516CAM_YCCGO_CFG4 (CAM_BASE + 0x610)
#define MT6516CAM_YCCGO_CFG5 (CAM_BASE + 0x614)
inline static int MT6516ISP_SetContrast(stMT6516ISPContrastCFG * a_pstCfg)
{
    u32 u4RegValue = 0;

    if((a_pstCfg->uCh00Offset > 127) || (a_pstCfg->uCh01Offset > 127) || \
    	(a_pstCfg->uCh10Offset > 127) || (a_pstCfg->uCh11Offset > 127))
    {
        printk("Err: %d, %d, %d, %d \n", a_pstCfg->uCh00Offset, a_pstCfg->uCh01Offset, a_pstCfg->uCh10Offset, a_pstCfg->uCh10Offset);
        return -EINVAL;
    }

    u4RegValue = (1 << 31) + ((u32)a_pstCfg->uCh00Offset << 24) + \
    (1 << 23) + ((u32)a_pstCfg->uCh01Offset << 16) + \
    (1 << 15) + ((u32)a_pstCfg->uCh10Offset << 8) + \
    (1 << 7) + ((u32)a_pstCfg->uCh00Offset);

    if(MT6516ISP_QueryRun()){
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_FLREOFF);
        u4RegValue = ((u32)a_pstCfg->uRGain << 16) + ((u32)a_pstCfg->uGGain << 8) + (u32)a_pstCfg->uBGain;
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_FLREGAIN);
    }
    else
    {
        iowrite32(u4RegValue , MT6516CAM_FLREOFF);
        u4RegValue = ((u32)a_pstCfg->uRGain << 16) + ((u32)a_pstCfg->uGGain << 8) + (u32)a_pstCfg->uBGain;
        iowrite32(u4RegValue , MT6516CAM_FLREGAIN);
    }


    //seperate to MT6516ISP_SetYCCGO
    return 0;
}

inline static int MT6516ISP_SetYCCGO(stMT6516ISPYCCGOCFG * a_pstCfg)
{
    u32 u4RegValue[7] = {0};
    u4RegValue[0] = ((u32)a_pstCfg->YCC_ENY1 << 5) + \
                    ((u32)a_pstCfg->YCC_ENY2 << 4) + \
                    ((u32)a_pstCfg->YCC_ENY3 << 3) + \
                    ((u32)a_pstCfg->YCC_ENC1 << 2) + \
                    ((u32)a_pstCfg->YCC_ENC2 << 1) + \
                    ((u32)a_pstCfg->YCC_ENC3);

    u4RegValue[1] = ((u32)a_pstCfg->YCC_MU  << 24) + \
                    ((u32)a_pstCfg->YCC_MV  << 16) + \
                    ((u32)a_pstCfg->YCC_H11 << 8) + \
                    ((u32)a_pstCfg->YCC_H12);

    u4RegValue[2] = ((u32)a_pstCfg->YCC_Y1 << 24) + \
                    ((u32)a_pstCfg->YCC_Y2 << 16) + \
                    ((u32)a_pstCfg->YCC_Y3 << 8) + \
                    ((u32)a_pstCfg->YCC_Y4);

    u4RegValue[3] = ((u32)a_pstCfg->YCC_G1 << 24) + \
                    ((u32)a_pstCfg->YCC_G2 << 16) + \
                    ((u32)a_pstCfg->YCC_G3 << 8) + \
                    ((u32)a_pstCfg->YCC_G4);

    u4RegValue[4] = ((u32)a_pstCfg->YCC_G5    << 24) + \
                    ((u32)a_pstCfg->YCC_OFSTY << 16) + \
                    ((u32)a_pstCfg->YCC_OFSTU << 8) + \
                    ((u32)a_pstCfg->YCC_OFSTV);

    u4RegValue[5] = ((u32)a_pstCfg->YCC_YBNDH << 24) + \
                    ((u32)a_pstCfg->YCC_YBNDL << 16) + \
                    ((u32)a_pstCfg->YCC_GAINY);

    u4RegValue[6] = ((u32)a_pstCfg->YCC_UBNDH << 24) + \
                    ((u32)a_pstCfg->YCC_UBNDL << 16) + \
                    ((u32)a_pstCfg->YCC_VBNDH << 8) + \
                    ((u32)a_pstCfg->YCC_VBNDL);

    if(MT6516ISP_QueryRun()){
        SetMT6516ISPDBuff(u4RegValue[0] , MT6516CAM_YCCGO_CON       );
        SetMT6516ISPDBuff(u4RegValue[1] , MT6516CAM_YCCGO_CON + 0x4 );
        SetMT6516ISPDBuff(u4RegValue[2] , MT6516CAM_YCCGO_CON + 0x8 );
        SetMT6516ISPDBuff(u4RegValue[3] , MT6516CAM_YCCGO_CON + 0xC );
        SetMT6516ISPDBuff(u4RegValue[4] , MT6516CAM_YCCGO_CON + 0x10);
        SetMT6516ISPDBuff(u4RegValue[5] , MT6516CAM_YCCGO_CON + 0x14);
        SetMT6516ISPDBuff(u4RegValue[6] , MT6516CAM_YCCGO_CON + 0x18);
    }
    else
    {
        iowrite32(u4RegValue[0] , MT6516CAM_YCCGO_CON       );
        iowrite32(u4RegValue[1] , MT6516CAM_YCCGO_CON + 0x4 );
        iowrite32(u4RegValue[2] , MT6516CAM_YCCGO_CON + 0x8 );
        iowrite32(u4RegValue[3] , MT6516CAM_YCCGO_CON + 0xC );
        iowrite32(u4RegValue[4] , MT6516CAM_YCCGO_CON + 0x10);
        iowrite32(u4RegValue[5] , MT6516CAM_YCCGO_CON + 0x14);
        iowrite32(u4RegValue[6] , MT6516CAM_YCCGO_CON + 0x18);
    }
    return 0;
}

inline static int MT6516ISP_SetOBLevel(stMT6516ISPOBLevelCFG* a_pstCfg)
{
    u32 u4RegValue = 0;

    if((a_pstCfg->uCh00Offset > 127) || (a_pstCfg->uCh01Offset > 127) || \
    	(a_pstCfg->uCh10Offset > 127) || (a_pstCfg->uCh11Offset > 127))
    {
        printk("Err: %d, %d, %d, %d \n", a_pstCfg->uCh00Offset, a_pstCfg->uCh01Offset, a_pstCfg->uCh10Offset, a_pstCfg->uCh10Offset);
        return -EINVAL;
    }

    u4RegValue = (1 << 31) + ((u32)a_pstCfg->uCh00Offset << 24) + \
    (1 << 23) + ((u32)a_pstCfg->uCh01Offset << 16) + \
    (1 << 15) + ((u32)a_pstCfg->uCh10Offset << 8) + \
    (1 << 7) + ((u32)a_pstCfg->uCh11Offset);

    if(MT6516ISP_QueryRun())
    {
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_RGBOFF);
    }
    else
    {
        iowrite32(u4RegValue , MT6516CAM_RGBOFF);
    }

    return 0;
}

//TODO : get contrast configuration

#define MT6516CAM_RAWACC (CAM_BASE + 0x1BC)
#define MT6516CAM_RAWWIN (CAM_BASE + 0x1C0)
#define MT6516CAM_RAWSUM_B (CAM_BASE + 0x1C4)//result , read only
#define MT6516CAM_RAWSUM_GB (CAM_BASE + 0x1C8)//result , read only
#define MT6516CAM_RAWSUM_GR (CAM_BASE + 0x1CC)//result , read only
#define MT6516CAM_RAWSUM_R (CAM_BASE + 0x1D0)//result , read only

#define MT6516CAM_SHADING2 (CAM_BASE + 0x218)
#define MT6516CAM_SD_RADDR (CAM_BASE + 0x21C)
#define MT6516CAM_SD_LBLOCK (CAM_BASE + 0x220)
#define MT6516CAM_SD_RATIO (CAM_BASE + 0x224)
inline static int MT6516ISP_SetVignettingComp(stMT6516ISPVignettingCFG * a_pstCfg)
{
    u32 u4RegValue = 0;

    if(MT6516ISP_QueryRun())
    {
        u4RegValue = (u32)a_pstCfg->bRawAccuWinEn;
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_RAWACC);
        u4RegValue = (u32)a_pstCfg->u4RawAccuWinDimension;
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_RAWWIN);

        u4RegValue = 0x00000000;
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_SHADING1);
        u4RegValue = a_pstCfg->uShadingPamrams[0];
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_SHADING2);
        u4RegValue = a_pstCfg->uShadingPamrams[1];
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_SD_RADDR);
        u4RegValue = a_pstCfg->uShadingPamrams[2];
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_SD_LBLOCK);
        u4RegValue = a_pstCfg->uShadingPamrams[3];
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_SD_RATIO);
        u4RegValue = ((u32)a_pstCfg->bShadingBlockTriggger<< 29) + ((u32)a_pstCfg->bShadingCompEnable << 28);
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_SHADING1);
    }
    else
    {
        iowrite32(a_pstCfg->bRawAccuWinEn,MT6516CAM_RAWACC);
        iowrite32(a_pstCfg->u4RawAccuWinDimension,MT6516CAM_RAWWIN);

        //reset shading controller
        iowrite32(0,MT6516CAM_SHADING1);
        //Reset ISP frame counter
        //iowrite32(1 , MT6516CAM_RESET);
       // mdelay(1);//Delay at least 100us for reseting the ISP
        //iowrite32(0 , MT6516CAM_RESET);
        //set parameter
        iowrite32(a_pstCfg->uShadingPamrams[0],MT6516CAM_SHADING2);
        iowrite32(a_pstCfg->uShadingPamrams[1],MT6516CAM_SD_RADDR);
        iowrite32(a_pstCfg->uShadingPamrams[2],MT6516CAM_SD_LBLOCK);
        iowrite32(a_pstCfg->uShadingPamrams[3],MT6516CAM_SD_RATIO);
        u4RegValue = ((u32)a_pstCfg->bShadingBlockTriggger<< 29) + ((u32)a_pstCfg->bShadingCompEnable << 28);
        iowrite32(u4RegValue,MT6516CAM_SHADING1);
    }
    return 0;
}

//TODO : get contrast configuration

//No double buffer
#define MT6516CAM_CTRL1 (CAM_BASE + 0x30)
#define MT6516CAM_RGBGAIN1 (CAM_BASE + 0x34)
#define MT6516CAM_RGBGAIN2 (CAM_BASE + 0x38)
inline static int MT6516ISP_SetColorGain(stMT6516ISPColorGainCFG * a_pstCfg)
{
    u32 u4RegValue = 0;

    if(MT6516ISP_QueryRun())//Run time modify
    {
        u4RegValue = (1 << 31) + (255 << 16) + (g_u4ColorOrder << 12) + (a_pstCfg->u2RawPreGain << 1);
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_CTRL1);

        u4RegValue = ((u32)a_pstCfg->u2BGain << 16) + a_pstCfg->u2GbGain;
        SetMT6516ISPDBuff(u4RegValue,MT6516CAM_RGBGAIN1);

        u4RegValue = ((u32)a_pstCfg->u2RGain << 16) + a_pstCfg->u2GrGain;
        SetMT6516ISPDBuff(u4RegValue,MT6516CAM_RGBGAIN2);
    }
    else
    {
        u4RegValue = (1 << 31) + (255 << 16) + (g_u4ColorOrder << 12) + (a_pstCfg->u2RawPreGain << 1);
        iowrite32(u4RegValue , MT6516CAM_CTRL1);

        u4RegValue = ((u32)a_pstCfg->u2BGain << 16) + a_pstCfg->u2GbGain;
        iowrite32(u4RegValue,MT6516CAM_RGBGAIN1);

        u4RegValue = ((u32)a_pstCfg->u2RGain << 16) + a_pstCfg->u2GrGain;
        iowrite32(u4RegValue,MT6516CAM_RGBGAIN2);
    }

    return 0;
}

//TODO : get color gain configuration

//No double buffer
#define MT6516CAM_CPSCON1 (CAM_BASE + 0x70)
#define MT6516CAM_INTER1 (CAM_BASE + 0x74)
#define MT6516CAM_INTER2 (CAM_BASE + 0x78)
inline static int MT6516ISP_SetInterpolation(stMT6516ISPInterpolationCFG * a_pstCfg)
{
    if(MT6516ISP_QueryRun())//Run time modify
    {
        SetMT6516ISPDBuff(a_pstCfg->u4Params[0],MT6516CAM_CPSCON1);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[1],MT6516CAM_INTER1);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[2],MT6516CAM_INTER2);
    }
    else
    {
        iowrite32(a_pstCfg->u4Params[0],MT6516CAM_CPSCON1);
        iowrite32(a_pstCfg->u4Params[1],MT6516CAM_INTER1);
        iowrite32(a_pstCfg->u4Params[2],MT6516CAM_INTER2);
    }

    return 0;
}

//TODO : get interpolation configuration

#define MT6516CAM_NR2_CON (CAM_BASE + 0x500)
#define MT6516CAM_NR2_CFG2 (CAM_BASE + 0x508)
#define MT6516CAM_NR2_CFG3 (CAM_BASE + 0x50C)
#define MT6516CAM_NR2_CFG4 (CAM_BASE + 0x510)
//#define MT6516CAM_NR1_CON (CAM_BASE + 0x550)
#define MT6516CAM_NR1_DP1 (CAM_BASE + 0x554)
#define MT6516CAM_NR1_DP2 (CAM_BASE + 0x558)
#define MT6516CAM_NR1_DP3 (CAM_BASE + 0x55C)
#define MT6516CAM_NR1_DP4 (CAM_BASE + 0x560)
#define MT6516CAM_NR1_CT (CAM_BASE + 0x564)
#define MT6516CAM_NR1_NR1 (CAM_BASE + 0x568)
//....
#define MT6516CAM_NR1_NR10 (CAM_BASE + 0x58C)
inline static int MT6516ISP_SetNoiseFilter(stMT6516ISPNoiseFilterCFG * a_pstCfg)
{
    u32 u4Index = 3;

    if (a_pstCfg->uReserved & 0x02)
    {
        iowrite32(((u32)a_pstCfg->bYNREn + ((u32)a_pstCfg->bUVNREn << 1)),MT6516CAM_NR2_CON);
        iowrite32(a_pstCfg->u4NRParam[0],MT6516CAM_NR2_CFG2);
        iowrite32(a_pstCfg->u4NRParam[1],MT6516CAM_NR2_CFG3);
        iowrite32(a_pstCfg->u4NRParam[2],MT6516CAM_NR2_CFG4);
    }
    if (a_pstCfg->uReserved & 0x01)
    {
        for(u4Index = 3 ; u4Index < 19 ; u4Index += 1){
            iowrite32(a_pstCfg->u4NRParam[u4Index], (MT6516CAM_NR1_CON + ((u4Index - 3) << 2)));
        }
    }

#if 0
    MT6516ISP_AdjGrabWin(0,0,0);
#endif

    return 0;
}

//TODO : get interpolation configuration

//No double buffer
#define MT6516CAM_MATRIX1 (CAM_BASE + 0x9C)
#define MT6516CAM_MATRIX2 (CAM_BASE + 0xA0)
#define MT6516CAM_MATRIX3 (CAM_BASE + 0xA4)
#define MT6516CAM_YCHAN (CAM_BASE + 0xB8)
#define MT6516CAM_RGB2YCC (CAM_BASE + 0xBC)
#define MT6516CAM_YCCGO_CFG1 (CAM_BASE + 0x604)
#define MT6516CAM_YCCGO_CFG6 (CAM_BASE + 0x618)
//TODO : set color process configuration
inline static int MT6516ISP_SetColor(stMT6516ISPColorCFG * a_pstCfg)
{
    u32 u4RegValue = 0;

    u4RegValue = ((u32)a_pstCfg->uCCMParam[0] << 16) + ((u32)a_pstCfg->uCCMParam[1] << 8) + \
        ((u32)a_pstCfg->uCCMParam[2]);
    if(MT6516ISP_QueryRun())//Run time modify
    {
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_MATRIX1);
    }
    else
    {
        iowrite32(u4RegValue , MT6516CAM_MATRIX1);
    }

    u4RegValue = ((u32)a_pstCfg->uCCMParam[3] << 16) + ((u32)a_pstCfg->uCCMParam[4] << 8) + \
        ((u32)a_pstCfg->uCCMParam[5]);
    if(MT6516ISP_QueryRun())//Run time modify
    {
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_MATRIX2);
    }
    else
    {
        iowrite32(u4RegValue , MT6516CAM_MATRIX2);
    }

    u4RegValue = ((u32)a_pstCfg->uCCMParam[6] << 16) + ((u32)a_pstCfg->uCCMParam[7] << 8) + \
        ((u32)a_pstCfg->uCCMParam[8]);
    if(MT6516ISP_QueryRun())//Run time modify
    {
        SetMT6516ISPDBuff(u4RegValue , MT6516CAM_MATRIX3);
    }
    else
    {
        iowrite32(u4RegValue , MT6516CAM_MATRIX3);
    }

//    iowrite32(0,MT6516CAM_YCHAN);
//    iowrite32(0xFF0190B7,MT6516CAM_RGB2YCC);

    return 0;
}

//TODO : get color process configuration

//No double buffer
#define MT6516CAM_GMA_REG1 (CAM_BASE + 0x1A8)
#define MT6516CAM_GMA_REG2 (CAM_BASE + 0x1AC)
#define MT6516CAM_GMA_REG3 (CAM_BASE + 0x1B0)
#define MT6516CAM_GMA_REG4 (CAM_BASE + 0x1B4)
#define MT6516CAM_GMA_REG5 (CAM_BASE + 0x1B8)
inline static int MT6516ISP_SetGamma(stMT6516ISPGammaCFG * a_pstCfg)
{
    if(MT6516ISP_QueryRun())//Run time modify
    {
        SetMT6516ISPDBuff(a_pstCfg->u4Params[0],MT6516CAM_GMA_REG1);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[1],MT6516CAM_GMA_REG2);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[2],MT6516CAM_GMA_REG3);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[3],MT6516CAM_GMA_REG4);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[4],MT6516CAM_GMA_REG5);
    }
    else
    {
        iowrite32(a_pstCfg->u4Params[0],MT6516CAM_GMA_REG1);
        iowrite32(a_pstCfg->u4Params[1],MT6516CAM_GMA_REG2);
        iowrite32(a_pstCfg->u4Params[2],MT6516CAM_GMA_REG3);
        iowrite32(a_pstCfg->u4Params[3],MT6516CAM_GMA_REG4);
        iowrite32(a_pstCfg->u4Params[4],MT6516CAM_GMA_REG5);
    }

    return 0;
}

//TODO : get color process configuration

//No double buffer
#define MT6516CAM_GDC_CON (CAM_BASE + 0x520)
inline static int MT6516ISP_SetLCE(stMT6516ISPLCECFG * a_pstCfg)
{
    u32 u4Index = 0;

    if(MT6516ISP_QueryRun())//Run time modify
    {
        for(; u4Index < MT6516ISP_LCE_PARAMCNT ; u4Index += 1)
        {
            SetMT6516ISPDBuff(a_pstCfg->u4Params[u4Index], (MT6516CAM_GDC_CON + (u4Index << 2)));
        }
    }
    else
    {
        for(; u4Index < MT6516ISP_LCE_PARAMCNT ; u4Index += 1)
        {
            iowrite32(a_pstCfg->u4Params[u4Index], (MT6516CAM_GDC_CON + (u4Index << 2)));
        }
    }

    return 0;
}
//TODO

//No double buffer
#define MT6516CAM_EDGCORE (CAM_BASE + 0x7C)
#define MT6516CAM_EDGGAIN1 (CAM_BASE + 0x80)
#define MT6516CAM_EDGGAIN2 (CAM_BASE + 0x84)
#define MT6516CAM_EDGTHRE (CAM_BASE + 0x88)
#define MT6516CAM_EDGVCON (CAM_BASE + 0x8C)
#define MT6516CAM_CPSCON2 (CAM_BASE + 0xAC)
#define MT6516CAM_EE_CTRL (CAM_BASE + 0x230)
#define MT6516CAM_EE_LUT_X (CAM_BASE + 0x234)
#define MT6516CAM_EE_LUT_Y (CAM_BASE + 0x238)
inline static int MT6516ISP_SetEdgeEnhance(stMT6516ISPEdgeEnhanceCFG * a_pstCfg)
{
    if(MT6516ISP_QueryRun())//Run time modify
    {
        SetMT6516ISPDBuff(a_pstCfg->u4Params[0],MT6516CAM_EDGCORE);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[1],MT6516CAM_EDGGAIN1);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[2],MT6516CAM_EDGGAIN2);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[3],MT6516CAM_EDGTHRE);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[4],MT6516CAM_EDGVCON);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[5],MT6516CAM_CPSCON2);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[6],MT6516CAM_EE_CTRL);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[7],MT6516CAM_EE_LUT_X);
        SetMT6516ISPDBuff(a_pstCfg->u4Params[8],MT6516CAM_EE_LUT_Y);
    }
    else
    {
        iowrite32(a_pstCfg->u4Params[0],MT6516CAM_EDGCORE);
        iowrite32(a_pstCfg->u4Params[1],MT6516CAM_EDGGAIN1);
        iowrite32(a_pstCfg->u4Params[2],MT6516CAM_EDGGAIN2);
        iowrite32(a_pstCfg->u4Params[3],MT6516CAM_EDGTHRE);
        iowrite32(a_pstCfg->u4Params[4],MT6516CAM_EDGVCON);
        iowrite32(a_pstCfg->u4Params[5],MT6516CAM_CPSCON2);
        iowrite32(a_pstCfg->u4Params[6],MT6516CAM_EE_CTRL);
        iowrite32(a_pstCfg->u4Params[7],MT6516CAM_EE_LUT_X);
        iowrite32(a_pstCfg->u4Params[8],MT6516CAM_EE_LUT_Y);
    }
    return 0;
}

//TODO : get edge enhancement configuration

inline static int MT6516ISP_ApplyCfg(void)
{
    if(MT6516ISP_QueryRun())
    {
        atomic_set(&g_MT6516ISPAtomic , 1);
    }

    return 0;
}

//No double buffer
#define MT6516CAM_GSCTRL (CAM_BASE + 0x54)
#define MT6516CAM_MSCTRL (CAM_BASE + 0x58)
#define MT6516CAM_MS1TIME (CAM_BASE + 0x5C)
#define MT6516CAM_MS2TIME (CAM_BASE + 0x60)
//TODO : set machenical shutter
inline static int MT6516ISP_SetMshutter(stMT6516ISPMshutCFG * a_pstCfg)
{
    return 0;
}

//TODO : get machenical shutter configuration

//No double buffer
#define MT6516CAM_FLASH_CTRL0 (CAM_BASE + 0x1E0)
#define MT6516CAM_FLASH_CTRL1 (CAM_BASE + 0x1E4)
#define MT6516CAM_FLASHB_CTRL0 (CAM_BASE + 0x1E8)
#define MT6516CAM_FLASHB_CTRL1 (CAM_BASE + 0x1EC)
//TODO : set flash light
inline static int MT6516ISP_SetFlashLight(stMT6516ISPFlashCFG * a_pstCfg)
{
    return 0;
}

//TODO : get flash light configuration

//

inline static int MT6516ISP_SetKernelBuffer(stMT6516ISPKernelBufferCFG *inBuffer)
{
    unsigned int * pu4dest;
    pu4dest= (unsigned int *)(0xF020E000+inBuffer->u4SRAMOffset);
    memcpy(pu4dest, inBuffer->u1Params,inBuffer->u4BufferLength);
    //printk("--SRAM Shading Table content 0x%x---",*pu4dest);
    return 0;
}

void MT6516ISP_DUMPREG(void)
{
    u32 u4RegValue = 0;
    u32 u4Index = 0;
    ISPDB("ISP REG:\n ********************\n");

//    for(; u4Index < 0x650; u4Index += 4){
    //General setting
    for(; u4Index < 0x28; u4Index += 4){
        u4RegValue = ioread32(CAM_BASE + u4Index);
        ISPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }
    //Subsample factor
    for(u4Index = 0x128 ; u4Index < 0x130; u4Index += 4){
        u4RegValue = ioread32(CAM_BASE + u4Index);
        ISPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }
    //Result window
    for(u4Index = 0x174 ; u4Index < 0x194; u4Index += 4){
        u4RegValue = ioread32(CAM_BASE + u4Index);
        ISPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }
    //TG status
    for(u4Index = 0x1D8 ; u4Index < 0x1E0; u4Index += 4){
        u4RegValue = ioread32(CAM_BASE + u4Index);
        ISPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }
    //ISP clocks

}
EXPORT_SYMBOL(MT6516ISP_DUMPREG);

inline static int MT6516ISP_SetReg(stMT6516ISPRegIO * a_pstCfg)
{
stMT6516ISPRegIO *pREGIO = 0;
u32 i=0;
            pREGIO = (stMT6516ISPRegIO*)a_pstCfg;
            if (pREGIO->count > MT6516ISP_MAX_REGIO_CNT)
            {
                ISPDB("[MT6516ISP] ioctl regio cnt invalid\n");
                return -EFAULT;
            }

            for (i=0;i<pREGIO->count;i++)
            {
                iowrite32(pREGIO->pVal[i],CAM_BASE + pREGIO->pAddr[i]);
                ISPDB("[MT6516ISP_SET] [0x%x][0x%x]\n",pREGIO->pAddr[i],pREGIO->pVal[i]);
            }
    return 0;
}

inline static int MT6516ISP_GetReg(stMT6516ISPRegIO * a_pstCfg)
{
stMT6516ISPRegIO *pREGIO = 0;
u32 i=0;
            pREGIO = (stMT6516ISPRegIO*)a_pstCfg;

            if (pREGIO->count > MT6516ISP_MAX_REGIO_CNT)
            {
                ISPDB("[MT6516ISP] ioctl regio cnt invalid\n");
                return -EFAULT;
            }

            for (i=0;i<pREGIO->count;i++)
            {
                pREGIO->pVal[i] = ioread32(CAM_BASE + pREGIO->pAddr[i]);
                ISPDB("[MT6516ISP_SET] [0x%x][0x%x]\n",pREGIO->pAddr[i],pREGIO->pVal[i]);
            }
    return 0;
}


static int MT6516_ISP_Ioctl(struct inode * a_pstInode,
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    int i4RetValue = 0;
    void * pBuff = 0;
    //Allocate memory and copy from user
//    _IOC_DIR
//    _IOC_SIZE

    if(_IOC_NONE == _IOC_DIR(a_u4Command))
    {
    }
    else
    {
        pBuff = kmalloc(_IOC_SIZE(a_u4Command),GFP_KERNEL);

        if(NULL == pBuff)
        {
            ISPDB("[MT6516ISP] ioctl allocate mem failed\n");
            return -ENOMEM;
        }

        if(_IOC_WRITE & _IOC_DIR(a_u4Command))
        {
            if(copy_from_user(pBuff , (void *) a_u4Param, _IOC_SIZE(a_u4Command)))
            {
                kfree(pBuff);
                ISPDB("[MT6516ISP] ioctl copy from user failed\n");
                return -EFAULT;
            }
        }
    }

    switch(a_u4Command)
    {
        case MT6516ISPIOC_S_IOIF :
            //input and output interface
            ISPPARMDB("SetIOIF \n");
            i4RetValue = MT6516ISP_SetIOIF((stMT6516ISPIFCFG *)pBuff);
            break;

        case MT6516ISPIOC_G_WAITIRQ :
            i4RetValue = MT6516ISP_WaitIRQ((u32 *)pBuff);
            break;

        case MT6516ISPIOC_G_CHECKIRQ :
            i4RetValue = MT6516ISP_CheckIRQ((u32 *)pBuff);
            break;

        case MT6516ISPIOC_S_SETIRQ :
            ISPPARMDB("SetIRQ \n");
            i4RetValue = MT6516ISP_SetIRQ((stMT6516ISPIRQCfg *)pBuff);
            break;

        case MT6516ISPIOC_T_RUN :
            ISPPARMDB("Run \n");
            i4RetValue = MT6516ISP_Run(*(u32 *)pBuff);
            break;

        case MT6516ISPIOC_Q_RUN :
            *(unsigned long *)pBuff = MT6516ISP_QueryRun();
            break;

        case MT6516ISPIOC_S_3A_STATISTICS_CFG :
            i4RetValue = MT6516ISP_Set3AStatistic((stMT6516ISP3AStatisticCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_RAWGAIN :
            i4RetValue = MT6516ISP_SetRawGain((stMT6516ISPRawGainCFG *)pBuff);
            break;

        case MT6516ISPIOC_T_3A_UpdatesSTATISTICS :
            i4RetValue = MT6516ISP_Update3AStatistic((stMT6516ISP3AStat *)pBuff);
            break;

        case MT6516ISPIOC_S_BADPIXEL_CFG :
            i4RetValue = MT6516ISP_SetBadPixelComp((stMT6516ISPBadPixelCFG *)pBuff);
            break;
        case MT6516ISPIOC_S_OBLEVEL :
            i4RetValue = MT6516ISP_SetOBLevel((stMT6516ISPOBLevelCFG *) pBuff);
            break;

        case MT6516ISPIOC_S_CONTRAST :
            i4RetValue = MT6516ISP_SetContrast((stMT6516ISPContrastCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_VIGNETTING_CFG :
            i4RetValue = MT6516ISP_SetVignettingComp((stMT6516ISPVignettingCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_COLORGAIN_CFG :
            i4RetValue = MT6516ISP_SetColorGain((stMT6516ISPColorGainCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_INTERPOLATION_CFG :
            i4RetValue = MT6516ISP_SetInterpolation((stMT6516ISPInterpolationCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_NOISEFILTER_CFG :
            i4RetValue = MT6516ISP_SetNoiseFilter((stMT6516ISPNoiseFilterCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_COLORPROCESS_CFG :
            i4RetValue = MT6516ISP_SetColor((stMT6516ISPColorCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_GAMMA_CFG :
            i4RetValue = MT6516ISP_SetGamma((stMT6516ISPGammaCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_EDGEENHANCE_CFG :
            i4RetValue = MT6516ISP_SetEdgeEnhance((stMT6516ISPEdgeEnhanceCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_LCE_CFG :
            i4RetValue = MT6516ISP_SetLCE((stMT6516ISPLCECFG *)pBuff);
            break;

        case MT6516ISPIOC_S_MSHUT_CFG :
            i4RetValue = MT6516ISP_SetMshutter((stMT6516ISPMshutCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_FLASH_CFG :
            i4RetValue = MT6516ISP_SetFlashLight((stMT6516ISPFlashCFG *)pBuff);
            break;

        case MT6516ISPIOC_T_APPLY_CFG :
            i4RetValue = MT6516ISP_ApplyCfg();
            break;

        case MT6516ISPIOC_T_DUMPREG :
            MT6516ISP_DUMPREG();
            break;

        case MT6516ISPIOC_S_REGISTER:
            i4RetValue = MT6516ISP_SetReg((stMT6516ISPRegIO*)pBuff);
            break;
        case MT6516ISPIOC_G_REGISTER:
            //copy command set
            if(copy_from_user(pBuff , (void *) a_u4Param, _IOC_SIZE(a_u4Command)))
            {
                kfree(pBuff);
                ISPDB("[MT6516ISP] ioctl copy from user failed\n");
                return -EFAULT;
            }

            i4RetValue = MT6516ISP_GetReg((stMT6516ISPRegIO*)pBuff);

            break;
        case MT6516ISPIOC_S_YCCGO :
            i4RetValue = MT6516ISP_SetYCCGO((stMT6516ISPYCCGOCFG *)pBuff);
            break;

        case MT6516ISPIOC_S_KernelBuffer_CFG :
            i4RetValue = MT6516ISP_SetKernelBuffer((stMT6516ISPKernelBufferCFG*)pBuff);
            break;

        default :
            ISPDB("No CMD \n");
            i4RetValue = -EPERM;
            break;
    }

    if(_IOC_READ & _IOC_DIR(a_u4Command))
    {
        if(copy_to_user((void __user *) a_u4Param , pBuff , _IOC_SIZE(a_u4Command)))
        {
            kfree(pBuff);
            ISPDB("[MT6516ISP] ioctl copy to user failed\n");
            return -EFAULT;
        }
    }

    kfree(pBuff);

    return i4RetValue;
}

////////////////Kernel space driver//////////////////////

//#define
//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
// 3.Update f_op pointer.
// 4.Fill data structures into private_data
//CAM_RESET
static int MT6516_ISP_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    int i4RetValue = 0;

    spin_lock_irq(&g_MT6516ISPLock);

    if(u4MT6516ISPRes)
    {
        i4RetValue = -EBUSY;
    }
    else
    {
        //Allocate memory for double buffer
        if(NULL != g_pMT6516ISPDBuff)
        {
            ISPDB("[MT6516ISP] in open warning : previous allocated memory is not freed\n");
            kfree(g_pMT6516ISPDBuff);
            g_pMT6516ISPDBuff = NULL;
        }
        g_pMT6516ISPDBuff = kmalloc(sizeof(stMT6516ISPDBuffRegMap),GFP_ATOMIC);
        if(NULL == g_pMT6516ISPDBuff)
        {
            ISPDB("[MT6516ISP] Not enough memory for ISP double buffer\n");
            spin_unlock_irq(&g_MT6516ISPLock);
            return -ENOMEM;
        }
        memset(g_pMT6516ISPDBuff,0,sizeof(stMT6516ISPDBuffRegMap));
        u4MT6516ISPRes += 1;
        atomic_set(&g_MT6516ISPAtomic , 0);
        hwEnableClock(MT6516_PDN_MM_ISP,"ISP");
        g_MT6516ISPCLKSTATUS = 1;
//        hwEnableClock(MT6516_PDN_MM_RESZLB,"ISP");
    }

    spin_unlock_irq(&g_MT6516ISPLock);

    return i4RetValue;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int MT6516_ISP_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    return 0;
}

static int MT6516_ISP_Flush(struct file * a_pstFile , fl_owner_t a_id)
{

    if(MT6516ISP_QueryRun())
    {
        ISPDB("ISP error handling!!!\n");
        MT6516ISP_Run(0);
    }

    spin_lock_irq(&g_MT6516ISPLock);

    u4MT6516ISPRes = 0;

    atomic_set(&g_MT6516ISPAtomic , 0);

    if(NULL == g_pMT6516ISPDBuff)
    {
        ISPDB("[MT6516ISP] in release warning : previous allocated memory is freed too early\n");
    }
    else
    {
        kfree(g_pMT6516ISPDBuff);
        g_pMT6516ISPDBuff = NULL;
    }

    hwDisableClock(MT6516_PDN_MM_ISP,"ISP");
    g_MT6516ISPCLKSTATUS = 0;
    spin_unlock_irq(&g_MT6516ISPLock);
    ISPPARMDB("[ISP] closed \n");

    return 0;
}

static const struct file_operations g_stMT6516_ISP_fops =
{
    .owner = THIS_MODULE,
    .open = MT6516_ISP_Open,
    .release = MT6516_ISP_Release,
    .flush = MT6516_ISP_Flush,
    .ioctl = MT6516_ISP_Ioctl
};

#define MT6516ISP_DYNAMIC_ALLOCATE_DEVNO 1
inline static int RegisterMT6516ISPCharDrv(void)
{
#if MT6516ISP_DYNAMIC_ALLOCATE_DEVNO

    if( alloc_chrdev_region(&g_MT6516ISPdevno, 0, 1,"mt6516-ISP") )
    {
        ISPDB("[mt6516_IDP] Allocate device no failed\n");

        return -EAGAIN;
    }

#else

    if( register_chrdev_region(  g_MT6516ISPdevno , 1 , "mt6516-ISP") )
    {
        ISPDB("[mt6516_IDP] Register device no failed\n");

        return -EAGAIN;
    }
#endif

    //Allocate driver
    g_pMT6516ISP_CharDrv = cdev_alloc();

    if(NULL == g_pMT6516ISP_CharDrv)
    {
        unregister_chrdev_region(g_MT6516ISPdevno, 1);

        ISPDB("[mt6516_IDP] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pMT6516ISP_CharDrv, &g_stMT6516_ISP_fops);

    g_pMT6516ISP_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pMT6516ISP_CharDrv, g_MT6516ISPdevno, 1))
    {
        ISPDB("[mt6516_IDP] Attatch file operation failed\n");

        unregister_chrdev_region(g_MT6516ISPdevno, 1);

        return -EAGAIN;
    }

    return 0;
}

inline static void UnregisterMT6516ISPCharDrv(void)
{
    //Release char driver
    cdev_del(g_pMT6516ISP_CharDrv);

    unregister_chrdev_region(g_MT6516ISPdevno, 1);
}

//! Enable Master clock to sensor 
inline void MT6516ISP_EnableSensorClock() 
{
    u32 u4Reg = ioread32(MT6516CAM_PHSCNT); 

    iowrite32(u4Reg | ((u32)0x1 << 29), MT6516CAM_PHSCNT);  
    //printk("MT6516CAM_PHSCNT:0x%8x\n", ioread32(MT6516CAM_PHSCNT));
}	

//! Disable Master clock to sensor 
inline void MT6516ISP_DisableSensorClock()
{
    u32 u4Reg = ioread32(MT6516CAM_PHSCNT); 

    iowrite32(u4Reg & (~((u32)0x1 << 29)), MT6516CAM_PHSCNT); 
    //printk("MT6516CAM_PHSCNT:0x%8x\n", ioread32(MT6516CAM_PHSCNT));
}

void MT6516ISP_tasklet(unsigned long a_u4Param)
{
    stMT6516ISPDBuffRegMap * pDBuff = NULL;
    unsigned long u4Index = 0;

    //The reason doing everything here is to keep the loading consist.
    if(atomic_read(&g_MT6516ISPAtomic))
    {
        GetMT6516ISPDBuff(&pDBuff);

        if(NULL == pDBuff)
        {
             ISPDB("[MT6516ISP] warning : try to set NULL DBuff\n");
             return ;
         }

        if(0 == pDBuff->u4Counter)
        {
            atomic_set(&g_MT6516ISPAtomic , 0);
            return;
        }
//        ISPDB("[MT6516ISP] DBuff counter : %lu\n" , pDBuff->u4Counter);

        for(u4Index = 0 ; u4Index < pDBuff->u4Counter ; u4Index += 1)
        {
//            ISPDB("Addr : 0x%x , Val : 0x%x\n",pDBuff->u4Addr[u4Index],pDBuff->u4Value[u4Index]);
            iowrite32(pDBuff->u4Value[u4Index],pDBuff->u4Addr[u4Index]);
        }

        pDBuff->u4Counter = 0;

        atomic_set(&g_MT6516ISPAtomic , 0);

    }
}
DECLARE_TASKLET(MT6516ISPTaskLet, MT6516ISP_tasklet, 0);

#define MT6516CRZ_CFG (CRZ_BASE)
#define MT6516CRZ_CON (CRZ_BASE + 0x4)
#define MT6516IMGDMA0_OVL_MO_0_STR (IMGDMA_BASE + 0x780)
#define MT6516IMGDMA1_OVL_MO_1_STR (IMGDMA1_BASE + 0x780)
#define MT6516IMGDMA1_VDOENC_STR (IMGDMA1_BASE + 0x200)
#define MT6516DRZ_STR (DRZ_BASE)
#define MT6516IMPROC_EN (IMG_BASE + 0x320)
#define MT6516IMGDMA0_IBW1_STR (IMGDMA_BASE + 0x300)
#define MT6516IMGDMA1_JPEG_CON (IMGDMA1_BASE + 0x104)
#define MT6516IMGDMA0_IBW2_CON (IMGDMA_BASE + 0x404)
static void inline MT6516ISP_OVERRUN_ErrHandle(void)
{
    unsigned long u4RegValue = 0;
    unsigned long u4DRZPathOn = 0;

    if(ioread32(MT6516CAM_PATH) & (1 << 16))
    {
        return;
    }

    //Disable and reset path
    //ISP
    u4RegValue = ioread32(MT6516CAM_VFCON);
    u4RegValue &= (~(1<<6));
    iowrite32(u4RegValue , MT6516CAM_VFCON);
    //CRZ
    iowrite32((1 << 16), MT6516CRZ_CON);
    iowrite32(0, MT6516CRZ_CON);
    //OVL
    iowrite32(0,MT6516IMGDMA0_OVL_MO_0_STR);
    iowrite32(0,MT6516IMGDMA1_OVL_MO_1_STR);
    //VDODECDMA
    iowrite32(0,MT6516IMGDMA1_VDOENC_STR);
    //DRZ
    u4DRZPathOn = ioread32(MT6516DRZ_STR);
    if(u4DRZPathOn)
    {
        iowrite32(0,MT6516DRZ_STR);    
         //Y2R1
        u4RegValue = ioread32(MT6516IMPROC_EN);
        u4RegValue &= (~0x100);
        iowrite32(u4RegValue,MT6516IMPROC_EN);
        //IBW1
        iowrite32(0,MT6516IMGDMA0_IBW1_STR);
    }

    //Make sure frame sync is on
    u4RegValue = ioread32(MT6516CRZ_CFG);
    u4RegValue |= (1 << 13);
    iowrite32(u4RegValue , MT6516CRZ_CFG);
    u4RegValue = ioread32(MT6516IMGDMA1_JPEG_CON);
    u4RegValue |= (1<<7);
    iowrite32(u4RegValue , MT6516IMGDMA1_JPEG_CON);
    u4RegValue = ioread32(MT6516IMGDMA0_IBW2_CON);
    u4RegValue |= (1<<5);
    iowrite32(u4RegValue , MT6516IMGDMA0_IBW2_CON);

    //Enable path
    if(u4DRZPathOn)
    {
        //IBW1
        iowrite32(1,MT6516IMGDMA0_IBW1_STR);
        //Y2R1
        u4RegValue = ioread32(MT6516IMPROC_EN);
        u4RegValue |= 0x100;
        iowrite32(u4RegValue,MT6516IMPROC_EN);
        //DRZ
        iowrite32(1,MT6516DRZ_STR);
    }
    //VDOENCDMA
    iowrite32(1,MT6516IMGDMA1_VDOENC_STR);
//    VDOENCDMA_BUFFIND(RESET_BUFF , NULL , 0);
    //OVL
    iowrite32(1,MT6516IMGDMA1_OVL_MO_1_STR);
    iowrite32(1,MT6516IMGDMA0_OVL_MO_0_STR);
    //CRZ
    iowrite32(0x1,MT6516CRZ_CON);
    //ISP
    iowrite32(1 , MT6516CAM_RESET);
    mdelay(1);//Delay at least 100us for reseting the ISP
    iowrite32(0 , MT6516CAM_RESET);
    u4RegValue = ioread32(MT6516CAM_VFCON);
    iowrite32(u4RegValue , MT6516CAM_VFCON);
    u4RegValue |= (1<<6);
    iowrite32(u4RegValue , MT6516CAM_VFCON);
}

static __tcmfunc irqreturn_t MT6516ISP_irq(int irq, void *dev_id)
{
    g_u4MT6516ISPIRQ |= ioread32(MT6516CAM_INTSTA);
    g_u4MT6516ISPIRQ |= ioread32(MT6516CAM_INTSTA);

    if(g_u4MT6516ISPIRQ & 0x6)
    {
        ISPDB("Over run!! isp IRQ: 0x%x, reset camera path \n",g_u4MT6516ISPIRQ);
        g_u4MT6516ISPIRQ &= (~0x6); //clear
        MT6516ISP_OVERRUN_ErrHandle();
    }

    wake_up_interruptible(&g_MT6516ISPWQ);

    if(g_u4MT6516ISPIRQ & 0x1)
    {
        tasklet_schedule(&MT6516ISPTaskLet);
        strobe_StillExpEndIrqCbf();//stop capture flash
    }

    return IRQ_HANDLED;
}

// Called to probe if the device really exists. and create the semaphores
static int MT6516ISP_probe(struct platform_device *pdev)
{
    struct resource * pstRes = NULL;
    struct device* isp_device = NULL;

    ISPPARMDB("[mt6516_ISP] probing MT6516 ISP\n");

    //Check platform_device parameters
    if(NULL == pdev)
    {
        dev_err(&pdev->dev,"platform data is missed\n");

        return -ENXIO;
    }

    //register char driver
    //Allocate major no
    if(RegisterMT6516ISPCharDrv())
    {
        dev_err(&pdev->dev,"register char failed\n");

        return -EAGAIN;
    }

    //Mapping to IDP registers
    //For MT6516, there is no diff by calling request_region() or request_mem_region(),
    pstRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(NULL == pstRes)
    {
        dev_err(&pdev->dev,"[MT6516ISP] Get I/O mem failed\n");
    }

    pstRes = request_mem_region(pstRes->start, pstRes->end - pstRes->start + 1,pdev->name);

    if(NULL == pstRes)
    {
        dev_err(&pdev->dev,"[MT6516ISP] request I/O mem failed\n");
    }
//    No need to call it because currently(MT6516) IO mem physical address is aligned to virtual address.
//    ioremap_nocache(pstRes->start,pstRes->end - pstRes->start + 1);
//    TODO : ioremap_nocache

    pstRes = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if(NULL == pstRes)
    {
        dev_err(&pdev->dev,"[MT6516ISP] Get statistics I/O memory failed\n");

        return -ENOMEM;
    }

    pstRes = request_mem_region(pstRes->start, pstRes->end - pstRes->start + 1,pdev->name);
    if(NULL == pstRes)
    {
        dev_err(&pdev->dev,"[MT6516ISP] request statistics I/O mem failed\n");
    }
//    No need to call it because currently(MT6516) IO mem physical address is aligned to virtual address.
//    ioremap_nocache(pstRes->start,pstRes->end - pstRes->start + 1);
//    TODO : ioremap_nocache

    //Request IRQ
    g_MT6516ISPIRQNum = platform_get_irq(pdev, 0);

    if(request_irq(g_MT6516ISPIRQNum, MT6516ISP_irq, 0, pdev->name, NULL))
    {
        dev_err(&pdev->dev,"request IRQ failed\n");
    }
    //By default, disable it.
    disable_irq(g_MT6516ISPIRQNum);

    //Initialize waitqueue
    init_waitqueue_head(&g_MT6516ISPWQ);

    //Initialize critical section
    spin_lock_init(&g_MT6516ISPLock);

    //HW initialization
    iowrite32(1 , MT6516CAM_RESET);
    mdelay(1);//Delay at least 100us for reseting the ISP
    iowrite32(0 , MT6516CAM_RESET);

    isp_class = class_create(THIS_MODULE, "ispdrv");
    if (IS_ERR(isp_class)) {
        int ret = PTR_ERR(isp_class);
        ISPDB("Unable to create class, err = %d\n", ret);
        return ret;
    }
    isp_device = device_create(isp_class, NULL, g_MT6516ISPdevno, NULL, "mt6516-ISP");

    ISPPARMDB("[mt6516_ISP] probe MT6516 ISP success\n");

    return 0;
}

// Called when the device is being detached from the driver
static int MT6516ISP_remove(struct platform_device *pdev)
{
    struct resource * pstRes = NULL;
    int i4IRQ = 0;

    //unregister char driver.
    UnregisterMT6516ISPCharDrv();

    //unmaping ISP registers
    pstRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    release_mem_region(pstRes->start, (pstRes->end - pstRes->start + 1));

    //Release IRQ
    i4IRQ = platform_get_irq(pdev, 0);

    free_irq(i4IRQ , NULL);

    device_destroy(isp_class, g_MT6516ISPdevno);
    class_destroy(isp_class);

    //Turn off power

    ISPPARMDB("MT6516 ISP is removed\n");

    return 0;
}

// PM suspend
static int MT6516ISP_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    spin_lock_irq(&g_MT6516ISPLock);

    if(1 == g_MT6516ISPCLKSTATUS)
    {
        hwDisableClock(MT6516_PDN_MM_ISP,"ISP");
    }

    spin_unlock_irq(&g_MT6516ISPLock);

    return 0;
}

// PM resume
static int MT6516ISP_resume(struct platform_device *pdev)
{
    spin_lock_irq(&g_MT6516ISPLock);

    if(1 == g_MT6516ISPCLKSTATUS)
    {
        hwEnableClock(MT6516_PDN_MM_ISP,"ISP");
    }

    spin_unlock_irq(&g_MT6516ISPLock);

    return 0;
}

// platform structure
static struct platform_driver g_stMT6516ISP_Driver = {
    .probe		= MT6516ISP_probe,
    .remove	= MT6516ISP_remove,
    .suspend	= MT6516ISP_suspend,
    .resume	= MT6516ISP_resume,
    .driver		= {
        .name	= "mt6516-ISP",
        .owner	= THIS_MODULE,
    }
};

static int __init MT6516_ISP_Init(void)
{
	if(platform_driver_register(&g_stMT6516ISP_Driver)){
		ISPDB("failed to register MT6516 ISP driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit MT6516_ISP_Exit(void)
{
	platform_driver_unregister(&g_stMT6516ISP_Driver);
}

EXPORT_SYMBOL(MT6516ISP_EnableSensorClock); 
EXPORT_SYMBOL(MT6516ISP_DisableSensorClock);

module_init(MT6516_ISP_Init);
module_exit(MT6516_ISP_Exit);
MODULE_DESCRIPTION("MT6516 ISP driver");
MODULE_AUTHOR("Gipi <Gipi.Lin@Mediatek.com>");
MODULE_LICENSE("GPL");

