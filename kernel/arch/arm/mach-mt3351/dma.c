
#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_pmu_sw.h>
#include <mach/irqs.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>

//#define DMA_DEBUG


    #define DMA_BASE_CH(n)                  (DMA_BASE + 0x0100*(n+1))

    #define DMA_GLBSTA_L                	(DMA_BASE + 0x0000) 
    #define DMA_GLBSTA_H                	(DMA_BASE + 0x0004)

    #define DMA_VPORT_BASE                  0xF0110000
    #define DMA_VPORT_CH(ch)                (DMA_VPORT_BASE + (ch - 14) * 0x00002000)
    /* for full-size DMA channel only, _n = 1 ~ 8 */
    #define DMA_SRC(base)                 	(base)           
    #define DMA_DST(base)                 	(base + 0x0004)
    /* 
     * for both full-size DMA channel and 
     *          half-size DMA channel, 
     *          _n = 1 ~ 14 
     */    
    #define DMA_WPPT(base)                	(base + 0x0008)    
    #define DMA_WPTO(base)                	(base + 0x000c)    
    /* 
     * for full-size DMA channel, 
     *     half-size DMA channel, and
     *     virtual FIFO, 
     *     _n = 1 ~ 20
     */
    #define DMA_COUNT(base)               	(base + 0x0010)   
    #define DMA_CON(base)                 	(base + 0x0014)   
    #define DMA_START(base)               	(base + 0x0018)   
    #define DMA_INTSTA(base)              	(base + 0x001c)    
    #define DMA_ACKINT(base)              	(base + 0x0020)    
    #define DMA_RLCT(base)                	(base + 0x0024)    
    #define DMA_LIMITER(base)             	(base + 0x0028)   
    /* 
     * for both half-size DMA channel and
     *          virtual FIFO, 
     *          _n = 9 ~ 20
     */
    #define DMA_PGMADDR(base)             	(base + 0x002c)   

    /*
     * for virtual FIFO only, _n = 15 ~ 20
     */

    #define DMA_WRPTR(base)               	(base + 0x0030)
    #define DMA_RDPTR(base)               	(base + 0x0034)
    #define DMA_FFCNT(base)               	(base + 0x0038)
    #define DMA_FFSTA(base)               	(base + 0x003c)
    #define DMA_ALTLEN(base)              	(base + 0x0040)
    #define DMA_FFSIZE(base)              	(base + 0x0044)



    /* 2-1. DMA_GLBSTA */
    
        #define DMA_GLBSTA_RUN(ch)          	(0x00000001 << (2*(ch))) 
        #define DMA_GLBSTA_IT(ch)           	(0x00000002 << (2*(ch)))  
                                   
        #define DMA_COUNT_MASK              	 0x0000ffff

    /* 2-2. DMA_CON */
                        
        /* SINC,DINC,DREQ */                                    
            #define DMA_CON_SINC                	0x00000004
            #define DMA_CON_DINC                	0x00000008
            #define DMA_CON_DRQ                 	0x00000010  /*1:hw, 0:sw handshake*/
        /* B2W */                                                                       
            #define DMA_CON_B2W                 	0x00000020  /*word to byte or byte to word, only used in half size dma*/
            
        /* ITEN,WPSD,WPEN,DIR, DREQ */                                   
            #define DMA_CON_ITEN                	0x00008000  /*Interrupt enable*/
            #define DMA_CON_WPSD                	0x00010000  /*0:at source, 1: at destination*/
            #define DMA_CON_WPEN                	0x00020000  /*0:disable, 1: enable*/
            #define DMA_CON_DIR                 	0x00040000  /*Only valid when dma = 9 ~ 20*/

    /* 2-3. DMA_START,DMA_INTSTA,DMA_ACKINT */                                   
        #define DMA_START_BIT               	0x00008000
        #define DMA_STOP_BIT                	0x00000000
        #define DMA_ACKINT_BIT              	0x00008000
        #define DMA_INTSTA_BIT              	0x00008000
    /* 2-4 DMA_FFSTA MASK */
        #define DMA_FFSTA_FULL                  0x00000001
        #define DMA_FFSTA_EMPTY                 0x00000002
        #define DMA_FFSTA_ALT                   0x00000004
    
    #define DMA_FULL_CHANNEL_NO       8
    #define DMA_HALF_CHANNEL_NO       6
    #define DMA_VFIFO_CHANNEL_NO      6
    #define DMA_FS_START              0
    #define DMA_HS_START              8
    #define DMA_VF_START             14

    #define DMA_NO (DMA_FULL_CHANNEL_NO + DMA_HALF_CHANNEL_NO + DMA_VFIFO_CHANNEL_NO)

    #define UART0 14
    #define UART1 15
    #define UART2 16
    #define UART3 17
    #define UART4 18
    #define UART5 19
    

