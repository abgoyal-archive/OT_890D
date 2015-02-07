

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <linux/sched.h>	//Kevin for get APP name

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/memory.h>

#include "M3DDriver.h"

#ifdef _MT6516_GLES_CMQ_	//Kevin
#include <linux/kthread.h>	//for kthread_run
#include <asm/io.h>	//For address remap to virtual 
#include <asm/cacheflush.h>	//for flush cache
#endif

#ifdef __M3D_INTERRUPT__
#include <linux/interrupt.h>
#endif

#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/tcm.h>

#define CLAMP(x, min, max)  ((x) < (min)? (min): ((x) > (max)?(max): (x)))

#define LIKELY( exp )       (__builtin_expect( (exp) != 0, true  ))
#define UNLIKELY( exp )     (__builtin_expect( (exp) != 0, false ))

#define	GRAPH_SYS_RAM2_M3D_PA	0x40043800

#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
#define CHECK_PROC_DEBUG(pCtx, u4DebugBit) \
    (pCtx->u4DebugFlag & (u4DebugBit))
#endif

#ifdef M3D_DEBUG
// 1:Process Context
// 2:M3D Thread
// 3:Interrupt
#define DbgPrt(fmt, ...)  \
    printk(KERN_INFO"[M3D] "fmt, ##__VA_ARGS__)
#define DbgPrt2(fmt, ...)  \
    printk(KERN_INFO"[M3D2] "fmt, ##__VA_ARGS__)
#define DbgPrt3(fmt, ...)  \
    printk(KERN_INFO"[M3D3] "fmt, ##__VA_ARGS__)
#define DbgCmdEnable 0
#else
#define DbgPrt(fmt, ...)
#define DbgPrt2(fmt, ...)
#define DbgPrt3(fmt, ...)
#define DbgCmdEnable 0
#endif

#ifdef M3D_PROFILE
#define DbgPrtTime(kTime,fmt, ...) \
    do_gettimeofday(&kTime); \
    printk(KERN_INFO"[M3D]%d.%d "fmt, (UINT32)kTime.tv_sec, (UINT32)kTime.tv_usec, ##__VA_ARGS__);
#define DbgPrt2Time(kTime,fmt, ...) \
    do_gettimeofday(&kTime); \
    printk(KERN_INFO"[M3D2]%d.%d "fmt, (UINT32)kTime.tv_sec, (UINT32)kTime.tv_usec, ##__VA_ARGS__);
#else
#define DbgPrtTime(kTime,fmt, ...)
#define DbgPrt2Time(kTime,fmt, ...)
#endif

#ifdef M3D_PROC_PROFILE
static M3DContextStruct *g_pCurCtx;
static struct timeval gSWTimeBegin;
static struct timeval gSWTimeEnd;
static struct timeval gHWTimeBegin;
static struct timeval gHWTimeEnd;
static struct timeval gInterruptTimeEnd;
#define GetProfile(start, end) \
((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec));
#define SWTimeBegin(pCtx) \
    if(CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){\
        do_gettimeofday(&gSWTimeBegin);}
#define SWTimeEnd(pCtx) \
    if(CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){\
        do_gettimeofday(&gSWTimeEnd);\
        pCtx->kProfileData.SWTime += GetProfile(gSWTimeBegin, gSWTimeEnd);}
#define HWTimeBegin(pCtx) \
    if(CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){\
        do_gettimeofday(&gHWTimeBegin);\
        gInterruptTimeEnd = gHWTimeBegin;}
#define HWTimeEnd(pCtx) \
    if(CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){\
        do_gettimeofday(&gHWTimeEnd);\
        pCtx->kProfileData.HWTime += GetProfile(gHWTimeBegin, gHWTimeEnd);\
        pCtx->kProfileData.InterruptTime += GetProfile(gHWTimeBegin, gInterruptTimeEnd);\
    }
#define IOCTLTimeEnd(pCtx, time) \
    if(pCtx && CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){\
        struct timeval kTimeEnd;\
        do_gettimeofday(&kTimeEnd);\
        pCtx->kProfileData.IOCTLTime += GetProfile(time, kTimeEnd);\
    }
#else
#define SWTimeBegin(pCtx)
#define SWTimeEnd(pCtx)
#define HWTimeBegin(pCtx)
#define HWTimeEnd(pCtx)
#define IOCTLTimeEnd(pCtx, time)
#endif

#if DbgCmdEnable
#define DbgCmd(fmt, ...)  \
    printk(KERN_INFO"[M3D2] "fmt, ##__VA_ARGS__)
#else
#define DbgCmd(fmt, ...)
#endif

#ifdef M3D_MULTI_CONTEXT
static struct semaphore g_ctxLock;
static struct semaphore g_cmqLock;
#endif
static UINT32 g_ctxIDCount = 0;
static M3DContextStruct g_M3DContext[MAX_M3D_CONTEXT];
#ifndef M3D_MULTI_CONTEXT
static M3DContextStruct *g_currentCtx = NULL;
#endif

static void SwapDrawSurfacePhy(SwapDrawSurfacePhyStruct*);
static void UpdateContext(M3DContextStruct*, NewContextStruct*);
static void InvalCacheCtrl(M3DContextStruct*, CacheCtrlStruct*);
static void FlushCacheCtrl(CacheCtrlStruct   *pCacheCtrl);
static void UpdateTexMatrix(TexMatrixStruct  *pMatrix);
static void UpdateNormalize(NormalizeStruct  *pNormalize);
static void UpdateClipPlane(ClipPlaneStruct  *pPlane);
static void UpdateFogCtrl(FogCtrlStruct      *pFogCtrl);
static void UpdateLightCtrl(M3DContextStruct*, LightCtrlStruct*);
static void UpdateTexture(TextureCtrlStruct *pTexCtrl);
static void UpdateColorCtrl(ColorCtrlStruct *pColorCtrl);
static void UpdatePerFragmentTest(PerFragmentTestStruct *pPerFragment);
static void UpdatePolygonx(PolygonxStruct   *pPolygonx);
static void UpdateCullMode(M3DContextStruct*, CullModexStruct*);
static void UpdateStencil(StencilStruct     *pStencil);
static void UpdateArrayInfo(ArrayInfoStruct *pArrayInfo);
static void UpdateTexDrawOES(M3DContextStruct*, TexDrawOESStruct*);
static void NewDrawCommand(M3DContextStruct*, DrawEventStruct*);
static void UpdatePrimitiveCtrl(PrimitiveCtrlStruct *pPrimitiveCtrl);
static void UpdateBBVPTransformMatrix(M3DContextStruct*, BBViewportTransformStruct*);
      
static UINT8 *g_pM3DRegBase;
#ifdef M3D_MULTI_CONTEXT
static UINT8 *g_pHWRegData = NULL;
static UINT8 *g_pHWRegDirty = NULL;
static M3DContextStruct *g_pPreCtx = NULL;
#endif
/* Multi-Context should take care*/
static UINT32 g_GLESProfile;
    
/* Reference count protected by atomic */
static atomic_t g_Reference = ATOMIC_INIT(0);
static INT32 g_PowerState;

/* Common shared constant resource */
#ifdef __USING_HWDRAWTEX_CLEAR__    
static char* g_pTextureBuffer;
static UINT32 g_TextureBufferPA;
#endif

#define M3D_STOP    0x1
#define M3D_PAUSE  0x2
#define M3D_SUSPEND 0x4

static UINT32 g_u4M3DTrigger;
static UINT32 g_u4M3DStatus;

static GLuint _gl_convert_texture_src_rgba(GLenum CombineSource);
static GLuint _gl_convert_texture_operand_rgba(GLenum CombineOperand);   

__inline void M3D_WriteReg32NoBackup(UINT32 ADDR, UINT32 VAL)//No Backup
{
    *((volatile UINT32*)(g_pM3DRegBase + ADDR)) = VAL;
}

__inline void M3D_WriteReg32(UINT32 ADDR, UINT32 VAL)
{
    *((volatile UINT32*)(g_pM3DRegBase + ADDR)) = VAL;    
#ifdef M3D_MULTI_CONTEXT
    *((UINT32*)(g_pHWRegData + ADDR)) = VAL;
    g_pHWRegDirty[(ADDR >> 2) >> 3] |= 1 << ((ADDR >> 2) & 0x7);
#endif
}    

__inline void M3D_Native_WriteReg32(UINT32 ADDR, UINT32 VAL)
{
    if (__COMMON_PROFILE__ == g_GLESProfile){
        ADDR |= M3D_FLOAT_INPUT_OFFSET;
    }
    
    *((volatile UINT32*)(g_pM3DRegBase + ADDR)) = VAL;
#ifdef M3D_MULTI_CONTEXT
    *((UINT32*)(g_pHWRegData + ADDR)) = VAL;
    g_pHWRegDirty[(ADDR >> 2) >> 3] |= 1 << ((ADDR >> 2) & 0x7);
#endif
}  

__inline void M3D_FLT_WriteReg32(UINT32 ADDR, UINT32 VAL) 
{
    ADDR |= M3D_FLOAT_INPUT_OFFSET;
    *((volatile UINT32*)(g_pM3DRegBase + ADDR)) = VAL;
#ifdef M3D_MULTI_CONTEXT
    *((UINT32*)(g_pHWRegData + ADDR)) = VAL;
    g_pHWRegDirty[(ADDR >> 2) >> 3] |= 1 << ((ADDR >> 2) & 0x7);
#endif
}  

__inline UINT32 M3D_ReadReg32(UINT32 ADDR) {
    return *((volatile UINT32*)(g_pM3DRegBase + ADDR));
}

__inline UINT32 M3D_ReadSWReg32(UINT32 ADDR) {
    return *((UINT32*)(g_pHWRegData + ADDR));
}

__inline UINT32 M3D_FLT_ReadSWReg32(UINT32 ADDR) {
    return *((UINT32*)(g_pHWRegData + (ADDR | M3D_FLOAT_INPUT_OFFSET)));
}
//------------------------------------------------------------------------------
static GLuint _gl_convert_texture_src_rgba(GLenum CombineSource)
{
   switch(CombineSource)
   {
   case GL_TEXTURE:
      return M3D_TEX_SRC_TEXTURE;
   case GL_CONSTANT:
      return M3D_TEX_SRC_CONSTANT;
   case GL_PRIMARY_COLOR:
      return M3D_TEX_SRC_PRIMARY;
   case GL_PREVIOUS:
      return M3D_TEX_SRC_PREVIOUS;
   }
   GL_ASSERT(GL_FALSE);
   return 0;
}
//------------------------------------------------------------------------------
static GLuint _gl_convert_texture_operand_rgba(GLenum CombineOperand)
{
   return (CombineOperand & 0x03);
}
//------------------------------------------------------------------------------
static GLfixed _gl_x_sub(GLfixed b, GLfixed c)              \
{                                                           \
    GLfixed res;                                            \
    GLfixed al;                                             \
    GLfixed ah;                                             \
    GLint64 temp;                                           \
                                                            \
    temp = ((GLint64)b - c);                                \
    al = (GLfixed)((temp >> 0) & 0xFFFFFFFF);               \
    ah = (GLfixed)((temp>> 32) & 0xFFFFFFFF);               \
                                                            \
    if ((ah >= 0) && (((GLuint)al) >= 0x7FFFFFFF)) {        \
        res = 0x7FFFFFFF;                                   \
    } else if ((ah < 0) && (((GLuint)al) < 0x80000000)) {   \
        res = (GLfixed)0x80000000;                          \
    } else {                                                \
        res = al;                                           \
    }                                                       \
                                                            \
    return res;                                             \
}
//------------------------------------------------------------------------------
#ifdef M3D_HW_DEBUG_HANG
static void M3D_HWReg_Dump(M3DContextStruct* pCtx)
{
    UINT32* pData;
    GLuint i;
    CMQInfoStruct* pCMQInfo = pCtx->pCMQInfo;
    static bool bFirstTime = true;

    if (!bFirstTime)
        return;

    printk("memoryPA = %p\n", pCMQInfo->memoryPA);
    printk("memoryPA End = %p\n", pCMQInfo->memoryPA + pCMQInfo->memorySize);
    printk("Phy readPointer = %p\n", pCMQInfo->memoryPA + pCMQInfo->infoSize +
        (UINT32)pCMQInfo->readPointer - (UINT32)pCMQInfo->bufferBase);
    printk("Phy writePointer = %p\n", pCMQInfo->memoryPA + pCMQInfo->infoSize +
        (UINT32)pCMQInfo->writePointer - (UINT32)pCMQInfo->bufferBase);
    printk("U Vir bufferBase = %p\n", pCMQInfo->bufferBase);
    printk("U Vir readPointer = %p\n", pCMQInfo->readPointer);
    printk("U Vir writePointer = %p\n", pCMQInfo->writePointer);
    pData = (UINT32*)g_pHWRegDirty;
    for (i = 0; i < M3D_HW_REGDIRTY_SIZE; i+=32)
    {
        printk("[%08x] %08x %08x %08x %08x %08x %08x %08x %08x\n", i,
            pData[0], pData[1], pData[2], pData[3],
            pData[4], pData[5], pData[6], pData[7]);
            pData += 8;
    }
    printk("====================================================\n");
    pData = (UINT32*)g_pM3DRegBase;//pCtx->pHWRegData;
    for (i = 0; i < M3D_HW_REGDATA_SIZE; i+=32)
    {
        printk("[%08x] %08x %08x %08x %08x %08x %08x %08x %08x\n",
            /*0xF00A2000 +*/ i,
            pData[0], pData[1], pData[2], pData[3],
            pData[4], pData[5], pData[6], pData[7]);
            pData += 8;
    }
    bFirstTime = false;
}
#endif

#ifdef __M3D_INTERRUPT__
static DECLARE_WAIT_QUEUE_HEAD(gM3DWaitQueueHead);
static DECLARE_WAIT_QUEUE_HEAD(gWaitQueueHead);
#ifdef M3D_MULTI_CONTEXT
#define HWREG_RESTORE_OFFSET 0x28
#define MAX_CONTEXT_QUEUE 64
static struct semaphore g_QueueLock;
static struct semaphore gQueueSemaphore;
static M3DContextStruct* g_CtxQueue[MAX_CONTEXT_QUEUE];
static int g_iCtxReader = 0;
static int g_iCtxWriter = 0;
#ifdef M3D_DEBUG
static int g_iCtxTotal = 0;
#endif

#ifdef SUPPORT_LOCK_DRIVER
//static M3DContextStruct* g_BackupCtxArray[MAX_CONTEXT_QUEUE];
//static int g_iBackupNum;
static M3DContextStruct* g_pCtxLockDriver = NULL;
static DECLARE_WAIT_QUEUE_HEAD(gCtxWaitQueueHead);
#endif
//------------------------------------------------------------------------------
static void CtxQueue_Init(void)
{
    memset(g_CtxQueue, 0, sizeof(g_CtxQueue));
    sema_init(&gQueueSemaphore, MAX_CONTEXT_QUEUE - 1);
    init_MUTEX(&g_QueueLock);
}
//------------------------------------------------------------------------------
static int CtxQueue_Push(M3DContextStruct* pCtx)
{
    int i;

    if (down_interruptible(&gQueueSemaphore)) { /* get the slot */
        return -ERESTARTSYS;
    }
    
    if (down_interruptible(&g_QueueLock)) { /* get the lock */
        up(&gQueueSemaphore);
        return -ERESTARTSYS;
    }
    
    i = g_iCtxWriter;
    g_CtxQueue[i] = pCtx;

    i++;
    if (UNLIKELY(i >= MAX_CONTEXT_QUEUE)){
        i = 0;
    }
    g_iCtxWriter = i;
    
#ifdef M3D_DEBUG /* protected by gQueueSemaphore */
    g_iCtxTotal++;
    DbgPrt("(%d, %x) Push - Total Ctx is %d\n", 
        pCtx->pid, pCtx->ctxHandle, g_iCtxTotal);
    if (UNLIKELY(i == g_iCtxReader)){
        DbgPrt("CtxQueue is full now\n");
    }
#endif
    
    up(&g_QueueLock); /* release the lock */
    return 0;
}
//------------------------------------------------------------------------------
static int CtxQueue_Pop(M3DContextStruct** ppCtx)
{
    int i;
    ASSERT(ppCtx);
    if (down_interruptible(&g_QueueLock)) { /* get the lock */
        return -ERESTARTSYS;
    }

    i = g_iCtxReader;
    if (UNLIKELY(i == g_iCtxWriter)){
        *ppCtx = NULL;
        goto END;
    }
    
    *ppCtx = g_CtxQueue[i];
    g_CtxQueue[i] = NULL;

    i++;
    if (UNLIKELY(i >= MAX_CONTEXT_QUEUE)){
        i = 0;
    }
    g_iCtxReader = i;

#ifdef M3D_DEBUG /* protected by gQueueSemaphore */
    g_iCtxTotal--;
    DbgPrt2("Pop - Total Ctx is %d\n", g_iCtxTotal);
#endif
    up(&gQueueSemaphore); /* release the slot */

END:
    up(&g_QueueLock); /* release the lock */
    return 0;
}
//------------------------------------------------------------------------------
#ifndef CMD_LIST_BREAK
static bool CtxQueue_TryToMerge(M3DContextStruct* pCtx)
{
    int i;
    bool bRet = false;
    if (down_interruptible(&g_QueueLock)) { /* get the lock */
        return -ERESTARTSYS;
    }

    i = g_iCtxWriter - 1;
    if (i < 0){
        i = MAX_CONTEXT_QUEUE - 1;
    }
    if (pCtx == g_CtxQueue[i]){
        bRet = true;
    }
    up(&g_QueueLock); /* release the lock */
    return bRet;
}
#endif
static int CtxQueue_Remove(M3DContextStruct* pCtx)
{
    int i;
    M3DContextStruct** ppCtx;
    ASSERT(pCtx);

    //printk(KERN_INFO"[M3D] wait m3dthread paused begin\n");
    g_u4M3DTrigger |= M3D_PAUSE;
    wake_up_interruptible(&gM3DWaitQueueHead);

    wait_event_interruptible(gWaitQueueHead,
        g_u4M3DTrigger == g_u4M3DStatus);

    if (down_interruptible(&g_QueueLock)) { /* get the lock */
        return -ERESTARTSYS;
    }
    
    ppCtx = g_CtxQueue;
    for (i = 0; i < MAX_CONTEXT_QUEUE; ++i){
        if (*ppCtx == pCtx){
            *ppCtx = NULL;
        }
        ppCtx++;
    }

    g_u4M3DTrigger &= ~M3D_PAUSE;
    wake_up_interruptible(&gM3DWaitQueueHead);
    //printk(KERN_INFO"[M3D] wait m3dthread paused end\n");
    
    up(&g_QueueLock); /* release the lock */    
    return 0;
}

#ifdef SUPPORT_LOCK_DRIVER
static int LockDriverForCtx(M3DContextStruct* pCtx)
{
    ASSERT(pCtx);

    if (g_pCtxLockDriver){
        printk(KERN_ERR"[M3D] %p Lock Assert!\n", g_pCtxLockDriver);        
        return -1;
    }
    
    g_pCtxLockDriver = pCtx;
    //printk(KERN_INFO"[M3D] %p Lock Driver \n", g_pCtxLockDriver);
// Backup Plan
#if 0
    if (down_interruptible(&g_QueueLock)) { /* get the lock */
        return -ERESTARTSYS;
    }

    ppCtx = &g_CtxQueue[g_iCtxReader];
    g_iBackupNum = 0;
    if (g_iCtxReader < g_iCtxWriter)
    {
        for (i = g_iCtxReader; i < g_iCtxWriter; ++i){
            pExistCtx = *ppCtx;
            if (pExistCtx && pExistCtx != pCtx){
                g_BackupCtxArray[g_iBackupNum++] = pExistCtx;
                *ppCtx = NULL;
            }
            ppCtx++;
        }
    }
    else
    {
        for (i = g_iCtxReader; i < MAX_CONTEXT_QUEUE; ++i){
            pExistCtx = *ppCtx;
            if (pExistCtx && pExistCtx != pCtx){
                g_BackupCtxArray[g_iBackupNum++] = pExistCtx;
                *ppCtx = NULL;
            }
            ppCtx++;
        }
        ppCtx = g_CtxQueue;
        for (i = 0; i < g_iCtxWriter; ++i){
            pExistCtx = *ppCtx;
            if (pExistCtx && pExistCtx != pCtx){
                g_BackupCtxArray[g_iBackupNum++] = pExistCtx;
                *ppCtx = NULL;
            }
            ppCtx++;
        }
    }
    up(&g_QueueLock); /* release the lock */
#endif
    return 0;
}

static void UnlockDriverForCtx(M3DContextStruct* pCtx)
{
    if (g_pCtxLockDriver != pCtx)
    {
        printk(KERN_ERR"[M3D] %p Unlock Assert!\n", g_pCtxLockDriver);
        return;
    }
// Backup Plan
#if 0
    ppCtx = g_BackupCtxArray;
    for (i = 0; i < g_iBackupNum; ++i)
    {
        CtxQueue_Push(*ppCtx);
        ppCtx++;
    }
    g_iBackupNum = 0;
#endif
    //printk(KERN_INFO"[M3D] %p Unlock Driver\n", g_pCtxLockDriver);
    g_pCtxLockDriver = NULL;
    wake_up_interruptible(&gCtxWaitQueueHead);    
}
#endif
//------------------------------------------------------------------------------
static void ResetHWReg(void)
{
    M3D_WriteReg32NoBackup(M3D_PRIMITIVE_AA, 0);
    M3D_WriteReg32NoBackup(M3D_POLYGON_OFFSET_ENABLE, 0);
    M3D_WriteReg32NoBackup(M3D_POINT_SPRITE, 0);
    M3D_WriteReg32NoBackup(M3D_VERTEX, 0);
    M3D_WriteReg32NoBackup(M3D_BBOX_EXPAND, M3D_BBOX_EXPAND_X | M3D_BBOX_EXPAND_Y);
    M3D_WriteReg32NoBackup(M3D_COLOR_0, M3D_ENABLE);
    M3D_WriteReg32NoBackup(M3D_COLOR_1, M3D_ENABLE);
    M3D_WriteReg32NoBackup(M3D_TEX_COORD_0, 0x22);
    M3D_WriteReg32NoBackup(M3D_TEX_COORD_1, 0x22);
    M3D_WriteReg32NoBackup(M3D_TEX_COORD_2, 0x22);
    M3D_WriteReg32NoBackup(M3D_PNT_SIZE_INPUT, 0);
    M3D_WriteReg32NoBackup(M3D_CLIP_PLANE_ENABLE, 0);
    M3D_WriteReg32NoBackup(M3D_NORMAL_SCALE_ENABLE, 0);
    M3D_WriteReg32NoBackup(M3D_LINE_LAST_PIXEL, 0);
    M3D_WriteReg32NoBackup(M3D_CULL, 0xBF);
    M3D_WriteReg32NoBackup(M3D_SHADE_MODEL, M3D_SHADE_SMOOTH);
    M3D_WriteReg32NoBackup(M3D_LIGHT_CTRL, 0);
    M3D_WriteReg32NoBackup(M3D_TEX_CTRL, 0x07E00000);
    M3D_WriteReg32NoBackup(M3D_SCISSOR_ENABLE, 0);
    M3D_WriteReg32NoBackup(M3D_SCISSOR_LEFT, 0);
    M3D_WriteReg32NoBackup(M3D_SCISSOR_BOTTOM, 0);
    M3D_WriteReg32NoBackup(M3D_SCISSOR_RIGHT, 0);
    M3D_WriteReg32NoBackup(M3D_SCISSOR_TOP, 0);
    M3D_WriteReg32NoBackup(M3D_ALPHA_TEST, 0);
    M3D_WriteReg32NoBackup(M3D_STENCIL_TEST, 0);
    M3D_WriteReg32NoBackup(M3D_DEPTH_TEST, 0);
    M3D_WriteReg32NoBackup(M3D_BLEND, 0x1E00);
    M3D_WriteReg32NoBackup(M3D_LOGIC_OP, 0);
    M3D_WriteReg32NoBackup(M3D_COLOR_FORMAT, 0);
    M3D_WriteReg32NoBackup(M3D_COLOR_BUFFER, 0);    
    M3D_WriteReg32NoBackup(M3D_BUFFER_WIDTH, 0);
    M3D_WriteReg32NoBackup(M3D_BUFFER_HEIGHT, 0);
    M3D_WriteReg32NoBackup(M3D_STENCIL_MASK, 0xFF);
    M3D_WriteReg32NoBackup(M3D_DEPTH_CLEAR, 0);
}
//------------------------------------------------------------------------------
static void BackupHWContext(M3DContextStruct* pCtx)
{
    //memcpy(pCtx->pHWRegData, g_au4HWInit, M3D_HW_REGDATA_SIZE);
    memset(pCtx->pHWRegData, 0, M3D_HW_REGDATA_SIZE);
    memset(pCtx->auDirty,    0, M3D_HW_REGDIRTY_SIZE);
}
//------------------------------------------------------------------------------
static void RestoreHWContext(M3DContextStruct* pCtx)
{
    int i;
    UINT32 *pSrc, *pDst;
    UINT8* pDirty;

    ResetHWReg();

    //printk("1 M3D_ReadReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadReg32(M3D_COLOR_BUFFER));
    //printk("1 M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
    
    pSrc = (UINT32*)(pCtx->pHWRegData + HWREG_RESTORE_OFFSET);
    pDst = (UINT32*)(g_pM3DRegBase + HWREG_RESTORE_OFFSET);
    pDirty = g_pHWRegDirty;
#if 1
    for (i = HWREG_RESTORE_OFFSET; i < M3D_HW_REGDATA_SIZE; 
          i += sizeof(UINT32*), pDst++, pSrc++)
    {
        if (pDirty[(i >> 2) >> 3] & (1 << ((i >> 2) & 0x7)))
        {
            *((volatile UINT32*)pDst) = *((UINT32*)pSrc);
        }
    }
#else
    if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
    {
        for (i = HWREG_RESTORE_OFFSET; i < 0x800;
              i += sizeof(UINT32*), pDst++, pSrc++)
        {
            if (pDirty[(i >> 2) >> 3] & (1 << ((i >> 2) & 0x7)))
            {
                *((volatile UINT32*)pDst) = *((UINT32*)pSrc);
            }
        }
    }
    else
    {
        for (i = HWREG_RESTORE_OFFSET; i < 0x260; 
              i += sizeof(UINT32*), pDst++, pSrc++)
        {
            if (pDirty[(i >> 2) >> 3] & (1 << ((i >> 2) & 0x7)))
            {
                *((volatile UINT32*)pDst) = *((UINT32*)pSrc);
            }
        }
        pSrc = (UINT32*)(pCtx->pHWRegData + 0x84c);
        pDst = (UINT32*)(g_pM3DRegBase + 0x84c);              
        for (i = 0x84c; i < M3D_HW_REGDATA_SIZE; 
              i += sizeof(UINT32*), pDst++, pSrc++)
        {
            if (pDirty[(i >> 2) >> 3] & (1 << ((i >> 2) & 0x7)))
            {
                *((volatile UINT32*)pDst) = *((UINT32*)pSrc);
            }
        }              
    }
#endif
    //printk("2 M3D_ReadReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadReg32(M3D_COLOR_BUFFER));
    //printk("2 M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
}
#else
static struct semaphore gSemaphore;
#endif
static struct completion gCompRenderDone;
static struct task_struct* gpTask;
static int M3D_thread(void *data);
static __tcmfunc irqreturn_t mt6516_m3d_irq_handler(int irqno, void *dev_id)
//static irqreturn_t mt6516_m3d_irq_handler(int irqno, void *dev_id)
{
    UINT32 uStatus = M3D_ReadReg32(M3D_INTERRUPT_STATUS);
   	(void)irqno;
    //DbgPrt3("mt6516_m3d_irq_handler: enter\n");
    if (uStatus & M3D_RENDER_DONE_INTR){
        //DbgPrt3("mt6516_m3d_irq_handler: complete\n");
#ifdef M3D_PROC_PROFILE
        if(CHECK_PROC_DEBUG(g_pCurCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){
            do_gettimeofday(&gInterruptTimeEnd);
        }
#endif
        M3D_WriteReg32NoBackup(M3D_INTERRUPT_ACK, M3D_RENDER_DONE_INTR);
        complete(&gCompRenderDone);
    } 
    return IRQ_HANDLED;
}
#endif
//------------------------------------------------------------------------------
INT32 M3dInit(void)
{
#ifdef __M3D_INTERRUPT__
    int err;
#endif
    DbgPrt("M3dInit enter\n");
    memset(g_M3DContext, 0, sizeof(g_M3DContext));
    g_PowerState = M3D_STATE_POWER_OFF;

#ifdef __USING_HWDRAWTEX_CLEAR__
    g_pTextureBuffer = kmalloc(4 * sizeof(UINT32), GFP_KERNEL);
    if (!g_pTextureBuffer){
        printk(KERN_ERR "[M3D] alloc tmp texture buffer fail\n");
        return M3D_STATUS_OUTOFMEM;
    }
    DbgPrt("M3dInit: allocate tmp texture buffer\n");
    g_TextureBufferPA = (UINT32)__virt_to_phys(g_pTextureBuffer);
#endif

#ifdef __M3D_INTERRUPT__
#ifdef M3D_MULTI_CONTEXT
    init_MUTEX(&g_ctxLock);
    init_MUTEX(&g_cmqLock);
    CtxQueue_Init();
#else
    init_MUTEX(&gSemaphore);
#endif
    /*  register M3D IRQ handler. */
    err = request_irq(MT6516_M3D_IRQ_LINE, mt6516_m3d_irq_handler, 0, 
                     "MT6516-M3D", NULL);
    if (0 != err) {
        printk(KERN_ERR "[M3D] Request IRQ fail: err = %d\n", err);
        return err;
    }
    DbgPrt("M3dInit: request irq\n");
    
    /* create m3d thread and run */
    g_u4M3DTrigger = g_u4M3DStatus = 0;
    gpTask = kthread_create(M3D_thread, NULL, "m3d_thread");
    if (IS_ERR(gpTask)) {
        return PTR_ERR(gpTask);
    }
    DbgPrt("M3dInit: create M3D thread\n");
    wake_up_process(gpTask);
#endif

#ifdef __DRV_TEAPOT_DEBUG__
    TestDriver();
#endif

    return 0;
}
//------------------------------------------------------------------------------
INT32 M3dDeinit(void)
{
#ifdef __M3D_INTERRUPT__
    g_u4M3DTrigger |= M3D_STOP;
#ifdef M3D_MULTI_CONTEXT
    wake_up_interruptible(&gM3DWaitQueueHead);
#else
    up(&gSemaphore);
    DbgPrt("M3dDeinit: decrease gSemaphore\n");
#endif
    kthread_stop(gpTask);
    DbgPrt("M3dDeinit: stop M3D thread\n");
    free_irq(MT6516_M3D_IRQ_LINE, NULL);
    DbgPrt("M3dDeinit: free irq\n");
#endif

#ifdef __USING_HWDRAWTEX_CLEAR__
    kfree(g_pTextureBuffer);
    DbgPrt("M3dDeinit: free tmp texture buffer\n");
#endif
    return 0;
}
//------------------------------------------------------------------------------
static INT32 PowerOn(void)
{
    hwEnableClock(MT6516_PDN_MM_GMC2,"M3D");
    hwEnableClock(MT6516_PDN_MM_M3D,"M3D");

    //set register base address
    g_pM3DRegBase = (UINT8*)M3D_BASE;

    //open GRAPHSYS2 SRAM
    *((volatile UINT32*)G2_MEM_PDN) &= ~0x1;

    //set internal SRAM for vertex cache
    M3D_WriteReg32NoBackup(M3D_VERTEX_CACHE_POINTER, GRAPH_SYS_RAM2_M3D_PA);

#ifdef __M3D_INTERRUPT__
    /*  register M3D IRQ handler. */
    M3D_WriteReg32NoBackup(M3D_INTERRUPT_ACK, M3D_RENDER_DONE_INTR);
    M3D_WriteReg32NoBackup(M3D_INTERRUPT_ENABLE, M3D_RENDER_DONE_INTR);
#endif

   return M3D_STATUS_SUCCESS;
}
//------------------------------------------------------------------------------
static INT32 PowerOff(void)
{
    hwDisableClock(MT6516_PDN_MM_M3D,"M3D");
    hwDisableClock(MT6516_PDN_MM_GMC2,"M3D");

    //close GRAPHSYS2 SRAM
    *((volatile UINT32*)G2_MEM_PDN) |= 0x1;

    return M3D_STATUS_SUCCESS;
}
//------------------------------------------------------------------------------
static INT32 AddReference(void)
{
    DbgPrt("AddReference: enter g_Reference = %x\n", atomic_read(&g_Reference));
    DbgPrt("AddReference: enter g_PowerState = %x\n", g_PowerState);
    /* increase the reference counter */    
    atomic_inc(&g_Reference);
    
    if ((1 == atomic_read(&g_Reference)) && 
        (M3D_STATE_POWER_OFF == g_PowerState))
    {
        PowerOn();
        DbgPrt("AddReference: PowerOn()\n");
        g_PowerState = M3D_STATE_POWER_ON;
    }

    DbgPrt("AddReference: leave g_Reference = %x\n", atomic_read(&g_Reference));
    DbgPrt("AddReference: leave g_PowerState = %x\n", g_PowerState);
    
    return M3D_STATUS_SUCCESS;
}
//------------------------------------------------------------------------------
static INT32 DecReference(void)
{
    DbgPrt("DecReference: enter g_Reference = %d\n", atomic_read(&g_Reference));
    DbgPrt("DecReference: enter g_PowerState = %d\n", g_PowerState);
    
    /* decrease the reference counter */
    atomic_dec(&g_Reference);

    if ((0 == atomic_read(&g_Reference)) && 
        (M3D_STATE_POWER_ON == g_PowerState))
    {
        PowerOff();
        DbgPrt("DecReference: PowerOff()\n");        
        g_PowerState = M3D_STATE_POWER_OFF; 
    }

    DbgPrt("DecReference: leave g_Reference = %x\n", atomic_read(&g_Reference));
    DbgPrt("DecReference: leave g_PowerState = %x\n", g_PowerState);

    return M3D_STATUS_SUCCESS;
}
//------------------------------------------------------------------------------
#ifndef _MT6516_GLES_CMQ_
static void UpdateRenderState(UINT32 condition, struct MESSAGE *struct_msg)
{
        ASSERT(g_currentCtx);

        g_currentCtx->dirtyCondition |= condition;

        switch(condition) 
        {
            case NEW_ARRAY_INFO:
            	  {
        	  memcpy(&(g_currentCtx->ArrayInfo), &(struct_msg->ArrayInfo), sizeof(struct ArrayInfo));
            	  }
                break;   
            case NEW_TEXTURE:
            	  {
        	  memcpy(&(g_currentCtx->TextureCtrl), &(struct_msg->TextureCtrl), sizeof(struct TextureCtrl));
            	  }
                break;  
            case NEW_BB_VP_TRANSFORM_CTRL:
            	  {
        	  memcpy(&(g_currentCtx->BBViewPortXformCtrl), &(struct_msg->BBViewPortXformCtrl), sizeof(struct BBViewportTransform));
            	  }
                break;                  
#if 0	//move out
            case NEW_DRAW_EVENT:
            	  {
                NewDrawCommand(&(struct_msg.DrawEvent));
            	  }
                break;              
#endif          
#if 0	//move out
            case NEW_FLUSH_CACHE:
            	  {
        	  memcpy(&(g_currentCtx->CacheCtrl), &(struct_msg->CacheCtrl), sizeof(struct CacheCtrl));
            	  }
                break;                                    
#endif                
            case NEW_DRAW_CONTEXT:
            	  {
        	  memcpy(&(g_currentCtx->NewContext), &(struct_msg->NewContext), sizeof(struct NewContext));
            	  }
                break;
            case NEW_INVAL_CACHE:
            	  {
        	  memcpy(&(g_currentCtx->CacheCtrl), &(struct_msg->CacheCtrl), sizeof(struct CacheCtrl));
            	  }
                break;
            case NEW_TEX_MATRIX:
            	  {
        	  memcpy(&(g_currentCtx->TexMatrix), &(struct_msg->TexMatrix), sizeof(struct TexMatrix));
            	  }
                break;
            case NEW_NORMALIZE:
            	  {
        	  memcpy(&(g_currentCtx->Normalize), &(struct_msg->Normalize), sizeof(struct Normalize));
            	  }
                break;
            case NEW_USER_CLIP:
            	  {
        	  memcpy(&(g_currentCtx->ClipPlane), &(struct_msg->ClipPlane), sizeof(struct ClipPlane));
            	  }
                break;
            case NEW_FOG_CTRL:
            	  {
        	  memcpy(&(g_currentCtx->FogCtrl), &(struct_msg->FogCtrl), sizeof(struct FogCtrl));
            	  }
                break;
            case NEW_LIGHT_CTRL:
            	  {
        	  memcpy(&(g_currentCtx->LightCtrl), &(struct_msg->LightCtrl), sizeof(struct LightCtrlStruct));
            	  }
                break;
#if 0	//Kevin update partial lighting code from Robin on 9/16
            case NEW_MATERIAL_CTRL:
            	  {
        	  memcpy(&(g_currentCtx->MaterialCtrl), &(struct_msg->MaterialCtrl), sizeof(struct MaterialCtrl));
            	  }
                break;     
#endif                
            case NEW_PRIMITIVE_CTRL:
            	  {
        	  memcpy(&(g_currentCtx->PrimitiveCtrl), &(struct_msg->PrimitiveCtrl), sizeof(struct PrimitiveCtrl));
            	  }
                break;            
            case NEW_COLOR_CTRL:
            	  {
        	  memcpy(&(g_currentCtx->ColorCtrl), &(struct_msg->ColorCtrl), sizeof(struct ColorCtrl));
            	  }
                break;
            case NEW_PER_FRAGMENT_TEST:
            	  {
        	  memcpy(&(g_currentCtx->PerFragment), &(struct_msg->PerFragment), sizeof(struct PerFragmentTest));
            	  }
                break;
            case NEW_POLYGON:
            	  {
        	  memcpy(&(g_currentCtx->Polygonx), &(struct_msg->Polygonx), sizeof(struct Polygonx));
            	  }
                break;
            case NEW_CULL_MODE:
            	  {
        	  memcpy(&(g_currentCtx->CullModex), &(struct_msg->CullModex), sizeof(struct CullModex));
            	  }
                break;
            case NEW_STENCIL:
            	  {
        	  memcpy(&(g_currentCtx->Stencil), &(struct_msg->Stencil), sizeof(struct Stencil));
            	  }
                break;
#if 0           
            case NEW_TEXDRAWOES_CTRL:
            	  {
        	  memcpy(&(g_currentCtx->TexDrawOES), &(struct_msg->TexDrawOES), sizeof(struct TexDrawOES));
            	  }
                break;                                 
#endif          
#if 0
            case NEW_POWER_HANDLE_CTRL:
            	  {
            	  void *struct_ptr = kmalloc(sizeof(PowerEventCtrlStruct), GFP_KERNEL);
            	  ret = copy_from_user(struct_ptr, (UINT8*)arg, sizeof(PowerEventCtrlStruct));
                UpdatePowerCtrl((PowerEventCtrlStruct*)&(struct_ptr));
                kfree(struct_ptr);
            	  }
                break;                                   
#endif                
#if 0
            default:
                ASSERT(0);
                //RETAILMSG(TRUE, (TEXT("Unknown message type!\n")));
               printk("\n----MT6516 M3D: M3DWorkerEntry: Unknown message type!----\n");            
                break;
#endif          
        }

}

static void  SetRenderState(void)
{
	UINT32 condition = g_currentCtx->dirtyCondition;
	
	if (condition & NEW_DRAW_CONTEXT)
	{
		UpdateContext(&(g_currentCtx->NewContext));
	}
	
	if (condition & NEW_INVAL_CACHE)
	{
		InvalCacheCtrl(&(g_currentCtx->CacheCtrl));
	}
	
#if 0	//move out
	if (condition & NEW_FLUSH_CACHE)
       {
    	  	FlushCacheCtrl(&(g_currentCtx->CacheCtrl));
       }
#endif

	if (condition & NEW_ARRAY_INFO)
	{
        	UpdateArrayInfo(&(g_currentCtx->ArrayInfo));	
	}
	 
	if (condition & NEW_TEXTURE)
	{
		UpdateTexture(&(g_currentCtx->TextureCtrl));
	}

	if (condition & NEW_BB_VP_TRANSFORM_CTRL)
	{
		UpdateBBVPTransformMatrix(&(g_currentCtx->BBViewPortXformCtrl));
	}
#if 0	//move out
            case NEW_DRAW_EVENT:
            	  {
                NewDrawCommand(&(struct_msg.DrawEvent));
            	  }
                break;              
#endif                

	if (condition & NEW_TEX_MATRIX)
	{
		UpdateTexMatrix(&(g_currentCtx->TexMatrix));
	}

	if (condition & NEW_NORMALIZE)
	{
		UpdateNormalize(&(g_currentCtx->Normalize));
	}

	if (condition & NEW_USER_CLIP)
	{
		UpdateClipPlane(&(g_currentCtx->ClipPlane));
	}

	if (condition & NEW_FOG_CTRL)
	{
		UpdateFogCtrl(&(g_currentCtx->FogCtrl));
	}

	if (condition & NEW_LIGHT_CTRL)
	{
		UpdateLightCtrl(&(g_currentCtx->LightCtrl));
	}
#if 0	//Kevin update partial lighting code from Robin on 9/16 
	if (condition & NEW_MATERIAL_CTRL)
	{
		UpdateMaterialCtrl(&(g_currentCtx->MaterialCtrl));
	}
#endif
	if (condition & NEW_PRIMITIVE_CTRL)
	{
		UpdatePrimitiveCtrl(&(g_currentCtx->PrimitiveCtrl));
	}

	if (condition & NEW_COLOR_CTRL)
	{
		UpdateColorCtrl(&(g_currentCtx->ColorCtrl));
	}

	if (condition & NEW_PER_FRAGMENT_TEST)
	{
		UpdatePerFragmentTest(&(g_currentCtx->PerFragment));
	}

	if (condition & NEW_POLYGON)
	{
		UpdatePolygonx(&(g_currentCtx->Polygonx));
	}

	if (condition & NEW_CULL_MODE)
	{
		UpdateCullMode(&(g_currentCtx->CullModex));
	}

	if (condition & NEW_STENCIL)
	{
		UpdateStencil(&(g_currentCtx->Stencil));
	}
#if 0           
            case NEW_TEXDRAWOES_CTRL:
            	  {
                UpdateTexDrawOES(&(g_currentCtx->TexDrawOES));
            	  }
                break;                                 
#endif          
#if 0
            case NEW_POWER_HANDLE_CTRL:
            	  {
            	  void *struct_ptr = kmalloc(sizeof(PowerEventCtrlStruct), GFP_KERNEL);
            	  ret = copy_from_user(struct_ptr, (UINT8*)arg, sizeof(PowerEventCtrlStruct));
                UpdatePowerCtrl((PowerEventCtrlStruct*)&(struct_ptr));
                kfree(struct_ptr);
            	  }
                break;                                   
#endif                
#if 0
            default:
                ASSERT(0);
                //RETAILMSG(TRUE, (TEXT("Unknown message type!\n")));
               printk("\n----MT6516 M3D: M3DWorkerEntry: Unknown message type!----\n");            
                break;
#endif          

	g_currentCtx->dirtyCondition = 0;

}
#endif

static M3DContextStruct* CreateContext(UINT32 handle)
{
    UINT32 i = 0;
    M3DContextStruct* pCtx = NULL;

#ifdef M3D_MULTI_CONTEXT
    if (down_interruptible(&g_ctxLock)) { /* get the lock */
        DbgPrt("CreateContext lock interrupted\n");
        return NULL;
    }
#endif

    printk("CreateContext enter\n");
    if (g_ctxIDCount >= MAX_M3D_CONTEXT){
        printk("[M3D] 3DContext Slot Full!\n");
        goto END;
    }

    for (i = 0; i < MAX_M3D_CONTEXT; i++)
    {
        if (handle == g_M3DContext[i].ctxHandle &&
            current->pid == g_M3DContext[i].pid)
        {
            DbgPrt("pid = %d, CreateContext conflict handle = %x\n", 
                current->pid, handle);
            ASSERT(0);	//duplicate context
        }
        else if (0 == g_M3DContext[i].ctxHandle)
        {
            g_M3DContext[i].pid = current->pid;
            strcpy(g_M3DContext[i].comm, current->comm);
            g_M3DContext[i].ctxHandle = handle;
#ifdef M3D_PROC_PROFILE
            g_M3DContext[i].kProfileData.SWTime = 0;
            g_M3DContext[i].kProfileData.HWTime = 0;
            g_M3DContext[i].kProfileData.InterruptTime = 0;
            g_M3DContext[i].kProfileData.IOCTLTime = 0;
#endif            
#ifdef M3D_MULTI_CONTEXT
            g_M3DContext[i].bHWRegDataValid = FALSE;
#endif
            g_M3DContext[i].pCMQMemoryBase = NULL;
            g_ctxIDCount++;
            printk("CreateContext new handle = %x\n", handle);
            printk("CreateContext pid = %d\n", current->pid);     
            printk("CreateContext ctxIDCount = %x\n", g_ctxIDCount);            
            printk("M3D CreateContext SUCCESS!\n");
            pCtx = &(g_M3DContext[i]);
            goto END;
        }
    }
    printk("M3D CreateContext FAIL!\n");
    
END:
#ifdef M3D_MULTI_CONTEXT    
    up(&g_ctxLock);
#endif

    return pCtx;	//out of context count
}

static BOOL DestroyContext(UINT32 handle)
{
    UINT32 i = 0;
    BOOL bRet = FALSE;
    
#ifdef M3D_MULTI_CONTEXT
    if (down_interruptible(&g_ctxLock)) { /* get the lock */
        DbgPrt("CreateContext lock interrupted\n");
        return FALSE;
    }
#endif

    printk("M3D DestroyContext enter\n");
    printk("M3D DestroyContext handle = %x\n", handle);
    printk("M3D DestroyContext pid = %d\n", current->pid);

    for (i = 0; i < MAX_M3D_CONTEXT; i++)
    {
        if (handle == g_M3DContext[i].ctxHandle &&
            current->pid == g_M3DContext[i].pid)
        {
            g_M3DContext[i].ctxHandle = 0;
            g_M3DContext[i].pid = 0;
            g_ctxIDCount--;
            if (g_pPreCtx == &g_M3DContext[i])
            {
                g_pPreCtx = NULL; //invalid the previous value
            }
            printk("M3D DestroyContext ctxIDCount = %x\n", g_ctxIDCount);            
            printk("M3D DestroyContext SUCCESS!\n");
            bRet = TRUE;
            goto END;
        }
    }
    printk("M3D DestroyContext FAIL!\n");

END:
#ifdef M3D_MULTI_CONTEXT    
    up(&g_ctxLock);
#endif
    return bRet;	//can not find context
}

#ifdef M3D_MULTI_CONTEXT
static M3DContextStruct* GetContext(UINT32 handle)
{
    UINT32 i;
    M3DContextStruct* pCtx = NULL;
    if (down_interruptible(&g_ctxLock)) { /* get the lock */
        DbgPrt("CreateContext lock interrupted\n");
        return NULL;
    }
    for (i = 0; i < MAX_M3D_CONTEXT; i++)
    {
        if (handle == g_M3DContext[i].ctxHandle &&
            current->pid == g_M3DContext[i].pid)
        {
            pCtx = &(g_M3DContext[i]);
            goto END;
        }
    }

END:
    up(&g_ctxLock);
    return pCtx;
}
#else
static UINT32 GetContextID(UINT32 handle)
{
    UINT32 i, ret = MAX_M3D_CONTEXT;

    for (i = 0; i < MAX_M3D_CONTEXT; i++)
    {
        if (handle == g_M3DContext[i].ctxHandle)
        {
            ret = i;
            goto END;
        }
    }
END:
    return ret;
}
#endif

#ifdef _MT6516_GLES_CMQ_	//Kevin
static void CMQInit(M3DContextStruct* pCtx, CMQStruct* pCMQCtrl)
{
    struct vm_area_struct *vma;
    if (down_interruptible(&g_cmqLock)) { /* get the lock */
        DbgPrt("CMQInit lock interrupted\n");
        return;
    }

#ifdef M3D_HW_DEBUG_HANG
    memcpy(&pCtx->CMQCtrl, pCMQCtrl, sizeof(CMQStruct));
#endif
    
    vma = find_vma(current->mm, (UINT32)pCMQCtrl->memoryVA);
    DbgPrt("CMQInit:  vma = %x=============\n", (UINT32)vma);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    DbgPrt("CMQInit:  vma: 0x%x, 0x%x, 0x%x=============\n",  (UINT32)vma->vm_start, (UINT32)vma->vm_end, (UINT32)vma->vm_pgoff);	
    zap_page_range(vma, vma->vm_start, vma->vm_end - vma->vm_start, NULL);
    remap_pfn_range(vma , vma->vm_start , vma->vm_pgoff ,
                    vma->vm_end - vma->vm_start , vma->vm_page_prot);
    DbgPrt("CMQInit:  vma: 0x%x, 0x%x, 0x%x=============\n",  (UINT32)vma->vm_start, (UINT32)vma->vm_end, (UINT32)vma->vm_pgoff);

    pCtx->pCMQMemoryBase = (UBYTE*)ioremap_nocache ((UINT32)pCMQCtrl->memoryPA, pCMQCtrl->memorySize);
    pCtx->pCMQInfo = (CMQInfoStruct*)(pCtx->pCMQMemoryBase);
    pCtx->pCMQBufferBase = pCtx->pCMQMemoryBase + pCtx->pCMQInfo->infoSize;
    pCtx->pCMQBufferPointer = pCtx->pCMQBufferBase;
       //m3d_leave_thread = FALSE;
    DbgPrt("CMQInit:  currentCtx->pCMQMemoryBase = %x\n", (UINT32)pCtx->pCMQMemoryBase);
    DbgPrt("CMQInit:  currentCtx->pCMQBufferBase = %x\n", (UINT32)pCtx->pCMQBufferBase);
    DbgPrt("CMQInit:  currentCtx->pCMQBufferPointer = %x\n", (UINT32)pCtx->pCMQBufferPointer);

    DbgPrt("CMQInit:  currentCtx->pCMQInfo->infoBase = %x\n", (UINT32)pCtx->pCMQInfo->infoBase);
    DbgPrt("CMQInit:  currentCtx->pCMQInfo->infoSize = %x\n", (UINT32)pCtx->pCMQInfo->infoSize);
    DbgPrt("CMQInit:  currentCtx->pCMQInfo->bufferBase = %x\n", (UINT32)pCtx->pCMQInfo->bufferBase);
    DbgPrt("CMQInit:  currentCtx->pCMQInfo->bufferEnd = %x\n", (UINT32)pCtx->pCMQInfo->bufferEnd);
    DbgPrt("CMQInit:  currentCtx->pCMQInfo->bufferSize = %x\n", (UINT32)pCtx->pCMQInfo->bufferSize);

    DbgPrt("CMQInit:  currentCtx->pCMQInfo->readPointer = %x\n", (UINT32)pCtx->pCMQInfo->readPointer);
    DbgPrt("CMQInit:  currentCtx->pCMQInfo->writePointer = %x\n", (UINT32)pCtx->pCMQInfo->writePointer);

#ifdef M3D_PROC_DEBUG
    if (pCMQCtrl->HWRegDataSize == M3D_HW_REGDATA_SIZE * 2){
        vma = find_vma(current->mm, (UINT32)pCMQCtrl->HWRegDataVA);
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        zap_page_range(vma, vma->vm_start, vma->vm_end - vma->vm_start, NULL);
        remap_pfn_range(vma , vma->vm_start , vma->vm_pgoff ,
                        vma->vm_end - vma->vm_start , vma->vm_page_prot);
        pCtx->pHWRegData = (UBYTE*)ioremap_nocache(
            (UINT32)pCMQCtrl->HWRegDataPA, pCMQCtrl->HWRegDataSize);
    } else {
        pCtx->pHWRegData = NULL;
        ASSERT(0);
        printk(KERN_ERR"[M3D] HWRegDataSize is not 4096\n");
    }
#else
    pCtx->pHWRegData = kmalloc(M3D_HW_REGDATA_SIZE, GFP_KERNEL);
    if (!pCtx->pHWRegData){
        ASSERT(0);
        printk(KERN_ERR"kernel malloc failed\n");
    }
#endif
    up(&g_cmqLock);
#endif
    //AddReference();
}

static void CMQDeInit(M3DContextStruct* pCtx)
{
    if (down_interruptible(&g_cmqLock)) { /* get the lock */
        DbgPrt("CMQInit lock interrupted\n");
        return;
    }
    if (pCtx->pCMQMemoryBase) {
        iounmap(pCtx->pCMQMemoryBase);
        pCtx->pCMQMemoryBase = NULL;
        pCtx->pCMQInfo = NULL;
        pCtx->pCMQBufferBase = NULL;
        pCtx->pCMQBufferPointer = NULL;
    }
#ifdef M3D_PROC_DEBUG
    if (pCtx->pHWRegData){
        iounmap(pCtx->pHWRegData);
    }
#else
    if (pCtx->pHWRegData){
        kfree(pCtx->pHWRegData);
    }
#endif
    //DecReference();
    up(&g_cmqLock);    
}

#ifdef __M3D_INTERRUPT__
static int M3D_thread(void *data)
{
    CMQInfoStruct* pCMQInfo;
    UBYTE* pCMQBufferBase;
    UBYTE* pCMQBufferPointer;
    M3DContextStruct *pCtx = NULL;
    PMESSAGE pMsg;
    UINT32 condition;
#ifdef M3D_PROC_DEBUG
    UINT32 u4SwitchTag = 0;
    UINT32 *p;
#endif
#ifdef M3D_PROFILE
    struct timeval kTime;
#endif
#ifdef M3D_DEBUG
    int iCmdCount = 0;
#endif

    while(1)
    {
        DbgPrt2Time(kTime,"M3D_thread: sleep...\n");
#ifdef M3D_MULTI_CONTEXT
        if (wait_event_interruptible(gM3DWaitQueueHead,
                g_u4M3DTrigger || (g_iCtxReader != g_iCtxWriter)))
        {
            printk(KERN_ERR"[M3D] wait event interrupted\n");
            continue;
        }
        if (g_u4M3DTrigger & M3D_STOP){ /* stop m3d thread */
            break;
        }
        if (g_u4M3DTrigger){
            //printk(KERN_INFO"[M3D] g_u4M3DTrigger = %x\n", g_u4M3DTrigger);
            bool bReady = true;
            if ((g_u4M3DTrigger & M3D_SUSPEND) && (g_iCtxReader != g_iCtxWriter)){
                bReady = false;
            }
            if (bReady) {
                g_u4M3DStatus = g_u4M3DTrigger;
                wake_up_interruptible(&gWaitQueueHead);
                wait_event_interruptible(gM3DWaitQueueHead,
                    g_u4M3DTrigger != g_u4M3DStatus);
                g_u4M3DStatus = g_u4M3DTrigger;
                continue;
            }            
        }
#else
        if (down_interruptible(&gSemaphore)) {
            return -ERESTARTSYS;
        }
#endif
        DbgPrt2Time(kTime,"M3D_thread: ...wake up\n");

#ifdef M3D_MULTI_CONTEXT
        if (CtxQueue_Pop(&pCtx)){
            printk(KERN_ERR"[M3D] CtxQueue pop interrupted\n");
            return -ERESTARTSYS;
        }
#else
        pCtx = g_currentCtx;
#endif
        if (!pCtx || !pCtx->pCMQBufferBase) {/* take care g_currentCtx changed */
            DbgPrt2("current context not ready\n");
            continue;
        }
#ifdef M3D_PROC_PROFILE
        g_pCurCtx = pCtx;
#endif
        SWTimeBegin(pCtx);
        DbgPrt2("%s: %x ready\n", pCtx->comm, pCtx->ctxHandle);

#ifdef M3D_MULTI_CONTEXT
        if (pCtx != g_pPreCtx)
        { /* GLES context changed! */
            g_pHWRegData  = pCtx->pHWRegData;
            g_pHWRegDirty = pCtx->auDirty;
            g_GLESProfile = pCtx->NewContext.GLESProfile;            

            if (pCtx->bHWRegDataValid)
            {
                if (M3D_ReadReg32(M3D_DEPTH_BUFFER) != 0){
                    M3D_WriteReg32NoBackup(
                        M3D_CACHE_FLUSH_CTRL, M3D_DEPTH_CACHE_FLUSH);
                }
                if (M3D_ReadReg32(M3D_COLOR_BUFFER) != 0){
                    M3D_WriteReg32NoBackup(
                        M3D_CACHE_FLUSH_CTRL, M3D_COLOR_CACHE_FLUSH);
                }
                WAIT_M3D();

                M3D_WriteReg32NoBackup(M3D_CACHE_INVAL_CTRL, 
                    M3D_STECIL_CACHE_INVAL | 
                    M3D_DEPTH_CACHE_INVAL | 
                    M3D_COLOR_CACHE_INVAL);
                
                RestoreHWContext(pCtx);

#ifdef M3D_PROC_DEBUG
                if (CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_HW_REG_DUMP))
                {
                    p = (UINT32*)(pCtx->pHWRegData + 0x8);
                    *p |= u4SwitchTag << 8;
                    u4SwitchTag++;
                }
#endif
            }

            g_pPreCtx = pCtx;
        }
#endif

#ifdef M3D_DEBUG
        iCmdCount = 0;
#endif
        pCMQInfo = pCtx->pCMQInfo;
        pCMQBufferBase = pCtx->pCMQBufferBase;
        pCMQBufferPointer = pCtx->pCMQBufferPointer;
        while(!(g_u4M3DTrigger & M3D_STOP) && 
               (pCMQInfo->readPointer != pCMQInfo->writePointer))
        {
            pMsg = (PMESSAGE)pCMQBufferPointer;
            condition = pMsg->hdr.Condition;
#ifdef M3D_CMQ_CHECK
            if ((condition !=0) &&
                (pMsg->hdr.Reserved[0] != 'M' || pMsg->hdr.Reserved[1] != 'T'))
            {
                printk(KERN_ERR"pMsg->hdr.condition = %d\n", condition);
                ASSERT(0);
            }
#endif
            switch(condition)
            {
            case 0:	//NULL command => skip => reset to base
                DbgCmd("RESET_TO_BASE\n");            
                pCMQBufferPointer = pCMQBufferBase;
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case SWAP_DRAW_SURFACE_PHY:
                DbgCmd("SWAP_DRAW_SURFACE_PHY\n");
                SwapDrawSurfacePhy(&(pMsg->SwapDrawSurfacePhy));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(SwapDrawSurfacePhyStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));            
                break;

            case NEW_DRAW_CONTEXT:
                DbgCmd("NEW_DRAW_CONTEXT\n");
                UpdateContext(pCtx, &(pMsg->NewContext));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(NewContextStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_INVAL_CACHE:
                DbgCmd("NEW_INVAL_CACHE\n");
                InvalCacheCtrl(pCtx, &(pMsg->CacheCtrl));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(CacheCtrlStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_ARRAY_INFO:
                DbgCmd("NEW_ARRAY_INFO\n");
                UpdateArrayInfo(&(pMsg->ArrayInfo));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(ArrayInfoStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;        

            case NEW_TEXTURE:
                DbgCmd("NEW_TEXTURE\n");
                UpdateTexture(&(pMsg->TextureCtrl));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(TextureCtrlStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_BB_VP_TRANSFORM_CTRL:
                DbgCmd("NEW_BB_VP_TRANSFORM_CTRL\n");
                UpdateBBVPTransformMatrix(pCtx, &(pMsg->BBViewPortXformCtrl));
                pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(
                        sizeof(BBViewportTransformStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_DRAW_EVENT:
                DbgCmd("NEW_DRAW_EVENT\n");
                NewDrawCommand(pCtx, &(pMsg->DrawEvent));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(DrawEventStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_TEX_MATRIX:
                DbgCmd("NEW_TEX_MATRIX\n");
                UpdateTexMatrix(&(pMsg->TexMatrix));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(TexMatrixStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_NORMALIZE:
                DbgCmd("NEW_NORMALIZE\n");
                UpdateNormalize(&(pMsg->Normalize));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(NormalizeStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_USER_CLIP:
                DbgCmd("NEW_USER_CLIP\n");
                UpdateClipPlane(&(pMsg->ClipPlane));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(ClipPlaneStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_FOG_CTRL:
                DbgCmd("NEW_FOG_CTRL\n");
                UpdateFogCtrl(&(pMsg->FogCtrl));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(FogCtrlStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_LIGHT_CTRL:
                DbgCmd("NEW_LIGHT_CTRL\n");
                UpdateLightCtrl(pCtx, &(pMsg->LightCtrl));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(LightCtrlStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_PRIMITIVE_CTRL:
                DbgCmd("NEW_PRIMITIVE_CTRL\n");
                UpdatePrimitiveCtrl(&(pMsg->PrimitiveCtrl));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PrimitiveCtrlStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_COLOR_CTRL:
                DbgCmd("NEW_COLOR_CTRL\n");
                UpdateColorCtrl(&(pMsg->ColorCtrl));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(ColorCtrlStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_PER_FRAGMENT_TEST:
                DbgCmd("NEW_PER_FRAGMENT_TEST\n");
                UpdatePerFragmentTest(&(pMsg->PerFragment));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PerFragmentTestStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_POLYGON:
                DbgCmd("NEW_POLYGON\n");
                UpdatePolygonx(&(pMsg->Polygonx));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PolygonxStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_CULL_MODE:
                DbgCmd("NEW_CULL_MODE\n");
                UpdateCullMode(pCtx, &(pMsg->CullModex));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(CullModexStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_STENCIL:
                DbgCmd("NEW_STENCIL\n");
                UpdateStencil(&(pMsg->Stencil));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(StencilStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_TEXDRAWOES_CTRL:
                DbgCmd("NEW_TEXDRAWOES_CTRL\n");
                UpdateTexDrawOES(pCtx, &(pMsg->TexDrawOES));
                pCMQBufferPointer += 
                    GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(TexDrawOESStruct));
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_CMQ_VERTEX_BUFFER:
                DbgCmd("NEW_CMQ_VERTEX_BUFFER\n");
                pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(
                    sizeof(CMQVertexBufferStruct) + 
                    pMsg->CMQVertexBufferCtrl.size);
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;

            case NEW_FLUSH_CACHE:
                DbgCmd("NEW_FLUSH_CACHE\n");
                FlushCacheCtrl(&(pMsg->CacheCtrl));
                pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(0);
                //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));
                break;
            }

            pCtx->pCMQBufferPointer = pCMQBufferPointer;

            pCMQInfo->readPointer = 
                pCMQInfo->bufferBase + (pCMQBufferPointer - pCMQBufferBase);

#ifdef M3D_DEBUG
            iCmdCount++;
#endif

#ifdef M3D_MULTI_CONTEXT
            if (pMsg->hdr.WaitIdle)
            {
#ifdef M3D_PROC_DEBUG
                if (CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_CMDLIST_MASK))
                {
                    FlushCacheCtrl(NULL);
                }
                if (CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_HW_REG_DUMP))
                {
                    int i;
                    UINT32 *pSrc, *pDst;
                    pSrc = (UINT32*)(g_pM3DRegBase);
                    pDst = (UINT32*)(pCtx->pHWRegData + M3D_HW_REGDATA_SIZE);
                    for (i = 0; i < M3D_HW_REGDATA_SIZE; 
                          i += sizeof(UINT32*), pDst++, pSrc++)
                    {
                        *((UINT32*)pDst) = *((volatile UINT32*)pSrc);
                    }
                }
#endif
                complete(&pCtx->JobDone);
                DbgPrt2("wake up %s %x\n", pCtx->comm, pCtx->ctxHandle);
            }
#ifdef CMD_LIST_BREAK
            if (pMsg->hdr.IsListEnd){
                break;
            }
#endif
#endif
        }
        
        DbgPrt2("completed commands = %d\n", iCmdCount);

#ifndef M3D_MULTI_CONTEXT
        wake_up_interruptible(&gM3DWaitQueueHead);
        DbgPrt2("M3D_thread: wake up process context\n");
#endif
        SWTimeEnd(pCtx);
    }

    DbgPrt2("prepare to exit\n");
    return 0;
}
#else
void M3D_thread()
{
	CMQInfoStruct* pCMQInfo = g_currentCtx->pCMQInfo;
	//UBYTE* pCMQMemoryBase = g_currentCtx->pCMQMemoryBase;
	UBYTE* pCMQBufferBase = g_currentCtx->pCMQBufferBase;
	UBYTE* pCMQBufferPointer = g_currentCtx->pCMQBufferPointer;

	CMQProcessStruct* pCMQprocessCtrl = &(g_currentCtx->CMQProcessCtrl);

	//UINT32 gap = 0;

#if 0
	if ((FALSE == pCMQprocessCtrl->waitIdle) && IS_M3D_BUSY)
	{
	//printk("M3D_thread:   ((FALSE == pCMQprocessCtrl->waitIdle) && IS_M3D_BUSY) break 1=============\n");
		return;
	}
#endif

    	//gap = (pCMQInfo->writePointer > pCMQInfo->readPointer) ? (pCMQInfo->writePointer - pCMQInfo->readPointer) : (pCMQInfo->bufferSize - (pCMQInfo->readPointer - pCMQInfo->writePointer));

//printk("M3D_thread:  enter=============\n");

//printk("M3D_thread:  pCMQInfo->readPointer = %x=============\n", pCMQInfo->readPointer);
//printk("M3D_thread:  pCMQInfo->writePointer = %x=============\n", pCMQInfo->writePointer);
//printk("M3D_thread:  pCMQBufferPointer = %x=============\n", pCMQBufferPointer);

#ifdef _MT6516_GLES_PROFILE_	//Kevin for fps
       do_gettimeofday(&t1);
#endif
	//dmac_flush_range(pCMQInfo->readPointer, (pCMQInfo->readPointer + gap));	//Kevin (TODO) optimize the range
	//dmac_flush_range(pCMQInfo->memoryVA, (pCMQInfo->memoryVA + pCMQInfo->memorySize));	//Kevin (TODO) optimize the range
//printk("M3D_thread:  dmac_flush_range=============\n");
#ifdef _MT6516_GLES_PROFILE_	//Kevin for fps
       do_gettimeofday(&t2);
       flush_t1_t2 += (t2.tv_usec - t1.tv_usec);
#endif

	while(pCMQInfo->readPointer != pCMQInfo->writePointer)
	{
		PMESSAGE pMsg = (PMESSAGE)pCMQBufferPointer;
		UINT32 condition = pMsg->Condition;

//printk("M3D_thread: while(rw) before pCMQInfo->readPointer = %x=============\n", pCMQInfo->readPointer);
//printk("M3D_thread: while(rw) before pCMQInfo->writePointer = %x=============\n", pCMQInfo->writePointer);
//printk("M3D_thread: while(rw) before pCMQBufferPointer = %x=============\n", pCMQBufferPointer);

//printk("M3D_thread: while(rw) before condition = %x=============\n", condition);

#if 0
		if (NEW_POWER_HANDLE_CTRL == condition)	//this must be before check IS_M3D_BUSY since the power and reg base may not be enabled
		{
			UpdatePowerCtrl(&(pMsg->PowerEventCtrl));
			pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PowerEventCtrlStruct));

			//read/write pointer use user mode VA space
			pCMQInfo->readPointer = pCMQInfo->bufferBase + (pCMQBufferPointer - pCMQBufferBase);
		}
#endif		
		if (NEW_FLUSH_CACHE == condition)
	       {
	    	  	FlushCacheCtrl(&(pMsg->CacheCtrl));
			pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(0);
			//WAIT_M3D();

			//read/write pointer use user mode VA space
			pCMQInfo->readPointer = pCMQInfo->bufferBase + (pCMQBufferPointer - pCMQBufferBase);
//printk("M3D_thread:  if (NEW_FLUSH_CACHE == condition)=============\n");
	       }
		else
		{

#if 0
			WAIT_M3D();
			FlushCacheCtrl(NULL);
#endif
#if 1
			if (TRUE == pCMQprocessCtrl->waitIdle)
			{
				WAIT_M3D();				
    				//FlushCacheCtrl(NULL);
			}
			else if (IS_M3D_BUSY)
			{
				break;
			}
//			else
//			{
//				WAIT_M3D();				
//    				FlushCacheCtrl(NULL);
//			}
#endif
#if 0
			if ((FALSE == pCMQprocessCtrl->waitIdle) && IS_M3D_BUSY)
			{
//printk("M3D_thread:   ((FALSE == pCMQprocessCtrl->waitIdle) && IS_M3D_BUSY) break 2=============\n");
				break;
			}
			else
			{
				WAIT_M3D();
    				FlushCacheCtrl(NULL);
//printk("M3D_thread:   else ((FALSE == pCMQprocessCtrl->waitIdle) && IS_M3D_BUSY) wait idle=============\n");
//printk("M3D_thread:   else ((FALSE == pCMQprocessCtrl->waitIdle) && IS_M3D_BUSY) wait idle condition = %x=============\n", condition);
			}
#endif

			switch(condition)	
			{
				case 0:	//NULL command => skip => reset to base
				{
					pCMQBufferPointer = pCMQBufferBase;
					break;
				}
				case NEW_CREATE_CONTEXT:
				{
					CreateContext(pMsg->ctxHandle);
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(0);
					break;
				}
				case NEW_DESTROY_CONTEXT:
				{
					DestroyContext(pMsg->ctxHandle);
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(0);
					//m3d_leave_thread = TRUE;
					break;
				}
				case NEW_DRAW_CONTEXT:
				{
					UpdateContext(&(pMsg->NewContext));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(NewContextStruct));
			             break;                                 
				}
				
				case NEW_INVAL_CACHE:
				{
					InvalCacheCtrl(&(pMsg->CacheCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(CacheCtrlStruct));
			             break;                                 
				}
				
			#if 0
				case NEW_FLUSH_CACHE:
			       {
			    	  	FlushCacheCtrl(&(pMsg->CacheCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(0);
					WAIT_M3D();
			             break;                                 
			       }
			#endif

				case NEW_ARRAY_INFO:
				{
			        	UpdateArrayInfo(&(pMsg->ArrayInfo));	
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(ArrayInfoStruct));
			             break;                                 
				}
				 
				case NEW_TEXTURE:
				{
					UpdateTexture(&(pMsg->TextureCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(TextureCtrlStruct));
			             break;                                 
				}

				case NEW_BB_VP_TRANSFORM_CTRL:	//0x30
				{
					UpdateBBVPTransformMatrix(&(pMsg->BBViewPortXformCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(BBViewportTransformStruct));
			             break;                                 
				}
			#if 1
		            case NEW_DRAW_EVENT:
		            	  {
		                	NewDrawCommand(&(pMsg->DrawEvent));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(DrawEventStruct));
					//WAIT_M3D();
	 		              //wait_event_interruptible(m3dWaitQueue, g_m3d_interrupt_handler);
			              //g_m3d_interrupt_handler = FALSE;
		            	  }
		                break;              
			#endif                

				case NEW_TEX_MATRIX:
				{
					UpdateTexMatrix(&(pMsg->TexMatrix));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(TexMatrixStruct));
			             break;                                 
				}

				case NEW_NORMALIZE:
				{
					UpdateNormalize(&(pMsg->Normalize));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(NormalizeStruct));
			             break;                                 
				}

				case NEW_USER_CLIP:
				{
					UpdateClipPlane(&(pMsg->ClipPlane));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(ClipPlaneStruct));
			             break;                                 
				}

				case NEW_FOG_CTRL:
				{
					UpdateFogCtrl(&(pMsg->FogCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(FogCtrlStruct));
			             break;                                 
				}

				case NEW_LIGHT_CTRL:
				{
					UpdateLightCtrl(&(pMsg->LightCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(LightCtrlStruct));
	 		              break;                                 
				}
			#if 0	//Kevin update partial lighting code from Robin on 9/16 
				case NEW_MATERIAL_CTRL:
				{
					UpdateMaterialCtrl(&(g_currentCtx->MaterialCtrl));
			             break;                                 
				}
			#endif
				case NEW_PRIMITIVE_CTRL:
				{
					UpdatePrimitiveCtrl(&(pMsg->PrimitiveCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PrimitiveCtrlStruct));
			             break;                                 
				}

				case NEW_COLOR_CTRL:
				{
					UpdateColorCtrl(&(pMsg->ColorCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(ColorCtrlStruct));
			             break;                                 
				}

				case NEW_PER_FRAGMENT_TEST:
				{
					UpdatePerFragmentTest(&(pMsg->PerFragment));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PerFragmentTestStruct));
			             break;                                 
				}

				case NEW_POLYGON:
				{
					UpdatePolygonx(&(pMsg->Polygonx));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PolygonxStruct));
			             break;                                 
				}

				case NEW_CULL_MODE:
				{
					UpdateCullMode(&(pMsg->CullModex));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(CullModexStruct));
			             break;                                 
				}

				case NEW_STENCIL:
				{
					UpdateStencil(&(pMsg->Stencil));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(StencilStruct));
			             break;                                 
				}	
		#if 1           
				 case NEW_TEXDRAWOES_CTRL:
				{
					UpdateTexDrawOES(&(pMsg->TexDrawOES));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(TexDrawOESStruct));
					//WAIT_M3D();
	 		              //wait_event_interruptible(m3dWaitQueue, g_m3d_interrupt_handler);
			              //g_m3d_interrupt_handler = FALSE;
			             break;                                 
				}
		#endif          
		#if 0
				case NEW_POWER_HANDLE_CTRL:
				{
					UpdatePowerCtrl(&(pMsg->PowerEventCtrl));
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(PowerEventCtrlStruct));
				}
		             break;                                   
		#endif                

				case NEW_CMQ_VERTEX_BUFFER:
				{
					CMQVertexBufferStruct* pCMQVertexBufferCtrl = &(pMsg->CMQVertexBufferCtrl);
					pCMQBufferPointer += GET_CMQ_BUFFER_ALLOCATE_SIZE(sizeof(CMQVertexBufferStruct) + pCMQVertexBufferCtrl->size);
			             break;                                 
				}	

		#if 0
		         default:
		             ASSERT(0);
		             //RETAILMSG(TRUE, (TEXT("Unknown message type!\n")));
		            printk("\n----MT6516 M3D: M3DWorkerEntry: Unknown message type!----\n");            
		             break;
		#endif          

			}

			//read/write pointer use user mode VA space
			pCMQInfo->readPointer = pCMQInfo->bufferBase + (pCMQBufferPointer - pCMQBufferBase);
		}


//printk("M3D_thread: while(rw) before pCMQInfo->readPointer = %x=============\n", pCMQInfo->readPointer);
//printk("M3D_thread: while(rw) before pCMQInfo->writePointer = %x=============\n", pCMQInfo->writePointer);
//printk("M3D_thread: while(rw) before pCMQBufferPointer = %x=============\n", pCMQBufferPointer);
	
	}

	if (TRUE == pCMQprocessCtrl->waitIdle)
	{
    		FlushCacheCtrl(NULL);
	}

	g_currentCtx->pCMQBufferPointer = pCMQBufferPointer;  //write back to context
	
//printk("M3D_thread:  leave=============\n");

}
#endif

void M3DOpen(struct file *file)
{
#if 1//Kevin use to get app name
    struct task_struct *p = current;
    printk("=====current process name is %s=================\n", p->comm);
    printk("=====current process pid is %d===================\n", p->pid);
    p = p->real_parent;
    printk("=====current real_parent process name is %s=================\n", p->comm);
    printk("=====current real_parent process pid is %d===================\n", p->pid);
#endif
    file->private_data = NULL;
    AddReference();
}

void M3DRelease(struct file *file)
{
    M3DContextStruct *pCtx = file->private_data;
    if (pCtx)
    {
#ifdef M3D_MULTI_CONTEXT
        down(&g_ctxLock);
#endif
        printk("M3D M3DRelease enter\n");
        printk("M3D M3DRelease pid = %d\n", current->pid);
#ifdef M3D_MULTI_CONTEXT
        CtxQueue_Remove(pCtx);
#endif
        CMQDeInit(pCtx);
#ifdef SUPPORT_LOCK_DRIVER        
        if (g_pCtxLockDriver && g_pCtxLockDriver == pCtx){
            UnlockDriverForCtx(pCtx);
        }
#endif
        g_ctxIDCount--;            
        printk("M3D M3DRelease handle = %x\n", pCtx->ctxHandle);
        printk("M3D M3DRelease ctxIDCount = %x\n", g_ctxIDCount);
        pCtx->ctxHandle = 0;
        pCtx->pid = 0;
        if (g_pPreCtx == pCtx){
            g_pPreCtx = NULL;//invalid the previous value
        }
#ifdef M3D_MULTI_CONTEXT    
        up(&g_ctxLock);
#endif
        file->private_data = NULL;
    }

    DecReference();
}

void M3DSuspend()
{
    //printk(KERN_INFO"[M3D] wait m3dthread suspend begin\n");
    g_u4M3DTrigger |= M3D_SUSPEND;    
    wake_up_interruptible(&gM3DWaitQueueHead);

    wait_event_interruptible(gWaitQueueHead,
        g_u4M3DTrigger == g_u4M3DStatus);
    
    g_pPreCtx = NULL;
    PowerOff();
}

void M3DResume()
{
#ifdef M3D_DEBUG
    int i = atomic_read(&g_Reference);
#endif
    DbgPrt("M3DResume: enter g_Reference = %x\n", i);
    DbgPrt("M3DResume: enter g_Reference = %x\n", i);

    if ((1 <= atomic_read(&g_Reference)) && 
        (M3D_STATE_POWER_ON == g_PowerState))
    {
        PowerOn();
        DbgPrt("M3DResume: PowerOn()\n");
    }
    g_u4M3DTrigger &= ~M3D_SUSPEND;
    wake_up_interruptible(&gM3DWaitQueueHead);
    //printk(KERN_INFO"[M3D] wait m3dthread suspend end\n");
    DbgPrt("M3DResume: leave g_Reference = %x\n", i);
    DbgPrt("M3DResume: leave g_Reference = %x\n", i);    
}

#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
char *
strtok_r(char *s, const char *delim, char **last)
{
    char *spanp;
    int c, sc;
    char *tok;

    if (s == NULL && (s = *last) == NULL)
        return (NULL);

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) {/* no non-delimiter characters */
        *last = NULL;
        return (NULL);
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *last = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

char *
strtok(char *s, const char *delim)
{
    static char *last;
    return strtok_r(s, delim, &last);
}

int M3DProcRead()
{
    UINT32 i;
    M3DContextStruct* pCtx = NULL;
    printk(KERN_INFO"M3D Proc Debug:\n");
    printk(KERN_INFO"\t-p pid\n");
    printk(KERN_INFO"\t-d debugFlag\n");
    printk(KERN_INFO"\t-n debugCount\n");

    for (i = 0; i < MAX_M3D_CONTEXT; i++)
    {
        pCtx = &(g_M3DContext[i]);
        if (pCtx->ctxHandle)
        {
            printk(KERN_INFO"pid=%d handle=%d debugFlag=0x%08x debugCount=%u\n",
                pCtx->pid, pCtx->ctxHandle, pCtx->u4DebugFlag, pCtx->u4DebugCount);
        }
    }
    
    return 0;
}

int M3DProcWrite(const char* buffer, unsigned long count)
{
    UINT32 i;
    M3DContextStruct* pCtx = NULL;
    UINT32 pid = 0, u4DebugFlag = 0, u4DebugCount = (UINT32)-1;
    char *pBuf, *p;
    if (count < 1)
    {
        return 0;
    }

    pBuf = kmalloc(count, GFP_KERNEL);
    if (copy_from_user(pBuf, buffer, count)) {
        kfree(pBuf);
        return -EFAULT;
    }

    p = strtok(pBuf, " ");
    while(p){
        if (strcmp(p, "-p") == 0){
            p = strtok(NULL, " ");
            if (p){
                pid = (UINT32)simple_strtol(p, NULL, 10);
            }
        } else if (strcmp(p, "-d") == 0){
            p = strtok(NULL, " ");
            if (p){
                u4DebugFlag = (UINT32)simple_strtol(p, NULL, 10);
            }
        } else if (strcmp(p, "-n") == 0){
            p = strtok(NULL, " ");
            if (p){
                u4DebugCount = (UINT32)simple_strtol(p, NULL, 10);
            }
        }
        p = strtok(NULL, " ");
    }
    kfree(pBuf);

    printk(KERN_INFO"M3D Proc Debug:\n");
    printk(KERN_INFO"\tpid = %d\n", pid);
    printk(KERN_INFO"\tdebugFlag = 0x%08x\n", u4DebugFlag);
    printk(KERN_INFO"\tdebugCount = %d\n", u4DebugCount);

    if (down_interruptible(&g_ctxLock)) { /* get the lock */
        return 0;
    }
    
    if (pid == 0)
    {
        for (i = 0; i < MAX_M3D_CONTEXT; i++)
        {
            pCtx = &(g_M3DContext[i]);
            if (pCtx->ctxHandle){                
                pCtx->u4DebugFlag = u4DebugFlag;
                pCtx->u4DebugCount = u4DebugCount;
            }
        }
    }
    else
    {
        for (i = 0; i < MAX_M3D_CONTEXT; i++)
        {
            pCtx = &(g_M3DContext[i]);            
            if (pCtx->ctxHandle && pCtx->pid == pid)
            {
                pCtx->u4DebugFlag = u4DebugFlag;
                pCtx->u4DebugCount = u4DebugCount;
            }
        }
    }

    up(&g_ctxLock);
    
    return 0;
}
#endif

#ifdef __M3D_INTERRUPT__
int M3DWorkerEntry(struct file *file, UINT32 condition, UINT32 arg)
{        
    int ret = 0;
    void __user *argp = (void __user *)arg;
    struct MESSAGE struct_msg;
#ifndef M3D_MULTI_CONTEXT  
    UINT32 newCtxHandle = 0;
    UINT32 newCtxID = 0;
    BOOL ctxChanged = FALSE;    
#endif
    CMQInfoStruct* pCMQInfo;
    M3DContextStruct *pCtx = NULL;
#ifdef M3D_PROFILE
    struct timeval kTime;
#endif
#ifdef M3D_PROC_PROFILE
    struct timeval kTimeBegin;
    do_gettimeofday(&kTimeBegin);
#endif
    DbgPrtTime(kTime,"M3DWorkerEntry: entry...\n");
#ifdef SUPPORT_LOCK_DRIVER
    if (LOCK_DRIVER_OPERATION == condition)
    {
        LockDriverOperation kLockDriverOp;
        if (copy_from_user(&kLockDriverOp, argp, sizeof(kLockDriverOp))){
            ret = -EFAULT;
            goto END;
        }
        pCtx = GetContext(kLockDriverOp.ctxHandle);
        if (kLockDriverOp.bIsLock){
            LockDriverForCtx(pCtx);
        } else {
            UnlockDriverForCtx(pCtx);        
        }
        goto END;
    }
#endif

    if (copy_from_user(&struct_msg, argp, sizeof(struct_msg)))
    {
        ret = -EFAULT;
        goto END;
    }

#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
    if (QUERY_DEBUG_FLAG == condition)
    {
#ifdef M3D_PROFILE
        printk(KERN_INFO"---------------------------------------\n");
#endif
        pCtx = GetContext(struct_msg.hdr.ctxHandle);
        if (!pCtx){
            pCtx = (M3DContextStruct*)file->private_data;
        }
        if (pCtx)
        {
#ifdef M3D_PROC_PROFILE            
            pCtx->kProfileData.SWTime = 0;
            pCtx->kProfileData.HWTime = 0;
            pCtx->kProfileData.InterruptTime = 0;
            pCtx->kProfileData.IOCTLTime = 0;
#endif
            if (pCtx->u4DebugCount > 0)
            {
                if (pCtx->u4DebugCount != (UINT32)-1)
                {
                    printk("[3D] Proc debug frames count down: %d\n", pCtx->u4DebugCount);
                    pCtx->u4DebugCount--;                    
                }                
            }
            else
            {
                pCtx->u4DebugFlag = 0;
            }
            struct_msg.u4DebugFlag = pCtx->u4DebugFlag;
            if (copy_to_user(argp, &struct_msg, sizeof(struct_msg)))
            {
                ret = -EFAULT;
            }
        }
        goto END;
    }
#endif    
#ifdef M3D_PROC_PROFILE
    if (QUERY_PROFILE == condition)
    {
        pCtx = GetContext(struct_msg.hdr.ctxHandle);
        if (!pCtx){
            pCtx = (M3DContextStruct*)file->private_data;
        }        
        if (pCtx)
        {
            struct_msg.kProfileData = pCtx->kProfileData;
            // SW profiling contain HW time, so exclude HW time
            struct_msg.kProfileData.SWTime -= struct_msg.kProfileData.HWTime;            
            if (copy_to_user(argp, &struct_msg, sizeof(struct_msg)))
            {
                ret = -EFAULT;
            }
        }
        goto END;
    }
#endif        

    if (NEW_CREATE_CONTEXT == condition)
    {
        printk("M3DWorkerEntry CreateContext before: (struct_msg.ctxHandle = %x)\n", struct_msg.hdr.ctxHandle);
        pCtx = CreateContext(struct_msg.hdr.ctxHandle);
        if (!pCtx){
            goto END;
        }
        file->private_data = (void*)pCtx;
    }
    else if (NEW_DESTROY_CONTEXT == condition)
    {
        printk("M3DWorkerEntry DestroyContext before: (struct_msg.ctxHandle = %x)\n", struct_msg.hdr.ctxHandle);
        if (!DestroyContext(struct_msg.hdr.ctxHandle))
        {
            printk("M3DWorkerEntry DestroyContext after NOT found context handle\n");
            ASSERT(0);
        }
        file->private_data = NULL;
        goto END;
    }
    
#ifdef M3D_MULTI_CONTEXT    
    pCtx = GetContext(struct_msg.hdr.ctxHandle);
    if (!pCtx){
        pCtx = (M3DContextStruct*)file->private_data;
    }
    if (!pCtx){
        printk("M3DWorkerEntry GetContext after NOT found (pid = %d, struct_msg.ctxHandle = %x)\n", 
            current->pid, struct_msg.hdr.ctxHandle);
        goto END;
    }
#else    
    newCtxHandle = struct_msg.hdr.ctxHandle;
    newCtxID = GetContextID(newCtxHandle);
    if (MAX_M3D_CONTEXT == newCtxID)
    {
        printk("M3DWorkerEntry GetContextID after NOT found context ID: (struct_msg.ctxHandle = %x)\n", struct_msg.hdr.ctxHandle);
        ASSERT(0);
        return 0;	//NOT found context ID
    }

    if (pCtx != g_currentCtx)//kevin debug
    {
        printk("M3DWorkerEntry context change\n");
        ctxChanged = TRUE;
        g_currentCtx = pCtx;
        printk("M3DWorkerEntry (ctxHandle = %x)\n", g_currentCtx->ctxHandle);
    }
#endif

#ifdef _MT6516_GLES_CMQ_	//Kevin
    switch(condition)
    {
    case NEW_CMQ_INIT: 
        CMQInit(pCtx, &(struct_msg.CMQCtrl));
        break;
    
    case NEW_CMQ_DEINIT:
#ifdef SUPPORT_LOCK_DRIVER        
        if (g_pCtxLockDriver && g_pCtxLockDriver == pCtx){
            UnlockDriverForCtx(pCtx);
        }
#endif
#ifdef M3D_MULTI_CONTEXT
        //printk(KERN_ERR"[M3D] NEW_CMQ_DEINIT: %d\n", current->pid);
        ret = CtxQueue_Remove(pCtx);
        if (ret){
            printk(KERN_ERR"[M3D] CtxQueue remove interrupted: %d\n", ret);
            goto END;
        }
#endif
        CMQDeInit(pCtx);
        pCtx->dirtyCondition &= ~NEW_CMQ_DEINIT;
        break;

    case NEW_CMQ_PROCESS:
        if (!pCtx->pCMQMemoryBase){
            printk(KERN_ERR"[M3D] CMQMemoryBase is NULL\n");
            goto END;
        }
#ifndef __M3D_INTERRUPT__
        memcpy(&(pCtx->CMQProcessCtrl), &(struct_msg.CMQProcessCtrl), sizeof(struct CMQProcess));
        M3D_thread();
#else

        //4//We just up semaphore and the worker thread will be waken up
        //4// to process M3D_thread
        pCMQInfo = pCtx->pCMQInfo;
        if (pCMQInfo->readPointer != pCMQInfo->writePointer)
        {
#ifdef SUPPORT_LOCK_DRIVER            
            if ((g_pCtxLockDriver) && (pCtx != g_pCtxLockDriver)){
                //printk(KERN_INFO"[M3D] %p Wait Driver\n", pCtx);
                wait_event_interruptible(gCtxWaitQueueHead, !g_pCtxLockDriver);
                //printk(KERN_INFO"[M3D] %p Get Driver\n", pCtx);
            }
#endif
#ifdef M3D_MULTI_CONTEXT
            if (struct_msg.CMQProcessCtrl.waitIdle){
                init_completion(&pCtx->JobDone);
            }

#ifndef CMD_LIST_BREAK
            if (!CtxQueue_TryToMerge(pCtx))
#endif
            {
                ret = CtxQueue_Push(pCtx);
                if (ret){
                    printk(KERN_ERR"[M3D] CtxQueue push interrupted: %d\n", ret);
                    goto END;
                }
                wake_up_interruptible(&gM3DWaitQueueHead);
            }
#else
            up(&gSemaphore);
#endif
            if (struct_msg.CMQProcessCtrl.waitIdle)
            {
#ifdef M3D_MULTI_CONTEXT                
                //printk("Process Context (%d,%x): ...sleep\n", pCtx->pid, pCtx->ctxHandle);
                DbgPrtTime(kTime,"Process Context (%d,%x): ...sleep\n", 
                    pCtx->pid, pCtx->ctxHandle);

                if (wait_for_completion_interruptible(&pCtx->JobDone)){
                    printk(KERN_ERR"[M3D] Wait for job done interrupted\n");
                }

                DbgPrtTime(kTime,"Process Context (%d,%x): ...wakeup\n", 
                    pCtx->pid, pCtx->ctxHandle);
                //printk("Process Context (%d,%x): ...wakeup\n", pCtx->pid, pCtx->ctxHandle);
#else
                DEFINE_WAIT(wait);
                prepare_to_wait(&gM3DWaitQueueHead, &wait, TASK_INTERRUPTIBLE);
                if (pCMQInfo->readPointer != pCMQInfo->writePointer)
                {
                    DbgPrtTime(kTime,"M3DWorkerEntry: sleep...\n");
                    schedule();
                    DbgPrtTime(kTime,"M3DWorkerEntry: ...wakeup\n");
                }
                finish_wait(&gM3DWaitQueueHead, &wait);
#endif
            }
        }
#endif        
        break;
    }

END:
    IOCTLTimeEnd(pCtx, kTimeBegin);
    DbgPrtTime(kTime,"M3DWorkerEntry: leave...\n");    
    return ret;
#else
// TODO:  Remember UnComment Below Codes
#if 0
    //for no CMQ
	UpdateRenderState(condition, &struct_msg);
    //SetRenderState();

	if ((NEW_DRAW_EVENT == condition) || (NEW_TEXDRAWOES_CTRL == condition))
	{
		if (ctxChanged)	//debug
		{
			g_currentCtx->dirtyCondition = NEW_ALL_CHANGED;
 			ctxChanged = FALSE;
		}

#ifdef _MT6516_GLES_PROFILE_	//Kevin for fps
        do_gettimeofday(&t1);
#endif
		//WAIT_M3D();	//kevin  wait before next list update
		
#ifdef _MT6516_GLES_PROFILE_	//Kevin for fps
        do_gettimeofday(&t2);
        //printk("M3DWorkerEntry ((t2.tv_usec - t1.tv_usec) = %d)=============\n", (t2.tv_usec - t1.tv_usec));
        wait_idle_t1_t2 += (t2.tv_usec - t1.tv_usec);
#endif

		SetRenderState();

		if (NEW_DRAW_EVENT == condition)
		{
            memcpy(&(g_currentCtx->DrawEvent), &(struct_msg.DrawEvent), sizeof(struct DrawEvent));
            NewDrawCommand(&(g_currentCtx->DrawEvent));
			//FlushCacheCtrl(&(g_currentCtx->CacheCtrl));	
		}

		if(NEW_TEXDRAWOES_CTRL == condition)
		{
            memcpy(&(g_currentCtx->TexDrawOES), &(struct_msg.TexDrawOES), sizeof(struct TexDrawOES));
            UpdateTexDrawOES(&(g_currentCtx->TexDrawOES));
			//FlushCacheCtrl(&(g_currentCtx->CacheCtrl));	
		}
	}

	if (NEW_FLUSH_CACHE == condition)
	{
        memcpy(&(g_currentCtx->CacheCtrl), &(struct_msg.CacheCtrl), sizeof(struct CacheCtrl));
        FlushCacheCtrl(&(g_currentCtx->CacheCtrl));
        
#ifdef _MT6516_GLES_PROFILE_	//Kevin for profile
        printk("M3DWorkerEntry (t1_t2 = %d)\n", t1_t2);
        printk("M3DWorkerEntry (flush_t1_t2 = %d)\n", flush_t1_t2);
        printk("M3DWorkerEntry (wait_idle_t1_t2 = %d)\n", wait_idle_t1_t2);
        wait_idle_t1_t2 = 0;
        flush_t1_t2 = 0;
        t1_t2 = 0;
#endif
	}

    //printk("M3DWorkerEntry leave: (condition = %x, arg = %x)=============\n", condition, arg);
    return ret;
#endif //#if 0
#endif 
}
#else
UINT32 M3DWorkerEntry(UINT32 condition, UINT32 arg)
{        
	//Kevin (TODO) destroy context!!

        int ret = 0;
	 void __user *argp = (void __user *)arg;
        struct MESSAGE struct_msg;
        UINT32 newCtxHandle = 0;
        UINT32 newCtxID = 0;
        BOOL ctxChanged = FALSE;
        

    //printk("M3DWorkerEntry enter: (condition = %x, arg = %x)=============\n", condition, arg);

	if (ALLOC_PHYS_MEM == condition)
	{
                dma_addr_t dma_pa;
                void *dma_va;
                UINT32 size = 0;
            	  UINT32 struct_ptr[3] = {0};
            	  if (copy_from_user(struct_ptr, argp, sizeof(struct_ptr)))
                {
                	return -EFAULT;
                }
            	  size = struct_ptr[0];
//printk("M3DWorkerEntry dma_alloc_coherent before: (dma_pa = %x, dma_va = %x, size = %x)=============\n", dma_pa, dma_va, size);
                dma_va = dma_alloc_coherent(0, size, &dma_pa, GFP_KERNEL);
//printk("M3DWorkerEntry dma_alloc_coherent after: (dma_pa = %x, dma_va = %x, size = %x)=============\n", dma_pa, dma_va, size);

                struct_ptr[1] = (UINT32)dma_pa;
                struct_ptr[2] = (UINT32)dma_va;
                ret = (copy_to_user((void*)arg, struct_ptr, sizeof(struct_ptr))) ? -EFAULT : 0;	

                return ret;
	}

	if (FREE_PHYS_MEM == condition)
	{
                dma_addr_t dma_pa;
                void *dma_va;
                UINT32 size = 0;
            	  UINT32 struct_ptr[3] = {0};
            	  if (copy_from_user(struct_ptr, argp, sizeof(struct_ptr)))
                {
                	return -EFAULT;
                }
            	  dma_va = (void*)struct_ptr[0];
            	  dma_pa = (dma_addr_t)struct_ptr[1];
            	  size = struct_ptr[2];
//printk("M3DWorkerEntry dma_free_coherent before: (dma_pa = %x, dma_va = %x, size = %x)=============\n", dma_pa, dma_va, size);
	         dma_free_coherent(0, size, dma_va, dma_pa);            	
//printk("M3DWorkerEntry dma_free_coherent after: (dma_pa = %x, dma_va = %x, size = %x)=============\n", dma_pa, dma_va, size);

		  return 0;
	}

	if (CHECK_APP_NAME == condition)
       {
              UINT32 size = 0;
		CheckAppNameStruct	CheckAppNameCtrl = {0};
 		struct task_struct *p = current;
            	  
printk("M3DWorkerEntry CHECK_APP_NAME=============\n");

            	  if (copy_from_user(&CheckAppNameCtrl, argp, sizeof(CheckAppNameCtrl)))
                {
                	return -EFAULT;
                }
            	  size = CheckAppNameCtrl.size;

printk("=====M3DWorkerEntry current process name is %s=================\n", p->comm);
printk("=====M3DWorkerEntry current process pid is %d===================\n", p->pid);

		memcpy(CheckAppNameCtrl.name, p->comm, sizeof(CheckAppNameCtrl.name));
              ret = (copy_to_user((void*)arg, &CheckAppNameCtrl, sizeof(CheckAppNameCtrl))) ? -EFAULT : 0;	

              return ret;
       }

	if (CHECK_CREATE_CONTEXT == condition)
       {
		CanCreateContextStruct	CanCreateContextCtrl = {0};

printk("M3DWorkerEntry CHECK_CREATE_CONTEXT=============\n");
            	  
            	  if (copy_from_user(&CanCreateContextCtrl, argp, sizeof(CanCreateContextCtrl)))
                {
                	return -EFAULT;
                }

            	  CanCreateContextCtrl.result = CanCreateContext();
            	  
printk("M3DWorkerEntry CanCreateContextCtrl.result  = %x=============\n", CanCreateContextCtrl.result );

              ret = (copy_to_user((void*)arg, &CanCreateContextCtrl, sizeof(CanCreateContextCtrl))) ? -EFAULT : 0;	

              return ret;
       }

	if (copy_from_user(&struct_msg, argp, sizeof(struct_msg)))
	{
		return -EFAULT;
	}

#if 0
	if (NEW_POWER_HANDLE_CTRL == condition)
       {
printk("M3DWorkerEntry UpdatePowerCtrl before: (struct_msg.PowerEventCtrl.PowerOnFlag = %x)=============\n", struct_msg.PowerEventCtrl.PowerOnFlag);
               UpdatePowerCtrl((PowerEventCtrlStruct*)&(struct_msg.PowerEventCtrl));

               return 0;
       }
#endif

	if (NEW_CREATE_CONTEXT == condition)
       {
printk("M3DWorkerEntry CreateContext before: (struct_msg.ctxHandle = %x)=============\n", struct_msg.ctxHandle);
		 if (!CreateContext(struct_msg.ctxHandle))
	 	{
printk("M3DWorkerEntry CreateContext after out of context count: (ctxIDCount = %x)=============\n", g_ctxIDCount);	 		
			ASSERT(0);
	 	}
               return 0;
       }

	if (NEW_DESTROY_CONTEXT == condition)
       {
printk("M3DWorkerEntry DestroyContext before: (struct_msg.ctxHandle = %x)=============\n", struct_msg.ctxHandle);
		 if (!DestroyContext(struct_msg.ctxHandle))
	 	{
printk("M3DWorkerEntry DestroyContext after NOT found context handle=============\n");	 		
			ASSERT(0);
	 	}
		 
		 g_currentCtxID = MAX_M3D_CONTEXT;	//reset
		 
               return 0;
       }

	newCtxHandle = struct_msg.ctxHandle;
       //printk("\n----MT6516 M3D: M3DWorkerEntry: (newCtxHandle = %x)----\n", newCtxHandle);            

	newCtxID = GetContextID(newCtxHandle);
	if (MAX_M3D_CONTEXT == newCtxID)
	{
printk("M3DWorkerEntry GetContextID after NOT found context ID: (struct_msg.ctxHandle = %x)=============\n", struct_msg.ctxHandle);
		ASSERT(0);
		return 0;	//NOT found context ID
	}

	if (newCtxID != g_currentCtxID)	//kevin debug
	{
printk("M3DWorkerEntry context change=============\n");
		ctxChanged = TRUE;
		g_currentCtxID = newCtxID;
		g_currentCtx = &(g_M3DContext[g_currentCtxID]);
printk("M3DWorkerEntry new g_currentCtxID  = %x: (ctxHandle = %x)=============\n", g_currentCtxID, g_currentCtx->ctxHandle);
	}


#ifdef _MT6516_GLES_CMQ_	//Kevin
	if (NEW_CMQ_INIT == condition)
       {
printk("M3DWorkerEntry NEW_CMQ_INIT=============\n");
        	memcpy(&(g_currentCtx->CMQCtrl), &(struct_msg.CMQCtrl), sizeof(struct CMQ));
        	
		CMQInit();
		//kthread_run(m3d_thread_kthread, NULL, "m3d_thread_kthread"); 
		
               return 0;
       }
	if (NEW_CMQ_DEINIT == condition)
       {
printk("M3DWorkerEntry NEW_CMQ_DEINIT=============\n");
		CMQDeInit();
		//m3d_leave_thread = TRUE;
		g_currentCtx->dirtyCondition &= ~NEW_CMQ_DEINIT;
		
               return 0;
       }
	if (NEW_CMQ_PROCESS == condition)
       {
//printk("M3DWorkerEntry NEW_CMQ_PROCESS=============\n");
		memcpy(&(g_currentCtx->CMQProcessCtrl), &(struct_msg.CMQProcessCtrl), sizeof(struct CMQProcess));

              M3D_thread();
		
               return 0;
       }
#endif

#if 1	//for no CMQ
#ifndef _MT6516_GLES_CMQ_
	UpdateRenderState(condition, &struct_msg);

		//SetRenderState();

	if ((NEW_DRAW_EVENT == condition) || (NEW_TEXDRAWOES_CTRL == condition))
	{
#if 1
		if (ctxChanged)	//debug
		{
			g_currentCtx->dirtyCondition = NEW_ALL_CHANGED;
 			ctxChanged = FALSE;
		}
#endif

#ifdef _MT6516_GLES_PROFILE_	//Kevin for fps
       do_gettimeofday(&t1);
#endif
		//WAIT_M3D();	//kevin  wait before next list update
#ifdef _MT6516_GLES_PROFILE_	//Kevin for fps
       do_gettimeofday(&t2);
       //printk("M3DWorkerEntry ((t2.tv_usec - t1.tv_usec) = %d)=============\n", (t2.tv_usec - t1.tv_usec));
       wait_idle_t1_t2 += (t2.tv_usec - t1.tv_usec);
#endif

		SetRenderState();

		if (NEW_DRAW_EVENT == condition)
		{
        	  memcpy(&(g_currentCtx->DrawEvent), &(struct_msg.DrawEvent), sizeof(struct DrawEvent));
              	NewDrawCommand(&(g_currentCtx->DrawEvent));
			//FlushCacheCtrl(&(g_currentCtx->CacheCtrl));	
		}

		if(NEW_TEXDRAWOES_CTRL == condition)
		{
        	  memcpy(&(g_currentCtx->TexDrawOES), &(struct_msg.TexDrawOES), sizeof(struct TexDrawOES));
              	UpdateTexDrawOES(&(g_currentCtx->TexDrawOES));
			//FlushCacheCtrl(&(g_currentCtx->CacheCtrl));	
		}
	}

	if (NEW_FLUSH_CACHE == condition)
	{
    	  memcpy(&(g_currentCtx->CacheCtrl), &(struct_msg.CacheCtrl), sizeof(struct CacheCtrl));
    	  	FlushCacheCtrl(&(g_currentCtx->CacheCtrl));
#ifdef _MT6516_GLES_PROFILE_	//Kevin for profile
    printk("M3DWorkerEntry (t1_t2 = %d)=============\n", t1_t2);
    printk("M3DWorkerEntry (flush_t1_t2 = %d)=============\n", flush_t1_t2);
    printk("M3DWorkerEntry (wait_idle_t1_t2 = %d)=============\n", wait_idle_t1_t2);
    wait_idle_t1_t2 = 0;
    flush_t1_t2 = 0;
    t1_t2 = 0;
#endif
	}

#endif
#endif
    //printk("M3DWorkerEntry leave: (condition = %x, arg = %x)=============\n", condition, arg);
    return ret;
}
#endif
static void SwapDrawSurfacePhy(
    SwapDrawSurfacePhyStruct *pSwap)
{
    WAIT_M3D();

    M3D_WriteReg32NoBackup(M3D_CACHE_INVAL_CTRL, 
        M3D_STECIL_CACHE_INVAL | 
        M3D_DEPTH_CACHE_INVAL  | 
        M3D_COLOR_CACHE_INVAL);
    
    M3D_WriteReg32(M3D_COLOR_BUFFER, pSwap->ColorBase);
}

static void UpdateContext(
    M3DContextStruct *pCtx,
    NewContextStruct *pContext)
{
    UINT32 i;
    UINT32 colorFmt = pContext->ColorFormat;
    volatile unsigned int dummy_counter = 0;

    pCtx->NewContext.BufferWidth  = pContext->BufferWidth;
    pCtx->NewContext.BufferHeight = pContext->BufferHeight;
    pCtx->NewContext.GLESProfile = pContext->GLESProfile;

    if (!pCtx->bHWRegDataValid)
    {
        M3D_WriteReg32NoBackup(M3D_RESET, 1);
        while (dummy_counter++ <= 0xFFF) {};
        M3D_WriteReg32NoBackup(M3D_RESET, 0);

        /// invalidate fragment cache here....
        M3D_WriteReg32NoBackup(M3D_CACHE_INVAL_CTRL, 
            M3D_STECIL_CACHE_INVAL | 
            M3D_DEPTH_CACHE_INVAL  | 
            M3D_COLOR_CACHE_INVAL);
        
        ResetHWReg();
        /// initial the H/W registers
        M3D_WriteReg32(M3D_TEX_COORD(0), 0x22);
        M3D_WriteReg32(M3D_TEX_COORD(1), 0x22);
        M3D_WriteReg32(M3D_TEX_COORD(2), 0x22);

        M3D_WriteReg32(M3D_COLOR_1, M3D_DISABLE);
        M3D_WriteReg32(M3D_CULL, 0xFFFF);
        M3D_WriteReg32(M3D_SHADE_MODEL, 0x1);
        M3D_WriteReg32(M3D_CLIP_PLANE_ENABLE, 0x0);
        M3D_WriteReg32(M3D_NORMAL_SCALE_ENABLE, 0x0);
        
        M3D_WriteReg32(M3D_LIGHT_CTRL, 0x0);
        M3D_WriteReg32(M3D_PRIMITIVE_AA, 0x0);
        M3D_WriteReg32(M3D_POLYGON_OFFSET_ENABLE, 0x0);
        M3D_WriteReg32(M3D_FILL_MODE, 0x2);
        M3D_WriteReg32(M3D_TEX_CTRL, 0x07E00000);

        for (i = 0; i < MAX_TEXTURE_UNITS; i++)
        {
            M3D_WriteReg32(M3D_LOD_BIAS(i), 0x0);
            M3D_WriteReg32(
                M3D_TEX_OP(i), (M3D_TEX_OP_MODULATE << M3D_TEX_OP_RGB_OFFSET) |
                (M3D_TEX_OP_MODULATE << M3D_TEX_OP_A_OFFSET));
            M3D_WriteReg32(
                M3D_TEX_SRC_OPD(i), (M3D_TEX_SRC_TEXTURE << M3D_TEX_SRC1_RGB_OFFSET) |
                (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC2_RGB_OFFSET) |
                (M3D_TEX_SRC_CONSTANT << M3D_TEX_SRC0_RGB_OFFSET) |
                (M3D_TEX_SRC_TEXTURE << M3D_TEX_SRC1_A_OFFSET) |
                (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC2_A_OFFSET) |
                (M3D_TEX_SRC_CONSTANT << M3D_TEX_SRC0_A_OFFSET) |
                (M3D_TEX_OPD_SRC_COLOR << M3D_TEX_OPD1_RGB_OFFSET) |
                (M3D_TEX_OPD_SRC_COLOR << M3D_TEX_OPD2_RGB_OFFSET) |
                (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD0_RGB_OFFSET) |
                (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD1_A_OFFSET) |
                (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD2_A_OFFSET) |
                (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD0_A_OFFSET));
        }
        BackupHWContext(pCtx);
        pCtx->bHWRegDataValid = TRUE;
    }
    
    if (colorFmt == COLOR_FORMAT_RGB565) {
        M3D_WriteReg32(M3D_COLOR_FORMAT, M3D_COLOR_RGB565);
    } else if (colorFmt == COLOR_FORMAT_RGB888) {
        M3D_WriteReg32(M3D_COLOR_FORMAT, M3D_COLOR_RGB888);
    } else if (colorFmt == COLOR_FORMAT_ARGB888) {
        M3D_WriteReg32(M3D_COLOR_FORMAT, M3D_COLOR_ARGB8888);
    } else {
        ASSERT(0);
    }

    M3D_WriteReg32(M3D_COLOR_BUFFER, pContext->ColorBase);
    M3D_WriteReg32(M3D_BUFFER_WIDTH, pContext->BufferWidth);
    M3D_WriteReg32(M3D_BUFFER_HEIGHT, pContext->BufferHeight);
    M3D_WriteReg32(M3D_DEPTH_BUFFER, pContext->DepthBase);
    M3D_WriteReg32(M3D_STENCIL_BUFFER, pContext->StencilBase);

    //printk("pContext->ColorBase = %x\n", pContext->ColorBase);
    //printk("M3D_ReadSWReg32(M3D_COLOR_BUFFER) = %x\n", M3D_ReadSWReg32(M3D_COLOR_BUFFER));

    dummy_counter = 0;
    while (dummy_counter++ <= 0xFF) {};
}

#if defined (__USING_HWDRAWTEX_CLEAR__)
//#define _MT6516_GLES_PROFILE2_
static void InvalCacheCtrl(
    M3DContextStruct *pCtx,
    CacheCtrlStruct *pCacheCtrl)
{
#ifdef M3D_PROFILE
    struct timeval kTime;
#endif    
    UINT32 Temp;
    UINT32 mask = pCacheCtrl->mask;

    // TODO: ? WinMo Reference : 
    //4 (1) Critical Section
    //4 (2) Check Power Off

#ifndef __M3D_INTERRUPT__
    UINT32 u4RegLightCtrl     = M3D_ReadReg32(M3D_LIGHT_CTRL);
#else
    UINT32 u4RegDepthTest     = M3D_ReadReg32(M3D_DEPTH_TEST);
    UINT32 u4RegBlendTest     = M3D_ReadReg32(M3D_BLEND);
    UINT32 u4RegTexCtrl       = M3D_ReadReg32(M3D_TEX_CTRL);
    UINT32 u4RegLightCtrl     = M3D_ReadReg32(M3D_LIGHT_CTRL);
    UINT32 u4RegLogicTest     = M3D_ReadReg32(M3D_LOGIC_TEST);
    UINT32 u4RegStencilTest   = M3D_ReadReg32(M3D_STENCIL_TEST);
    UINT32 u4RegAlphaTest     = M3D_ReadReg32(M3D_ALPHA_TEST);
    UINT32 u4RegScissorEnable = M3D_ReadReg32(M3D_SCISSOR_ENABLE);

    UINT32 u4RegDrawTexXY     = M3D_ReadReg32(M3D_DRAWTEX_XY);
    UINT32 u4RegDrawTexWH     = M3D_ReadReg32(M3D_DRAWTEX_WH);
    //UINT32 u4RegDrawTexZ      = M3D_ReadReg32(M3D_DRAWTEX_Z);
    UINT32 u4RegTexImg0       = M3D_ReadReg32(M3D_TEX_IMG(0));
    UINT32 u4RegTexImgPtr0    = M3D_ReadReg32(M3D_TEX_IMG_PTR_0_0);
    UINT32 u4RegTexPara0      = M3D_ReadReg32(M3D_TEX_PARA_0);
    UINT32 u4TexOP0           = M3D_ReadReg32(M3D_TEX_OP_0);
    UINT32 u4TexSrcOPD0       = M3D_ReadReg32(M3D_TEX_SRC_OPD_0);
    UINT32 u4FragCacheInvalid = 0;
    UINT32 _width = 1 << ((u4RegTexImg0 >> 6) & 0x000F);
    UINT32 _height = 1 << ((u4RegTexImg0 >> 10) & 0x000F);
    u4RegTexImg0 = (u4RegTexImg0 & 0x003f) | (_width << 6) | (_height << 16);
#endif
    
    if (mask & GL_COLOR_BUFFER_BIT) 
    {
        u4FragCacheInvalid |= M3D_COLOR_CACHE_FLUSH;
    }
    if (mask & GL_DEPTH_BUFFER_BIT) 
    {
        u4FragCacheInvalid |= M3D_DEPTH_CACHE_FLUSH;
    }
    if (mask & GL_STENCIL_BUFFER_BIT) 
    {
        u4FragCacheInvalid |= M3D_STECIL_CACHE_FLUSH;
    }
    M3D_WriteReg32NoBackup(M3D_CACHE_INVAL_CTRL, u4FragCacheInvalid); 
             
    if ((mask & GL_COLOR_BUFFER_BIT) || (mask & GL_DEPTH_BUFFER_BIT))
    { 
        GLnative *pClearColor = pCacheCtrl->ClearColor;
        INT32  i_x = pCacheCtrl->x;
        INT32  i_y = pCacheCtrl->y;  
        UINT32 i_w = pCacheCtrl->w;
        UINT32 i_h = pCacheCtrl->h;
        
        ///  set the clear region
        M3D_WriteReg32(M3D_DRAWTEX_XY, i_x | (i_y << 10));
        M3D_WriteReg32(M3D_DRAWTEX_WH, i_w | (i_h << 10));
        
        /// set the depth clear value
        M3D_Native_WriteReg32(M3D_DRAWTEX_Z, (INT32)pCacheCtrl->ClearZValue);      
                
        // 0x07E00011
        M3D_WriteReg32(M3D_TEX_CTRL, 0x07E00011);
        /*
        M3D_WriteReg32(M3D_TEX_CTRL,         
            M3D_TEX_ENABLE_0        |
            M3D_TEX_STAGE_EN_0      |
            M3D_TEX_THRESHOLD_EN_0	|
            M3D_TEX_THRESHOLD_EN_1	|
            M3D_TEX_THRESHOLD_EN_2	|
            M3D_TEX_LOD_HALF_EN_0	|
            M3D_TEX_LOD_HALF_EN_1	|
            M3D_TEX_LOD_HALF_EN_2);
        */

        // 0x20081 => 2x2 OpenGLES R5G6B5
        M3D_WriteReg32(M3D_TEX_IMG(0), 0x20081);
        /*
        M3D_WriteReg32(M3D_TEX_IMG(0), 
            (M3D_TEX_FMT_R5G6B5 << M3D_TEX_IMG_FORMAT_OFFSET)           |
            (M3D_TEX_BYTE_ORDER_OPENGLES << M3D_TEX_BYTE_ORDER_OFFSET)  |
            (2 << M3D_TEX_IMG_WIDTH_OFFSET)                             |
            (2 << M3D_TEX_IMG_HEIGHT_OFFSET));
        */

        // Set texture image pointer
        M3D_WriteReg32(M3D_TEX_IMG_PTR_0_0, (GLuint)g_TextureBufferPA);
        
        M3D_WriteReg32(M3D_TEX_PARA_0, M3D_TEX_NEAREST);
                                            
        if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
        {
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_R_0, *pClearColor++);
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_G_0, *pClearColor++);
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_B_0, *pClearColor++);
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_A_0, *pClearColor++);

            M3D_WriteReg32(M3D_DRAWTEX_CRU_0, 0);
            M3D_WriteReg32(M3D_DRAWTEX_CRV_0, 0);
            M3D_WriteReg32(M3D_DRAWTEX_DCRU_0, 0);
            M3D_WriteReg32(M3D_DRAWTEX_DCRV_0, 0);
        }
        else
        {
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_R_0, *pClearColor++);
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_G_0, *pClearColor++);
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_B_0, *pClearColor++);
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_A_0, *pClearColor++);
            
            M3D_FLT_WriteReg32(M3D_DRAWTEX_CRU_0, 0);
            M3D_FLT_WriteReg32(M3D_DRAWTEX_CRV_0, 0);
            M3D_FLT_WriteReg32(M3D_DRAWTEX_DCRU_0, 0);
            M3D_FLT_WriteReg32(M3D_DRAWTEX_DCRV_0, 0);
        }
        
        M3D_WriteReg32(M3D_TEX_OP_0, 0x00);
        M3D_WriteReg32(M3D_TEX_SRC_OPD_0, 0x1008);
        
        mask &= ~GL_STENCIL_BUFFER_BIT;
        switch(mask)
        {
        case (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT):
            M3D_WriteReg32(M3D_DEPTH_TEST, 0x1F);
            M3D_WriteReg32(M3D_BLEND, 0x1E00);
            break;

        case GL_COLOR_BUFFER_BIT:
            M3D_WriteReg32(M3D_DEPTH_TEST, M3D_DISABLE_VALUE);
            M3D_WriteReg32(M3D_BLEND, 0x1E00);
            break;

        case GL_DEPTH_BUFFER_BIT:
            M3D_WriteReg32(M3D_DEPTH_TEST, 0x1F);
            M3D_WriteReg32(M3D_BLEND, 0x0);
            break;

        default:
            printk(KERN_ERR"[M3D]H/W Clear, invalid mask value\n");
            ASSERT(0);
            break;
        }

        
        // Clean fog enable bit and fog mode bit
        Temp = u4RegLightCtrl & 0x230007FF;
        M3D_WriteReg32(M3D_LIGHT_CTRL, Temp);

        // Disable logic test
        M3D_WriteReg32(M3D_LOGIC_TEST, M3D_DISABLE_VALUE);

        // Disable stencil test
        M3D_WriteReg32(M3D_STENCIL_TEST, M3D_DISABLE_VALUE);

        // Disable alpha test
        M3D_WriteReg32(M3D_ALPHA_TEST, M3D_DISABLE_VALUE);
              
        // Disable scissor test
        M3D_WriteReg32(M3D_SCISSOR_ENABLE, M3D_DISABLE_VALUE);
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        if(!CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){
#endif
#ifndef __M3D_INTERRUPT__          
        // Trigger, using draw texture to clear color buffer and depth buffer
        M3D_WriteReg32(M3D_TRIGGER, M3D_DRAWTEX_ENABLE | M3D_TRIGGER_BIT);
#else
        init_completion(&gCompRenderDone);
        M3D_WriteReg32NoBackup(M3D_TRIGGER, M3D_DRAWTEX_ENABLE | M3D_TRIGGER_BIT);
        DbgPrt2Time(kTime,"InvalCacheCtrl: wait hw render done\n");
        HWTimeBegin(pCtx);
#ifndef M3D_HW_DEBUG_HANG
        wait_for_completion(&gCompRenderDone);
#else
        if(!wait_for_completion_timeout(&gCompRenderDone, 5000)){
            // timeout
            printk(KERN_ERR"InvalCacheCtrl HW Hang\n");
            M3D_HWReg_Dump(pCtx);
        }
#endif
        HWTimeEnd(pCtx);
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        }
#endif
        DbgPrt2Time(kTime,"InvalCacheCtrl: hw render done\n");

        /// restore the gles states
        M3D_WriteReg32(M3D_DEPTH_TEST, u4RegDepthTest);
        M3D_WriteReg32(M3D_BLEND, u4RegBlendTest);
        M3D_WriteReg32(M3D_TEX_CTRL, u4RegTexCtrl);
        M3D_WriteReg32(M3D_LIGHT_CTRL, u4RegLightCtrl);
        M3D_WriteReg32(M3D_LOGIC_TEST, u4RegLogicTest);
        M3D_WriteReg32(M3D_STENCIL_TEST, u4RegStencilTest);
        M3D_WriteReg32(M3D_ALPHA_TEST, u4RegAlphaTest);
        M3D_WriteReg32(M3D_SCISSOR_ENABLE, u4RegScissorEnable);

        M3D_WriteReg32(M3D_DRAWTEX_XY, u4RegDrawTexXY);
        M3D_WriteReg32(M3D_DRAWTEX_WH, u4RegDrawTexWH);
        //M3D_WriteReg32(M3D_DRAWTEX_Z, u4RegDrawTexZ);
        M3D_WriteReg32(M3D_TEX_IMG(0), u4RegTexImg0);
        M3D_WriteReg32(M3D_TEX_IMG_PTR_0_0, u4RegTexImgPtr0);
        M3D_WriteReg32(M3D_TEX_PARA_0, u4RegTexPara0);
        M3D_WriteReg32(M3D_TEX_OP_0, u4TexOP0);
        M3D_WriteReg32(M3D_TEX_SRC_OPD_0, u4TexSrcOPD0);
#endif
    }
}

#else

void InvalCacheCtrl(CacheCtrlStruct *pCacheCtrl)
{
    UINT32   mask = pCacheCtrl->mask;
    UINT32   m3dFragCacheInvalid = 0;            
         
    if (mask & GL_COLOR_BUFFER_BIT) 
    {
       m3dFragCacheInvalid |= M3D_COLOR_CACHE_FLUSH;
    }
    
    if (mask & GL_DEPTH_BUFFER_BIT) 
    {
       m3dFragCacheInvalid |= M3D_DEPTH_CACHE_FLUSH;
    }

    if (mask & GL_STENCIL_BUFFER_BIT) 
    {
       m3dFragCacheInvalid |= M3D_STECIL_CACHE_FLUSH;
    }
             
    M3D_WriteReg32(M3D_CACHE_INVAL_CTRL, m3dFragCacheInvalid); 

#if 0    //Kevin            
   SetEvent(m_hM3DGLESIPCInfo.IdleEventPool[m_CurrentClientIndex]);         
   SetEvent(m_hEventForMultiUser);
   m_hM3DGLESIPCInfo.CurrentIndexCounter[m_CurrentClientIndex]++;     
#endif      
}
#endif

#ifdef __M3D_INTERRUPT__
static void FlushCacheCtrl(CacheCtrlStruct *pCacheCtrl)
{
    if (M3D_ReadReg32(M3D_DEPTH_BUFFER) != 0)
    {
        M3D_WriteReg32(M3D_CACHE_FLUSH_CTRL, M3D_DEPTH_CACHE_FLUSH);
    }
    if (M3D_ReadReg32(M3D_COLOR_BUFFER) != 0)
    {
        M3D_WriteReg32(M3D_CACHE_FLUSH_CTRL, M3D_COLOR_CACHE_FLUSH);
    }
    WAIT_M3D();
}
#else
void FlushCacheCtrl(CacheCtrlStruct *pCacheCtrl)
{        
    WAIT_M3D();	//kevin if NOT wait idle per list , we must wait idle before flush, else "hang"
#if 1	//Kevin
    if (M3D_ReadReg32(M3D_DEPTH_BUFFER) != 0)
    {
        M3D_WriteReg32(M3D_CACHE_FLUSH_CTRL, M3D_DEPTH_CACHE_FLUSH);
    }
    if (M3D_ReadReg32(M3D_COLOR_BUFFER) != 0)
    {
        M3D_WriteReg32(M3D_CACHE_FLUSH_CTRL, M3D_COLOR_CACHE_FLUSH);
    }
#else
    M3D_WriteReg32(M3D_CACHE_FLUSH_CTRL, (M3D_STECIL_CACHE_FLUSH |
                   M3D_DEPTH_CACHE_FLUSH |
                   M3D_COLOR_CACHE_FLUSH));
#endif
    WAIT_M3D();        

#ifdef _MT6516_GLES_PROFILE_	//Kevin for profile
    printk("M3DWorkerEntry (t1_t2 = %d)=============\n", t1_t2);
    printk("M3DWorkerEntry (flush_t1_t2 = %d)=============\n", flush_t1_t2);
    flush_t1_t2 = 0;
    t1_t2 = 0;
#endif

#if 0    //Kevin    
    SetEvent(m_hM3DGLESIPCInfo.IdleEventPool[m_CurrentClientIndex]);           
    SetEvent(m_hEventForMultiUser);               
    m_hM3DGLESIPCInfo.CurrentIndexCounter[m_CurrentClientIndex]++;
#endif    
}
#endif

static void UpdateBBVPTransformMatrix(
    M3DContextStruct            *pCtx,
    BBViewportTransformStruct   *pBBViewPortXformCtrl)
{
    int i;
    UINT32 u4BufferWidth = pCtx->NewContext.BufferWidth;
    UINT32 u4BufferHeight = pCtx->NewContext.BufferHeight;    
    INT32 LowerLeftX;
    INT32 LowerLeftY;
    INT32 UpperRightX;
    INT32 UpperRightY;
        
    BBViewportTransformStruct *pBBViewPortXform = pBBViewPortXformCtrl;

    UINT32 ViewPortW = pBBViewPortXformCtrl->ViewportW;
    UINT32 ViewPortH = pBBViewPortXformCtrl->ViewportH;
    INT32  ViewportX = pBBViewPortXformCtrl->ViewportX;
    INT32  ViewportY = pBBViewPortXformCtrl->ViewportY;
    
    UINT32 ScissorW = pBBViewPortXformCtrl->ScissorW;
    UINT32 ScissorH = pBBViewPortXformCtrl->ScissorH;
    INT32  ScissorX = pBBViewPortXformCtrl->ScissorX;
    INT32  ScissorY = pBBViewPortXformCtrl->ScissorY;
    
    GLnative lNear = pBBViewPortXformCtrl->Near;    
    GLnative lFar  = pBBViewPortXformCtrl->Far;
    
    if ((u4BufferWidth  != ViewPortW) ||
        (u4BufferHeight != ViewPortH))
    {
        M3D_WriteReg32(M3D_BBOX_EXPAND, 0);
    } else {
        M3D_WriteReg32(M3D_BBOX_EXPAND, 0x3);
    }


    LowerLeftX  = (ViewportX > 0)? ViewportX: 0;
    LowerLeftY  = (ViewportY > 0)? ViewportY: 0;

    UpperRightX = (u4BufferWidth  > ViewportX + ViewPortW)?
                  (ViewportX + ViewPortW): u4BufferWidth;

    UpperRightY = (u4BufferHeight > ViewportY + ViewPortH)?
                  (ViewportY + ViewPortH): u4BufferHeight;

    if (pBBViewPortXformCtrl->ScissorTest) {
        LowerLeftX  = (LowerLeftX > ScissorX) ? LowerLeftX: ScissorX;
        LowerLeftY  = (LowerLeftY > ScissorY) ? LowerLeftY: ScissorY;

        UpperRightX = (UpperRightX > (ScissorX + (INT32)ScissorW))?
                      (ScissorX + (INT32)ScissorW): UpperRightX;
                        
        UpperRightY = (UpperRightY > (ScissorY + (INT32)ScissorH))?
                      (ScissorY + (INT32)ScissorH): UpperRightY;

        M3D_WriteReg32(M3D_VB_LOWER_LEFT_X,  _GL_X_SUB(_GL_I_2_X(LowerLeftX), _GL_X_HALF));
        M3D_WriteReg32(M3D_VB_LOWER_LEFT_Y,  _GL_X_SUB(_GL_I_2_X(LowerLeftY), _GL_X_HALF));
        M3D_WriteReg32(M3D_VB_UPPER_RIGHT_X, _GL_X_SUB(_GL_I_2_X(UpperRightX), _GL_X_HALF));
        M3D_WriteReg32(M3D_VB_UPPER_RIGHT_Y, _GL_X_SUB(_GL_I_2_X(UpperRightY), _GL_X_HALF));

        M3D_WriteReg32(M3D_SCISSOR_ENABLE, M3D_ENABLE_VALUE);
        M3D_WriteReg32(M3D_SCISSOR_LEFT,   ScissorX);
        M3D_WriteReg32(M3D_SCISSOR_BOTTOM, ScissorY);
        M3D_WriteReg32(M3D_SCISSOR_RIGHT,  ScissorX + ScissorW);
        M3D_WriteReg32(M3D_SCISSOR_TOP,    ScissorY + ScissorH);

    } else {
        M3D_WriteReg32(M3D_VB_LOWER_LEFT_X,  _GL_X_SUB(_GL_I_2_X(LowerLeftX),  _GL_X_HALF));
        M3D_WriteReg32(M3D_VB_LOWER_LEFT_Y,  _GL_X_SUB(_GL_I_2_X(LowerLeftY),  _GL_X_HALF));
        M3D_WriteReg32(M3D_VB_UPPER_RIGHT_X, _GL_X_SUB(_GL_I_2_X(UpperRightX), _GL_X_HALF));
        M3D_WriteReg32(M3D_VB_UPPER_RIGHT_Y, _GL_X_SUB(_GL_I_2_X(UpperRightY), _GL_X_HALF));

        M3D_WriteReg32(M3D_SCISSOR_ENABLE, M3D_DISABLE_VALUE);
    }


    if (lNear > lFar)
    {
        M3D_Native_WriteReg32(M3D_VIEWPORT_NEAR, lFar);
        M3D_Native_WriteReg32(M3D_VIEWPORT_FAR, lNear);
    } else {
        M3D_Native_WriteReg32(M3D_VIEWPORT_NEAR, lNear);
        M3D_Native_WriteReg32(M3D_VIEWPORT_FAR, lFar);
    }


    if (g_GLESProfile == __COMMON_LITE_PROFILE__)
    {
        for (i = 0; i < 16; i++)
        {
            M3D_WriteReg32(M3D_MODEL_VIEW(i), pBBViewPortXformCtrl->Element[i]);
            M3D_WriteReg32(M3D_PV(i), pBBViewPortXform->Value[i]);
        }
        M3D_WriteReg32(M3D_NORMAL_N(0), pBBViewPortXform->Inverse[0]);
        M3D_WriteReg32(M3D_NORMAL_N(1), pBBViewPortXform->Inverse[1]);
        M3D_WriteReg32(M3D_NORMAL_N(2), pBBViewPortXform->Inverse[2]);
        M3D_WriteReg32(M3D_NORMAL_N(3), pBBViewPortXform->Inverse[4]);
        M3D_WriteReg32(M3D_NORMAL_N(4), pBBViewPortXform->Inverse[5]);
        M3D_WriteReg32(M3D_NORMAL_N(5), pBBViewPortXform->Inverse[6]);
        M3D_WriteReg32(M3D_NORMAL_N(6), pBBViewPortXform->Inverse[8]);
        M3D_WriteReg32(M3D_NORMAL_N(7), pBBViewPortXform->Inverse[9]);
        M3D_WriteReg32(M3D_NORMAL_N(8), pBBViewPortXform->Inverse[10]);
    }
    else
    {
        for (i = 0; i < 16; i++)
        {
            M3D_FLT_WriteReg32(M3D_MODEL_VIEW(i), pBBViewPortXformCtrl->Element[i]);
            M3D_FLT_WriteReg32(M3D_PV(i), pBBViewPortXform->Value[i]);
        }
        M3D_FLT_WriteReg32(M3D_NORMAL_N(0), pBBViewPortXform->Inverse[0]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(1), pBBViewPortXform->Inverse[1]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(2), pBBViewPortXform->Inverse[2]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(3), pBBViewPortXform->Inverse[4]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(4), pBBViewPortXform->Inverse[5]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(5), pBBViewPortXform->Inverse[6]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(6), pBBViewPortXform->Inverse[8]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(7), pBBViewPortXform->Inverse[9]);
        M3D_FLT_WriteReg32(M3D_NORMAL_N(8), pBBViewPortXform->Inverse[10]);
    }
}


#if 1 //Kevin
static void UpdateTexMatrix(TexMatrixStruct *pMatrix)
{
    UINT32 i = 0;
    struct TexMatrixCtrlUnit *pTexMatrixCtrlInfo;
    UINT32 unit = 0;
    GLuint m3d_tex_coord = 0;

    for (i = 0; i < MAX_TEXTURE_UNITS; i++)
    {
        pTexMatrixCtrlInfo = &pMatrix->TexMatrixCtrlInfo[i];        
        unit = pTexMatrixCtrlInfo->Unit;
        m3d_tex_coord = M3D_ReadReg32(M3D_TEX_COORD(unit));
    
        if (pTexMatrixCtrlInfo->Enable)
        {
           m3d_tex_coord |= (3 << 4);
           M3D_WriteReg32(M3D_TEX_COORD(unit), m3d_tex_coord);
    
            if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
            {
                M3D_WriteReg32(M3D_TEX_M(unit,  0), pTexMatrixCtrlInfo->Value[ 0]);
                M3D_WriteReg32(M3D_TEX_M(unit,  1), pTexMatrixCtrlInfo->Value[ 1]);
                M3D_WriteReg32(M3D_TEX_M(unit,  2), pTexMatrixCtrlInfo->Value[ 3]);
                M3D_WriteReg32(M3D_TEX_M(unit,  3), pTexMatrixCtrlInfo->Value[ 4]);
                M3D_WriteReg32(M3D_TEX_M(unit,  4), pTexMatrixCtrlInfo->Value[ 5]);
                M3D_WriteReg32(M3D_TEX_M(unit,  5), pTexMatrixCtrlInfo->Value[ 7]);
                M3D_WriteReg32(M3D_TEX_M(unit,  6), pTexMatrixCtrlInfo->Value[ 8]);
                M3D_WriteReg32(M3D_TEX_M(unit,  7), pTexMatrixCtrlInfo->Value[ 9]);
                M3D_WriteReg32(M3D_TEX_M(unit,  8), pTexMatrixCtrlInfo->Value[11]);
                M3D_WriteReg32(M3D_TEX_M(unit,  9), pTexMatrixCtrlInfo->Value[12]);
                M3D_WriteReg32(M3D_TEX_M(unit, 10), pTexMatrixCtrlInfo->Value[13]);
                M3D_WriteReg32(M3D_TEX_M(unit, 11), pTexMatrixCtrlInfo->Value[15]);
            }
            else
            {
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  0), pTexMatrixCtrlInfo->Value[ 0]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  1), pTexMatrixCtrlInfo->Value[ 1]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  2), pTexMatrixCtrlInfo->Value[ 3]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  3), pTexMatrixCtrlInfo->Value[ 4]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  4), pTexMatrixCtrlInfo->Value[ 5]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  5), pTexMatrixCtrlInfo->Value[ 7]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  6), pTexMatrixCtrlInfo->Value[ 8]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  7), pTexMatrixCtrlInfo->Value[ 9]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  8), pTexMatrixCtrlInfo->Value[11]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit,  9), pTexMatrixCtrlInfo->Value[12]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit, 10), pTexMatrixCtrlInfo->Value[13]);
                M3D_FLT_WriteReg32(M3D_TEX_M(unit, 11), pTexMatrixCtrlInfo->Value[15]);
            }
        }
        else
        {
           m3d_tex_coord &= ~(3 << 4);
           M3D_WriteReg32(M3D_TEX_COORD(unit), m3d_tex_coord);
        }
    }
}
#else
void UpdateTexMatrix(TexMatrixStruct *pMatrix)
{
    UINT32 unit = pMatrix->Unit;
    GLuint m3d_tex_coord = M3D_ReadReg32(M3D_TEX_COORD(unit));

    if (pMatrix->Enable)
    {
       m3d_tex_coord |= (3 << 4);
       M3D_WriteReg32(M3D_TEX_COORD(unit), m3d_tex_coord);

        if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
        {
            M3D_WriteReg32(M3D_TEX_M(unit,  0), pMatrix->Value[ 0]);
            M3D_WriteReg32(M3D_TEX_M(unit,  1), pMatrix->Value[ 1]);
            M3D_WriteReg32(M3D_TEX_M(unit,  2), pMatrix->Value[ 3]);
            M3D_WriteReg32(M3D_TEX_M(unit,  3), pMatrix->Value[ 4]);
            M3D_WriteReg32(M3D_TEX_M(unit,  4), pMatrix->Value[ 5]);
            M3D_WriteReg32(M3D_TEX_M(unit,  5), pMatrix->Value[ 7]);
            M3D_WriteReg32(M3D_TEX_M(unit,  6), pMatrix->Value[ 8]);
            M3D_WriteReg32(M3D_TEX_M(unit,  7), pMatrix->Value[ 9]);
            M3D_WriteReg32(M3D_TEX_M(unit,  8), pMatrix->Value[11]);
            M3D_WriteReg32(M3D_TEX_M(unit,  9), pMatrix->Value[12]);
            M3D_WriteReg32(M3D_TEX_M(unit, 10), pMatrix->Value[13]);
            M3D_WriteReg32(M3D_TEX_M(unit, 11), pMatrix->Value[15]);
        }
        else
        {
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  0), pMatrix->Value[ 0]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  1), pMatrix->Value[ 1]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  2), pMatrix->Value[ 3]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  3), pMatrix->Value[ 4]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  4), pMatrix->Value[ 5]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  5), pMatrix->Value[ 7]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  6), pMatrix->Value[ 8]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  7), pMatrix->Value[ 9]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  8), pMatrix->Value[11]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit,  9), pMatrix->Value[12]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit, 10), pMatrix->Value[13]);
            M3D_FLT_WriteReg32(M3D_TEX_M(unit, 11), pMatrix->Value[15]);
        }
    }
    else
    {
       m3d_tex_coord &= ~(3 << 4);
       M3D_WriteReg32(M3D_TEX_COORD(unit), m3d_tex_coord);
    }
}
#endif

