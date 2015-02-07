

 
#ifndef __M3D_DRIVER_H__
#define __M3D_DRIVER_H__

//#include <windows.h>
//#include <ddraw.h>
#include "m3d_config.h"
#include "M3DRegs.h"
//#include "bus_exp.h"
//#include "irq_reg.h"
#include "m3d_exp.h"
//#include "SystemRam_cfg.h"
//#include "ceddk_exp.h"

/// #define DEBUG
/// #define __DRV_DEBUG_REG__
//#define __DRV_TEAPOT_DEBUG__

#ifdef __DRV_TEAPOT_DEBUG__
#include "Teapot.h"
#endif  /// __DRV_TEAPOT_DEBUG__

#define INITIAL_STATE_MARKER 0xE265163D
/// Bits per color channel
/// Fixed at 8
#define CHAN_BITS 8

#define DRV_565_R_MASK 0xF800
#define DRV_565_G_MASK 0x07E0
#define DRV_565_B_MASK 0x001F

/// Color channel component order
#define RCOMP 0
#define GCOMP 1
#define BCOMP 2
#define ACOMP 3

#define DRV_RGB_TO_565(R,G,B)   ((unsigned short)(((B)&0xF8)>>3)|(((G)&0xFC)<<3)|(((R)&0xF8)<<8) )
#define DRV_565_TO_RGB_R(VALUE)	((((VALUE)&DRV_565_R_MASK)>>8))
#define DRV_565_TO_RGB_G(VALUE)	((((VALUE)&DRV_565_G_MASK)>>3))
#define DRV_565_TO_RGB_B(VALUE)	((((VALUE)&DRV_565_B_MASK)<<3))


#define GL_ASSERT  ASSERT

/// in: [0, 0x10000]
/// out: [0, 0xFF]
#define CLAMPED_X_TO_CHAN(out, in) \
do {                               \
   if (65536 == in)            \
      out = 0xFF;                  \
   else                            \
      out = in >> 8;               \
} while (0)


#define CLAMPED_FLOAT_TO_CHAN(out, in)             \
do {                                               \
   out = ((UINT32) _GL_F_IROUND((in) * 255.0F));  \
} while (0)


#define UNCLAMPED_X_TO_CHAN(out, in) \
do {                                 \
   if (in<=0)               \
      out = 0;                       \
   else if (in>=65536)           \
      out = 0xFF;                    \
   else                              \
      out = in >> 8;                 \
} while (0)


#define UNCLAMPED_X_TO_RGBA_CHAN(dst, f)	\
do {						\
   UNCLAMPED_X_TO_CHAN(dst[0], f[0]);	\
   UNCLAMPED_X_TO_CHAN(dst[1], f[1]);	\
   UNCLAMPED_X_TO_CHAN(dst[2], f[2]);	\
   UNCLAMPED_X_TO_CHAN(dst[3], f[3]);	\
} while (0)
   
/// fixed-point mathematical computation and definition
//typedef __int64   GLint64;
typedef long long   GLint64;

#define _GL_X_FRAC_BITS                 16
#define _GL_I_2_X(a)     ((a) << _GL_X_FRAC_BITS)
#define _GL_X_SUB(a, b)  _gl_x_sub(a, b)
#define _GL_X_HALF       ((GLfixed)0x00008000)   /* S15.16  0.5 */


/* PixelFormat */
#define GL_ARGB                           0x1905  /* MTK internal use */
#define GL_ALPHA                          0x1906
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_BGRA                           0x80E1
#define GL_LUMINANCE                      0x1909
#define GL_LUMINANCE_ALPHA                0x190A

#define WAIT_M3D()  while (0 != (M3D_ReadReg32(M3D_STATUS)));
#define IS_M3D_BUSY  (M3D_ReadReg32(M3D_STATUS))

#define M3D_STATE_POWER_OFF     0x00
#define M3D_STATE_POWER_ON      0x01

#if 1

INT32 M3dInit(void);
INT32 M3dDeinit(void);
int M3DWorkerEntry(struct file *file, UINT32 condition, UINT32 arg);
void M3DOpen(struct file *file);
void M3DRelease(struct file *file);
void M3DSuspend(void);
void M3DResume(void); 
#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
int M3DProcRead(void);
int M3DProcWrite(const char* buffer, unsigned long count);
#endif
#ifdef _MT6516_GLES_CMQ_	//Kevin
#ifndef __M3D_INTERRUPT__
void M3D_thread(void);
#endif
#endif

#ifdef __DRV_TEAPOT_DEBUG__
INT32 TestDriver(void);
#endif

