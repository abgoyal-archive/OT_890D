
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>

//Arch dependent files
#include <mach/mt6516_IDP.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_mm_mem.h>
#include <mach/irqs.h>

#include <asm/tcm.h>

#include "../../../drivers/video/mtk/disp_drv.h"

static dev_t g_MT6516IDPdevno = MKDEV(IDP_DEV_MAJOR_NUMBER,0);
static struct cdev * g_pMT6516IDP_CharDrv = 0;
static wait_queue_head_t g_WaitQueue;
static wait_queue_head_t g_DirectLinkWaitQueue;
static u32 g_u4IRQTbl = 0;
static u32 g_u4ResourceTbl = 0;
static u32 g_u4KeyMode;
static spinlock_t g_IDPLock;
static spinlock_t g_MT6516IDPOpenLock;
static struct class *idp_class = NULL;

extern int is_pmem_range(unsigned long *base, unsigned long size);

//#define MT6516IDP_EARLYSUSPEND
#ifdef MT6516IDP_EARLYSUSPEND
static unsigned int g_u4EarlySuspend = 0;
#endif

#define MT6516IDP_DEBUG
#ifdef MT6516IDP_DEBUG
#define IDPDB printk
#else
#define IDPDB(x,...)
#endif

//#define MT6516IDP_PARM
#ifdef MT6516IDP_PARM
#define IDPPARMDB printk
#else
#define IDPPARMDB(x,...)
#endif

#define SRAM_WKAROUND 0
#define FRAME_SYNC 1
#define DeviceNeedSramCnt 7
typedef struct {
    u32 u4PhysicalAddr;
    u32 u4Size;
} stSRAMMap;
static stSRAMMap g_stSRAMMap[DeviceNeedSramCnt];

typedef struct {
    u32 bAllocated;
    u32 * pYBuff;
    u32 * pUBuff;
    u32 * pVBuff;
    u32 u4PhyAddr;
} stPRZLineBuff;
static stPRZLineBuff g_stPRZLineBuff;

//For Direct link
typedef struct {
    unsigned long u4MT6516CRZ_CFG;
    unsigned long u4MT6516CRZ_SRCSZ1;
    unsigned long u4MT6516CRZ_TARSZ1;
    unsigned long u4MT6516CRZ_HRATIO1;
    unsigned long u4MT6516CRZ_VRATIO1;
    unsigned long u4MT6516CRZ_FRCFG;
    unsigned long u4MT6516CRZ_BUSY;
} MT6516_CRZ_REG;
typedef struct {
    unsigned long u4MT6516PRZ_CFG;
    unsigned long u4MT6516PRZ_SRCSZ1;
    unsigned long u4MT6516PRZ_TARSZ1;
    unsigned long u4MT6516PRZ_HRATIO1;
    unsigned long u4MT6516PRZ_VRATIO1;
    unsigned long u4MT6516PRZ_HRES1;
    unsigned long u4MT6516PRZ_VRES1;
    unsigned long u4MT6516PRZ_BLKCSCFG;
    unsigned long u4MT6516PRZ_YLMBASE;
    unsigned long u4MT6516PRZ_ULMBASE;
    unsigned long u4MT6516PRZ_VLMBASE;
    unsigned long u4MT6516PRZ_FRCFG;
    unsigned long u4MT6516PRZ_YLBSIZE;
    unsigned long u4MT6516PRZ_PRWMBASE;
} MT6516_PRZ_REG;
typedef struct {
    unsigned long u4MT6516IPP_CFG;
    unsigned long u4MT6516IPP_SDTCON;
    unsigned long u4MT6516IMPROC_HUE11;
    unsigned long u4MT6516IMPROC_HUE12;
    unsigned long u4MT6516IMPROC_HUE21;
    unsigned long u4MT6516IMPROC_HUE22;
    unsigned long u4MT6516IMPROC_SAT;
    unsigned long u4MT6516IMPROC_BRIADJ1;
    unsigned long u4MT6516IMPROC_BRIADJ2;
    unsigned long u4MT6516IMPROC_CONADJ;
    unsigned long u4MT6516IMPROC_COLORIZEU;
    unsigned long u4MT6516IMPROC_COLORIZEV;
//be noticed that Gamma information is not dumped, because it is seldom used
    unsigned long u4MT6516IMPROC_IO_MUX;// this register records R2Y0,Y2R1,IPP(0~6),must do bit operation
    unsigned long u4MT6516IMPROC_IPP_RGB_DETECT;
    unsigned long u4MT6516IMPROC_IPP_RGB_REPLACE;
} MT6516_IPP_REG;
typedef struct {
    unsigned long u4MT6516IMGDMA0_IRT0_MO_0_CON;
    unsigned long u4MT6516IMGDMA1_IRT0_MO_1_CON;
    unsigned long u4MT6516IMGDMA1_IRT0_CON;
    unsigned long u4MT6516IMGDMA1_IRT0_FIFO_BASE;
    unsigned long u4MT6516IMGDMA1_IRT0_FIFOYLEN;
    unsigned long u4MT6516IMGDMA1_IRT0_XSIZE;
    unsigned long u4MT6516IMGDMA1_IRT0_YSIZE;
} MT6516_IRT0_REG;
typedef struct {
    unsigned long u4MT6516IMGDMA0_IRT1_CON;
    unsigned long u4MT6516IMGDMA0_IRT1_ALPHA;
    unsigned long u4MT6516IMGDMA0_IRT1_SRC_XSIZE;
    unsigned long u4MT6516IMGDMA0_IRT1_SRC_YSIZE;
    unsigned long u4MT6516IMGDMA0_IRT1_BASE_ADDR0;
    unsigned long u4MT6516IMGDMA0_IRT1_BASE_ADDR1;
    unsigned long u4MT6516IMGDMA0_IRT1_BASE_ADDR2;
    unsigned long u4MT6516IMGDMA0_IRT1_BKGD_XSIZE0;
    unsigned long u4MT6516IMGDMA0_IRT1_BKGD_XSIZE1;
    unsigned long u4MT6516IMGDMA0_IRT1_BKGD_XSIZE2;
    unsigned long u4MT6516IMGDMA0_IRT1_FIFO_BASE;
} MT6516_IRT1_REG;
typedef struct {
    unsigned long u4MT6516IMGDMA0_IBW2_CON;
    unsigned long u4MT6516IMGDMA0_IBW2_ALPHA;
    unsigned long u4MT6516IMGDMA0_IBW2_XSIZE;
    unsigned long u4MT6516IMGDMA0_IBW2_YSIZE;
    unsigned long u4MT6516IMGDMA0_IBW2_CLIPLR;
    unsigned long u4MT6516IMGDMA0_IBW2_CLIPTB;
} MT6516_IBW2_REG;
typedef struct {
    unsigned long u4MT6516IMGDMA1_VDODEC_CON;
    unsigned long u4MT6516IMGDMA1_VDODEC_Y_BASE;
    unsigned long u4MT6516IMGDMA1_VDODEC_U_BASE;
    unsigned long u4MT6516IMGDMA1_VDODEC_V_BASE;
    unsigned long u4MT6516IMGDMA1_VDODEC_XSIZE;
    unsigned long u4MT6516IMGDMA1_VDODEC_YSIZE;
    unsigned long u4MT6516IMGDMA1_VDODEC_PXLNUM;
} MT6516_VDODECDMA_REG;
typedef struct {
    unsigned long u4MT6516MP4DEBLK_CONF;
    unsigned long u4MT6516MP4DEBLK_LIN_BUFF_ADDR;
    unsigned long u4MT6516MP4DEBLK_QUANT_ADDR;
} MT6516_MP4DBLK_REG;
typedef struct {
MT6516_CRZ_REG CRZReg;
MT6516_PRZ_REG PRZReg;
MT6516_IPP_REG IPPReg;
MT6516_IRT0_REG IRT0Reg;
MT6516_IRT1_REG IRT1Reg;
MT6516_IBW2_REG IBW2Reg;
MT6516_VDODECDMA_REG VDODECDMAReg;
MT6516_MP4DBLK_REG MP4DBLKReg;
} MT6516_IDP_REG;
//