static void UpdateNormalize(NormalizeStruct *pNormalize)
{
    BOOL RescaleNormals = pNormalize->RescaleNormals;
    BOOL Normalized = pNormalize->Normalized;

    UINT32 m3d_light_ctrl_reg = M3D_ReadReg32(M3D_LIGHT_CTRL);

    if ((GL_TRUE == RescaleNormals) || (GL_TRUE == Normalized)) {
        m3d_light_ctrl_reg |= 0x0400;
    } else {
        m3d_light_ctrl_reg &= ~(0x0400);
    }

    M3D_WriteReg32(M3D_LIGHT_CTRL, m3d_light_ctrl_reg);
}

static void UpdateClipPlane(ClipPlaneStruct *pPlane)
{
    if (pPlane->Enabled) {
        M3D_WriteReg32(M3D_CLIP_PLANE_ENABLE, M3D_ENABLE_VALUE);
        M3D_Native_WriteReg32(M3D_C_N_X, pPlane->Plane[0]);
        M3D_Native_WriteReg32(M3D_C_N_Y, pPlane->Plane[1]);
        M3D_Native_WriteReg32(M3D_C_N_Z, pPlane->Plane[2]);
        M3D_Native_WriteReg32(M3D_C_N_W, pPlane->Plane[3]);
    } else {
        M3D_WriteReg32(M3D_CLIP_PLANE_ENABLE, M3D_DISABLE_VALUE);
    }
}

