

 
#ifndef __M3D_REGISTER_H__
#define __M3D_REGISTER_H__

//#include <windows.h>
//#include <ceddk.h>
//#include "base_regs.h"

#define M3D_DISABLE_VALUE               (0x0000)
#define M3D_ENABLE_VALUE                (0x0001)

#define M3D_DISABLE                      0x00
#define M3D_ENABLE                       0x01

#define M3D_PIPELINE_TRIGGER            (0x0000)

#define M3D_PIPELINE_RESET              (0x0004)

#define M3D_PIPELINE_STATUS             (0x0008)

#define M3D_FLOAT_INPUT_OFFSET          (0x800)
//##############################################################################
// group0
//##############################################################################
////////////////////////////////////////
// Sequencer -- 5
////////////////////////////////////////

#define M3D_TRIGGER                   (0x000) // m3d_register.h 0x000  
#define M3D_TRIGGER_BIT               0x01
#define M3D_STENCIL_BUF_CLEAR_TRIGGER 0x02 // reserved for HW simulator
#define M3D_DEPTH_BUF_CLEAR_TRIGGER   0x04 // reserved for HW simulator
#define M3D_COLOR_BUF_CLEAR_TRIGGER   0x08 // reserved for HW simulator
#define M3D_DRAWTEX_ENABLE            0x10
#define M3D_EARLY_Z_BUF_CLEAR_TRIGGER 0x20 // reserved for HW simulator
       
#define M3D_RESET                     (0x004)

#define M3D_STATUS                    (0x008)
/// Bit[0]: M3D_BUSY
/// Bit[1]: M3D_STENCIL_CLR_BUSY
/// Bit[2]: M3D_Z_CLR_BUSY
/// Bit[3]: M3D_COLOR_CLR_BUSY
/// Bit[4]: M3D_EARLYZ_CLR_BUSY
/// Bit[5]: M3D_Z_FLUSH_BUSY
/// Bit[6]: M3D_COLOR_FLUSH_BUSY
/// Bit[7]: M3D_FSAA_INVAL_BUSY
/// Read only register

#define M3D_INTERRUPT_ENABLE            (0x000C)
#define M3D_RENDER_DONE_INTR            (0x0001)
#define M3D_DEPTH_DONE_INTR             (0x0020)
#define M3D_COLOR_DONE_INTR             (0x0040)

#define M3D_INTERRUPT_STATUS            (0x0010)

#define M3D_CACHE_ENABLE_CTRL           (0x0014)

#define M3D_CACHE_FLUSH_CTRL            (0x0018)
#define M3D_STECIL_CACHE_FLUSH          (0x0001)
#define M3D_DEPTH_CACHE_FLUSH           (0x0002)
#define M3D_COLOR_CACHE_FLUSH           (0x0004)

#define M3D_CACHE_INVAL_CTRL            (0x001C)
#define M3D_STECIL_CACHE_INVAL          (0x0001)
#define M3D_DEPTH_CACHE_INVAL           (0x0002)
#define M3D_COLOR_CACHE_INVAL           (0x0004)

////////////////////////////////////////  
// Primitive -- 13                        
////////////////////////////////////////  
                                          
#define M3D_PRIMITIVE_MODE             (0x028)
#define M3D_POINTS                     0x0
#define M3D_LINES                      0x4
#define M3D_LINE_LOOP                  0x6
#define M3D_LINE_STRIP                 0x7
#define M3D_TRIANGLES                  0x8
#define M3D_TRIANGLE_STRIP_0           0xa
#define M3D_TRIANGLE_STRIP_1           0xb
#define M3D_TRIANGLE_FAN_0             0xc
#define M3D_TRIANGLE_FAN_1             0xd


#define M3D_PRIMITIVE_POINTS            (0x0000)
#define M3D_PRIMITIVE_LINES             (0x0004)
#define M3D_PRIMITIVE_LOOP              (0x0006)
#define M3D_PRIMITIVE_STRIP             (0x0007)
#define M3D_PRIMITIVE_TRIANGLES         (0x0008)
#define M3D_PRIMITIVE_TRI_STRIP0        (0x000A)
#define M3D_PRIMITIVE_TRI_STRIP1        (0x000B)
#define M3D_PRIMITIVE_TRI_FAN0          (0x000C)
#define M3D_PRIMITIVE_TRI_FAN1          (0x000D)

#define M3D_DRAW_MODE_CTRL              (0x002C)
#define M3D_DRAW_MODE_ARRAY             (0x0000)
#define M3D_DRAW_MODE_ELEMENT           (0x0001)

#define M3D_DRAW_TOTAL_COUNT            (0x0030)

#define M3D_DRAW_ARRAY_FIRST            (0x0034)

#define M3D_DRAW_ARRAY_END              (0x0038)

#define M3D_DRAW_ELEMENT_TYPE           (0x003C)
#define M3D_DRAW_ELEMENT_BYTE           (0x0001)
#define M3D_DRAW_ELEMENT_SHORT          (0x0003)

#define M3D_DRAW_ELEMENT_BUFFER         (0x0040)

#define M3D_PRIMITIVE_AA                (0x0044)

///// triangle
#define M3D_POLYGON_OFFSET_ENABLE      (0x048)
#define M3D_POLYGON_OFFSET_FACTOR      (0x04C)
#define M3D_POLYGON_OFFSET_UNITS       (0x050)

#define M3D_POLYGON_ENABLE              (0x0048)
#define M3D_POLYGON_FACTOR              (0x004C)
#define M3D_POLYGON_UNITS               (0x0050)

#define M3D_LINE_WIDTH                  (0x0054)

///// point
#define M3D_POINT_SPRITE               (0x058) // coord_replace(3:0) + enable

////////////////////////////////////////
// Vertex -- 5
////////////////////////////////////////

#define M3D_VERTEX                     (0x05C) // screen_space + type(3:0) + size(1:0)
#define M3D_VERTEX_STRIDE_B            (0x060)
#define M3D_VERTEX_POINTER             (0x064)

#define M3D_VERTEX_CACHE_POINTER       (0x068)
/// #define M3D_BBOX_EXPAND                (0x01B)