//Buffer indicator
///////
//Temperary solution to ECO1 phones
///////
#if 0
typedef struct {
    unsigned long u4Sec;
    unsigned long u4USec;
    unsigned long u4TimeMark;
} stIDPLOGPAGE;
typedef struct {
    unsigned long u4LogDepth;
    unsigned long u4Counter;
    unsigned long u4MaxTimeMark;
    stIDPLOGPAGE * pPage;
} stIDPLOG;
typedef enum {
    ALLOC_IDPTIMEPROFILEBUFF = 0,
    FREE_IDPTIMEPROFILEBUFF,
    PUT_IDPTIMEPROFILE,
    PRINT_IDPTIMEPROFILE,
    RESET_IDPTIMEPROFILE,
} IDPLOG_CMD;
void IDPLOG(IDPLOG_CMD eCMD , unsigned long a_u4Val)
{
    static stIDPLOG Log = {0,0,0,NULL};
    struct timeval tv;
    unsigned long u4Index,u4Index2;

    switch(eCMD)
    {
        case RESET_IDPTIMEPROFILE :
            Log.u4Counter = 0;
            Log.u4MaxTimeMark = 0;
        break;
        case ALLOC_IDPTIMEPROFILEBUFF :
            if(NULL != Log.pPage){return;}
            Log.u4LogDepth = a_u4Val;
            Log.u4Counter = 0;
            Log.u4MaxTimeMark = 0;
            Log.pPage = (stIDPLOGPAGE *)kmalloc(a_u4Val*sizeof(stIDPLOGPAGE) , GFP_ATOMIC);
            if(NULL == Log.pPage){printk("Not enough memory for IDP LOG\n");}
        break;
        case FREE_IDPTIMEPROFILEBUFF :
            if(NULL == Log.pPage){return;}
            kfree(Log.pPage);
            Log.pPage = NULL;
            Log.u4Counter = 0;
            Log.u4LogDepth = 0;
            Log.u4MaxTimeMark = 0;
        break;
        case PUT_IDPTIMEPROFILE:
            if(NULL == Log.pPage){return;}
            if(Log.u4Counter >= Log.u4LogDepth){return;}
            do_gettimeofday(&tv);
            Log.pPage[Log.u4Counter].u4Sec = tv.tv_sec;
            Log.pPage[Log.u4Counter].u4USec = tv.tv_usec;
            Log.pPage[Log.u4Counter].u4TimeMark = a_u4Val;
            if(a_u4Val > Log.u4MaxTimeMark){Log.u4MaxTimeMark = a_u4Val;}
            Log.u4Counter += 1;
        break;
        case PRINT_IDPTIMEPROFILE:
            if(NULL == Log.pPage){printk("Allocate IDP Log buffer first!!\n");return;}
            printk("\nIDP driver time profile\n");
            printk("Sec,USec,Mark\n");
            for(u4Index2 = 0; u4Index2 < (Log.u4MaxTimeMark + 1) ; u4Index2 += 1)
            {
                printk("\nTime Mark : %lu\n",u4Index2);
                for(u4Index = 0; u4Index < Log.u4LogDepth;u4Index += 1)
                {
                    if(Log.pPage[u4Index].u4TimeMark == u4Index2)
                    {
                        printk("%lu,%lu,%lu\n" , Log.pPage[u4Index].u4Sec , Log.pPage[u4Index].u4USec , Log.pPage[u4Index].u4TimeMark);
                    }
                }
            }
        break;
        default :
        break;
    }
}
#endif
typedef enum {
    SET_BUFF = 0,
    SET_YBUFF,
    SET_UBUFF,
    SET_VBUFF,
    RESET_BUFF,
    UPDATE_BLK,
    QUERY_BUFF,
    SWITCH_BUFF,
    GET_TIMES,
    GET_TIMEUS,
} BUFFIND_CMD;
#define MT6516IMGDMA1_VDOENC_Y_BASE1 (IMGDMA1_BASE + 0x210)
#define MT6516IMGDMA1_VDOENC_U_BASE1 (IMGDMA1_BASE + 0x214)
#define MT6516IMGDMA1_VDOENC_V_BASE1 (IMGDMA1_BASE + 0x218)
#define MT6516IMGDMA1_VDOENC_Y_BASE2 (IMGDMA1_BASE + 0x220)
#define MT6516IMGDMA1_VDOENC_U_BASE2 (IMGDMA1_BASE + 0x224)
#define MT6516IMGDMA1_VDOENC_V_BASE2 (IMGDMA1_BASE + 0x228)
#define MT6516IMGDMA1_VDOENC_WXCNT (IMGDMA1_BASE + 0x240)
#define NEXT_FRAME(a,b) ((a + 1) >= b ? 0 : (a + 1))
#define PRV_FRAME(a,b) ((a == 0) ? (b-1) : (a - 1))
static void VDOENCDMA_BUFFIND(BUFFIND_CMD eCMD, unsigned long * a_pu4BufferPtr , unsigned long a_u4Buffer)
{
    //Todo : allocate from heap instead of statistic allocate, 400 bytes now
    static unsigned long u4BuffCnt = 0;
    static unsigned long pu4YBuffArray[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
    static unsigned long pu4UBuffArray[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
    static unsigned long pu4VBuffArray[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
    static unsigned long pu4TSec[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
    static unsigned long pu4TuSec[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];

    static unsigned long u4CurrentBuff = 0;
    static unsigned long u4PingPong = 0;
    static unsigned long u4StallPingPong = 0;//0 : normal, 1 : might stall, 2 : stalled
    static unsigned long u4BlockBuff = 0;

    static unsigned long pu4Hist[3];
    static unsigned long u4HistInx = 2;

    unsigned long u4FutureBuffer = 0;
    static struct timeval ktv;

    stWaitBuff * pBuff;

//Test
//static unsigned long AB[2];
    switch(eCMD)
    {
        case SET_YBUFF :
            spin_lock_irq(&g_IDPLock);
            u4BuffCnt = a_u4Buffer;
            memcpy(pu4YBuffArray,a_pu4BufferPtr,u4BuffCnt*sizeof(unsigned long));
            u4CurrentBuff = 0;
            u4PingPong = 0;
            u4StallPingPong = 0;
            u4BlockBuff = 0;
            pu4Hist[0] = 0;
            pu4Hist[1] = 1;
            pu4Hist[2] = 2;
            u4HistInx = 2;
            spin_unlock_irq(&g_IDPLock);
        break;

        case SET_UBUFF :
            spin_lock_irq(&g_IDPLock);
            memcpy(pu4UBuffArray,a_pu4BufferPtr,u4BuffCnt*sizeof(unsigned long));
            spin_unlock_irq(&g_IDPLock);
        break;

        case SET_VBUFF :
            spin_lock_irq(&g_IDPLock);
            memcpy(pu4VBuffArray,a_pu4BufferPtr,u4BuffCnt*sizeof(unsigned long));
            spin_unlock_irq(&g_IDPLock);
        break;

        case RESET_BUFF :
            spin_lock_irq(&g_IDPLock);
            u4CurrentBuff = 0;
            u4PingPong = 0;
            u4StallPingPong = 0;
            u4BlockBuff = 0;
            pu4Hist[0] = 0;
            pu4Hist[1] = 1;
            pu4Hist[2] = 2;
            u4HistInx = 2;
            iowrite32(pu4YBuffArray[0],MT6516IMGDMA1_VDOENC_Y_BASE1);
            iowrite32(pu4UBuffArray[0],MT6516IMGDMA1_VDOENC_U_BASE1);
            iowrite32(pu4VBuffArray[0],MT6516IMGDMA1_VDOENC_V_BASE1);
            if(u4BuffCnt > 1)
            {
                iowrite32(pu4YBuffArray[1],MT6516IMGDMA1_VDOENC_Y_BASE2);
                iowrite32(pu4UBuffArray[1],MT6516IMGDMA1_VDOENC_U_BASE2);
                iowrite32(pu4VBuffArray[1],MT6516IMGDMA1_VDOENC_V_BASE2);
            }
            else
            {
                iowrite32(pu4YBuffArray[0],MT6516IMGDMA1_VDOENC_Y_BASE2);
                iowrite32(pu4UBuffArray[0],MT6516IMGDMA1_VDOENC_U_BASE2);
                iowrite32(pu4VBuffArray[0],MT6516IMGDMA1_VDOENC_V_BASE2);
            }
            spin_unlock_irq(&g_IDPLock);
        break;

        case UPDATE_BLK:
            spin_lock_irq(&g_IDPLock);
            if(a_u4Buffer >= u4BuffCnt){IDPDB("Incorrect buffer no!!\n");break;}
            u4BlockBuff = a_u4Buffer;

            if(((u4CurrentBuff + 2) == u4BlockBuff) || ((u4CurrentBuff + 2) == (u4BlockBuff + u4BuffCnt)))
            {
                //Stalled by video buffer
                u4StallPingPong = NEXT_FRAME(u4StallPingPong,2);
                if(u4StallPingPong){u4FutureBuffer = NEXT_FRAME(u4CurrentBuff,u4BuffCnt);}
                else{u4FutureBuffer = u4CurrentBuff;}
            }
            else
            {
                u4FutureBuffer = NEXT_FRAME(u4CurrentBuff,u4BuffCnt);
                if(u4StallPingPong){u4FutureBuffer = NEXT_FRAME(u4FutureBuffer,u4BuffCnt);}
            }

            //Fill address
            if(u4PingPong)
            {
                iowrite32(pu4YBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_Y_BASE1);
                iowrite32(pu4UBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_U_BASE1);
                iowrite32(pu4VBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_V_BASE1);
            }
            else
            {
                iowrite32(pu4YBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_Y_BASE2);
                iowrite32(pu4UBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_U_BASE2);
                iowrite32(pu4VBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_V_BASE2);
            }
            pu4Hist[PRV_FRAME(u4HistInx,3)] = u4FutureBuffer;
            //Time stamp
            u4FutureBuffer = PRV_FRAME(u4HistInx,3);
            u4FutureBuffer = pu4Hist[PRV_FRAME(u4FutureBuffer,3)];
            pu4TSec[u4FutureBuffer] = ktv.tv_sec;
            pu4TuSec[u4FutureBuffer] = ktv.tv_usec;

            spin_unlock_irq(&g_IDPLock);
        break;

        case QUERY_BUFF:
            spin_lock_irq(&g_IDPLock);

            pBuff = (stWaitBuff *)a_pu4BufferPtr;

            //calculate available buffer count.
            u4FutureBuffer = (u4CurrentBuff >= u4BlockBuff ? (u4CurrentBuff - u4BlockBuff) : (u4CurrentBuff + u4BuffCnt - u4BlockBuff));

            if(u4FutureBuffer >= (u4BuffCnt - 2))
            {
                //Stalled
                pBuff->u4VDODMAReadyBuffCount = (u4BuffCnt - 2);
            }
            else
            {
                //Normal
                pBuff->u4VDODMAReadyBuffCount = u4FutureBuffer;
            }

            pBuff->u4VDODMADispBuffNo = pu4Hist[u4HistInx];

            spin_unlock_irq(&g_IDPLock);
        break;

        case SWITCH_BUFF :
            do_gettimeofday(&ktv);

            //Add ping-pong counter
            //u4PingPong = (u4PingPong > 0 ? 0 : 1);
            u4PingPong = (ioread32(MT6516IMGDMA1_VDOENC_WXCNT) & 0x1);

            //Determin next frame to write
            if(((u4CurrentBuff + 2) == u4BlockBuff) || ((u4CurrentBuff + 2) == (u4BlockBuff + u4BuffCnt)))
            {
                //Stalled by video buffer
//                u4StallPingPong = NEXT_FRAME(u4StallPingPong,2);
                if(u4StallPingPong){u4FutureBuffer = NEXT_FRAME(u4CurrentBuff,u4BuffCnt);}
                else{u4FutureBuffer = u4CurrentBuff;}
            }
            else
            {
                //Normal condition
                if(u4StallPingPong){u4StallPingPong = 0;}
                else{u4CurrentBuff = NEXT_FRAME(u4CurrentBuff,u4BuffCnt);}
                u4FutureBuffer = NEXT_FRAME(u4CurrentBuff,u4BuffCnt);
            }

            //Fill address
            if(u4PingPong)
            {
                iowrite32(pu4YBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_Y_BASE1);
                iowrite32(pu4UBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_U_BASE1);
                iowrite32(pu4VBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_V_BASE1);
            }
            else
            {
                iowrite32(pu4YBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_Y_BASE2);
                iowrite32(pu4UBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_U_BASE2);
                iowrite32(pu4VBuffArray[u4FutureBuffer],MT6516IMGDMA1_VDOENC_V_BASE2);
            }

            pu4Hist[u4HistInx] = u4FutureBuffer;

            //Fill time stamp
            u4FutureBuffer = pu4Hist[PRV_FRAME(u4HistInx,3)];
            pu4TSec[u4FutureBuffer] = ktv.tv_sec;
            pu4TuSec[u4FutureBuffer] = ktv.tv_usec;

            u4HistInx = NEXT_FRAME(u4HistInx,3);
        break;

        case GET_TIMES :
            spin_lock_irq(&g_IDPLock);
            memcpy(a_pu4BufferPtr , pu4TSec , MT6516IDP_VDOENCDMA_MAXBUFF_CNT*sizeof(unsigned long));
            spin_unlock_irq(&g_IDPLock);
        break;

        case GET_TIMEUS :
            spin_lock_irq(&g_IDPLock);
            memcpy(a_pu4BufferPtr , pu4TuSec , MT6516IDP_VDOENCDMA_MAXBUFF_CNT*sizeof(unsigned long));
            spin_unlock_irq(&g_IDPLock);        
        break;

        default :
        break;
    }

}

static void IRT1_BUFFIND(BUFFIND_CMD eCMD, unsigned long * a_pu4BufferPtr , unsigned long a_u4Buffer )
{
    static unsigned long u4BuffCnt = 0;
    static unsigned long u4CurrentBuff = 0;
    switch(eCMD)
    {
        case SET_BUFF :
            u4BuffCnt = a_u4Buffer;
            u4CurrentBuff = 0;
        break;

        case RESET_BUFF :
            u4CurrentBuff = 0;
        break;

        case QUERY_BUFF :
            if(u4CurrentBuff > 0){ *a_pu4BufferPtr = (u4CurrentBuff - 1);}
            else{*a_pu4BufferPtr = (u4BuffCnt - 1);}
        break;

        case SWITCH_BUFF :
            u4CurrentBuff += 1;
            if(u4CurrentBuff >=  u4BuffCnt){u4CurrentBuff = 0;}
	break;

        default :
        break;
    }
        
}

typedef enum {
    DBUFF_ALLOC = 0,
    DBUFF_GET, // Get double buffer pointer
    DBUFF_SET, // set to registers
    DBUFF_FREE //free mem
} DBUFF_CMD;
typedef struct {
    unsigned long u4ISPRWINV;
    unsigned long u4ISPRWINH;
    unsigned long u4CRZCFG;
    unsigned long u4CRZSRCSZ1;
    unsigned long u4CRZTARSZ1;
    unsigned long u4CRZHRATIO1;
    unsigned long u4CRZVRATIO1;
    unsigned long u4Dirty;
}stCRZDBuff;
#define MT6516CRZ_CFG (CRZ_BASE)
#define MT6516CRZ_SRCSZ1 (CRZ_BASE + 0x10)
#define MT6516CRZ_TARSZ1 (CRZ_BASE + 0x14)
#define MT6516CRZ_HRATIO1 (CRZ_BASE + 0x18)
#define MT6516CRZ_VRATIO1 (CRZ_BASE + 0x1C)
#define MT6516CAM_RWINV_SEL (CAM_BASE + 0x174)
#define MT6516CAM_RWINH_SEL (CAM_BASE + 0x178)
static void CRZDBuff(DBUFF_CMD eCMD , stCRZDBuff ** a_ppstBuff)
{
    static stCRZDBuff * Buff = NULL;
    switch(eCMD)
    {
        case DBUFF_ALLOC :
//IDPDB("[MT6516IDP]Allocate buffer!!\n");
            if(NULL != Buff){IDPDB("[MT6516IDP] CRZ double buffer already exist!\n");return;}
            Buff = kmalloc(sizeof(stCRZDBuff),GFP_ATOMIC);
            if(NULL == Buff){IDPDB("[MT6516IDP] Not enough memory for CRZ double buffer!\n");return;}
            memset(Buff,0,sizeof(stCRZDBuff));
        break;
        case DBUFF_GET :
//IDPDB("[MT6516IDP]Get buffer!!\n");
            *a_ppstBuff = Buff;
        break;
        case DBUFF_SET :
            if(NULL == Buff){return;}
            if(0 == Buff->u4Dirty){return;}
//IDPDB("[MT6516IDP]Set!!\n");
            iowrite32(Buff->u4ISPRWINV , MT6516CAM_RWINV_SEL);
            iowrite32(Buff->u4ISPRWINH , MT6516CAM_RWINH_SEL);
            iowrite32(Buff->u4CRZCFG , MT6516CRZ_CFG);
            iowrite32(Buff->u4CRZSRCSZ1 , MT6516CRZ_SRCSZ1);
            iowrite32(Buff->u4CRZTARSZ1 , MT6516CRZ_TARSZ1);
            iowrite32(Buff->u4CRZHRATIO1 , MT6516CRZ_HRATIO1);
            iowrite32(Buff->u4CRZVRATIO1 , MT6516CRZ_VRATIO1);
//printk("CRZresultV:0x%x,0x%x\n",ioread32(MT6516CAM_RWINV_SEL),ioread32(MT6516CAM_RWINH_SEL));
            Buff->u4Dirty = 0;
        break;
        case DBUFF_FREE :
//IDPDB("[MT6516IDP]Free!!\n");
            if(NULL == Buff){return;}
            kfree(Buff);
            Buff = NULL;
        break;
        default :
        break;
    }
}

typedef enum {
    MT6516_ALLOC = 0, //probe
    MT6516_FREE, //remove
    MT6516_ADD, //called on Open
    MT6516_REMOVE, //flush
    MT6516_ERR//Error handling
} MT6516EHandleCMD;
typedef struct {
    unsigned long u4ID;//A given ID
    pid_t PID;//PID
    unsigned long u4Table;//Resource table
    unsigned long u4ScenarioTbl;
}stMT6516ResTbl;
#define MT6516RESTBLCNT 20
static void MT6516_DisableChainInternal(eMT6516IDP_NODES * peChain , unsigned long a_u4Cnt );
inline static int MT6516IDP_UnLockResource(unsigned long a_u4Tag);
static int MT6516IDP_UnlockMode(unsigned int a_u4Tag);
inline void MT6516_TblToChain(unsigned long a_u4Tbl , eMT6516IDP_NODES * a_eChain , unsigned long * a_Cnt)
{
    *a_Cnt = 0;
    //Desti
    if(a_u4Tbl & MT6516IDP_RES_VDOENCDMA){a_eChain[*a_Cnt] = IDP_VDOENCDMA;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_JPEGDMA){a_eChain[*a_Cnt] = IDP_JPEGDMA;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_IRT0){a_eChain[*a_Cnt] = IDP_IRT0;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_IRT1){a_eChain[*a_Cnt] = IDP_IRT1;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_IRT3){a_eChain[*a_Cnt] = IDP_IRT3;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_IBW1){a_eChain[*a_Cnt] = IDP_IBW1;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_IBW2){a_eChain[*a_Cnt] = IDP_IBW2;*a_Cnt += 1;}

    if(a_u4Tbl & MT6516IDP_RES_IPP){a_eChain[*a_Cnt] = IDP_IPP1_Y2R0_IPP2;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_R2Y0){a_eChain[*a_Cnt] = IDP_R2Y0;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_Y2R1){a_eChain[*a_Cnt] = IDP_Y2R1;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_OVL){a_eChain[*a_Cnt] = IDP_OVL;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_MP4DBLK){a_eChain[*a_Cnt] = IDP_MP4DBLK;*a_Cnt += 1;}

    //Source
    if(a_u4Tbl & MT6516IDP_RES_CRZ){a_eChain[*a_Cnt] = IDP_CRZ;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_PRZ){a_eChain[*a_Cnt] = IDP_PRZ;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_DRZ){a_eChain[*a_Cnt] = IDP_DRZ;*a_Cnt += 1;}
    if(a_u4Tbl & MT6516IDP_RES_IBR1){a_eChain[*a_Cnt] = IDP_IBR1;*a_Cnt += 1;}    
    if(a_u4Tbl & MT6516IDP_RES_VDODECDMA){a_eChain[*a_Cnt] = IDP_VDODECDMA;*a_Cnt += 1;}

}
int MT6516IDPResManager(MT6516EHandleCMD a_eCMD , unsigned long a_u4ID , void ** a_pPtr)
{
    static stMT6516ResTbl * pResTbl = NULL;
    unsigned long u4Index = 0;
    eMT6516IDP_NODES peChain[20];
    int retVal = 0;
    
    switch(a_eCMD)
    {
        case MT6516_ALLOC :
            pResTbl = kmalloc(MT6516RESTBLCNT*sizeof(stMT6516ResTbl),GFP_ATOMIC);
            if(NULL == pResTbl){IDPDB("Not enough memory for IDP resource manager\n");while(1){;}}
            memset(pResTbl,0,(MT6516RESTBLCNT*sizeof(stMT6516ResTbl)));
        break;
        case MT6516_FREE :
      	     kfree(pResTbl);
        break;
        case MT6516_ADD :
            if(NULL == pResTbl){IDPDB("NULL pointer!!!\n");while(1){;}}

            spin_lock_irq(&g_MT6516IDPOpenLock);
            retVal = -1;
            for(u4Index = 0;u4Index < MT6516RESTBLCNT;u4Index += 1)
            {
                if(0 == pResTbl[u4Index].PID)
                {
                    pResTbl[u4Index].u4ID = u4Index;
                    pResTbl[u4Index].PID = current->pid;
                    pResTbl[u4Index].u4Table = 0;
                    pResTbl[u4Index].u4ScenarioTbl = 0;
                    *a_pPtr = &pResTbl[u4Index];
                    retVal = 0;
                    break;
                }
            }

            if(u4Index >= MT6516RESTBLCNT)
            {
                IDPDB("Not enough entries for IDP resource\n");
            }

            spin_unlock_irq(&g_MT6516IDPOpenLock);

        break;
        case MT6516_ERR :
            if(NULL == pResTbl){IDPDB("NULL pointer!!!\n");while(1){;}}
IDPDB("\n\n[MT6516IDP]Error handle!!!\n");
IDPDB("PID:%lu,ID:%lu,Scn:0x%x,Res:0x%x\n",
	pResTbl[a_u4ID].PID,pResTbl[a_u4ID].u4ID,pResTbl[a_u4ID].u4ScenarioTbl,pResTbl[a_u4ID].u4Table);

for(u4Index = 0;u4Index < MT6516RESTBLCNT;u4Index += 1)
{
printk("ID:%lu,PID:%lu,Tbl:0x%x,Scen:0x%x\n",pResTbl[u4Index].u4ID,pResTbl[u4Index].PID,pResTbl[u4Index].u4Table,pResTbl[u4Index].u4ScenarioTbl);
}

            //Disable hardware
            MT6516_TblToChain(pResTbl[a_u4ID].u4Table , peChain , &u4Index);
            MT6516_DisableChainInternal(peChain , u4Index);
            //Unlock resource
            MT6516IDP_UnLockResource(pResTbl[a_u4ID].u4Table);
            MT6516IDP_UnlockMode(pResTbl[a_u4ID].u4ScenarioTbl);

        case MT6516_REMOVE :
            if(NULL == pResTbl){IDPDB("NULL pointer!!!\n");while(1){;}}
            if(pResTbl[a_u4ID].u4Table || pResTbl[a_u4ID].u4ScenarioTbl)
            {
                printk("Fetal error!!PID:%d should release resource before close!!\n",pResTbl[a_u4ID].PID);
                MT6516IDP_UnLockResource(pResTbl[a_u4ID].u4Table);
                MT6516IDP_UnlockMode(pResTbl[a_u4ID].u4ScenarioTbl);
            }
            pResTbl[a_u4ID].PID = 0;
        break;
        default :
        break;
    }
    return retVal;
}

//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
// 3.Update f_op pointer.
// 4.Fill data structures into private_data
// Q1 : Try open multiple times.
#define MT6516IMGDMA1_JPEG_CON (IMGDMA1_BASE + 0x104)
#define MT6516IMGDMA0_IBW2_CON (IMGDMA_BASE + 0x404)
//#define MT6516CRZ_CFG (CRZ_BASE)
static int MT6516_IDP_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    MT6516IDPResManager(MT6516_ADD , 0 , &a_pstFile->private_data);

    hwEnableClock(MT6516_PDN_MM_GMC1,"IDP");
    hwEnableClock(MT6516_PDN_MM_GMC2,"IDP");

    if(NULL == a_pstFile->private_data)
    {
        IDPDB("Not enough entry for IDP open operation\n");
        return -ENOMEM;
    }

    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int MT6516_IDP_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    hwDisableClock(MT6516_PDN_MM_GMC1,"IDP");
    hwDisableClock(MT6516_PDN_MM_GMC2,"IDP");
    return 0;
}

static int MT6516_IDP_Flush(struct file * a_pstFile , fl_owner_t a_id)
{
    stMT6516ResTbl * pTbl = (stMT6516ResTbl *)a_pstFile->private_data;

    if(pTbl->u4Table || pTbl->u4ScenarioTbl)
    {
        MT6516IDPResManager(MT6516_ERR , pTbl->u4ID , NULL);
    }
    else
    {
        MT6516IDPResManager(MT6516_REMOVE , pTbl->u4ID , NULL);
    }
    
    return 0;
}

//TODO : Check the lock algorithm!!
inline static int MT6516IDP_LockResource(unsigned long a_u4Tag)
{
    spin_lock_irq(&g_IDPLock);

    //To add owner information to resource table, to avoid race condition inside allocated resources.
    if((~g_u4ResourceTbl) &  a_u4Tag)
    {
//    printk("Res:0x%x,Acq:0x%x\n",g_u4ResourceTbl,a_u4Tag);
        spin_unlock_irq(&g_IDPLock);

        return -EBUSY;
    }
    else
    {
        g_u4ResourceTbl &= (~a_u4Tag);
    }

    if(a_u4Tag & MT6516IDP_RES_CRZ)
    {
        CRZDBuff(DBUFF_ALLOC,NULL);
    }

    spin_unlock_irq(&g_IDPLock);

    return 0;
}


inline static int MT6516IDP_UnLockResource(unsigned long a_u4Tag)
{

    spin_lock_irq(&g_IDPLock);

    g_u4ResourceTbl |= a_u4Tag;

    if(a_u4Tag & MT6516IDP_RES_CRZ)
    {
        CRZDBuff(DBUFF_FREE,NULL);
    }

    wake_up_interruptible(&g_DirectLinkWaitQueue);

    spin_unlock_irq(&g_IDPLock);

    //Free all SRAM allocated
    if((0 != g_stSRAMMap[0].u4PhysicalAddr) && (a_u4Tag & MT6516IDP_RES_PRZ))
    {
        free_internal_sram(INTERNAL_SRAM_IDP_PRZ , g_stSRAMMap[0].u4PhysicalAddr);
        g_stSRAMMap[0].u4PhysicalAddr = 0;
        g_stSRAMMap[0].u4Size = 0;
    }

    if(g_stPRZLineBuff.bAllocated)
    {
        //kfree(g_stPRZLineBuff.pYBuff);
        //kfree(g_stPRZLineBuff.pUBuff);
        //kfree(g_stPRZLineBuff.pVBuff);
        free_internal_sram(INTERNAL_SRAM_IDP_PRZ_MCU , g_stPRZLineBuff.u4PhyAddr);
        g_stPRZLineBuff.bAllocated = 0;
    }

    if((0 != g_stSRAMMap[1].u4PhysicalAddr) && (a_u4Tag & MT6516IDP_RES_IRT0))
    {
        free_internal_sram(INTERNAL_SRAM_IDP_IRT0 , g_stSRAMMap[1].u4PhysicalAddr);
        g_stSRAMMap[1].u4PhysicalAddr = 0;
        g_stSRAMMap[1].u4Size = 0;
    }

    if((0 != g_stSRAMMap[2].u4PhysicalAddr) && (a_u4Tag & MT6516IDP_RES_IRT1))
    {
        free_internal_sram(INTERNAL_SRAM_IDP_IRT1 , g_stSRAMMap[2].u4PhysicalAddr);
        g_stSRAMMap[2].u4PhysicalAddr = 0;
        g_stSRAMMap[2].u4Size = 0;
    }

    if((0 != g_stSRAMMap[3].u4PhysicalAddr) && (a_u4Tag & MT6516IDP_RES_IRT3))
    {
        free_internal_sram(INTERNAL_SRAM_IDP_IRT3 , g_stSRAMMap[3].u4PhysicalAddr);
        g_stSRAMMap[3].u4PhysicalAddr = 0;
        g_stSRAMMap[3].u4Size = 0;        
    }

    if((0 != g_stSRAMMap[4].u4PhysicalAddr) && (a_u4Tag & MT6516IDP_RES_JPEGDMA))
    {
        free_internal_sram(INTERNAL_SRAM_IDP_JPEG_DMA , g_stSRAMMap[4].u4PhysicalAddr);
        g_stSRAMMap[4].u4PhysicalAddr = 0;
        g_stSRAMMap[4].u4Size = 0;
    }

    if((0 != g_stSRAMMap[5].u4PhysicalAddr) && (a_u4Tag & MT6516IDP_RES_VDOENCDMA))
    {
        free_internal_sram(INTERNAL_SRAM_IDP_VDO_ENC_WDMA , g_stSRAMMap[5].u4PhysicalAddr);
        g_stSRAMMap[5].u4PhysicalAddr = 0;
        g_stSRAMMap[5].u4Size = 0;
    }

    if((0 != g_stSRAMMap[6].u4PhysicalAddr) && (a_u4Tag & MT6516IDP_RES_MP4DBLK))
    {
        free_internal_sram(INTERNAL_SRAM_IDP_MP4DEBLK , g_stSRAMMap[6].u4PhysicalAddr);
        g_stSRAMMap[6].u4PhysicalAddr = 0;
        g_stSRAMMap[6].u4Size = 0;
    }

    return 0;
}

static int MT6516IDP_LockMode(unsigned long a_modeParm , unsigned long * a_pu4CurrProcScn)
{
    stModeParam modeParam;
    int i4RetValue = 0;
//    int i;
          
    if(copy_from_user(&modeParam,(stModeParam *)a_modeParm,sizeof(stModeParam)))
    {
        IDPDB("[MT6516_IDP] Copy from user failed!\n");
        return -EFAULT;
    }    
    
    spin_lock_irq(&g_IDPLock);    
    if((g_u4KeyMode & modeParam.u4Mode) == 0)
    {
        g_u4KeyMode |= (modeParam.u4Mode & LOCK_MODE_MASK);
        *a_pu4CurrProcScn |= (modeParam.u4Mode & LOCK_MODE_MASK);
        g_u4KeyMode &= ~modeParam.u4WaitFlag;
        *a_pu4CurrProcScn &= ~modeParam.u4WaitFlag;
    }
    else
    {
        i4RetValue = -EBUSY;

        if(modeParam.u4WaitFlag > 0)
        {
            g_u4KeyMode |= modeParam.u4WaitFlag;
            *a_pu4CurrProcScn |= modeParam.u4WaitFlag;
        }
        
        if(copy_to_user((void *)(modeParam.pNowMode) , &g_u4KeyMode , sizeof(unsigned int)))
        {
            IDPDB("[MT6516IDP] Lock mode copy to user failed \n");
            i4RetValue = -EFAULT;
        }   
    }

    spin_unlock_irq(&g_IDPLock);

    return i4RetValue;
}

static int MT6516IDP_UnlockMode(unsigned int a_u4Tag)
{
    int i4RetValue = 0;
       
    spin_lock_irq(&g_IDPLock);

    if(((g_u4KeyMode & a_u4Tag) & 0xFF) == 0)
    {
        if((a_u4Tag & 0xF00000) != 0)
        {
            g_u4KeyMode &= ~(a_u4Tag & 0xF00000);
        }
        else
        {
            i4RetValue = -EBUSY;
        }
    }
    else
    {
        g_u4KeyMode &= (~a_u4Tag);
    }
    spin_unlock_irq(&g_IDPLock);

    return i4RetValue;
}


static inline int MT6516IDP_CheckCRZDimension(u32 a_u4Src, u32 a_u4Dest, u32 * a_pu4Ratio)
{
    //must larger than 3.
    if((a_u4Src < 3) || (a_u4Dest < 3)){
        IDPDB("[MT6516_IDP] CRZ input must larger than 3\n");
        return -EINVAL;
    }

    if(a_u4Src > a_u4Dest){
    //Down scale
        if((a_u4Src < 4095) && (a_u4Dest < 2688) && (((a_u4Dest -1) <<11) >= (a_u4Src - 1))){
            (*a_pu4Ratio) = (((a_u4Dest-1)<<20) + ((a_u4Src-1)>>1))/(a_u4Src-1);
        }
        else{
            IDPDB("[MT6516_IDP] CRZ incorrect dimension\n");
            return -EINVAL;
        }
    }
    else if(a_u4Src < a_u4Dest){
    //Up scale
        if((a_u4Dest < 4095) && (a_u4Src < 2688) && ((a_u4Dest -1) <= (a_u4Src - 1)<<6)){
            (*a_pu4Ratio) = (((a_u4Src-1)<<20) + ((a_u4Dest-1)>>1))/(a_u4Dest-1);
        }
        else{
            IDPDB("[MT6516_IDP] CRZ incorrect dimension\n");
            return -EINVAL;
        }
    }
    else{
    //By pass
        if(a_u4Dest < 4095){
            (*a_pu4Ratio) = (((a_u4Src-1)<<20) + ((a_u4Dest-1)>>1))/(a_u4Dest-1);
        }
        else{
            IDPDB("[MT6516_IDP] CRZ incorrect dimension \n");
            return -EINVAL;
        }
    }
    return 0;
}

//Registers
//#define MT6516CRZ_CFG (CRZ_BASE)
#define MT6516CRZ_CON (CRZ_BASE + 0x4)
#define MT6516CRZ_STA (CRZ_BASE + 0x8)
#define MT6516CRZ_INT (CRZ_BASE + 0xC)
//#define MT6516CRZ_SRCSZ1 (CRZ_BASE + 0x10)
//#define MT6516CRZ_TARSZ1 (CRZ_BASE + 0x14)
//#define MT6516CRZ_HRATIO1 (CRZ_BASE + 0x18)
//#define MT6516CRZ_VRATIO1 (CRZ_BASE + 0x1C)
#define MT6516CRZ_FRCFG (CRZ_BASE + 0x40)
#define MT6516CRZ_BUSY (CRZ_BASE + 0x44)
//ISP registers
#define MT6516CAM_GRABCOL (CAM_BASE + 0x8)
#define MT6516CAM_GRABROW (CAM_BASE + 0xC)
#define MT6516CAM_PATH (CAM_BASE + 0x24)
#define MT6516CAM_INPUT_SEL(x) ((x & 0x00000700) >> 8)
#define MT6516GMC1 (GMC1_BASE)
#define MT6516GMC2 (GMC2_BASE)
//TODO : parameter check should be done in user space, to save copy_from_user.
static int MT6516IDP_ConfigCRZ(void * a_pCfg)
{
    stCRZCfg CRZCfg;
    u32 u4HRatio = 0, u4VRatio = 0;
    u32 u4LineBuff = 0;
    u32 u4RegValue = 0;
    u32 u4InputFmt = 0; 
    stCRZDBuff * pDBuff = NULL;

    if(copy_from_user(&CRZCfg,a_pCfg,sizeof(stCRZCfg))){
        IDPDB("[MT6516IDP] ConfigCRZ, copy from user failed \n");
        return -EFAULT;
    }

    //Check setting validation
    // Rule 1 : Check input, output dimension, 
    // down scale , target width must < 2688, and (target width -1)/(source width - 1) >= 1/2048 ???
    // up scale , source width must < 2688, and (target width -1)/(source width - 1) <= 64
    if(MT6516IDP_CheckCRZDimension((u32)CRZCfg.u2SrcImgWidth,(u32)CRZCfg.u2DestImgWidth,&u4HRatio)){
        IDPDB("[MT6516IDP] CRZ Dimension incorrect \n");
        return -EINVAL;
    }

    if(MT6516IDP_CheckCRZDimension((u32)CRZCfg.u2SrcImgHeight,(u32)CRZCfg.u2DestImgHeight,&u4VRatio)){
        IDPDB("[MT6516IDP] CRZ Dimension incorrect \n");
        return -EINVAL;
    }

#ifdef CRZ_CUST
    if(CRZCfg.uUpsampleCoeff > 12 || CRZCfg.uDownsampleCoeff > 12)
    {
        IDPDB("[MT6516IDP] incorrect CRZ interpolation coefficeint \n");
        return -EINVAL;
    }
#endif

//Debug message
    IDPPARMDB("CRZ In w : %d h: %d \n Out  w: %d h: %d \n", CRZCfg.u2SrcImgWidth ,CRZCfg.u2SrcImgHeight,
        CRZCfg.u2DestImgWidth,CRZCfg.u2DestImgHeight);

    u4LineBuff = CRZCfg.u2SrcImgWidth > CRZCfg.u2DestImgWidth ? 
    	(2688/(CRZCfg.u2DestImgWidth + (CRZCfg.u2DestImgWidth & 0x1))) : 
    	(2688/(CRZCfg.u2SrcImgWidth + (CRZCfg.u2SrcImgWidth & 0x1)));
    u4LineBuff = u4LineBuff*6;
    u4LineBuff = (u4LineBuff > 1023) ? 1023 : u4LineBuff;

    //Fill registers.
    u4RegValue = CRZCfg.eSrc + ((u32)CRZCfg.bToOVL << 4) + ((u32)CRZCfg.bToIPP1 << 5) + ((u32)CRZCfg.bToY2R1 << 6) +
          ((u32)CRZCfg.bContinousRun << 8) + (1<<9) + (1<<11) + (1<<13) + (u4LineBuff << 16);

    if((0x1 & ioread32(MT6516CRZ_CON)) && (CRZSRC_ISP == CRZCfg.eSrc))
    {
    //Digital zoom case.
        spin_lock_irq(&g_IDPLock);
//IDPDB("[MT6516IDP] DZoom!!\n");

        u4InputFmt = MT6516CAM_INPUT_SEL(ioread32(MT6516CAM_PATH));
        CRZDBuff(DBUFF_GET , &pDBuff);
        if(NULL == pDBuff)
        {
            IDPDB("[MT6516IDP] CRZ double buffer is null\n");
            return -EINVAL;
        }

        pDBuff->u4CRZCFG = u4RegValue;

        if (u4InputFmt == 1 || u4InputFmt == 5) //YUV or YCbCr 
        {
            u4LineBuff = (ioread32(MT6516CAM_GRABROW) & 0xFFF) - ((ioread32(MT6516CAM_GRABROW) >> 16) & 0xFFF);
        }
        else //Byaer or others 
        {
            u4LineBuff = (ioread32(MT6516CAM_GRABROW) & 0xFFF) - ((ioread32(MT6516CAM_GRABROW) >> 16) & 0xFFF) - 6;
        }

        if(u4LineBuff > CRZCfg.u2SrcImgHeight)
        {
            u4LineBuff = ((unsigned long)(u4LineBuff - CRZCfg.u2SrcImgHeight) >> 1) & (~0x1) ;//reuse this variable
        }
        else
        {
            u4LineBuff = 0;
        }
        u4RegValue = (1 << 28) + (u4LineBuff << 16) + u4LineBuff + (u32)(CRZCfg.u2SrcImgHeight);

        pDBuff->u4ISPRWINV = u4RegValue;
//            iowrite32(u4RegValue,MT6516CAM_RWINV_SEL);

        if (u4InputFmt == 1 || u4InputFmt == 5) //YUV or YCbCr 
        {
            u4LineBuff = ((ioread32(MT6516CAM_GRABCOL) & 0xFFF) - ((ioread32(MT6516CAM_GRABCOL) >> 16) & 0xFFF)) / 2;
        }
        else    //Bayer or others 
        {
            u4LineBuff = (ioread32(MT6516CAM_GRABCOL) & 0xFFF) - ((ioread32(MT6516CAM_GRABCOL) >> 16) & 0xFFF) - 8;
        }

        if(u4LineBuff > CRZCfg.u2SrcImgWidth)
        {
            u4LineBuff = ((unsigned long)(u4LineBuff - CRZCfg.u2SrcImgWidth) >> 1) & (~0x1) ;//reuse this variable
        }
        else
        {
            u4LineBuff = 0;
        }
        u4RegValue = (u4LineBuff << 16) + u4LineBuff + (u32)(CRZCfg.u2SrcImgWidth);

        pDBuff->u4ISPRWINH = u4RegValue;
//            iowrite32(u4RegValue,MT6516CAM_RWINH_SEL);            
        u4RegValue = (u32)CRZCfg.u2SrcImgWidth + ((u32)CRZCfg.u2SrcImgHeight << 16);
        pDBuff->u4CRZSRCSZ1 = u4RegValue;

        u4RegValue = (u32)CRZCfg.u2DestImgWidth + ((u32)CRZCfg.u2DestImgHeight << 16);
        pDBuff->u4CRZTARSZ1 = u4RegValue;
        pDBuff->u4CRZHRATIO1 = u4HRatio;
        pDBuff->u4CRZVRATIO1 = u4VRatio;
        pDBuff->u4Dirty = 1;

        spin_unlock_irq(&g_IDPLock);
    }
    else{
    //If CRZ is off, reset and stop it first
        iowrite32((1>>16),MT6516CRZ_CON);
        iowrite32(0,MT6516CRZ_CON);

        iowrite32(u4RegValue,MT6516CRZ_CFG);

        u4RegValue = (u32)CRZCfg.u2SrcImgWidth + ((u32)CRZCfg.u2SrcImgHeight << 16);

        iowrite32(u4RegValue,MT6516CRZ_SRCSZ1);

        u4RegValue = (u32)CRZCfg.u2DestImgWidth + ((u32)CRZCfg.u2DestImgHeight << 16);

        iowrite32(u4RegValue,MT6516CRZ_TARSZ1);

        iowrite32(u4HRatio,MT6516CRZ_HRATIO1);

        iowrite32(u4VRatio,MT6516CRZ_VRATIO1);
    }

    u4RegValue = ioread32(MT6516CRZ_INT);//Read clean reg.

    if(CRZSRC_ISP == CRZCfg.eSrc)
    {
#ifdef CRZ_CUST
        iowrite32((((((u32)CRZCfg.uUpsampleCoeff) & 0x1F) << 8) | (((u32)CRZCfg.uDownsampleCoeff) & 0x1F)), MT6516CRZ_FRCFG);
#else
        iowrite32((((((u32)8) & 0x1F) << 8) | (((u32)1) & 0x1F)), MT6516CRZ_FRCFG);
#endif
    }
    else
    {
        iowrite32((((((u32)8) & 0x1F) << 8) | (((u32)8) & 0x1F)), MT6516CRZ_FRCFG);
    }

    iowrite32(0x80FF , MT6516CRZ_BUSY);

//    IDPDB("GMC1 : %lu, GMC2 : %lu\n",ioread32(MT6516GMC1),ioread32(MT6516GMC2));

    IDPPARMDB("Config CRZ done \n");

    return 0;
}
EXPORT_SYMBOL(MT6516IDP_ConfigCRZ);

#define MT6516PRZ_CFG (PRZ_BASE)
#define MT6516PRZ_CON (PRZ_BASE + 0x4)
#define MT6516PRZ_STA (PRZ_BASE + 0x8)
#define MT6516PRZ_INT (PRZ_BASE + 0xC)
#define MT6516PRZ_SRCSZ1 (PRZ_BASE + 0x10)
#define MT6516PRZ_TARSZ1 (PRZ_BASE + 0x14)
#define MT6516PRZ_HRATIO1 (PRZ_BASE + 0x18)
#define MT6516PRZ_VRATIO1 (PRZ_BASE + 0x1C)
#define MT6516PRZ_HRES1 (PRZ_BASE + 0x20)
#define MT6516PRZ_VRES1 (PRZ_BASE + 0x24)
#define MT6516PRZ_BLKCSCFG (PRZ_BASE + 0x30)
#define MT6516PRZ_YLMBASE (PRZ_BASE + 0x34)
#define MT6516PRZ_ULMBASE (PRZ_BASE + 0x38)
#define MT6516PRZ_VLMBASE (PRZ_BASE + 0x3C)
#define MT6516PRZ_FRCFG (PRZ_BASE + 0x40)
#define MT6516PRZ_YLBSIZE (PRZ_BASE + 0x50)
#define MT6516PRZ_PRWMBASE (PRZ_BASE + 0x5C)
//TODO : parameter check should be done in user space, to save copy_from_user.
static int MT6516IDP_ConfigPRZ(void * a_pCfg)
{
    stPRZCfg PRZCfg; 
    u32 u4RegValue = 0;
    u32 u4Factor = 0;
    u32 u4LineBufferCnt = 0;
    u32 u4YBufferSize = 0 , u4UBufferSize = 0 , u4VBufferSize = 0;
    u32 u4Addr = 0, u4YAddr = 0, u4UAddr = 0, u4VAddr = 0;
//    dma_addr_t stDMAAddr;

    if(copy_from_user(&PRZCfg,a_pCfg,sizeof(stPRZCfg))){
        IDPDB("[MT6516IDP] ConfigPRZ, copy from user failed \n");
        return -EFAULT;
    }

    //Reset PRZ
    iowrite32((7<<16),MT6516PRZ_CON);
    iowrite32(0,MT6516PRZ_CON);

    if(0 != g_stSRAMMap[0].u4PhysicalAddr)
    {
        free_internal_sram(INTERNAL_SRAM_IDP_PRZ , g_stSRAMMap[0].u4PhysicalAddr);
        g_stSRAMMap[0].u4PhysicalAddr = 0;
        g_stSRAMMap[0].u4Size = 0;
    }

    //Check setting validation
    // Rule 1. Check input, output dimension.
    // output width, height must be less than 4095, and aligned with 32
    if( (PRZCfg.u2DestImgWidth > 4095) || (PRZCfg.u2DestImgHeight > 4095) || 
    	(PRZCfg.u2DestImgWidth == 0) || (PRZCfg.u2DestImgHeight == 0) )
    {
        IDPDB("[MT6516IDP] ConfigPRZ, image size exceeds max limitation:W:%d,H:%d \n",PRZCfg.u2DestImgWidth,PRZCfg.u2DestImgHeight);
        return -EINVAL;
    }

    if((PRZSRC_JPGDEC == PRZCfg.eSrc))//block base, if input size > 4095, use coarse shrink
    {
        if((PRZCfg.u2SrcImgWidth > 32760) || (PRZCfg.u2SrcImgHeight > 32760))
        {
            IDPDB("[MT6516IDP] ConfigPRZ, image size exceeds max limitation:W:%d,H:%d \n",PRZCfg.u2SrcImgWidth,PRZCfg.u2SrcImgHeight);
            return -EINVAL;
        }

        u4RegValue = (PRZCfg.u2SrcImgWidth > PRZCfg.u2SrcImgHeight ? PRZCfg.u2SrcImgWidth : PRZCfg.u2SrcImgHeight);

        //Avoid using division
        if(u4RegValue >= 4096)
        {
            u4Factor = 1;
            if((u4RegValue + 1) > (4096 << 1)){u4Factor = 2;}
            if((u4RegValue + 1) > (4096 << 2)){u4Factor = 3;}

            PRZCfg.u2SrcImgWidth = ((u32)PRZCfg.u2SrcImgWidth >> u4Factor);
            PRZCfg.u2SrcImgHeight = ((u32)PRZCfg.u2SrcImgHeight >> u4Factor);
        }

        //Allocate strip buffer
        //BuffSize = PRZCfg.u2SrcImgWidth x 64 (32 for Y, 16 for U, 16 for V);
        //Use sys2
        u4LineBufferCnt = 16;//8 is minimum,16 is better
        u4YBufferSize = 0;
        u4UBufferSize = 0;
        u4VBufferSize = 0;
        if(0xC != (PRZCfg.u2SampleFactor & 0xC))
        {
            u4YBufferSize = PRZCfg.u2SrcImgWidth*u4LineBufferCnt*(1 << ((PRZCfg.u2SampleFactor & 0xC) >> 2));
        }
        if(0xC0 != (PRZCfg.u2SampleFactor & 0xC0))
        {
            u4UBufferSize = PRZCfg.u2SrcImgWidth*u4LineBufferCnt*(1 << ((PRZCfg.u2SampleFactor & 0xC0) >> 6));
        }
        if(0xC00 != (PRZCfg.u2SampleFactor & 0xC00))
        {
            u4VBufferSize = PRZCfg.u2SrcImgWidth*u4LineBufferCnt*(1 << ((PRZCfg.u2SampleFactor & 0xC00) >> 10));
        }

        IDPPARMDB("PRZ SRAM Y size : %u\n" , u4YBufferSize);
        IDPPARMDB("PRZ SRAM U size : %u\n" , u4UBufferSize);
        IDPPARMDB("PRZ SRAM V size : %u\n" , u4VBufferSize);
        
        if(g_stPRZLineBuff.bAllocated)
        {
            free_internal_sram(INTERNAL_SRAM_IDP_PRZ_MCU , g_stPRZLineBuff.u4PhyAddr);
            //kfree(g_stPRZLineBuff.pYBuff);
            //kfree(g_stPRZLineBuff.pUBuff);
            //kfree(g_stPRZLineBuff.pVBuff);
            g_stPRZLineBuff.bAllocated = 0;
        }

        u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_PRZ_MCU , (u4YBufferSize + u4UBufferSize + u4VBufferSize) , 4);
        if(0 == u4Addr)
        {
            u4LineBufferCnt = 8;
            u4YBufferSize = u4YBufferSize >> 1;
            u4UBufferSize = u4UBufferSize >> 1;
            u4VBufferSize = u4VBufferSize >> 1;
            u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_PRZ_MCU , (u4YBufferSize + u4UBufferSize + u4VBufferSize) , 4);

            if(0 == u4Addr)
            {
                IDPDB("[MT6516IDP] PRZ Not enough SRAM space, W:%d, H:%d, S:%d \n",PRZCfg.u2SrcImgWidth,PRZCfg.u2SrcImgHeight,(u4YBufferSize + u4UBufferSize + u4VBufferSize));
                return -ENOMEM;
            }
        }
        
        g_stPRZLineBuff.u4PhyAddr = u4Addr;
        //g_stPRZLineBuff.pYBuff = kmalloc(u4YBufferSize , GFP_ATOMIC);
        //g_stPRZLineBuff.pUBuff = kmalloc(u4UBufferSize , GFP_ATOMIC);
        //g_stPRZLineBuff.pVBuff = kmalloc(u4VBufferSize , GFP_ATOMIC);
        g_stPRZLineBuff.bAllocated = 1;
        u4YAddr = u4Addr;
        u4UAddr = (u4YAddr + u4YBufferSize + 3) & 0xFFFFFFFC;
        u4VAddr = (u4UAddr + u4UBufferSize + 3) & 0xFFFFFFFC;

        IDPPARMDB("PRZ Y buffer address : 0x%x", u4YAddr);
        IDPPARMDB("PRZ U buffer address : 0x%x", u4UAddr);
        IDPPARMDB("PRZ V buffer address : 0x%x", u4VAddr);
        
        iowrite32(u4YAddr, MT6516PRZ_YLMBASE);
        iowrite32(u4UAddr, MT6516PRZ_ULMBASE);
        iowrite32(u4VAddr, MT6516PRZ_VLMBASE);

        iowrite32(u4LineBufferCnt*(1 << ((PRZCfg.u2SampleFactor & 0xC) >> 2)), MT6516PRZ_YLBSIZE);

//        u4RegValue = u4Factor + (u4Factor<<4) + (u4Factor<<6) + (u4Factor<<8) + (u4Factor<<10) + (u4Factor<<12) + (u4Factor<<14);
        u4RegValue = u4Factor + (PRZCfg.u2SampleFactor << 4);
        iowrite32(u4RegValue, MT6516PRZ_BLKCSCFG);
        u4Factor = 0;
    }
    else
    {
        //Pixel base, input size cannot be larger than 4095
        if((PRZCfg.u2SrcImgWidth > 4095) || (PRZCfg.u2DestImgHeight > 4095))
        {
            IDPDB("[MT6516IDP] PRZ input size exceed range \n");
            return -EINVAL;
        }

        if(PRZCfg.u2SrcImgWidth > (PRZCfg.u2DestImgWidth << 1)){u4Factor = 1;}
        if(PRZCfg.u2SrcImgWidth > (PRZCfg.u2DestImgWidth << 2)){u4Factor = 2;}
        if(PRZCfg.u2SrcImgWidth > (PRZCfg.u2DestImgWidth << 3)){u4Factor = 3;}

        PRZCfg.u2SrcImgWidth = PRZCfg.u2SrcImgWidth >> u4Factor;
        //Allocate SRAM. 4 bytes aligned, 
        //BuffSize = PRZCfg.u2SrcImgWidth x 4
        //u4LineBufferCnt = 4;//8 will be better.
        //Use sys2

        u4LineBufferCnt = 4;// 2 is minimum
        u4YBufferSize = PRZCfg.u2SrcImgWidth*u4LineBufferCnt*2;
        u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_PRZ , u4YBufferSize , 4);
        if(0 == u4Addr)
        {
            IDPPARMDB("[MT6516IDP] PRZ Not enough SRAM space , try single buffer... \n");
            u4YBufferSize = u4YBufferSize >>1;
            u4LineBufferCnt = 4;// 4 is minimum
            u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_PRZ , u4YBufferSize , 4);
            if(0 == u4Addr)
            {
                IDPDB("[MT6516IDP] PRZ Not enough SRAM space \n");
                return -ENOMEM;
            }
        }

        g_stSRAMMap[0].u4Size = u4YBufferSize;
        g_stSRAMMap[0].u4PhysicalAddr = u4Addr;

        //suppose the address from kmalloc should be 4 bytes aligned.
        if(u4Addr & 0x3)
        {
            IDPDB("[MT6516IDP] PRZ SRAM Address is not aligned \n");
            return -EINVAL;
        }

#if SRAM_WKAROUND
//Size : 1280*8
//        pAddr = (u32 *)0x40020000;
        u4Addr = 0x4003E600;//From Jackal
//0x4002_0000~0x4002_2800
//sys1 : 0x4000_0000~0x4001_7FFF
//sys2 : 0x4002_0000~0x4004_3FFF
#endif

        iowrite32(u4Addr,MT6516PRZ_PRWMBASE);

    }

    u4RegValue = PRZCfg.eSrc + ((u32)PRZCfg.bContinousRun << 4);
    u4RegValue = (PRZCfg.eSrc == PRZSRC_JPGDEC ? u4RegValue : (u4RegValue + (1<<5)) );
    iowrite32(u4RegValue,MT6516PRZ_CFG);

    u4RegValue = ioread32(MT6516PRZ_INT);//read clear reg

    u4RegValue = (u32)PRZCfg.u2SrcImgWidth + ((u32)PRZCfg.u2SrcImgHeight << 16);
    iowrite32(u4RegValue,MT6516PRZ_SRCSZ1);

    u4RegValue = (u32)PRZCfg.u2DestImgWidth + ((u32)PRZCfg.u2DestImgHeight << 16);
    iowrite32(u4RegValue,MT6516PRZ_TARSZ1);

    u4RegValue = ((u32)PRZCfg.u2SrcImgWidth<<20)/PRZCfg.u2DestImgWidth;
    iowrite32(u4RegValue,MT6516PRZ_HRATIO1);

    u4RegValue = ((u32)PRZCfg.u2SrcImgHeight<<20)/PRZCfg.u2DestImgHeight;
    iowrite32(u4RegValue,MT6516PRZ_VRATIO1);

    u4RegValue = PRZCfg.u2SrcImgWidth%PRZCfg.u2DestImgWidth;
    if(u4RegValue > 4094)
    {
        IDPDB("[MT6516IDP] PRZ Dimension incorrect \n");
        return -EINVAL;
    }
    iowrite32(u4RegValue,MT6516PRZ_HRES1);

    u4RegValue = PRZCfg.u2SrcImgHeight%PRZCfg.u2DestImgHeight;
    if(u4RegValue > 4094)
    {
        IDPDB("[MT6516IDP] PRZ Dimension incorrect \n");
        return -EINVAL;
    }
    iowrite32(u4RegValue,MT6516PRZ_VRES1);

    //
    u4RegValue = ((u32)PRZCfg.bToCRZ<<13) + ((u32)PRZCfg.bToIPP1<<14) + 
        ((u32)PRZCfg.bToY2R1<<15) + (u4LineBufferCnt<<16) + (u4Factor << 8);
    iowrite32(u4RegValue,MT6516PRZ_FRCFG);

    IDPPARMDB("Config PRZ done \n");

    return 0;
}

