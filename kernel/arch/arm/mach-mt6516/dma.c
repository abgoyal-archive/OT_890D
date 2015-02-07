
#include <mach/mt6516_reg_base.h>
//TODO: include it once pdn and pmu are OK
#if 0
#include <mach/mt6516_pdn_sw.h>
#include <mach/mt6516_pmu_sw.h>
#endif
#include <mach/irqs.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>
#include <asm/tcm.h>

extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);

#define DMA_DEBUG   0

#if(DMA_DEBUG == 1)
#define dbgmsg printk
static void dbg_print_dma_info(struct mt_dma_conf *config);
#else
#define dbg_print_dma_info(...)
#define dbgmsg(...)
#endif

#define NR_DMA_FULL_CHANNEL       8
#define NR_DMA_HALF_CHANNEL       8
#define NR_DMA_VFIFO_CHANNEL      8
#define DMA_FS_START              0
#define DMA_HS_START              (DMA_FS_START + NR_DMA_FULL_CHANNEL)
#define DMA_VF_START              (DMA_HS_START + NR_DMA_HALF_CHANNEL)

#define NR_DMA (NR_DMA_FULL_CHANNEL + NR_DMA_HALF_CHANNEL + NR_DMA_VFIFO_CHANNEL)

#define UART0 (DMA_VF_START + 0)
#define UART1 (DMA_VF_START + 1)
#define UART2 (DMA_VF_START + 2)
#define UART3 (DMA_VF_START + 3)
#define UART4 (DMA_VF_START + 4)
#define UART5 (DMA_VF_START + 5)
    




#define DMA_BASE_CH(n)                  (DMA_BASE + 0x0080*(n + 1))

#define DMA_GLBSTA0                 	(DMA_BASE + 0x0000) 
#define DMA_GLBSTA1                 	(DMA_BASE + 0x0004)

#define DMA_VPORT_BASE                   0xF0110000
#define DMA_VPORT_CH(ch)                (DMA_VPORT_BASE + (ch - 16) * 0x00002000)
/* for full-size DMA channel only, _n = 1 ~ 8 */
#define DMA_SRC(base)                 	(base)           
#define DMA_DST(base)                 	(base + 0x0004)
#define DMA_WPPT(base)                	(base + 0x0008)    
#define DMA_WPTO(base)                	(base + 0x000c)    
#define DMA_COUNT(base)               	(base + 0x0010)   
#define DMA_CON(base)                 	(base + 0x0014)   
#define DMA_START(base)               	(base + 0x0018)   
#define DMA_INTSTA(base)              	(base + 0x001c)    
#define DMA_ACKINT(base)              	(base + 0x0020)    
#define DMA_RLCT(base)                	(base + 0x0024)    
#define DMA_LIMITER(base)             	(base + 0x0028)   
#define DMA_PGMADDR(base)             	(base + 0x002c)   


#define DMA_WRPTR(base)               	(base + 0x0030)
#define DMA_RDPTR(base)               	(base + 0x0034)
#define DMA_FFCNT(base)               	(base + 0x0038)
#define DMA_FFSTA(base)               	(base + 0x003c)
#define DMA_ALTLEN(base)              	(base + 0x0040)
#define DMA_FFSIZE(base)              	(base + 0x0044)



/* 2-1. DMA_GLBSTA */

#define DMA_GLBSTA_RUN(ch)          	(0x00000001 << (2*(ch))) 
#define DMA_GLBSTA_IT(ch)           	(0x00000002 << (2*(ch)))  
#define NR_INT_IN_GLBSTA0                16
#define NR_INT_IN_GLBSTA1                8

#define DMA_COUNT_MASK              	 0x0000ffff

/* 2-2. DMA_CON */
                    
/* SINC,DINC,DREQ */                                    
#define DMA_CON_SINC                	(0x1 << 2)
#define DMA_CON_DINC                	(0x1 << 3)
#define DMA_CON_DRQ                 	(0x1 << 4)  /*1:hw, 0:sw handshake*/
/* B2W */                                                                       
#define DMA_CON_B2W                 	(0x1 << 5)  /*word to byte or byte to word, only used in half size dma*/