static void UpdateFogCtrl(FogCtrlStruct *pFogCtrl)
{
    UINT32 m3d_light_ctrl_reg;

    m3d_light_ctrl_reg = M3D_ReadReg32(M3D_LIGHT_CTRL);

    // Clean fog enable bit and fog mode bit
    m3d_light_ctrl_reg &= 0x230007FF;

    if (pFogCtrl->Enabled) {
        switch(pFogCtrl->Mode) {
            case GL_EXP:
                m3d_light_ctrl_reg |= 0x0800;
                break;
            case GL_EXP2:
                m3d_light_ctrl_reg |= 0x1800;
                break;
            case GL_LINEAR:
                m3d_light_ctrl_reg |= 0x2800;
                break;
            default:
                ASSERT(0);
                break;
        }

        M3D_Native_WriteReg32(M3D_FOG_DENSITY, pFogCtrl->Density);
        M3D_Native_WriteReg32(M3D_FOG_START_VAL, pFogCtrl->Start);
        M3D_Native_WriteReg32(M3D_FOG_END_VAL, pFogCtrl->End);
        M3D_Native_WriteReg32(M3D_FOG_COLOR_R, pFogCtrl->Color[0]);
        M3D_Native_WriteReg32(M3D_FOG_COLOR_G, pFogCtrl->Color[1]);
        M3D_Native_WriteReg32(M3D_FOG_COLOR_B, pFogCtrl->Color[2]);
        M3D_Native_WriteReg32(M3D_FOG_COLOR_A, pFogCtrl->Color[3]);
    }

    M3D_WriteReg32(M3D_LIGHT_CTRL, m3d_light_ctrl_reg);
}