#define M3D_VERTEX_ARRAY                (0x005C)
#define M3D_VERTEX_SIZE_2               (0x0000)
#define M3D_VERTEX_SIZE_3               (0x0001)
#define M3D_VERTEX_SIZE_4               (0x0002)
#define M3D_VERTEX_BYTE                 (0x0000)
#define M3D_VERTEX_SHORT                (0x0008)
#define M3D_VERTEX_FLOAT                (0x0018)
#define M3D_VERTEX_FIXED                (0x0030)

#define M3D_VERTEX_STRIDE               (0x0060)

#define M3D_VERTEX_BUFFER               (0x0064)

#define M3D_VERTEX_CACHE                (0x0068)


#define M3D_BBOX_EXPAND                 (0x006C)
#define M3D_BBOX_EXPAND_X               (0x0001)
#define M3D_BBOX_EXPAND_Y               (0x0002)

////////////////////////////////////////
// Normal -- 3
////////////////////////////////////////

#define M3D_NORMAL_TYPE                (0x070)
#define M3D_NORMAL_STRIDE_B            (0x074)
#define M3D_NORMAL_POINTER             (0x078)

#define M3D_NORMAL_ARRAY                (0x0070)
#define M3D_NORMAL_BYTE                 (0x0000)
#define M3D_NORMAL_SHORT                (0x0002)
#define M3D_NORMAL_FLOAT                (0x0006)
#define M3D_NORMAL_FIXED                (0x000C)

#define M3D_NORMAL_STRIDE               (0x0074)

#define M3D_NORMAL_BUFFER               (0x0078)

////////////////////////////////////////
// Color -- 7
////////////////////////////////////////

#define M3D_COLOR_0                    (0x07C) // type(7:4) + + size(3) + order(2) + input(1) + enable(0)
#define M3D_COLOR_INPUT_REGISTER       0x0
#define M3D_COLOR_INPUT_POINTER        0x1
#define M3D_COLOR_ORDER_ABGR           0x0
#define M3D_COLOR_ORDER_ARGB           0x1

#define M3D_COLOR_1                   (0x080)

#define M3D_COLOR_STRIDE_B_0           (0x084)  
#define M3D_COLOR_STRIDE_B_1           (0x088)  

#define M3D_COLOR_STRIDE_B(I)          (0x084+((I)<<2))

#define M3D_COLOR_POINTER_0            (0x08C)  
#define M3D_COLOR_POINTER_1            (0x090) 

#define M3D_COLOR_POINTER(I)           (0x08C+((I)<<2))

#define M3D_COLOR_0_CTRL                (0x007C)
#define M3D_COLOR_0_ENABLE              (0x0001)
#define M3D_COLOR_0_ARRAY               (0x0002)
#define M3D_COLOR_0_ARGB                (0x0004)
#define M3D_COLOR_0_SIZE_3              (0x0000)
#define M3D_COLOR_0_SIZE_4              (0x0008)
#define M3D_COLOR_0_UBYTE               (0x0010)
#define M3D_COLOR_0_FLOAT               (0x0060)
#define M3D_COLOR_0_FIXED               (0x00C0)

#define M3D_COLOR_1_CTRL                (0x0080)
#define M3D_COLOR_1_ENABLE              (0x0001)
#define M3D_COLOR_1_ARRAY               (0x0002)
#define M3D_COLOR_1_ARGB                (0x0004)
#define M3D_COLOR_1_SIZE_3              (0x0000)
#define M3D_COLOR_1_SIZE_4              (0x0008)
#define M3D_COLOR_1_UBYTE               (0x0010)
#define M3D_COLOR_1_FLOAT               (0x0060)
#define M3D_COLOR_1_FIXED               (0x00C0)

#define M3D_COLOR_0_STRIDE              (0x0084)

#define M3D_COLOR_1_STRIDE              (0x0088)

#define M3D_COLOR_0_BUFFER              (0x008C)

#define M3D_COLOR_1_BUFFER              (0x0090)

#define M3D_TEX_COORD_CTRL(x)           (0x00A8 + ((x)<<2))
#define M3D_TEX_COORD_DIM_1D            (0x0001)
#define M3D_TEX_COORD_DIM_2D            (0x0002)
#define M3D_TEX_WRAP_S_1D               (0x0000)
#define M3D_TEX_WRAP_S_2D               (0x0004)
#define M3D_TEX_WRAP_T_1D               (0x0000)
#define M3D_TEX_WARP_T_2D               (0x0008)
#define M3D_TEX_COORD_SIZE_2            (0x0080)
#define M3D_TEX_COORD_SIZE_3            (0x0100)
#define M3D_TEX_COORD_SIZE_4            (0x0180)
#define M3D_TEX_COORD_BYTE              (0x0000)
#define M3D_TEX_COORD_SHORT             (0x0400)
#define M3D_TEX_COORD_FLOAT             (0x0600)
#define M3D_TEX_COORD_FIXED             (0x1800)

#define M3D_TEX_COORD_STRIDE(x)         (0x00B4 + ((x)<<2))
#define M3D_TEX_COORD_BUFFER(x)         (0x00C0 + ((x)<<2))

////////////////////////////////////////
// Texture coordinate -- 13
////////////////////////////////////////
#define M3D_TEX_COORD_0                (0x0A8) // type(12:9) + size(8:6) + x_proj(5) + x_en(4) + wrap_t(3) + wrap_s(2) + 2d(1) + tex_en(0)
#define M3D_TEX_COORD_1                (0x0AC)
#define M3D_TEX_COORD_2                (0x0B0)

#define M3D_TEX_COORD(I)               (0x0A8+((I)<<2))

#define M3D_TEX_COORD_STRIDE_B_0       (0x0B4)
#define M3D_TEX_COORD_STRIDE_B_1       (0x0B8)  
#define M3D_TEX_COORD_STRIDE_B_2       (0x0BC)

#define M3D_TEX_COORD_STRIDE_B(I)      (0x0B4+((I)<<2))

#define M3D_TEX_COORD_POINTER_0        (0x0C0)
#define M3D_TEX_COORD_POINTER_1        (0x0C4)
#define M3D_TEX_COORD_POINTER_2        (0x0C8)

#define M3D_TEX_COORD_POINTER(I)       (0x0C0+((I)<<2))

////////////////////////////////////////
// Point size -- 4
////////////////////////////////////////

#define M3D_PNT_SIZE_INPUT             (0x0CC) // type(3:0) + input
#define M3D_PNT_SIZE_INPUT_REGISTER    0x00
#define M3D_PNT_SIZE_INPUT_POINTER     0x01