#define MT6516DRZ_STR (DRZ_BASE)
#define MT6516DRZ_CON (DRZ_BASE + 0x4)
#define MT6516DRZ_STA (DRZ_BASE + 0x8)
#define MT6516DRZ_ACKINT (DRZ_BASE + 0xC)
#define MT6516DRZ_SRC_SIZE (DRZ_BASE + 0x10)
#define MT6516DRZ_TAR_SIZE (DRZ_BASE + 0x14)
#define MT6516DRZ_RAT_H (DRZ_BASE + 0x20)
#define MT6516DRZ_RAT_V (DRZ_BASE + 0x24)
//TODO : parameter check should be done in user space, to save copy_from_user.
static int MT6516IDP_ConfigDRZ(void * a_pCfg)
{
    stDRZCfg DRZCfg;
    u32 u4RegValue = 0;

    if(copy_from_user(&DRZCfg,a_pCfg,sizeof(stDRZCfg))){
        return -EFAULT;
    }

    //Check setting validation
    // Rule 1. Check input, output dimension,
    // input and output cannot exceed 4096x4096,
    // size down ratio must be larger than 1/2047
    if((DRZCfg.u2DestImgWidth > DRZCfg.u2SrcImgWidth) || \
    	(DRZCfg.u2DestImgHeight > DRZCfg.u2SrcImgHeight) || \
    	(DRZCfg.u2DestImgWidth > 4096) || (DRZCfg.u2DestImgHeight > 4096) || \
    	(DRZCfg.u2SrcImgWidth > 4096) || (DRZCfg.u2SrcImgHeight > 4096) || \
    	((DRZCfg.u2SrcImgWidth>>11) > DRZCfg.u2DestImgWidth) || \
    	((DRZCfg.u2SrcImgHeight>>11) > DRZCfg.u2DestImgHeight) || \
    	(DRZCfg.u2DestImgWidth == 0) || (DRZCfg.u2DestImgHeight == 0))
    {
        IDPDB("[MT6516IDP] ConfigPRZ, image size is not correct \n");
        return -EINVAL;
    }

    iowrite32(0,MT6516DRZ_STR);

    u4RegValue = (DRZCfg.bContinousRun << 3);
    iowrite32(u4RegValue,MT6516DRZ_CON);

    u4RegValue = (u32)(DRZCfg.u2SrcImgWidth-1) + ((u32)(DRZCfg.u2SrcImgHeight-1) <<16);
    iowrite32(u4RegValue,MT6516DRZ_SRC_SIZE);

    u4RegValue = (u32)(DRZCfg.u2DestImgWidth-1) + ((u32)(DRZCfg.u2DestImgHeight-1) <<16);
    iowrite32(u4RegValue,MT6516DRZ_TAR_SIZE);    

    u4RegValue = ((u32)(DRZCfg.u2SrcImgWidth/DRZCfg.u2DestImgWidth)<<16) + (DRZCfg.u2SrcImgWidth%DRZCfg.u2DestImgWidth);
    
    iowrite32(u4RegValue,MT6516DRZ_RAT_H);    

    u4RegValue = ((u32)(DRZCfg.u2SrcImgHeight/DRZCfg.u2DestImgHeight)<<16) + (DRZCfg.u2SrcImgHeight%DRZCfg.u2DestImgHeight);

    iowrite32(u4RegValue,MT6516DRZ_RAT_V);

    IDPPARMDB("Config DRZ done \n");

    return 0;
}

#define RBITDEPTH(x) (x >> 16)
#define GBITDEPTH(x) ((x & 0xFF00) >> 8)
#define BBITDEPTH(x) (x & 0xFF)