static void UpdateLightCtrl(
    M3DContextStruct    *pCtx,
    LightCtrlStruct     *pLight)
{
    UINT32 i;

    UINT32 m3d_light_ctrl_reg;
    UINT32 DirtyFlags;
    GLnative *sceneAmbient = pLight->sceneAmbient;
    struct LightCtrlUnit *pLightCtrlInfo;
    struct MaterialCtrlUnit *pMaterialCtrlInfo = &pLight->MaterialCtrlInfo;
    BOOL  ColorMaterialEnabled = (BOOL)pMaterialCtrlInfo->mat_ColorMaterial;

    /// clear color material bit and light bits
    /// m3d_light_ctrl_reg &= ~(0x003F01FF);

    /// no matter lighting is enabled or not, the shade model should be cirrectly set
    /// M3D_WriteReg32(M3D_SHADE_MODEL, pLight->ShadeModel);

    /// the case of glDisable(GL_LIGHTING)
    m3d_light_ctrl_reg = M3D_ReadReg32(M3D_LIGHT_CTRL);
    pCtx->LightCtrl.LightingCtrl = pLight->LightingCtrl;
    if(FALSE == pLight->LightingCtrl)
    {
       m3d_light_ctrl_reg &= ~(M3D_ENABLE_VALUE);
       M3D_WriteReg32(M3D_LIGHT_CTRL, m3d_light_ctrl_reg);
       return;
    }
    else
    {
       /// Lighting Model Parameters
       M3D_Native_WriteReg32(M3D_A_CS_R, sceneAmbient[0]);
       M3D_Native_WriteReg32(M3D_A_CS_G, sceneAmbient[1]);
       M3D_Native_WriteReg32(M3D_A_CS_B, sceneAmbient[2]);
       M3D_Native_WriteReg32(M3D_A_CS_A, sceneAmbient[3]);
       
       /// LIGHT Enable control
       m3d_light_ctrl_reg |= M3D_ENABLE;
       M3D_WriteReg32(M3D_LIGHT_CTRL, m3d_light_ctrl_reg);
    }

    for (i = 0; i < MAX_LIGHT_SOURCES; i++)
    {
       pLightCtrlInfo = &pLight->LightCtrlInfo[i];

       m3d_light_ctrl_reg = M3D_ReadReg32(M3D_LIGHT_CTRL);

       if (FALSE == pLightCtrlInfo->LightEnable)
       {
          /// Index = pLightCtrlInfo->LightSource;
          m3d_light_ctrl_reg &= ~(1 << (i + 1));
          M3D_WriteReg32(M3D_LIGHT_CTRL, m3d_light_ctrl_reg);
          continue;
       }

       /// the case of glEnable(GL_LIGHTING)
       if (pLightCtrlInfo->LightEnable)
       {
          /// Index = pLightCtrlInfo->LightSource;
          DirtyFlags = pLightCtrlInfo->DirtyFlags;
                       
          /// LIGHT Enable control
          /// m3d_light_ctrl_reg |= M3D_ENABLE;
          m3d_light_ctrl_reg |= 0x23000000;

          /// enable each lihgt source
          m3d_light_ctrl_reg |= (1 << (i + 1));                   
                      
          if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
          {                              
             if (DirtyFlags & NEW_AMBIENT_COLOR) {
                 M3D_WriteReg32(M3D_AMBIENT_CL_R(i), pLightCtrlInfo->Ambient[0]);
                 M3D_WriteReg32(M3D_AMBIENT_CL_G(i), pLightCtrlInfo->Ambient[1]);
                 M3D_WriteReg32(M3D_AMBIENT_CL_B(i), pLightCtrlInfo->Ambient[2]);
                 M3D_WriteReg32(M3D_AMBIENT_CL_A(i), pLightCtrlInfo->Ambient[3]);
             }
             
             if (DirtyFlags & NEW_DIFFUSE_COLOR) {
                 M3D_WriteReg32(M3D_DIFFUSE_CL_R(i), pLightCtrlInfo->Diffuse[0]);
                 M3D_WriteReg32(M3D_DIFFUSE_CL_G(i), pLightCtrlInfo->Diffuse[1]);
                 M3D_WriteReg32(M3D_DIFFUSE_CL_B(i), pLightCtrlInfo->Diffuse[2]);
                 M3D_WriteReg32(M3D_DIFFUSE_CL_A(i), pLightCtrlInfo->Diffuse[3]);
             }
             
             if (DirtyFlags & NEW_SPECULAR_COLOR) {
                 M3D_WriteReg32(M3D_SPECULAR_CL_R(i), pLightCtrlInfo->Specular[0]);
                 M3D_WriteReg32(M3D_SPECULAR_CL_G(i), pLightCtrlInfo->Specular[1]);
                 M3D_WriteReg32(M3D_SPECULAR_CL_B(i), pLightCtrlInfo->Specular[2]);
                 M3D_WriteReg32(M3D_SPECULAR_CL_A(i), pLightCtrlInfo->Specular[3]);
             }
             
             if (DirtyFlags & NEW_POSITION_VALUE) {
                 M3D_WriteReg32(M3D_POSITION_PL_X(i), pLightCtrlInfo->Position[0]);
                 M3D_WriteReg32(M3D_POSITION_PL_Y(i), pLightCtrlInfo->Position[1]);
                 M3D_WriteReg32(M3D_POSITION_PL_Z(i), pLightCtrlInfo->Position[2]);
                 M3D_WriteReg32(M3D_POSITION_PL_W(i), pLightCtrlInfo->Position[3]);
             }
             
             if (DirtyFlags & NEW_SPOT_DIRECTION) {
                 M3D_WriteReg32(M3D_SPOT_LIGHT_DL_X(i), pLightCtrlInfo->Direction[0]);
                 M3D_WriteReg32(M3D_SPOT_LIGHT_DL_Y(i), pLightCtrlInfo->Direction[1]);
                 M3D_WriteReg32(M3D_SPOT_LIGHT_DL_Z(i), pLightCtrlInfo->Direction[2]);
             }
             
             if (DirtyFlags & NEW_SPOT_EXPONENT) {
                 M3D_WriteReg32(M3D_SPOT_LIGHT_RL(i), pLightCtrlInfo->SpotExponent);
             }
             
             if (DirtyFlags & NEW_SPOT_CUTOFF) {
                 M3D_WriteReg32(M3D_SPOT_CUTOFF_RL(i), pLightCtrlInfo->CosCutoff);
             }
             
             if (DirtyFlags & NEW_CONST_ATTENUX) {
                 M3D_WriteReg32(M3D_CONST_ATTENUX(i), pLightCtrlInfo->ConstAttenx);
             }
             
             if (DirtyFlags & NEW_LINEAR_ATTENUX) {
                 M3D_WriteReg32(M3D_LINEAR_ATTENUX(i), pLightCtrlInfo->LinearAttenx);
             }
             
             if (DirtyFlags & NEW_QUADX_ATTENUX) {
                 M3D_WriteReg32(M3D_QUADX_ATTENUX(i), pLightCtrlInfo->QuadAttenx);
             }                                
          }
          else
          {
             if (DirtyFlags & NEW_AMBIENT_COLOR) {
                 M3D_FLT_WriteReg32(M3D_AMBIENT_CL_R(i), pLightCtrlInfo->Ambient[0]);
                 M3D_FLT_WriteReg32(M3D_AMBIENT_CL_G(i), pLightCtrlInfo->Ambient[1]);
                 M3D_FLT_WriteReg32(M3D_AMBIENT_CL_B(i), pLightCtrlInfo->Ambient[2]);
                 M3D_FLT_WriteReg32(M3D_AMBIENT_CL_A(i), pLightCtrlInfo->Ambient[3]);
             }

             if (DirtyFlags & NEW_DIFFUSE_COLOR) {
                 M3D_FLT_WriteReg32(M3D_DIFFUSE_CL_R(i), pLightCtrlInfo->Diffuse[0]);
                 M3D_FLT_WriteReg32(M3D_DIFFUSE_CL_G(i), pLightCtrlInfo->Diffuse[1]);
                 M3D_FLT_WriteReg32(M3D_DIFFUSE_CL_B(i), pLightCtrlInfo->Diffuse[2]);
                 M3D_FLT_WriteReg32(M3D_DIFFUSE_CL_A(i), pLightCtrlInfo->Diffuse[3]);
             }

             if (DirtyFlags & NEW_SPECULAR_COLOR) {
                 M3D_FLT_WriteReg32(M3D_SPECULAR_CL_R(i), pLightCtrlInfo->Specular[0]);
                 M3D_FLT_WriteReg32(M3D_SPECULAR_CL_G(i), pLightCtrlInfo->Specular[1]);
                 M3D_FLT_WriteReg32(M3D_SPECULAR_CL_B(i), pLightCtrlInfo->Specular[2]);
                 M3D_FLT_WriteReg32(M3D_SPECULAR_CL_A(i), pLightCtrlInfo->Specular[3]);
             }

             if (DirtyFlags & NEW_POSITION_VALUE) {
                 M3D_FLT_WriteReg32(M3D_POSITION_PL_X(i), pLightCtrlInfo->Position[0]);
                 M3D_FLT_WriteReg32(M3D_POSITION_PL_Y(i), pLightCtrlInfo->Position[1]);
                 M3D_FLT_WriteReg32(M3D_POSITION_PL_Z(i), pLightCtrlInfo->Position[2]);
                 M3D_FLT_WriteReg32(M3D_POSITION_PL_W(i), pLightCtrlInfo->Position[3]);
             }

             if (DirtyFlags & NEW_SPOT_DIRECTION) {
                 M3D_FLT_WriteReg32(M3D_SPOT_LIGHT_DL_X(i), pLightCtrlInfo->Direction[0]);
                 M3D_FLT_WriteReg32(M3D_SPOT_LIGHT_DL_Y(i), pLightCtrlInfo->Direction[1]);
                 M3D_FLT_WriteReg32(M3D_SPOT_LIGHT_DL_Z(i), pLightCtrlInfo->Direction[2]);
             }

             if (DirtyFlags & NEW_SPOT_EXPONENT) {
                 M3D_FLT_WriteReg32(M3D_SPOT_LIGHT_RL(i), pLightCtrlInfo->SpotExponent);
             }

             if (DirtyFlags & NEW_SPOT_CUTOFF) {
                 M3D_FLT_WriteReg32(M3D_SPOT_CUTOFF_RL(i), pLightCtrlInfo->CosCutoff);
             }

             if (DirtyFlags & NEW_CONST_ATTENUX) {
                 M3D_FLT_WriteReg32(M3D_CONST_ATTENUX(i), pLightCtrlInfo->ConstAttenx);
             }

             if (DirtyFlags & NEW_LINEAR_ATTENUX) {
                 M3D_FLT_WriteReg32(M3D_LINEAR_ATTENUX(i), pLightCtrlInfo->LinearAttenx);
             }

             if (DirtyFlags & NEW_QUADX_ATTENUX) {
                 M3D_FLT_WriteReg32(M3D_QUADX_ATTENUX(i), pLightCtrlInfo->QuadAttenx);
             }
          }

          M3D_WriteReg32(M3D_LIGHT_CTRL, m3d_light_ctrl_reg);
        }
    }


    {
        UINT32 bitmask = pMaterialCtrlInfo->mat_DirtyFlags;
        GLnative (*mat)[4] = pMaterialCtrlInfo->mat_Attrib;
        UINT32 m3d_light_ctrl_reg = M3D_ReadReg32(M3D_LIGHT_CTRL);

        /// M3D_LIGHT_CTRL[24]: 1  (M3D_COLOR_SUM_LIGHT)
        /// M3D_LIGHT_CTRL[25]: 1  (M3D_LIGHT_SPOT_ENABLE)
        /// M3D_LIGHT_CTRL[26]: 0  (M3D_LIGHT_LVIEW_ENABLE)
        /// M3D_LIGHT_CTRL[27]: 0  (M3D_LIGHT_RANGE_ENBALE)
        /// M3D_LIGHT_CTRL[28]: 0  (M3D_FOG_CUSTOM)
        /// M3D_LIGHT_CTRL[29]: 1  (M3D_LIGHT_F_ENABLE)
        /// M3D_LIGHT_CTRL[30]: 0  (M3D_LIGHT_INF_POS)
        m3d_light_ctrl_reg |= 0x23000000;

        if (ColorMaterialEnabled){
            m3d_light_ctrl_reg |= 0x00050000;
        }
        else
     	 {
            m3d_light_ctrl_reg &= ~0x00050000;
    	 }

        m3d_light_ctrl_reg |= M3D_ENABLE;
        M3D_WriteReg32(M3D_LIGHT_CTRL, m3d_light_ctrl_reg);

        /// config ambient
        if (bitmask & MAT_BIT_FRONT_AMBIENT)
        {
            M3D_Native_WriteReg32(M3D_A_CM_R, mat[MAT_ATTRIB_FRONT_AMBIENT][0]);
            M3D_Native_WriteReg32(M3D_A_CM_G, mat[MAT_ATTRIB_FRONT_AMBIENT][1]);
            M3D_Native_WriteReg32(M3D_A_CM_B, mat[MAT_ATTRIB_FRONT_AMBIENT][2]);
            M3D_Native_WriteReg32(M3D_A_CM_A, mat[MAT_ATTRIB_FRONT_AMBIENT][3]);
        }

        /// config diffuse
        if (bitmask & MAT_BIT_FRONT_DIFFUSE)
        {
            M3D_Native_WriteReg32(M3D_D_CM_R, mat[MAT_ATTRIB_FRONT_DIFFUSE][0]);
            M3D_Native_WriteReg32(M3D_D_CM_G, mat[MAT_ATTRIB_FRONT_DIFFUSE][1]);
            M3D_Native_WriteReg32(M3D_D_CM_B, mat[MAT_ATTRIB_FRONT_DIFFUSE][2]);
            M3D_Native_WriteReg32(M3D_D_CM_A, mat[MAT_ATTRIB_FRONT_DIFFUSE][3]);
        }

        /// config specular
        if (bitmask & MAT_BIT_FRONT_SPECULAR)
        {
            M3D_Native_WriteReg32(M3D_S_CM_R, mat[MAT_ATTRIB_FRONT_SPECULAR][0]);
            M3D_Native_WriteReg32(M3D_S_CM_G, mat[MAT_ATTRIB_FRONT_SPECULAR][1]);
            M3D_Native_WriteReg32(M3D_S_CM_B, mat[MAT_ATTRIB_FRONT_SPECULAR][2]);
            M3D_Native_WriteReg32(M3D_S_CM_A, mat[MAT_ATTRIB_FRONT_SPECULAR][3]);
        }

        /// config emission
        if (bitmask & MAT_BIT_FRONT_EMISSION)
        {
            M3D_Native_WriteReg32(M3D_E_CM_R, mat[MAT_ATTRIB_FRONT_EMISSION][0]);
            M3D_Native_WriteReg32(M3D_E_CM_G, mat[MAT_ATTRIB_FRONT_EMISSION][1]);
            M3D_Native_WriteReg32(M3D_E_CM_B, mat[MAT_ATTRIB_FRONT_EMISSION][2]);
            M3D_Native_WriteReg32(M3D_E_CM_A, mat[MAT_ATTRIB_FRONT_EMISSION][3]);
        }

        /// config shineness
        if (bitmask & MAT_BIT_FRONT_SHININESS)
        {
            M3D_Native_WriteReg32(M3D_S_RM, mat[MAT_ATTRIB_FRONT_SHININESS][0]);
        }
    }
}