#define M3D_PNT_SIZE                   (0x0D0)
#define M3D_PNT_SIZE_STRIDE_B          (0x0D4)
#define M3D_PNT_SIZE_POINTER           (0x0D8)

#define M3D_POINT_INPUT                 (0x00CC)
#define M3D_POINT_ARRAY                 (0x0001)
#define M3D_POINT_FLOAT                 (0x000C)
#define M3D_POINT_FIXED                 (0x0018)

#define M3D_CLIP_ENABLE                 (0x00E0)

#define M3D_CLIP_PLANE_ENABLE          (0x0E0)
#define M3D_NORMAL_SCALE_ENABLE        (0x0E4)
#define M3D_LINE_LAST_PIXEL            (0x0E8)

#define M3D_CULL_MODE_CTRL              (0x00EC)
#define M3D_CULL_FACE_MASK              (0x00E0)

#define M3D_CULL                       (0x0EC) // front face(7) + face(6:5) + clip_plane(4) + z(3) + zero_area(2) + all_out(1) + small_triangle(0)
#define M3D_CULL_FACE_BIT_OFFSET       5
#define M3D_CULL_FRONT                 0x0
#define M3D_CULL_BACK                  0x1
#define M3D_CULL_FRONT_AND_BACK        0x2
#define M3D_CULL_NONE                  0x3

#define M3D_FRONT_FACE_BIT_OFFSET      7
#define M3D_CW                         0x0
#define M3D_CCW                        0x1


#define M3D_SHADE_MODEL                (0x0F0) // flat + type shade mode
#define M3D_SHADE_FLAT                  (0x0000)
#define M3D_SHADE_SMOOTH               (0x0001)
#define M3D_FALT_LAST                  0x0
#define M3D_FALT_FIRST                 0x1

////////////////////////////////////////
// Lighting ctrl -- 1
//////////////////////////////////////// 
#define M3D_LIGHT_CTRL                  (0x00F4) 
#define M3D_FOG_EXP                    0x00
#define M3D_FOG_EXP2                   0x01
#define M3D_FOG_LINEAR                 0x02

////////////////////////////////////////
// Texturing -- 37
////////////////////////////////////////

////////// tex image
#define M3D_TEX_IMG_0                  (0x0F8) // height(25:16) + width(15:6) + order(4) + format(3:0)
#define M3D_TEX_IMG_1                  (0x0FC) 
#define M3D_TEX_IMG_2                  (0x100) 

#define M3D_TEX_IMG(I)                 (0x0F8 + ((I)<<2))
#define M3D_TEX_FMT_R8G8B8				0x0
#define M3D_TEX_FMT_R5G6B5				0x1
#define M3D_TEX_FMT_A8R8G8B8			0x2
#define M3D_TEX_FMT_X8R8G8B8			0x3
#define M3D_TEX_FMT_A1R5G5B5			0x4
#define M3D_TEX_FMT_X1R5G5B5			0x5
#define M3D_TEX_FMT_L8					0x6
#define M3D_TEX_FMT_A8					0x7
#define M3D_TEX_FMT_A4R4G4B4			0x8
#define M3D_TEX_FMT_X4R4G4B4			0x9
#define M3D_TEX_FMT_L8A8				0xA
#define M3D_TEX_FMT_DXT1				0xD
#define M3D_TEX_FMT_DXT2				0xE
#define M3D_TEX_FMT_DXT3				0xF
#define M3D_TEX_BYTE_ORDER				0x10

#define M3D_TEX_CTRL				   (0x104) //lod_less_half2(26) + lod_less_half1(25) + lod_less_half0(24) + threshold_en2(23) + threshold_en1(22) + threshold_en(21) + max_lod2(20:17) + max_lod1(16:13) + max_lod0(12:9) + en_spec(8) + stage_en(7:4) + enable(3:0)


#define M3D_SPECULAR_ENABLE				0x100

#define M3D_TEX_THRESHOLD_EN_0			0x200000
#define M3D_TEX_THRESHOLD_EN_1			0x400000
#define M3D_TEX_THRESHOLD_EN_2			0x800000

#define M3D_TEX_LOD_HALF_EN_0			0x1000000
#define M3D_TEX_LOD_HALF_EN_1			0x2000000
#define M3D_TEX_LOD_HALF_EN_2			0x4000000

// [31:0]
#define M3D_TEX_IMG_PTR_0_0			   (0x108)
#define M3D_TEX_IMG_PTR_0_1			   (0x10C)
#define M3D_TEX_IMG_PTR_0_2			   (0x110)
#define M3D_TEX_IMG_PTR_0_3			   (0x114)
#define M3D_TEX_IMG_PTR_0_4			   (0x118)
#define M3D_TEX_IMG_PTR_0_5			   (0x11C)
#define M3D_TEX_IMG_PTR_0_6			   (0x120)
#define M3D_TEX_IMG_PTR_0_7			   (0x124)
#define M3D_TEX_IMG_PTR_0_8			   (0x128)
                                    
// [31:0]                           
#define M3D_TEX_IMG_PTR_1_0			   (0x12C)
#define M3D_TEX_IMG_PTR_1_1			   (0x130)
#define M3D_TEX_IMG_PTR_1_2			   (0x134)
#define M3D_TEX_IMG_PTR_1_3			   (0x138)
#define M3D_TEX_IMG_PTR_1_4			   (0x13C)
#define M3D_TEX_IMG_PTR_1_5			   (0x140)
#define M3D_TEX_IMG_PTR_1_6			   (0x144)
#define M3D_TEX_IMG_PTR_1_7			   (0x148)
#define M3D_TEX_IMG_PTR_1_8			   (0x14C)
                                    
// [31:0]                           
#define M3D_TEX_IMG_PTR_2_0			   (0x150)
#define M3D_TEX_IMG_PTR_2_1			   (0x154)
#define M3D_TEX_IMG_PTR_2_2			   (0x158)
#define M3D_TEX_IMG_PTR_2_3			   (0x15C)
#define M3D_TEX_IMG_PTR_2_4			   (0x160)
#define M3D_TEX_IMG_PTR_2_5			   (0x164)
#define M3D_TEX_IMG_PTR_2_6			   (0x168)
#define M3D_TEX_IMG_PTR_2_7			   (0x16C)
#define M3D_TEX_IMG_PTR_2_8			   (0x170)
                                             
#define M3D_TEX_IMG_PTR(I,J)		      (0x108+(((I)*9+(J))<<2))

