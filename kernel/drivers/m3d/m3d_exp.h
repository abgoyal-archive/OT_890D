

 
#ifndef __M3D_EXP_H__
#define __M3D_EXP_H__

//#define NO_M3D_THREAD
//#define M3D_PROC_PROFILE_DISABLE_TRIGGER
//#define _MT6516_GLES_PROFILE_
//#define M3D_DEBUG
#define _MT6516_GLES_CMQ_	//Kevin
#define M3D_MULTI_CONTEXT
#ifdef M3D_MULTI_CONTEXT
#define M3D_HW_REGDATA_SIZE 4096
#define M3D_HW_REGDIRTY_SIZE 128 // => 4096 / 4 / 8
#define M3D_HW_DEBUG_HANG
//#define M3D_CMQ_CHECK
#endif
//#define M3D_IOCTL_LOCK
#define CMD_LIST_BREAK
#define SUPPORT_BGRA
#define SUPPORT_LOCK_DRIVER
//#define M3D_PROC_DEBUG
//#define M3D_PROC_PROFILE
#define M3D_DEBUG_SAVE_FRAME                (0x00000001)
#define M3D_DEBUG_HW_REG_DUMP               (0x00000002)
#define M3D_DEBUG_PROFILE_PERFORMANCE       (0x00000004)
#define M3D_DEBUG_MONITOR_MEMORY            (0x00000008)
#define M3D_DEBUG_BUS_TRAFFIC_LOG_BEGIN     (0x00000020)
#define M3D_DEBUG_BUS_TRAFFIC_LOG_END       (0x00000040)
#define M3D_DEBUG_CMDLIST_MASK              (0x0FFF0000)
#define M3D_DEBUG_CMDLIST_FRAME_BUFFER      (0x00010000)
#define M3D_DEBUG_CMDLIST_DEPTH_BUFFER      (0x00020000)
#define M3D_DEBUG_CMDLIST_VERTEX_BUFFER     (0x00040000)
#define M3D_DEBUG_CMDLIST_INDEX_BUFFER      (0x00080000)
#define M3D_DEBUG_CMDLIST_COLOR_BUFFER      (0x00100000)
#define M3D_DEBUG_CMDLIST_NORMAL_BUFFER     (0x00200000)
#define M3D_DEBUG_CMDLIST_TEXCOOR_BUFFER    (0x00400000)
#define M3D_DEBUG_CMDLIST_TEXTURE_IMAGE     (0x00800000)
#define M3D_DEBUG_CMDLIST_TEXTURE_RAWDATA   (0x01000000)
#define M3D_DRAWTEX_BY_DRAWARRAY

#ifdef _MT6516_GLES_PROFILE_	//Kevin for profile
#include <linux/time.h>
#endif

#if 1
//Kevin
#include <mach/mt6516_pll.h>
#include <mach/mt6516_ap_config.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_typedefs.h>
#else
#include <windows.h>
#endif
#include "gles/gl.h"
#include "gles/glext.h"
#include "m3d_config.h"

#define MAX_M3D_CONTEXT         10
#define M3D_DEVICE_NAME         L"M3D1:"
#define FILE_DEVICE_M3D         0x88000
#define IOCTL_M3D_MSG_POST      CTL_CODE(FILE_DEVICE_M3D, 0x0102, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAX_NUMBER_OF_THREAD_ENTRIES  32
#define GLES_THREAD_FILE_MAPPING_SIZE (MAX_NUMBER_OF_THREAD_ENTRIES * sizeof(DWORD) + 8)

typedef struct
{
   DWORD ThreadID[MAX_NUMBER_OF_THREAD_ENTRIES];
   DWORD ExistedThreadCounts;
   DWORD CurrentExecThreadID;
} GLES_FILE_MAPPING_INFO, *PGLES_FILE_MAPPING_INFO;

