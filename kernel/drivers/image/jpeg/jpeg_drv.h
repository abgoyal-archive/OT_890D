
#include <linux/ioctl.h>

#ifndef __JPEG_DRV_H__
#define __JPEG_DRV_H__

// JPEG Decoder Structure
typedef struct
{
    int decID;

    unsigned int srcStreamAddr;
    unsigned int srcStreamSize;
        
    unsigned int mcuRow;                  ///< number of MCU row in the JPEG file
    unsigned int mcuColumn;               ///< number of MCU column in the JPEG file

    unsigned int totalDU[3];              ///< (required by HW decoder) number of DU for each component
    unsigned int duPerMCURow[3];          ///< (required by HW decoder) DU per MCU row for each component
    unsigned int dummyDU[3];              ///< (required by HW decoder) number of dummy DU for each component

    unsigned int samplingFormat;
    
    // JPEG component information
    unsigned int  componentNum;
    unsigned int  componentID[3];          ///< Ci
    unsigned int  hSamplingFactor[3];      ///< Hi
    unsigned int  vSamplingFactor[3];      ///< Vi
    unsigned int  qTableSelector[3];       ///< Tqi

    // JPEG Destination 
    unsigned char isPhyAddr;
    unsigned int *dstBufferPA;
    unsigned char *dstBufferVA;
    unsigned int dstBufferSize;

    // second dst buffer
    unsigned char needTempBuffer;
    unsigned int *tempBufferPA;
    unsigned int tempBufferSize;
    
}JPEG_DEC_DRV_IN;

typedef struct
{
    int decID;

    unsigned int srcStreamAddr;
    unsigned int srcStreamSize;

    unsigned char isPhyAddr;
    
}JPEG_DEC_RESUME_IN;

typedef struct
{
    int decID;

    unsigned int startIndex;
    unsigned int endIndex;
    unsigned int skipIndex1;
    unsigned int skipIndex2;
    unsigned int idctNum;
    
}JPEG_DEC_RANGE_IN;

typedef struct
{
    int decID;
    long timeout;
    unsigned int *result;
    
}JPEG_DEC_DRV_OUT;

// JPEG Encoder Structure
typedef struct
{
    int encID;
    
    unsigned char* dstBufferAddr;
    unsigned int dstBufferSize;
    
    unsigned int dstWidth;
    unsigned int dstHeight;
    
    unsigned char enableEXIF;
    unsigned char allocBuffer;
    
    unsigned int dstQuality;
    unsigned int dstFormat; 

}JPEG_ENC_DRV_IN;

typedef struct
{
    int encID;

    long timeout;
    unsigned int *fileSize;  
    unsigned int *result;
    
}JPEG_ENC_DRV_OUT;

#define JPEG_IOCTL_MAGIC        'x'

#define JPEG_DEC_IOCTL_INIT     _IOR(JPEG_IOCTL_MAGIC, 1, int)
#define JPEG_DEC_IOCTL_CONFIG   _IOWR(JPEG_IOCTL_MAGIC, 2, JPEG_DEC_DRV_IN)
#define JPEG_DEC_IOCTL_START    _IOW(JPEG_IOCTL_MAGIC, 3, int)
#define JPEG_DEC_IOCTL_RESUME   _IOW(JPEG_IOCTL_MAGIC, 4, JPEG_DEC_RESUME_IN)
#define JPEG_DEC_IOCTL_RANGE    _IOWR(JPEG_IOCTL_MAGIC, 5, JPEG_DEC_RANGE_IN) 
#define JPEG_DEC_IOCTL_WAIT     _IOWR(JPEG_IOCTL_MAGIC, 6, JPEG_DEC_DRV_OUT) 
#define JPEG_DEC_IOCTL_COPY     _IOWR(JPEG_IOCTL_MAGIC, 7, int) 
#define JPEG_DEC_IOCTL_DEINIT   _IOW(JPEG_IOCTL_MAGIC, 8, int)

#define JPEG_ENC_IOCTL_INIT     _IOR(JPEG_IOCTL_MAGIC, 11, int)
#define JPEG_ENC_IOCTL_CONFIG   _IOW(JPEG_IOCTL_MAGIC, 12, JPEG_ENC_DRV_IN)
#define JPEG_ENC_IOCTL_WAIT     _IOWR(JPEG_IOCTL_MAGIC, 13, JPEG_ENC_DRV_OUT)
#define JPEG_ENC_IOCTL_DEINIT   _IOW(JPEG_IOCTL_MAGIC, 14, int)

#endif