#define M3D_COLOR_R_0_VALUE             (0x0174)
#define M3D_COLOR_G_0_VALUE             (0x0178)
#define M3D_COLOR_B_0_VALUE             (0x017C)
#define M3D_COLOR_A_0_VALUE             (0x0180)

////////////////////////////////////////
// Scissor test -- 5
////////////////////////////////////////
#define M3D_SCISSOR_ENABLE              (0x0198)
#define M3D_SCISSOR_LEFT                (0x019C)
#define M3D_SCISSOR_BOTTOM              (0x01A0)
#define M3D_SCISSOR_RIGHT               (0x01A4)
#define M3D_SCISSOR_TOP                 (0x01A8)

////////////////////////////////////////
// Alpha test -- 1
////////////////////////////////////////

#define M3D_ALPHA_TEST                 (0x1AC) // ref(7:0) + func(2:0) + enable

////////////////////////////////////////
// Stencil test -- 1
////////////////////////////////////////

#define M3D_STENCIL_TEST               (0x1B0) // s_mask + dppass(2:0) + dpfail(2:0) + sfail(2:0) + mask(7:0) + ref(7:0) + func(2:0) + enable
#define M3D_STENCIL_TEST_ZERO          4
#define M3D_STENCIL_TEST_INVERT        5

#define M3D_STENCIL_TEST_ENABLE_OFFSET 0
#define M3D_STENCIL_TEST_FUNC_OFFSET   1
#define M3D_STENCIL_TEST_REF_OFFSET    4
#define M3D_STENCIL_TEST_MASK_OFFSET   12
#define M3D_STENCIL_SFAIL_OP_OFFSET    20
#define M3D_STENCIL_DPFAIL_OP_OFFSET   23
#define M3D_STENCIL_DPPASS_OP_OFFSET   26
#define M3D_STENCIL_FMT_OFFSET         29
////////////////////////////////////////
// Depth test -- 1
////////////////////////////////////////

#define M3D_DEPTH_TEST                 (0x1B4) // z_mask + func(2:0) + enable
      
////////////////////////////////////////
// Blending -- 1
////////////////////////////////////////

#define M3D_BLEND                      (0x1B8) // a_mask + b_mask + g_mask + r_mask + dest(3:0) + src(3:0) + enable

////////////////////////////////////////
// Logic Operation -- 1
////////////////////////////////////////

#define M3D_LOGIC_OP                   (0x1BC) // op(3:0) + enable


#define M3D_BLEND_TEST                  (0x01B8)
#define M3D_BLEND_R_MASK                (0x1000)
#define M3D_BLEND_G_MASK                (0x0800)
#define M3D_BLEND_B_MASK                (0x0400)
#define M3D_BLEND_A_MASK                (0x0200)

#define M3D_LOGIC_TEST                  (0x01BC)
#define M3D_LOGIC_CLEAR                 (0x0000)


#define M3D_COLOR_FORMAT                (0x01C0)

#define M3D_COLOR_RGBA8888              (0x0000)
#define M3D_COLOR_RGB888                (0x0001)
#define M3D_COLOR_RGBA5658              (0x0003)
#define M3D_COLOR_RGB565		        (0x0002)
#define M3D_COLOR_ARGB8888	            (0x0004)

#define M3D_COLOR_BUFFER                (0x01C4)

#define M3D_BUFFER_WIDTH                (0x01C8)

#define M3D_BUFFER_HEIGHT               (0x01CC)

#define M3D_STENCIL_MASK                (0x01D0)

#define M3D_DEPTH_BUFFER                (0x01D4)

#define M3D_STENCIL_BUFFER              (0x01D8)

#define M3D_DEPTH_CLEAR                 (0x01DC)

#define M3D_INTERRUPT_ACK               (0x01FC)


#define M3D_DEPTH_CLEAR_VAL            (0x1DC)
#define M3D_STENCIL_CLEAR_VAL          (0x1E0)
#define M3D_COLOR_CLEAR_VAL            (0x1E4)


////////////////////////////////////////
// Texturing - 24
////////////////////////////////////////

////////// tex env
#define M3D_TEX_ENV_COLOR_R_0          (0x200)
#define M3D_TEX_ENV_COLOR_G_0          (0x204)
#define M3D_TEX_ENV_COLOR_B_0          (0x208)
#define M3D_TEX_ENV_COLOR_A_0          (0x20C)
#define M3D_TEX_ENV_COLOR_R_1          (0x210)
#define M3D_TEX_ENV_COLOR_G_1          (0x214)
#define M3D_TEX_ENV_COLOR_B_1          (0x218)
#define M3D_TEX_ENV_COLOR_A_1          (0x21C)
#define M3D_TEX_ENV_COLOR_R_2          (0x220)
#define M3D_TEX_ENV_COLOR_G_2          (0x224)
#define M3D_TEX_ENV_COLOR_B_2          (0x228)
#define M3D_TEX_ENV_COLOR_A_2          (0x22C)
                          
#define M3D_TEX_ENV_COLOR_R(I)         (0x200+((I)<<4))
#define M3D_TEX_ENV_COLOR_G(I)         (0x204+((I)<<4))
#define M3D_TEX_ENV_COLOR_B(I)         (0x208+((I)<<4))
#define M3D_TEX_ENV_COLOR_A(I)		   (0x20C+((I)<<4))

#define M3D_LOD_BIAS_0                 (0x230)
#define M3D_LOD_BIAS_1                 (0x234)
#define M3D_LOD_BIAS_2                 (0x238)

#define M3D_LOD_BIAS(I)				   (0x230+((I)<<2))

////////////////////////////////////////
//RESERVED
//                                     (0x08F)        
////////////////////////////////////////
                                          
#define M3D_TEX_OP_0				   (0x240) // dot3(14) + a_scale(13:12) + rgb_scale(11:10) + op_a(9:5) + op_rgb(4:0)
#define M3D_TEX_OP_1				   (0x244) 
#define M3D_TEX_OP_2				   (0x248) 

#define M3D_TEX_OP(I)				   (0x240+((I)<<2))