/// #define __USE_INTERRUPT_SCHEME__
/// #define __DUMP_PRINTF__
/// #define __DUMP_PRINTF_BUFFER_CTRL__
/// #define __DUMP_PRINTF_TEXTURE__
/// #define __MOVE_GL_CLEAR_TO_KERNEL__
/// #define __NEW_BUFFER_CONTROL__
/// #define __USE_POST_MESSAGE__

#define __USING_HWDRAWTEX_CLEAR__ // Nick
#define __M3D_INTERRUPT__ //Nick
#define __M3D_PROFILE__

#define MAX_MESSAGE_POOL_NUMBERS 64
#define BUFFER_CONTROL_RELEASE_SIZE 32
#define MAX_MESSAGE_NUMBER_FOR_SHARE 4
#define MAX_NUMBER_OF_CLIENTS 16

#define HW_STATUS_RESET    0x0
#define HW_STATUS_IDLE     0x1
#define HW_STATUS_BUSY  0x2
//#include <assert.h>

#ifndef ASSERT
#define ASSERT(x)    RETAILMSG(TRUE, (L"ASSERT FAILED!!\n")); \
                     assert(x)

#endif

#ifdef _MT6516_GLES_CMQ_
#define GET_CMQ_BUFFER_ALLOCATE_SIZE(size)	((((size) + 3) & ~3) + sizeof(MESSAGE_HEADER))
#endif

#if defined(__COMMON__)
   typedef GLfloat GLnative;
#elif defined(__COMMON_LITE__)
   typedef GLfixed GLnative;
#else
   typedef GLfixed GLnative;
#endif

// Bits to indicate what state has changed.
#define NEW_DRAW_CONTEXT        (1 << 0)  /// 0x00000001
#define NEW_POWER_HANDLE_CTRL   (1 << 1) /// 0x00000002	 
#define NEW_INVAL_CACHE         (1 << 2)  /// 0x00000004
#define NEW_FLUSH_CACHE         (1 << 3)  /// 0x0000008
#define NEW_MODELVIEW           (1 << 4)  /// 0x00000010
#define NEW_PROJECTION          (1 << 5)  /// 0x00000020
#define NEW_TEX_MATRIX          (1 << 6)  /// 0x00000040
#define NEW_NORMALIZE           (1 << 7)  /// 0x00000080
#define NEW_USER_CLIP           (1 << 8)  /// 0x00000100
#define NEW_FOG_CTRL            (1 << 9)  /// 0x00000200
#define NEW_LIGHT_CTRL          (1 << 10)  /// 0x00000400
#define NEW_POINT_CTRL          (1 << 11) /// 0x00000800
#define NEW_LINE_CTRL           (1 << 12) /// 0x000001000
#if 1//Kevin
#define NEW_BOUND_BOX           (1 << 4) /// 0x00000010
#define NEW_VIEWPORT            (1 << 4) /// 0x00000010
#else
#define NEW_BOUND_BOX           (1 << 13) /// 0x00001000
#define NEW_VIEWPORT            (1 << 14) /// 0x00002000
#endif
#define NEW_CREATE_CONTEXT      (1 << 13)
#define NEW_DESTROY_CONTEXT     (1 << 14)
#define NEW_TEXTURE             (1 << 15) /// 0x00008000
#define NEW_COLOR_CTRL          (1 << 16) /// 0x00010000
#define NEW_PER_FRAGMENT_TEST   (1 << 17) /// 0x00020000
#define NEW_POLYGON             (1 << 18) /// 0x00040000
#define NEW_CULL_MODE           (1 << 19) /// 0x00080000
#define NEW_STENCIL             (1 << 20) /// 0x00100000
#define NEW_ARRAY_INFO          (1 << 21) /// 0x00200000
#define NEW_TEXDRAWOES_CTRL     (1 << 22) /// 0x00400000
#define NEW_DRAW_EVENT          (1 << 23) /// 0x00800000
#define SWAP_DRAW_SURFACE_PHY   (1 << 25)