static void UpdatePrimitiveCtrl(PrimitiveCtrlStruct *pPrimitiveCtrl)
{
    struct PointCtrlStruct  *pPoint = &pPrimitiveCtrl->PointCtrlInfo;
    struct LineCtrlStruct *pLine = &pPrimitiveCtrl->LineCtrlInfo;

    UINT32 Temp;
    {
        BOOL pntSprite = pPoint->PntSprite;
        UINT32 PntSpriteCtrl = pPoint->PntSpriteCtrl;

        Temp = M3D_ReadReg32(M3D_PRIMITIVE_AA);

        if (pPoint->Smooth) {
            Temp |= 0x0004;
        } else {
            Temp &= ~0x0004;
        }

        M3D_WriteReg32(M3D_PRIMITIVE_AA, Temp);

        M3D_Native_WriteReg32(M3D_POINT_SIZE_MIN, pPoint->MinSize);
        M3D_Native_WriteReg32(M3D_POINT_SIZE_MAX, pPoint->MaxSize);

        M3D_Native_WriteReg32(M3D_PNT_SIZE, pPoint->CurrSize);
        M3D_WriteReg32(M3D_PNT_SIZE_INPUT, M3D_PNT_SIZE_INPUT_REGISTER);

        M3D_Native_WriteReg32(M3D_POINT_ATTENUX_A, pPoint->Factor[0]);
        M3D_Native_WriteReg32(M3D_POINT_ATTENUX_B, pPoint->Factor[1]);
        M3D_Native_WriteReg32(M3D_POINT_ATTENUX_C, pPoint->Factor[2]);

        if (GL_TRUE == pntSprite) {
            M3D_WriteReg32(M3D_POINT_SPRITE, (PntSpriteCtrl << 1) | M3D_ENABLE);
        } else {
            M3D_WriteReg32(M3D_POINT_SPRITE, M3D_DISABLE);
        }
    }

    Temp = M3D_ReadReg32(M3D_PRIMITIVE_AA);
    if (pLine->Smooth) {
        Temp |= 0x0002;
    } else {
        Temp &= ~0x0002;
    }
    M3D_WriteReg32(M3D_PRIMITIVE_AA, Temp);

    M3D_Native_WriteReg32(M3D_LINE_WIDTH, pLine->Width);

}