typedef struct DMA_CHAN{
    struct mt_dma_conf config;
    u32 baseAddr;
    DMA_TYPE type;
    u8 chan_num;
    unsigned int registered;
}DMA_CHAN;


DMA_CHAN dma_chan[DMA_NO];

spinlock_t free_list_lock;

u8 free_list_head_fs;
u8 free_list_fs[DMA_FULL_CHANNEL_NO];

u8 free_list_head_hs;
u8 free_list_hs[DMA_HALF_CHANNEL_NO];

struct mt_dma_conf *mt_request_dma(DMA_TYPE type){

    struct mt_dma_conf *config = NULL;
    unsigned long irq_flag;
    
/* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        printk("DMA Module - Request DMA Channel: ");
        switch(type)
        {
            case DMA_FULL_CHANNEL:
                printk("Full-Size DMA Channel");
                break;
            case DMA_HALF_CHANNEL:
                printk("Half-Size DMA Channel");
                break;
            case DMA_VIRTUAL_FIFO:
                printk("No more available!!");
                break;
            case DMA_VIRTUAL_FIFO_UART0:
            case DMA_VIRTUAL_FIFO_UART1:
            case DMA_VIRTUAL_FIFO_UART2:
            case DMA_VIRTUAL_FIFO_UART3:
            case DMA_VIRTUAL_FIFO_UART4:
            case DMA_VIRTUAL_FIFO_UART5:
                printk("Virtual FIFO");
                break;
        }
        printk("\n");        
    #endif
    /* ===============DEBUG-END================ */

    /* grab free_list_lock */
    spin_lock_irqsave(&free_list_lock, irq_flag);

    switch(type){
        case DMA_FULL_CHANNEL:
            if(free_list_head_fs != DMA_FULL_CHANNEL_NO){
                config = &(dma_chan[DMA_FS_START + free_list_head_fs].config);
                dma_chan[DMA_FS_START + free_list_head_fs].registered = 
DMA_TRUE;
                free_list_head_fs = free_list_fs[free_list_head_fs];
            }
            break;
            
        case DMA_HALF_CHANNEL:
            if(free_list_head_hs != DMA_HALF_CHANNEL_NO){
                config = &(dma_chan[DMA_HS_START + free_list_head_hs].config);
                dma_chan[DMA_HS_START + free_list_head_hs].registered = 
DMA_TRUE;
                free_list_head_hs = free_list_hs[free_list_head_hs];
            }
            break;
            
        case DMA_VIRTUAL_FIFO_UART0:
            if(!dma_chan[UART0].registered){
                config = &(dma_chan[UART0].config);
                dma_chan[UART0].registered = DMA_TRUE;
            }
            break;
            
        case DMA_VIRTUAL_FIFO_UART1:
            if(!dma_chan[UART1].registered){
                config = &(dma_chan[UART1].config);
                dma_chan[UART1].registered = DMA_TRUE;
            }
            break;
            
        case DMA_VIRTUAL_FIFO_UART2:
            if(!dma_chan[UART2].registered){
                config = &(dma_chan[UART2].config);
                dma_chan[UART2].registered = DMA_TRUE;
            }
            break;

        case DMA_VIRTUAL_FIFO_UART3:
            if(!dma_chan[UART3].registered){
                config = &(dma_chan[UART3].config);
                dma_chan[UART3].registered = DMA_TRUE;
            }
            break;

        case DMA_VIRTUAL_FIFO_UART4:
            if(!dma_chan[UART4].registered){
                config = &(dma_chan[UART4].config);
                dma_chan[UART4].registered = DMA_TRUE;
            }
            break;

        case DMA_VIRTUAL_FIFO_UART5:
            if(!dma_chan[UART5].registered){
                config = &(dma_chan[UART5].config);
                dma_chan[UART5].registered = DMA_TRUE;
            }
            break;
            
        default:
            printk(KERN_ERR"DMA Module - The Requested Channel Type Doesn't \
            Exist!!\n");
            break;
    }
    