#define MT6516IPP_CFG (IMG_BASE)
#define MT6516IPP_SDTCON (IMG_BASE + 0xC)
#define MT6516IMPROC_HUE11 (IMG_BASE + 0x100)
#define MT6516IMPROC_HUE12 (IMG_BASE + 0x104)
#define MT6516IMPROC_HUE21 (IMG_BASE + 0x108)
#define MT6516IMPROC_HUE22 (IMG_BASE + 0x10C)
#define MT6516IMPROC_SAT (IMG_BASE + 0x110)
#define MT6516IMPROC_BRIADJ1 (IMG_BASE + 0x120)
#define MT6516IMPROC_BRIADJ2 (IMG_BASE + 0x124)
#define MT6516IMPROC_CONADJ (IMG_BASE + 0x128)
#define MT6516IMPROC_COLORIZEU (IMG_BASE + 0x130)
#define MT6516IMPROC_COLORIZEV (IMG_BASE + 0x134)
#define MT6516IMPROC_GAMMA_OFF0 (IMG_BASE + 0x170)
#define MT6516IMPROC_GAMMA_SLP0 (IMG_BASE + 0x190)
#define MT6516IMPROC_GAMMA_CON (IMG_BASE + 0x1B0)
#define MT6516IMPROC_COLOR1R_OFFX (IMG_BASE + 0x200)
#define MT6516IMPROC_COLOR0R_SLOPE (IMG_BASE + 0x240)
#define MT6516IMPROC_IO_MUX (IMG_BASE + 0x318)
#define MT6516IMPROC_EN (IMG_BASE + 0x320)
#define MT6516IMPROC_IPP_RGB_DETECT (IMG_BASE + 0x324)
#define MT6516IMPROC_IPP_RGB_REPLACE (IMG_BASE + 0x328)
static int MT6516IDP_ConfigIPP1_Y2R0_IPP2(void * a_pCfg)
{
    stIPP1Y2R0IPP2Cfg Cfg;
    u32 u4RegValue = 0;
    u32 u4Index = 0;
    PANEL_COLOR_FORMAT ePANEL_FMT;

    if(copy_from_user(&Cfg,a_pCfg,sizeof(stIPP1Y2R0IPP2Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigIPP, Copy from user failed\n");
    }
#if 0
    //Check setting
    // Rule 1. SDT0_en and SDT1_en must be mutual exclusive
    if(Cfg.bSDT0 & Cfg.bSDT1)
    {
        IDPDB("[MT6516IDP] ConfigIPP, SDT0 and SDT1 cannot be 1 simutaneously\n");
        return -EINVAL;
    }

    //Fill registers
    u4RegValue = ((u32)Cfg.bContrastNBrightness << 4) + ((u32)Cfg.bHue << 5) + ((u32)Cfg.bSAT << 8) +
        ((u32)Cfg.bColorize << 9) + ((u32)Cfg.bSDT0 << 12) + ((u32)Cfg.bY2R0 << 16) + ((u32)Cfg.bY2R0Round << 17) +
        ((u32)Cfg.bColorAdj << 20) + ((u32)Cfg.bGammaCorrect << 24) + ((u32)Cfg.bColorInverse << 28) +
        ((u32)Cfg.bSDT1 << 29) + ((u32)Cfg.bRGBDetectNReplace << 30);
    iowrite32(u4RegValue,MT6516IPP_CFG);

    u4RegValue = (u32)Cfg.uBD3 + ((u32)Cfg.uBD2 << 4) + ((u32)Cfg.uBD1 << 8) + ((u32)Cfg.uSeed3 << 12) +
        ((u32)Cfg.uSeed2 << 16) + ((u32)Cfg.uSeed1 << 20);
    iowrite32(u4RegValue,MT6516IPP_SDTCON);
#else
    if(Cfg.bSDT1)
    {
        ePANEL_FMT = DISP_GetPanelColorFormat();
//        u4RegValue |= (8 - RBITDEPTH(ePANEL_FMT));
        u4RegValue |= 3;
        u4RegValue |= (u4RegValue << 8);//Suppose symetric
//        u4RegValue |= ((8 - GBITDEPTH(ePANEL_FMT)) << 4);
        u4RegValue |= (2 << 4);
        u4RegValue |= ((8 << 20) + (8 << 16) + (8 << 11));
        iowrite32(u4RegValue,MT6516IPP_SDTCON);

        if(0 == u4RegValue)
        {
            Cfg.bSDT1 = 0;
        }
    }

    u4RegValue = ((u32)Cfg.bContrastNBrightness << 4) + ((u32)Cfg.bHue << 5) + ((u32)Cfg.bSAT << 8) +
        ((u32)Cfg.bColorize << 9) + ((u32)Cfg.bY2R0 << 16) + ((u32)Cfg.bY2R0Round << 17) +
        ((u32)Cfg.bColorAdj << 20) + ((u32)Cfg.bGammaCorrect << 24) + ((u32)Cfg.bColorInverse << 28) +
        ((u32)Cfg.bSDT1 << 29) + ((u32)Cfg.bRGBDetectNReplace << 30);
    iowrite32(u4RegValue,MT6516IPP_CFG);
#endif

    iowrite32(Cfg.uHueC11,MT6516IMPROC_HUE11);
    iowrite32(Cfg.uHueC12,MT6516IMPROC_HUE12);
    iowrite32(Cfg.uHueC21,MT6516IMPROC_HUE21);
    iowrite32(Cfg.uHueC22,MT6516IMPROC_HUE22);
    iowrite32(Cfg.uSaturation,MT6516IMPROC_SAT);
    iowrite32(Cfg.uBrightCoeffB1,MT6516IMPROC_BRIADJ1);
    iowrite32(Cfg.uBrightCoeffB2,MT6516IMPROC_BRIADJ2);
    iowrite32(Cfg.uContrast,MT6516IMPROC_CONADJ);
    iowrite32(Cfg.uColorizeU,MT6516IMPROC_COLORIZEU);
    iowrite32(Cfg.uColorizeV,MT6516IMPROC_COLORIZEV);

    for(u4Index = MT6516IPP_GAMMA_COEFF_CNT; u4Index > 0 ; u4Index --)
    {
        iowrite32(Cfg.uGammaOffset[u4Index],(MT6516IMPROC_GAMMA_OFF0 + (u4Index << 2)));
        iowrite32(Cfg.uGammaSlope[u4Index],(MT6516IMPROC_GAMMA_SLP0 + (u4Index << 2)));
    }

    iowrite32(Cfg.bGammaGreatThanOne,MT6516IMPROC_GAMMA_CON);

    for(u4Index = MT6516IPP_COLOR_OFF_COEFF_CNT; u4Index > 0 ; u4Index --)
    {
        iowrite32(Cfg.uColorAdjOffet[u4Index],(MT6516IMPROC_COLOR1R_OFFX + (u4Index << 2)));
    }

    for(u4Index = MT6516IPP_COLOR_SLP_COEFF_CNT; u4Index > 0 ; u4Index --)
    {
        iowrite32(Cfg.uColorAdjOffet[u4Index],(MT6516IMPROC_COLOR0R_SLOPE + (u4Index << 2)));
    }

    u4RegValue = ioread32(MT6516IMPROC_IO_MUX);
    u4RegValue &= (~0x7F);
    u4RegValue += (Cfg.eSrc + ((u32)Cfg.bOverlap << 4) + ((u32)Cfg.bToOVL << 5) + ((u32)Cfg.bToIBW2 << 6));
    iowrite32(u4RegValue,MT6516IMPROC_IO_MUX);

    u4RegValue = ((u32)Cfg.uRGBDetect[0] << 16) + ((u32)Cfg.uRGBDetect[1] << 8) + (u32)Cfg.uRGBDetect[2];
    iowrite32(u4RegValue,MT6516IMPROC_IPP_RGB_DETECT);
    u4RegValue = ((u32)Cfg.uRGBReplace[0] << 16) + ((u32)Cfg.uRGBReplace[1] << 8) + (u32)Cfg.uRGBReplace[2];
    iowrite32(u4RegValue,MT6516IMPROC_IPP_RGB_REPLACE);

    IDPPARMDB("Config IPP done \n");

    return 0;
}

#define MT6516R2Y0_CFG (IMG_BASE + 0x4)
static int MT6516IDP_ConfigR2Y0(void * a_pCfg)
{
    stR2Y0Cfg R2Y0Cfg;
    u32 u4RegValue = 0;

    if(copy_from_user(&R2Y0Cfg,a_pCfg,sizeof(stR2Y0Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigR2Y0, Copy from user failed\n");
    }
    //Check setting

    //Fill registers.
    u4RegValue = (u32)R2Y0Cfg.bR2Y + ((u32)R2Y0Cfg.bR2Y0Round << 1);
    iowrite32(u4RegValue,MT6516R2Y0_CFG);

    u4RegValue = ioread32(MT6516IMPROC_IO_MUX);
    u4RegValue &= (~0x1F0000);
    u4RegValue += (((u32)R2Y0Cfg.bToPRZ << 20) | ((u32)R2Y0Cfg.bToCRZ << 19) | ((u32)R2Y0Cfg.eSrc << 16));
    iowrite32(u4RegValue,MT6516IMPROC_IO_MUX);

    IDPPARMDB("Config R2Y0 done \n");

    return 0;
}

#define MT6516Y2R1_CFG (IMG_BASE + 0x8)
#define MT6516Y2R1_SDTCON (IMG_BASE + 0x10)
#define MT6516Y2R1_RGB_DETECT (IMG_BASE + 0x32C)
#define MT6516Y2R1_RGB_REPLACE (IMG_BASE + 0x330)
static int MT6516IDP_ConfigY2R1(void * a_pCfg)
{
    int retVal = 0;
    stY2R1Cfg Y2R1Cfg;
    u32 u4RegValue = 0;
    PANEL_COLOR_FORMAT ePANEL_FMT;

    if(copy_from_user(&Y2R1Cfg,a_pCfg,sizeof(stY2R1Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigY2R1, Copy from user failed\n");
    }
#if 0
    //Check setting
    // Rule 1.SDT3_en and SDT2_en must be mutual exclusive
    if(Y2R1Cfg.bSDT2 & Y2R1Cfg.bSDT3)
    {
        IDPDB("[MT6516IDP] ConfigIPP, SDT0 and SDT1 cannot be 1 simutaneously\n");
        return -EINVAL;
    }

    //Fill registers.
    u4RegValue = Y2R1Cfg.bY2R1 + ((u32)Y2R1Cfg.bRoundY2R1 << 1) + ((u32)Y2R1Cfg.bSDT2 << 2) + ((u32)Y2R1Cfg.bSDT3 << 3) +
        ((u32)Y2R1Cfg.bRGBDetectNReplace << 4);
    iowrite32(u4RegValue,MT6516Y2R1_CFG);

    u4RegValue = (u32)Y2R1Cfg.uBD3 + ((u32)Y2R1Cfg.uBD2 << 4) + ((u32)Y2R1Cfg.uBD1 << 8) + ((u32)Y2R1Cfg.uSeed3 << 12) +
        ((u32)Y2R1Cfg.uSeed2 << 16) + ((u32)Y2R1Cfg.uSeed1 << 20);
    iowrite32(u4RegValue,MT6516Y2R1_SDTCON);
#else
    if(Y2R1Cfg.bSDT3)
    {
        ePANEL_FMT = DISP_GetPanelColorFormat();
//        u4RegValue |= (8 - RBITDEPTH(ePANEL_FMT));
        u4RegValue |= 3;
        u4RegValue |= (u4RegValue << 8);//Suppose symetric
//        u4RegValue |= ((8 - GBITDEPTH(ePANEL_FMT)) << 4);
        u4RegValue |= (2 << 4);
        u4RegValue |= ((8 << 20) + (8 << 16) + (8 << 11));
        iowrite32(u4RegValue,MT6516Y2R1_SDTCON);

        if(0 == u4RegValue)
        {
            Y2R1Cfg.bSDT3 = 0;
        }
    }

    u4RegValue = Y2R1Cfg.bY2R1 + ((u32)Y2R1Cfg.bRoundY2R1 << 1) + (Y2R1Cfg.bSDT3 << 3) + ((u32)Y2R1Cfg.bRGBDetectNReplace << 4);
    iowrite32(u4RegValue,MT6516Y2R1_CFG);
#endif

    u4RegValue = ((u32)Y2R1Cfg.uRGBDetect[0] << 16) + ((u32)Y2R1Cfg.uRGBDetect[1] << 8) + (u32)Y2R1Cfg.uRGBDetect[2];
    iowrite32(u4RegValue,MT6516Y2R1_RGB_DETECT);
    u4RegValue = ((u32)Y2R1Cfg.uRGBReplace[0] << 16) + ((u32)Y2R1Cfg.uRGBReplace[1] << 8) + (u32)Y2R1Cfg.uRGBReplace[2];
    iowrite32(u4RegValue,MT6516Y2R1_RGB_REPLACE);

    u4RegValue = ioread32(MT6516IMPROC_IO_MUX);
    u4RegValue &= (~0xF00);
    u4RegValue += ((1 << 11) + (Y2R1Cfg.eSrc << 8)) ;
    iowrite32(u4RegValue,MT6516IMPROC_IO_MUX);

    IDPPARMDB("Config Y2R1 done \n");

    return retVal;
}

#define MT6516IMGDMA0_STA (IMGDMA_BASE)
#define MT6516IMGDMA0_ACKINT (IMGDMA_BASE + 0x4)
#define MT6516IMGDMA0_SW_RSTB (IMGDMA_BASE + 0x10)
#define MT6516IMGDMA0_GMCIF_STA (IMGDMA_BASE + 0x20)
#define MT6516IMGDMA0_CURR_FRAME (IMGDMA_BASE + 0x40)

#define MT6516IMGDMA0_IBW1_STR (IMGDMA_BASE + 0x300)
#define MT6516IMGDMA0_IBW1_CON (IMGDMA_BASE + 0x304)
#define MT6516IMGDMA0_IBW1_ALPHA (IMGDMA_BASE + 0x308)
#define MT6516IMGDMA0_IBW1_SRC_XSIZE (IMGDMA_BASE + 0x310)
#define MT6516IMGDMA0_IBW1_SRC_YSIZE (IMGDMA_BASE + 0x314)
#define MT6516IMGDMA0_IBW1_CLIPLR (IMGDMA_BASE + 0x318)
#define MT6516IMGDMA0_IBW1_CLIPTB (IMGDMA_BASE + 0x31C)
#define MT6516IMGDMA0_IBW1_BASE_ADDR0 (IMGDMA_BASE + 0x320)
#define MT6516IMGDMA0_IBW1_BASE_ADDR1 (IMGDMA_BASE + 0x324)
#define MT6516IMGDMA0_IBW1_BASE_ADDR2 (IMGDMA_BASE + 0x328)
#define MT6516IMGDMA0_IBW1_BKGD_XSIZE0 (IMGDMA_BASE + 0x330)
#define MT6516IMGDMA0_IBW1_BKGD_XSIZE1 (IMGDMA_BASE + 0x334)
#define MT6516IMGDMA0_IBW1_BKGD_XSIZE2 (IMGDMA_BASE + 0x338)
#define MT6516IMGDMA0_IBW1_RX_XCNT (IMGDMA_BASE + 0x340)
#define MT6516IMGDMA0_IBW1_RX_YCNT (IMGDMA_BASE + 0x344)
#define MT6516IMGDMA0_IBW1_HORI_CNT (IMGDMA_BASE + 0x350)
#define MT6516IMGDMA0_IBW1_VERT_CNT (IMGDMA_BASE + 0x354)
static int MT6516IDP_ConfigIBW1(void * a_pCfg)
{
    stIBW1Cfg IBW1Cfg;
    u32 u4RegValue = 0;
    u32 u4Mask = 0;

    if(copy_from_user(&IBW1Cfg,a_pCfg,sizeof(stIBW1Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigIBW1, Copy from user failed\n");
    }

    //Check setting
    // Rule 1. if RGB 565, address alignment should be 2x, if ARGB8888, it shoud be 4 x.
    u4Mask = (MT6516IBW1_OUTFMT_RGB565 == IBW1Cfg.eFmt ? 0x1 : 0x3);
    if(IBW1Cfg.u4DestBufferPhysAddr0 & u4Mask)
    {
        IDPDB("[MT6516IDP] ConfigIBW1, the base address is not aligned\n");
        return -EINVAL;
    }

    if((1 == IBW1Cfg.bAutoRestart) && (IBW1Cfg.u4DestBufferPhysAddr1 & u4Mask))
    {
        IDPDB("[MT6516IDP] ConfigIBW1, the base address is not aligned\n");
        return -EINVAL;        
    }

    if((1 == IBW1Cfg.bAutoRestart) && (1 == IBW1Cfg.bTripleBuffer) && (IBW1Cfg.u4DestBufferPhysAddr2 & u4Mask))
    {
        IDPDB("[MT6516IDP] ConfigIBW1, the base address is not aligned\n");
        return -EINVAL;        
    }

    // Rule 2. Make sure IBW1 is stopped.
    iowrite32(0,MT6516IMGDMA0_IBW1_STR);

    //Fill registers.
    u4RegValue = IBW1Cfg.bInterrupt + ((u32)IBW1Cfg.bAutoRestart << 1) + ((u32)IBW1Cfg.bTripleBuffer << 2) +
        ((u32)IBW1Cfg.bPitch << 4) + ((u32)IBW1Cfg.bClip << 5) + ((u32)IBW1Cfg.bInformLCDDMA << 6) +
        (IBW1Cfg.eFmt << 8);
    iowrite32(u4RegValue,MT6516IMGDMA0_IBW1_CON);
    iowrite32(IBW1Cfg.uAlpha,MT6516IMGDMA0_IBW1_ALPHA);
    iowrite32(IBW1Cfg.u2SrcImgWidth,MT6516IMGDMA0_IBW1_SRC_XSIZE);
    iowrite32(IBW1Cfg.u2SrcImgHeight,MT6516IMGDMA0_IBW1_SRC_YSIZE);
    u4RegValue = ((u32)IBW1Cfg.u2ClipStartX << 16) + (u32)IBW1Cfg.u2ClipEndX;
    iowrite32(u4RegValue,MT6516IMGDMA0_IBW1_CLIPLR);
    u4RegValue = ((u32)IBW1Cfg.u2ClipStartY << 16) + (u32)IBW1Cfg.u2ClipEndY;
    iowrite32(u4RegValue,MT6516IMGDMA0_IBW1_CLIPTB);
    iowrite32(IBW1Cfg.u4DestBufferPhysAddr0,MT6516IMGDMA0_IBW1_BASE_ADDR0);
    iowrite32(IBW1Cfg.u4DestBufferPhysAddr1,MT6516IMGDMA0_IBW1_BASE_ADDR1);
    iowrite32(IBW1Cfg.u4DestBufferPhysAddr2,MT6516IMGDMA0_IBW1_BASE_ADDR2);
    iowrite32(IBW1Cfg.u2PitchImg0Width,MT6516IMGDMA0_IBW1_BKGD_XSIZE0);
    iowrite32(IBW1Cfg.u2PitchImg1Width,MT6516IMGDMA0_IBW1_BKGD_XSIZE1);
    iowrite32(IBW1Cfg.u2PitchImg2Width,MT6516IMGDMA0_IBW1_BKGD_XSIZE2);

    IDPPARMDB("Config IBW1 done \n");

    return 0;
}

#define MT6516IMGDMA0_IBW2_STR (IMGDMA_BASE + 0x400)
//#define MT6516IMGDMA0_IBW2_CON (IMGDMA_BASE + 0x404)
#define MT6516IMGDMA0_IBW2_ALPHA (IMGDMA_BASE + 0x408)
#define MT6516IMGDMA0_IBW2_XSIZE (IMGDMA_BASE + 0x410)
#define MT6516IMGDMA0_IBW2_YSIZE (IMGDMA_BASE + 0x414)
#define MT6516IMGDMA0_IBW2_CLIPLR (IMGDMA_BASE + 0x418)
#define MT6516IMGDMA0_IBW2_CLIPTB (IMGDMA_BASE + 0x41C)
#define MT6516IMGDMA0_IBW2_XCNT (IMGDMA_BASE + 0x420)
#define MT6516IMGDMA0_IBW2_YCNT (IMGDMA_BASE + 0x424)
static int MT6516IDP_ConfigIBW2(void * a_pCfg)
{
    stIBW2Cfg IBW2Cfg;
    u32 u4RegValue = 0;

    if(copy_from_user(&IBW2Cfg,a_pCfg,sizeof(stIBW2Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigIBW2, copy from user failed\n");
    }

    //Fill registers.
    u4RegValue = IBW2Cfg.bInterrupt + ((u32)IBW2Cfg.bInformLCDDMA << 1) + ((u32)IBW2Cfg.bAutoRestart << 2) +
        ((u32)IBW2Cfg.bClip << 3) + (1 << 5) + ((u32)IBW2Cfg.bToLCDDMA << 6) +
        ((u32)IBW2Cfg.bToIRT1 << 7) + ((u32)IBW2Cfg.bToR2Y0 << 8);
    iowrite32(u4RegValue, MT6516IMGDMA0_IBW2_CON);
    iowrite32(IBW2Cfg.uAlpha , MT6516IMGDMA0_IBW2_ALPHA);
    iowrite32(IBW2Cfg.u2SrcImgWidth, MT6516IMGDMA0_IBW2_XSIZE);
    iowrite32(IBW2Cfg.u2SrcImgHeight, MT6516IMGDMA0_IBW2_YSIZE);
    u4RegValue = ((u32)IBW2Cfg.u2ClipStartX << 16) + (u32)IBW2Cfg.u2ClipEndX;
    iowrite32(u4RegValue,MT6516IMGDMA0_IBW2_CLIPLR);
    u4RegValue = ((u32)IBW2Cfg.u2ClipStartY << 16) + (u32)IBW2Cfg.u2ClipEndY;
    iowrite32(u4RegValue,MT6516IMGDMA0_IBW2_CLIPTB);

    IDPPARMDB("Config IBW2 done \n");

    return 0;
}

#define MT6516IMGDMA0_IBR1_STR (IMGDMA_BASE + 0x500)
#define MT6516IMGDMA0_IBR1_CON (IMGDMA_BASE + 0x504)
#define MT6516IMGDMA0_IBR1_BASE (IMGDMA_BASE + 0x508)
#define MT6516IMGDMA0_IBR1_PXLNUM (IMGDMA_BASE + 0x50C)
#define MT6516IMGDMA0_IBR1_PXLCNT (IMGDMA_BASE + 0x510)
static int MT6516IDP_ConfigIBR1(void * a_pCfg)
{
    stIBR1Cfg IBR1Cfg;
    u32 u4RegValue = 0;

    if(copy_from_user(&IBR1Cfg,a_pCfg,sizeof(stIBR1Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigIBR1, the base address is not aligned\n");
    }

    // Check setting
    // Rule 1. Address should be 8x aligned
    if(IBR1Cfg.u4SrcBufferPhysAddr & 0x7)
    {
        IDPDB("[MT6516IDP] ConfigIBR1, the base address is not aligned\n");
        return -EINVAL;
    }

    //Fill registers.
    u4RegValue = (u32)IBR1Cfg.bInterrupt + (IBR1Cfg.eFmt << 1);
    iowrite32(u4RegValue,MT6516IMGDMA0_IBR1_CON);
    iowrite32(IBR1Cfg.u4SrcBufferPhysAddr,MT6516IMGDMA0_IBR1_BASE);
    iowrite32(IBR1Cfg.u4SrcImgPixelCnt,MT6516IMGDMA0_IBR1_PXLNUM);

    IDPPARMDB("Config IBR1 done \n");

    return 0;
}

#define MT6516IMGDMA0_OVL_STR (IMGDMA_BASE + 0x700)
#define MT6516IMGDMA0_OVL_CON (IMGDMA_BASE + 0x704)
#define MT6516IMGDMA0_OVL_BASE (IMGDMA_BASE + 0x708)
#define MT6516IMGDMA0_OVL_CFG (IMGDMA_BASE + 0x70C)
#define MT6516IMGDMA0_OVL_XSIZE (IMGDMA_BASE + 0x710)
#define MT6516IMGDMA0_OVL_YSIZE (IMGDMA_BASE + 0x714)
#define MT6516IMGDMA0_OVL_XCNT (IMGDMA_BASE + 0x718)
#define MT6516IMGDMA0_OVL_YCNT (IMGDMA_BASE + 0x71C)
#define MT6516IMGDMA0_OVL_MO_0_STR (IMGDMA_BASE + 0x780)
#define MT6516IMGDMA0_OVL_MO_0_CON (IMGDMA_BASE + 0x784)
#define MT6516IMGDMA0_OVL_MO_0_BUSY (IMGDMA_BASE + 0x788)
#define MT6516IMGDMA0_OVL_PAL_BASE (IMGDMA_BASE + 0x800)
#define MT6516IMGDMA1_OVL_MO_1_STR (IMGDMA1_BASE + 0x780)
#define MT6516IMGDMA1_OVL_MO_1_CON (IMGDMA1_BASE + 0x784)
#define MT6516IMGDMA1_OVL_MO_1_BUSY (IMGDMA1_BASE + 0x788)
#define MT6516IDP_G1MEMPDN (CONFIG_BASE + 0x30C)
static int MT6516IDP_ConfigOVL(void * a_pCfg)
{
    int retVal = 0;
    stOVLCfg OVLCfg;
    u32 u4RegValue = 0;
    u32 * pu4Palette = NULL;

    if(copy_from_user(&OVLCfg,a_pCfg,sizeof(stOVLCfg))){
        IDPDB("[MT6516IDP] ConfigOVL, copy from user failed\n");
        return -EFAULT;
    }

    //Stop OVL
    iowrite32(0,MT6516IMGDMA0_OVL_MO_0_STR);
    iowrite32(0,MT6516IMGDMA1_OVL_MO_1_STR);

    //Power on sram
    u4RegValue = ioread32(MT6516IDP_G1MEMPDN);
    u4RegValue &= (~(1<<7));
    iowrite32(u4RegValue , MT6516IDP_G1MEMPDN);

    if(OVLCfg.bOverlay)
    {
        //Check setting
        // Rule 1.source image buffer address should be 8x aligned
        if(OVLCfg.u4SrcImgPhyAddr & 0x7)
        {
            IDPDB("[MT6516IDP] ConfigOVL, the base address is not aligned\n");
            return -EINVAL;
        }
        // Rule 2.Vratio, Hratio should not be zero.
        if((0 == OVLCfg.uHRatio) || (0 == OVLCfg.uVRatio) || (OVLCfg.uHRatio > 15) || (OVLCfg.uVRatio > 15))
        {
            IDPDB("[MT6516IDP] ConfigOVL, Vratio and Hratio incorrect\n");
            return -EINVAL;
        }

        pu4Palette = kmalloc(1024,GFP_ATOMIC);
        if(NULL == pu4Palette)
        {
            IDPDB("[MT6516IDP] ConfigOVL, Cannot allocate memory\n");
            return -EINVAL;
        }

        if(copy_from_user(pu4Palette,OVLCfg.pu4Palette,1024)){
            kfree(pu4Palette);
            IDPDB("[MT6516IDP] ConfigOVL, copy from user failed\n");
            return -EFAULT;
        }

        memcpy_toio((volatile void *)MT6516IMGDMA0_OVL_PAL_BASE,pu4Palette,1024);
        kfree(pu4Palette);

        iowrite32(OVLCfg.u4SrcImgPhyAddr,MT6516IMGDMA0_OVL_BASE);
        iowrite32(OVLCfg.u2MaskImgWidth,MT6516IMGDMA0_OVL_XSIZE);
        iowrite32(OVLCfg.u2MaskImgHeight,MT6516IMGDMA0_OVL_YSIZE);
        u4RegValue = (u32)OVLCfg.uHRatio + ((u32)OVLCfg.uVRatio << 4) + ((u32)OVLCfg.uColorKey << 8);
        iowrite32(u4RegValue,MT6516IMGDMA0_OVL_CFG);
    }

    //Fill registers.
    u4RegValue = ((u32)OVLCfg.uMaskDataDepth << 1) + ((u32)OVLCfg.bOverlay << 4) + ((u32)OVLCfg.eSrc << 5);
    iowrite32(u4RegValue,MT6516IMGDMA0_OVL_CON);

    u4RegValue = ((u32)OVLCfg.bToY2R0 << 1) + ((u32)OVLCfg.bToDRZ << 2);
    if(OVLCfg.bToJPEGDMA |OVLCfg.bToPRZ |OVLCfg.bToVDOENC)
    {
        u4RegValue += 1;//OVLCfg.bToIMGDMA1;
    }
    iowrite32(u4RegValue,MT6516IMGDMA0_OVL_MO_0_CON);

    u4RegValue = (u32)OVLCfg.bToJPEGDMA + ((u32)OVLCfg.bToVDOENC << 1) + ((u32)OVLCfg.bToPRZ << 2);
    iowrite32(u4RegValue,MT6516IMGDMA1_OVL_MO_1_CON);

    IDPPARMDB("Config OVL done \n");

    return retVal;
}

#define MT6516IMGDMA0_IRT0_MO_0_STR (IMGDMA_BASE + 0xC80)
#define MT6516IMGDMA0_IRT0_MO_0_CON (IMGDMA_BASE + 0xC84)
#define MT6516IMGDMA0_IRT0_MO_0_BUSY (IMGDMA_BASE + 0xC88)
#define MT6516IMGDMA1_IRT0_MO_1_STR (IMGDMA1_BASE + 0xC80)
#define MT6516IMGDMA1_IRT0_MO_1_CON (IMGDMA1_BASE + 0xC84)
#define MT6516IMGDMA1_IRT0_MO_1_BUSY (IMGDMA1_BASE + 0xC88)
#define MT6516IMGDMA1_IRT0_STR (IMGDMA1_BASE + 0xC00)
#define MT6516IMGDMA1_IRT0_CON (IMGDMA1_BASE + 0xC04)
#define MT6516IMGDMA1_IRT0_FIFO_BASE (IMGDMA1_BASE + 0xC10)
#define MT6516IMGDMA1_IRT0_FIFOYLEN (IMGDMA1_BASE + 0xC14)
#define MT6516IMGDMA1_IRT0_XSIZE (IMGDMA1_BASE + 0xC20)
#define MT6516IMGDMA1_IRT0_YSIZE (IMGDMA1_BASE + 0xC24)
#define MT6516IMGDMA1_IRT0_WRPTR (IMGDMA1_BASE + 0xC30)
#define MT6516IMGDMA1_IRT0_RDPTR (IMGDMA1_BASE + 0xC40)
#define MT6516IMGDMA1_IRT0_RDXCNT (IMGDMA1_BASE + 0xC44)
#define MT6516IMGDMA1_IRT0_RDYCNT (IMGDMA1_BASE + 0xC48)
#define MT6516IMGDMA1_IRT0_FIFOCNT (IMGDMA1_BASE + 0xC50)
#define MT6516IMGDMA1_IRT0_WRYIDX (IMGDMA1_BASE + 0xC54)
#define MT6516IMGDMA1_IRT0_RDYIDX (IMGDMA1_BASE + 0xC58)
static int MT6516IDP_ConfigIRT0(void * a_pCfg)
{
    u32 u4RegValue = 0;
    stIRT0Cfg IRT0Cfg;
    u32 u4BufferSize = 0;
    u32 u4Addr = 0;

    if(copy_from_user(&IRT0Cfg,a_pCfg,sizeof(stIRT0Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigIRT0, copy from user failed\n");
        return -EFAULT;
    }

    //Check setting
    // Rule 1. soure dimension must be 16x aligned, and less than 4096.
    if((IRT0Cfg.u2SrcImgWidth & 0xF) || (IRT0Cfg.u2SrcImgHeight & 0xF) ||
        (IRT0Cfg.u2SrcImgWidth > 4095) || (IRT0Cfg.u2SrcImgHeight > 4095))
    {
        IDPDB("[MT6516IDP] ConfigIRT0, input dimension is incorrect\n");
        return -EFAULT;
    }

    if((IRT0Cfg.bFlip || IRT0Cfg.bRotate) && (VDODEC_SCANLINE == IRT0Cfg.eSrc))
    {
        IDPDB("[MT6516IDP] ConfigIRT0, no rotation when scan line source\n");
        return -EFAULT;        
    }

    //Stop IRT0
    iowrite32(0,MT6516IMGDMA1_IRT0_STR);

    //Fill registers.
    u4RegValue = IRT0Cfg.bInterrupt + ((u32)IRT0Cfg.bAutoRestart << 1) + ((u32)IRT0Cfg.bRotate << 4) + ((u32)IRT0Cfg.bFlip << 6);
    if( VDODEC_SCANLINE != IRT0Cfg.eSrc)
    {
        u4RegValue += ((u32)IRT0Cfg.eSrc << 2);
    }
    iowrite32(u4RegValue,MT6516IMGDMA1_IRT0_CON);

    //Allocate line buffer, the address must be aligned with 8
    //TODO, allocate SRAM from 0x40020000 ~ 0x40043fff, size = 1.5x16xXsize(if 90/270), Ysize(if 0/180)
    //sys2
    if(IRT0Cfg.bRotate & (0x1))
    {
        u4BufferSize = 24*IRT0Cfg.u2SrcImgHeight;
    }
    else
    {
        u4BufferSize = 24*IRT0Cfg.u2SrcImgWidth;
    }

    if(0 != g_stSRAMMap[1].u4PhysicalAddr)
    {
        free_internal_sram(INTERNAL_SRAM_IDP_IRT0 , g_stSRAMMap[1].u4PhysicalAddr);
        g_stSRAMMap[1].u4PhysicalAddr = 0;
        g_stSRAMMap[1].u4Size = 0;
    }

    u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_IRT0 , u4BufferSize , 8);
    if(0 == u4Addr)
    {
        printk("malloc fail\n");
        return -EFAULT;
    }
    g_stSRAMMap[1].u4Size = u4BufferSize;
    g_stSRAMMap[1].u4PhysicalAddr = u4Addr;

#if SRAM_WKAROUND
//Size : 24x1280
//    pAddr = (u32 *)0x40022800;
    u4Addr = 0x40031170;//From Jackal
//0x4000_0000
//0x4002_2800~0x4002_A000

//sys1 : 0x4000_0000~0x4001_7FFF
//sys2 : 0x4002_0000~0x4004_3FFF
#endif

    iowrite32(u4Addr,MT6516IMGDMA1_IRT0_FIFO_BASE);
    iowrite32(16,MT6516IMGDMA1_IRT0_FIFOYLEN);

    iowrite32(IRT0Cfg.u2SrcImgWidth,MT6516IMGDMA1_IRT0_XSIZE);
    iowrite32(IRT0Cfg.u2SrcImgHeight,MT6516IMGDMA1_IRT0_YSIZE);

    u4RegValue = IRT0Cfg.bToCRZ + ((u32)IRT0Cfg.bToIPP1 << 1);
    iowrite32(u4RegValue,MT6516IMGDMA0_IRT0_MO_0_CON);

    u4RegValue = ((u32)IRT0Cfg.bToPRZ << 1) + ((u32)IRT0Cfg.bToMP4DeBlk << 2);
    if( VDODEC_SCANLINE == IRT0Cfg.eSrc)
    {
        u4RegValue += (1 << 8);
    }

    if( IRT0Cfg.bToCRZ | IRT0Cfg.bToIPP1 )
    {
        u4RegValue += 1;
    }

    iowrite32(u4RegValue,MT6516IMGDMA1_IRT0_MO_1_CON);

    IDPPARMDB("Config IRT0 done \n");

    return 0;
}

#define MT6516IMGDMA0_IRT1_STR (IMGDMA_BASE + 0xD00)
#define MT6516IMGDMA0_IRT1_CON (IMGDMA_BASE + 0xD04)
#define MT6516IMGDMA0_IRT1_ALPHA (IMGDMA_BASE + 0xD08)
#define MT6516IMGDMA0_IRT1_SRC_XSIZE (IMGDMA_BASE + 0xD10)
#define MT6516IMGDMA0_IRT1_SRC_YSIZE (IMGDMA_BASE + 0xD14)
#define MT6516IMGDMA0_IRT1_BASE_ADDR0 (IMGDMA_BASE + 0xD20)
#define MT6516IMGDMA0_IRT1_BASE_ADDR1 (IMGDMA_BASE + 0xD24)
#define MT6516IMGDMA0_IRT1_BASE_ADDR2 (IMGDMA_BASE + 0xD28)
#define MT6516IMGDMA0_IRT1_BKGD_XSIZE0 (IMGDMA_BASE + 0xD30)
#define MT6516IMGDMA0_IRT1_BKGD_XSIZE1 (IMGDMA_BASE + 0xD34)
#define MT6516IMGDMA0_IRT1_BKGD_XSIZE2 (IMGDMA_BASE + 0xD38)
#define MT6516IMGDMA0_IRT1_FIFO_BASE (IMGDMA_BASE + 0xD40)
#define MT6516IMGDMA0_IRT1_RX_XCNT (IMGDMA_BASE + 0xD50)
#define MT6516IMGDMA0_IRT1_RX_YCNT (IMGDMA_BASE + 0xD54)
#define MT6516IMGDMA0_IRT1_HORI_CNT (IMGDMA_BASE + 0xD60)
#define MT6516IMGDMA0_IRT1_VERT_CNT (IMGDMA_BASE + 0xD64)
static int MT6516IDP_ConfigIRT1(void * a_pCfg)
{
    stIRT1Cfg IRT1Cfg;
    u32 u4RegValue = 0;
    u32 u4Mask = 0;
    u32 u4BufferSize = 0;
    u32 u4Addr = 0;

    if(copy_from_user(&IRT1Cfg,a_pCfg,sizeof(stIRT1Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigIRT1, copy from user failed\n");
        return -EFAULT;
    }

    //Check setting
    //Rule 1. Base addr must be 2x aligned if RGB565, 4x aligned if RGB8888
    u4Mask = (MT6516IRT1_OUTFMT_RGB888 == IRT1Cfg.eFmt ? 0 : 0x3);
    if(IRT1Cfg.u4DestBufferPhysAddr0 & u4Mask)
    {
        IDPDB("[MT6516IDP] ConfigIRT1, the base address is not aligned\n");
        return -EINVAL;
    }

    u4RegValue = (u32)IRT1Cfg.u2SrcImgWidth*IRT1Cfg.u2SrcImgHeight*(IRT1Cfg.eFmt == MT6516IRT1_OUTFMT_RGB565 ? 2 : 3);
    if(!is_pmem_range((unsigned long *)IRT1Cfg.u4DestBufferPhysAddr0 , (unsigned long)u4RegValue))
    {
        IDPDB("[MT6516IDP] Fetal error !! destination address is not inside of pmem range!!! PID : %d , Address : 0x%x \n" ,
        	current->pid, IRT1Cfg.u4DestBufferPhysAddr0);
        return -EINVAL;
    }

    if((1 == IRT1Cfg.bAutoRestart) && (IRT1Cfg.u4DestBufferPhysAddr1 & u4Mask))
    {
        IDPDB("[MT6516IDP] ConfigIRT1, the base address is not aligned\n");
        return -EINVAL;        
    }

    if((1 == IRT1Cfg.bAutoRestart) && (1 == IRT1Cfg.bTripleBuffer) && (IRT1Cfg.u4DestBufferPhysAddr2 & u4Mask))
    {
        IDPDB("[MT6516IDP] ConfigIRT1, Base address miss aligned\n");
        return -EINVAL;        
    }

    //Fill registers.
    // Allocate SRAM for rotation line buffer
    // Size : IRT1Cfg.u2SrcImgWidthx32 or IRT1Cfg.u2SrcImgWidthx64, 8x alignment
    // within 0x40000000~0x40017fff
    // sys1
    if(IRT1Cfg.bRotate & 0x1)//90 or 270 degree
    {
        if(0 != g_stSRAMMap[2].u4PhysicalAddr)
        {
            free_internal_sram(INTERNAL_SRAM_IDP_IRT1 , g_stSRAMMap[2].u4PhysicalAddr);
            g_stSRAMMap[2].u4PhysicalAddr = 0;
            g_stSRAMMap[2].u4Size = 0;
        }

        u4BufferSize = 64*IRT1Cfg.u2SrcImgWidth;
        
        u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_IRT1 , u4BufferSize, 8);
        if(0 == u4Addr)
        {
            return -EFAULT;
        }
        g_stSRAMMap[2].u4PhysicalAddr = u4Addr;
        g_stSRAMMap[2].u4Size = u4BufferSize;

#if SRAM_WKAROUND
//sys1, size :64x1280
//    pAddr = (u32 *)0x40000000;
        u4Addr = 0x40000000;//From Jackal
//0x4000_0000~0x4001_4000
//0x4002_A000~

//sys1 : 0x4000_0000~0x4001_7FFF
//sys2 : 0x4002_0000~0x4004_3FFF
#endif

        iowrite32(u4Addr,MT6516IMGDMA0_IRT1_FIFO_BASE);
    }

//    u4RegValue = (u32)IRT1Cfg.bInterrupt + ((u32)IRT1Cfg.bAutoRestart << 1) + ((u32)IRT1Cfg.bTripleBuffer << 2) + 
    u4RegValue = 1 + ((u32)IRT1Cfg.bAutoRestart << 1) + ((u32)IRT1Cfg.bTripleBuffer << 2) + 
        ((u32)IRT1Cfg.bPitch << 4) + ((u32)IRT1Cfg.bRotate << 5) + ((u32)IRT1Cfg.bFlip << 7) + (IRT1Cfg.eFmt << 8);

    //If SRAM support double buffer, 
    //TODO
    //u4RegValue += (1<<10);

    iowrite32(u4RegValue,MT6516IMGDMA0_IRT1_CON);
    iowrite32(IRT1Cfg.uAlpha,MT6516IMGDMA0_IRT1_ALPHA);
    iowrite32(IRT1Cfg.u2SrcImgWidth,MT6516IMGDMA0_IRT1_SRC_XSIZE);
    iowrite32(IRT1Cfg.u2SrcImgHeight,MT6516IMGDMA0_IRT1_SRC_YSIZE);
    iowrite32(IRT1Cfg.u4DestBufferPhysAddr0,MT6516IMGDMA0_IRT1_BASE_ADDR0);
    iowrite32(IRT1Cfg.u4DestBufferPhysAddr1,MT6516IMGDMA0_IRT1_BASE_ADDR1);
    iowrite32(IRT1Cfg.u4DestBufferPhysAddr2,MT6516IMGDMA0_IRT1_BASE_ADDR2);
    iowrite32(IRT1Cfg.u2PitchImg0Width,MT6516IMGDMA0_IRT1_BKGD_XSIZE0);
    iowrite32(IRT1Cfg.u2PitchImg1Width,MT6516IMGDMA0_IRT1_BKGD_XSIZE1);
    iowrite32(IRT1Cfg.u2PitchImg2Width,MT6516IMGDMA0_IRT1_BKGD_XSIZE2);

    if(IRT1Cfg.bAutoRestart)
    {
        if(IRT1Cfg.bTripleBuffer)
        {
            IRT1_BUFFIND(SET_BUFF , NULL , 3);
        }
        else
        {
            IRT1_BUFFIND(SET_BUFF , NULL , 2);
        }
    }else
    {
        IRT1_BUFFIND(SET_BUFF , NULL , 1);
    }
    
    IDPPARMDB("Config IRT1 done \n");

    return 0;
}

#define MT6516IMGDMA0_IRT3_STR (IMGDMA_BASE + 0xF00)
#define MT6516IMGDMA0_IRT3_CON (IMGDMA_BASE + 0xF04)
#define MT6516IMGDMA0_IRT3_ALPHA (IMGDMA_BASE + 0xF08)
#define MT6516IMGDMA0_IRT3_SRC_XSIZE (IMGDMA_BASE + 0xF10)
#define MT6516IMGDMA0_IRT3_SRC_YSIZE (IMGDMA_BASE + 0xF14)
#define MT6516IMGDMA0_IRT3_BASE_ADDR0 (IMGDMA_BASE + 0xF20)
#define MT6516IMGDMA0_IRT3_BASE_ADDR1 (IMGDMA_BASE + 0xF24)
#define MT6516IMGDMA0_IRT3_BASE_ADDR2 (IMGDMA_BASE + 0xF28)
#define MT6516IMGDMA0_IRT3_BKGD_XSIZE0 (IMGDMA_BASE + 0xF30)
#define MT6516IMGDMA0_IRT3_BKGD_XSIZE1 (IMGDMA_BASE + 0xF34)
#define MT6516IMGDMA0_IRT3_BKGD_XSIZE2 (IMGDMA_BASE + 0xF38)
#define MT6516IMGDMA0_IRT3_FIFO_BASE (IMGDMA_BASE + 0xF40)
#define MT6516IMGDMA0_IRT3_RX_XCNT (IMGDMA_BASE + 0xF50)
#define MT6516IMGDMA0_IRT3_RX_YCNT (IMGDMA_BASE + 0xF54)
#define MT6516IMGDMA0_IRT3_HORI_CNT (IMGDMA_BASE + 0xF60)
#define MT6516IMGDMA0_IRT3_VERT_CNT (IMGDMA_BASE + 0xF64)
static int MT6516IDP_ConfigIRT3(void * a_pCfg)
{
    stIRT3Cfg IRT3Cfg;
    u32 u4RegValue = 0;
    u32 u4Mask = 0;
    u32 u4BufferSize = 0;
    u32 u4Addr = 0;

    if(copy_from_user(&IRT3Cfg,a_pCfg,sizeof(stIRT3Cfg)))
    {
        IDPDB("[MT6516IDP] ConfigIRT3, copy from user failed\n");
        return -EFAULT;
    }

    //Check setting
    //Rule 1. Base addr must be 2x aligned if RGB565, 4x aligned if RGB8888
    u4Mask = (MT6516IRT3_OUTFMT_RGB888 == IRT3Cfg.eFmt ? 0x3 : 0x1);
    if(IRT3Cfg.u4DestBufferPhysAddr0 & u4Mask)
    {
        IDPDB("[MT6516IDP] ConfigIRT3, the base address is not aligned\n");
        return -EINVAL;
    }

    if((1 == IRT3Cfg.bAutoRestart) && (IRT3Cfg.u4DestBufferPhysAddr1 & u4Mask))
    {
        IDPDB("[MT6516IDP] ConfigIRT3, the base address is not aligned\n");
        return -EINVAL;        
    }

    if((1 == IRT3Cfg.bAutoRestart) && (1 == IRT3Cfg.bTripleBuffer) && (IRT3Cfg.u4DestBufferPhysAddr2 & u4Mask))
    {
        IDPDB("[MT6516IDP] ConfigIRT3, Base address miss aligned\n");
        return -EINVAL;        
    }

    //Fill registers.
    // Allocate SRAM for rotation line buffer
    // Size : IRT1Cfg.u2SrcImgWidthx32 or IRT1Cfg.u2SrcImgWidthx64, 8x alignment
    // within 0x40000000~0x40017fff
    // sys1
    if(IRT3Cfg.bRotate & 0x1)//90 or 270 degree
    {
        if(0 != g_stSRAMMap[3].u4PhysicalAddr)
        {
            free_internal_sram(INTERNAL_SRAM_IDP_IRT3 , g_stSRAMMap[3].u4PhysicalAddr);
            g_stSRAMMap[3].u4PhysicalAddr = 0;
            g_stSRAMMap[3].u4Size = 0;
        }

        u4BufferSize = IRT3Cfg.u2SrcImgWidth*64;

        u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_IRT3 , u4BufferSize, 8);
        if(0 == u4Addr)
        {
            return -EFAULT;
        }

        g_stSRAMMap[3].u4Size = u4BufferSize;
        g_stSRAMMap[3].u4PhysicalAddr = u4Addr;

        if(u4Addr & 0x7)
        {
            return -EINVAL;
        }

#if SRAM_WKAROUND
//sys1, size : share with PRZ
    u4Addr = 0x40000000;
//0x4001_4000~
//0x4002_A000~

//sys1 : 0x4000_0000~0x4001_7FFF
//sys2 : 0x4002_0000~0x4004_3FFF
#endif

        iowrite32(u4Addr , MT6516IMGDMA0_IRT3_FIFO_BASE);
    }

    //Fill registers.
    u4RegValue = ((u32)IRT3Cfg.bAutoRestart << 1) + ((u32)IRT3Cfg.bTripleBuffer << 2) + ((u32)IRT3Cfg.bPitch << 4) +
        ((u32)IRT3Cfg.bRotate << 5) + ((u32)IRT3Cfg.bFlip << 7) + ((u32)IRT3Cfg.eFmt << 8);
    //If SRAM support double buffer, 
    u4RegValue += (1<<10);
    iowrite32(u4RegValue,MT6516IMGDMA0_IRT3_CON);
    iowrite32(IRT3Cfg.uAlpha,MT6516IMGDMA0_IRT3_ALPHA);
    iowrite32(IRT3Cfg.u2SrcImgWidth,MT6516IMGDMA0_IRT3_SRC_XSIZE);
    iowrite32(IRT3Cfg.u2SrcImgHeight,MT6516IMGDMA0_IRT3_SRC_YSIZE);
    iowrite32(IRT3Cfg.u4DestBufferPhysAddr0,MT6516IMGDMA0_IRT3_BASE_ADDR0);
    iowrite32(IRT3Cfg.u4DestBufferPhysAddr1,MT6516IMGDMA0_IRT3_BASE_ADDR1);
    iowrite32(IRT3Cfg.u4DestBufferPhysAddr2,MT6516IMGDMA0_IRT3_BASE_ADDR2);

    IDPPARMDB("Config IRT3 done \n");

    return 0;
}

#define MT6516IMGDMA1_STA (IMGDMA1_BASE)
#define MT6516IMGDMA1_ACKINT (IMGDMA1_BASE + 0x4)
#define MT6516IMGDMA1_GMCIF_STA (IMGDMA1_BASE + 0x20)

#define MT6516IMGDMA1_JPEG_STR (IMGDMA1_BASE + 0x100)
//#define MT6516IMGDMA1_JPEG_CON (IMGDMA1_BASE + 0x104)
#define MT6516IMGDMA1_JPEG_FIFO_BASE (IMGDMA1_BASE + 0x110)
#define MT6516IMGDMA1_JPEG_FIFOLEN (IMGDMA1_BASE + 0x114)
#define MT6516IMGDMA1_JPEG_XSIZE (IMGDMA1_BASE + 0x120)
#define MT6516IMGDMA1_JPEG_YSIZE (IMGDMA1_BASE + 0x124)
#define MT6516IMGDMA1_JPEG_WRPTR (IMGDMA1_BASE + 0x130)
#define MT6516IMGDMA1_JPEG_WRXCNT (IMGDMA1_BASE + 0x134)
#define MT6516IMGDMA1_JPEG_WRYCNT (IMGDMA1_BASE + 0x138)
#define MT6516IMGDMA1_JPEG_RDPTR (IMGDMA1_BASE + 0x140)
#define MT6516IMGDMA1_JPEG_RDXBLK_CNT (IMGDMA1_BASE + 0x144)
#define MT6516IMGDMA1_JPEG_RDYBLK_CNT (IMGDMA1_BASE + 0x148)
#define MT6516IMGDMA1_JPEG_FFCNT (IMGDMA1_BASE + 0x150)
#define MT6516IMGDMA1_JPEG_FFWRLIDX (IMGDMA1_BASE + 0x154)
#define MT6516IMGDMA1_JPEG_FFRDLIDX (IMGDMA1_BASE + 0x158)
static int MT6516IDP_ConfigJPEGDMA(void * a_pCfg)
{
    u32 u4RegValue = 0;
    stJPEGDMACfg JPEGDMACfg;
    u32 u4BufferSize = 0;
    u32 u4Addr = 0;
    int retVal = 0;

    if(copy_from_user(&JPEGDMACfg,a_pCfg,sizeof(stJPEGDMACfg)))
    {
        IDPDB("[MT6516IDP] ConfigJPEGDMA , copy from user failed\n");
        return -EFAULT;
    }

    if(0 != g_stSRAMMap[4].u4PhysicalAddr)
    {
        free_internal_sram(INTERNAL_SRAM_IDP_JPEG_DMA , g_stSRAMMap[4].u4PhysicalAddr);
        g_stSRAMMap[4].u4PhysicalAddr = 0;
        g_stSRAMMap[4].u4Size = 0;
    }

    //Allocate SRAM
    //address must be 8x aligned, length must be 8 lines / 16 lines(double buffer)
    //Gray : ((JPEGDMACfg.u2SrcImgWidth + 7) & (~0x7)) x (8 or 16)
    //YUV422 : ((JPEGDMACfg.u2SrcImgWidth + 15) & (~0xF)) x (8 or 16) x 2
    //YUV411 : ((JPEGDMACfg.u2SrcImgWidth + 31) & (~0x1F)) x (8 or 16) x 1.5
    //YUV420 : ((JPEGDMACfg.u2SrcImgWidth + 15) & (~0xF)) x (8 or 16) x 3
    //within 0x40020000~0x40043fff
    switch(JPEGDMACfg.eFmt)
    {
        case YUV422 :
            u4BufferSize = ((JPEGDMACfg.u2SrcImgWidth + 15) & (~0xF)) << 5;
        break;
        case GRAY :
            u4BufferSize = ((JPEGDMACfg.u2SrcImgWidth + 7) & (~0x7)) << 4;
        break;
        case YUV411 :
            u4BufferSize = ((JPEGDMACfg.u2SrcImgWidth + 31) & (~0x1F)) * 24;
        break;
        case YUV420 :
            u4BufferSize = ((JPEGDMACfg.u2SrcImgWidth + 15) & (~0xF)) * 48;
        break;
        default :
            IDPDB("[MT6516IDP] ConfigJPEGDMA , format error \n");
            retVal = -EFAULT;
        break;
    }

    if(retVal){return retVal;}    

    u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_JPEG_DMA , u4BufferSize, 8);
    if(0 == u4Addr)
    {
        u4BufferSize = (u4BufferSize >> 1);
        u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_JPEG_DMA , u4BufferSize, 8);
        if(0 == u4Addr)
        {
            return -ENOMEM;
        }
    }

    g_stSRAMMap[4].u4PhysicalAddr = u4Addr;
    g_stSRAMMap[4].u4Size = u4BufferSize;

    if(u4Addr & 0x7)
    {
        return -EFAULT;
    }


#if SRAM_WKAROUND
//sys2, size :48x2560
//    pAddr = (u32 *)0x40020000;
    u4Addr = 0x40020000;//From Jackal
//0x4000_0000~0x4001_4000
//0x4002_A000

//sys1 : 0x4000_0000~0x4001_7FFF
//sys2 : 0x4002_0000~0x4004_3FFF
#endif

    iowrite32(u4Addr,MT6516IMGDMA1_JPEG_FIFO_BASE);
    iowrite32(16 , MT6516IMGDMA1_JPEG_FIFOLEN);

    //Fill registers.
    u4RegValue = (JPEGDMACfg.eFmt << 1) + ((u32)JPEGDMACfg.bAutoRestart << 3) + (1 << 7);
    iowrite32(u4RegValue,MT6516IMGDMA1_JPEG_CON);
    iowrite32(JPEGDMACfg.u2SrcImgWidth,MT6516IMGDMA1_JPEG_XSIZE);
    iowrite32(JPEGDMACfg.u2SrcImgHeight,MT6516IMGDMA1_JPEG_YSIZE);

    return 0;
}

#define MT6516IMGDMA1_VDOENC_STR (IMGDMA1_BASE + 0x200)
#define MT6516IMGDMA1_VDOENC_CON (IMGDMA1_BASE + 0x204)
//#define MT6516IMGDMA1_VDOENC_Y_BASE1 (IMGDMA1_BASE + 0x210)
//#define MT6516IMGDMA1_VDOENC_U_BASE1 (IMGDMA1_BASE + 0x214)
//#define MT6516IMGDMA1_VDOENC_V_BASE1 (IMGDMA1_BASE + 0x218)
//#define MT6516IMGDMA1_VDOENC_Y_BASE2 (IMGDMA1_BASE + 0x220)
//#define MT6516IMGDMA1_VDOENC_U_BASE2 (IMGDMA1_BASE + 0x224)
//#define MT6516IMGDMA1_VDOENC_V_BASE2 (IMGDMA1_BASE + 0x228)
#define MT6516IMGDMA1_VDOENC_XSIZE (IMGDMA1_BASE + 0x230)
#define MT6516IMGDMA1_VDOENC_YSIZE (IMGDMA1_BASE + 0x234)
#define MT6516IMGDMA1_VDOENC_PXLNUM (IMGDMA1_BASE + 0x238)
//#define MT6516IMGDMA1_VDOENC_WXCNT (IMGDMA1_BASE + 0x240)
#define MT6516IMGDMA1_VDOENC_WYCNT (IMGDMA1_BASE + 0x244)
#define MT6516IMGDMA1_VDOENC_Y_XCNT (IMGDMA1_BASE + 0x250)
#define MT6516IMGDMA1_VDOENC_Y_YCNT (IMGDMA1_BASE + 0x254)
#define MT6516IMGDMA1_VDOENC_V_XCNT (IMGDMA1_BASE + 0x258)
#define MT6516IMGDMA1_VDOENC_V_YCNT (IMGDMA1_BASE + 0x25C)
#define MT6516IMGDMA1_VDOENC_DROP_FCNT (IMGDMA1_BASE + 0x260)
#define MT6516IMGDMA1_VDOENC_LB_BASE (IMGDMA1_BASE + 0x270)
#define MT6516IMGDMA1_VDOENC_LB_YLEN (IMGDMA1_BASE + 0x274)
#define MT6516IMGDMA1_VDOENC_LB_WR_XCNT (IMGDMA1_BASE + 0x278)
#define MT6516IMGDMA1_VDOENC_LB_WR_YCNT (IMGDMA1_BASE + 0x27C)
static int MT6516IDP_ConfigVDOENCDMA(void * a_pCfg)
{
    stVDOENCDMACfg VDOENCDMACfg;
    unsigned long u4RegValue = 0;
    u32 u4BufferSize = 0;
    u32 u4Addr = 0;
    unsigned long u4Size = 0;

    if(copy_from_user(&VDOENCDMACfg,a_pCfg,sizeof(stVDOENCDMACfg)))
    {
        IDPDB("[MT6516IDP] ConfigVDOENCDMA , copy from user failed\n");
        return -EFAULT;
    }
    //Check setting
    // Rules : image width and height must be 16x aligned, less than 4096
    // base address must 8x aligned.
    if((VDOENCDMACfg.u2SrcImgWidth & 0xF) || (VDOENCDMACfg.u2SrcImgHeight & 0xF) ||
    	(VDOENCDMACfg.u2SrcImgWidth > 4095) || (VDOENCDMACfg.u2SrcImgHeight > 4095) ||
    	(0 == VDOENCDMACfg.u2SrcImgWidth) || (0 == VDOENCDMACfg.u2SrcImgHeight))
    {
        IDPDB("[MT6516IDP] ConfigVDOENCDMA , input dimension is incorrect\n");
        return -EFAULT;
    }

    //Buffer count should not exceed.
    if(VDOENCDMACfg.u4BufferCnt > MT6516IDP_VDOENCDMA_MAXBUFF_CNT)
    {
        IDPDB("[MT6516IDP] ConfigVDOENCDMA , Buffer count exceeds\n");
    }

    //The reason is 3 pipeline stages - display, video encoding, camera.
    if((VDOENCDMACfg.u4BufferCnt < 3) && (1 == VDOENCDMACfg.bAutoRestart))
    {
        IDPDB("[MT6516IDP] ConfigVDOENCDMA , Continous mode, should use at least tripple buffers\n");
    }

    u4Size = (unsigned long)VDOENCDMACfg.u2SrcImgWidth*VDOENCDMACfg.u2SrcImgHeight;
    //Physical address must be 16x alignment
    for(u4RegValue = 0; u4RegValue < VDOENCDMACfg.u4BufferCnt ; u4RegValue += 1){
        if((VDOENCDMACfg.pu4DestYBuffPAArray[u4RegValue] & 0x7) || (VDOENCDMACfg.pu4DestUBuffPAArray[u4RegValue] & 0x7) ||
            (VDOENCDMACfg.pu4DestVBuffPAArray[u4RegValue] & 0x7))
        {
            IDPDB("[MT6516IDP] ConfigVDOENCDMA , %lu th Base address miss align\n", u4RegValue);
            return -EFAULT;
        }

        if(!is_pmem_range((unsigned long *)VDOENCDMACfg.pu4DestYBuffPAArray[u4RegValue] , u4Size))
        {
            IDPDB("[MT6516IDP] Fetal error !! VDOENCDMA destination address is not inside of pmem range!!! PID : %d , Address : 0x%x \n" , 
            	current->pid, VDOENCDMACfg.pu4DestYBuffPAArray[u4RegValue]);
            return -EFAULT;
        }

        if(!is_pmem_range((unsigned long *)VDOENCDMACfg.pu4DestUBuffPAArray[u4RegValue] , (u4Size >> 2)))
        {
            IDPDB("[MT6516IDP] Fetal error !! VDOENCDMA destination address is not inside of pmem range!!! PID : %d , Address : 0x%x \n" ,
            	current->pid, VDOENCDMACfg.pu4DestYBuffPAArray[u4RegValue]);
            return -EFAULT;
        }

        if(!is_pmem_range((unsigned long *)VDOENCDMACfg.pu4DestVBuffPAArray[u4RegValue] , (u4Size >> 2)))
        {
            IDPDB("[MT6516IDP] Fetal error !! VDOENCDMA destination address is not inside of pmem range!!! PID : %d , Address : 0x%x \n" ,
            	current->pid, VDOENCDMACfg.pu4DestYBuffPAArray[u4RegValue]);
            return -EFAULT;
        }
    }

    if(0 != g_stSRAMMap[5].u4PhysicalAddr)
    {
        free_internal_sram(INTERNAL_SRAM_IDP_VDO_ENC_WDMA , g_stSRAMMap[5].u4PhysicalAddr);
        g_stSRAMMap[5].u4PhysicalAddr = 0;
        g_stSRAMMap[5].u4Size = 0;
    }

    //Allocate SRAM, address must be 8x aligned, size : VDOENCDMACfg.u2SrcImgWidth x 4 x 2
    //TODO : check if more line buffer improve performance.
    //sys 2
    u4BufferSize = (VDOENCDMACfg.u2SrcImgWidth << 4);
    u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_VDO_ENC_WDMA , u4BufferSize, 8);
    if(0 == u4Addr)
    {
        return -ENOMEM;
    }

    g_stSRAMMap[5].u4PhysicalAddr = u4Addr;
    g_stSRAMMap[5].u4Size = u4BufferSize;

#if SRAM_WKAROUND
//sys2, size : 8x1280
//    pAddr = (u32 *)0x4002A000;
    u4Addr = 0x400354F0;//From Jackal
//0x4000_0000~0x4001_4000
//0x4002_A000~0x4002_C800

//sys1 : 0x4000_0000~0x4001_7FFF
//sys2 : 0x4002_0000~0x4004_3FFF
#endif

    iowrite32(u4Addr,MT6516IMGDMA1_VDOENC_LB_BASE);
    iowrite32(8,MT6516IMGDMA1_VDOENC_LB_YLEN);

    //Fill registers.
    //Interrupt always on
//    u4RegValue = VDOENCDMACfg.bInterrupt + ((u32)VDOENCDMACfg.bWriteTriggerRead << 1) + 
    u4RegValue = 1 + ((u32)VDOENCDMACfg.bWriteTriggerRead << 1) + 
    ((u32)VDOENCDMACfg.bAutoRestart << 2) + ((u32)VDOENCDMACfg.bRotate << 4) + 
    ((u32)VDOENCDMACfg.bFlip << 6) + (1 << 7);

    iowrite32(u4RegValue,MT6516IMGDMA1_VDOENC_CON);

    iowrite32(VDOENCDMACfg.pu4DestYBuffPAArray[0],MT6516IMGDMA1_VDOENC_Y_BASE1);
    iowrite32(VDOENCDMACfg.pu4DestUBuffPAArray[0],MT6516IMGDMA1_VDOENC_U_BASE1);
    iowrite32(VDOENCDMACfg.pu4DestVBuffPAArray[0],MT6516IMGDMA1_VDOENC_V_BASE1);

#if 0
    if(VDOENCDMACfg.bAutoRestart)
    {
        iowrite32(VDOENCDMACfg.pu4DestYBuffPAArray[1],MT6516IMGDMA1_VDOENC_Y_BASE2);
        iowrite32(VDOENCDMACfg.pu4DestUBuffPAArray[1],MT6516IMGDMA1_VDOENC_U_BASE2);
        iowrite32(VDOENCDMACfg.pu4DestVBuffPAArray[1],MT6516IMGDMA1_VDOENC_V_BASE2);
    }
#else
    iowrite32(VDOENCDMACfg.pu4DestYBuffPAArray[0],MT6516IMGDMA1_VDOENC_Y_BASE2);
    iowrite32(VDOENCDMACfg.pu4DestUBuffPAArray[0],MT6516IMGDMA1_VDOENC_U_BASE2);
    iowrite32(VDOENCDMACfg.pu4DestVBuffPAArray[0],MT6516IMGDMA1_VDOENC_V_BASE2);
#endif

    VDOENCDMA_BUFFIND(SET_YBUFF , VDOENCDMACfg.pu4DestYBuffPAArray , VDOENCDMACfg.u4BufferCnt);
    VDOENCDMA_BUFFIND(SET_UBUFF , VDOENCDMACfg.pu4DestUBuffPAArray , VDOENCDMACfg.u4BufferCnt);
    VDOENCDMA_BUFFIND(SET_VBUFF , VDOENCDMACfg.pu4DestVBuffPAArray , VDOENCDMACfg.u4BufferCnt);

    iowrite32(VDOENCDMACfg.u2SrcImgWidth,MT6516IMGDMA1_VDOENC_XSIZE);
    iowrite32(VDOENCDMACfg.u2SrcImgHeight,MT6516IMGDMA1_VDOENC_YSIZE);
    u4RegValue = (u32)VDOENCDMACfg.u2SrcImgWidth*(u32)VDOENCDMACfg.u2SrcImgHeight;
    iowrite32(u4RegValue,MT6516IMGDMA1_VDOENC_PXLNUM);

    IDPPARMDB("Config VDOENC done \n");

    return 0;
}

#define MT6516IMGDMA1_VDODEC_STR (IMGDMA1_BASE + 0x280)
#define MT6516IMGDMA1_VDODEC_CON (IMGDMA1_BASE + 0x284)
#define MT6516IMGDMA1_VDODEC_Y_BASE (IMGDMA1_BASE + 0x290)
#define MT6516IMGDMA1_VDODEC_U_BASE (IMGDMA1_BASE + 0x294)
#define MT6516IMGDMA1_VDODEC_V_BASE (IMGDMA1_BASE + 0x298)
#define MT6516IMGDMA1_VDODEC_XSIZE (IMGDMA1_BASE + 0x2A0)
#define MT6516IMGDMA1_VDODEC_YSIZE (IMGDMA1_BASE + 0x2A4)
#define MT6516IMGDMA1_VDODEC_PXLNUM (IMGDMA1_BASE + 0x2A8)
#define MT6516IMGDMA1_VDODEC_Y_XCNT (IMGDMA1_BASE + 0x2B0)
#define MT6516IMGDMA1_VDODEC_Y_YCNT (IMGDMA1_BASE + 0x2B4)
#define MT6516IMGDMA1_VDODEC_V_XCNT (IMGDMA1_BASE + 0x2B8)
#define MT6516IMGDMA1_VDODEC_V_YCNT (IMGDMA1_BASE + 0x2BC)
static int MT6516IDP_ConfigVDODECDMA(void * a_pCfg)
{
    u32 u4RegValue = 0;
    stVDODECDMACfg VDODECDMACfg;

    if(copy_from_user(&VDODECDMACfg,a_pCfg,sizeof(stVDODECDMACfg)))
    {
        IDPDB("[MT6516IDP] ConfigVDODECDMA , copy from user failed\n");
        return -EFAULT;
    }

    //Check setting
    // Rules : input dimension must be 16x , < 1024
    // Address must be 8x aligned 
    if((VDODECDMACfg.u2SrcImgWidth & 0xF) || (VDODECDMACfg.u2SrcImgHeight & 0xF) ||
    	(VDODECDMACfg.u2SrcImgWidth > 1023) || (VDODECDMACfg.u2SrcImgHeight > 1023) ||
    	(0 == VDODECDMACfg.u2SrcImgWidth) || (0 == VDODECDMACfg.u2SrcImgHeight))
    {
        IDPDB("[MT6516IDP] ConfigVDOENCDMA , input dimension is incorrect\n");
        return -EFAULT;
    }
    if((VDODECDMACfg.u4SrcYBufferPhyAddr & 0x7) || (VDODECDMACfg.u4SrcUBufferPhyAddr & 0x7) ||
        (VDODECDMACfg.u4SrcVBufferPhyAddr & 0x7))
    {
        IDPDB("[MT6516IDP] ConfigVDODECDMA , Base address miss align\n");
        return -EFAULT;
    }

    //Fill registers.
    u4RegValue = ((u32)VDODECDMACfg.bRotate << 4) + ((u32)VDODECDMACfg.bFlip << 6) +
        ((u32)VDODECDMACfg.bScanMode << 7);
    iowrite32(u4RegValue,MT6516IMGDMA1_VDODEC_CON);
    iowrite32(VDODECDMACfg.u4SrcYBufferPhyAddr,MT6516IMGDMA1_VDODEC_Y_BASE);
    iowrite32(VDODECDMACfg.u4SrcUBufferPhyAddr,MT6516IMGDMA1_VDODEC_U_BASE);
    iowrite32(VDODECDMACfg.u4SrcVBufferPhyAddr,MT6516IMGDMA1_VDODEC_V_BASE);
    iowrite32(VDODECDMACfg.u2SrcImgWidth,MT6516IMGDMA1_VDODEC_XSIZE);
    iowrite32(VDODECDMACfg.u2SrcImgHeight,MT6516IMGDMA1_VDODEC_YSIZE);
    u4RegValue = ((u32)VDODECDMACfg.u2SrcImgWidth)*((u32)VDODECDMACfg.u2SrcImgHeight);
    iowrite32(u4RegValue,MT6516IMGDMA1_VDODEC_PXLNUM);

    IDPPARMDB("Config VDODEC done \n");

    return 0;
}

static void AccessMT6516_IDP_REG_BACK(MT6516_IDP_REG ** pReg);

#define MT6516MP4DEBLK_COMD MP4_DEBLK_BASE
#define MT6516MP4DEBLK_CONF (MP4_DEBLK_BASE + 0x4)
#define MT6516MP4DEBLK_STS (MP4_DEBLK_BASE + 0x8)
#define MT6516MP4DEBLK_IRQ_STS (MP4_DEBLK_BASE + 0xC)
#define MT6516MP4DEBLK_IRQ_ACK (MP4_DEBLK_BASE + 0x10)
#define MT6516MP4DEBLK_LIN_BUFF_ADDR (MP4_DEBLK_BASE + 0x14)
#define MT6516MP4DEBLK_QUANT_ADDR (MP4_DEBLK_BASE + 0x18)
static int MT6516IDP_ConfigMP4DBLK(void * a_pCfg)
{
    stMP4DBLKCfg MP4DBLKCfg;
    u32 u4RegValue = 0;
    u32 u4Addr = 0;
    u32 u4BuffSize = 0;
    u32 u4X_limit, u4Y_limit, u4MB_X;

    MT6516_IDP_REG * pReg = NULL;

    AccessMT6516_IDP_REG_BACK(&pReg);
    if(NULL == pReg)
    {
        IDPDB("[MT6516IDP] Config MP4DBLK ptr error!!\n");
        return -EFAULT;
    }
    
    if(copy_from_user(&MP4DBLKCfg , a_pCfg , sizeof(stMP4DBLKCfg)))
    {
        IDPDB("[MT6516IDP] ConfigMP4DBLK , copy from user failed\n");
        return -EFAULT;
    }

    //Rule 1 . input width height must be < 4096
    if((MP4DBLKCfg.u2Width > 4096) || (MP4DBLKCfg.u2Height > 4096))
    {
        IDPDB("[MT6516IDP] ConfigMP4DBLK , input diemnsion exceeds range \n");
        return -EINVAL;
    }

    if(TRUE != hwEnableClock(MT6516_PDN_MM_MP4DBLK, "IDP"))
    {
        IDPDB("enable MP4 DBLK failed \n");
        return -EIO;
    }

    //Reset HW
    iowrite32(0x3,MT6516MP4DEBLK_COMD);
    iowrite32(0,MT6516MP4DEBLK_COMD);    

    //Config registers
    //Allocate SRAM for MP4 deblock , 12xwidth
    if(0 != g_stSRAMMap[6].u4PhysicalAddr)
    {
        free_internal_sram(INTERNAL_SRAM_IDP_MP4DEBLK , g_stSRAMMap[6].u4PhysicalAddr);
        g_stSRAMMap[6].u4PhysicalAddr = 0;
        g_stSRAMMap[6].u4Size = 0;
    }

    u4BuffSize = MP4DBLKCfg.u2Width*12;
    u4Addr = alloc_internal_sram(INTERNAL_SRAM_IDP_MP4DEBLK , u4BuffSize, 4);
    if(0 == u4Addr)
    {
        return -ENOMEM;
    }

    g_stSRAMMap[6].u4PhysicalAddr = u4Addr;
    g_stSRAMMap[6].u4Size = u4BuffSize;
#if SRAM_WKAROUND
//Size : 1280*12
    u4Addr = 0x400354F0;//From Jackal
#endif
    iowrite32(u4Addr , MT6516MP4DEBLK_LIN_BUFF_ADDR);
    pReg->MP4DBLKReg.u4MT6516MP4DEBLK_LIN_BUFF_ADDR = u4Addr;

    u4X_limit = (MP4DBLKCfg.u2Width + 15) >> 4;
    u4Y_limit = (MP4DBLKCfg.u2Height + 15) >> 4;
    u4MB_X = ((u4X_limit + 3) & (~0x3));

    u4RegValue = ((u32)MP4DBLKCfg.bToIPP << 26) + ((u32)MP4DBLKCfg.bToPRZ << 25) + ((u32)MP4DBLKCfg.bToCRZ << 24)
    + (u4X_limit << 8) + (u4Y_limit << 16) +
    + ((u32)MP4DBLKCfg.bFlip << 4) + ((u32)MP4DBLKCfg.uRotate << 2) + 2 + MP4DBLKCfg.bInterrupt;
    iowrite32(u4RegValue , MT6516MP4DEBLK_CONF);
    pReg->MP4DBLKReg.u4MT6516MP4DEBLK_CONF = u4RegValue;

    //
    if(((0 == MP4DBLKCfg.bFlip) && (0 == MP4DBLKCfg.uRotate)) || 
    	((1 == MP4DBLKCfg.bFlip) && (1 == MP4DBLKCfg.uRotate)))
    {
        u4RegValue = MP4DBLKCfg.u4QuantizerAddr;
    }

    if(((0 == MP4DBLKCfg.bFlip) && (1 == MP4DBLKCfg.uRotate)) || 
    	((1 == MP4DBLKCfg.bFlip) && (2 == MP4DBLKCfg.uRotate)))
    {
        u4RegValue = (MP4DBLKCfg.u4QuantizerAddr + u4MB_X*(u4Y_limit - 1));
    }

    if(((0 == MP4DBLKCfg.bFlip) && (2 == MP4DBLKCfg.uRotate)) || 
    	((1 == MP4DBLKCfg.bFlip) && (3 == MP4DBLKCfg.uRotate)))
    {
        u4RegValue = (MP4DBLKCfg.u4QuantizerAddr + u4MB_X*(u4Y_limit - 1) + u4X_limit - 1);
    }

    if(((0 == MP4DBLKCfg.bFlip) && (3 == MP4DBLKCfg.uRotate)) || 
    	((1 == MP4DBLKCfg.bFlip) && (0 == MP4DBLKCfg.uRotate)))
    {
        u4RegValue = (MP4DBLKCfg.u4QuantizerAddr + u4X_limit - 1);
    }
    
    iowrite32(u4RegValue , MT6516MP4DEBLK_QUANT_ADDR);
    pReg->MP4DBLKReg.u4MT6516MP4DEBLK_QUANT_ADDR = u4RegValue;

    if(TRUE != hwDisableClock(MT6516_PDN_MM_MP4DBLK, "IDP"))
    {
        IDPDB("enable MP4 DBLK failed \n");
        return -EIO;
    }

    return 0;
}

typedef int (*MT6516IDP_ConfigFunc)(void *);

//Be noticed the sequence must follow eMT6516IDP_NODES sequence.
static MT6516IDP_ConfigFunc g_MT6516IDPConfigFuncs[IDP_ConfigFuncCnt] = 
{
    MT6516IDP_ConfigCRZ,
    MT6516IDP_ConfigPRZ,
    MT6516IDP_ConfigDRZ,
    MT6516IDP_ConfigIPP1_Y2R0_IPP2,
    MT6516IDP_ConfigR2Y0,
    MT6516IDP_ConfigY2R1,
    MT6516IDP_ConfigIBW1,
    MT6516IDP_ConfigIBW2,
    MT6516IDP_ConfigIBR1,
    MT6516IDP_ConfigOVL,
    MT6516IDP_ConfigIRT0,
    MT6516IDP_ConfigIRT1,
    MT6516IDP_ConfigIRT3,
    MT6516IDP_ConfigJPEGDMA,
    MT6516IDP_ConfigVDOENCDMA,
    MT6516IDP_ConfigVDODECDMA,
    MT6516IDP_ConfigMP4DBLK
};

static int MT6516IDP_ConfigHW(unsigned long a_pNodeCfg)
{
    stNodeCfg NodeCfg;
    stNodeCfg * pNextNodeCfg;
    MT6516IDP_ConfigFunc * pFunc;
    int i4RetVal = 0;
    pNextNodeCfg = (stNodeCfg *)a_pNodeCfg;

    while(NULL != pNextNodeCfg){

        //Check if locked
        //TODO

        if(copy_from_user(&NodeCfg, pNextNodeCfg , sizeof(stNodeCfg))){
            IDPDB("[MT6516_IDP] ConfigHW, Copy from user failed 1 \n");
            return -EFAULT;
        }

        //Configure registers
        pFunc = &g_MT6516IDPConfigFuncs[NodeCfg.eNodeType];

        if(NULL == pFunc)
        {
            IDPDB("[MT6516_IDP] ConfigHW, IDP config func is not linked\n");
            return -EFAULT;
        }

        i4RetVal = (*pFunc)(NodeCfg.pNodeCfg);

        if(i4RetVal){
            break;
        }
    
        pNextNodeCfg = (stNodeCfg *)NodeCfg.pNextCfg;

    }

//TODO : auto generate the configuration
    // 
    // 1st rule : IRT0 DMA must cowork with VDOENC RDMA or VDODEC DMA, so the setting must be the same.
    // 2nd rule : 

    return i4RetVal;
}
   
//MT6516CRZ_CON
//MT6516PRZ_CON
//MT6516DRZ_STR
//MT6516IMPROC_EN
//MT6516IMGDMA0_IBW1_STR
//MT6516IMGDMA0_IBW2_STR
//MT6516IMGDMA0_IBR1_STR
//MT6516IMGDMA1_OVL_MO_1_STR
//MT6516IMGDMA0_OVL_MO_0_STR
//MT6516IMGDMA0_OVL_STR
//OVL DMA MO 1 -> OVL DMA MO 0 -> OVL DMA
//MT6516IMGDMA0_IRT0_MO_0_STR
//MT6516IMGDMA1_IRT0_MO_1_STR
//MT6516IMGDMA1_IRT0_STR
//Start IRT0_MO_0 -> IRT0_MO_1 -> IRT0 DMA/VDODEC DMA
//MT6516IMGDMA0_IRT1_STR
//MT6516IMGDMA0_IRT3_STR
//MT6516IMGDMA1_JPEG_STR
#define MT6516JPG_ENC_CTL (JPEG_BASE + 0x104)
//MT6516IMGDMA1_VDOENC_STR
//MT6516IMGDMA1_VDODEC_STR
//CRZ interrupt it the most useful interrupt.
//Create a interrupt handler to return ack to IMGDMA0_ACKINT,IMGDMA1_ACKINT
//MT6516IMGDMA0_ACKINT
//MT6516IMGDMA1_ACKINT
//#define MT6516IDP_G1MEMPDN (CONFIG_BASE + 0x30C)
void MT6516IDP_DUMPREG(void);
static int MT6516_EnableChainInternal(eMT6516IDP_NODES * peChain , unsigned long a_u4Cnt )
{
    u32 u4Index = 0;
    u32 u4RegValue = 0;

    //Enable the clock first
    for(u4Index = 0;u4Index < a_u4Cnt; u4Index += 1)
    {
        switch(peChain[u4Index])
        {
            case IDP_CRZ :
                //power on sram
                u4RegValue = ioread32(MT6516IDP_G1MEMPDN);
                u4RegValue &= (~(1<<8));
                iowrite32(u4RegValue , MT6516IDP_G1MEMPDN);
                if(hwEnableClock(MT6516_PDN_MM_CRZ,"IDP") != TRUE || 
                	hwEnableClock(MT6516_PDN_MM_RESZLB,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable CRZ clock failed \n");
                    return -EIO;
                }
    
            break;

            case IDP_PRZ :
                if(hwEnableClock(MT6516_PDN_MM_PRZ,"IDP") != TRUE 
                	||hwEnableClock(MT6516_PDN_MM_PRZ2,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable PRZ clock failed \n");
                    return -EIO;
                }
            break;

            case IDP_DRZ :
                if(hwEnableClock(MT6516_PDN_MM_DRZ,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable DRZ clock failed \n");
                    return -EIO;
                }
            break;

            case IDP_IPP1_Y2R0_IPP2 :
            case IDP_R2Y0 :
            case IDP_Y2R1 :
                if(hwEnableClock(MT6516_PDN_MM_IPP,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable IPP clock failed \n");
                    return -EIO;
                }
            break;

            case IDP_IBW1 :
            case IDP_IBW2 :
            case IDP_IBR1 :
            case IDP_IRT1 :
            case IDP_IRT3 :
                if(hwEnableClock(MT6516_PDN_MM_IMGDMA0,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable IMGDMA0 clock failed \n");
                    return -EIO;
                }
            break;

            case IDP_OVL : // 
            case IDP_IRT0 : // 
                if(hwEnableClock(MT6516_PDN_MM_IMGDMA0,"IDP") != TRUE
                || hwEnableClock(MT6516_PDN_MM_IMGDMA1,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable IMGDMA0/1 clock failed \n");
                    return -EIO;
                }
            break;

            case IDP_JPEGDMA :
                if(hwEnableClock(MT6516_PDN_MM_JPEG,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable JPEG DMA clock failed \n");
                    return -EIO;
                }
            case IDP_VDOENCDMA :
            case IDP_VDODECDMA :
                if(hwEnableClock(MT6516_PDN_MM_IMGDMA1,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable IMGDMA1 clock failed \n");
                    return -EIO;
                }
            break;

            case IDP_MP4DBLK :
                if(hwEnableClock(MT6516_PDN_MM_MP4DBLK,"IDP") != TRUE)
                {
                    IDPDB("[MT6516IDP] enable MP4De block clock failed \n");
                    return -EIO;
                }                	
            break;

            default :
            break;
        }
    }

    spin_lock_irq(&g_IDPLock);
    //Enable each module
    for(u4Index = 0;u4Index < a_u4Cnt; u4Index += 1)
    {
        switch(peChain[u4Index])
        {
            case IDP_CRZ :
                g_u4IRQTbl &= (~CRZ_IRQ);
                iowrite32(0x1,MT6516CRZ_CON);
                IDPPARMDB("enable CRZ\n");
            break;
            case IDP_PRZ :
                g_u4IRQTbl &= (~PRZ_IRQ);
                if(PRZSRC_JPGDEC == (ioread32(MT6516PRZ_CFG) & 0xF)){
                    iowrite32(0x1,MT6516PRZ_CON);
                }
                else
                {
                    iowrite32(0x6,MT6516PRZ_CON);
                }
                IDPPARMDB("enable PRZ\n");
            break;
            case IDP_DRZ :
                g_u4IRQTbl &= (~DRZ_IRQ);
                iowrite32(1,MT6516DRZ_STR);
                IDPPARMDB("enable DRZ\n");
            break;
            case IDP_IPP1_Y2R0_IPP2 :
                u4RegValue = ioread32(MT6516IMPROC_EN);
                u4RegValue |=0x1001;
                iowrite32(u4RegValue,MT6516IMPROC_EN);
                IDPPARMDB("enable IPP\n");
            break;
            case IDP_R2Y0 :
                u4RegValue = ioread32(MT6516IMPROC_EN);
                u4RegValue |= 0x10;
                iowrite32(u4RegValue,MT6516IMPROC_EN);
                IDPPARMDB("enable R2Y0\n");
            break;
            case IDP_Y2R1 :
                u4RegValue = ioread32(MT6516IMPROC_EN);
                u4RegValue |= 0x100;
                iowrite32(u4RegValue,MT6516IMPROC_EN);
                IDPPARMDB("enable Y2R1\n");
            break;
            case IDP_IBW1 :
                g_u4IRQTbl &= (~IBW1_IRQ);
                iowrite32(1,MT6516IMGDMA0_IBW1_STR);
                IDPPARMDB("enable IBW1\n");
            break;
            case IDP_IBW2 :
                g_u4IRQTbl &= (~IBW2_IRQ);
                iowrite32(1,MT6516IMGDMA0_IBW2_STR);
                IDPPARMDB("enable IBW2\n");
            break;
            case IDP_IBR1 :
                g_u4IRQTbl &= (~IBR1_IRQ);
                iowrite32(1,MT6516IMGDMA0_IBR1_STR);
                IDPPARMDB("enable IBR1\n");
            break;
            case IDP_OVL :
                g_u4IRQTbl &= (~OVL_IRQ);
                iowrite32(1,MT6516IMGDMA1_OVL_MO_1_STR);
                iowrite32(1,MT6516IMGDMA0_OVL_MO_0_STR);
               u4RegValue = ioread32(MT6516IMGDMA0_OVL_CON);
               if((u4RegValue & 0x10))
               {
                   iowrite32(1, MT6516IMGDMA0_OVL_STR);
               }
                IDPPARMDB("enable OVL\n");
            break;
            case IDP_IRT0 :
                g_u4IRQTbl &= (~IRT0_IRQ);
                iowrite32(1,MT6516IMGDMA0_IRT0_MO_0_STR);
                iowrite32(1,MT6516IMGDMA1_IRT0_MO_1_STR);
                u4RegValue = ioread32(MT6516IMGDMA1_IRT0_MO_1_CON);
                if(u4RegValue & 0x100){
                    iowrite32(0,MT6516IMGDMA1_IRT0_STR);}
                else{
                    iowrite32(1,MT6516IMGDMA1_IRT0_STR);
                }
                IDPPARMDB("enable IRT0\n");
            break;
            case IDP_IRT1 :
                g_u4IRQTbl &= (~IRT1_IRQ);
                iowrite32(1,MT6516IMGDMA0_IRT1_STR);
                IDPPARMDB("enable IRT1\n");
                IRT1_BUFFIND(RESET_BUFF , NULL , 0);
            break;
            case IDP_IRT3 :
                g_u4IRQTbl &= (~IRT3_IRQ);
                iowrite32(1,MT6516IMGDMA0_IRT3_STR);
                IDPPARMDB("enable IRT3\n");
            break;
            case IDP_JPEGDMA :
                g_u4IRQTbl &= (~JPEGDMA_IRQ);
                iowrite32(1,MT6516IMGDMA1_JPEG_STR);
                u4RegValue = ioread32(MT6516JPG_ENC_CTL);
                u4RegValue |= 0x1;
                iowrite32(u4RegValue,MT6516JPG_ENC_CTL);
                IDPPARMDB("enable JPGDMA\n");
            break;
            case IDP_VDOENCDMA :
#ifdef FRAME_SYNC
//    iowrite32((1<<7),MT6516IMGDMA1_JPEG_CON);
//    iowrite32((1<<5),MT6516IMGDMA0_IBW2_CON);
//    iowrite32((1<<13),MT6516CRZ_CFG);
                u4RegValue = ioread32(MT6516CRZ_CFG);
                u4RegValue |= (1 << 13);
                iowrite32(u4RegValue , MT6516CRZ_CFG);
                u4RegValue = ioread32(MT6516IMGDMA1_JPEG_CON);
                u4RegValue |= (1<<7);
                iowrite32(u4RegValue , MT6516IMGDMA1_JPEG_CON);
                u4RegValue = ioread32(MT6516IMGDMA0_IBW2_CON);
                u4RegValue |= (1<<5);
                iowrite32(u4RegValue , MT6516IMGDMA0_IBW2_CON);
#endif
                g_u4IRQTbl &= (~VDOENCDMA_IRQ);
                iowrite32(1,MT6516IMGDMA1_VDOENC_STR);
                IDPPARMDB("enable VDOENCDMA\n");
                VDOENCDMA_BUFFIND(RESET_BUFF , NULL , 0);
            break;
            case IDP_VDODECDMA :
                g_u4IRQTbl &= (~VDODECDMA_IRQ);
                iowrite32(1,MT6516IMGDMA1_VDODEC_STR);
                IDPPARMDB("enable VDODECDMA\n");
            break;
            case IDP_MP4DBLK :
                g_u4IRQTbl &= (~MP4DBLK_IRQ);
                iowrite32(0x4,MT6516MP4DEBLK_COMD);
            break;
            default :
            break;
        }
    }
    spin_unlock_irq(&g_IDPLock);

    return 0;
}

inline static int MT6516IDP_EnableChain(unsigned long a_pstEnSeq)
{
    eMT6516IDP_NODES * peArray = NULL;
    u32 u4Length = 0;
    stEnableSeq EnableSeq;

    if(copy_from_user(&EnableSeq,(stEnableSeq *)a_pstEnSeq,sizeof(stEnableSeq)))
    {
        IDPDB("[MT6516_IDP] Copy from user failed!\n");
        return -EFAULT;
    }    

    u4Length = sizeof(eMT6516IDP_NODES)*EnableSeq.u4Length;

    peArray = kmalloc(u4Length,GFP_ATOMIC);

    if(NULL == peArray)
    {
        IDPDB("[MT6516_IDP] Enable chain , not enough memory!\n");
        return -EFAULT;
    }

    if(copy_from_user(peArray,EnableSeq.pEnableSeq,u4Length))
    {
        IDPDB("[MT6516_IDP] Copy from user failed!\n");
        return -EFAULT;
    }
    if(MT6516_EnableChainInternal(peArray , EnableSeq.u4Length))
    {
        IDPDB("[MT6516_IDP] Enable chain failed!\n");
        return -EIO;
    }

    kfree(peArray);
//ResetPingPongCounter();

    return 0;
}

//Dump reg - head

static void AccessMT6516_IDP_REG_BACK(MT6516_IDP_REG ** pReg)
{
    static MT6516_IDP_REG reg;
    *pReg = &reg;
}
//Only dump direct link register.
#define STOREREG_SUPPORT_RESOURCE (MT6516IDP_RES_VDODECDMA | MT6516IDP_RES_IRT0 | MT6516IDP_RES_IPP | \
          MT6516IDP_RES_IBW2 | MT6516IDP_RES_IRT1 | MT6516IDP_RES_CRZ | MT6516IDP_RES_PRZ | MT6516IDP_RES_MP4DBLK)
static void StoreSelectedModuleReg(unsigned long u4ResourceTbl)
{
    MT6516_IDP_REG * pReg = NULL;

    if( u4ResourceTbl & (~STOREREG_SUPPORT_RESOURCE))
    {
        IDPDB("[MT6516IDP]Store reg don't support one of those resource\n");
//        IDPDB("[MT6516IDP] 0x%x Store Reg only support this module 0x%x!!\n",u4ResourceTbl , STOREREG_SUPPORT_RESOURCE);
        return;
    }

    //Critical section - start
    spin_lock_irq(&g_IDPLock);

    AccessMT6516_IDP_REG_BACK(&pReg);

    if(NULL == pReg)
    {
        IDPDB("[MT6516IDP]Store reg buffer is gone!!\n");
        spin_unlock_irq(&g_IDPLock);
        return;
    }

    //Store registers
    if(u4ResourceTbl & MT6516IDP_RES_CRZ)
    {
        pReg->CRZReg.u4MT6516CRZ_CFG = ioread32(MT6516CRZ_CFG);
        pReg->CRZReg.u4MT6516CRZ_SRCSZ1 = ioread32(MT6516CRZ_SRCSZ1);
        pReg->CRZReg.u4MT6516CRZ_TARSZ1 = ioread32(MT6516CRZ_TARSZ1);
        pReg->CRZReg.u4MT6516CRZ_HRATIO1 = ioread32(MT6516CRZ_HRATIO1);
        pReg->CRZReg.u4MT6516CRZ_VRATIO1 = ioread32(MT6516CRZ_VRATIO1);
        pReg->CRZReg.u4MT6516CRZ_FRCFG = ioread32(MT6516CRZ_FRCFG);
        pReg->CRZReg.u4MT6516CRZ_BUSY = ioread32(MT6516CRZ_BUSY);
    }

    if(u4ResourceTbl & MT6516IDP_RES_PRZ)
    {
        pReg->PRZReg.u4MT6516PRZ_CFG = ioread32(MT6516PRZ_CFG);
        pReg->PRZReg.u4MT6516PRZ_SRCSZ1 = ioread32(MT6516PRZ_SRCSZ1);
        pReg->PRZReg.u4MT6516PRZ_TARSZ1 = ioread32(MT6516PRZ_TARSZ1);
        pReg->PRZReg.u4MT6516PRZ_HRATIO1 = ioread32(MT6516PRZ_HRATIO1);
        pReg->PRZReg.u4MT6516PRZ_VRATIO1 = ioread32(MT6516PRZ_VRATIO1);
        pReg->PRZReg.u4MT6516PRZ_HRES1 = ioread32(MT6516PRZ_HRES1);
        pReg->PRZReg.u4MT6516PRZ_VRES1 = ioread32(MT6516PRZ_VRES1);
        pReg->PRZReg.u4MT6516PRZ_BLKCSCFG = ioread32(MT6516PRZ_BLKCSCFG);
        pReg->PRZReg.u4MT6516PRZ_YLMBASE = ioread32(MT6516PRZ_YLMBASE);
        pReg->PRZReg.u4MT6516PRZ_ULMBASE = ioread32(MT6516PRZ_ULMBASE);
        pReg->PRZReg.u4MT6516PRZ_VLMBASE = ioread32(MT6516PRZ_VLMBASE);
        pReg->PRZReg.u4MT6516PRZ_FRCFG = ioread32(MT6516PRZ_FRCFG);
        pReg->PRZReg.u4MT6516PRZ_YLBSIZE = ioread32(MT6516PRZ_YLBSIZE);
        pReg->PRZReg.u4MT6516PRZ_PRWMBASE = ioread32(MT6516PRZ_PRWMBASE);
    }

    if(u4ResourceTbl & MT6516IDP_RES_VDODECDMA)
    {
        pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_CON = ioread32(MT6516IMGDMA1_VDODEC_CON);
        pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_Y_BASE = ioread32(MT6516IMGDMA1_VDODEC_Y_BASE);
        pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_U_BASE = ioread32(MT6516IMGDMA1_VDODEC_U_BASE);
        pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_V_BASE = ioread32(MT6516IMGDMA1_VDODEC_V_BASE);
        pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_XSIZE = ioread32(MT6516IMGDMA1_VDODEC_XSIZE);
        pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_YSIZE = ioread32(MT6516IMGDMA1_VDODEC_YSIZE);
        pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_PXLNUM = ioread32(MT6516IMGDMA1_VDODEC_PXLNUM);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IRT0)
    {
        pReg->IRT0Reg.u4MT6516IMGDMA0_IRT0_MO_0_CON = ioread32(MT6516IMGDMA0_IRT0_MO_0_CON);
        pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_MO_1_CON = ioread32(MT6516IMGDMA1_IRT0_MO_1_CON);
        pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_CON = ioread32(MT6516IMGDMA1_IRT0_CON);
        pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_FIFO_BASE = ioread32(MT6516IMGDMA1_IRT0_FIFO_BASE);
        pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_FIFOYLEN = ioread32(MT6516IMGDMA1_IRT0_FIFOYLEN);
        pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_XSIZE = ioread32(MT6516IMGDMA1_IRT0_XSIZE);
        pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_YSIZE = ioread32(MT6516IMGDMA1_IRT0_YSIZE);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IPP)
    {
        pReg->IPPReg.u4MT6516IPP_CFG = ioread32(MT6516IPP_CFG);
        pReg->IPPReg.u4MT6516IPP_SDTCON = ioread32(MT6516IPP_SDTCON);
        pReg->IPPReg.u4MT6516IMPROC_HUE11 = ioread32(MT6516IMPROC_HUE11);
        pReg->IPPReg.u4MT6516IMPROC_HUE12 = ioread32(MT6516IMPROC_HUE12);
        pReg->IPPReg.u4MT6516IMPROC_HUE21 = ioread32(MT6516IMPROC_HUE21);
        pReg->IPPReg.u4MT6516IMPROC_HUE22 = ioread32(MT6516IMPROC_HUE22);
        pReg->IPPReg.u4MT6516IMPROC_SAT = ioread32(MT6516IMPROC_SAT);
        pReg->IPPReg.u4MT6516IMPROC_BRIADJ1 = ioread32(MT6516IMPROC_BRIADJ1);
        pReg->IPPReg.u4MT6516IMPROC_BRIADJ2 = ioread32(MT6516IMPROC_BRIADJ2);
        pReg->IPPReg.u4MT6516IMPROC_CONADJ = ioread32(MT6516IMPROC_CONADJ);
        pReg->IPPReg.u4MT6516IMPROC_COLORIZEU = ioread32(MT6516IMPROC_COLORIZEU);
        pReg->IPPReg.u4MT6516IMPROC_COLORIZEV = ioread32(MT6516IMPROC_COLORIZEV);
        pReg->IPPReg.u4MT6516IMPROC_IO_MUX = (ioread32(MT6516IMPROC_IO_MUX) & 0x7F);
        pReg->IPPReg.u4MT6516IMPROC_IPP_RGB_DETECT = ioread32(MT6516IMPROC_IPP_RGB_DETECT);
        pReg->IPPReg.u4MT6516IMPROC_IPP_RGB_REPLACE = ioread32(MT6516IMPROC_IPP_RGB_REPLACE);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IBW2)
    {
        pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_CON = ioread32(MT6516IMGDMA0_IBW2_CON);
        pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_ALPHA = ioread32(MT6516IMGDMA0_IBW2_ALPHA);
        pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_XSIZE = ioread32(MT6516IMGDMA0_IBW2_XSIZE);
        pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_YSIZE = ioread32(MT6516IMGDMA0_IBW2_YSIZE);
        pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_CLIPLR = ioread32(MT6516IMGDMA0_IBW2_CLIPLR);
        pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_CLIPTB = ioread32(MT6516IMGDMA0_IBW2_CLIPTB);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IRT1)
    {
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_CON = ioread32(MT6516IMGDMA0_IRT1_CON);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_ALPHA = ioread32(MT6516IMGDMA0_IRT1_ALPHA);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_SRC_XSIZE = ioread32(MT6516IMGDMA0_IRT1_SRC_XSIZE);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_SRC_YSIZE = ioread32(MT6516IMGDMA0_IRT1_SRC_YSIZE);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BASE_ADDR0 = ioread32(MT6516IMGDMA0_IRT1_BASE_ADDR0);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BASE_ADDR1 = ioread32(MT6516IMGDMA0_IRT1_BASE_ADDR1);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BASE_ADDR2 = ioread32(MT6516IMGDMA0_IRT1_BASE_ADDR2);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BKGD_XSIZE0 = ioread32(MT6516IMGDMA0_IRT1_BKGD_XSIZE0);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BKGD_XSIZE1 = ioread32(MT6516IMGDMA0_IRT1_BKGD_XSIZE1);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BKGD_XSIZE2 = ioread32(MT6516IMGDMA0_IRT1_BKGD_XSIZE2);
        pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_FIFO_BASE = ioread32(MT6516IMGDMA0_IRT1_FIFO_BASE);
    }

    spin_unlock_irq(&g_IDPLock);
    //Critical section - end
}

static void RestoreSelectedModuleReg(unsigned long u4ResourceTbl)
{
    MT6516_IDP_REG * pReg = NULL;
    unsigned long u4Reg = 0;

    if( u4ResourceTbl & (~STOREREG_SUPPORT_RESOURCE))
    {
        IDPDB("[MT6516IDP]Restore reg don't support one of those resource\n");
//        IDPDB("[MT6516IDP] 0x%x Restore Reg only support this module 0x%x!!\n",u4ResourceTbl , STOREREG_SUPPORT_RESOURCE);
        return ;
    }

    //Critical section - start
    spin_lock_irq(&g_IDPLock);

    AccessMT6516_IDP_REG_BACK(&pReg);

    if(NULL == pReg)
    {
        IDPDB("[MT6516IDP]Restore reg buffer is gone!!\n");
        spin_unlock_irq(&g_IDPLock);
        return;
    }

    if(u4ResourceTbl & MT6516IDP_RES_CRZ)
    {
        iowrite32(pReg->CRZReg.u4MT6516CRZ_CFG,MT6516CRZ_CFG);
        iowrite32(pReg->CRZReg.u4MT6516CRZ_SRCSZ1,MT6516CRZ_SRCSZ1);
        iowrite32(pReg->CRZReg.u4MT6516CRZ_TARSZ1 , MT6516CRZ_TARSZ1);
        iowrite32(pReg->CRZReg.u4MT6516CRZ_HRATIO1 , MT6516CRZ_HRATIO1);
        iowrite32(pReg->CRZReg.u4MT6516CRZ_VRATIO1 , MT6516CRZ_VRATIO1);
        iowrite32(pReg->CRZReg.u4MT6516CRZ_FRCFG , MT6516CRZ_FRCFG);
        iowrite32(pReg->CRZReg.u4MT6516CRZ_BUSY , MT6516CRZ_BUSY);
    }

    if(u4ResourceTbl & MT6516IDP_RES_PRZ)
    {
        iowrite32(pReg->PRZReg.u4MT6516PRZ_CFG , MT6516PRZ_CFG);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_SRCSZ1 , MT6516PRZ_SRCSZ1);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_TARSZ1 , MT6516PRZ_TARSZ1);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_HRATIO1 , MT6516PRZ_HRATIO1);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_VRATIO1 , MT6516PRZ_VRATIO1);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_HRES1 , MT6516PRZ_HRES1);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_VRES1 , MT6516PRZ_VRES1);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_BLKCSCFG , MT6516PRZ_BLKCSCFG);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_YLMBASE , MT6516PRZ_YLMBASE);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_ULMBASE , MT6516PRZ_ULMBASE);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_VLMBASE , MT6516PRZ_VLMBASE);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_FRCFG , MT6516PRZ_FRCFG);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_YLBSIZE , MT6516PRZ_YLBSIZE);
        iowrite32(pReg->PRZReg.u4MT6516PRZ_PRWMBASE , MT6516PRZ_PRWMBASE);
    }

    if(u4ResourceTbl & MT6516IDP_RES_VDODECDMA)
    {
        iowrite32(pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_CON , MT6516IMGDMA1_VDODEC_CON);
        iowrite32(pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_Y_BASE , MT6516IMGDMA1_VDODEC_Y_BASE);
        iowrite32(pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_U_BASE , MT6516IMGDMA1_VDODEC_U_BASE);
        iowrite32(pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_V_BASE , MT6516IMGDMA1_VDODEC_V_BASE);
        iowrite32(pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_XSIZE , MT6516IMGDMA1_VDODEC_XSIZE);
        iowrite32(pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_YSIZE , MT6516IMGDMA1_VDODEC_YSIZE);
        iowrite32(pReg->VDODECDMAReg.u4MT6516IMGDMA1_VDODEC_PXLNUM , MT6516IMGDMA1_VDODEC_PXLNUM);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IRT0)
    {
        iowrite32(pReg->IRT0Reg.u4MT6516IMGDMA0_IRT0_MO_0_CON , MT6516IMGDMA0_IRT0_MO_0_CON);
        iowrite32(pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_MO_1_CON , MT6516IMGDMA1_IRT0_MO_1_CON);
        iowrite32(pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_CON , MT6516IMGDMA1_IRT0_CON);
        iowrite32(pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_FIFO_BASE , MT6516IMGDMA1_IRT0_FIFO_BASE);
        iowrite32(pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_FIFOYLEN , MT6516IMGDMA1_IRT0_FIFOYLEN);
        iowrite32(pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_XSIZE , MT6516IMGDMA1_IRT0_XSIZE);
        iowrite32(pReg->IRT0Reg.u4MT6516IMGDMA1_IRT0_YSIZE , MT6516IMGDMA1_IRT0_YSIZE);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IPP)
    {
        iowrite32(pReg->IPPReg.u4MT6516IPP_CFG , MT6516IPP_CFG);
        iowrite32(pReg->IPPReg.u4MT6516IPP_SDTCON , MT6516IPP_SDTCON);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_HUE11 , MT6516IMPROC_HUE11);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_HUE12 , MT6516IMPROC_HUE12);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_HUE21 , MT6516IMPROC_HUE21);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_HUE22 , MT6516IMPROC_HUE22);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_SAT , MT6516IMPROC_SAT);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_BRIADJ1 , MT6516IMPROC_BRIADJ1);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_BRIADJ2 , MT6516IMPROC_BRIADJ2);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_CONADJ , MT6516IMPROC_CONADJ);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_COLORIZEU , MT6516IMPROC_COLORIZEU);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_COLORIZEV , MT6516IMPROC_COLORIZEV);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_IPP_RGB_DETECT , MT6516IMPROC_IPP_RGB_DETECT);
        iowrite32(pReg->IPPReg.u4MT6516IMPROC_IPP_RGB_REPLACE , MT6516IMPROC_IPP_RGB_REPLACE);
        u4Reg = (ioread32(MT6516IMPROC_IO_MUX) & (~0x7F));
        u4Reg |= (pReg->IPPReg.u4MT6516IMPROC_IO_MUX & 0x7F);
        iowrite32(u4Reg , MT6516IMPROC_IO_MUX);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IBW2)
    {
        iowrite32(pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_CON , MT6516IMGDMA0_IBW2_CON);
        iowrite32(pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_ALPHA , MT6516IMGDMA0_IBW2_ALPHA);
        iowrite32(pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_XSIZE , MT6516IMGDMA0_IBW2_XSIZE);
        iowrite32(pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_YSIZE , MT6516IMGDMA0_IBW2_YSIZE);
        iowrite32(pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_CLIPLR , MT6516IMGDMA0_IBW2_CLIPLR);
        iowrite32(pReg->IBW2Reg.u4MT6516IMGDMA0_IBW2_CLIPTB , MT6516IMGDMA0_IBW2_CLIPTB);
    }

    if(u4ResourceTbl & MT6516IDP_RES_IRT1)
    {
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_CON , MT6516IMGDMA0_IRT1_CON);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_ALPHA , MT6516IMGDMA0_IRT1_ALPHA);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_SRC_XSIZE , MT6516IMGDMA0_IRT1_SRC_XSIZE);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_SRC_YSIZE , MT6516IMGDMA0_IRT1_SRC_YSIZE);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BASE_ADDR0 , MT6516IMGDMA0_IRT1_BASE_ADDR0);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BASE_ADDR1 , MT6516IMGDMA0_IRT1_BASE_ADDR1);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BASE_ADDR2 , MT6516IMGDMA0_IRT1_BASE_ADDR2);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BKGD_XSIZE0 , MT6516IMGDMA0_IRT1_BKGD_XSIZE0);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BKGD_XSIZE1 , MT6516IMGDMA0_IRT1_BKGD_XSIZE1);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_BKGD_XSIZE2 , MT6516IMGDMA0_IRT1_BKGD_XSIZE2);
        iowrite32(pReg->IRT1Reg.u4MT6516IMGDMA0_IRT1_FIFO_BASE , MT6516IMGDMA0_IRT1_FIFO_BASE);
    }

    if(u4ResourceTbl & MT6516IDP_RES_MP4DBLK)
    {
        if(TRUE != hwEnableClock(MT6516_PDN_MM_MP4DBLK, "IDP"))
        {
            IDPDB("enable MP4 DBLK failed \n");
            return;
        }
        iowrite32(0x3,MT6516MP4DEBLK_COMD);
        iowrite32(0,MT6516MP4DEBLK_COMD);
        iowrite32(pReg->MP4DBLKReg.u4MT6516MP4DEBLK_CONF , MT6516MP4DEBLK_CONF);
        iowrite32(pReg->MP4DBLKReg.u4MT6516MP4DEBLK_LIN_BUFF_ADDR , MT6516MP4DEBLK_LIN_BUFF_ADDR);
        iowrite32(pReg->MP4DBLKReg.u4MT6516MP4DEBLK_QUANT_ADDR , MT6516MP4DEBLK_QUANT_ADDR);
        if(TRUE != hwDisableClock(MT6516_PDN_MM_MP4DBLK, "IDP"))
        {
            IDPDB("Dsiable MP4 DBLK failed \n");
            return;
        }
    }

    spin_unlock_irq(&g_IDPLock);
    //Critical section - end
}
//Dump reg - tail