static void UpdateTexture(TextureCtrlStruct *pTexCtrl)
{
    register GLuint unit;

    if (!pTexCtrl->TextureEnabled) {
        GLuint tex_ctrl = M3D_ReadReg32(M3D_TEX_CTRL);
        tex_ctrl &= ~(0x07E000FF);
        M3D_WriteReg32(M3D_TEX_CTRL, tex_ctrl);
        return;
    }

    for (unit = 0; unit < MAX_TEXTURE_UNITS; unit++)
    {
        struct TexInfoUnit *pTexInfoUnit = &pTexCtrl->TexInfo[unit];

        register GLuint i; 
        UINT32 value;
        UINT32 temp;
        GLuint m3d_tex_para_reg = 0;
        GLuint m3d_tex_img_reg = 0;
        GLuint m3d_tex_op_reg = 0;
        GLuint m3d_tex_src_opd_reg = 0;       
        GLuint enable;       
        GLuint EnabledUnits = 0;
        UINT32 TexFormat = pTexInfoUnit->Format;
        UINT32 TexType   = pTexInfoUnit->Type;
       
        enable = pTexInfoUnit->Enable;

        value = M3D_ReadReg32(M3D_TEX_CTRL);
        value |= 0x07E00000;

        EnabledUnits |= (1 << unit);
        temp = EnabledUnits | (EnabledUnits << 4);
        if (enable) {
            M3D_WriteReg32(M3D_TEX_CTRL, value | temp);
        } else {
            M3D_WriteReg32(M3D_TEX_CTRL, value & ~temp);
            continue;
        }

        switch (TexFormat) {
            case GL_ARGB:
            /// (2006.02.08) Currently GL_ARGB supports only 4444 format requested by Java
            switch (TexType) {
                case GL_UNSIGNED_SHORT_4_4_4_4:
                   m3d_tex_img_reg = M3D_TEX_FMT_A4R4G4B4;
                   break;
                default:
                   GL_ASSERT(0);
                   break;
            }
            break;

            case GL_ALPHA:
                GL_ASSERT(TexType == GL_UNSIGNED_BYTE);
                m3d_tex_img_reg = M3D_TEX_FMT_A8;
                break;

            case GL_RGB:
            switch (TexType) {
                case GL_UNSIGNED_BYTE:
                    m3d_tex_img_reg = M3D_TEX_FMT_R8G8B8;
                    break;
                case GL_UNSIGNED_SHORT_5_6_5:
                    m3d_tex_img_reg = M3D_TEX_FMT_R5G6B5;
                    break;
                default:
                    GL_ASSERT(0);
                    break;
            }
            break;

#ifdef SUPPORT_BGRA
            case GL_BGRA:
#endif            
            case GL_RGBA:
            switch (TexType) {
                case GL_UNSIGNED_BYTE:
                    m3d_tex_img_reg = M3D_TEX_FMT_A8R8G8B8;
                    break;
                case GL_UNSIGNED_SHORT_4_4_4_4:
                    m3d_tex_img_reg = M3D_TEX_FMT_A4R4G4B4;
                    break;
                case GL_UNSIGNED_SHORT_5_5_5_1:
                    m3d_tex_img_reg = M3D_TEX_FMT_A1R5G5B5;
                    break;
                default:
                    GL_ASSERT(0);
                    break;
            }
            break;

            case GL_LUMINANCE:
                GL_ASSERT(TexType == GL_UNSIGNED_BYTE);
                m3d_tex_img_reg = M3D_TEX_FMT_L8;
                break;

            case GL_LUMINANCE_ALPHA:
                GL_ASSERT(TexType == GL_UNSIGNED_BYTE);
                m3d_tex_img_reg = M3D_TEX_FMT_L8A8;
                break;
            default:
                GL_ASSERT(0);
                break;
        }

        m3d_tex_img_reg |= ((pTexInfoUnit->Width  & 0x03FF) << M3D_TEX_IMG_WIDTH_OFFSET);
        m3d_tex_img_reg |= ((pTexInfoUnit->Height & 0x03FF) << M3D_TEX_IMG_HEIGHT_OFFSET);

#ifdef SUPPORT_BGRA
        if (TexFormat == GL_BGRA){
            m3d_tex_img_reg |= 1 << M3D_TEX_BYTE_ORDER_OFFSET; /*Enable D3DM*/
        }
#endif

        M3D_WriteReg32(M3D_TEX_IMG(unit), m3d_tex_img_reg);

        /// Set texture image pointer
        for (i=0; i<MAX_TEXTURE_LEVELS; i++) {
            M3D_WriteReg32(M3D_TEX_IMG_PTR(unit, i), (GLuint)(pTexInfoUnit->image_data[i]));
        }

        /// Set texture parameter (min/mag filter, wrap S/T)
        switch(pTexInfoUnit->MinFilter) {
        case GL_NEAREST:
            m3d_tex_para_reg = M3D_TEX_NEAREST;
            break;
        case GL_LINEAR:
            m3d_tex_para_reg = M3D_TEX_LINEAR;
            break;
        case GL_NEAREST_MIPMAP_NEAREST:
            m3d_tex_para_reg = M3D_TEX_NEAREST_MIPMAP_NEAREST;
            break;
        case GL_LINEAR_MIPMAP_NEAREST:
            m3d_tex_para_reg = M3D_TEX_LINEAR_MIPMAP_NEAREST;
            break;
        case GL_NEAREST_MIPMAP_LINEAR:
            m3d_tex_para_reg = M3D_TEX_NEAREST_MIPMAP_LINEAR;
            break;
        case GL_LINEAR_MIPMAP_LINEAR:
            m3d_tex_para_reg = M3D_TEX_LINEAR_MIPMAP_LINEAR;
            break;
        }

        if (GL_LINEAR == pTexInfoUnit->MagFilter) {
            m3d_tex_para_reg |= M3D_TEX_MAG_FILTER_LINEAR;
        }

        if (GL_CLAMP_TO_EDGE == pTexInfoUnit->WrapS) {
            m3d_tex_para_reg |= M3D_TEX_WRAP_S_CLAMP_TO_EDGE;
        }

        if (GL_CLAMP_TO_EDGE == pTexInfoUnit->WrapT) {
            m3d_tex_para_reg |= M3D_TEX_WRAP_T_CLAMP_TO_EDGE;
        }
        M3D_WriteReg32(M3D_TEX_PARA(unit), m3d_tex_para_reg);

        /// Set texture environment color
        if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
        {
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_R(unit), pTexInfoUnit->EnvColor[0]);
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_G(unit), pTexInfoUnit->EnvColor[1]);
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_B(unit), pTexInfoUnit->EnvColor[2]);
            M3D_WriteReg32(M3D_TEX_ENV_COLOR_A(unit), pTexInfoUnit->EnvColor[3]);
        }
        else
        {
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_R(unit), pTexInfoUnit->EnvColor[0]);
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_G(unit), pTexInfoUnit->EnvColor[1]);
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_B(unit), pTexInfoUnit->EnvColor[2]);
            M3D_FLT_WriteReg32(M3D_TEX_ENV_COLOR_A(unit), pTexInfoUnit->EnvColor[3]);
        }
        /// Set texture environment
        switch(pTexInfoUnit->EnvMode)
        {
        case GL_MODULATE:
            /// RGB part
            switch (TexFormat)
            {
            case GL_ALPHA:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_RGB_OFFSET);
                break;
            case GL_LUMINANCE:
            case GL_LUMINANCE_ALPHA:
            case GL_RGB:
            case GL_RGBA:
#ifdef SUPPORT_BGRA
            case GL_BGRA:
#endif            
            case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_MODULATE << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC2_RGB_OFFSET);
                break;
            default:
                GL_ASSERT(0);
            }

            /// Alpha part
            switch (TexFormat)
            {
            case GL_ALPHA:
            case GL_RGBA:
#ifdef SUPPORT_BGRA
            case GL_BGRA:
#endif            
            case GL_ARGB:
            case GL_LUMINANCE_ALPHA:
                m3d_tex_op_reg |= (M3D_TEX_OP_MODULATE << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET) |
                                       (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC2_A_OFFSET);
                break;
            case GL_LUMINANCE:
            case GL_RGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET);
                break;
            default:
                GL_ASSERT(0);
                break;
            }
            break;

       case GL_DECAL:
          /// RGB part
          switch (TexFormat) {
             case GL_RGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_TEXTURE << M3D_TEX_SRC1_RGB_OFFSET);
                break;
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_LERP << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC0_RGB_OFFSET) |
                                       (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC2_RGB_OFFSET) |
                                       (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD0_RGB_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }

          /// Alpha part
          switch (TexFormat) {
             case GL_RGB:
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }
          break;

       case GL_BLEND:
          /// RGB part
          switch (TexFormat) {
             case GL_ALPHA:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_RGB_OFFSET);
                break;
             case GL_LUMINANCE:
             case GL_LUMINANCE_ALPHA:
             case GL_RGB:
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_LERP << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC0_RGB_OFFSET) |
                                       (M3D_TEX_SRC_CONSTANT << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }

          /// Alpha part
          switch (TexFormat) {
             case GL_ALPHA:
             case GL_LUMINANCE_ALPHA:
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_MODULATE << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET) |
                                       (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC2_A_OFFSET);
                break;
             case GL_LUMINANCE:
             case GL_RGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }
          break;

       case GL_REPLACE:
          /// RGB part
          switch (TexFormat) {
             case GL_ALPHA:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_RGB_OFFSET);
                break;
             case GL_LUMINANCE:
             case GL_LUMINANCE_ALPHA:
             case GL_RGB:
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_TEXTURE << M3D_TEX_SRC1_RGB_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }

          /// Alpha part
          switch (TexFormat) {
             case GL_ALPHA:
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
             case GL_LUMINANCE_ALPHA:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_TEXTURE << M3D_TEX_SRC1_A_OFFSET);
                break;
             case GL_LUMINANCE:
             case GL_RGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }
          break;

       case GL_ADD:
          /// RGB part
          switch (TexFormat) {
             case GL_ALPHA:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_RGB_OFFSET);
                break;
             case GL_LUMINANCE:
             case GL_LUMINANCE_ALPHA:
             case GL_RGB:
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_ADD << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }

          /// Alpha part
          switch (TexFormat) {
             case GL_ALPHA:
             case GL_LUMINANCE_ALPHA:
             case GL_RGBA:
#ifdef SUPPORT_BGRA
             case GL_BGRA:
#endif            
             case GL_ARGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_MODULATE << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET) |
                                       (M3D_TEX_SRC_TEXTURE  << M3D_TEX_SRC2_A_OFFSET);
                break;
             case GL_LUMINANCE:
             case GL_RGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (M3D_TEX_SRC_PREVIOUS << M3D_TEX_SRC1_A_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }
          break;

       case GL_COMBINE:
          /// RGB part
          switch(pTexInfoUnit->CombineModeRGB) {
             case GL_REPLACE:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET);
                break;
             case GL_MODULATE:
                m3d_tex_op_reg |= (M3D_TEX_OP_MODULATE << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[1]) << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             case GL_ADD:
                m3d_tex_op_reg |= (M3D_TEX_OP_ADD << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[1]) << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             case GL_ADD_SIGNED:
                m3d_tex_op_reg |= (M3D_TEX_OP_ADDSIGNED << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[1]) << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             case GL_INTERPOLATE:
                m3d_tex_op_reg |= (M3D_TEX_OP_LERP << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[2]) << M3D_TEX_SRC0_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[1]) << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             case GL_SUBTRACT:
                m3d_tex_op_reg |= (M3D_TEX_OP_SUBTRACT << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[1]) << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             case GL_DOT3_RGB:
                m3d_tex_op_reg |= (M3D_TEX_OP_DOTPRODUCT3 << M3D_TEX_OP_RGB_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[1]) << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             case GL_DOT3_RGBA:
                m3d_tex_op_reg |= (M3D_TEX_OP_DOTPRODUCT3 << M3D_TEX_OP_RGB_OFFSET) |
                                  (1 << M3D_TEX_DOT3_RGBA_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[0]) << M3D_TEX_SRC1_RGB_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceRGB[1]) << M3D_TEX_SRC2_RGB_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }

          /// Alpha part
          switch(pTexInfoUnit->CombineModeA) {
             case GL_REPLACE:
                m3d_tex_op_reg |= (M3D_TEX_OP_SELECTARG1 << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[0]) << M3D_TEX_SRC1_A_OFFSET);
                break;
             case GL_MODULATE:
                m3d_tex_op_reg |= (M3D_TEX_OP_MODULATE << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[0]) << M3D_TEX_SRC1_A_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[1]) << M3D_TEX_SRC2_A_OFFSET);
                break;
             case GL_ADD:
                m3d_tex_op_reg |= (M3D_TEX_OP_ADD << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[0]) << M3D_TEX_SRC1_A_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[1]) << M3D_TEX_SRC2_A_OFFSET);
                break;
             case GL_ADD_SIGNED:
                m3d_tex_op_reg |= (M3D_TEX_OP_ADDSIGNED << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[0]) << M3D_TEX_SRC1_A_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[1]) << M3D_TEX_SRC2_A_OFFSET);
                break;
             case GL_INTERPOLATE:
                m3d_tex_op_reg |= (M3D_TEX_OP_LERP << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[2]) << M3D_TEX_SRC0_A_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[0]) << M3D_TEX_SRC1_A_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[1]) << M3D_TEX_SRC2_A_OFFSET);
                break;
             case GL_SUBTRACT:
                m3d_tex_op_reg |= (M3D_TEX_OP_SUBTRACT << M3D_TEX_OP_A_OFFSET);
                m3d_tex_src_opd_reg |= (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[0]) << M3D_TEX_SRC1_A_OFFSET) |
                                       (_gl_convert_texture_src_rgba(pTexInfoUnit->CombineSourceA[1]) << M3D_TEX_SRC2_A_OFFSET);
                break;
             default:
                GL_ASSERT(0);
          }
          break;
       }

       /// Modify by Jett Liu:
       /// In OpenGL-ES, texture source operand and texture scale registers are set
       /// only when the environment mode is COMBINE.
       if (GL_COMBINE == pTexInfoUnit->EnvMode) {
          m3d_tex_op_reg |= (pTexInfoUnit->CombineScaleShiftRGB << M3D_TEX_SCALE_RGB_OFFSET) |
                            (pTexInfoUnit->CombineScaleShiftA << M3D_TEX_SCALE_A_OFFSET);

          m3d_tex_src_opd_reg |= (_gl_convert_texture_operand_rgba(pTexInfoUnit->CombineOperandRGB[0]) << M3D_TEX_OPD1_RGB_OFFSET) |
                                 (_gl_convert_texture_operand_rgba(pTexInfoUnit->CombineOperandRGB[1]) << M3D_TEX_OPD2_RGB_OFFSET) |
                                 (_gl_convert_texture_operand_rgba(pTexInfoUnit->CombineOperandRGB[2]) << M3D_TEX_OPD0_RGB_OFFSET);

          m3d_tex_src_opd_reg |= (_gl_convert_texture_operand_rgba(pTexInfoUnit->CombineOperandA[0]) << M3D_TEX_OPD1_A_OFFSET) |
                                 (_gl_convert_texture_operand_rgba(pTexInfoUnit->CombineOperandA[1]) << M3D_TEX_OPD2_A_OFFSET) |
                                 (_gl_convert_texture_operand_rgba(pTexInfoUnit->CombineOperandA[2]) << M3D_TEX_OPD0_A_OFFSET);
       } else {
          /// initialize as default values
          m3d_tex_src_opd_reg |= (M3D_TEX_OPD_SRC_COLOR << M3D_TEX_OPD1_RGB_OFFSET) |
                                 (M3D_TEX_OPD_SRC_COLOR << M3D_TEX_OPD2_RGB_OFFSET) |
                                 (M3D_TEX_OPD_SRC_COLOR << M3D_TEX_OPD0_RGB_OFFSET);

          m3d_tex_src_opd_reg |= (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD1_A_OFFSET) |
                                 (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD2_A_OFFSET) |
                                 (M3D_TEX_OPD_SRC_ALPHA << M3D_TEX_OPD0_A_OFFSET);
       }

       M3D_WriteReg32(M3D_TEX_OP(unit), m3d_tex_op_reg);
       M3D_WriteReg32(M3D_TEX_SRC_OPD(unit), m3d_tex_src_opd_reg);
   }
}