#ifdef _MT6516_GLES_CMQ_	//Kevin
#define NEW_CMQ_PROCESS         (1 << 26)	//0x04000000
#define NEW_CMQ_VERTEX_BUFFER	(1 << 27)	//0x08000000
#define NEW_CMQ_INIT            (1 << 28)	//0x10000000
#define NEW_CMQ_DEINIT          (1 << 29)	//0x20000000
#endif
#define NEW_ALL_CHANGED  0xFFFFFFFF

#define GET_PHYS_MEM            (0x80000001)
#define CHECK_APP_NAME          (0x80000002)
#define NEW_DCACHE_OP           (0x80000003)
#define QUERY_PROFILE           (0x80000004)
#define QUERY_DEBUG_FLAG        (0x80000005)
#define LOCK_DRIVER_OPERATION   (0x80000006)

#define NEW_TRANSFORM_MATRIX       (NEW_MODELVIEW  | NEW_PROJECTION)
#define NEW_PRIMITIVE_CTRL         (NEW_POINT_CTRL | NEW_LINE_CTRL)
#define NEW_BOUND_BOX_VIEW_PORT    (NEW_BOUND_BOX  | NEW_VIEWPORT)
#define NEW_BB_VP_TRANSFORM_CTRL   (NEW_BOUND_BOX_VIEW_PORT | NEW_TRANSFORM_MATRIX)
#define NEW_POLY_CULL_STENCIL_CTRL (NEW_POLYGON | NEW_CULL_MODE | NEW_STENCIL)
#define NEW_COLOR_PER_FRAG_CTRL    (NEW_COLOR_CTRL | NEW_PER_FRAGMENT_TEST)

typedef enum
{
    DCACHE_OP_INVALID,
    DCACHE_OP_FLUSH,
} ENUM_DCACHE_OP;
typedef struct DCacheOperation
{
    ENUM_DCACHE_OP  eOP;
    void*           pStart;
    int             i4Size;
} DCacheOperation;

typedef struct ProfileData
{
    GLuint SWTime;
    GLuint HWTime;
    GLuint InterruptTime;    
    GLuint IOCTLTime;
} ProfileData;

typedef struct ProcessInfo
{
    UINT32 pid;
    char comm[TASK_COMM_LEN];
} ProcessInfo;

typedef struct LockDriverOperation
{
    UINT32  ctxHandle;
    BOOL    bIsLock;
} LockDriverOperation;

typedef enum {
    M3D_STATUS_SUCCESS      =  0,
    M3D_STATUS_PARAMETER    = -1,    
    M3D_STATUS_MESSAGE      = -2,       
    M3D_STATUS_UNKNOW       = -3,
    M3D_STATUS_OUTOFMEM     = -4
} M3D_HARDWARE_STATUS;

enum ColorFormat
{
    COLOR_FORMAT_RGB565,
    COLOR_FORMAT_RGB888,
    COLOR_FORMAT_ARGB888
};

enum GLESProfile
{
   __COMMON_PROFILE__,
   __COMMON_LITE_PROFILE__  
};

typedef struct SwapDrawSurfacePhyStruct
{
    UINT32      ColorBase;
} SwapDrawSurfacePhyStruct;

typedef struct NewContext
{
    UINT32      ColorFormat;    
    UINT32      BufferWidth;
    UINT32      BufferHeight;
    UINT32      ColorBase;
    UINT32      DepthBase;
    UINT32      StencilBase;    
    UINT32      GLESProfile;    
} NewContextStruct;

typedef struct CacheCtrl
{
    UINT32      mask; 
    GLnative    ClearColor[4];   /// clear value for color buffer
    GLnative    ClearZValue;     /// clear value for depth buffer
    INT32      x; /// indicate clear rectangle
    INT32      y; /// indicate clear rectangle
    UINT32     w; /// indicate clear rectangle
    UINT32     h; /// indicate clear rectangle
} CacheCtrlStruct;


