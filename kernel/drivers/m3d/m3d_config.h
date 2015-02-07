

 
#ifndef __M3D_CONFIG_H__
#define __M3D_CONFIG_H__

/// #include "macros.h"
#define MAX_BUFFER_OBJECT   256

#define MAX_VIEW_WIDTH      240
#define MAX_VIEW_HEIGHT     320

#define MAX_LIGHT_SOURCES   8

#define MAX_MATRIX_DEPTH    32

/// #define MIN_POINT_SIZE      65536
/// #define MAX_POINT_SIZE      (65536 * 17)

/// Minimum point size
/// #define MIN_POINT_SIZE _GL_ONE
#define MIN_POINT_SIZE GL_N_ONE
/// Maximum point size
/// #define MAX_POINT_SIZE (_GL_ONE * SQRT_MAX_WIDTH)
/// #define MAX_POINT_SIZE (65536 * 17)
#define MAX_POINT_SIZE (GL_N_ONE * 17)

/// Point size granularity
#if defined(__COMMON_LITE__)
   #define POINT_SIZE_GRANULARITY (0.1F * 65536)
#else
   #define POINT_SIZE_GRANULARITY (0.1F)
#endif

#define MIN_LINE_WIDTH      GL_N_ONE       
#define MAX_LINE_WIDTH      (GL_N_ONE * 4)

#define MAX_CLIP_PLANES     1

/// Number of 2D texture mipmap levels
#define MAX_TEXTURE_LEVELS   9

#define MAX_TEXTURE_SIZE    256//(1 << (MAX_TEXTURE_LEVELS - 1))
#define MAX_TEXTURE_UNITS   3

#define SUB_PIXEL_BITS      4

/// Bit flags used for updating material values.
#define MAT_ATTRIB_FRONT_AMBIENT           0
#define MAT_ATTRIB_FRONT_DIFFUSE           1
#define MAT_ATTRIB_FRONT_SPECULAR          2
#define MAT_ATTRIB_FRONT_EMISSION          3
#define MAT_ATTRIB_FRONT_SHININESS         4
#define MAT_ATTRIB_MAX                     5

#define MAT_BIT_FRONT_AMBIENT         (1 << MAT_ATTRIB_FRONT_AMBIENT)
#define MAT_BIT_FRONT_DIFFUSE         (1 << MAT_ATTRIB_FRONT_DIFFUSE)
#define MAT_BIT_FRONT_SPECULAR        (1 << MAT_ATTRIB_FRONT_SPECULAR)
#define MAT_BIT_FRONT_EMISSION        (1 << MAT_ATTRIB_FRONT_EMISSION)
#define MAT_BIT_FRONT_SHININESS       (1 << MAT_ATTRIB_FRONT_SHININESS)
#define MAT_BIT_ALL ~0

#define GL_IMPLEMENTATION_COLOR_READ_TYPE_OES_VALUE     GL_UNSIGNED_BYTE
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES_VALUE   GL_RGBA
            
/// The number of vertex of each 3d model should be less than this value
/// to achieve maximal performance.
#ifdef __HW_C_MODEL__
   #define MAX_ARRAY_LOCK_SIZE 512
#else
   #define MAX_ARRAY_LOCK_SIZE 256
#endif


            
#endif  // __M3D_CONFIG_H__