/* ITEN,WPSD,WPEN,DIR, DREQ */                                   
#define DMA_CON_ITEN                	(0x1 << 15) /*Interrupt enable*/
#define DMA_CON_WPSD                	(0x1 << 16) /*0:at source, 1: at destination*/
#define DMA_CON_WPEN                	(0x1 << 17) /*0:disable, 1: enable*/
#define DMA_CON_DIR                 	(0x1 << 18) /*Only valid when dma = 9 ~ 20*/

/* 2-3. DMA_START,DMA_INTSTA,DMA_ACKINT */                                   
#define DMA_START_BIT               	(0x1 << 15)
#define DMA_STOP_BIT                	(~(DMA_START_BIT) & (DMA_START_BIT))//=0
#define DMA_ACKINT_BIT              	(0x1 << 15)
#define DMA_INTSTA_BIT              	(0x1 << 15)
/* 2-4 DMA_FFSTA MASK */
#define DMA_FFSTA_FULL                  (0x1 << 0)
#define DMA_FFSTA_EMPTY                 (0x1 << 1)
#define DMA_FFSTA_ALT                   (0x1 << 2)




    
typedef enum{
    DMA_FULL_CHANNEL = 0,
    DMA_HALF_CHANNEL,
    DMA_VIRTUAL_FIFO
}DMA_TYPE;

typedef struct DMA_CHAN{
    struct mt_dma_conf config;
    u32 baseAddr;
    DMA_TYPE type;
    u8 chan_num;
    unsigned int registered;
}DMA_CHAN;


static DMA_CHAN dma_chan[NR_DMA];

static spinlock_t free_list_lock;

static u8 free_list_head_fs;
static u8 free_list_fs[NR_DMA_FULL_CHANNEL];

static u8 free_list_head_hs;
static u8 free_list_hs[NR_DMA_HALF_CHANNEL];


struct mt_dma_conf *_allocate_dma(u8* free_list, u8 *free_list_head, DMA_CHAN *dma_chan_buf, int nr_dma)
{
    struct mt_dma_conf *config = NULL;
    unsigned long irq_flag;

    /* grab free_list_lock */
    spin_lock_irqsave(&free_list_lock, irq_flag);
    
    if(*free_list_head != nr_dma){
         config = &(dma_chan_buf[*free_list_head].config);
         dma_chan_buf[*free_list_head].registered = DMA_TRUE;
         *free_list_head = free_list[*free_list_head];
    }
    else
    {
        printk(KERN_ERR"DMA Module - All DMA channel were used!\n");
    }
    
    dbg_print_dma_info(config);
        
    /* release free_list_lock */
    spin_unlock_irqrestore(&free_list_lock, irq_flag);    
    
    return config;
}
struct mt_dma_conf *mt_request_full_size_dma(void)
{    
    dbgmsg("Full-Size DMA Channel\n");
    
    return _allocate_dma(free_list_fs, &free_list_head_fs, &dma_chan[DMA_FS_START], NR_DMA_FULL_CHANNEL);
}


struct mt_dma_conf *mt_request_half_size_dma(void)
{
    dbgmsg("Half-Size DMA Channel\n");
    
    return _allocate_dma(free_list_hs, &free_list_head_hs, &dma_chan[DMA_HS_START], NR_DMA_HALF_CHANNEL);
}

struct mt_dma_conf *mt_request_virtual_fifo_dma(unsigned int vf_dma_ch)
{
    struct mt_dma_conf *config = NULL;
    unsigned long irq_flag;
    unsigned ch = DMA_VF_START + vf_dma_ch;
    
    dbgmsg("Virtual fifo DMA channel %d\n", vf_dma_ch);
    
    /* grab free_list_lock */
    spin_lock_irqsave(&free_list_lock, irq_flag);

    if (ch >= NR_DMA)
    {
        printk(KERN_ERR"DMA Module - The requested virtual fifo %d DMA channel is not exist!!\n", vf_dma_ch);        
        return NULL;
    }        
    
    if(dma_chan[ch].registered == DMA_FALSE){
          config = &(dma_chan[ch].config);
          dma_chan[ch].registered = DMA_TRUE;
    }
    else
    {
        printk(KERN_ERR"DMA Module - The DMA channel %d was used!\n", ch);
    }
        
    dbg_print_dma_info(config);
        
    /* release free_list_lock */
    spin_unlock_irqrestore(&free_list_lock, irq_flag);

    return config;
}