#if 1//Kevin
/// structure for texture matrix
struct TexMatrixCtrlUnit
{    
    BOOL        Enable;
    UINT32      Unit;
    GLnative    Value[16];
};

typedef struct TexMatrixStruct
{
   struct TexMatrixCtrlUnit TexMatrixCtrlInfo[MAX_TEXTURE_UNITS]; 
} TexMatrixStruct; 
#else
/// structure for texture matrix
typedef struct TexMatrix
{    
    BOOL        Enable;
    UINT32      Unit;
    GLnative      Value[16];
} TexMatrixStruct;
#endif

#define TEXTURE0_ENABLE (1 << 0)
#define TEXTURE1_ENABLE (1 << 1)
#define TEXTURE2_ENABLE (1 << 2)

#define TEXTURE_ANY_ENABLE (TEXTURE0_ENABLE | TEXTURE1_ENABLE | TEXTURE2_ENABLE)
#define TEXTURE_N_ENABLE(i) (1 << (i))

struct TexInfoUnit
{  
    GLnative EnvColor[4];
    UINT32 EnvMode;
    UINT32 CombineModeRGB;
    UINT32 CombineModeA;
    /// Set by glTexEnv.
    UINT32 CombineSourceRGB[3];
    /// Set by glTexEnv.
    UINT32 CombineSourceA[3];
    /// Set by glTexEnv.
    UINT32 CombineOperandRGB[3];
    /// Set by glTexEnv.
    UINT32 CombineOperandA[3];
    /// Set by glTexEnv.
    UINT32 CombineScaleShiftRGB;
    /// Set by glTexEnv.
    UINT32 CombineScaleShiftA;

    BOOL   CoordReplace[3]; /// no use in H/W register
    BOOL   DUMMY;		//Kevin
    ///========================    
    GLboolean   Enable;
    UINT32 Unit;
    UINT32 Type;
    UINT32 MagFilter;
    UINT32 MinFilter;
    UINT32 Width;
    UINT32 Height;
    UINT32 Format;
    UINT32 WrapS;
    UINT32 WrapT;
    
    UINT32 image_data[9];    /// store the mipmap image data address      
};

typedef struct TexEnvInfo
{
    GLnative EnvColor[4];
    
    UINT32 EnvMode;
    
    UINT32 CombineModeRGB;
    UINT32 CombineModeA;
    /// Set by glTexEnv.
    UINT32 CombineSourceRGB[3];
    /// Set by glTexEnv.
    UINT32 CombineSourceA[3];
    /// Set by glTexEnv.
    UINT32 CombineOperandRGB[3];
    /// Set by glTexEnv.
    UINT32 CombineOperandA[3];
    /// Set by glTexEnv.
    UINT32 CombineScaleShiftRGB;
    /// Set by glTexEnv.
    UINT32 CombineScaleShiftA; 
            
    BOOL   CoordReplace[3]; /// no use in H/W register
} TexEnvInfoStruct;

typedef struct TextureCtrl
{
   struct TexInfoUnit TexInfo[MAX_TEXTURE_UNITS];
   UINT32 TextureEnabled;
   
} TextureCtrlStruct;

typedef struct Normalize
{
    BOOL    Normalized;     /// enable/disable flag for normalization, normalize all normal or not
    BOOL    RescaleNormals; 
} NormalizeStruct;

typedef struct ClipPlane
{
    BOOL        Enabled;
    GLnative    Plane[4];
} ClipPlaneStruct;

typedef struct FogCtrl
{
    BOOL        Enabled;
    UINT32      Mode;
    GLnative    Density;
    GLnative    Start;
    GLnative    End;
    GLnative    Color[4];
} FogCtrlStruct;

#define NEW_AMBIENT_COLOR       0x00000001
#define NEW_DIFFUSE_COLOR       0x00000002
#define NEW_SPECULAR_COLOR      0x00000004
#define NEW_POSITION_VALUE      0x00000008
#define NEW_SPOT_DIRECTION      0x00000010
#define NEW_SPOT_EXPONENT       0x00000020
#define NEW_SPOT_CUTOFF         0x00000040
#define NEW_CONST_ATTENUX       0x00000080
#define NEW_LINEAR_ATTENUX      0x00000100
#define NEW_QUADX_ATTENUX       0x00000200