static unsigned long SeqToTbl(stEnableSeq * a_Seq)
{
    unsigned long u4Index = 0, u4Tbl = 0;

    for(;u4Index < a_Seq->u4Length ; u4Index += 1)
    {
        switch(a_Seq->pEnableSeq[u4Index])
        {
            case IDP_VDOENCDMA :
                u4Tbl |= MT6516IDP_RES_VDOENCDMA;
            break;
            case IDP_VDODECDMA :
                u4Tbl |= MT6516IDP_RES_VDODECDMA;
            break;
            case IDP_MP4DBLK :
                u4Tbl |= MT6516IDP_RES_MP4DBLK;
            break;
            default :
                u4Tbl |= (1 << a_Seq->pEnableSeq[u4Index]);
            break;
        }
    }

    return u4Tbl;
}

////Direct link zone - head
typedef struct 
{
    unsigned long u4Configured;
    unsigned long u4Running;
    unsigned long u4ResourceTbl;
    unsigned long u4ChainLength;
    eMT6516IDP_NODES eChainSeq[16];
} stDirectLinkData;
static stDirectLinkData g_DirectLinkData;
inline static int MT6516IDP_PrepareDirectLink(unsigned long a_pstEnSeq)
{
    stEnableSeq EnableSeq;

    if(copy_from_user(&EnableSeq,(stEnableSeq *)a_pstEnSeq,sizeof(stEnableSeq)))
    {
        IDPDB("[MT6516IDP_PrepareDirectLink] Copy from user failed! 0\n");
        return -EFAULT;
    }    

    spin_lock_irq(&g_IDPLock);

    if(g_DirectLinkData.u4Running)
    {
        printk("[MT6516IDP]Attemp to config reg while running!!\n");

        spin_unlock_irq(&g_IDPLock);

        return -EAGAIN;
    }

    g_DirectLinkData.u4ChainLength = sizeof(eMT6516IDP_NODES)*EnableSeq.u4Length;

    if(copy_from_user(g_DirectLinkData.eChainSeq , EnableSeq.pEnableSeq, g_DirectLinkData.u4ChainLength))
    {
        IDPDB("[MT6516IDP_PrepareDirectLink] Copy from user failed! 1\n");
        return -EFAULT;
    }

    g_DirectLinkData.u4ChainLength = EnableSeq.u4Length;
    g_DirectLinkData.u4ResourceTbl = SeqToTbl(&EnableSeq);

    g_DirectLinkData.u4Configured = 1;

    StoreSelectedModuleReg(g_DirectLinkData.u4ResourceTbl);

    spin_unlock_irq(&g_IDPLock);

// TODO: how about error handling?
//MT6516IDPResManager(MT6516_ADD , 0 , &a_pstFile->private_data);

    return 0;
}