void mt_free_dma(struct mt_dma_conf *config)
{

    DMA_CHAN *chan = (DMA_CHAN *)config;
    u8 ch = chan->chan_num;
    unsigned long irq_flag;
    
    dbgmsg("DMA Module - Free DMA Channel %d - START\n", chan->chan_num);
    
    /* grab free_list_lock */
    spin_lock_irqsave(&free_list_lock, irq_flag);

    if (chan->chan_num < NR_DMA_FULL_CHANNEL){
        free_list_fs[ch] = free_list_head_fs;
        free_list_head_fs = ch;        
    } 
    else if (chan->chan_num < (NR_DMA_FULL_CHANNEL + NR_DMA_HALF_CHANNEL)){
        ch -= DMA_HS_START;
        free_list_hs[ch] = free_list_head_hs;
        free_list_head_hs = ch;    
    }
    else if (chan->chan_num < (NR_DMA_FULL_CHANNEL + NR_DMA_HALF_CHANNEL + NR_DMA_VFIFO_CHANNEL))
    {
    
    }      
    else
    {
        printk(KERN_ERR"DMA Module - The DMA channel %d is not exist!\n", chan->chan_num);
    }
    
    dma_chan[chan->chan_num].registered = DMA_FALSE;

    mt_reset_dma(config);

    /* release free_list_lock */
    spin_unlock_irqrestore(&free_list_lock, irq_flag);

    /* ===============DEBUG-START================ */
    #if(DMA_DEBUG == 1)
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
        printk("DMA Module - Free DMA Channel %d - END\n", chan->chan_num);
    #endif
    /* ===============DEBUG-END================ */

    return;
}

extern void MT6516_traceCallStack(void);

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
                    printk("%u %d %d %d", chan->baseAddr, (int) chan->type, chan->chan_num, chan->registered);
                    printk("count = %d", chan->config.count);
                    printk("master = %u", chan->config.mas);
                    printk("burst = %u", chan->config.burst);
                    printk("iten, dreq, dinc, sinc = %d, %d, %d, %d",
                            chan->config.iten, chan->config.dreq, chan->config.dinc, chan->config.sinc);
                    printk("size = %d, limiter = %d\n",
                            chan->config.size, chan->config.limiter);
                    printk("data = %p, callback = %p\n",
                            chan->config.data, chan->config.callback);
                    printk("src = %u, dst = %u\n",
                            chan->config.src, chan->config.dst);
                    printk("wpen = %d, wpsd = %d\n",
                            chan->config.wpen, chan->config.wpsd);
                    printk("wppt = %d, wpto = %u, pgmaddr = %u\n",
                            chan->config.wppt, chan->config.wpto, chan->config.pgmaddr);
                    printk("dir = %d, b2w = %d\n",
                            chan->config.dir, chan->config.b2w);
                    printk("altlen = %d, ffsize = %d\n",
                            chan->config.altlen, chan->config.ffsize);
                    MT6516_traceCallStack();
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

    dbgmsg("DMA Module - Start DMA Transfer (channel %d)\n", chan->chan_num);

    __raw_writel(DMA_STOP_BIT, DMA_START(chan->baseAddr));
    __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(chan->baseAddr));
    __raw_writel(DMA_START_BIT, DMA_START(chan->baseAddr));

    /* ===============DEBUG-START================ */
    #if(DMA_DEBUG == 1)
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
         
    dbgmsg("DMA Module - Stop DMA Transfer (channel %d)\n", chan->chan_num);

    __raw_writel(DMA_STOP_BIT, DMA_START(chan->baseAddr));
    __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(chan->baseAddr));

    dbgmsg("DMA Module - Stop DMA Transfer: Success (channel %d)\n", chan->chan_num);
   
    return;
}

