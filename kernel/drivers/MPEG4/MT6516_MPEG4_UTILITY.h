

#ifndef __MT6516_MPEG4_UTILITY_H__
#define __MT6516_MPEG4_UTILITY_H__
#include <mach/mt6516_typedefs.h> 


/////////////////////////////////////////
//  spec related
#define I_VOP		               0
#define P_VOP		               1
#define B_VOP		               2
#define S_VOP		               3

#define VO_START_CODE		      0x8
#define VO_START_CODE_MIN	      0x100
#define VO_START_CODE_MAX	      0x11f

#define VOL_START_CODE		      0x12
#define VOL_START_CODE_MIN	      0x120
#define VOL_START_CODE_MAX	      0x12f

#define VOS_START_CODE           0x1b0
#define USR_START_CODE           0x1b2
#define GOP_START_CODE           0x1b3
#define VSO_START_CODE           0x1b5
#define VOP_START_CODE	         0x1b6
#define STF_START_CODE           0x1c3 // stuffing_start_code
#define SHV_START_CODE           0x020
#define SHV_END_MARKER           0x03f

///////////////////////////
//   utility definition

#define MP4_MAX(a,b)                 ( ((a)>(b))? a : b )
#define MP4_MIN(a,b)                 ( ((a)<(b))? a : b )
#define MP4_ENFORCE_MINMAX(a,b,c)    MP4_MAX(a,(MP4_MIN(b,c)))

#define VIDEO_MAX(a,b)                 ( ((a)>(b))? a : b )
#define VIDEO_MIN(a,b)                 ( ((a)<(b))? a : b )
#define ENFORCE_MINMAX(a,b,c)    VIDEO_MAX(a,(VIDEO_MIN(b,c)))

//#define MP4_ANYBASE_TO_ANYBASE(_TIME_SRC, _TIME_BASE_SRC, _TIME_BASE_DST)    ((kal_uint64)_TIME_SRC*(kal_uint64)_TIME_BASE_DST/(kal_uint64)_TIME_BASE_SRC)


/////////////////////////////
// functions
/////////////////////////////

extern void mp4_warning(UINT32 line);

extern INT32 mp4_util_show_bits(UINT8 * data, INT32 bitcnt, INT32 num);
extern INT32 mp4_util_get_bits(UINT8 * data, INT32 *bitcnt, INT32 num);
extern INT32 mp4_util_show_word(UINT8 * a);
extern INT32 mp4_util_log2ceil(INT32 arg);

extern INT32 mp4_util_user_data(UINT8 * data, INT32 * bitcnt, UINT32 max_parse_data_size);

extern void mp4_putbits(UINT8 * in, INT32 * bitcnt, INT32 data, INT32 data_length);


#endif /*__MT6516_MPEG4_UTILITY_H__*/