#define M3D_TEX_OP_SELECTARG1			0x0
#define M3D_TEX_OP_SELECTARG2			0x1
#define M3D_TEX_OP_MODULATE				0x2
#define M3D_TEX_OP_ADD					0x3
#define M3D_TEX_OP_ADDSIGNED			0x4
#define M3D_TEX_OP_SUBTRACT				0x5
#define M3D_TEX_OP_PREMODULATE			0x6
#define M3D_TEX_OP_DOTPRODUCT3			0x7
#define M3D_TEX_OP_MULTIPLYADD			0x8
#define M3D_TEX_OP_LERP					0x9


////////////////////////////////////////
//RESERVED
//                                     (0x093)        
////////////////////////////////////////
                                          
#define M3D_TEX_SRC_OPD_0           (0x250) // operand(29:18) + src(17:0)
#define M3D_TEX_SRC_OPD_1           (0x254) 
#define M3D_TEX_SRC_OPD_2           (0x258) 

#define M3D_TEX_SRC_OPD(I)          (0x250+((I)<<2))

#define M3D_TEX_SRC_TEXTURE			0x0
#define M3D_TEX_SRC_CONSTANT		0x1
#define M3D_TEX_SRC_PRIMARY			0x2
#define M3D_TEX_SRC_PREVIOUS		0x3
#define M3D_TEX_SRC_SECONDARY		0x4
#define M3D_TEX_SRC_TEMP			0x5
#define M3D_TEX_SRC_ZERO			0x6

#define M3D_TEX_OPD_SRC_COLOR             0
#define M3D_TEX_OPD_ONE_MINUS_SRC_COLOR   1
#define M3D_TEX_OPD_SRC_ALPHA             2
#define M3D_TEX_OPD_ONE_MINUS_SRC_ALPHA   3

/// M3D_TEX_IMG
#define M3D_TEX_IMG_FORMAT_OFFSET  0
#define M3D_TEX_BYTE_ORDER_OFFSET  4
#define M3D_TEX_IMG_WIDTH_OFFSET   6
#define M3D_TEX_IMG_HEIGHT_OFFSET  16

/// M3D_TEX_OP
#define M3D_TEX_OP_RGB_OFFSET      0
#define M3D_TEX_OP_A_OFFSET        5
#define M3D_TEX_SCALE_RGB_OFFSET   10
#define M3D_TEX_SCALE_A_OFFSET     12
#define M3D_TEX_DOT3_RGBA_OFFSET   14

/// M3D_TEX_SRC_OPD
#define M3D_TEX_SRC0_RGB_OFFSET 0
#define M3D_TEX_SRC1_RGB_OFFSET 3
#define M3D_TEX_SRC2_RGB_OFFSET 6
#define M3D_TEX_SRC0_A_OFFSET   9
#define M3D_TEX_SRC1_A_OFFSET   12
#define M3D_TEX_SRC2_A_OFFSET   15
#define M3D_TEX_OPD0_RGB_OFFSET 18
#define M3D_TEX_OPD1_RGB_OFFSET 20
#define M3D_TEX_OPD2_RGB_OFFSET 22
#define M3D_TEX_OPD0_A_OFFSET   24
#define M3D_TEX_OPD1_A_OFFSET   26
#define M3D_TEX_OPD2_A_OFFSET   28


#define M3D_FILL_MODE                   (0x025C)
#define M3D_FILL_POINT                  0x0
#define M3D_FILL_WIREFRAME              0x1
#define M3D_FILL_SOLID                  0x2

#define M3D_VB_LOWER_LEFT_X             (0x0260)
#define M3D_VB_LOWER_LEFT_Y             (0x0264)
#define M3D_VB_UPPER_RIGHT_X            (0x0268)
#define M3D_VB_UPPER_RIGHT_Y            (0x026C)

#define M3D_VIEWPORT_NEAR               (0x0278)

#define M3D_VIEWPORT_FAR                (0x027C)

/// I:0~15
#define M3D_MODEL_VIEW(x)               (0x0280 + ((x)<<2))
               
               /// I:0~15
#define M3D_PV(I)                      (0x02C0+((I)<<2))

#define M3D_C_N_X                      (0x300)
#define M3D_C_N_Y                      (0x304)
#define M3D_C_N_Z                      (0x308)
#define M3D_C_N_W                      (0x30C)
 
#define M3D_PROJECTION(x)               (0x02C0 + ((x)<<2))

#define M3D_CLIP_PLANE_X                (0x0300)
#define M3D_CLIP_PLANE_Y                (0x0304)
#define M3D_CLIP_PLANE_Z                (0x0308)
#define M3D_CLIP_PLANE_W                (0x030C)

/// U:0~3 I:0~11
#define M3D_TEX_M(x, y)            (0x0340 + (((x) * 12 + (y))<<2))

//##############################################################################
// group1
//##############################################################################

#define M3D_NORMAL_N_1                 (0x400) // m3d_register.h 0x100
#define M3D_NORMAL_N_2                 (0x404)
#define M3D_NORMAL_N_3                 (0x408)
#define M3D_NORMAL_N_4                 (0x40C)
#define M3D_NORMAL_N_5                 (0x410)
#define M3D_NORMAL_N_6                 (0x414)
#define M3D_NORMAL_N_7                 (0x418)
#define M3D_NORMAL_N_8                 (0x41C)
#define M3D_NORMAL_N_9                 (0x420)

#define M3D_NORMAL_N(x)                 (0x0400+((x)<<2))

#define M3D_NORMAL_SCALE               (0x424)

#define M3D_FOG_COLOR_R                 (0x0428)
#define M3D_FOG_COLOR_G                 (0x042C)
#define M3D_FOG_COLOR_B                 (0x0430)
#define M3D_FOG_COLOR_A                 (0x0434)

////////////////////////////////////////
// Frame buffer mode
////////////////////////////////////////


     
////////////////////////////////////////
// RESERVED  
//                                     (0x438)
////////////////////////////////////////

#define M3D_PXL_CNT                    (0x43C)
////////////////////////////////////////
// Lighting data - 221
////////////////////////////////////////

#define M3D_A_CM_R                     (0x440)
#define M3D_A_CM_G                     (0x444)
#define M3D_A_CM_B                     (0x448)

#define M3D_TXCACHE_CNT                (0x44C)
#define M3D_A_CM_A                     (0x44C) // obsolete

