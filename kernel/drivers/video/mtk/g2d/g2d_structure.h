

#ifndef __G2D_STRUCTURE_H__
#define __G2D_STRUCTURE_H__

#include "g2d_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    P_U8             addr;
    UINT32           width;
    UINT32           height;
    G2D_COLOR_FORMAT format;
    G2D_RECT         rect;
} G2D_SURFACE;


typedef struct
{
    UINT16 up;
    UINT16 down;
} G2D_STRETCH_FACTOR;


typedef struct
{
    BOOL     enableClip;
    G2D_RECT clipWnd;
    
    G2D_ROTATE    rotate;
    G2D_DIRECTION direction;

    BOOL   enableSrcColorKey;
    UINT32 srcColorKey;
    
    BOOL   enableDstColorKey;
    UINT32 dstColorKey;
    
    BOOL   enableColorReplacement;
    UINT32 avoidColor;
    UINT32 replaceColor;

    BOOL enableStretch;
    G2D_STRETCH_FACTOR stretchFactor; 

    BOOL enableItalicBitblt;
    UINT8 tiltValues[32];

    G2D_SURFACE dstSurf;
    G2D_SURFACE srcSurf;
    G2D_SURFACE patSurf;
    G2D_SURFACE mskSurf;

    G2D_COORD patOffset;
     
    G2D_CALLBACK callback;

    // For performance profiling
    //
    unsigned long lastOpDuration;   // unit: 1/32K sec

} G2D_CONTEXT, *G2D_HANDLE;

#ifdef __cplusplus
}
#endif


#endif // __G2D_STRUCTURE_H__