typedef struct M3DContextStruct
{
    UINT32 pid;
    char comm[TASK_COMM_LEN];
    UINT32 ctxHandle;
    UINT32 dirtyCondition;

#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
    UINT32 u4DebugFlag;
    UINT32 u4DebugCount;
#endif
#ifdef M3D_PROC_PROFILE
    ProfileData kProfileData;
#endif
#ifdef M3D_MULTI_CONTEXT
    struct completion JobDone;
#endif

#ifdef _MT6516_GLES_CMQ_	//Kevin
//spinlock_t m3d_thread_lock;
//unsigned long m3d_thread_lock_flag;

//BOOL m3d_leave_thread;
    CMQInfoStruct* pCMQInfo;
    UBYTE*  pCMQMemoryBase;
    UBYTE*  pCMQBufferBase;
    UBYTE*  pCMQBufferPointer;
#ifdef M3D_MULTI_CONTEXT
    UBYTE*  pHWRegData;
    UBYTE   auDirty[M3D_HW_REGDIRTY_SIZE];
    BOOL    bHWRegDataValid;
#endif
#endif
    
#ifdef _MT6516_GLES_CMQ_	//Kevin
#ifdef M3D_HW_DEBUG_HANG
    CMQStruct                   CMQCtrl;
#endif
    CMQVertexBufferStruct       CMQVertexBufferCtrl;
#ifndef __M3D_INTERRUPT__
    CMQProcessStruct		CMQProcessCtrl;
#endif
#endif
        NewContextStruct          NewContext;
        CacheCtrlStruct           CacheCtrl;
        TexMatrixStruct           TexMatrix;
        NormalizeStruct           Normalize;
        ClipPlaneStruct           ClipPlane;
        FogCtrlStruct             FogCtrl;
        LightCtrlStruct           LightCtrl;
        PrimitiveCtrlStruct       PrimitiveCtrl;
        BBViewportTransformStruct BBViewPortXformCtrl;
        TextureCtrlStruct         TextureCtrl;
        ColorCtrlStruct           ColorCtrl;
        PerFragmentTestStruct     PerFragment;
        PolygonxStruct            Polygonx;
        CullModexStruct           CullModex;
        SampleCovStruct           SampleCov;
        StencilStruct             Stencil;
        ArrayInfoStruct           ArrayInfo;
        TexDrawOESStruct          TexDrawOES;
        DrawEventStruct           DrawEvent;
        PowerEventCtrlStruct      PowerEventCtrl;       
} M3DContextStruct, *PM3DContextStruct;

#else	//mopve to .c

class M3DDriver
{
public:
    M3DDriver();
    ~M3DDriver();

    INT32 Init();
    INT32 Deinit();
    
    INT32 PowerOn();
    
    INT32 PowerOff();

    INT32 PowerQuery(PCEDEVICE_POWER_STATE pInfo);

    INT32 AddReference();

    INT32 DecReference();
    
private:
    static UINT32 WorkerEntry(M3DDriver *pDriver) {
        return pDriver->M3DWorkerEntry();
    }

    static GLuint _gl_convert_texture_src_rgba(GLenum CombineSource);
    static GLuint _gl_convert_texture_operand_rgba(GLenum CombineOperand);   
    void _gl_config_texture_info(const GLnative *ClearColor, UINT32 TextureBufferPA);
    void _gl_config_blend_info();

    __inline void M3D_WriteReg32(UINT32 ADDR, UINT32 VAL) {                   
     *((volatile UINT32*)(pM3DRegBase + (ADDR << 2))) = VAL;    

#if defined(__DRV_DEBUG_REG__)     
       m_registers[ADDR] = VAL; /// for backup register, in order to restore the M3D reg state
#endif
       
///       if ((ADDR == 0x0)  || (ADDR == 0x0006))
///       {          
///           M3D_Arch_Prof_Kernel_Config_REG_Stop();
///           M3D_Arch_Prof_Kernel_HWBusy_Start();
///       }
#if defined (__DRV_DEBUG_REG__)       
      {
        UINT32 Index;
        m_registers[ADDR] = VAL;
        if (ADDR == 0x0) {
            for (Index = 0; Index < 0x200; Index++) {
                RETAILMSG(TRUE, (TEXT("REG 0x%08x, Value 0x%08x\n"), Index * 4, m_registers[Index]));
            }
        }
      }  
#endif /// __DRV_DEBUG_REG__
    }    

