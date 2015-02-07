
#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

#define MAX_DMA_ADDRESS		0xffffffff
#define MAX_DMA_CHANNELS	0

#endif

#ifndef MT_DMA_H
#define MT_DMA_H

#include <linux/types.h>

typedef u32 INFO;

typedef enum{
    DMA_FALSE = 0,
    DMA_TRUE
}DMA_BOOL;


typedef enum{
    DMA_OK = 0,
    DMA_FAIL
}DMA_STATUS;

typedef enum{
    REMAINING_LENGTH = 0, /* not valid for virtual FIFO */
    VF_READPTR,           /* only valid for virtual FIFO */
    VF_WRITEPTR,          /* only valid for virtual FIFO */
    VF_FFCNT,             /* only valid for virtual FIFO */
    VF_ALERT,             /* only valid for virtual FIFO */
    VF_EMPTY,             /* only valid for virtual FIFO */
    VF_FULL,              /* only valid for virtual FIFO */
    VF_PORT
}INFO_TYPE;

typedef enum{
    ALL = 0,
    SRC,
    DST,
    SRC_AND_DST
}DMA_CONF_FLAG;

/* MASTER */   
#define DMA_CON_MASTER_SIM2         (0  << 20)
#define DMA_CON_MASTER_MSDC0        (1  << 20)
#define DMA_CON_MASTER_MSDC1        (2  << 20)
#define DMA_CON_MASTER_IRDATX       (3  << 20)
#define DMA_CON_MASTER_IRDARX       (4  << 20)
#define DMA_CON_MASTER_UART0TX      (5  << 20)
#define DMA_CON_MASTER_UART0RX      (6  << 20)
#define DMA_CON_MASTER_UART1TX      (7  << 20)
#define DMA_CON_MASTER_UART1RX      (8  << 20)
#define DMA_CON_MASTER_UART2TX      (9  << 20)
#define DMA_CON_MASTER_UART2RX      (10 << 20)
#define DMA_CON_MASTER_NFITX        (11 << 20)
#define DMA_CON_MASTER_NFIRX        (12 << 20)
#define DMA_CON_MASTER_VFE          (13 << 20)
#define DMA_CON_MASTER_I2CTX        (14 << 20)
#define DMA_CON_MASTER_I2CRX        (15 << 20)
#define DMA_CON_MASTER_UART3TX      (16 << 20)
#define DMA_CON_MASTER_UART3RX      (17 << 20)
#define DMA_CON_MASTER_MSDC2        (18 << 20)


/* burst */
#define DMA_CON_BURST_SINGLE        	0x00000000  /*without burst mode*/
#define DMA_CON_BURST_4BEAT         	0x00000200  /*4-beat incrementing burst*/
#define DMA_CON_BURST_8BEAT         	0x00000400  /*8-beat incrementing burst*/
#define DMA_CON_BURST_16BEAT        	0x00000600  /*16-beat incrementing burst*/

/* size */                        
#define DMA_CON_SIZE_BYTE           	0x00000000
#define DMA_CON_SIZE_SHORT          	0x00000001
#define DMA_CON_SIZE_LONG           	0x00000002




struct mt_dma_conf{             /*   full-size    half-size    virtual-FIFO */
    
    u16 count;                  /*           o            o               o */
    u32   mas;                  /*           o            o               o */
    DMA_BOOL iten;              /*           o            o               o */
    u32 burst;                  /*           o            o               o */
    DMA_BOOL dreq;              /*           o            o               o */
    DMA_BOOL dinc;              /*           o            o               o */
    DMA_BOOL sinc;              /*           o            o               o */
    u8 size;                    /*           o            o               o */
    u8 limiter;                 /*           o            o               o */
    void *data;                 /*           o            o               o */
    void (*callback)(void *);   /*           o            o               o */  
    u32 src;                    /*           o            x               x */
    u32 dst;                    /*           o            x               x */
    DMA_BOOL wpen;              /*           o            o               x */
    DMA_BOOL wpsd;              /*           o            o               x */
    u16 wppt;                   /*           o            o               x */
    u32 wpto;                   /*           o            o               x */
    u32 pgmaddr;                /*           x            o               o */
    DMA_BOOL dir;               /*           x            o               o */
    DMA_BOOL b2w;               /*           x            o               x */
    u8 altlen;                  /*           x            x               o */
    u16 ffsize;                 /*           x            x               o */
};

struct mt_dma_conf *mt_request_full_size_dma(void);

struct mt_dma_conf *mt_request_half_size_dma(void);

struct mt_dma_conf *mt_request_falf_size_dma(void);

struct mt_dma_conf *mt_request_virtual_fifo_dma(unsigned int vf_dma_ch);


void mt_free_dma(struct mt_dma_conf *);
void mt_start_dma(struct mt_dma_conf *);
void mt_stop_dma(struct mt_dma_conf *);

DMA_STATUS mt_config_dma(struct mt_dma_conf *, DMA_CONF_FLAG flag);

void mt_reset_dma(struct mt_dma_conf *);

DMA_STATUS mt_get_info(struct mt_dma_conf *, INFO_TYPE type, INFO *info);

#endif