struct LightCtrlUnit
{
    /// light source
    UINT32      LightSource;
	BOOL	    LightEnable;      /// enable/disable each lightx source enable flag
    UINT32      DirtyFlags;
    GLnative    Ambient[4];
    GLnative    Diffuse[4];
    GLnative    Specular[4];
    GLnative    Position[4];
    GLnative    sceneAmbient[4];
    GLnative    Direction[3];
    GLnative    SpotExponent;    
    GLnative    CosCutoff;
    GLnative    ConstAttenx;
    GLnative    LinearAttenx;
    GLnative    QuadAttenx;    
    /// UINT32      ShadeModel;       /// no matter lighting is enabled or not, the shade model should be cirrectly set
};

#define MAT_ATTRIB_MAX 5

struct MaterialCtrlUnit
{
    /// light material
    BOOL        mat_ColorMaterial;
    UINT32      mat_DirtyFlags;    
    GLnative    mat_Attrib[MAT_ATTRIB_MAX][4];
};

typedef struct LightCtrlStruct
{
   struct MaterialCtrlUnit MaterialCtrlInfo; 
   struct LightCtrlUnit   LightCtrlInfo[MAX_LIGHT_SOURCES];   
   GLnative     sceneAmbient[4];
   BOOL         LightingCtrl;     /// global light control enabl/disable flag   
} LightCtrlStruct; 


struct PointCtrlStruct
{
    BOOL        Smooth;
    BOOL        PntSprite;
    UINT32      PntSpriteCtrl;
    GLnative    CurrSize;
    GLnative    MinSize;
    GLnative    MaxSize;
    GLnative    Factor[3];
};


struct LineCtrlStruct
{
    BOOL        Smooth;
    GLnative    Width;
} ;


typedef struct PrimitiveCtrl
{
   struct LineCtrlStruct    LineCtrlInfo;	//Kevin
   struct PointCtrlStruct   PointCtrlInfo;	//Kevin
} PrimitiveCtrlStruct; 



typedef struct BBViewportTransform
{
   INT32       ViewportX;
   INT32       ViewportY;
   GLsizei     ViewportW;
   GLsizei     ViewportH;
   INT32       ScissorX;
   INT32       ScissorY;
   GLsizei     ScissorW;
   GLsizei     ScissorH;
   BOOL        ScissorTest;
   
   GLnative    Near;
   GLnative    Far;       

   GLnative    Element[16];    /// Modelvie
   GLnative    Inverse[16];    /// Modelvie
   GLnative    Value[16];      /// projection view        
   
} BBViewportTransformStruct;


typedef struct BBViewportInfoOnly
{
   INT32       ViewportX;
   INT32       ViewportY;
   UINT32      ViewportW;
   UINT32      ViewportH;
   INT32       ScissorX;
   INT32       ScissorY;
   UINT32      ScissorW;
   UINT32      ScissorH;
   BOOL        ScissorTest;
   
   GLnative    Near;
   GLnative    Far;           
} BBViewportInfoStruct; 

typedef struct ColorCtrl
{
    GLnative    Color[4];
    UINT32      Mask[4];
    UINT32      ShadeModel;    
} ColorCtrlStruct;

struct AlphaTestUnit
{
    BOOL        Enabled;
    UINT32      Function;
    BYTE    Reference;
};

struct BlendTestUnit
{
    BOOL        Enabled;
    UINT32      SFactor;
    UINT32      DFactor;
    UINT32      ColorMask[4];
} ;

struct DepthTestUnit
{
    BOOL        Enabled;
    GLnative    ClearVal;
    UINT32      MaskVal;
    UINT32      Function;
} ;