    /* release free_list_lock */
    spin_unlock_irqrestore(&free_list_lock, irq_flag);

    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
    
        if(config != NULL){
            DMA_CHAN *chan = (DMA_CHAN *)config;
            printk("DMA Module - Request Channel: Success\n");
            printk("DMA Module - The Granted Channel Number: %d\n", \
            chan->chan_num + 1);
            printk("DMA Module - The Granted Channel Base: %x\n", chan->
baseAddr);
        }
        else{
            printk("DMA Module - Request Channel: Failed\n");
        }
        
    #endif
    /* ===============DEBUG-END================ */
    return config;
}
          

void mt_free_dma(struct mt_dma_conf *config){

    DMA_CHAN *chan = (DMA_CHAN *)config;
    u8 ch = chan->chan_num;
    unsigned long irq_flag;

    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        printk("DMA Module - Free DMA Channel %d - START\n", ch + 1);
    #endif
    /* ===============DEBUG-END================ */

    /* grab free_list_lock */
    spin_lock_irqsave(&free_list_lock, irq_flag);

    switch(chan->type){
        case DMA_FULL_CHANNEL:
            free_list_fs[ch] = free_list_head_fs;
            free_list_head_fs = ch;
            break;
        case DMA_HALF_CHANNEL:
            ch -= DMA_HS_START;
            free_list_hs[ch] = free_list_head_hs;
            free_list_head_hs = ch;
            break;
        case DMA_VIRTUAL_FIFO:
        case DMA_VIRTUAL_FIFO_UART0:
        case DMA_VIRTUAL_FIFO_UART1:
        case DMA_VIRTUAL_FIFO_UART2:
        case DMA_VIRTUAL_FIFO_UART3:
        case DMA_VIRTUAL_FIFO_UART4:
        case DMA_VIRTUAL_FIFO_UART5:
            break;
    }

    dma_chan[ch].registered = DMA_FALSE;

    /* release free_list_lock */
    spin_unlock_irqrestore(&free_list_lock, irq_flag);

    mt_reset_dma(config);

    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        switch(chan->type){
            case DMA_FULL_CHANNEL:
                break;
            case DMA_HALF_CHANNEL:
                ch += DMA_HS_START;
                break;
            case DMA_VIRTUAL_FIFO:
                break;
            default:
                break;
        }
        printk("DMA Module - Free DMA Channel %d - END\n", ch + 1);
    #endif
    /* ===============DEBUG-END================ */

    return;
}