    __inline void M3D_Native_WriteReg32(UINT32 ADDR, UINT32 VAL) {       
      
        if (__COMMON_LITE_PROFILE__ == m_GLESProfile) { 
             *((volatile UINT32*)(pM3DRegBase  + (ADDR << 2))) = VAL;
             
#if defined(__DRV_DEBUG_REG__)                  
             m_registers[ADDR] = VAL;
#endif             
        }
        else 
        {
             *((volatile UINT32*)(pM3DRegBase  + ((ADDR | M3D_FLOAT_INPUT_OFFSET) << 2))) = VAL;
             
#if defined(__DRV_DEBUG_REG__)                  
             m_registers[ADDR | M3D_FLOAT_INPUT_OFFSET] = VAL;
#endif             
        }  

#if defined (__DRV_DEBUG_REG__)
      {
        UINT32 Index;
        if (__COMMON_LITE_PROFILE__ == m_GLESProfile) { 
            m_registers[ADDR] = VAL;
        } else {
            m_registers[ADDR | M3D_FLOAT_INPUT_OFFSET] = VAL;
        }
        if (ADDR == 0x0) {
            for (Index = 0; Index < 0x400; Index++) {
                RETAILMSG(TRUE, (TEXT("REG 0x%08x, Value 0x%08x\n"), Index * 4, m_registers[Index]));
            }
        }
      }
#endif /// __DRV_DEBUG_REG__
        
  	 }  
  	   
    __inline void M3D_FLT_WriteReg32(UINT32 ADDR, UINT32 VAL) 
    {             
        *((volatile UINT32*)(pM3DRegBase  + ((ADDR | M3D_FLOAT_INPUT_OFFSET) << 2))) = VAL;
        
#if defined(__DRV_DEBUG_REG__)             
        m_registers[ADDR | M3D_FLOAT_INPUT_OFFSET] = VAL;
#endif        
    }  
  
          	        
    __inline UINT32 M3D_ReadReg32(UINT32 ADDR) {
        return *((volatile UINT32*)pM3DRegBase + (ADDR));
    }
    
    BOOL MapIRQ2SysIntr(DWORD Irq, DWORD *pSysIntr);
    BOOL UnMapSysIntr(DWORD dwSysIntr);

    UINT32  M3DWorkerEntry();

    void UpdateContext(NewContextStruct   *pContext  );
    void InvalCacheCtrl(CacheCtrlStruct   *pCacheCtrl);
    void FlushCacheCtrl(CacheCtrlStruct   *pCacheCtrl);
    void UpdateTexMatrix(TexMatrixStruct  *pMatrix);
    void UpdateNormalize(NormalizeStruct  *pNormalize);
    void UpdateClipPlane(ClipPlaneStruct  *pPlane);
    void UpdateFogCtrl(FogCtrlStruct      *pFogCtrl);
    void UpdateLightCtrl(LightCtrlStruct  *pLight);
    void UpdateTexture(TextureCtrlStruct *pTexCtrl);
    void UpdateColorCtrl(ColorCtrlStruct *pColorCtrl);
    void UpdatePerFragmentTest(PerFragmentTestStruct *pPerFragment);
    void UpdatePolygonx(PolygonxStruct   *pPolygonx);
    void UpdateCullMode(CullModexStruct  *pCullModex);    
    void UpdateStencil(StencilStruct     *pStencil);
    void UpdateArrayInfo(ArrayInfoStruct *pArrayInfo);
    void UpdateTexDrawOES(TexDrawOESStruct *pTexDrawOES);
    void NewDrawCommand(DrawEventStruct    *pDrawEvent);
    void UpdatePrimitiveCtrl(PrimitiveCtrlStruct *pPrimitiveCtrl);
    void UpdateBBVPTransformMatrix(BBViewportTransformStruct *pBBViewPortXformCtrl);
    void UpdatePowerCtrl(PowerEventCtrlStruct *pPowerEventCtrl);
      
#if defined(__DRV_DEBUG_REG__)           
    UINT32  m_registers[0x400];
#endif    

    HANDLE  m_hPowerCtrl;
    UINT8   *m_pCFGRegBaseEMI;
    UINT8   *m_pCFGRegBaseMEM;
    UINT8   *pM3DRegBase;
    HANDLE  m_hM3DWorker;
    DWORD   m_M3DSysIntr;
    HANDLE  m_hM3DEvent;
    UINT32  m_BufferWidth;
    UINT32  m_BufferHeight;
    HANDLE  m_hFinishEvt;    
    BOOL    m_CullModeEnable;
    BOOL    m_PolyCullMode;
    BOOL    m_TwoSideLight;
    BOOL    m_LightingEnable;
    UINT32  m_GLESProfile;
    SyncEvent m_BufferEvent;
    
    // Reference count
    CRITICAL_SECTION    m_Critical;
    INT32               m_PowerState;
    INT32               m_Reference;           
    
    /// For IPC            
    HANDLE              m_hEventForMultiUser;
    HANDLE              m_hM3DStartEvent;        
    IPCHanldeStruct     m_hM3DGLESIPCInfo;        
    UINT32              m_CurrentClientIndex;                       
    DWORD               m_PreviousPID;                 
    UINT8               *m_pTextureBufferVA;
    UINT32              m_TextureBufferPA;
};

typedef M3DDriver    *PM3D_DRIVER_CLASS;

#endif
#endif  // __M3D_DRIVER_H__