struct LogicTestUnit
{
    BOOL        Enabled;
    UINT32      LogicOP;
} ;

typedef struct PerFragmentTest
{
    GLboolean   AlphaTestEnabled;
    UINT32      AlphaTestFunction;
    BYTE        AlphaTestReference;
           
    GLboolean   BlendTestEnabled;
    UINT32      BlendTestSFactor;
    UINT32      BlendTestDFactor;
    /// UINT32      BlendTestColorMask[4];
        
    GLboolean   DepthTestEnabled;
    GLnative    DepthTestClearVal;
    UINT32      DepthTestMaskVal;
    UINT32      DepthTestFunction;
          
    GLboolean   LogicTestEnabled;
    UINT32      LogicTestLogicOP;
                   
} PerFragmentTestStruct;

typedef struct Polygonx
{
    BOOL        OffsetFill;
    GLnative      OffsetUnits;
    GLnative      OffsetFactor;
} PolygonxStruct;

typedef struct CullModex
{
    BOOL        Enabled;
    UINT32      FrontFace;
    UINT32      CullMode;
} CullModexStruct;

typedef struct SampleCov
{
    UINT32      Value;
    BOOL        Invert;
} SampleCovStruct;

typedef struct Stencil
{
    BOOL        Enabled;
    UINT32      Function;
    UINT32      Reference;
    UINT32      ValueMask;
    UINT32      FailFunc;
    UINT32      ZFailFunc;
    UINT32      ZPassFunc;
    UINT32      WriteMask;
} StencilStruct;

typedef enum ArrayType {
    NEW_VERTEX_ARRAY,
    NEW_NORMAL_ARRAY,
    NEW_TEX_COORD_ARRAY,
    NEW_POINT_SIZE_ARRAY,
    NEW_COLOR_ARRAY
} ArrayType;


struct gl_client_array
{    
    UINT32      Pointer;
     INT32      Size;
    UINT32      Type;
    UINT32      Stride;
    GLboolean   Enable;
} ;

#define _TEXTURE_ARRAY0_POINTER  (1 << 0)
#define _TEXTURE_ARRAY1_POINTER  (1 << 1)
#define _TEXTURE_ARRAY2_POINTER  (1 << 2)
#define _TEXTURE_ARRAY_BIT(i)    (1 << (i))

#define _TEXTURE_ARRAY_ANY       (_TEXTURE_ARRAY0_POINTER | _TEXTURE_ARRAY1_POINTER | _TEXTURE_ARRAY2_POINTER)   

#define _VERTEX_ARRAY_POINTER    (1 << 3)
#define _NORMAL_ARRAY_POINTER    (1 << 4)
#define _POINT_ARRAY_POINTER     (1 << 5)
#define _COLOR_ARRAY_POINTER     (1 << 6)

typedef struct ArrayInfo
{
    UINT32 CurrentNormal[4];	//normal has 3 DOWRD but allocate 4 DWORD due to 2 DWORD aligh for HW access   
    struct gl_client_array  VertexArray;	//Kevin
    struct gl_client_array  NormalArray;	//Kevin
    struct gl_client_array  TexCoordArray[MAX_TEXTURE_UNITS];	//Kevin
    struct gl_client_array  PointSizeArray;	//Kevin
    struct gl_client_array  ColorArray;	//Kevin
    UINT32           ArrayEnabled;  /// bit mask for indicating each array enable/disable
} ArrayInfoStruct;

typedef enum DrawOP
{
    NEW_DRAW_ARRAY,
    NEW_DRAW_ELEMENT,
    NEW_DRAW_TEX
} DrawOP;

typedef struct TexDrawOES
{
    INT32     i_x;      /// 
    INT32     i_y;
    GLnative     z;
    GLnative     s;
    GLnative     t;
    GLnative    w_over_width;
    GLnative    h_over_height;
    UINT32      i_new_width;
    UINT32      i_new_height;
    GLnative    output;    
    UINT32      TexEnabled;
    BOOL        enable_trigger;    
} TexDrawOESStruct;


