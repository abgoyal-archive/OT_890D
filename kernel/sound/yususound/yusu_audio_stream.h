


#ifndef _YUSU_AUDIO_STREAM_H_
#define _YUSU_AUDIO_STREAM_H_

#define ASM_SRC_BLOCK_MAX (1)
#define EDI_BLOCK_MAX (1)
#define ASM_STREAM_DIGITAL_GAIN (0x1000)

//#define CONFIG_DEBUG_MSG
//#define SOUND_FAKE_READ

typedef enum
{
    Mmap_None = -1,
    Mmap_Src_Block0,
    Mmap_Src_Block1,
    Mmap_Src_Block2
}_MMAP_BLOCK;


typedef struct
{
    kal_uint32 pucPhysBufAddr;
    kal_uint8 *pucVirtBufAddr;
    kal_int32 u4BufferSize;
    kal_int32 u4DataRemained;
    kal_uint32 u4SampleNumMask;    // sample number mask
    kal_uint32 u4SamplesPerInt;    // number of samples to play before interrupting
    kal_int32 u4WriteIdx;         // Previous Write Index.
    kal_int32 u4DMAReadIdx;       // Previous DMA Read Index.
    kal_uint32 u4InterruptLine;
    kal_uint32 u4fsyncflag;
    struct file *flip;
} ASM_SRC_BLOCK_T;


typedef struct
{

    ASM_SRC_BLOCK_T rSRCBlock[ASM_SRC_BLOCK_MAX];
    kal_uint32    u4BufferSize;
    kal_uint32    u4SRCBlockOccupiedNum;          // Occupied Block Number
    kal_bool        bRunning;
} ASM_CONTROL_T;

typedef struct
{
    ASM_SRC_BLOCK_T rSRCBlock[EDI_BLOCK_MAX];
    kal_uint32    u4BufferSize;
    kal_uint32    u4SRCBlockOccupiedNum;          // Occupied Block Number
    kal_bool        bRunning;
} EDI_CONTROL_T;

typedef struct
{
    int bSpeechFlag;
    int bBgsFlag;
    int bRecordFlag;
    int bTtyFlag;    
}Audio_Control_T;

#endif