DMA_STATUS mt_config_dma(struct mt_dma_conf *config, DMA_CONF_FLAG flag){

    DMA_CHAN *chan = (DMA_CHAN *)config;

    u32 dma_con = 0x0;

    switch(flag){
        
        case ALL:

            switch(chan->type){
                case DMA_FULL_CHANNEL:
                    
                    __raw_writel(config->src, DMA_SRC(chan->baseAddr));
                    __raw_writel(config->dst, DMA_DST(chan->baseAddr));
                    __raw_writew(config->wppt, DMA_WPPT(chan->baseAddr));
                    __raw_writel(config->wpto, DMA_WPTO(chan->baseAddr));
                    __raw_writew(config->count, DMA_COUNT(chan->baseAddr));

                    dma_con |= config->mas;
                    if(config->wpen)
                        dma_con |= DMA_CON_WPEN;
                    if(config->wpsd)
                        dma_con |= DMA_CON_WPSD;
                    if(config->iten)
                        dma_con |= DMA_CON_ITEN;
                    dma_con |= config->burst;
                    if(config->dreq)
                        dma_con |= DMA_CON_DRQ;
                    if(config->dinc)
                        dma_con |= DMA_CON_DINC;
                    if(config->sinc)
                        dma_con |= DMA_CON_SINC;
                    dma_con |= config->size;
                    __raw_writel(dma_con, DMA_CON(chan->baseAddr));

                    __raw_writeb(config->limiter, DMA_LIMITER(chan->baseAddr));

                    break;
                    
                case DMA_HALF_CHANNEL:

                    __raw_writel(config->pgmaddr, DMA_PGMADDR(chan->baseAddr));
                    __raw_writew(config->wppt, DMA_WPPT(chan->baseAddr));
                    __raw_writel(config->wpto, DMA_WPTO(chan->baseAddr));
                    __raw_writew(config->count, DMA_COUNT(chan->baseAddr));

                    dma_con |= config->mas;
                    if(config->dir)
                        dma_con |= DMA_CON_DIR;
                    if(config->wpen)
                        dma_con |= DMA_CON_WPEN;
                    if(config->wpsd)
                        dma_con |= DMA_CON_WPSD;
                    if(config->iten)
                        dma_con |= DMA_CON_ITEN;
                    dma_con |= config->burst;
                    if(config->b2w)
                        dma_con |= DMA_CON_B2W;
                    if(config->dreq)
                        dma_con |= DMA_CON_DRQ;
                    if(config->dinc)
                        dma_con |= DMA_CON_DINC;
                    if(config->sinc)
                        dma_con |= DMA_CON_SINC;
                    dma_con |= config->size;
                    __raw_writel(dma_con, DMA_CON(chan->baseAddr));

                    __raw_writeb(config->limiter, DMA_LIMITER(chan->baseAddr));

                    break;
                    
                case DMA_VIRTUAL_FIFO:

                    __raw_writel(config->pgmaddr, DMA_PGMADDR(chan->baseAddr));
                    __raw_writew(config->count, DMA_COUNT(chan->baseAddr));

                    dma_con |= config->mas;
                    if(config->dir)
                        dma_con |= DMA_CON_DIR;
                    if(config->iten)
                        dma_con |= DMA_CON_ITEN;
                    dma_con |= config->burst;
                    if(config->dreq)
                        dma_con |= DMA_CON_DRQ;
                    if(config->dinc)
                        dma_con |= DMA_CON_DINC;
                    if(config->sinc)
                        dma_con |= DMA_CON_SINC;
                    dma_con |= config->size;
                    __raw_writel(dma_con, DMA_CON(chan->baseAddr));

                    __raw_writeb(config->limiter, DMA_LIMITER(chan->baseAddr));
                    __raw_writeb(config->altlen, DMA_ALTLEN(chan->baseAddr));
                    __raw_writew(config->ffsize, DMA_FFSIZE(chan->baseAddr));

                    break;
                    
                default:
                    printk(KERN_ERR"DMA Module - Unknown DMA Channel Type!!\n");
                    return DMA_FAIL;
            }
            
            return DMA_OK;
            
        case SRC:
            
            if(chan->type == DMA_FULL_CHANNEL){
                __raw_writel(config->src, DMA_SRC(chan->baseAddr));
            }
            else{
                __raw_writel(config->pgmaddr, DMA_PGMADDR(chan->baseAddr));
                if(config->dir == 1)
                    config->dir = 0;
            }
            
            return DMA_OK;
            
        case DST:
            
            if(chan->type == DMA_FULL_CHANNEL){
                __raw_writel(config->dst, DMA_DST(chan->baseAddr));
            }
            else{
                __raw_writel(config->pgmaddr, DMA_PGMADDR(chan->baseAddr));
                if(config->dir == 0)
                    config->dir = 1;
            }
            
            return DMA_OK;
            
        case SRC_AND_DST:

            if(chan->type != DMA_FULL_CHANNEL){
                printk(KERN_ERR"DMA Module - Operation Not Supported For The \
                Given Channel Type");
                return DMA_FAIL;
            }

            __raw_writel(config->src, DMA_SRC(chan->baseAddr));
            __raw_writel(config->dst, DMA_DST(chan->baseAddr));
            
            return DMA_OK;
            
        default:
            break;
    }

    return DMA_FAIL;
}