int MT6516IDP_EnableDirectLink(void)
{

    spin_lock_irq(&g_IDPLock);
    if(0 == g_DirectLinkData.u4Configured)
    {
        IDPDB("Should configure IDP before enable it\n");
        spin_unlock_irq(&g_IDPLock);
        return 0;
    }

#ifdef MT6516IDP_EARLYSUSPEND
    if(g_u4EarlySuspend)
    {
        IDPDB("Early suspending\n");
        spin_unlock_irq(&g_IDPLock);
        return 0;
    }
#endif

    spin_unlock_irq(&g_IDPLock);

    // Wait for device is ready then lock it
    wait_event_interruptible(g_DirectLinkWaitQueue , (0 == MT6516IDP_LockResource(g_DirectLinkData.u4ResourceTbl)));

    spin_lock_irq(&g_IDPLock);

    // Config IDP
    RestoreSelectedModuleReg(g_DirectLinkData.u4ResourceTbl);

    //Enable IDP
    if(MT6516_EnableChainInternal(g_DirectLinkData.eChainSeq , g_DirectLinkData.u4ChainLength))
    {
        MT6516IDP_UnLockResource(g_DirectLinkData.u4ResourceTbl);
        spin_unlock_irq(&g_IDPLock);
        IDPDB("[MT6516_IDP] Enable chain failed!\n");
        return -EIO;
    }

    g_DirectLinkData.u4Running = 1;

    spin_unlock_irq(&g_IDPLock);

    return 0;
}
EXPORT_SYMBOL(MT6516IDP_EnableDirectLink);