#define M3D_D_CM_R                     (0x450)
#define M3D_D_CM_G                     (0x454)
#define M3D_D_CM_B                     (0x458)
#define M3D_D_CM_A                     (0x45C)
#define M3D_S_CM_R                     (0x460)  
#define M3D_S_CM_G                     (0x464)
#define M3D_S_CM_B                     (0x468)
#define M3D_S_CM_A                     (0x46C)
#define M3D_E_CM_R                     (0x470)
#define M3D_E_CM_G                     (0x474)
#define M3D_E_CM_B                     (0x478)

#define M3D_ZC_CNT                     (0x47C)
#define M3D_E_CM_A                     (0x47C) // obsolete

#define M3D_AMBIENT_CL_R(x)             (0x0480 + ((x) << 4))
#define M3D_AMBIENT_CL_G(x)             (0x0484 + ((x) << 4))
#define M3D_AMBIENT_CL_B(x)             (0x0488 + ((x) << 4))
#define M3D_AMBIENT_CL_A(x)             (0x048C + ((x) << 4))

#define M3D_DIFFUSE_CL_R(x)             (0x0500 + ((x) << 4))
#define M3D_DIFFUSE_CL_G(x)             (0x0504 + ((x) << 4))
#define M3D_DIFFUSE_CL_B(x)             (0x0508 + ((x) << 4))
#define M3D_DIFFUSE_CL_A(x)             (0x050C + ((x) << 4))

#define M3D_SPECULAR_CL_R(x)            (0x0580 + ((x) << 4))
#define M3D_SPECULAR_CL_G(x)            (0x0584 + ((x) << 4))
#define M3D_SPECULAR_CL_B(x)            (0x0588 + ((x) << 4))
#define M3D_SPECULAR_CL_A(x)            (0x058C + ((x) << 4))

#define M3D_POSITION_PL_X(x)            (0x0600 + ((x) << 4))
#define M3D_POSITION_PL_Y(x)            (0x0604 + ((x) << 4))
#define M3D_POSITION_PL_Z(x)            (0x0608 + ((x) << 4))
#define M3D_POSITION_PL_W(x)            (0x060C + ((x) << 4))

#define M3D_SPOT_LIGHT_DL_X(x)          (0x0680 + ((x) << 4))
#define M3D_SPOT_LIGHT_DL_Y(x)          (0x0684 + ((x) << 4))
#define M3D_SPOT_LIGHT_DL_Z(x)          (0x0688 + ((x) << 4))

#define M3D_SPOT_LIGHT_RL(x)            (0x068C + ((x) << 4))

#define M3D_SPOT_CUTOFF_RL(x)           (0x0700 + ((x) << 4))

#define M3D_A_CL_0_R                   (0x480)  
#define M3D_A_CL_0_G                   (0x484)
#define M3D_A_CL_0_B                   (0x488)
#define M3D_A_CL_0_A                   (0x48C)
#define M3D_A_CL_1_R                   (0x490)
#define M3D_A_CL_1_G                   (0x494)
#define M3D_A_CL_1_B                   (0x498)
#define M3D_A_CL_1_A                   (0x49C)
#define M3D_A_CL_2_R                   (0x4A0)
#define M3D_A_CL_2_G                   (0x4A4)
#define M3D_A_CL_2_B                   (0x4A8)
#define M3D_A_CL_2_A                   (0x4AC)
#define M3D_A_CL_3_R                   (0x4B0)
#define M3D_A_CL_3_G                   (0x4B4)
#define M3D_A_CL_3_B                   (0x4B8)
#define M3D_A_CL_3_A                   (0x4BC)
#define M3D_A_CL_4_R                   (0x4C0)
#define M3D_A_CL_4_G                   (0x4C4)
#define M3D_A_CL_4_B                   (0x4C8)
#define M3D_A_CL_4_A                   (0x4CC)
#define M3D_A_CL_5_R                   (0x4D0)
#define M3D_A_CL_5_G                   (0x4D4)
#define M3D_A_CL_5_B                   (0x4D8)
#define M3D_A_CL_5_A                   (0x4DC)
#define M3D_A_CL_6_R                   (0x4E0)
#define M3D_A_CL_6_G                   (0x4E4)
#define M3D_A_CL_6_B                   (0x4E8)
#define M3D_A_CL_6_A                   (0x4EC)
#define M3D_A_CL_7_R                   (0x4F0)
#define M3D_A_CL_7_G                   (0x4F4)
#define M3D_A_CL_7_B                   (0x4F8)
#define M3D_A_CL_7_A                   (0x4FC)
                                       
#define M3D_A_CL_R(I)                  (0x480+((I)<<4))
#define M3D_A_CL_G(I)                  (0x484+((I)<<4))
#define M3D_A_CL_B(I)                  (0x488+((I)<<4))
#define M3D_A_CL_A(I)                  (0x48C+((I)<<4))
                                       
#define M3D_D_CL_0_R                   (0x500)
#define M3D_D_CL_0_G                   (0x504)
#define M3D_D_CL_0_B                   (0x508)
#define M3D_D_CL_0_A                   (0x50C)
#define M3D_D_CL_1_R                   (0x510)
#define M3D_D_CL_1_G                   (0x514)
#define M3D_D_CL_1_B                   (0x518)
#define M3D_D_CL_1_A                   (0x51C)
#define M3D_D_CL_2_R                   (0x520)
#define M3D_D_CL_2_G                   (0x524)
#define M3D_D_CL_2_B                   (0x528)
#define M3D_D_CL_2_A                   (0x52C)
#define M3D_D_CL_3_R                   (0x530)
#define M3D_D_CL_3_G                   (0x534)
#define M3D_D_CL_3_B                   (0x538)
#define M3D_D_CL_3_A                   (0x53C)
#define M3D_D_CL_4_R                   (0x540)  
#define M3D_D_CL_4_G                   (0x544)
#define M3D_D_CL_4_B                   (0x548)
#define M3D_D_CL_4_A                   (0x54C)
#define M3D_D_CL_5_R                   (0x550)
#define M3D_D_CL_5_G                   (0x554)
#define M3D_D_CL_5_B                   (0x558)
#define M3D_D_CL_5_A                   (0x55C)
#define M3D_D_CL_6_R                   (0x560)
#define M3D_D_CL_6_G                   (0x564)
#define M3D_D_CL_6_B                   (0x568)
#define M3D_D_CL_6_A                   (0x56C)
#define M3D_D_CL_7_R                   (0x570)
#define M3D_D_CL_7_G                   (0x574)
#define M3D_D_CL_7_B                   (0x578)
#define M3D_D_CL_7_A                   (0x57C)
                                       