void mt_start_dma(struct mt_dma_conf *config){

    DMA_CHAN *chan = (DMA_CHAN *)config;

    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        printk("DMA Module - Start DMA Transfer (channel %d)\n", chan->chan_num);
    #endif
    /* ===============DEBUG-END================ */

    __raw_writel(DMA_STOP_BIT, DMA_START(chan->baseAddr));
    __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(chan->baseAddr));
    __raw_writel(DMA_START_BIT, DMA_START(chan->baseAddr));

    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        if(__raw_readl(DMA_START(chan->baseAddr))){
            printk("DMA Module - Start DMA Transfer: Success (channel %d)\n",chan->chan_num);
        }
        else{
            printk("DMA Module - Start DMA Transfer: Failed\n");
        }
    #endif
    /* ===============DEBUG-END================ */
    
    return;
}

void mt_stop_dma(struct mt_dma_conf *config){

     DMA_CHAN *chan = (DMA_CHAN *)config;

    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        printk("DMA Module - Stop DMA Transfer (channel %d)\n", chan->chan_num);
    #endif
    /* ===============DEBUG-END================ */

    __raw_writel(DMA_STOP_BIT, DMA_START(chan->baseAddr));
    __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(chan->baseAddr));

    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        printk("DMA Module - Stop DMA Transfer: Success (channel %d)\n", chan->chan_num);
    #endif
    /* ===============DEBUG-END================ */
    
    return;
}

void mt_reset_dma(struct mt_dma_conf *config){

    memset(config, 0, sizeof(struct mt_dma_conf));

    mt_config_dma(config, ALL);

    return;
}

DMA_STATUS mt_get_info(struct mt_dma_conf *config, INFO_TYPE type, INFO *info){

    DMA_CHAN *chan = (DMA_CHAN *)config;

    switch(type){
        case REMAINING_LENGTH:
            
            if(chan->type == DMA_VIRTUAL_FIFO){
                break;
            }
            *info = __raw_readw(DMA_RLCT(chan->baseAddr));
            
            return DMA_OK;
            
        case VF_READPTR:
            
            if(chan->type != DMA_VIRTUAL_FIFO){
                break;
            }
            *info = __raw_readl(DMA_RDPTR(chan->baseAddr));
            
            return DMA_OK;
            
        case VF_WRITEPTR:

            if(chan->type != DMA_VIRTUAL_FIFO){
                break;
            }
            *info = __raw_readl(DMA_WRPTR(chan->baseAddr));

            return DMA_OK;
            
        case VF_FFCNT:

            if(chan->type != DMA_VIRTUAL_FIFO){
                break;
            }
            *info = __raw_readw(DMA_FFCNT(chan->baseAddr));

            return DMA_OK;
            
        case VF_ALERT:

            if(chan->type != DMA_VIRTUAL_FIFO){
                break;
            }
            *info = __raw_readb(DMA_FFSTA(chan->baseAddr)) & DMA_FFSTA_ALT;
            
            return DMA_OK;
            
        case VF_EMPTY:

            if(chan->type != DMA_VIRTUAL_FIFO){
                break;
            }
            *info = __raw_readb(DMA_FFSTA(chan->baseAddr)) & DMA_FFSTA_EMPTY;

            return DMA_OK;
            
        case VF_FULL:

            if(chan->type != DMA_VIRTUAL_FIFO){
                break;
            }
            *info = __raw_readb(DMA_FFSTA(chan->baseAddr)) & DMA_FFSTA_FULL;

            return DMA_OK;

        case VF_PORT:

            if(chan->type != DMA_VIRTUAL_FIFO){
                break;
            }
            *info = DMA_VPORT_CH(chan->chan_num);

            return DMA_OK;
            
        default:
            break;
    }

    printk(KERN_ERR"DMA Module - Information Type Not Available!!\n");
    return DMA_FAIL;
}

