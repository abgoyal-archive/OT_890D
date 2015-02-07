

#ifndef __MT6516_GRAPHSYS__
#define __MT6516_GRAPHSYS__

#include    <mach/mt6516_typedefs.h>
#include    <mach/mt6516_reg_base.h>



#define GRAPH1SYS_CG_CON        (GRAPH1SYS_CONFG_BASE + 0x300)
#define GRAPH1SYS_CG_SET        (GRAPH1SYS_CONFG_BASE + 0x320)
#define GRAPH1SYS_CG_CLR        (GRAPH1SYS_CONFG_BASE + 0x340)
#define GRAPH1SYS_LCD_IO_SEL    (GRAPH1SYS_CONFG_BASE + 0x400)
#define GRAPH1SYS_DELSEL0       (GRAPH1SYS_CONFG_BASE + 0x600)
#define GRAPH1SYS_DELSEL1       (GRAPH1SYS_CONFG_BASE + 0x604)
#define GRAPH1SYS_DELSEL2       (GRAPH1SYS_CONFG_BASE + 0x608)
#define GRAPH1SYS_DELSEL3       (GRAPH1SYS_CONFG_BASE + 0x60C)

#define GRAPH2SYS_CG_CON        (GRAPH2SYS_BASE + 0x0)
#define GRAPH2SYS_CG_SET        (GRAPH2SYS_BASE + 0x4)
#define GRAPH2SYS_CG_CLR        (GRAPH2SYS_BASE + 0x8)
#define GRAPH2SYS_DESEL0        (GRAPH2SYS_BASE + 0x10)
#define GRAPH2SYS_DESEL1        (GRAPH2SYS_BASE + 0x14)
#define GRAPH2SYS_DESEL2        (GRAPH2SYS_BASE + 0x18)



typedef enum
{
    PDN_MM_GMC1	    =	0,
    PDN_MM_G2D	    =	1,
    PDN_MM_GCMQ	    =	2,
    PDN_MM_BLS	    =	3,
    PDN_MM_IMGDMA0	=	4,
    PDN_MM_PNG	    =	5,
    PDN_MM_DSI	    =	6,
    PDN_MM_TVE	    =	8,
    PDN_MM_TVC	    =	9,
    PDN_MM_ISP	    =	10,
    PDN_MM_IPP	    =	11,
    PDN_MM_PRZ	    =	12,
    PDN_MM_CRZ	    =	13,
    PDN_MM_DRZ	    =	14,
    PDN_MM_WT	    =	16,
    PDN_MM_AFE	    =	17,
    PDN_MM_SPI	    =	19,
    PDN_MM_ASM	    =	20,                    
    PDN_MM_RESZLB	=	22,                    
    PDN_MM_LCD	    =	23,                    
    PDN_MM_DPI	    =	24,                    
    PDN_MM_G1FAKE	=	25,                                 
} GRAPH1SYS_PDN_MODE;


typedef enum
{
    //GRAPH2SYS Clock Gating    
    PDN_MM_GMC2	    =	0,
    PDN_MM_IMGDMA1	=	1,
    PDN_MM_PRZ2	    =	2,
    PDN_MM_M3D	    =	3,
    PDN_MM_H264	    =	4,
    PDN_MM_DCT	    =	5,
    PDN_MM_JPEG	    =	6,
    PDN_MM_MP4	    =	7,
    PDN_MM_MP4DBLK	=	8,       
} GRAPH2SYS_PDN_MODE;


typedef enum
{
    GRAPH1SYS_LCD_IO_SEL_CPU_IF_ONLY = 0,
    GRAPH1SYS_LCD_IO_SEL_8CPU_18RGB  = 1,
    GRAPH1SYS_LCD_IO_SEL_9CPU_16RGB  = 2,
    GRAPH1SYS_LCD_IO_SEL_18CPU_8RGB  = 3,
    GRAPH1SYS_LCD_IO_SEL_MASK        = 0x3,
} GRAPH1SYS_LCD_IO_SEL_MODE;


#endif 