static void UpdateColorCtrl(ColorCtrlStruct *pColorCtrl)
{
    UINT32 Temp;

    //M3D_WriteReg32(M3D_COLOR_0, M3D_ENABLE_VALUE);	//UpdateArrayInfo will set //Kevin
    M3D_Native_WriteReg32(M3D_COLOR_R_0_VALUE, pColorCtrl->Color[0]);
    M3D_Native_WriteReg32(M3D_COLOR_G_0_VALUE, pColorCtrl->Color[1]);
    M3D_Native_WriteReg32(M3D_COLOR_B_0_VALUE, pColorCtrl->Color[2]);
    M3D_Native_WriteReg32(M3D_COLOR_A_0_VALUE, pColorCtrl->Color[3]);

    Temp = M3D_ReadReg32(M3D_BLEND);

    if (pColorCtrl->Mask[0] == 0xFF)
    {
        Temp |= M3D_BLEND_R_MASK;
    }
    else
    {
        Temp &= ~M3D_BLEND_R_MASK;
    }

    if (pColorCtrl->Mask[1] == 0xFF)
    {
        Temp |= M3D_BLEND_G_MASK;
    }
    else
    {
        Temp &= ~M3D_BLEND_G_MASK;
    }

    if (pColorCtrl->Mask[2] == 0xFF)
    {
        Temp |= M3D_BLEND_B_MASK;
    }
    else
    {
        Temp &= ~M3D_BLEND_B_MASK;
    }

    if (pColorCtrl->Mask[3] == 0xFF) {
        Temp |= M3D_BLEND_A_MASK;
    }
    else
    {
        Temp &= ~M3D_BLEND_A_MASK;
    }

    M3D_WriteReg32(M3D_BLEND, Temp);

    if (pColorCtrl->ShadeModel == GL_FLAT)
    {
        M3D_WriteReg32(M3D_SHADE_MODEL, M3D_SHADE_FLAT);
    }
    else
    {
        M3D_WriteReg32(M3D_SHADE_MODEL, M3D_SHADE_SMOOTH);
    }
}

static void UpdatePerFragmentTest(PerFragmentTestStruct *pPerFragment)
{
    UINT32 Temp;
    UINT32 m3d_blend_reg;

    if (pPerFragment->AlphaTestEnabled) {

        M3D_WriteReg32(M3D_ALPHA_TEST, (((pPerFragment->AlphaTestReference) << 4) |
                                    ((pPerFragment->AlphaTestFunction & 0x07) << 1) |
                                     M3D_ENABLE_VALUE));
   } else {
        M3D_WriteReg32(M3D_ALPHA_TEST, M3D_DISABLE_VALUE);
   }
   
   m3d_blend_reg = M3D_ReadReg32(M3D_BLEND);
   m3d_blend_reg &= ~(0x000001FF);

   if (pPerFragment->BlendTestEnabled) {
        switch(pPerFragment->BlendTestSFactor) {
            case GL_SRC_COLOR:
            case GL_ONE_MINUS_SRC_COLOR:
            case GL_SRC_ALPHA:
            case GL_ONE_MINUS_SRC_ALPHA:
            case GL_DST_ALPHA:
            case GL_ONE_MINUS_DST_ALPHA:
            case GL_DST_COLOR:
            case GL_ONE_MINUS_DST_COLOR:
            case GL_SRC_ALPHA_SATURATE:
                m3d_blend_reg |= ((pPerFragment->BlendTestSFactor & 0x0F) << 1);
                break;
            case GL_ZERO:
            case GL_ONE:
                m3d_blend_reg |= (((pPerFragment->BlendTestSFactor & 0x0F) + 9) << 1);
                break;
        }

        switch(pPerFragment->BlendTestDFactor) {
            case GL_SRC_COLOR:
            case GL_ONE_MINUS_SRC_COLOR:
            case GL_SRC_ALPHA:
            case GL_ONE_MINUS_SRC_ALPHA:
            case GL_DST_ALPHA:
            case GL_ONE_MINUS_DST_ALPHA:
            case GL_DST_COLOR:
            case GL_ONE_MINUS_DST_COLOR:
            case GL_SRC_ALPHA_SATURATE:
                m3d_blend_reg |= ((pPerFragment->BlendTestDFactor & 0x0F) << 5);
                break;
            case GL_ZERO:
            case GL_ONE:
                m3d_blend_reg |= (((pPerFragment->BlendTestDFactor & 0x0F) + 9) << 5);
                break;
        }
        M3D_WriteReg32(M3D_BLEND, m3d_blend_reg | M3D_ENABLE_VALUE);
    } else {
        m3d_blend_reg &= ~M3D_ENABLE_VALUE;

        M3D_WriteReg32(M3D_BLEND, m3d_blend_reg);
    }

    if (pPerFragment->DepthTestEnabled) {
        Temp = (pPerFragment->DepthTestMaskVal << 4) | ((pPerFragment->DepthTestFunction & 0x07) << 1);
        M3D_WriteReg32(M3D_DEPTH_TEST, Temp | M3D_ENABLE_VALUE);

    } else {
        M3D_WriteReg32(M3D_DEPTH_TEST, M3D_DISABLE_VALUE);
    }
    M3D_Native_WriteReg32(M3D_DEPTH_CLEAR, pPerFragment->DepthTestClearVal);


    if (pPerFragment->LogicTestEnabled) {
        M3D_WriteReg32(M3D_LOGIC_TEST, ((pPerFragment->LogicTestLogicOP & 0x0F) << 1) | M3D_ENABLE_VALUE);
    } else {
        M3D_WriteReg32(M3D_LOGIC_TEST, M3D_DISABLE_VALUE);
    }
}

static void UpdatePolygonx(PolygonxStruct *pPolygonx)
{
    if (pPolygonx->OffsetFill) {
        /// INT32 temp = ((INT32)pPolygonx->OffsetUnits / 65536);

        M3D_WriteReg32(M3D_POLYGON_OFFSET_ENABLE, pPolygonx->OffsetFill);

        // Rey: The r constant of HW is 2^-11.
        /// M3D_Native_WriteReg32(M3D_POLYGON_UNITS, pPolygonx->OffsetUnits/ 65536);
        M3D_Native_WriteReg32(M3D_POLYGON_OFFSET_UNITS, pPolygonx->OffsetUnits);
        M3D_Native_WriteReg32(M3D_POLYGON_OFFSET_FACTOR, pPolygonx->OffsetFactor);
    } else {
        M3D_WriteReg32(M3D_POLYGON_OFFSET_ENABLE, pPolygonx->OffsetFill);
    }
}

static void UpdateTexDrawOES(
    M3DContextStruct *pCtx,
    TexDrawOESStruct *pTexDrawOES)
{
#ifdef M3D_PROFILE
    struct timeval kTime;
#endif    
    //UINT32 index;
    //GLuint m3d_tex_op_reg = 0;
    INT32 i_x = pTexDrawOES->i_x;
    INT32 i_y = pTexDrawOES->i_y;
    INT32 z = pTexDrawOES->z;
    //UINT32 texEnabled = pTexDrawOES->TexEnabled;
    UINT32 i_new_width  = pTexDrawOES->i_new_width;
    UINT32 i_new_height = pTexDrawOES->i_new_height;

    UINT32 output = pTexDrawOES->output;

    INT32 s = pTexDrawOES->s;
    INT32 t = pTexDrawOES->t;
    UINT32 w_over_width  = pTexDrawOES->w_over_width;
    UINT32 h_over_height = pTexDrawOES->h_over_height;

#if 1//Kevin
    UINT32 tex_op_backup = M3D_ReadReg32(M3D_TEX_OP_0);      
    UINT32 tex_src_opd_backup = M3D_ReadReg32(M3D_TEX_SRC_OPD_0);     
#endif
    UINT32 scissor_enable = 0;
    UINT32 scissor_left_backup = 0;
    UINT32 scissor_bottom_backup = 0;
    UINT32 scissor_right_backup = 0;
    UINT32 scissor_top_backup = 0;
    UINT32 drawtex_width_hright_backup = 0;

#if 1//Kevin
    //force texture env setting to use texture 0      
    M3D_WriteReg32(M3D_TEX_OP_0, 0x21);      
    M3D_WriteReg32(M3D_TEX_SRC_OPD_0, 0x2A000000);
#else
    for (index = 0; index < MAX_TEXTURE_UNITS; index++)
    {
        if (texEnabled  & (1 << index))
        {
            /// patch, to fixed the error (DUMA00121500)
            m3d_tex_op_reg = M3D_ReadReg32(M3D_TEX_OP(index));
            m3d_tex_op_reg = 0x21;
            M3D_WriteReg32(M3D_TEX_OP(index), m3d_tex_op_reg);
        }
    }
#endif

#ifndef M3D_DRAWTEX_BY_DRAWARRAY        
{
    UINT32 value = M3D_ReadReg32(M3D_TEX_CTRL);
    //if ((value & 0xFF) == 0x33)
    if ((value & 0xFF) == 0x66)
    {
    //printk("=============UpdateTexDrawOES: workaround draw texture with 2 texture=============\n");
        
        value &= ~0x000000FF;
        value |= 0x00000036;
        //value |= 0x00000022;
        M3D_WriteReg32(M3D_TEX_CTRL, value);

      //force texture env setting to use texture 0      
      M3D_WriteReg32(M3D_TEX_OP_1, 0x21);      
      M3D_WriteReg32(M3D_TEX_SRC_OPD_1, 0x2A000000);

  //printk("=============UpdateTexDrawOES: M3D_TEX_ENV_COLOR_A(1) = %x=============\n", M3D_ReadReg32(M3D_TEX_ENV_COLOR_A(1)));
  //printk("=============UpdateTexDrawOES: M3D_TEX_ENV_COLOR_A(2) = %x=============\n", M3D_ReadReg32(M3D_TEX_ENV_COLOR_A(2)));
    }
}
#endif

      
      M3D_WriteReg32(M3D_DRAWTEX_XY, i_x | (i_y << 10));
      M3D_WriteReg32(M3D_DRAWTEX_WH, i_new_width | (i_new_height << 10));


      if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
      {
         M3D_WriteReg32(M3D_DRAWTEX_Z, z);
         M3D_WriteReg32(M3D_DRAWTEX_FOG, output);
         
         M3D_WriteReg32(M3D_DRAWTEX_CRU_0, s);
         M3D_WriteReg32(M3D_DRAWTEX_CRV_0, t);
         M3D_WriteReg32(M3D_DRAWTEX_DCRU_0, w_over_width);
         M3D_WriteReg32(M3D_DRAWTEX_DCRV_0, h_over_height);
      }
      else
      {
         M3D_FLT_WriteReg32(M3D_DRAWTEX_Z, z);
         M3D_FLT_WriteReg32(M3D_DRAWTEX_FOG, output);
            
         M3D_FLT_WriteReg32(M3D_DRAWTEX_CRU_0, s);
         M3D_FLT_WriteReg32(M3D_DRAWTEX_CRV_0, t);
         M3D_FLT_WriteReg32(M3D_DRAWTEX_DCRU_0, w_over_width);
         M3D_FLT_WriteReg32(M3D_DRAWTEX_DCRV_0, h_over_height);
      }      

#if 1   //set cross region if drawtex with scissor enabled
{   //modify the drawtex region to the scissor cropped
    scissor_enable = M3D_ReadReg32(M3D_SCISSOR_ENABLE);
    if (0x1 & scissor_enable)
    {
        UINT32 draw_right = i_x + i_new_width;
        UINT32 draw_top = i_y + i_new_height;
        UINT32 new_scissor_left = 0;
        UINT32 new_scissor_bottom = 0;
        UINT32 new_scissor_right = 0;
        UINT32 new_scissor_top = 0;

//printk("=============UpdateTexDrawOES with scissor=============\n");

        scissor_left_backup = M3D_ReadReg32(M3D_SCISSOR_LEFT);
        scissor_bottom_backup = M3D_ReadReg32(M3D_SCISSOR_BOTTOM);
        scissor_right_backup = M3D_ReadReg32(M3D_SCISSOR_RIGHT);
        scissor_top_backup = M3D_ReadReg32(M3D_SCISSOR_TOP);
        drawtex_width_hright_backup = M3D_ReadReg32(M3D_DRAWTEX_WH);
        
        new_scissor_left = (scissor_left_backup > i_x) ? scissor_left_backup : i_x;
        new_scissor_bottom = (scissor_bottom_backup > i_y) ? scissor_bottom_backup : i_y;
        new_scissor_right = (scissor_right_backup < draw_right) ? scissor_right_backup : draw_right;
        new_scissor_top = (scissor_top_backup < draw_top) ? scissor_top_backup : draw_top;
        //M3D_WriteReg32(M3D_SCISSOR_LEFT, new_scissor_left);
        //M3D_WriteReg32(M3D_SCISSOR_BOTTOM, new_scissor_bottom);
        //M3D_WriteReg32(M3D_SCISSOR_RIGHT,  new_scissor_right);
        //M3D_WriteReg32(M3D_SCISSOR_TOP, new_scissor_top);

        M3D_WriteReg32(M3D_DRAWTEX_WH, (new_scissor_right - new_scissor_left) | ((new_scissor_top - new_scissor_bottom) << 10));
    }
}
#endif

    if (pTexDrawOES->enable_trigger == TRUE)
    {
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        if(!CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){
#endif
#ifdef __M3D_INTERRUPT__
        init_completion(&gCompRenderDone);
        M3D_WriteReg32NoBackup(M3D_TRIGGER, M3D_DRAWTEX_ENABLE | M3D_TRIGGER_BIT);
        DbgPrt2Time(kTime,"UpdateTexDrawOES: wait hw render done\n");
        HWTimeBegin(pCtx);
#ifndef M3D_HW_DEBUG_HANG
        wait_for_completion(&gCompRenderDone);
#else
        if(!wait_for_completion_timeout(&gCompRenderDone, 5000)){
            // timeout
            printk(KERN_ERR"UpdateTexDrawOES HW Hang\n");
            M3D_HWReg_Dump(pCtx);
        }
#endif
        HWTimeEnd(pCtx);
        DbgPrt2Time(kTime,"UpdateTexDrawOES: hw render done\n");
#else
        M3D_WriteReg32(M3D_TRIGGER, M3D_DRAWTEX_ENABLE | M3D_TRIGGER_BIT);
        WAIT_M3D();
#endif
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        }
#endif

          //SetEvent(m_hEventForMultiUser);	//Kevin
    }

#if 1   //set cross region if drawtex with scissor enabled
if (0x1 & scissor_enable)
{       //restore register
        //M3D_WriteReg32(M3D_SCISSOR_LEFT, scissor_left_backup);
        //M3D_WriteReg32(M3D_SCISSOR_BOTTOM, scissor_bottom_backup);
        //M3D_WriteReg32(M3D_SCISSOR_RIGHT, scissor_right_backup);
        //M3D_WriteReg32(M3D_SCISSOR_TOP, scissor_top_backup);

        M3D_WriteReg32(M3D_DRAWTEX_WH, drawtex_width_hright_backup);
}
#endif

      //recover texture env setting      
      M3D_WriteReg32(M3D_TEX_OP_0, tex_op_backup);      
      M3D_WriteReg32(M3D_TEX_SRC_OPD_0, tex_src_opd_backup);      
      
      //m_hM3DGLESIPCInfo.CurrentIndexCounter[m_CurrentClientIndex]++;	//Kevin
}

static void UpdateCullMode(
    M3DContextStruct    *pCtx,
    CullModexStruct     *pCullModex)
{
    UINT32 m3d_cull_reg;

    m3d_cull_reg = M3D_ReadReg32(M3D_CULL);

    m3d_cull_reg &= ~M3D_CULL_FACE_MASK;

    if (pCullModex->FrontFace == GL_CCW) {
        m3d_cull_reg |= (M3D_CCW << M3D_FRONT_FACE_BIT_OFFSET);;
    }

    pCtx->CullModex.Enabled = pCullModex->Enabled;
    if (pCullModex->Enabled) {
        /// glEnable(GL_CULL_FACE);
        pCtx->CullModex.CullMode = pCullModex->CullMode;
        switch(pCullModex->CullMode) {
            case GL_FRONT:
                m3d_cull_reg |= (M3D_CULL_FRONT << M3D_CULL_FACE_BIT_OFFSET);;
                break;
            case GL_BACK:
                m3d_cull_reg |= (M3D_CULL_BACK << M3D_CULL_FACE_BIT_OFFSET);;
                break;
            case GL_FRONT_AND_BACK:
                m3d_cull_reg |= (M3D_CULL_FRONT_AND_BACK << M3D_CULL_FACE_BIT_OFFSET);;
                break;
            default:
                ASSERT(0);
                return;
        }
    } else {
        m3d_cull_reg |= (M3D_CULL_NONE << M3D_CULL_FACE_BIT_OFFSET);;
    }

    M3D_WriteReg32(M3D_CULL, m3d_cull_reg);
}

static void UpdateStencil(StencilStruct *pStencil)
{
    UINT32 Temp;

    if (pStencil->Enabled) {
        Temp = M3D_ENABLE_VALUE;
        Temp |= ((pStencil->Function & 0x07) << 1);
        Temp |= (pStencil->Reference << 4);
        Temp |= (pStencil->ValueMask << 12);

        switch(pStencil->FailFunc) {
            case GL_KEEP:
            case GL_REPLACE:
            case GL_INCR:
            case GL_DECR:
                Temp |= ((pStencil->FailFunc & 0x07) << 20);
                break;
            case GL_ZERO:
                Temp |= (4 << 20);
                break;
            case GL_INVERT:
                Temp |= (5 << 20);
                break;
            default:
                ASSERT(0);
                return;
        }

        switch(pStencil->ZFailFunc) {
            case GL_KEEP:
            case GL_REPLACE:
            case GL_INCR:
            case GL_DECR:
                Temp |= ((pStencil->ZFailFunc & 0x07) << 23);
                break;
            case GL_ZERO:
                Temp |= (4 << 23);
                break;
            case GL_INVERT:
                Temp |= (5 << 23);
                break;
            default:
                ASSERT(0);
                return;
        }

        switch(pStencil->ZPassFunc) {
            case GL_KEEP:
            case GL_REPLACE:
            case GL_INCR:
            case GL_DECR:
                Temp |= ((pStencil->ZPassFunc & 0x07) << 26);
                break;
            case GL_ZERO:
                Temp |= (4 << 26);
                break;
            case GL_INVERT:
                Temp |= (5 << 26);
                break;
            default:
                ASSERT(0);
                return;
        }

        M3D_WriteReg32(M3D_STENCIL_MASK, pStencil->WriteMask);

        M3D_WriteReg32(M3D_STENCIL_TEST, Temp);
    } else {
        M3D_WriteReg32(M3D_STENCIL_TEST, M3D_DISABLE_VALUE);
    }
}

static void UpdateArrayInfo(ArrayInfoStruct *pArrayInfo)
{
    register UINT32 i;
    INT32 Size     ;
    UINT32 Type    ;
    UINT32 Pointer ;
    UINT32 Stride  ;
    GLuint m3d_tex_coord;
    UINT32 ArrayEnabled = pArrayInfo->ArrayEnabled;    

    for (i = 0; i < MAX_TEXTURE_UNITS; i++)
    {
       if(ArrayEnabled & _TEXTURE_ARRAY_BIT(i))
       {
            struct gl_client_array *pClientArray = &pArrayInfo->TexCoordArray[i];

            m3d_tex_coord = M3D_ReadReg32(M3D_TEX_COORD(i));
            Type = pClientArray->Type;
            Size = pClientArray->Size;
            Stride = pClientArray->Stride;
            Pointer = pClientArray->Pointer;

            if (0 == Stride)
            {
                switch (Type) {
                     case GL_BYTE:
                         Stride = Size * sizeof(GLbyte);
                         break;
                     case GL_SHORT:
                         Stride = Size * sizeof(GLshort);
                         break;
                     case GL_FLOAT:
                         Stride = Size * sizeof(GLfloat);
                         break;
                     case GL_FIXED:
                         Stride = Size * sizeof(GLfixed);
                         break;
                     default:
                         ASSERT(0);
                         return;
                }
            }

            m3d_tex_coord &= (3 << 4);
            m3d_tex_coord |= ((Type & 0x0F) << 9) | ((Size - 1) << 7) | 0x3;

            M3D_WriteReg32(M3D_TEX_COORD(i), m3d_tex_coord);
            M3D_WriteReg32(M3D_TEX_COORD_STRIDE_B(i), Stride);
            M3D_WriteReg32(M3D_TEX_COORD_POINTER(i), Pointer);
       }
       else
       {
            M3D_WriteReg32(M3D_TEX_COORD(i), 0x02);
       }
    }

    if (ArrayEnabled & _VERTEX_ARRAY_POINTER)
    {
        struct gl_client_array *pClientArray = &pArrayInfo->VertexArray;
        Type = pClientArray->Type;
        Size = pClientArray->Size;
        Stride = pClientArray->Stride;
        Pointer = pClientArray->Pointer;

        if (0 == Stride)
        {
            switch (Type) {
             case GL_BYTE:
                 Stride = Size * sizeof(GLbyte);
                 break;
             case GL_SHORT:
                 Stride = Size * sizeof(GLshort);
                 break;
             case GL_FLOAT:
                 Stride = Size * sizeof(GLfloat);
                 break;
             case GL_FIXED:
                 Stride = Size * sizeof(GLfixed);
                 break;
             default:
                 ASSERT(0);
                 return;
            }
        }
        M3D_WriteReg32(M3D_VERTEX, ((Type & 0x0F) << 2) | (Size - 2));
        M3D_WriteReg32(M3D_VERTEX_STRIDE_B, Stride);
        M3D_WriteReg32(M3D_VERTEX_BUFFER, Pointer);
    }

    if (ArrayEnabled & _NORMAL_ARRAY_POINTER)
    {
        struct gl_client_array *pClientArray = &pArrayInfo->NormalArray;

        Type = pClientArray->Type;
        Stride = pClientArray->Stride;
        Pointer = pClientArray->Pointer;

        if (pClientArray->Enable == TRUE)
        {
            if (0 == Stride)
            {
                switch(Type) {
                    case GL_BYTE:
                        Stride = 3 * sizeof(GLbyte);
                        break;
                    case GL_SHORT:
                        Stride = 3 * sizeof(GLshort);
                        break;
                    case GL_FLOAT:
                        Stride = 3 * sizeof(GLfloat);
                        break;
                    case GL_FIXED:
                        Stride = 3 * sizeof(GLfixed);
                        break;
                    default:
                        ASSERT(0);
                        return;
                }
            }
            M3D_WriteReg32(M3D_NORMAL_TYPE, Type & 0x000F);
            M3D_WriteReg32(M3D_NORMAL_STRIDE_B, Stride);
            M3D_WriteReg32(M3D_NORMAL_POINTER, Pointer);
        }
        else
        {
            M3D_WriteReg32(M3D_NORMAL_TYPE, Type & 0x000F);
            M3D_WriteReg32(M3D_NORMAL_STRIDE_B, 0);
            M3D_WriteReg32(M3D_NORMAL_POINTER, Pointer);
        }
    }

    if (ArrayEnabled & _POINT_ARRAY_POINTER)
    {
        struct gl_client_array *pClientArray = &pArrayInfo->PointSizeArray;
        Type = pClientArray->Type;
        Stride = pClientArray->Stride;
        Pointer = pClientArray->Pointer;

        if (0 == Stride)
        {
             switch (Type) {
             case GL_FLOAT:
                Stride = sizeof(GLfloat);
                break;
             case GL_FIXED:
                Stride = sizeof(GLfixed);
                break;
             default:
                ASSERT(0);
                return;
             }
        }

         M3D_WriteReg32(M3D_PNT_SIZE_INPUT, ((Type & 0x0F) << 1) |
                                          M3D_PNT_SIZE_INPUT_POINTER);

         M3D_WriteReg32(M3D_PNT_SIZE_STRIDE_B, Stride);
         M3D_WriteReg32(M3D_PNT_SIZE_POINTER, Pointer);
    }


    if (ArrayEnabled & _COLOR_ARRAY_POINTER)
    {
        struct gl_client_array *pClientArray = &pArrayInfo->ColorArray;
        Type = pClientArray->Type;
        Size = pClientArray->Size;
        Stride = pClientArray->Stride;
        Pointer = pClientArray->Pointer;

        if (0 == Stride)
        {
            switch (Type) {
                case GL_UNSIGNED_BYTE:
                    Stride = Size * sizeof(GLubyte);
                    break;
                case GL_FLOAT:
                    Stride = Size * sizeof(GLfloat);
                    break;
                case GL_FIXED:
                    Stride = Size * sizeof(GLfixed);
                    break;
                default:
                    ASSERT(0);
                    return;
            }
        }

       M3D_WriteReg32(M3D_COLOR_1, M3D_DISABLE);

       M3D_WriteReg32(M3D_COLOR_0, ((Type & 0x0F) << 4) |
                                ((Size - 3) << 3) |
                                (M3D_COLOR_ORDER_ABGR << 2)    |
                                (M3D_COLOR_INPUT_POINTER << 1) |
                                M3D_ENABLE);

       M3D_WriteReg32(M3D_COLOR_STRIDE_B_0, Stride);
       M3D_WriteReg32(M3D_COLOR_POINTER_0, Pointer);
    }
    else
    {
       M3D_WriteReg32(M3D_COLOR_0, M3D_ENABLE);
    }
}