static irqreturn_t dma_irq_handler(int irq, void *dev_id){

    int i;
    u32 glbsta_l = __raw_readl(DMA_GLBSTA_L);
    u32 glbsta_h = __raw_readl(DMA_GLBSTA_H);
    
    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        printk("DMA Module - ISR Start\n");
    #endif
    /* ===============DEBUG-END================ */
    
    /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
    printk("DMA MODULE - GLBSTA_L = %x\n", glbsta_l);
    printk("DMA MODULE - GLBSTA_H = %x\n", glbsta_h);
    #endif
    /* ===============DEBUG-END================ */
    
    for(i = 0; i < 15; i++){
        if(glbsta_l & DMA_GLBSTA_IT(i)){
           /* ===============DEBUG-START================ */
           #ifdef DMA_DEBUG
           printk("DMA Module - Interrupt Issued: %d\n", i + 1);
           #endif
           /* ===============DEBUG-END================ */
           if(dma_chan[i].config.callback){
               dma_chan[i].config.callback(dma_chan[i].config.data);
           }
           __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(dma_chan[i].baseAddr));

           #ifdef DMA_DEBUG
               glbsta_l = __raw_readl(DMA_GLBSTA_L);
               glbsta_h = __raw_readl(DMA_GLBSTA_H);
               printk("GLBSTA_L after ack: %x\n", glbsta_l);
               printk("GLBSTA_H after ack: %x\n", glbsta_h);
           #endif
        }    
    }

    for(i = 15; i < DMA_NO; i++){   
        if(glbsta_h & DMA_GLBSTA_IT(i - 15)){
           /* ===============DEBUG-START================ */
           #ifdef DMA_DEBUG
           printk("DMA Module - Interrupt Issued: %d\n", i + 1);
           #endif
           /* ===============DEBUG-END================ */
           if(dma_chan[i].config.callback){
               dma_chan[i].config.callback(dma_chan[i].config.data);
           }
           __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(dma_chan[i].baseAddr));

           #ifdef DMA_DEBUG
               glbsta_l = __raw_readl(DMA_GLBSTA_L);
               glbsta_h = __raw_readl(DMA_GLBSTA_H);
               printk("GLBSTA_L after ack: %x\n", glbsta_l);
               printk("GLBSTA_H after ack: %x\n", glbsta_h);
           #endif
        }
    }

     /* ===============DEBUG-START================ */
    #ifdef DMA_DEBUG
        printk("DMA Module - ISR END\n");
    #endif
    /* ===============DEBUG-END================ */

    return IRQ_HANDLED;
}

static int __init mt_init_dma(void){

    int i;
    struct DMA_CHAN *chan;

    spin_lock_init(&free_list_lock);

    /*
     * Full-size DMA channel: ch 1 ~ ch 8
     * Half-size DMA channel: ch 9 ~ ch 14
     * Virtual FIFO DMA channel: ch 15 ~ ch 20
     */

    for(i = 0; i < DMA_NO; i++)
    {
        
        chan = &(dma_chan[i]);

        chan->baseAddr = DMA_BASE_CH(i);
        chan->chan_num = i;
        chan->registered = DMA_FALSE;
          
        if(i < DMA_FULL_CHANNEL_NO){
            chan->type = DMA_FULL_CHANNEL;
        }
        else if(i < DMA_FULL_CHANNEL_NO + DMA_HALF_CHANNEL_NO){
            chan->type = DMA_HALF_CHANNEL;
        }
        else{
            chan->type = DMA_VIRTUAL_FIFO;
        }

        mt_reset_dma(&chan->config);
        mt_stop_dma(&chan->config); // Infinity added 
    }

    /* initialize free lists */

    free_list_head_fs = 0;
    for(i = 0; i < DMA_FULL_CHANNEL_NO; i++){
        free_list_fs[i] = i + 1;
    }

    free_list_head_hs = 0;
    for(i = 0; i < DMA_HALF_CHANNEL_NO; i++){
        free_list_hs[i] = i + 1;
    }

    if(request_irq(MT3351_DMA_IRQ_CODE, dma_irq_handler, IRQF_DISABLED, "DMA", \
    NULL)){
        printk(KERN_ERR"DMA IRQ LINE NOT AVAILABLE!!\n");
    }

    //2009/02/02, Kelvin modify for power management
    //PDN_Power_CONA_DOWN(PDN_PERI_DMA, DMA_FALSE);
    hwEnableClock(MT3351_CLOCK_DMA);

    

    return 0;
}

arch_initcall(mt_init_dma);

EXPORT_SYMBOL(mt_request_dma);
EXPORT_SYMBOL(mt_free_dma);
EXPORT_SYMBOL(mt_config_dma);
EXPORT_SYMBOL(mt_start_dma);
EXPORT_SYMBOL(mt_stop_dma);
EXPORT_SYMBOL(mt_reset_dma);
EXPORT_SYMBOL(mt_get_info);