#define M3D_D_CL_R(I)                  (0x500+((I)<<4))
#define M3D_D_CL_G(I)                  (0x504+((I)<<4))
#define M3D_D_CL_B(I)                  (0x508+((I)<<4))
#define M3D_D_CL_A(I)                  (0x50C+((I)<<4))
                                       
#define M3D_S_CL_0_R                   (0x580)
#define M3D_S_CL_0_G                   (0x584)
#define M3D_S_CL_0_B                   (0x588)
#define M3D_S_CL_0_A                   (0x58C)
#define M3D_S_CL_1_R                   (0x590)
#define M3D_S_CL_1_G                   (0x594)
#define M3D_S_CL_1_B                   (0x598)
#define M3D_S_CL_1_A                   (0x59C)
#define M3D_S_CL_2_R                   (0x5A0)
#define M3D_S_CL_2_G                   (0x5A4)
#define M3D_S_CL_2_B                   (0x5A8)
#define M3D_S_CL_2_A                   (0x5AC)
#define M3D_S_CL_3_R                   (0x5B0)
#define M3D_S_CL_3_G                   (0x5B4)
#define M3D_S_CL_3_B                   (0x5B8)
#define M3D_S_CL_3_A                   (0x5BC)
#define M3D_S_CL_4_R                   (0x5C0)
#define M3D_S_CL_4_G                   (0x5C4)
#define M3D_S_CL_4_B                   (0x5C8)
#define M3D_S_CL_4_A                   (0x5CC)
#define M3D_S_CL_5_R                   (0x5D0)
#define M3D_S_CL_5_G                   (0x5D4)
#define M3D_S_CL_5_B                   (0x5D8)
#define M3D_S_CL_5_A                   (0x5DC)
#define M3D_S_CL_6_R                   (0x5E0)
#define M3D_S_CL_6_G                   (0x5E4)
#define M3D_S_CL_6_B                   (0x5E8)
#define M3D_S_CL_6_A                   (0x5EC)
#define M3D_S_CL_7_R                   (0x5F0)
#define M3D_S_CL_7_G                   (0x5F4)
#define M3D_S_CL_7_B                   (0x5F8)
#define M3D_S_CL_7_A                   (0x5FC)
                                      
#define M3D_S_CL_R(I)                  (0x580+((I)<<4))
#define M3D_S_CL_G(I)                  (0x584+((I)<<4))
#define M3D_S_CL_B(I)                  (0x588+((I)<<4))
#define M3D_S_CL_A(I)                  (0x58C+((I)<<4))
                                       
#define M3D_P_PL_0_X                   (0x600)
#define M3D_P_PL_0_Y                   (0x604)
#define M3D_P_PL_0_Z                   (0x608)
#define M3D_P_PL_0_W                   (0x60C)
#define M3D_P_PL_1_X                   (0x610)
#define M3D_P_PL_1_Y                   (0x614)
#define M3D_P_PL_1_Z                   (0x618)
#define M3D_P_PL_1_W                   (0x61C)
#define M3D_P_PL_2_X                   (0x620)
#define M3D_P_PL_2_Y                   (0x624)
#define M3D_P_PL_2_Z                   (0x628)
#define M3D_P_PL_2_W                   (0x62C)
#define M3D_P_PL_3_X                   (0x630)
#define M3D_P_PL_3_Y                   (0x634)
#define M3D_P_PL_3_Z                   (0x638)
#define M3D_P_PL_3_W                   (0x63C)
#define M3D_P_PL_4_X                   (0x640)
#define M3D_P_PL_4_Y                   (0x644)
#define M3D_P_PL_4_Z                   (0x648)
#define M3D_P_PL_4_W                   (0x64C)
#define M3D_P_PL_5_X                   (0x650)
#define M3D_P_PL_5_Y                   (0x654)
#define M3D_P_PL_5_Z                   (0x658)
#define M3D_P_PL_5_W                   (0x65C)
#define M3D_P_PL_6_X                   (0x660)
#define M3D_P_PL_6_Y                   (0x664)
#define M3D_P_PL_6_Z                   (0x668)
#define M3D_P_PL_6_W                   (0x66C)
#define M3D_P_PL_7_X                   (0x670)
#define M3D_P_PL_7_Y                   (0x674)
#define M3D_P_PL_7_Z                   (0x678)
#define M3D_P_PL_7_W                   (0x67C)
                                       
#define M3D_P_PL_X(I)                  (0x600+((I)<<4))
#define M3D_P_PL_Y(I)                  (0x604+((I)<<4))
#define M3D_P_PL_Z(I)                  (0x608+((I)<<4))
#define M3D_P_PL_W(I)                  (0x60C+((I)<<4))
                                       
#define M3D_S_DL_0_X                   (0x680)
#define M3D_S_DL_0_Y                   (0x684)
#define M3D_S_DL_0_Z                   (0x688)
#define M3D_S_RL_0                     (0x68C)
#define M3D_S_DL_1_X                   (0x690)
#define M3D_S_DL_1_Y                   (0x694)
#define M3D_S_DL_1_Z                   (0x698)
#define M3D_S_RL_1                     (0x69C)
#define M3D_S_DL_2_X                   (0x6A0)
#define M3D_S_DL_2_Y                   (0x6A4)
#define M3D_S_DL_2_Z                   (0x6A8)
#define M3D_S_RL_2                     (0x6AC)
#define M3D_S_DL_3_X                   (0x6B0)
#define M3D_S_DL_3_Y                   (0x6B4)
#define M3D_S_DL_3_Z                   (0x6B8)
#define M3D_S_RL_3                     (0x6BC)
#define M3D_S_DL_4_X                   (0x6C0)
#define M3D_S_DL_4_Y                   (0x6C4)
#define M3D_S_DL_4_Z                   (0x6C8)
#define M3D_S_RL_4                     (0x6CC)
#define M3D_S_DL_5_X                   (0x6D0)
#define M3D_S_DL_5_Y                   (0x6D4)
#define M3D_S_DL_5_Z                   (0x6D8)
#define M3D_S_RL_5                     (0x6DC)
#define M3D_S_DL_6_X                   (0x6E0)
#define M3D_S_DL_6_Y                   (0x6E4)
#define M3D_S_DL_6_Z                   (0x6E8)
#define M3D_S_RL_6                     (0x6EC)
#define M3D_S_DL_7_X                   (0x6F0)
#define M3D_S_DL_7_Y                   (0x6F4)
#define M3D_S_DL_7_Z                   (0x6F8)
#define M3D_S_RL_7                     (0x6FC)
                                       