void mt_reset_dma(struct mt_dma_conf *config){

    memset(config, 0, sizeof(struct mt_dma_conf));

    if (mt_config_dma(config, ALL) != 0){
        return;
    }

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

static __tcmfunc irqreturn_t dma_irq_handler(int irq, void *dev_id){

    int i;
    DMA_CHAN *chan = NULL;
    u32 glbsta0 = __raw_readl(DMA_GLBSTA0);
    u32 glbsta1 = __raw_readl(DMA_GLBSTA1);
    
    dbgmsg("DMA Module - ISR Start\n");      
    dbgmsg("DMA MODULE - GLBSTA0 = %x\n", glbsta0);
    dbgmsg("DMA MODULE - GLBSTA1 = %x\n", glbsta1);
 
    for(i = 0; i < NR_INT_IN_GLBSTA0; i++){
        if(glbsta0 & DMA_GLBSTA_IT(i)){
      
           dbgmsg("DMA Module - Interrupt Issued: %d\n", i);
           
           chan = &dma_chan[i];
           
           if(chan->config.callback){
               chan->config.callback(chan->config.data);
           }
           __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(chan->baseAddr));

           #if(DMA_DEBUG == 1)
               glbsta0 = __raw_readl(DMA_GLBSTA0);
               glbsta1 = __raw_readl(DMA_GLBSTA1);
               printk("GLBSTA0 after ack: %x\n", glbsta0);
               printk("GLBSTA1 after ack: %x\n", glbsta1);
           #endif
        }    
    }

    for(i = NR_INT_IN_GLBSTA0; i <  NR_INT_IN_GLBSTA0 + NR_INT_IN_GLBSTA1; i++){   
        if(glbsta1 & DMA_GLBSTA_IT(i - NR_INT_IN_GLBSTA0)){
            
           dbgmsg("DMA Module - Interrupt Issued: %d\n", i);

           chan = &dma_chan[i];
           
           if(chan->config.callback){
               chan->config.callback(chan->config.data);
           }
           __raw_writel(DMA_ACKINT_BIT, DMA_ACKINT(chan->baseAddr));

           #if(DMA_DEBUG == 1)
               glbsta0 = __raw_readl(DMA_GLBSTA0);
               glbsta1 = __raw_readl(DMA_GLBSTA1);
               printk("GLBSTA0 after ack: %x\n", glbsta0);
               printk("GLBSTA1 after ack: %x\n", glbsta1);
           #endif
        }
    }
    
    dbgmsg("DMA Module - ISR END\n");
    
    return IRQ_HANDLED;
}

static int __init mt_init_dma(void){

    int i;
    struct DMA_CHAN *chan;

    spin_lock_init(&free_list_lock);


    for(i = 0; i < NR_DMA; i++)
    {
        
        chan = &(dma_chan[i]);

        chan->baseAddr = DMA_BASE_CH(i);

        chan->chan_num = i;
        chan->registered = DMA_FALSE;
          
        if(i < NR_DMA_FULL_CHANNEL){
            chan->type = DMA_FULL_CHANNEL;
        }
        else if(i < NR_DMA_FULL_CHANNEL + NR_DMA_HALF_CHANNEL){
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
    for(i = 0; i < NR_DMA_FULL_CHANNEL; i++){
        free_list_fs[i] = i + 1;
    }

    free_list_head_hs = 0;
    for(i = 0; i < NR_DMA_HALF_CHANNEL; i++){
        free_list_hs[i] = i + 1;
    }


    MT6516_IRQSensitivity(MT6516_DMA_IRQ_LINE, MT6516_LEVEL_SENSITIVE);

    if(request_irq(MT6516_DMA_IRQ_LINE, dma_irq_handler, IRQF_DISABLED, "DMA", \
    NULL)){
        printk(KERN_ERR"DMA IRQ LINE NOT AVAILABLE!!\n");
    }

    return 0;
}

#if(DMA_DEBUG == 1)

static void dbg_print_dma_info(struct mt_dma_conf *config)  
{
    if(config != NULL){
        DMA_CHAN *chan = (DMA_CHAN *)config;
        printk("DMA Module - Request Channel: Success\n");
        printk("DMA Module - The Granted Channel Number: %d\n", \
        chan->chan_num);
        printk("DMA Module - The Granted Channel Base: %x\n", chan->baseAddr);
    }
    else{
        printk("DMA Module - Request Channel: Failed\n");
    }

}
   
#endif

arch_initcall(mt_init_dma);

EXPORT_SYMBOL(mt_request_half_size_dma);
EXPORT_SYMBOL(mt_request_full_size_dma);
EXPORT_SYMBOL(mt_request_virtual_fifo_dma);
EXPORT_SYMBOL(mt_free_dma);
EXPORT_SYMBOL(mt_config_dma);
EXPORT_SYMBOL(mt_start_dma);
EXPORT_SYMBOL(mt_stop_dma);
EXPORT_SYMBOL(mt_reset_dma);
EXPORT_SYMBOL(mt_get_info);

