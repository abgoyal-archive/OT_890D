
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
    DMA_FULL_CHANNEL = 0,
    DMA_HALF_CHANNEL,
    DMA_VIRTUAL_FIFO,
    DMA_VIRTUAL_FIFO_UART0,
    DMA_VIRTUAL_FIFO_UART1,
    DMA_VIRTUAL_FIFO_UART2,
    DMA_VIRTUAL_FIFO_UART3,
    DMA_VIRTUAL_FIFO_UART4,
    DMA_VIRTUAL_FIFO_UART5
}DMA_TYPE;

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
#define DMA_CON_MASTER_MSDC0         	0x00000000
#define DMA_CON_MASTER_MSDC1            0x00100000
#define DMA_CON_MASTER_MSDC2       	    0x00200000
#define DMA_CON_MASTER_IRDATX       	0x00300000
#define DMA_CON_MASTER_IRDARX       	0x00400000
#define DMA_CON_MASTER_UART0TX      	0x00500000
#define DMA_CON_MASTER_UART0RX      	0x00600000
#define DMA_CON_MASTER_UART1TX      	0x00700000
#define DMA_CON_MASTER_UART1RX      	0x00800000
#define DMA_CON_MASTER_UART2TX      	0x00900000
#define DMA_CON_MASTER_UART2RX      	0x00a00000
#define DMA_CON_MASTER_UART3TX      	0x00b00000
#define DMA_CON_MASTER_UART3RX      	0x00c00000
#define DMA_CON_MASTER_UART4TX      	0x00d00000
#define DMA_CON_MASTER_UART4RX      	0x00e00000
#define DMA_CON_MASTER_I2CTX         	0x00f00000
#define DMA_CON_MASTER_I2CRX            0x01000000

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

struct mt_dma_conf *mt_request_dma(DMA_TYPE type);
void mt_free_dma(struct mt_dma_conf *);
void mt_start_dma(struct mt_dma_conf *);
void mt_stop_dma(struct mt_dma_conf *);

DMA_STATUS mt_config_dma(struct mt_dma_conf *, DMA_CONF_FLAG flag);

void mt_reset_dma(struct mt_dma_conf *);

DMA_STATUS mt_get_info(struct mt_dma_conf *, INFO_TYPE type, INFO *info);

#endif