int MT6516IDP_DisableDirectLink(void)
{
    spin_lock_irq(&g_IDPLock);

    if(0 == g_DirectLinkData.u4Running)
    {
        spin_unlock_irq(&g_IDPLock);
        return 0;
    }

    // Disable IDP
    MT6516_DisableChainInternal(g_DirectLinkData.eChainSeq , g_DirectLinkData.u4ChainLength);

    //Unlock
    MT6516IDP_UnLockResource(g_DirectLinkData.u4ResourceTbl);

    g_DirectLinkData.u4Running = 0;

    spin_unlock_irq(&g_IDPLock);

    return 0;
}
EXPORT_SYMBOL(MT6516IDP_DisableDirectLink);
////Direct link zone - tail

//Read MT6516CRZ_STA
//Read MT6516PRZ_STA
static void MT6516_DisableChainInternal(eMT6516IDP_NODES * peChain , unsigned long a_u4Cnt )
{
    unsigned long u4Index = 0;
    unsigned long u4RegValue = 0;

    spin_lock_irq(&g_IDPLock);
    //Disable modules first
    for(u4Index = a_u4Cnt ; u4Index > 0 ; u4Index -= 1)
    {
        switch(peChain[(u4Index-1)])
        {
            case IDP_CRZ :
                g_u4IRQTbl &= (~CRZ_IRQ);
                iowrite32((1 << 16), MT6516CRZ_CON);
                iowrite32(0, MT6516CRZ_CON);
            break;
            case IDP_PRZ :
                g_u4IRQTbl &= (~PRZ_IRQ);
                iowrite32((7 << 16), MT6516PRZ_CON);
                iowrite32(0, MT6516PRZ_CON);
            break;
            case IDP_DRZ :
                g_u4IRQTbl &= (~DRZ_IRQ);
                iowrite32(0,MT6516DRZ_STR);
            break;
            case IDP_IPP1_Y2R0_IPP2 :
                u4RegValue = ioread32(MT6516IMPROC_EN);
                u4RegValue &= (~0x1);
                iowrite32(u4RegValue,MT6516IMPROC_EN);
            break;
            case IDP_R2Y0 :
                u4RegValue = ioread32(MT6516IMPROC_EN);
                u4RegValue &= (~0x10);
                iowrite32(u4RegValue,MT6516IMPROC_EN);
            break;
            case IDP_Y2R1 :
                u4RegValue = ioread32(MT6516IMPROC_EN);
                u4RegValue &= (~0x100);
                iowrite32(u4RegValue,MT6516IMPROC_EN);
            break;
            case IDP_IBW1 :
                g_u4IRQTbl &= (~IBW1_IRQ);
                iowrite32(0,MT6516IMGDMA0_IBW1_STR);
            break;
            case IDP_IBW2 :
                g_u4IRQTbl &= (~IBW2_IRQ);
                iowrite32(0,MT6516IMGDMA0_IBW2_STR);
            break;
            case IDP_IBR1 :
                g_u4IRQTbl &= (~IBR1_IRQ);
                iowrite32(0,MT6516IMGDMA0_IBR1_STR);
            break;
            case IDP_OVL :
                g_u4IRQTbl &= (~OVL_IRQ);
                iowrite32(0,MT6516IMGDMA0_OVL_STR);
                iowrite32(0,MT6516IMGDMA0_OVL_MO_0_STR);
                iowrite32(0,MT6516IMGDMA1_OVL_MO_1_STR);
            break;
            case IDP_IRT0 :
                g_u4IRQTbl &= (~IRT0_IRQ);
                iowrite32(0,MT6516IMGDMA1_IRT0_STR);
                iowrite32(0,MT6516IMGDMA1_IRT0_MO_1_STR);
                iowrite32(0,MT6516IMGDMA0_IRT0_MO_0_STR);
            break;
            case IDP_IRT1 :
                IRT1_BUFFIND(RESET_BUFF , NULL , 0);
                g_u4IRQTbl &= (~IRT1_IRQ);
                iowrite32(0,MT6516IMGDMA0_IRT1_STR);
            break;
            case IDP_IRT3 :
                g_u4IRQTbl &= (~IRT3_IRQ);
                iowrite32(0,MT6516IMGDMA0_IRT3_STR);
            break;
            case IDP_JPEGDMA :
                g_u4IRQTbl &= (~JPEGDMA_IRQ);
                u4RegValue = ioread32(MT6516JPG_ENC_CTL);
                u4RegValue &= (~0x1);
                iowrite32(u4RegValue,MT6516JPG_ENC_CTL);
                iowrite32(0,MT6516IMGDMA1_JPEG_STR);
            break;
            case IDP_VDOENCDMA :
                g_u4IRQTbl &= (~VDOENCDMA_IRQ);
                iowrite32(0,MT6516IMGDMA1_VDOENC_STR);
                VDOENCDMA_BUFFIND(RESET_BUFF , NULL , 0);
            break;
            case IDP_VDODECDMA :
                g_u4IRQTbl &= (~VDODECDMA_IRQ);
                iowrite32(0,MT6516IMGDMA1_VDODEC_STR);
            break;
            case IDP_MP4DBLK :
                g_u4IRQTbl &= (~MP4DBLK_IRQ);
                iowrite32(0,MT6516MP4DEBLK_COMD);
            break;
            default :
            break;
        }
    }

    //Disable the clock later
    for(u4Index = a_u4Cnt ; u4Index > 0 ; u4Index -= 1)
    {
        switch(peChain[(u4Index-1)])
        {
            case IDP_CRZ :
                hwDisableClock(MT6516_PDN_MM_CRZ,"IDP");
                hwDisableClock(MT6516_PDN_MM_RESZLB,"IDP");
                u4RegValue = ioread32(MT6516IDP_G1MEMPDN);
                u4RegValue |= (1<<8);
                iowrite32(u4RegValue , MT6516IDP_G1MEMPDN);
            break;

            case IDP_PRZ :
                hwDisableClock(MT6516_PDN_MM_PRZ,"IDP");
                hwDisableClock(MT6516_PDN_MM_PRZ2,"IDP");
            break;

            case IDP_DRZ :
                hwDisableClock(MT6516_PDN_MM_DRZ,"IDP");
            break;

            case IDP_IPP1_Y2R0_IPP2 :
            case IDP_R2Y0 :
            case IDP_Y2R1 :
                //Check all are off
                hwDisableClock(MT6516_PDN_MM_IPP,"IDP");
            break;

            case IDP_IBW1 :
            case IDP_IBW2 :
            case IDP_IBR1 :
            case IDP_IRT1 :
            case IDP_IRT3 :
                //Check all are off
                hwDisableClock(MT6516_PDN_MM_IMGDMA0,"IDP");
            break;

            case IDP_JPEGDMA :
                hwDisableClock(MT6516_PDN_MM_JPEG,"IDP");
            case IDP_VDOENCDMA :
            case IDP_VDODECDMA :
                //Check all are off
                hwDisableClock(MT6516_PDN_MM_IMGDMA1,"IDP");
            break;

            case IDP_OVL : 
                u4RegValue = ioread32(MT6516IDP_G1MEMPDN);
                u4RegValue |= (1<<7);
                iowrite32(u4RegValue , MT6516IDP_G1MEMPDN);
            case IDP_IRT0 :
                //Check all are off
                hwDisableClock(MT6516_PDN_MM_IMGDMA0,"IDP");
                hwDisableClock(MT6516_PDN_MM_IMGDMA1,"IDP");
            break;

            case IDP_MP4DBLK :
                hwDisableClock(MT6516_PDN_MM_MP4DBLK,"IDP");
            break;
            default :
            break;
        }
    }
    spin_unlock_irq(&g_IDPLock);
}

static int MT6516IDP_DisableChain(unsigned long a_pstEnSeq)
{
    eMT6516IDP_NODES * peArray = NULL;
    u32 u4Length = 0;
    stEnableSeq EnableSeq;

    if(copy_from_user(&EnableSeq,(stEnableSeq *)a_pstEnSeq,sizeof(stEnableSeq)))
    {
        IDPDB("[MT6516_IDP] Copy from user failed!\n");
        return -EFAULT;
    }    

    u4Length = sizeof(eMT6516IDP_NODES)*EnableSeq.u4Length;

    peArray = kmalloc(u4Length,GFP_ATOMIC);

    if(NULL == peArray)
    {
        IDPDB("[MT6516_IDP] Enable chain , not enough memory!\n");
        return -EFAULT;
    }

    if(copy_from_user(peArray,EnableSeq.pEnableSeq,u4Length))
    {
        IDPDB("[MT6516_IDP] Copy from user failed!\n");
        return -EFAULT;
    }

    MT6516_DisableChainInternal(peArray,EnableSeq.u4Length);

    kfree(peArray);

    return 0;
}

void MT6516ISP_DUMPREG(void);
inline static int MT6516IDP_WaitIRQ(u32 a_pu4IRQMask)
{
    u32 u4Result = 0;
    u32 u4RetVal = 0;

//    if(get_user(u4Result,((int __user *)a_pu4IRQMask)))
    if(copy_from_user(&u4Result,(u32 *)a_pu4IRQMask , sizeof(u32)))
    {
        IDPDB("[MT6516IDP] get from user failed \n");
    }

//    wait_event_interruptible(g_WaitQueue, (g_u4IRQTbl & u4Result));
    wait_event_interruptible_timeout(g_WaitQueue, (g_u4IRQTbl & u4Result) , 1*HZ);

    spin_lock_irq(&g_IDPLock);

    if(!(g_u4IRQTbl & u4Result))
    {
        spin_unlock_irq(&g_IDPLock);
        printk("PID:%d Wait Interrupt Timeout!!T:0x%x R:0x%x\n",current->pid , g_u4IRQTbl , u4Result);
        MT6516IDP_DUMPREG();
//Gipi add : read more information from ISP and pm_api
MT6516ISP_DUMPREG();

        return -1;
    }

    u4RetVal = (g_u4IRQTbl & u4Result);

    g_u4IRQTbl &= (~u4Result);

    spin_unlock_irq(&g_IDPLock);

    put_user(u4RetVal , (u32 *)a_pu4IRQMask);

    return 0;
}

inline static int MT6516IDP_WaitBuff(unsigned long a_pu4Param)
{

    stWaitBuff Buff;

    if(copy_from_user(&Buff , (void *)a_pu4Param , sizeof(stWaitBuff)))
    {
        IDPDB("[MT6516IDP] wait buff copy from user failed \n");
    }

    VDOENCDMA_BUFFIND(UPDATE_BLK , NULL , Buff.u4OutBuffNo);
//    g_u4IRQTbl &= (~(VDOENCDMA_IRQ));
//    wait_event_interruptible(g_WaitQueue, (g_u4IRQTbl & VDOENCDMA_IRQ));
    wait_event_interruptible_timeout(g_WaitQueue, (g_u4IRQTbl & VDOENCDMA_IRQ) , 1*HZ);

    spin_lock_irq(&g_IDPLock);

    if(!(g_u4IRQTbl & VDOENCDMA_IRQ))
    {
        spin_unlock_irq(&g_IDPLock);
        printk("Wait Buffer Timeout!!\n");
        MT6516IDP_DUMPREG();
//Gipi add : read more information from ISP and pm_api
MT6516ISP_DUMPREG();

        return -1;
    }

    g_u4IRQTbl &= (~(VDOENCDMA_IRQ));

    spin_unlock_irq(&g_IDPLock);

    VDOENCDMA_BUFFIND(QUERY_BUFF , (unsigned long *)&Buff , 0);

    IRT1_BUFFIND(QUERY_BUFF , &Buff.u4IRT1BuffNo , 0);

    VDOENCDMA_BUFFIND(GET_TIMES, Buff.pu4TSec , 0);
    VDOENCDMA_BUFFIND(GET_TIMEUS, Buff.pu4TuSec , 0);

    if(copy_to_user((void *)a_pu4Param , &Buff , sizeof(stWaitBuff)))
    {
        IDPDB("[MT6516IDP] wait buff copy to user failed \n");
    }

    return 0;
}