typedef struct DrawEvent
{
    DrawOP      OPCode;
    UINT32      Mode;
    INT32       First;
    INT32       Count;
    UINT32      Type;
    UINT32      Bytes;
    UINT32      Indices;
    UINT32      IndicesPA;
    
    /// UINT32      Total;
    UINT32      Buffer;
    BOOL        TwoSideLight;    
} DrawEventStruct;

typedef struct PowerEvent
{
  BOOL PowerOnFlag;
} PowerEventCtrlStruct;

#if 0    //Kevin
typedef struct MESSAGEBLOCK
{
   HANDLE CurrentProcId;   
   UINT32 MMapAddrA;
   UINT32 MMapAddrB;
   HANDLE Semaphore;
   HANDLE IdleEvent;      
} MESSAGEBLOCK;
#endif

#ifdef _MT6516_GLES_CMQ_	//Kevin
    typedef struct CMQInfo {
        GLbyte          *writePointer;
        GLbyte          *readPointer;
#ifdef M3D_MULTI_CONTEXT
        GLbyte          *lastPointer;
#endif
        GLbyte          *memoryPA;
        GLbyte          *memoryVA;    
        GLuint           memorySize;
    
        GLbyte          *infoBase;
        GLuint           infoSize;
        GLbyte          *bufferBase;
        GLbyte          *bufferEnd;
        GLuint           bufferSize;
    
        GLbyte          *allocatePointer;
     } CMQInfoStruct;

    typedef struct CMQ {
        GLbyte          *memoryPA;
        GLbyte          *memoryVA;    
        GLuint           memorySize;
#ifdef M3D_PROC_DEBUG
        GLbyte          *HWRegDataPA;
        GLbyte          *HWRegDataVA;
        GLuint           HWRegDataSize;
#endif
     } CMQStruct;

    typedef struct CMQVertexBuffer {
        GLuint           size;
     } CMQVertexBufferStruct;

    typedef struct CMQProcess {
        BOOL            waitIdle;
     } CMQProcessStruct;

#endif

    #define APP_NAME_SIZE	64
    typedef struct CheckAppName{
        GLuint           size;
        char		name[APP_NAME_SIZE];
     } CheckAppNameStruct;

typedef struct MESSAGE_HEADER
{        
    UINT32 Condition;        
    UINT32 ctxHandle;  //Kevin
#ifdef M3D_MULTI_CONTEXT
    BOOL IsListEnd;
    BOOL WaitIdle;
    char Reserved[2];
#endif
} MESSAGE_HEADER;

typedef struct MESSAGE
{
    MESSAGE_HEADER                  hdr;
    union {
#ifdef _MT6516_GLES_CMQ_	//Kevin
        CMQStruct                 CMQCtrl;
        CMQVertexBufferStruct     CMQVertexBufferCtrl;
        CMQProcessStruct          CMQProcessCtrl;
#endif
#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
        UINT32                    u4DebugFlag;
#endif
#ifdef M3D_PROC_PROFILE
        ProfileData               kProfileData;
#endif
        SwapDrawSurfacePhyStruct  SwapDrawSurfacePhy;
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
    };
} MESSAGE, *PMESSAGE;

typedef struct MESSAGE_CONTAINER
{
   UINT32  MsgCounts;
   MESSAGE MsgDataBlock[MAX_MESSAGE_POOL_NUMBERS + 1];      
} MESSAGE_CONTAINER, *PMESSAGE_CONTAINER; 

#define SYNC_BUFFER_EVENT   0x00000001

typedef struct BufferEventStruct
{
    UINT32                  BufferCount;
    UINT32                  BufferNum[32];
} BufferEventStruct;

typedef struct SyncEvent {
    UINT32                  EventType;
    union {
        BufferEventStruct   BufferEvent;
    };
} SyncEvent;

#endif  // __M3D_EXP_H__