#define M3D_S_DL_X(I)                  (0x680+((I)<<4))
#define M3D_S_DL_Y(I)                  (0x684+((I)<<4))
#define M3D_S_DL_Z(I)                  (0x688+((I)<<4))
#define M3D_S_RL(I)                    (0x68C+((I)<<4))


#define M3D_C_RL_0                     (0x700)
#define M3D_K_0_0                      (0x704)
#define M3D_K_1_0                      (0x708)
#define M3D_K_2_0                      (0x70C)
#define M3D_C_RL_1                     (0x710)
#define M3D_K_0_1                      (0x714)
#define M3D_K_1_1                      (0x718)
#define M3D_K_2_1                      (0x71C)
#define M3D_C_RL_2                     (0x720)
#define M3D_K_0_2                      (0x724)
#define M3D_K_1_2                      (0x728)
#define M3D_K_2_2                      (0x72C)
#define M3D_C_RL_3                     (0x730)
#define M3D_K_0_3                      (0x734)
#define M3D_K_1_3                      (0x738)
#define M3D_K_2_3                      (0x73C)
#define M3D_C_RL_4                     (0x740)
#define M3D_K_0_4                      (0x744)
#define M3D_K_1_4                      (0x748)
#define M3D_K_2_4                      (0x74C)
#define M3D_C_RL_5                     (0x750)
#define M3D_K_0_5                      (0x754)
#define M3D_K_1_5                      (0x758)
#define M3D_K_2_5                      (0x75C)
#define M3D_C_RL_6                     (0x760)
#define M3D_K_0_6                      (0x764)
#define M3D_K_1_6                      (0x768)
#define M3D_K_2_6                      (0x76C)
#define M3D_C_RL_7                     (0x770)
#define M3D_K_0_7                      (0x774)
#define M3D_K_1_7                      (0x778)
#define M3D_K_2_7                      (0x77C)

#define M3D_C_RL(I)                    (0x700+((I)<<4))
#define M3D_K_0(I)                     (0x704+((I)<<4))
#define M3D_K_1(I)                     (0x708+((I)<<4))
#define M3D_K_2(I)                     (0x70C+((I)<<4))

#define M3D_CONST_ATTENUX(x)            (0x0704 + ((x) << 4))
#define M3D_LINEAR_ATTENUX(x)           (0x0708 + ((x) << 4))
#define M3D_QUADX_ATTENUX(x)            (0x070C + ((x) << 4))

#define M3D_A_CS_R                     (0x780)
#define M3D_A_CS_G                     (0x784)
#define M3D_A_CS_B                     (0x788)

#define M3D_CC_CNT                     (0x78C)
#define M3D_A_CS_A                     (0x78C) // obsolete

#define M3D_S_RM                       (0x790)
                                       

#define M3D_FOG_DENSITY                 (0x0794)
#define M3D_FOG_START_VAL               (0x0798)
#define M3D_FOG_END_VAL                 (0x079C)

#define M3D_POINT_SIZE_MIN              (0x07A0)
#define M3D_POINT_SIZE_MAX              (0x07A4)
#define M3D_POINT_ATTENUX_A             (0x07A8)
#define M3D_POINT_ATTENUX_B             (0x07AC)
#define M3D_POINT_ATTENUX_C             (0x07B0) 

////////// tex param                        
// [5:0]                                    
#define M3D_TEX_PARA_0				   (0x7B4) // wrap_t(9:7) + wrap_s(6:4) + mag_filer(3) + min_filter(2:0)
#define M3D_TEX_PARA_1				   (0x7B8)
#define M3D_TEX_PARA_2				   (0x7BC)

#define M3D_TEX_NEAREST                 0x00
#define M3D_TEX_LINEAR                  0x01
#define M3D_TEX_NEAREST_MIPMAP_NEAREST  0x04
#define M3D_TEX_LINEAR_MIPMAP_NEAREST   0x05
#define M3D_TEX_NEAREST_MIPMAP_LINEAR   0x06
#define M3D_TEX_LINEAR_MIPMAP_LINEAR    0x07

#define M3D_TEX_MAG_FILTER_LINEAR       0x08
#define M3D_TEX_WRAP_S_CLAMP_TO_EDGE    0x10
#define M3D_TEX_WRAP_S_MIRROR		    0x20
#define M3D_TEX_WRAP_S_CLAMP		    0x30
#define M3D_TEX_WRAP_S_BORDER		    0x40
#define M3D_TEX_WRAP_T_CLAMP_TO_EDGE    0x80
#define M3D_TEX_WRAP_T_MIRROR		    0x100
#define M3D_TEX_WRAP_T_CLAMP		    0x180
#define M3D_TEX_WRAP_T_BORDER		    0x200

#define M3D_TEX_PARA(I)				   (0x7B4+((I)<<2))

#define M3D_CACHE_PERF_CTRL            (0x7C0)        
                                          
#define M3D_SWIZZLE                    (0x7C4) // m3d_register.h 0x1F1
/// Bit[0]: M3D_TEX_SWIZZLE_0 : texture #0 swizzle
/// Bit[1]: M3D_TEX_SWIZZLE_1 : texture #1 swizzle
/// Bit[2]: M3D_TEX_SWIZZLE_2 : texture #2 swizzle

#define M3D_DRAWTEX_CRU_0              (0x7C8)
#define M3D_DRAWTEX_CRV_0              (0x7CC)
#define M3D_DRAWTEX_DCRU_0             (0x7D0)
#define M3D_DRAWTEX_DCRV_0             (0x7D4)
#define M3D_DRAWTEX_XY                 (0x7D8)
#define M3D_DRAWTEX_WH                 (0x7DC)
#define M3D_DRAWTEX_Z                  (0x7E0)
#define M3D_DRAWTEX_FOG                (0x7E4)
                                       
#define M3D_DBGRD_0                    (0x7E8)
#define M3D_DBGRD_1                    (0x7EC)
#define M3D_DBGRD_2                    (0x7F0)
#define M3D_DBGRD_3                    (0x7F4)
#define M3D_DBGRD_4                    (0x7F8)
#define M3D_DBGRD_5                    (0x7FC)

#endif  // __M3D_REGISTER_H__