inline static int MT6516IDP_CheckIRQ(u32 a_pu4IRQMask)
{

    u32 u4Result = 0;

    u32 u4CRZInt, u4DRZINT, u4PRZINT, u4IMGDMA0INT, u4IMGDMA1INT, u4MP4DBLKINT;

    u4CRZInt = ioread32(MT6516CRZ_INT);
    u4PRZINT = ioread32(MT6516PRZ_INT);
    u4DRZINT = ioread32(MT6516DRZ_STA);
    u4IMGDMA0INT = ioread32(MT6516IMGDMA0_STA);
    u4IMGDMA1INT = ioread32(MT6516IMGDMA1_STA);
    u4MP4DBLKINT = ioread32(MT6516MP4DEBLK_IRQ_STS);

    //ignore frame start
    if(u4CRZInt & 0x1){ u4Result |= CRZ_IRQ;}
    if(u4PRZINT & 0x7){ u4Result |= PRZ_IRQ;}
    if(u4DRZINT & 0x1){ u4Result |= DRZ_IRQ;}
    if(u4IMGDMA0INT & 0x10){u4Result |= IBW1_IRQ;}
    if(u4IMGDMA0INT & 0x20){u4Result |= IBW2_IRQ;}
    if(u4IMGDMA0INT & 0x100){u4Result |= IBR1_IRQ;}
    if(u4IMGDMA0INT & 0x400){u4Result |= OVL_IRQ;}
    if(u4IMGDMA0INT & 0x2000){u4Result |= IRT1_IRQ;}
    if(u4IMGDMA0INT & 0x8000){u4Result |= IRT3_IRQ;}

    if(u4IMGDMA1INT & 0x1){u4Result |= JPEGDMA_IRQ;}
    if(u4IMGDMA1INT & 0x2){u4Result |= VDOENCDMA_IRQ;}
    if(u4IMGDMA1INT & 0x4){u4Result |= VDOENCDMA_IRQ;}
    if(u4IMGDMA1INT & 0x8){u4Result |= VDODECDMA_IRQ;}
    if(u4IMGDMA1INT & 0x1000){u4Result |= IRT0_IRQ;}

    if(u4MP4DBLKINT & 0x1){u4Result |= MP4DBLK_IRQ;}

    put_user(u4Result , (u32 *)a_pu4IRQMask);

    return 0;
}

#define LCDSTA (LCD_BASE)
#define MTCMOS_ISO_EN (CONFIG_BASE + 0x0300)
#define MTCMOS_PWR_OFF (CONFIG_BASE + 0x0304)
#define G1_MEM_PDN (CONFIG_BASE + 0x030C)
#define MTCMOS_IN_ISO_EN (CONFIG_BASE + 0x0318)

// #define 
void MT6516IDP_DUMPREG(void)
{
    u32 u4RegValue = 0;
    u32 u4Index = 0;

    IDPDB("CRZ REG:\n ********************\n");
    for(u4Index = 0 ; u4Index < 0xC4 ; u4Index += 4){
        u4RegValue = ioread32(MT6516CRZ_CFG+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("PRZ REG:\n ********************\n");
    for(u4Index = 0; u4Index < 0x60 ; u4Index += 4){
        u4RegValue = ioread32(MT6516PRZ_CFG+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("DRZ REG:\n ********************\n");
    for(u4Index = 0; u4Index < 0x28 ; u4Index += 4){
        u4RegValue = ioread32(MT6516DRZ_STR+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("IPP REG:\n ********************\n");
    u4RegValue = ioread32(MT6516IPP_CFG);
    IDPDB("+0x0 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516R2Y0_CFG);
    IDPDB("+0x4 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516Y2R1_CFG);
    IDPDB("+0x8 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516IMPROC_IO_MUX);
    IDPDB("+0x318 0x%x\n",u4RegValue);
    u4RegValue = ioread32(MT6516IMPROC_EN);
    IDPDB("+0x320 0x%x\n",u4RegValue);

    IDPDB("IMGDMA0 REG:\n ********************\n");
    u4RegValue = ioread32(MT6516IMGDMA0_STA);
    IDPDB("+0x0 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516IMGDMA0_ACKINT);
    IDPDB("+0x4 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516IMGDMA0_SW_RSTB);
    IDPDB("+0x10 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516IMGDMA0_GMCIF_STA);
    IDPDB("+0x20 0x%x\n", u4RegValue);

    IDPDB("IBW1 REG:\n ********************\n");
    for(u4Index = 0x300 ; u4Index < 0x358 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("IBW2 REG:\n ********************\n");
    for(u4Index = 0x400 ; u4Index < 0x428 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("IBR1 REG:\n ********************\n");
    for(u4Index = 0x500 ; u4Index < 0x514 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("OVL REG:\n ********************\n");
    for(u4Index = 0x700 ; u4Index < 0x720 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    u4Index = 0x780;
    u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
    IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    u4Index = 0x784;
    u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
    IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    u4Index = 0x788;
    u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
    IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);

    IDPDB("IRT0 REG:\n ********************\n");
    for(u4Index = 0xC80 ; u4Index < 0xC8C ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("IRT1 REG:\n ********************\n");
    for(u4Index = 0xD00 ; u4Index < 0xD68 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("IRT3 REG:\n ********************\n");
    for(u4Index = 0xF00 ; u4Index < 0xF68 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA0_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("IMGDMA1 REG:\n ********************\n");
    u4RegValue = ioread32(MT6516IMGDMA1_STA);
    IDPDB("+0x0 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516IMGDMA1_ACKINT);
    IDPDB("+0x4 0x%x\n", u4RegValue);
    u4RegValue = ioread32(MT6516IMGDMA1_GMCIF_STA);
    IDPDB("+0x20 0x%x\n", u4RegValue);

    IDPDB("JPEGDMA REG:\n ********************\n");
    for(u4Index = 0x100 ; u4Index < 0x15C ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA1_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("VDOENCDMA REG:\n ********************\n");
    for(u4Index = 0x200 ; u4Index < 0x280 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA1_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("VDODECDMA REG:\n ********************\n");
    for(u4Index = 0x280 ; u4Index < 0x2C0 ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA1_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("OVL REG:\n ********************\n");
    for(u4Index = 0x780 ; u4Index < 0x78C ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA1_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("IRT0 REG:\n ********************\n");
    for(u4Index = 0xC00 ; u4Index < 0xC8C ; u4Index += 4){
        u4RegValue = ioread32(MT6516IMGDMA1_STA+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("MP4DBLK REG:\n ********************\n");
    for(u4Index = 0 ; u4Index < 0x1C ; u4Index += 4){
        u4RegValue = ioread32(MT6516MP4DEBLK_COMD+u4Index);
        IDPDB("+0x%x 0x%x\n", u4Index,u4RegValue);
    }

    IDPDB("Clocks:\n ********************\n");
    IDPDB("MTCMOS_ISO_EN : 0x1300 0x%x\n", ioread32(MTCMOS_ISO_EN));
    IDPDB("MTCMOS_PWR_OFF : 0x1304 0x%x\n", ioread32(MTCMOS_PWR_OFF));
    IDPDB("G1_MEM_PDN : 0x130C 0x%x\n", ioread32(G1_MEM_PDN));
    IDPDB("MTCMOS_IN_ISO_EN : 0x1318 0x%x\n", ioread32(MTCMOS_IN_ISO_EN));
    IDPDB("GRAPH1SYS_CG : 0x300 0x%x\n", ioread32((GRAPH1SYS_CONFG_BASE + 0x300)));
    IDPDB("GRAPH2SYS_CG : 0x300 0x%x\n", ioread32(GRAPH2SYS_BASE));
    
}

static int MT6516_IDP_Ioctl(struct inode * a_pstInode,
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    stMT6516ResTbl * pTbl = NULL;
    int i4RetVal = 0;

    switch(a_u4Command)
    {
        case MTK6516IDP_T_LOCK :
            i4RetVal = MT6516IDP_LockResource(a_u4Param);
            if(0 == i4RetVal)
            {
                pTbl = (stMT6516ResTbl *)a_pstFile->private_data;
                pTbl->u4Table |= a_u4Param;
            }
            break;

        case MTK6516IDP_T_UNLOCK :
            i4RetVal = MT6516IDP_UnLockResource(a_u4Param);
            if(0 == i4RetVal)
            {
                pTbl = (stMT6516ResTbl *)a_pstFile->private_data;
                pTbl->u4Table &= (~a_u4Param);
            }
            break;

        case MTK6516IDP_S_CONFIG :
            if(0 == a_u4Param){
                i4RetVal = -EINVAL;
            }
            else{
                i4RetVal = MT6516IDP_ConfigHW(a_u4Param);
            }
            break;

        case MTK6516IDP_S_ENABLE :
            if(0 == a_u4Param){
                i4RetVal = -EINVAL;
            }
            else{
                i4RetVal = MT6516IDP_EnableChain(a_u4Param);
            }
//            MT6516IDP_DUMPREG();
            break;

        case MTK6516IDP_S_DISABLE :
            if(0 == a_u4Param){
                i4RetVal = -EINVAL;
            }
            else{
                i4RetVal = MT6516IDP_DisableChain(a_u4Param);
            }
            break;

        case MT6516IDP_X_WAITIRQ :
            i4RetVal = MT6516IDP_WaitIRQ(a_u4Param);
            break;

        case MT6516IDP_X_CHECKIRQ :
            i4RetVal = MT6516IDP_CheckIRQ(a_u4Param);
            break;

        case MT6516IDP_T_DUMPREG :
            MT6516IDP_DUMPREG();
            break;

        case MT6516IDP_M_LOCKMODE :
            pTbl = (stMT6516ResTbl *)a_pstFile->private_data;
            i4RetVal = MT6516IDP_LockMode(a_u4Param , &pTbl->u4ScenarioTbl);
//printk("LockMode:0x%x,0x%x,%lu\n",pTbl->u4ScenarioTbl,g_u4KeyMode,pTbl->PID);
            break;

        case MT6516IDP_M_UNLOCKMODE :
            i4RetVal = MT6516IDP_UnlockMode(a_u4Param);
            if(0 == i4RetVal){
                pTbl = (stMT6516ResTbl *)a_pstFile->private_data;
//printk("UnLockMode:0x%x,0x%x,%lu\n",pTbl->u4ScenarioTbl,g_u4KeyMode,pTbl->PID);
                pTbl->u4ScenarioTbl &= (~a_u4Param);
            }
            break;

        case MT6516IDP_X_WAITBUFF :
            i4RetVal = MT6516IDP_WaitBuff(a_u4Param);
            break;

        case MTK6516IDP_S_DIRECT_LINK:
            if(0 == a_u4Param){
                i4RetVal = -EINVAL;
            }
            else{
                i4RetVal = MT6516IDP_PrepareDirectLink(a_u4Param);
            }
            break;

        default :
                //i4RetVal = ENOIOCTLCMD;
                i4RetVal = -EINVAL;
            break;
    }

    return i4RetVal;
}

static const struct file_operations g_stMT6516_IDP_fops = 
{
	.owner = THIS_MODULE,
	.open = MT6516_IDP_Open,
	.release = MT6516_IDP_Release,
	.flush = MT6516_IDP_Flush,
	.ioctl = MT6516_IDP_Ioctl
};

#define MT6516IDP_DYNAMIC_ALLOCATE_DEVNO 1
static inline int RegisterMT6516IDPCharDrv(void)
{
#if MT6516IDP_DYNAMIC_ALLOCATE_DEVNO

    if( alloc_chrdev_region(&g_MT6516IDPdevno, 0, 1,"mt6516-IDP") )
    {
        IDPDB("[mt6516_IDP] Allocate device no failed\n");

        return -EAGAIN;
    }

#else

    if( register_chrdev_region( g_MT6516IDPdevno , 1 , "mt6516-IDP") )
    {
        IDPDB("[mt6516_IDP] Register device no failed\n");

        return -EAGAIN;
    }
#endif

    //Allocate driver
    g_pMT6516IDP_CharDrv = cdev_alloc();

    if(NULL == g_pMT6516IDP_CharDrv)
    {
        unregister_chrdev_region(g_MT6516IDPdevno, 1);

        IDPDB("[mt6516_IDP] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pMT6516IDP_CharDrv, &g_stMT6516_IDP_fops);

    g_pMT6516IDP_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pMT6516IDP_CharDrv, g_MT6516IDPdevno, 1))
    {
        IDPDB("[mt6516_IDP] Attatch file operation failed\n");

        unregister_chrdev_region(g_MT6516IDPdevno, 1);

        return -EAGAIN;
    }

    return 0;

}

static inline void UnregisterMT6516IDPCharDrv(void)
{
    //Release char driver
    cdev_del(g_pMT6516IDP_CharDrv);

    unregister_chrdev_region(g_MT6516IDPdevno, 1);
}

void MT6516CRZ_tasklet(unsigned long a_u4Param)
{
    CRZDBuff(DBUFF_SET, NULL);
}
DECLARE_TASKLET(CRZTaskLet, MT6516CRZ_tasklet, 0);
extern void MT6516_IRQMask(unsigned int line);
extern void MT6516_IRQUnmask(unsigned int line);
static __tcmfunc irqreturn_t MT6516IDP_irq(int irq, void *dev_id)
{
    u32 u4RegValue = 0;

    MT6516_IRQMask(MT6516_IMGDMA_IRQ_LINE);
    MT6516_IRQMask(MT6516_IMGDMA2_IRQ_LINE);
    MT6516_IRQMask(MT6516_CRZ_IRQ_LINE);
    MT6516_IRQMask(MT6516_PRZ_IRQ_LINE);
    MT6516_IRQMask(MT6516_DRZ_IRQ_LINE);

    switch(irq)
    {
        case MT6516_CRZ_IRQ_LINE :
            u4RegValue = ioread32(MT6516CRZ_INT);
            if(u4RegValue & 0x2){ g_u4IRQTbl |= CRZ_IRQ;tasklet_schedule(&CRZTaskLet);}
        break;
        case MT6516_PRZ_IRQ_LINE :
            u4RegValue = ioread32(MT6516PRZ_INT);
            if(u4RegValue & 0x7){ g_u4IRQTbl |= PRZ_IRQ;}
        break;
        case MT6516_DRZ_IRQ_LINE:
            u4RegValue = ioread32(MT6516DRZ_STA);
            if(u4RegValue & 0x1){ g_u4IRQTbl |= DRZ_IRQ;iowrite32(1,MT6516DRZ_ACKINT);}
        break;
        case MT6516_IMGDMA_IRQ_LINE:
            u4RegValue = ioread32(MT6516IMGDMA0_STA);
            if(u4RegValue & 0x10){g_u4IRQTbl |= IBW1_IRQ;}
            if(u4RegValue & 0x20){g_u4IRQTbl |= IBW2_IRQ;}
            if(u4RegValue & 0x100){g_u4IRQTbl |= IBR1_IRQ;}
            if(u4RegValue & 0x400){g_u4IRQTbl |= OVL_IRQ;}
            if(u4RegValue & 0x2000){g_u4IRQTbl |= IRT1_IRQ;IRT1_BUFFIND(SWITCH_BUFF,NULL,0);}
            if(u4RegValue & 0x8000){g_u4IRQTbl |= IRT3_IRQ;}
            iowrite32(u4RegValue,MT6516IMGDMA0_ACKINT);
        break;
        case MT6516_IMGDMA2_IRQ_LINE:
            u4RegValue = ioread32(MT6516IMGDMA1_STA);
            if(u4RegValue & 0x1){g_u4IRQTbl |= JPEGDMA_IRQ;}
            if(u4RegValue & 0x2){g_u4IRQTbl |= VDOENCDMA_IRQ;VDOENCDMA_BUFFIND(SWITCH_BUFF,NULL,0);}
            if(u4RegValue & 0x8){g_u4IRQTbl |= VDODECDMA_IRQ;}
            if(u4RegValue & 0x1000){g_u4IRQTbl |= IRT0_IRQ;}
            iowrite32(u4RegValue,MT6516IMGDMA1_ACKINT);
        break;

        default:
            printk("[MT6516_IDP]something wrong on isr!!\n");
        break;
    }

    if(u4RegValue){
        wake_up_interruptible(&g_WaitQueue);
    }

    MT6516_IRQUnmask(MT6516_IMGDMA_IRQ_LINE);
    MT6516_IRQUnmask(MT6516_IMGDMA2_IRQ_LINE);
    MT6516_IRQUnmask(MT6516_CRZ_IRQ_LINE);
    MT6516_IRQUnmask(MT6516_PRZ_IRQ_LINE);
    MT6516_IRQUnmask(MT6516_DRZ_IRQ_LINE);

    return IRQ_HANDLED;
}

//extern void MT6516_IRQUnmask(unsigned int line);
//extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);
// Called to probe if the device really exists. and create the semaphores
static int MT6516IDP_probe(struct platform_device *pdev)
{
    int i4IRQ = 0;
    u32 u4Index = 0;
    struct resource * pstRes = NULL;
    struct device* idp_device = NULL;

    IDPPARMDB("[mt6516_IDP] probing MT6516 IDP\n");

    //Check platform_device parameters
    if(NULL == pdev)
    {
//        dev_err(&pdev->dev,"platform data is missed\n");
        IDPDB("[MT6516IDP] platform data is missed\n");

        return -ENXIO;
    }

    //register char driver
    //Allocate major no

    if(RegisterMT6516IDPCharDrv())
    {
        dev_err(&pdev->dev,"register char failed\n");

        return -EAGAIN;
    }

    //Mapping to IDP registers
    //For MT6516, there is no diff by calling request_region() or request_mem_region(),
    for(u4Index = 0; u4Index < 6; u4Index += 1){
        pstRes = platform_get_resource(pdev, IORESOURCE_MEM, u4Index);

        if(NULL == pstRes)
        {
            dev_err(&pdev->dev,"get resource failed\n");

            return -ENOMEM;
        }

        pstRes = request_mem_region(pstRes->start, pstRes->end - pstRes->start + 1,pdev->name);

        if(NULL == pstRes)
        {
            dev_err(&pdev->dev,"request I/O mem failed\n");
        }
        //ioremap_noncache()
    }

    //Request IRQ, CRZ interrupt is the most useful one to configure registers.
//    MT6516_IRQUnmask(MT6516_IRQ_IMAGEDMA_CODE);
//    MT6516_IRQSensitivity(MT6516_IRQ_IMAGEDMA_CODE,MT6516_LEVEL_SENSITIVE);
    for(u4Index = 0; u4Index < 6; u4Index += 1){

        i4IRQ = platform_get_irq(pdev, u4Index);

        if(request_irq(i4IRQ, MT6516IDP_irq, 0, pdev->name , NULL))
        {
            dev_err(&pdev->dev,"request IRQ failed\n");
        }
    }
    init_waitqueue_head(&g_WaitQueue);
    init_waitqueue_head(&g_DirectLinkWaitQueue);

    g_u4KeyMode = 0;
    
    g_u4IRQTbl = 0;

    g_u4ResourceTbl = (u32)(-1);

    spin_lock_init(&g_IDPLock);

    spin_lock_init(&g_MT6516IDPOpenLock);

    for(u4Index = 0 ; u4Index < DeviceNeedSramCnt ; u4Index += 1)
    {
        g_stSRAMMap[u4Index].u4Size = 0;
        g_stSRAMMap[u4Index].u4PhysicalAddr = 0;
    }

    g_stPRZLineBuff.bAllocated = 0;

    idp_class = class_create(THIS_MODULE, "idpdrv");
    if (IS_ERR(idp_class)) {
        int ret = PTR_ERR(idp_class);
        IDPDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }    
    idp_device = device_create(idp_class, NULL, g_MT6516IDPdevno, NULL, "mt6516-IDP");

    MT6516IDPResManager(MT6516_ALLOC , 0 , NULL);

    memset(&g_DirectLinkData , 0 , sizeof(stDirectLinkData));

    IDPPARMDB("[mt6516_IDP] probe MT6516 IDP success\n");

//Always enable frame sync, ECO
//    iowrite32((1<<7),MT6516IMGDMA1_JPEG_CON);
//    iowrite32((1<<5),MT6516IMGDMA0_IBW2_CON);
//    iowrite32((1<<13),MT6516CRZ_CFG);

    return 0;
}

// Called when the device is being detached from the driver
static int MT6516IDP_remove(struct platform_device *pdev)
{
    struct resource * pstRes = NULL;
    u32 u4Index = 0;
    int i4IRQ = 0;

    MT6516IDPResManager(MT6516_FREE , 0 , NULL);

    //unregister char driver.
    UnregisterMT6516IDPCharDrv();

    //unmaping IDP registers
    for(u4Index = 0; u4Index < 6; u4Index += 1)
    {
        pstRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);

        release_mem_region(pstRes->start, (pstRes->end - pstRes->start + 1));
    }
    //Release IRQ
    for(u4Index = 0; u4Index < 5; u4Index += 1)
    {
        i4IRQ = platform_get_irq(pdev, 0);

        free_irq(i4IRQ,NULL);
    }

    device_destroy(idp_class, g_MT6516IDPdevno);
    class_destroy(idp_class);

    IDPPARMDB("MT6516 IDP is removed\n");

    return 0;
}

// PM suspend
static int MT6516IDP_suspend(struct platform_device *pdev, pm_message_t mesg)
{

    return 0;
}

// PM resume
static int MT6516IDP_resume(struct platform_device *pdev)
{
//
//    iowrite32((1<<7),MT6516IMGDMA1_JPEG_CON);
//    iowrite32((1<<5),MT6516IMGDMA0_IBW2_CON);
//    iowrite32((1<<13),MT6516CRZ_CFG);

    return 0;
}

// platform structure
static struct platform_driver g_stMT6516IDP_Platform_Driver = {
    .probe		= MT6516IDP_probe,
    .remove	= MT6516IDP_remove,
    .suspend	= MT6516IDP_suspend,
    .resume	= MT6516IDP_resume,
    .driver		= {
        .name	= "mt6516-IDP",
        .owner	= THIS_MODULE,
    }
};

#ifdef MT6516IDP_EARLYSUSPEND
static void MT6516IDP_early_suspend(struct early_suspend *h)
{
    spin_lock_irq(&g_IDPLock);

    g_u4EarlySuspend = 1;

    spin_unlock_irq(&g_IDPLock);
}

static void MT6516IDP_late_resume(struct early_suspend *h)
{
    spin_lock_irq(&g_IDPLock);

    g_u4EarlySuspend = 0;

    spin_unlock_irq(&g_IDPLock);
}

static struct early_suspend mtkfb_early_suspend_handler = 
{
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	.suspend = MT6516IDP_early_suspend,
	.resume = MT6516IDP_late_resume,
};
#endif

static int __init MT6516_IDP_Init(void)
{
	if(platform_driver_register(&g_stMT6516IDP_Platform_Driver)){
		IDPDB("[MT6516IDP]failed to register MT6516 IDP driver\n");
		return -ENODEV;
	}

#ifdef MT6516IDP_EARLYSUSPEND
   	register_early_suspend(&mtkfb_early_suspend_handler);
#endif

	return 0;
}

static void __exit MT6516_IDP_Exit(void)
{
    platform_driver_unregister(&g_stMT6516IDP_Platform_Driver);
}

#define REG_JPEG_ENC_RESET              *(volatile kal_uint32 *)(JPEG_BASE + 0x100)
#define REG_JPEG_DEC_RESET              *(volatile kal_uint32 *)(JPEG_BASE + 0x44)

#define MP4_CODEC_COMD             (volatile unsigned long *)(MP4_BASE+0x0000)    /*WO*/
#define MP4_CODEC_COMD_CORE_RST                  0x0001      ///< Reset the hardware core excluding APB control register set of decoder and encoder 
#define MP4_CODEC_COMD_ENC_RST                   0x0002      ///< Reset the APB control register set of encoder
#define MP4_CODEC_COMD_DEC_RST                   0x0004      ///< Reset the APB control register set of decoder
#define HW_WRITE(ptr,data) (*(ptr) = (data))

void MT6516_DCT_Reset(void)
{
    BOOL ret;
    volatile unsigned int reset_count;
    
    ret = hwEnableClock(MT6516_PDN_MM_DCT,"IDP");
    ret = hwEnableClock(MT6516_PDN_MM_JPEG,"IDP");
    
    // reset jpeg decode
    REG_JPEG_DEC_RESET = 0;
    REG_JPEG_DEC_RESET = 1;
    
    // reset jpeg encode
    REG_JPEG_ENC_RESET = 0;
    REG_JPEG_ENC_RESET = 1;

    ret = hwDisableClock(MT6516_PDN_MM_DCT,"IDP");
    ret = hwDisableClock(MT6516_PDN_MM_JPEG,"IDP");

    ret = hwEnableClock(MT6516_PDN_MM_MP4,"IDP");
    ret = hwEnableClock(MT6516_PDN_MM_DCT,"IDP");

    //reset MPEG4 decode
    HW_WRITE(MP4_CODEC_COMD, MP4_CODEC_COMD_DEC_RST);
    for(reset_count = 0 ; reset_count < 1000 ; reset_count++)
    {
        ;
    }
    HW_WRITE(MP4_CODEC_COMD, 0);
    HW_WRITE(MP4_CODEC_COMD, MP4_CODEC_COMD_CORE_RST);
    for(reset_count = 0 ; reset_count < 1000 ; reset_count++)
    {
        ;
    }
    HW_WRITE(MP4_CODEC_COMD, 0);

    ret = hwDisableClock(MT6516_PDN_MM_MP4,"IDP");
    ret = hwDisableClock(MT6516_PDN_MM_DCT,"IDP");


    ///////////////////////////////////////////////////////////////
    //MPEG4 encoder section
    ///////////////////////////////////////////////////////////////
    ret = hwEnableClock(MT6516_PDN_MM_MP4,"IDP");
    ret = hwEnableClock(MT6516_PDN_MM_DCT,"IDP");

    //reset MPEG4 decode
    HW_WRITE(MP4_CODEC_COMD, MP4_CODEC_COMD_ENC_RST);
    for(reset_count = 0 ; reset_count < 1000 ; reset_count++)
    {
        ;
    }
    HW_WRITE(MP4_CODEC_COMD, 0);
    HW_WRITE(MP4_CODEC_COMD, MP4_CODEC_COMD_CORE_RST);
    for(reset_count = 0 ; reset_count < 1000 ; reset_count++)
    {
        ;
    }
    HW_WRITE(MP4_CODEC_COMD, 0);

    ret = hwDisableClock(MT6516_PDN_MM_MP4,"IDP");
    ret = hwDisableClock(MT6516_PDN_MM_DCT,"IDP");
    ///////////////////////////////////////////////////////////////

    NOT_REFERENCED(ret);
}


module_init(MT6516_IDP_Init);
module_exit(MT6516_IDP_Exit);
MODULE_DESCRIPTION("MT6516 IDP driver");
MODULE_AUTHOR("Gipi <Gipi.Lin@Mediatek.com>");
MODULE_LICENSE("GPL");