#ifdef __M3D_INTERRUPT__
static void NewDrawCommand(
    M3DContextStruct    *pCtx,
    DrawEventStruct     *pDrawEvent)
{
#ifdef M3D_PROFILE
    struct timeval kTime;
#endif
    int drawState;    
    GLenum currentPrimitiveMode;
    UINT32 first = pDrawEvent->First;
    UINT32 Count = pDrawEvent->Count;
    UINT32  PolyCullMode = pCtx->CullModex.CullMode;
    BOOL    bTwoSideLight = pDrawEvent->TwoSideLight;    
    BOOL    bCullModeEnable = pCtx->CullModex.Enabled;            
    BOOL    bLightingEnable = pCtx->LightCtrl.LightingCtrl;
        
    switch(pDrawEvent->Mode)
    {
    case GL_POINTS:
        M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_POINTS);
        break;
    case GL_LINES:
        M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_LINES);
        break;
    case GL_LINE_LOOP:
        M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_LOOP);
        break;
    case GL_LINE_STRIP:
        M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_STRIP);
        break;
    case GL_TRIANGLES:
        M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_TRIANGLES);
        break;
    case GL_TRIANGLE_STRIP:
        M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_TRI_STRIP0);
        break;
    case GL_TRIANGLE_FAN:
        M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_TRI_FAN0);
        break;
    default:
        ASSERT(0);
        return;
    }

    switch(pDrawEvent->OPCode) 
    {
    case NEW_DRAW_ARRAY:
        M3D_WriteReg32(M3D_DRAW_MODE_CTRL, M3D_DRAW_MODE_ARRAY);
        M3D_WriteReg32(M3D_DRAW_ARRAY_FIRST, first);
        M3D_WriteReg32(M3D_DRAW_ARRAY_END, first + Count - 1);
        M3D_WriteReg32(M3D_DRAW_TOTAL_COUNT, Count);
        break;
    case NEW_DRAW_ELEMENT:
        M3D_WriteReg32(M3D_DRAW_MODE_CTRL, M3D_DRAW_MODE_ELEMENT);
        M3D_WriteReg32(M3D_DRAW_TOTAL_COUNT, Count);
        if (pDrawEvent->Type == GL_UNSIGNED_BYTE) {
            M3D_WriteReg32(M3D_DRAW_ELEMENT_TYPE, M3D_DRAW_ELEMENT_BYTE);
        } else {
            M3D_WriteReg32(M3D_DRAW_ELEMENT_TYPE, M3D_DRAW_ELEMENT_SHORT);
        }
        M3D_WriteReg32(M3D_DRAW_ELEMENT_BUFFER, pDrawEvent->IndicesPA);
        break;
    default:
        ASSERT(0);
        return;
    }

    currentPrimitiveMode = M3D_ReadReg32(M3D_PRIMITIVE_MODE);

    switch(currentPrimitiveMode)
    {
    case M3D_TRIANGLES:
    case M3D_TRIANGLE_STRIP_0:
    case M3D_TRIANGLE_FAN_0:
        /// special handling for two-side lighting
        drawState = 1;
        if (bCullModeEnable && PolyCullMode == GL_FRONT_AND_BACK) {
            drawState = 0;
        } else if (bLightingEnable && bTwoSideLight) {
            if (bCullModeEnable) {
                if (PolyCullMode == GL_BACK) {
                    drawState = 1;
                } else if (PolyCullMode == GL_FRONT) {
                    drawState = 2;
                }
            } else {
                drawState = 3;
            }
        }
        if (drawState)
        {
            if (drawState == 1) {
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        if(!CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){
#endif
                /// waitm3d = GetTickCount();
                init_completion(&gCompRenderDone);
                M3D_WriteReg32NoBackup(M3D_TRIGGER, M3D_TRIGGER_BIT);   
                DbgPrt2Time(kTime,"NewDrawCommand1: wait hw render done\n");
                HWTimeBegin(pCtx);
#ifndef M3D_HW_DEBUG_HANG
                wait_for_completion(&gCompRenderDone);
#else
                if(!wait_for_completion_timeout(&gCompRenderDone, 5000)){
                    // timeout
                    printk(KERN_ERR"NewDrawCommand HW Hang\n");
                    M3D_HWReg_Dump(pCtx);
                }
#endif
                HWTimeEnd(pCtx);
                DbgPrt2Time(kTime,"NewDrawCommand1: hw render done\n");
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        }
#endif                
            } else {
                unsigned int m3d_cull_reg;
                unsigned int normal_scale_enable = M3D_ReadReg32(M3D_NORMAL_SCALE_ENABLE);
                unsigned int normal_scale = M3D_ReadReg32(M3D_NORMAL_SCALE);
                GLnative normal_neg_one;

                if (drawState == 3) {
                    /// first, cull back face and draw
                    m3d_cull_reg = M3D_ReadReg32(M3D_CULL);
                    m3d_cull_reg &= ~(M3D_CULL_NONE << M3D_CULL_FACE_BIT_OFFSET);
                    m3d_cull_reg |= (M3D_CULL_BACK << M3D_CULL_FACE_BIT_OFFSET);
                    M3D_WriteReg32(M3D_CULL, m3d_cull_reg);
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        if(!CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){
#endif
                    M3D_WriteReg32NoBackup(M3D_TRIGGER, M3D_TRIGGER_BIT);
                    WAIT_M3D();
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        }
#endif
                }
                /// second, cull back face and reverse normals
                M3D_WriteReg32(M3D_NORMAL_SCALE_ENABLE, M3D_ENABLE);

                #define _GL_NEG_ONE ((GLfixed)0xffff0000)   /* S15.16 -1.0 */

                if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
                {
                    normal_neg_one = (GLnative)_GL_NEG_ONE;
                }
                else
                {
                    normal_neg_one = (GLnative)(-1.0f);
                }
                M3D_Native_WriteReg32(M3D_NORMAL_SCALE, normal_neg_one);
                m3d_cull_reg = M3D_ReadReg32(M3D_CULL);
                m3d_cull_reg &= ~(M3D_CULL_NONE << M3D_CULL_FACE_BIT_OFFSET);
                M3D_WriteReg32(M3D_CULL, m3d_cull_reg);
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        if(!CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){
#endif
                M3D_WriteReg32NoBackup(M3D_TRIGGER, M3D_TRIGGER_BIT);
                WAIT_M3D();
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        }
#endif
                /// restore the state
                m3d_cull_reg = M3D_ReadReg32(M3D_CULL);
                m3d_cull_reg |= (M3D_CULL_NONE << M3D_CULL_FACE_BIT_OFFSET);
                M3D_WriteReg32(M3D_CULL, m3d_cull_reg);
                M3D_WriteReg32(M3D_NORMAL_SCALE_ENABLE, normal_scale_enable);
                M3D_WriteReg32(M3D_NORMAL_SCALE, normal_scale);
            }
        }
        break;

    default:
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        if(!CHECK_PROC_DEBUG(pCtx, M3D_DEBUG_PROFILE_PERFORMANCE)){
#endif
        init_completion(&gCompRenderDone);
        M3D_WriteReg32NoBackup(M3D_TRIGGER, M3D_TRIGGER_BIT);        
        DbgPrt2Time(kTime,"NewDrawCommand2: wait hw render done\n");
        HWTimeBegin(pCtx);        
#ifndef M3D_HW_DEBUG_HANG
        wait_for_completion(&gCompRenderDone);
#else
        if(!wait_for_completion_timeout(&gCompRenderDone, 5000)){
            // timeout
            printk(KERN_ERR"NewDrawCommand HW Hang\n");
            M3D_HWReg_Dump(pCtx);
        }
#endif
        HWTimeEnd(pCtx);
        DbgPrt2Time(kTime,"NewDrawCommand2: hw render done\n");
#ifdef M3D_PROC_PROFILE_DISABLE_TRIGGER
        }
#endif
        break;
    }
}
#else
void NewDrawCommand(DrawEventStruct *pDrawEvent)
{
    GLenum currentPrimitiveMode;
    UINT32 first = pDrawEvent->First;
    UINT32 Count = pDrawEvent->Count;
    
    g_TwoSideLight = pDrawEvent->TwoSideLight;        
        
    switch(pDrawEvent->Mode)
    {
        case GL_POINTS:
            M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_POINTS);
            break;
        case GL_LINES:
            M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_LINES);
            break;
        case GL_LINE_LOOP:
            M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_LOOP);
            break;
        case GL_LINE_STRIP:
            M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_STRIP);
            break;
        case GL_TRIANGLES:
            M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_TRIANGLES);
            break;
        case GL_TRIANGLE_STRIP:
            M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_TRI_STRIP0);
            break;
        case GL_TRIANGLE_FAN:
            M3D_WriteReg32(M3D_PRIMITIVE_MODE, M3D_PRIMITIVE_TRI_FAN0);
            break;
        default:
            ASSERT(0);
            return;
    }

    switch(pDrawEvent->OPCode) {
        case NEW_DRAW_ARRAY:
            M3D_WriteReg32(M3D_DRAW_MODE_CTRL, M3D_DRAW_MODE_ARRAY);
            M3D_WriteReg32(M3D_DRAW_ARRAY_FIRST, first);
            M3D_WriteReg32(M3D_DRAW_ARRAY_END, first + Count - 1);
            M3D_WriteReg32(M3D_DRAW_TOTAL_COUNT, Count);
            break;
        case NEW_DRAW_ELEMENT:
            M3D_WriteReg32(M3D_DRAW_MODE_CTRL, M3D_DRAW_MODE_ELEMENT);
            M3D_WriteReg32(M3D_DRAW_TOTAL_COUNT, Count);
            if (pDrawEvent->Type == GL_UNSIGNED_BYTE) {
                M3D_WriteReg32(M3D_DRAW_ELEMENT_TYPE, M3D_DRAW_ELEMENT_BYTE);
            } else {
                M3D_WriteReg32(M3D_DRAW_ELEMENT_TYPE, M3D_DRAW_ELEMENT_SHORT);
            }
            M3D_WriteReg32(M3D_DRAW_ELEMENT_BUFFER, pDrawEvent->IndicesPA);
            break;
        default:
            ASSERT(0);
            return;
    }

    currentPrimitiveMode = M3D_ReadReg32(M3D_PRIMITIVE_MODE);

    switch(currentPrimitiveMode)
    {
      case M3D_TRIANGLES:
      case M3D_TRIANGLE_STRIP_0:
      case M3D_TRIANGLE_FAN_0:

         {  /// special handling for two-side lighting
             int drawState = 1;

             if (g_CullModeEnable && g_PolyCullMode == GL_FRONT_AND_BACK) {
                drawState = 0;
             } else if (g_LightingEnable && g_TwoSideLight) {
                if (g_CullModeEnable) {
                   if (g_PolyCullMode == GL_BACK) {
                      drawState = 1;
                   } else if (g_PolyCullMode == GL_FRONT) {
                      drawState = 2;
                   }
                } else {
                   drawState = 3;
                }
             }

             if (drawState)
             {
                if (drawState == 1) {
                   /// waitm3d = GetTickCount();
                   M3D_WriteReg32(M3D_TRIGGER, M3D_TRIGGER_BIT);
//#ifdef _MT6516_GLES_PROFILE_	//Kevin for profile
#if 0
                   do_gettimeofday(&t1);
                   WAIT_M3D();	//Kevin
                   do_gettimeofday(&t2);
                   t1_t2 += (t2.tv_usec - t1.tv_usec);
#endif                   
                } else {
                   unsigned int m3d_cull_reg;
                   unsigned int normal_scale_enable = M3D_ReadReg32(M3D_NORMAL_SCALE_ENABLE);
                   unsigned int normal_scale = M3D_ReadReg32(M3D_NORMAL_SCALE);
                   GLnative normal_neg_one;

                   if (drawState == 3) {
                      /// first, cull back face and draw
                      m3d_cull_reg = M3D_ReadReg32(M3D_CULL);
                      m3d_cull_reg &= ~(M3D_CULL_NONE << M3D_CULL_FACE_BIT_OFFSET);
                      m3d_cull_reg |= (M3D_CULL_BACK << M3D_CULL_FACE_BIT_OFFSET);
                      M3D_WriteReg32(M3D_CULL, m3d_cull_reg);

                      M3D_WriteReg32(M3D_TRIGGER, M3D_TRIGGER_BIT);
                      WAIT_M3D();
    printk("NewDrawCommand WAIT_M3D=============\n");
                   }

                   /// second, cull back face and reverse normals
                   M3D_WriteReg32(M3D_NORMAL_SCALE_ENABLE, M3D_ENABLE);

                  #define _GL_NEG_ONE ((GLfixed)0xffff0000)   /* S15.16 -1.0 */

                   if (__COMMON_LITE_PROFILE__ == g_GLESProfile)
                   {
                       normal_neg_one = (GLnative)_GL_NEG_ONE;
                   }
                   else
                   {
                       normal_neg_one = (GLnative)(-1.0f);
                   }

                   M3D_Native_WriteReg32(M3D_NORMAL_SCALE, normal_neg_one);

                   m3d_cull_reg = M3D_ReadReg32(M3D_CULL);
                   m3d_cull_reg &= ~(M3D_CULL_NONE << M3D_CULL_FACE_BIT_OFFSET);
                   M3D_WriteReg32(M3D_CULL, m3d_cull_reg);

                   M3D_WriteReg32(M3D_TRIGGER, M3D_TRIGGER_BIT);
                   WAIT_M3D();
    printk("NewDrawCommand WAIT_M3D=============\n");
                   /// restore the state
                   m3d_cull_reg = M3D_ReadReg32(M3D_CULL);
                   m3d_cull_reg |= (M3D_CULL_NONE << M3D_CULL_FACE_BIT_OFFSET);
                   M3D_WriteReg32(M3D_CULL, m3d_cull_reg);

                   M3D_WriteReg32(M3D_NORMAL_SCALE_ENABLE, normal_scale_enable);
                   M3D_WriteReg32(M3D_NORMAL_SCALE, normal_scale);
                }
             }
         }
         break;
      default:
         M3D_WriteReg32(M3D_TRIGGER, M3D_TRIGGER_BIT);
         break;
    }

#if !defined(_MT6516_GLES_CMQ_) || defined(_MT6516_GLES_WAIT_HW_IDLE_)	//Kevin
    WAIT_M3D();		//wait idle per list
    FlushCacheCtrl(NULL);
#endif

#if 0    //Kevin
    SetEvent(m_hM3DGLESIPCInfo.IdleEventPool[m_CurrentClientIndex]);      
    SetEvent(m_hEventForMultiUser);     
    m_hM3DGLESIPCInfo.CurrentIndexCounter[m_CurrentClientIndex]++;        
#endif    
}
#endif

#ifdef __DRV_TEAPOT_DEBUG__
static INT32 TestDriver(void)
{
    INT32   TestM3DStatus;
    UINT32  IndexBufferPA;
    UINT8*  IndexBufferVA;
    UINT32  VertexBufferPA;
    UINT8*  VertexBufferVA;
    UINT32  VertexCachePA;
    UINT8*  VertexCacheVA;
    UINT32  ColorBufferPA;
    UINT8*  ColorBufferVA;
    UINT32  FrameBufferPA;
    UINT8*  FrameBufferVA;
    UINT32  StencilBufferPA;
    UINT8*  StencilBufferVA;
    UINT32  DepthBufferPA;
    UINT8*  DepthBufferVA;
    UINT32 Index = 0;

    //VertexCacheVA   = (UINT8*)AllocPhysMem(0x000800, PAGE_READWRITE, 0, 0, (PULONG)&VertexCachePA);
    VertexCacheVA   = (UINT8*)dma_alloc_coherent(0, 0x000800, &VertexCachePA, GFP_KERNEL);

    //FrameBufferVA   = (UINT8*)AllocPhysMem(0x025800, PAGE_READWRITE, 0, 0, (PULONG)&FrameBufferPA);
    FrameBufferVA   = (UINT8*)dma_alloc_coherent(0, 0x025800, &FrameBufferPA, GFP_KERNEL);

    //StencilBufferVA = (UINT8*)AllocPhysMem(0x012C00, PAGE_READWRITE, 0, 0, (PULONG)&StencilBufferPA);
    StencilBufferVA = (UINT8*)dma_alloc_coherent(0, 0x012C00, &StencilBufferPA, GFP_KERNEL);

    //DepthBufferVA   = (UINT8*)AllocPhysMem(0x025800, PAGE_READWRITE, 0, 0, (PULONG)&DepthBufferPA);
    DepthBufferVA   = (UINT8*)dma_alloc_coherent(0, 0x025800, &DepthBufferPA, GFP_KERNEL);

    //IndexBufferVA   = (UINT8*)AllocPhysMem(sizeof(IndexTemplate),  PAGE_READWRITE, 0, 0, (PULONG)&IndexBufferPA);
    IndexBufferVA   = (UINT8*)dma_alloc_coherent(0, sizeof(IndexTemplate), &IndexBufferPA, GFP_KERNEL);

    //CeSafeCopyMemory(IndexBufferVA,  IndexTemplate,  sizeof(IndexTemplate));
    memcpy(IndexBufferVA,  IndexTemplate,  sizeof(IndexTemplate));

    //VertexBufferVA  = (UINT8*)AllocPhysMem(sizeof(VertexTemplate), PAGE_READWRITE, 0, 0, (PULONG)&VertexBufferPA);
    VertexBufferVA  = (UINT8*)dma_alloc_coherent(0, sizeof(VertexTemplate), &VertexBufferPA, GFP_KERNEL);

    //CeSafeCopyMemory(VertexBufferVA, VertexTemplate, sizeof(VertexTemplate));
    memcpy(VertexBufferVA, VertexTemplate, sizeof(VertexTemplate));

    //ColorBufferVA   = (UINT8*)AllocPhysMem(sizeof(ColorTemplate),  PAGE_READWRITE, 0, 0, (PULONG)&ColorBufferPA);
    ColorBufferVA   = (UINT8*)dma_alloc_coherent(0, sizeof(ColorTemplate), &ColorBufferPA, GFP_KERNEL);

    //CeSafeCopyMemory(ColorBufferVA,  ColorTemplate,  sizeof(ColorTemplate));
    memcpy(ColorBufferVA,  ColorTemplate,  sizeof(ColorTemplate));

    for (Index = 0; Index < 0x025800; Index++) {
      FrameBufferVA[Index] = 0x00;
        DepthBufferVA[Index] = 0xff;
    }

    M3D_WriteReg32(0x0000001b, 0x00000003);
    M3D_WriteReg32(0x0000003b, 0x0000001f);
    M3D_WriteReg32(0x0000003c, 0x00000001);
    M3D_WriteReg32(0x00000038, 0x00000000);
    M3D_WriteReg32(0x00000039, 0x00000000);
    M3D_WriteReg32(0x0000002a, 0x00000022);
    M3D_WriteReg32(0x0000002b, 0x00000022);
    M3D_WriteReg32(0x0000002c, 0x00000022);
    M3D_WriteReg32(0x0000003d, 0x00000000);
    M3D_WriteReg32(0x00000011, 0x00000000);
    M3D_WriteReg32(0x00000012, 0x00000000);
    M3D_WriteReg32(0x00000097, 0x00000002);
    M3D_WriteReg32(0x00000041, 0x07e00000);
    M3D_WriteReg32(0x0000008c, 0x00000000);
    M3D_WriteReg32(0x00000090, 0x00000042);
    M3D_WriteReg32(0x00000094, 0x2a0982c1);
    M3D_WriteReg32(0x0000008d, 0x00000000);
    M3D_WriteReg32(0x00000091, 0x00000042);
    M3D_WriteReg32(0x00000095, 0x2a0982c1);
    M3D_WriteReg32(0x0000008e, 0x00000000);
    M3D_WriteReg32(0x00000092, 0x00000042);
    M3D_WriteReg32(0x00000096, 0x2a0982c1);
    M3D_WriteReg32(0x0000001a, VertexCachePA);
    M3D_WriteReg32(0x0000003a, 0x00000000);
    M3D_WriteReg32(0x0000003b, 0x0000ffff);
    M3D_WriteReg32(0x00000071, FrameBufferPA);
    M3D_WriteReg32(0x00000070, 0x00000002);
    M3D_WriteReg32(0x00000072, 0x000000f0);
    M3D_WriteReg32(0x00000073, 0x00000140);
    M3D_WriteReg32(0x00000025, 0x00000000);
    M3D_WriteReg32(0x00000076, StencilBufferPA);
    M3D_WriteReg32(0x00000075, DepthBufferPA);
    M3D_WriteReg32(0x00000007, 0x00000007);
    M3D_WriteReg32(0x00000003, 0x00000001);
    M3D_WriteReg32(0x00000071, FrameBufferPA);
    M3D_WriteReg32(0x00000025, 0x00000000);
    M3D_WriteReg32(0x000000af, 0x00010000);
    M3D_WriteReg32(0x000000ae, 0x00000000);
    M3D_WriteReg32(0x000000ad, 0x00000000);
    M3D_WriteReg32(0x000000ac, 0x00000000);
    M3D_WriteReg32(0x000000ab, 0x00000000);
    M3D_WriteReg32(0x000000aa, 0x00010000);
    M3D_WriteReg32(0x000000a9, 0x00000000);
    M3D_WriteReg32(0x000000a8, 0x00000000);
    M3D_WriteReg32(0x000000a7, 0x00000000);
    M3D_WriteReg32(0x000000a6, 0x00000000);
    M3D_WriteReg32(0x000000a5, 0x00010000);
    M3D_WriteReg32(0x000000a4, 0x00000000);
    M3D_WriteReg32(0x000000a3, 0x00000000);
    M3D_WriteReg32(0x000000a2, 0x00000000);
    M3D_WriteReg32(0x000000a1, 0x00000000);
    M3D_WriteReg32(0x000000a0, 0x00010000);
    M3D_WriteReg32(0x00000100, 0x00010000);
    M3D_WriteReg32(0x00000101, 0x00000000);
    M3D_WriteReg32(0x00000102, 0x00000000);
    M3D_WriteReg32(0x00000103, 0x00000000);
    M3D_WriteReg32(0x00000104, 0x00010000);
    M3D_WriteReg32(0x00000105, 0x00000000);
    M3D_WriteReg32(0x00000106, 0x00000000);
    M3D_WriteReg32(0x00000107, 0x00000000);
    M3D_WriteReg32(0x00000108, 0x00010000);
    M3D_WriteReg32(0x0000009e, 0x00000000);
    M3D_WriteReg32(0x0000009f, 0x00010000);
    M3D_WriteReg32(0x000000b0, 0x01d36ac0);
    M3D_WriteReg32(0x000000b1, 0x00000000);
    M3D_WriteReg32(0x000000b2, 0x00000000);
    M3D_WriteReg32(0x000000b3, 0x00000000);
    M3D_WriteReg32(0x000000b4, 0x00000000);
    M3D_WriteReg32(0x000000b5, 0x01d36ac0);
    M3D_WriteReg32(0x000000b6, 0x00000000);
    M3D_WriteReg32(0x000000b7, 0x00000000);
    M3D_WriteReg32(0x000000b8, 0xff888000);
    M3D_WriteReg32(0x000000b9, 0xff608000);
    M3D_WriteReg32(0x000000ba, 0xfffefff9);
    M3D_WriteReg32(0x000000bb, 0xffff0000);
    M3D_WriteReg32(0x000000bc, 0x00000000);
    M3D_WriteReg32(0x000000bd, 0x00000000);
    M3D_WriteReg32(0x000000be, 0xffffe667);
    M3D_WriteReg32(0x000000bf, 0x00000000);
    M3D_WriteReg32(0x00000038, 0x00000000);
    M3D_WriteReg32(0x00000041, 0x07e00000);
    M3D_WriteReg32(0x0000001b, 0x00000003);
    M3D_WriteReg32(0x00000098, 0xffff8000);
    M3D_WriteReg32(0x00000099, 0xffff8000);
    M3D_WriteReg32(0x0000009a, 0x00ef8000);
    M3D_WriteReg32(0x0000009b, 0x013f8000);
    M3D_WriteReg32(0x00000066, 0x00000000);
    M3D_WriteReg32(0x0000006d, 0x00000000);
    M3D_WriteReg32(0x00000077, 0x00010000);
    M3D_WriteReg32(0x0000006b, 0x00000000);
    M3D_WriteReg32(0x0000006e, 0x00001e00);
    M3D_WriteReg32(0x0000006f, 0x00000000);
    M3D_WriteReg32(0x00000079, 0x00000000);
    M3D_WriteReg32(0x0000006c, 0x00000000);
    M3D_WriteReg32(0x00000078, 0x00000000);
    M3D_WriteReg32(0x0000003d, 0x00000000);
    M3D_WriteReg32(0x0000003c, 0x00001d01);
    M3D_WriteReg32(0x0000003d, 0x00000000);
    M3D_WriteReg32(0x0000003d, 0x00000000);
    M3D_WriteReg32(0x00000011, 0x00000000);
    M3D_WriteReg32(0x000001e8, 0x00010000);
    M3D_WriteReg32(0x000001e9, 0x001d0000);
    M3D_WriteReg32(0x000001ea, 0x00010000);
    M3D_WriteReg32(0x000001eb, 0x00000000);
    M3D_WriteReg32(0x000001ec, 0x00000000);
    M3D_WriteReg32(0x00000016, 0x00000000);
    M3D_WriteReg32(0x00000011, 0x00000000);
    M3D_WriteReg32(0x00000015, 0x00010000);
    M3D_WriteReg32(0x00000012, 0x00000000);
    M3D_WriteReg32(0x0000003b, 0x0000ffff);
    M3D_WriteReg32(0x00000007, 0x00000004);
    M3D_WriteReg32(0x00000007, 0x00000002);
    M3D_WriteReg32(0x000000af, 0x00010000);
    M3D_WriteReg32(0x000000ae, 0xfef710a9);
    M3D_WriteReg32(0x000000ad, 0xfffb3693);
    M3D_WriteReg32(0x000000ac, 0xfff9b42b);
    M3D_WriteReg32(0x000000ab, 0x00000000);
    M3D_WriteReg32(0x000000aa, 0xffff1635);
    M3D_WriteReg32(0x000000a9, 0x00005e3e);
    M3D_WriteReg32(0x000000a8, 0x00002c8f);
    M3D_WriteReg32(0x000000a7, 0x00000000);
    M3D_WriteReg32(0x000000a6, 0x00006032);
    M3D_WriteReg32(0x000000a5, 0x0000ed31);
    M3D_WriteReg32(0x000000a4, 0x00000302);
    M3D_WriteReg32(0x000000a3, 0x00000000);
    M3D_WriteReg32(0x000000a2, 0xffffd7d1);
    M3D_WriteReg32(0x000000a1, 0x0000137e);
    M3D_WriteReg32(0x000000a0, 0xffff03f2);
    M3D_WriteReg32(0x00000100, 0xffff03e8);
    M3D_WriteReg32(0x00000101, 0x00000302);
    M3D_WriteReg32(0x00000102, 0x00002c90);
    M3D_WriteReg32(0x00000103, 0x0000137d);
    M3D_WriteReg32(0x00000104, 0x0000ed3d);
    M3D_WriteReg32(0x00000105, 0x00005e43);
    M3D_WriteReg32(0x00000106, 0xffffd7cf);
    M3D_WriteReg32(0x00000107, 0x00006035);
    M3D_WriteReg32(0x00000108, 0xffff162d);
    M3D_WriteReg32(0x00000038, 0x00000000);
    M3D_WriteReg32(0x00000041, 0x07e00000);
    M3D_WriteReg32(0x0000006d, 0x00000017);
    M3D_WriteReg32(0x00000077, 0x00010000);
    M3D_WriteReg32(0x0000003c, 0x00001d00);
    M3D_WriteReg32(0x0000003d, 0x00000000);
    M3D_WriteReg32(0x00000012, 0x00000001);
    M3D_WriteReg32(0x00000014, 0x00000000);
    M3D_WriteReg32(0x00000013, 0x00000000);
    M3D_WriteReg32(0x0000003b, 0x0000ff3f);
    M3D_WriteReg32(0x0000000a, 0x00000008);
    M3D_WriteReg32(0x0000000b, 0x00000001);
    M3D_WriteReg32(0x0000000c, 0x00000ba0);
    M3D_WriteReg32(0x0000000f, 0x00000003);
    M3D_WriteReg32(0x00000010, IndexBufferPA);
    M3D_WriteReg32(0x00000017, 0x00000031);
    M3D_WriteReg32(0x00000018, 0x0000000c);
    M3D_WriteReg32(0x00000019, VertexBufferPA);
    M3D_WriteReg32(0x00000020, 0x00000000);
    M3D_WriteReg32(0x0000001f, 0x00000013);
    M3D_WriteReg32(0x00000021, 0x00000003);
    M3D_WriteReg32(0x00000023, ColorBufferPA);
    M3D_WriteReg32(0x00000033, 0x00000000);
    M3D_WriteReg32(0x00000034, 0x00010000);
    M3D_WriteReg32(0x00000041, 0x00000000);
    M3D_WriteReg32(0x0000002a, 0x00000000);
    M3D_WriteReg32(0x0000002b, 0x00000000);
    M3D_WriteReg32(0x0000002c, 0x00000000);

    // Trigger the 3D hardware
    M3D_WriteReg32(M3D_TRIGGER, 0x0001);

    M3D_WriteReg32(M3D_TRIGGER, 0x0000);
    M3D_WriteReg32(M3D_RESET, 0x01);
    //Sleep(1);
    msleep(1);
    M3D_WriteReg32(M3D_RESET, 0x00);

    M3D_WriteReg32(M3D_TRIGGER, 0x0001);

    while (0 != (M3D_ReadReg32(M3D_STATUS)));

    M3D_WriteReg32(0x00000006, 0x00000006);

    for (Index = 0; Index < 0x025800; Index++) {
        if (FrameTemplate[Index] != FrameBufferVA[Index]) {
            ASSERT(0);
            TestM3DStatus = -1;
            goto Done;
        }
    }

    TestM3DStatus = 0;

Done:
    //FreePhysMem(ColorBufferVA);
    dma_free_coherent(0, sizeof(ColorTemplate), ColorBufferVA, ColorBufferPA);
    //FreePhysMem(VertexBufferVA);
    dma_free_coherent(0, sizeof(VertexTemplate), VertexBufferVA, VertexBufferPA);
    //FreePhysMem(IndexBufferVA);
    dma_free_coherent(0, sizeof(IndexTemplate), IndexBufferVA, IndexBufferPA);
    //FreePhysMem(DepthBufferVA);
    dma_free_coherent(0, 0x025800, DepthBufferVA, DepthBufferPA);
    //FreePhysMem(StencilBufferVA);
    dma_free_coherent(0, 0x012C00, StencilBufferVA, StencilBufferPA);
    //FreePhysMem(FrameBufferVA);
    dma_free_coherent(0, 0x025800, FrameBufferVA, FrameBufferPA);
    //FreePhysMem(VertexCacheVA);
    dma_free_coherent(0, 0x000800, VertexCacheVA, VertexCachePA);

    return TestM3DStatus;
}
#endif /// __DRV_TEAPOT_DEBUG__
