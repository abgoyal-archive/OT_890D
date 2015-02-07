
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/cdc.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/platform_device.h>

#include <linux/wakelock.h>
/* architecture dependent header files */
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_ap_config.h>
#include <mach/mt6516_reg_base.h>
#include <mach/irqs.h>
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include "mt6516_udc.h"
#include <mtk_usb_custom.h>
#include <asm/tcm.h>

/* Enable debug message with care!! */
/* Note that printk() will affect the transmission */
//#define USB_DEBUG

//#define USB_USE_DMA_MODE1
#define USB_USE_DMA_MODE0
//#define USB_USE_PIO_MODE

#define USB_LOG_ENABLE  0
#if USB_LOG_ENABLE
    #define USB_LOG     printk
#else
    #define USB_LOG     
#endif

#define IRQ_LOG_SIZE    65536

#define CHIP_VER_ECO_1 (0x8a00)
#define CHIP_VER_ECO_2 (0x8a01)

#define PMIC6326_ECO_1_VERSION		0x01
#define PMIC6326_ECO_2_VERSION		0x02
#define PMIC6326_ECO_3_VERSION		0x03
#define PMIC6326_ECO_4_VERSION		0x04

static const char driver_name[] = "mt_udc";

static struct mt_udc *udc;
extern UINT32 g_ChipVer;
extern UINT8 pmic6326_eco_version;
extern UINT32 g_USBStatus;

UINT8 g_usb_power_saving = 0;

int dma_active[MT_CHAN_NUM];
struct wake_lock usb_lock;

static const u8 mt_usb_test_packet[53] = {
        /* implicit SYNC then DATA0 to start */

        /* JKJKJKJK x9 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* JJKKJJKK x8 */
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        /* JJJJKKKK x8 */
        0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
        /* JJJJJJJKKKKKKK x8 */
        0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        /* JJJJJJJK x8 */
        0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd,
        /* JKKKKKKK x10, JK */
        0xfc, 0x7e, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0x7e

        /* implicit CRC16 then EOP to end */
};

#if USB_LOG_ENABLE
int irq_log[IRQ_LOG_SIZE];
unsigned int log_w = 0;
unsigned int log_r = 0;
#endif

extern kal_bool pmic_chrdet_status(void);
extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);

extern void BAT_SetUSBState(int usb_state_value);
extern void wake_up_bat (void);

/* ================== */
/* local declarations */
/* ================== */

static void irq_logging(int value);
static void irq_log_dump(void);

static inline int mt_udc_read_fifo(struct mt_ep *ep);
static inline int mt_udc_write_fifo(struct mt_ep *ep);

static void done(struct mt_ep *ep, struct mt_req *req, int status);

static void mt_ep0_handler(void);
static void mt_ep0_idle(void);
static void mt_ep0_rx(void);
static void mt_ep0_tx(void);
static void mt_epx_handler(int irq_src, USB_DIR dir);
static void mt_usb_reset(void);
static void mt_dev_connect(void);
static void mt_dev_disconnect(void);
static void mt_usb_load_testpacket(void);

static void mt_ep_fifo_flush(struct usb_ep *_ep);
static void mt_ep_fifo_flush_internal(struct usb_ep *_ep);

static irqreturn_t mt_udc_dma_handler(int irq_src);
static void mt_usb_config_dma(int channel, int burst_mode, int ep_num, int dma_mode, int dir, u32 addr, u32 count);

static void USB_UART_Share(u8 usb_mode);
static void USB_Charger_Detect_Init(void);
static void USB_Charger_Detect_Release(void);
static void USB_Check_Standard_Charger(void);

extern void mt_usb_phy_init(struct mt_udc *udc);
extern void mt_usb_phy_deinit(struct mt_udc *udc);

//u8 tx_set = 0;
kal_bool is_mp_ic(void){
    return ((pmic6326_eco_version == PMIC6326_ECO_4_VERSION) && (g_ChipVer == \
        CHIP_VER_ECO_2));
}


#if USB_LOG_ENABLE
static void irq_logging(int value){

    *(irq_log + log_w) = value;
    
    log_w++;
    
    if(log_w == IRQ_LOG_SIZE)
        log_w = 0;
    
    if(log_w == log_r)
        log_r++;

    if(log_r == IRQ_LOG_SIZE)
        log_r = 0;

    return;
}

static void irq_log_dump(){

    while(log_w != log_r){
        printk("%d", *(irq_log + log_r));
        log_r++;
        if(log_r == IRQ_LOG_SIZE)
            log_r = 0;
    }

    printk("\nirq log dump complete\n");
    
    return;
}
#else
void irq_logging(int value){
    return;
}

static void irq_log_dump(){
    return;
}
#endif

static void mt_usb_load_testpacket(void){
    u8 index;
    u8 i;

    index = __raw_readb(INDEX);
    __raw_writeb(0, INDEX);

    for(i = 0; i < sizeof(mt_usb_test_packet); i++){
        __raw_writeb(mt_usb_test_packet[i], FIFO(0));
    }

    __raw_writew(EP0_TXPKTRDY, IECSR + CSR0);

    __raw_writeb(index, INDEX);

    return;
}

/* must be called when udc lock is obtained */
static inline int mt_udc_read_fifo(struct mt_ep *ep){
    int count = 0;
    u8 ep_num;
    u8 index = 0;
    int len = 0;
    unsigned char *bufp = NULL;
    struct mt_req *req;

    if(!ep){
        printk("mt_udc_read_fifo, *ep null\n");
        return 0;
    }

    ep_num = ep->ep_num;

    req = container_of(ep->queue.next, struct mt_req, queue);

    if(list_empty(&ep->queue)){
        printk("[USB] SW buffer is not ready!!\n");
        return 0;
    }
   
    if(req){
        index = __raw_readb(INDEX);
        __raw_writeb(ep_num, INDEX);

        count = len = min((unsigned)__raw_readw(IECSR + RXCOUNT), req->req.length - req->req.actual);
        bufp = req->req.buf + req->req.actual;

        while(len > 0){
            *bufp = __raw_readb(FIFO(ep_num));
            bufp++;
            len--;
        }

        req->req.actual += count;

        __raw_writeb(index, INDEX);
    }
    else{// should not go here
         printk("[USB] ERROR: NO REQUEST\n");
    }

    return count;
}

/* must be called when udc lock is obtained */
static inline int mt_udc_write_fifo(struct mt_ep *ep){
    int count = 0;
    u8 ep_num;
    u8 index = 0;
    int len = 0;
    unsigned char *bufp = NULL;
    unsigned int maxpacket;
    struct mt_req *req;
   
    if(!ep){
        printk("mt_udc_write_fifo, *ep null\n");
        return 0;
    }

    ep_num = ep->ep_num;
 
    req = container_of(ep->queue.next, struct mt_req, queue);
    
    if(list_empty(&ep->queue)){
        printk("[USB] SW buffer is not ready!!\n");
        return 0;
    }

    if(req){
        index = __raw_readb(INDEX);
        __raw_writeb(ep_num, INDEX);
        
        maxpacket = ep->ep.maxpacket;

        count = len = min(maxpacket, req->req.length - req->req.actual);
        bufp = req->req.buf + req->req.actual;

        while(len > 0){
            __raw_writeb(*bufp, FIFO(ep_num));
            bufp++;
            len--;
        }
        req->req.actual += count;
        
        __raw_writeb(index, INDEX);
    }
    else{// should not go here
        printk("[USB] ERROR: NO REQUEST\n");
    }

    return count;
}

/* retire a request, removing it from the queue */
/* must be called when udc lock is obtained */
static void done(struct mt_ep *ep, struct mt_req *req, int status)
{   
    int locking;
    #if 0
    if(ep->ep_num == 1 || ep->ep_num == 3 || ep->ep_num == 5)
        printk("[USB] done, ep %d, req = %x, req->req.actual = %d\n", ep->ep_num, &req->req, req->req.actual);
    #endif
    if(!ep || !req){
        printk("[USB] done, invalid *ep or *req pointer!!\n");
        return;
    }

    if(req->queued <= 0){
        printk("[USB] done, EP%d, REQ_NOT_QUEUED, REQ_STAT = %d\n", ep->ep_num, req->req.status);
        return;
    }
    list_del(&req->queue);
    ep->busycompleting = 1;
    req->queued = 0;
    
    if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;

    locking = spin_is_locked(&udc->lock);
    if(locking){
        spin_unlock_irqrestore(&udc->lock, udc->flags);
    }
    
    if(req->req.complete)
        req->req.complete(&ep->ep, &req->req);
    
    if(locking)
        spin_lock_irqsave(&udc->lock, udc->flags);
    ep->busycompleting = 0;
    /*
    if(ep->ep_num == 1 || ep->ep_num == 3 || ep->ep_num == 5)
        printk("[USB] done(complete), ep%d, req->req.status = %d\n", ep->ep_num, req->req.status);
    */
    return;
}

/* ========================================== */
/* callback functions registered to mt_ep_eps */
/* ========================================== */

/* configure endpoint, making it usable */
static int mt_ep_enable(struct usb_ep *_ep, 
                        const struct usb_endpoint_descriptor *desc)
{
    struct mt_ep *ep;
    u16 maxp;
    u16 tmp;

    if(!_ep || !desc || !udc){
        printk("[USB] mt_ep_enable, invalid *_ep or *desc or *udc!!\n");
        return 0;
    }

    spin_lock_irqsave(&udc->lock, udc->flags);
    
    ep = container_of(_ep, struct mt_ep, ep);
    maxp = le16_to_cpu(desc->wMaxPacketSize);

    //printk("[USB] mt_ep_enable, ep%d\n", ep->ep_num);

    ep->desc = desc;
    ep->ep.maxpacket = maxp;
    
    //printk("mt_ep_enable: EP %d\n", ep->ep_num);

    __raw_writeb(ep->ep_num, INDEX);
    
    if(EP_IS_IN(ep)) /* TX */
    {
        /* flush fifo and clear data toggle */
        tmp = __raw_readw(IECSR + TXCSR);
        //tmp |= EPX_TX_FLUSHFIFO;
        tmp |= EPX_TX_CLRDATATOG;
        __raw_writew(tmp, IECSR + TXCSR);

        /* udpate max packet size to TXMAP */
        __raw_writew(maxp, IECSR + TXMAP);

        /* assign fifo address */
        //printk("TX FIFO ADDR = %x\n", udc->fifo_addr);
        __raw_writew((udc->fifo_addr) >> 3, TXFIFOADD);
        
        /* assign fifo size */
        tmp = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
        if(tmp == USB_ENDPOINT_XFER_BULK)
        {
            if(udc->gadget.speed == USB_SPEED_FULL){
                __raw_writeb(FIFOSZ_64 | DPB, TXFIFOSZ);
            }
            else if(udc->gadget.speed == USB_SPEED_HIGH){
                __raw_writeb(FIFOSZ_512 | DPB, TXFIFOSZ);
            }
            else{
                printk("[USB] Not Supported speed\n");
            }
            
            /* update global fifo address */
      		udc->fifo_addr += 2 * MT_BULK_MAXP;
        }
        else if(tmp == USB_ENDPOINT_XFER_INT)
        {
            /* for full speed */
            __raw_writeb(FIFOSZ_64 | DPB, TXFIFOSZ);
            /* update global fifo address */
            udc->fifo_addr += 2 * MT_INT_MAXP;
        }
		
		/* enable the interrupt */
		tmp = __raw_readb(INTRTXE);
		tmp |= EPMASK(ep->ep_num);
		__raw_writeb(tmp, INTRTXE);
		
    }
    else /* RX */
    {
        /* flush fifo and clear data toggle */
        tmp = __raw_readw(IECSR + RXCSR);
        //tmp |= EPX_RX_FLUSHFIFO;
        tmp |= EPX_RX_CLRDATATOG;
        __raw_writew(tmp, IECSR + RXCSR);

        /* udpate max packet size to RXMAP */
        __raw_writew(maxp, IECSR + RXMAP);

        /* assign fifo address */
        __raw_writew((udc->fifo_addr >> 3), RXFIFOADD);

        /* assign fifo size */
        tmp = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
        if(tmp == USB_ENDPOINT_XFER_BULK)
        {
            if(udc->gadget.speed == USB_SPEED_FULL){
                __raw_writeb(FIFOSZ_64 | DPB, RXFIFOSZ);
            }
            else if(udc->gadget.speed == USB_SPEED_HIGH){
                __raw_writeb(FIFOSZ_512 | DPB, RXFIFOSZ);
            }
            else{
                printk("[USB] Not supported speed\n");
            }
            
            /* update global fifo address */
    		udc->fifo_addr += 2 * MT_BULK_MAXP;
        }
        else if(tmp == USB_ENDPOINT_XFER_INT)
        {
            /* for full speed */
            __raw_writeb(FIFOSZ_64 | DPB, RXFIFOSZ);
            /* update global fifo address */
		    udc->fifo_addr += 2 * MT_INT_MAXP;
        }
		
		/* enable the interrupt */
		tmp = __raw_readb(INTRRXE);
		tmp |= EPMASK(ep->ep_num);
		__raw_writeb(tmp, INTRRXE);

    }

    spin_unlock_irqrestore(&udc->lock, udc->flags);

    return 0;
}

/* endpoint is no longer usable */
/* reset struct mt_ep */
static int mt_ep_disable(struct usb_ep *_ep)
{
    struct mt_ep *ep;
    struct mt_req *req = NULL;
    u8 tmp8;

    if(!_ep || !udc){
        printk("[USB] Invalid *_ep, *udc\n");
        return 0;
    }

    spin_lock_irqsave(&udc->lock, udc->flags);

    ep = container_of(_ep, struct mt_ep, ep);

    //printk("[USB] mt_ep_disable, ep%d\n", ep->ep_num);
    
    //mt_ep_fifo_flush_internal(_ep);

    tmp8 = __raw_readb(INTRTXE);
    tmp8 &= ~(EPMASK(ep->ep_num));
    __raw_writeb(tmp8, INTRTXE);

    tmp8 = __raw_readb(INTRRXE);
    tmp8 &= ~(EPMASK(ep->ep_num));
    __raw_writeb(tmp8, INTRRXE);

    while(!list_empty(&ep->queue)){
        req = container_of(ep->queue.next, struct mt_req, queue);

        if(!req){
            printk("[USB] mt_ep_disable, invalid *req\n");
            spin_unlock_irqrestore(&udc->lock, udc->flags);
            return 0;
        }

        done(ep, req, -ECONNRESET);
    }
    
    spin_unlock_irqrestore(&udc->lock, udc->flags);

    return 0;
}

/* allocate a request object to use with this endpoint */
static struct usb_request *mt_ep_alloc_request(struct usb_ep *_ep, unsigned 
gfp_flags)
{
    struct mt_req *req;

    //printk("mt_alloc_request\n");

    if(!(req = kmalloc(sizeof(struct mt_req), gfp_flags)))
    {
        printk("[USB] Request Allocation Failed\n");
        return NULL;
    }
    
    memset(req, 0, sizeof(struct mt_req));

    //printk("mt_ep_alloc_request, ep %s, req = %x\n", _ep->name, &req->req);

    return &req->req;
}

/* frees a request object */
static void mt_ep_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
    /* if the request is being transferred, then we must stop the activity */
    /* and remove the request from the queue of the endpoint, start the transfer */
    /* of the next request */
    struct mt_req *req;

    //printk("mt_free_request\n");

    if(!_ep || !_req){
        printk("[USB] mt_ep_free_request, invalid *_ep or !_req\n");
        return;
    }
    
    req = container_of(_req, struct mt_req, req);

    kfree(req);
    
    return;
}

/* queues(submits) an I/O request to an endpoint */
static int mt_ep_enqueue(struct usb_ep *_ep, struct usb_request *_req, unsigned gfp_flags)
{
    struct mt_ep *ep;
    struct mt_req *req;
    int ep_idle;
    int ep_num = 0;
    u16 csr;
    u8 index;

    if(!_req || !_ep || !udc){
        printk("[USB] mt_ep_enqueue, Invalid USB Enqueue Operation\n");
        return -1;
    }
    
    spin_lock_irqsave(&udc->lock, udc->flags);

    ep = container_of(_ep, struct mt_ep, ep);
    req = container_of(_req, struct mt_req, req);

    ep_num = ep->ep_num;

    ep_idle = list_empty(&ep->queue);
    /*
    if(ep->busycompleting)
        printk("[USB] ep %d is busy completing a transfer\n", ep_num);
    }
    */
    if(req->queued > 0){
        printk("[USB] Request for ep%d is already queued, req->queued = %d\n", ep_num, req->queued);
        if(ep_num != 0)
            done(ep, req, -ECONNRESET);
        spin_unlock_irqrestore(&udc->lock, udc->flags);
        return 0;
    }

    if((ep_num != 0) && !(ep->desc)){
        printk("Enqueue aborted, connection in reset\n");
        irq_logging(-6);
        spin_unlock_irqrestore(&udc->lock, udc->flags);
        return -ESHUTDOWN;
    }
    
    req->req.status = -EINPROGRESS;
    req->req.actual = 0;
    list_add_tail(&req->queue, &ep->queue);
    req->queued = 1;

    if((ep_num == 0) && (req->req.length == 0)){
        //printk("[USB] zero length control transfer\n");
        done(ep, req, 0);
        udc->ep0_state = EP0_IDLE;
        spin_unlock_irqrestore(&udc->lock, udc->flags);
        return 0;
    }

    if((ep_num == 0) && udc->ep0_state == EP0_TX){
        if(!ep_idle){
            spin_unlock_irqrestore(&udc->lock, udc->flags);
            return 0;
        }
        
        mt_ep0_tx();
    }

    if((ep_num != 0) && (ep->desc->bEndpointAddress & USB_DIR_IN)){
        /*printk("mt_ep_enqueue, tx, ep_num = %d, req = %x, req->length = %d, req->actual = %d\n", \
        ep_num, &req->req, req->req.length, req->req.actual); */
        if(ep_idle && (!ep->busycompleting)){
             
             index = __raw_readb(INDEX);
             __raw_writeb(ep_num, INDEX);

             #if defined USB_USE_DMA_MODE0
                 mt_usb_config_dma(ep_num, 3, ep_num, 0, 1, (u32)req->req.buf + req->req.actual, 
                 min((unsigned int)(ep->ep.maxpacket), req->req.length - req->req.actual));       
             #elif defined USB_USE_DMA_MODE1
                 csr = __raw_readw(IECSR + TXCSR);
                 csr |= EPX_TX_AUTOSET | EPX_TX_DMAREQEN | EPX_TX_DMAREQMODE;
                 __raw_writew(csr, IECSR + TXCSR);
                 mt_usb_config_dma(ep_num, 3, ep_num, 1, 1, (u32)req->req.buf, req->req.length);
             #elif defined USB_USE_PIO_MODE
                 mt_udc_write_fifo(ep);
                 csr = __raw_readw(IECSR + TXCSR);
                 csr |= EPX_TX_TXPKTRDY;
                 __raw_writew(csr, IECSR + TXCSR);
             #endif
             
             __raw_writeb(index, INDEX);
        }
    }

    if((ep_num != 0) && !(ep->desc->bEndpointAddress & USB_DIR_IN)){
        //USB_LOG("mt_ep_enqueue, rx\n");
        if(ep_idle  && (!ep->busycompleting)){
            index = __raw_readb(INDEX);
            __raw_writeb(ep_num, INDEX);

            csr = __raw_readw(IECSR + RXCSR);
            if(csr & EPX_RX_RXPKTRDY){

                mt_usb_config_dma(ep_num, 3, ep_num, 0, 0, (u32)req->req.buf + req->req.actual, \
                min((unsigned)__raw_readw(IECSR + RXCOUNT), req->req.length - req->req.actual));
            }

            __raw_writeb(index, INDEX);
        }
    }
    
    spin_unlock_irqrestore(&udc->lock, udc->flags);
    
    return 0;
}

/* dequeues(cancels, unlinks) an I/O request from an endoint */
static int mt_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
    struct mt_ep *ep;
    struct mt_req *req;

    if(!udc){
        printk("[USB] mt_ep_dequeue, invalid *udc!!\n");
        //return -EINVAL;
        return 0;
    }

    spin_lock_irqsave(&udc->lock, udc->flags);

    if(!_ep || !_req){
        //printk("[USB] mt_ep_dequeue, invalid dequeue operation, _ep = %x, _req = %x!!\n", (unsigned int)_ep, (unsigned int)_req);

        ep = container_of(_ep, struct mt_ep, ep);
        //printk("[USB] mt_ep_dequeue, ep_num = %d\n", ep->ep_num);
        spin_unlock_irqrestore(&udc->lock, udc->flags);
        //return -EINVAL;
        return 0;
    }

    ep = container_of(_ep, struct mt_ep, ep);

    list_for_each_entry(req, &ep->queue, queue)
    {
        if(&req->req == _req)
            break;
    }

    if(&req->req != _req)
    {
        spin_unlock_irqrestore(&udc->lock, udc->flags);
        return -EINVAL;
    }

    done(ep, req, -ECONNRESET);

    spin_unlock_irqrestore(&udc->lock, udc->flags);

    return 0;
}

/* sets the endpoint halt feature or */
/* clears endpoint halt, and resets toggle */
static int mt_ep_set_halt(struct usb_ep *_ep, int value)
{
    struct mt_ep *ep;
    u8 index;
    u16 csr;

    if (!_ep)
		return -EINVAL;

    spin_lock_irqsave(&udc->lock, udc->flags);

    ep = container_of(_ep, struct mt_ep, ep);

    index = __raw_readb(INDEX);
    __raw_writeb(ep->ep_num, INDEX);
    
    if(value && EP_IS_IN(ep)){
        csr = __raw_readw(IECSR + TXCSR);

        if(csr & EPX_TX_FIFONOTEMPTY){
            spin_unlock_irqrestore(&udc->lock, udc->flags);
            return -EAGAIN;
        }
        __raw_writeb(index, INDEX);
    }

    if(EP_IS_IN(ep)){
        csr = __raw_readw(IECSR + TXCSR);
        if(csr & EPX_TX_FIFONOTEMPTY)
            csr |= EPX_TX_FLUSHFIFO;
        csr |= EPX_TX_WZC_BITS;
        csr |= EPX_TX_CLRDATATOG;
        if(value){
            csr |= EPX_TX_SENDSTALL;
        }
        else{
            csr &= ~(EPX_TX_SENDSTALL | EPX_TX_SENTSTALL);
            csr &= ~EPX_TX_TXPKTRDY;
        }
        __raw_writew(csr, IECSR + TXCSR);
    }
    else{
        csr = __raw_readw(IECSR + RXCSR);
        csr |= EPX_RX_WZC_BITS;
        csr |= EPX_RX_FLUSHFIFO;
        csr |= EPX_RX_CLRDATATOG;

        if(value){
            csr |= EPX_RX_SENDSTALL;
        }
        else{
            csr &= ~(EPX_RX_SENDSTALL | EPX_RX_SENTSTALL);
        }

        __raw_writew(csr, IECSR + RXCSR);
    }

    if(!list_empty(&ep->queue) && !ep->busycompleting){
        if(EP_IS_IN(ep))
            mt_epx_handler(ep->ep_num, USB_TX);
        else
            mt_epx_handler(ep->ep_num, USB_RX);        
    }

    __raw_writeb(index, INDEX);

    spin_unlock_irqrestore(&udc->lock, udc->flags);

    return 0;
}

/* returns number of bytes in fifo, or error */
static int mt_ep_fifo_status(struct usb_ep *_ep)
{

    struct mt_ep *ep;
    int count;
    u8 index;

    if(!_ep){
        printk("[USB] mt_ep_fifo_status, invalid *_ep\n");
        return 0;
    }

    spin_lock_irqsave(&udc->lock, udc->flags);

    count = 0;
    
    ep = container_of(_ep, struct mt_ep, ep);

    index = __raw_readb(INDEX);
    __raw_writeb(ep->ep_num, INDEX);
    
    if(ep->ep_num == 0)
    {
        count = __raw_readb(IECSR + COUNT0);
    }
    else
    {
        count = __raw_readw(IECSR + RXCOUNT);
    }

    __raw_writeb(index, INDEX);

    spin_unlock_irqrestore(&udc->lock, udc->flags);

    return count;
}

static void mt_ep_fifo_flush(struct usb_ep *_ep)
{
    if(!_ep){
        printk("[USB] mt_ep_fifo_flush, invalid *_ep\n");
        return;
    }

    spin_lock_irqsave(&udc->lock, udc->flags);
    mt_ep_fifo_flush_internal(_ep);
    spin_unlock_irqrestore(&udc->lock, udc->flags);
 
    return;
}

/* flushes contents of a fifo, doe */
static void mt_ep_fifo_flush_internal(struct usb_ep *_ep)
{
    struct mt_ep *ep;
    u16 csr;
    u8 index;

    if(!_ep){
        printk("[USB] mt_ep_fifo_flush_internal, invalid *_ep\n");
        return;
    }

    ep = container_of(_ep, struct mt_ep, ep);

    //printk("mt_ep_fifo_flush_internal, ep%d\n", ep->ep_num);

    /* write into INDEX register to select the endpoint mapped */
    index = __raw_readb(INDEX);
    __raw_writeb(ep->ep_num, INDEX);

    /* the structure of different types of endpiont are different */
    if(ep->ep_num == 0)
    {
        //csr = __raw_readw(IECSR + CSR0);
        csr = EP0_FLUSH_FIFO;
        __raw_writew(csr, IECSR + CSR0);
        __raw_writew(csr, IECSR + CSR0);
    }
    else
    {
        //if(EP_IS_IN(ep))
        //{
            //csr = __raw_readw(IECSR + TXCSR);
            csr = EPX_TX_FLUSHFIFO;
            __raw_writew(csr, IECSR + TXCSR);
            __raw_writew(csr, IECSR + TXCSR);
        //}
        //else
        //{
            //csr = __raw_readw(IECSR + RXCSR);
            csr = EPX_RX_FLUSHFIFO;
            __raw_writew(csr, IECSR + RXCSR);
            __raw_writew(csr, IECSR + RXCSR);
        //}
    }

    __raw_writeb(index, INDEX);

    return;
}

/* ========= */
/* mt_ep_ops */
/* ========= */

static struct usb_ep_ops mt_ep_ops = 
{
    .enable = mt_ep_enable,
    .disable = mt_ep_disable,

    .alloc_request = mt_ep_alloc_request,
    .free_request = mt_ep_free_request,

    .queue = mt_ep_enqueue,
    .dequeue = mt_ep_dequeue,

    .set_halt = mt_ep_set_halt,
    .fifo_status = mt_ep_fifo_status,
    .fifo_flush = mt_ep_fifo_flush,
    
};

/* ============================================== */
/* callback functions registered to mt_gadget_eps */
/* ============================================== */

/* returns the current frame number */
static int mt_get_frame(struct usb_gadget *gadget)
{
    return __raw_readw(FRAME);
}

/* tries to wake up the host connected to this gadget */
static int mt_wakeup(struct usb_gadget *gadget)
{
    return -ENOTSUPP;
}

/* sets or cleares the device selpowered feature */
/* called by usb_gadget_set_selfpowered and usb_gadget_clear_selfpowered */
static int mt_set_selfpowered(struct usb_gadget *gadget, int is_selfpowered)
{
    /* not supported now */

    return -ENOTSUPP;
}

/* notify the controller that VBUS is powered or VBUS session end */
/* called by usb_gadget_vbus_connect and usb_gadget_vbus_disconnect */
static int mt_vbus_session(struct usb_gadget *gadget, int is_active)
{
    /* not supported now */
    return -ENOTSUPP;
}

/* constrain controller's VBUS power usage */
static int mt_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
    /* not supported now */
    return -ENOTSUPP;
}

/* software-controlled connect/disconnect from USB host */
/* called by usb_gadget_connect and usb_gadget_disconnect_gadget */
static int mt_pullup(struct usb_gadget *gadget, int is_on)
{
    if(is_on){
        if(!g_usb_power_saving){
            mt_dev_connect();
        }
        else{
            if(pmic_chrdet_status()){
                switch(mt6516_usb_charger_type_detection()){
                    case STANDARD_HOST:
                        mt_dev_connect();
                        break;
                    case CHARGER_UNKNOWN:
                    case STANDARD_CHARGER:
                    case NONSTANDARD_CHARGER:
                    case CHARGING_HOST:
                    default:
                        break;
                }
            }
        }
    }
    else{
        mt_dev_disconnect();
    }
    
    return 0;
}

/* ============= */
/* mt_gadget_ops */
/* ============= */

/* except for get_frame, most of the callback functions are not implemented */
struct usb_gadget_ops mt_gadget_ops = 
{
    .get_frame = mt_get_frame,
    .wakeup = mt_wakeup,
    .set_selfpowered = mt_set_selfpowered,
    .vbus_session = mt_vbus_session,
    .vbus_draw = mt_vbus_draw,
    .pullup = mt_pullup,
    .ioctl = NULL,
};

/* =================================================================== */
/* These are the functions that bind peripheral controller drivers and */
/* gadget drivers together                                             */
/* =================================================================== */

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
    /* declaration is in usb_gadget.h, but implementation is here */
    /* binds gadget driver and peripheral controller driver, and then calls */
    /* bind() callback function registered by gadget driver */
    
    int retval;
    
    if(!driver || !driver->bind || !driver->disconnect || !driver->setup)
        return -EINVAL;

    spin_lock_irqsave(&udc->lock, udc->flags);

    if(!udc){
        spin_unlock_irqrestore(&udc->lock, udc->flags);
        return -ENODEV;
    }

    if(udc->driver){
        printk("[USB] A driver has been associated with MT_UDC\n");
        spin_unlock_irqrestore(&udc->lock, udc->flags);
        return -EBUSY;
    }

    /* hook up gadget driver and peripheral controller driver */
    udc->driver = driver;
    //udc->gadget.dev.driver = &driver->driver;
    spin_unlock_irqrestore(&udc->lock, udc->flags);
    retval = device_add(&udc->gadget.dev);
    
    /* may call udc related functions in external callback functions */
   
    retval = driver->bind(&udc->gadget);
    
    spin_lock_irqsave(&udc->lock, udc->flags);
    
    if(retval)
    {
        printk("[USB] Error: %s binds to %s => FAIL\n", udc->gadget.name, 
        driver->driver.name);
        device_del(&udc->gadget.dev);

        udc->driver = NULL;
        udc->gadget.dev.driver = NULL;

        spin_unlock_irqrestore(&udc->lock, udc->flags);
        return retval;
    }

    spin_unlock_irqrestore(&udc->lock, udc->flags);

    if(!g_usb_power_saving){
        mt_dev_connect();
    }
    else{
        if(pmic_chrdet_status()){
            switch(mt6516_usb_charger_type_detection()){
                case STANDARD_HOST:
                    mt_dev_connect();
                    break;
                case CHARGER_UNKNOWN:
                case STANDARD_CHARGER:
                case NONSTANDARD_CHARGER:
                case CHARGING_HOST:
                default:
                    break;
            }
        }
    }
       
    return 0;
}

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
    /* declaration is in usb_gadget.h, but implementation is here */
    /* unbinds gadget driver and peripheral controller driver, and then calls */
    /* unbind() callback function registered by gadget driver */
    //int i;

    if(!udc)
        return -ENODEV;

    if(!driver || driver != udc->driver)
        return -EINVAL;

    spin_lock_irqsave(&udc->lock, udc->flags);

    mt_dev_disconnect();
   
    udc->driver = NULL;
    /*
    for(i = 0; i < MT_EP_NUM; i++)
    {
        mt_ep_fifo_flush_internal(&udc->ep[i].ep);
    }
    */
    spin_unlock_irqrestore(&udc->lock, udc->flags);
 
    driver->unbind(&udc->gadget);
    
    device_del(&udc->gadget.dev);
    printk("[USB] Unregistered gadget driver '%s'\n", driver->driver.name);
    
    return 0;
}

/* ==================================================== */
/* interrut service routine when interrupts are issued  */
/* there are two interrupt sources in mt3351 USB module */
/* one for USB, one for USB_DMA                             */
/* ==================================================== */

static __tcmfunc irqreturn_t mt_udc_irq(int irq, void *dev_id)
{
    irqreturn_t status;
    unsigned int ep;

    u8 intrtx, intrrx, intrusb;

    //printk("mt_udc_irq\n");
    irq_logging(1);

    ep = 0;
    status = IRQ_HANDLED;

    if(!udc->driver){
        return status;
    }

    //printk("USB: INTERRUPT, START\n");
    
    /* find the interrupt source and call the corresonding handler */
    /* RX interrupts must be stored and INTRRX cleared immediately */
    /* RX interrupts will be lost if INTRRX not cleared and RX interrupt is 
    issued */
    
    intrtx =  __raw_readb(INTRTX);
    intrrx =  __raw_readb(INTRRX);
    __raw_writeb(~intrrx, INTRRX);
    intrusb = __raw_readb(INTRUSB);

    intrtx &= __raw_readb(INTRTXE);
    intrrx &= __raw_readb(INTRRXE);
    intrusb &= __raw_readb(INTRUSBE);
    
    /* print error message and enter an infinite while loop */
    if(intrusb & INTRUSB_VBUS_ERROR)
    {
        //printk("INTRUSB: VBUS ERROR\n");
    }

    /* the device receives reset signal and call reset function */
    if(intrusb & INTRUSB_RESET)
    {
        //printk("INTRUSB: RESET\n");
        wake_lock(&usb_lock);
        mt_usb_reset();
        status = IRQ_HANDLED;
    }
   
    if(intrusb & INTRUSB_SESS_REQ)
    {
        //printk("INTRUSB: SESS_REQ\n");
        status = IRQ_HANDLED;
    }
    if(intrusb & INTRUSB_DISCON)
    {
        //printk("INTRUSB: DISCON\n");
        wake_unlock(&usb_lock);
        status = IRQ_HANDLED;
    }
    if(intrusb & INTRUSB_CONN) /* only valid in host mode */
    {
        //printk("INTRUSB: CONN\n");
        status = IRQ_HANDLED;
    }
    if(intrusb & INTRUSB_SOF)
    {
        //printk("INTRUSB: SOF\n");
        status = IRQ_HANDLED;
    }
    if(intrusb & INTRUSB_RESUME)
    {
        //printk("INTRUSB: RESUME\n");
        status = IRQ_HANDLED;
    }
    if(intrusb & INTRUSB_SUSPEND)
    { 
        //printk("INTRUSB: SUSPEND\n");
        irq_log_dump();
        if(!g_usb_power_saving){
            mt_dev_disconnect();
            mt_dev_connect();
        }

        #ifndef CONFIG_MT6516_EVB_BOARD
        BAT_SetUSBState(USB_SUSPEND);
        wake_up_bat();
        #endif
        
        wake_unlock(&usb_lock);
		wake_lock_timeout(&usb_lock, 5 * HZ);	   
        
        status = IRQ_HANDLED;
    }
    
    /* the interrupt source is endpoint 0 and calls the state handler */
    if(intrtx)
    { 
        if(intrtx & EPMASK(0))
        {
            mt_ep0_handler();
            status = IRQ_HANDLED;
        }
        
        for(ep = 1; ep < MT_EP_NUM; ep++)
        {
            if(intrtx & EPMASK(ep))
            {
                mt_epx_handler(ep, USB_TX);
                status = IRQ_HANDLED;
            }
        }
    }

    if(intrrx)
    {
        for(ep = 1; ep < MT_EP_NUM; ep++)
        {
            if(intrrx & EPMASK(ep))
            {
                mt_epx_handler(ep, USB_RX);
                status = IRQ_HANDLED;
            }
        }
    }

    irq_logging(-1);
    return status;
} 

static __tcmfunc irqreturn_t mt_udc_dma_irq(int irq, void *_mt_udc)
{
    irqreturn_t status;
    unsigned int chan;
    u32 dma_intr;

    status = IRQ_NONE;
    chan = 0;
    if(!udc){
        printk("[USB] mt_udc_dma_irq, invalid *udc\n");
        return status;
    }

    //printk("mt_udc_dma_irq\n");
    irq_logging(2);
    /* find the interrupt source and call the associated handler */
    dma_intr = __raw_readb(DMA_INTR);
    __raw_writeb(~dma_intr, DMA_INTR);
    
    for(chan = 1; chan <= MT_CHAN_NUM; chan++){
        if(dma_intr & CHANMASK(chan)){
           //USB_LOG("mt_udc_dma_irq: chan %d\n", chan);
           status = mt_udc_dma_handler(chan);
        }
    }

    irq_logging(-2);
    return status;
}

/* initialize all data structures */
static void mt_gadget_init(struct device *dev){

    int i;

    if(!udc){
        printk("mt_gadget_init, invalid *udc\n");
        return;
    }
    /* initialize endpoint data structures */
    /* ==================================================== */
    /* Endpoint 0(default control endpoint)                 */
    /* ==================================================== */
    udc->ep[0].ep_num = 0;
    /* initialize struct usb_ep part */
    udc->ep[0].ep.driver_data = udc;
    udc->ep[0].ep.ops = &mt_ep_ops;
    INIT_LIST_HEAD(&udc->ep[0].ep.ep_list);
    udc->ep[0].ep.maxpacket = MT_EP0_FIFOSIZE;
    INIT_LIST_HEAD(&udc->ep[0].queue); /* initialize request queue */
    udc->ep[0].desc = NULL;
    udc->ep[0].busycompleting = 0;

    /* ==================================================== */
    /* Endpoint 1 ~ 4                                       */
    /* ==================================================== */
    for(i = 1; i < MT_EP_NUM; i++){
        udc->ep[i].ep_num = i;
        /* initialize struct usb_ep part */
        udc->ep[i].ep.driver_data = udc;
        udc->ep[i].ep.ops = &mt_ep_ops;
        INIT_LIST_HEAD(&udc->ep[i].ep.ep_list);
        udc->ep[i].ep.maxpacket = MT_BULK_MAXP;
        INIT_LIST_HEAD(&udc->ep[i].queue); /* initialize request queue */
        //printk("ep %d, queue = %x\n", i, &udc->ep[i].queue);
        udc->ep[i].desc = NULL;
        udc->ep[i].busycompleting = 0;
    }
    udc->ep[MT_EP_NUM - 1].ep.maxpacket = MT_INT_MAXP;
    udc->ep[0].ep.name = "ep0-control";
    udc->ep[1].ep.name = "ep1out-bulk";
    udc->ep[2].ep.name = "ep2out-bulk";
    udc->ep[3].ep.name = "ep3in-bulk";
    udc->ep[4].ep.name = "ep4in-bulk";
    udc->ep[5].ep.name = "ep5in-int";

    /* initialize udc data structure */
    udc->gadget.ops = &mt_gadget_ops;
    udc->gadget.ep0 = &udc->ep[0].ep;
    INIT_LIST_HEAD(&udc->gadget.ep_list);
    udc->gadget.speed = USB_SPEED_FULL;
    udc->gadget.is_dualspeed = 1;
    udc->gadget.is_otg = 0;
    udc->gadget.is_a_peripheral = 1;
    udc->gadget.b_hnp_enable = 0;
    udc->gadget.a_hnp_support = 0;
    udc->gadget.a_alt_hnp_support = 0;
    udc->gadget.name = driver_name;

    udc->ep0_state = EP0_IDLE;
    udc->faddr = 0;
    udc->fifo_addr = FIFO_START_ADDR;
    udc->driver = NULL;
    udc->set_address = 0;
    udc->test_mode = 0;
    udc->test_mode_nr = 0;
    udc->power = USB_FALSE;
    udc->ready = USB_FALSE;
    udc->charger_type = CHARGER_UNKNOWN;
    udc->udc_req = kmalloc(sizeof(struct mt_req), GFP_KERNEL);
    memset(udc->udc_req, 0, sizeof(struct mt_req));
    udc->udc_req->req.buf = kmalloc(2, GFP_KERNEL);

    device_initialize(&udc->gadget.dev);
    strcpy(udc->gadget.dev.bus_id, "gadget");

    spin_lock_init(&udc->lock);

    /* build ep_list structures */
    for(i = 0; i < MT_EP_NUM; i++)
    {
        struct mt_ep *ep = &udc->ep[i];
        list_add_tail(&ep->ep.ep_list, &udc->gadget.ep_list);
    }

    dev_set_drvdata(dev, udc);


    return;
}

/* ============================================== */
/* callback functions registered to mt_udc_driver */
/* ============================================== */

static int mt_udc_probe(struct device *dev)
{
    int status;


    udc = kmalloc(sizeof(struct mt_udc), GFP_KERNEL);
    if(!udc){
        printk("[USB] udc alloation failed\n");
        return -1;
    }

    memset(udc, 0, sizeof(struct mt_udc));

    mt_gadget_init(dev);
    
    __raw_readb(INTRTX);
    __raw_writeb(0, INTRRX);
    __raw_readb(INTRUSB);
    __raw_writeb(0, DMA_INTR);
    /* request irq line for both USB and USB_DMA */
    status = request_irq(MT6516_USB_IRQ_LINE, mt_udc_irq, IRQF_DISABLED, "MT6516_USB", NULL);

    if(status)
    {
        kfree(udc);
        udc = NULL;
        return -1;
    }
    
    status = request_irq(MT6516_USBDMA_IRQ_LINE, mt_udc_dma_irq, IRQF_DISABLED, "MT6516_USB_DMA", \
    NULL);

    if(status)
    {
        kfree(udc);
        udc = NULL;
        return -1;
    }

    udc->ready = USB_TRUE;

    #if defined CONFIG_MT6516_EVB_BOARD
        g_usb_power_saving = 0;
    #else
        g_usb_power_saving = is_mp_ic();
    #endif

    wake_lock_init(&usb_lock, WAKE_LOCK_SUSPEND, "USB suspend lock");

    if(!g_usb_power_saving){
        /* always enable usb power for evb load */
        /* disable usb power down */
        hwEnableClock(MT6516_PDN_PERI_USB,"USB");
    }

    /* always power on VUSB */
    hwPowerOn(MT6516_POWER_VUSB, VOL_3300,"USB");

    return status;
}

static void mt_dev_connect()
{
    int i;
    u8  tmpReg8;

    if(!udc || (udc->power == USB_TRUE))
        return;

    mt_usb_phy_init(udc);

    __raw_writeb(0x0, INTRTXE);
    __raw_writeb(0x0, INTRRXE);
    __raw_writeb(0x0, INTRUSBE);
    
    __raw_readb(INTRTX);
    __raw_writeb(0, INTRRX);
    __raw_readb(INTRUSB);

    /* reset INTRUSBE */
    __raw_writeb(INTRUSB_RESET, INTRUSBE);

    /* connect */
    tmpReg8 = __raw_readb(POWER);   
    #ifdef USB_FORCE_FULL_SPEED
    tmpReg8 &= ~PWR_HS_ENAB;
    #else
    tmpReg8 |= PWR_HS_ENAB;
    #endif
    tmpReg8 |= PWR_SOFT_CONN;
    tmpReg8 |= PWR_ENABLE_SUSPENDM;
    __raw_writeb(tmpReg8, POWER);

    for(i = 0; i < MT_CHAN_NUM; i++){
        dma_active[i] = 0;
    }

    return;
}

static void mt_dev_disconnect()
{
    int i;
    struct mt_ep *ep0;
    
    if(!udc || (udc->power == USB_FALSE))
        return;

    ep0 = &udc->ep[0];

    /* reinitialize ep0 */
    mt_ep_fifo_flush_internal(&ep0->ep);
    
    while(!list_empty(&ep0->queue)){
        struct mt_req *req0 = container_of(ep0->queue.next, struct mt_req, queue);
        list_del(&req0->queue);
        req0->queued--;
    }

    mt_usb_phy_deinit(udc);
    
    if(udc->driver)
        udc->driver->disconnect(&udc->gadget);
    
    udc->faddr = 0;
    udc->fifo_addr = FIFO_START_ADDR;
    udc->ep0_state = EP0_IDLE;
    //printk("3\n");
    for(i = 1; i < MT_EP_NUM; i++)
    {
        mt_ep_fifo_flush_internal(&udc->ep[i].ep);
    }

    __raw_readb(INTRTX);
    __raw_writeb(0, INTRRX);
    __raw_readb(INTRUSB);
    
     /* diable interrupts */
    __raw_writeb(0, INTRTXE);
    __raw_writeb(0, INTRRXE);
    __raw_writeb(INTRUSB_RESET, INTRUSBE);

    /* reset function address to 0 */
    __raw_writeb(0, FADDR);
    //printk("4\n");

    /* clear all dma channels */
    for(i = 1; i <= MT_CHAN_NUM; i++){
        __raw_writew(0, USB_DMA_CNTL(i));
        __raw_writel(0, USB_DMA_ADDR(i));
        __raw_writel(0, USB_DMA_COUNT(i));
    }
    
    return;
}

static int mt_udc_remove(struct device *_dev)
{
    struct mt_udc *udc = 0;

    if(!_dev){
        printk("[USB] mt_udc_remove, invalid *_dev\n");
        return -1;
    }

    udc = _dev->driver_data;
    
    if(!udc){
        printk("[USB] mt_udc_remove, invalid *udc\n");
        return -1;
    }

    /* reset the function address to zero */
    __raw_writeb(0, FADDR);

    if(udc->driver){
        usb_gadget_unregister_driver(udc->driver);
    }
    else{
        printk("[USB] mt_udc_remove, invalid udc->driver\n");
        return -1;
    }

    free_irq(MT6516_USB_IRQ_LINE, udc);
    free_irq(MT6516_USBDMA_IRQ_LINE, udc);

    dev_set_drvdata(_dev, NULL);

    udc = NULL;

    return 0;
}

void mt_udc_shutdown(struct device *dev){

    mt6516_usb_disconnect();
    
    return;
}

/* do nothing due to usb power management is done by other way */
static int mt_udc_suspend(struct device *dev, pm_message_t state)
{
    return 0;
}

/* do nothing due to usb power management is done by other way */
static int mt_udc_resume(struct device *dev)
{
    return 0;
}

/* ================================ */
/* device driver for the mt_udc */
/* ================================ */

static struct platform_driver mt_udc_driver =
{
    .driver     = {
        .name = (char *)driver_name,
        .bus = &platform_bus_type,
        .probe = mt_udc_probe,
        .remove = mt_udc_remove,
        .shutdown = mt_udc_shutdown,
        .suspend = mt_udc_suspend,
        .resume = mt_udc_resume,
    }    
};

/* ===================================================== */
/* handlers which service interrupts originated from USB */
/* ===================================================== */

/* if bit 0 of INTRTX is set when USB issues an interrupt, this function will */
/* be called to handler it. */
static void mt_ep0_handler()
{
    u16 csr0;
    u8 index;
    u8 has_error = 0;

    if(!udc){
        printk("[USB] mt_ep0_handler, invalid *udc\n");
        return;
    }

    index = __raw_readb(INDEX);
    __raw_writeb(0, INDEX);

    csr0 = __raw_readw(IECSR + CSR0);

    if(csr0 & EP0_SENTSTALL){
        printk("[USB] EP0: STALL\n");
        /* needs to implement exception handling here */
        __raw_writew(csr0 &~ EP0_SENTSTALL, IECSR + CSR0);
        udc->ep0_state = EP0_IDLE;
        has_error = 1;
    }

    if(csr0 & EP0_SETUPEND){
        printk("[USB] EP0: SETUPEND\n");
        __raw_writew(csr0 | EP0_SERVICE_SETUP_END, IECSR + CSR0);
        udc->ep0_state = EP0_IDLE;
        has_error = 1;
    }

    if(has_error){
        if(!(csr0 & EP0_RXPKTRDY)){
            return;
        }
    }

    switch(udc->ep0_state){
        case EP0_IDLE:
            if(udc->set_address){
                udc->set_address = 0;
                __raw_writeb(udc->faddr, FADDR);
                __raw_writeb(index, INDEX);
                return;
            }

            if(udc->test_mode){

                #ifndef CONFIG_MT6516_EVB_BOARD
                BAT_SetUSBState(USB_SUSPEND);
                wake_up_bat();
                #endif
                
                if(udc->test_mode_nr == USB_TST_TEST_PACKET);
                    mt_usb_load_testpacket();

                __raw_writeb(udc->test_mode_nr, TESTMODE);
                printk("Enter test mode: %d\n", udc->test_mode_nr);
                return;
            }

            if(__raw_readw(IECSR + RXCOUNT) == 0){
                /* the transfer has completed, this is a confirmation */
                __raw_writeb(index, INDEX);
                return;
            }
            
            mt_ep0_idle();
            break;
        case EP0_TX:
            mt_ep0_tx();
            break;
        case EP0_RX:
            mt_ep0_rx();
            break;
        default:
            printk("[USB] Unknown EP0 State\n");
            break;
    }
    __raw_writeb(index, INDEX);

    return;
}

/* called when an interrupt is issued and the state of endpoint 0 is EP0_IDLE.*/
/* this function is called when a packet is received in setup stage.          */
/* must decode the command and prepare for the request sent, and change ep0   */
/* state accordingly */
static void mt_ep0_idle()
{
    int count;//, i;
    int retval = -1;
    u8 type;
    u16 csr0;
    u8 index;
    u8 recip; // the recipient maybe a device, interface or endpoint
    u8 ep_num;
    struct mt_ep *ep = NULL;
    struct mt_req *req = NULL;
    u8 is_in;
    u16 csr;
    u8 tmp, result[2] = {0, 0};
    u8 udc_handled = 0;
    
    union u{
        u8    word[8];
        struct usb_ctrlrequest r;
    }ctrlreq;

    if(!udc){
        printk("[USB] mt_ep0_idle, invalid *udc\n");
        return;
    }

    /* read the request from fifo */
    for(count = 0; count < 8; count++)
    {
        ctrlreq.word[count] = __raw_readb(FIFO(0));
    }

#define	w_value		le16_to_cpup (&ctrlreq.r.wValue)
#define	w_index		le16_to_cpup (&ctrlreq.r.wIndex)
#define	w_length	le16_to_cpup (&ctrlreq.r.wLength)
    
    if(ctrlreq.r.bRequestType & USB_DIR_IN){
        udc->ep0_state = EP0_TX;
    }
    else{
        udc->ep0_state = EP0_RX; // request without data stage will be picked 
        // out later
    }

    type = ctrlreq.r.bRequestType;
    type &= USB_TYPE_MASK;

    recip = ctrlreq.r.bRequestType & USB_RECIP_MASK;

    switch(type){
        case USB_TYPE_STANDARD:
            /* update ep0 state according to the request */
            switch(ctrlreq.r.bRequest){
                case USB_REQ_SET_ADDRESS:
                    udc->faddr = w_value;
                    udc->set_address = 1;
                    retval = 1;
                case USB_REQ_CLEAR_FEATURE:
                    switch(recip){
                        case USB_RECIP_DEVICE:
                            break;
                        case USB_RECIP_INTERFACE:
                            break;
                        case USB_RECIP_ENDPOINT:
                            ep_num = w_index & 0x0f;
                            if((ep_num == 0) || (ep_num > MT_EP_NUM) || (w_value != USB_ENDPOINT_HALT))
                                break;

                            ep = udc->ep + ep_num;
                            mt_ep_set_halt(&ep->ep,0);

                            __raw_writeb(0, INDEX);
                            retval = 1;
                                
                            break;
                    }
                case USB_REQ_SET_CONFIGURATION:
                case USB_REQ_SET_INTERFACE:
                    udc->ep0_state = EP0_IDLE; /* standard requests with no 
                    data stage */
                    break;
                case USB_REQ_SET_FEATURE:
                    udc->ep0_state = EP0_IDLE;
                    switch(recip){
                        case USB_RECIP_DEVICE:
                            if(udc->gadget.speed != USB_SPEED_HIGH)
                                break;
                            if(w_index & 0xff)
                                break;
                            switch(w_index >> 8){
                                case 1:
                                    printk("Try to enter test mode: TEST_J\n");
                                    udc->test_mode_nr = USB_TST_TEST_J;
                                    retval = 1;
                                    break;
                                case 2:
                                    printk("Try to enter test mode: TEST_K\n");
                                    udc->test_mode_nr = USB_TST_TEST_K;
                                    retval = 1;
                                    break;
                                case 3:
                                    printk("Try to enter test mode: SE0_NAK\n");
                                    udc->test_mode_nr = USB_TST_SE0_NAK;
                                    retval = 1;
                                    break;
                                case 4:
                                    printk("Try to enter test mode: TEST_PACKET\n");
                                    udc->test_mode_nr = USB_TST_TEST_PACKET;
                                    retval = 1;
                                    break;
                                default:
                                    break;
                            }
                            if(retval >= 0)
                                udc->test_mode = 1;

                            break;
                        case USB_RECIP_INTERFACE:
                            break;
                        case USB_RECIP_ENDPOINT:
                            // Only handle USB_ENDPOINT_HALT for now

                            ep_num = w_index & 0x0f;
                            
                            if((ep_num == 0) || (ep_num > MT_EP_NUM) || (w_value != USB_ENDPOINT_HALT))
                                break;
                                
                            is_in = w_index & USB_DIR_IN;
                            
                            __raw_writeb(ep_num, INDEX);

                            if(is_in){
                                csr = __raw_readw(IECSR + TXCSR);
                                if(csr & EPX_TX_FIFONOTEMPTY)
                                    csr |= EPX_TX_FLUSHFIFO;
                                csr |= EPX_TX_SENDSTALL;
                                csr |= EPX_TX_CLRDATATOG;
                                /* write 1 ignore, write 0 clear bits */
                                csr |= EPX_TX_WZC_BITS;
                                
                                __raw_writew(csr, IECSR + TXCSR);
                            }
                            else{
                                csr = __raw_readw(IECSR + RXCSR);

                                csr |= EPX_RX_SENDSTALL;
                                csr |= EPX_RX_FLUSHFIFO;
                                csr |= EPX_RX_CLRDATATOG;
                                csr |= EPX_RX_WZC_BITS;

                                __raw_writew(csr, IECSR + RXCSR);
                            }
                            
                            __raw_writeb(0, INDEX);
                            
                            retval = 1;
                            break;
                        default:
                            break;
                    }
                case USB_REQ_GET_STATUS:
                        req = udc->udc_req;
                        switch(recip){
                            case USB_RECIP_DEVICE:
                                result[0] = 1 << USB_DEVICE_SELF_POWERED;
                                // WE DON'T SUPPORT REMOTE WAKEUP
                                ep = udc->ep;
                                retval = 1;
                                break;
                            case USB_RECIP_INTERFACE:
                                result[0] = 0;
                                ep = udc->ep;
                                retval = 1;
                                break;
                            case USB_RECIP_ENDPOINT:
                                ep_num = (u8)w_index & 0x0f;
                                if(!ep_num){
                                    result[0] = 0;
                                    retval = 1;
                                    break;
                                }

                                is_in = (u8)w_index & USB_DIR_IN;
                                ep = udc->ep + ep_num;
 
                                if(ep_num > MT_EP_NUM || !ep->desc){
                                    retval = -EINVAL;
                                    break;
                                } 

                                __raw_writeb(ep_num, INDEX);

                                if(is_in){
                                    tmp = __raw_readw(IECSR + TXCSR) & EPX_TX_SENDSTALL;
                                }
                                else{
                                    tmp = __raw_readw(IECSR + RXCSR) & EPX_RX_SENDSTALL;
                                }

                                result[0] = tmp ? 1 : 0;

                                __raw_writeb(0, INDEX);
                                 
                                retval = 1;
                                break;
                            default:
                                break;
                        }

                        if(retval > 0){
                            req->req.length = w_length;

                            if(req->req.length > 2)
                                req->req.length = 2;

                            memcpy(req->req.buf, result, w_length);
                            udc_handled = 1;                            
                        }
                        
                        break;

                default:
                    break;
            }
            break;
        case USB_TYPE_CLASS:
            switch(ctrlreq.r.bRequest){
                case USB_CDC_REQ_SET_CONTROL_LINE_STATE: /* no data stage */
                case 0xff:
                    udc->ep0_state = EP0_IDLE;
                default:
                    break;
            }
            break;
        case USB_TYPE_VENDOR:
            printk("[USB] Vendor specific command\n");
            break;
        case USB_TYPE_RESERVED:
            printk("[USB] Reserved command\n");
            break;
        default:
            break;
    }

    index = __raw_readb(INDEX);
    __raw_writeb(0, INDEX);

    csr0 = __raw_readw(IECSR + CSR0);
    /* if no data stage */
    if(udc->ep0_state == EP0_IDLE){
        csr0 |= (EP0_SERVICED_RXPKTRDY | EP0_DATAEND);
    }
    else{
        csr0 |= EP0_SERVICED_RXPKTRDY;
    }

    __raw_writew(csr0, IECSR + CSR0);

    if(retval >= 0){
        // do not forward to gadget driver if the request is already handled
        if(udc_handled)
            mt_ep_enqueue(&udc->ep[0].ep, &req->req, GFP_ATOMIC); 

        udc_handled = 0;
    }
    else if(!((type == USB_TYPE_STANDARD) && (ctrlreq.r.bRequest == USB_REQ_SET_ADDRESS))){
        retval = udc->driver->setup(&udc->gadget, &ctrlreq.r);
    }

    #ifndef CONFIG_MT6516_EVB_BOARD
    if(ctrlreq.r.bRequest == USB_REQ_SET_CONFIGURATION){
        if(ctrlreq.r.wValue & 0xff){
            BAT_SetUSBState(USB_CONFIGURED);
        }
        else{
            BAT_SetUSBState(USB_UNCONFIGURED);
        }
        wake_up_bat();
    }
    #endif
    
    if(retval < 0){
        //not supported by gadget driver;
        printk("[USB] Request from Host is not supported!!\n");
        printk("[USB] bRequestType = %x\n", ctrlreq.r.bRequestType);
        printk("[USB] bRequest = %x\n", ctrlreq.r.bRequest);
        printk("[USB] w_value = %x\n", w_value);
        printk("[USB] w_index = %x\n", w_index);
        printk("[USB] w_length = %x\n", w_length);
        #if 0
        for(i = 0; i < MT_EP_NUM; i++)
        {
            mt_ep_fifo_flush_internal(&udc->ep[i].ep);
        }
        #endif
        mt_ep_fifo_flush_internal(&udc->ep[0].ep);

        __raw_writew(EP0_SENDSTALL, IECSR + CSR0);
        printk("[USB] EP0 SENDSTALL\n");
    }

    __raw_writeb(index, INDEX);
   
    return;
}

/* called when an interrupt is issued and the state of endpoint 0 is EP0_RX */
static void mt_ep0_rx()
{
    struct mt_ep *ep;
    struct mt_req *req;
    int count = 0;
    u8 index;
    u16 csr0;

    //printk("EP0_RX\n");
    if(!udc){
        printk("[USB] mt_ep0_rx, invalid *udc\n");
        return;
    }
    
    ep = &udc->ep[0];

    if(!ep){
        printk("[USB] mt_ep0_rx, invalid *ep\n");
        return;
    }

    req = container_of(ep->queue.next, struct mt_req, queue);

    if(req){
        index = __raw_readb(INDEX);
        __raw_writeb(0, INDEX);

        csr0 = __raw_readw(IECSR + CSR0);
        
        count = mt_udc_read_fifo(ep);
        if(count < ep->ep.maxpacket){     
            done(ep, req, 0);
            udc->ep0_state = EP0_IDLE;
            csr0 |= EP0_DATAEND;
        }
        csr0 |= EP0_SERVICED_RXPKTRDY;
        __raw_writew(csr0, IECSR + CSR0);
        
        __raw_writeb(index, INDEX);
    }
    else{
        printk("[USB] ERROR: NO REQUEST\n");
    }

    return;
}

/* called when an interrupt is issued and the state of endpoint 0 is EP0_TX */
static void mt_ep0_tx()
{
    struct mt_ep *ep;
    struct mt_req *req;
    int count = 0;
    u8 index;
    u16 csr0;
    
    //printk("EP0_TX\n");
    if(!udc){
        printk("[USB] mt_ep0_tx, invalid *udc\n");
        return;
    }

    ep = &udc->ep[0];

    if(!ep){
        printk("[USB] mt_ep0_tx, invalid *ep\n");
        return;
    }

    if(list_empty(&ep->queue)){
        printk("[USB] SW buffer is not ready!!\n");
        return;
    }
    
    
    req = container_of(ep->queue.next, struct mt_req, queue);
    
    if(req){
        index = __raw_readb(INDEX);
        __raw_writeb(0, INDEX);

        csr0 = __raw_readw(IECSR + CSR0);  
        count = mt_udc_write_fifo(ep);
        if(count < ep->ep.maxpacket){
            done(ep, req, 0);
            csr0 |= EP0_DATAEND;
            udc->ep0_state = EP0_IDLE;
        }
        csr0 |= EP0_TXPKTRDY;
       
        __raw_writew(csr0, IECSR + CSR0);
        
        __raw_writeb(index, INDEX);
    }
    else{
        printk("[USB] ERROR: NO REQUEST\n");
    }
    
    return;
}

/* if any bit of INTRTX except for bit 0 is set, then this function will be */
/* called with the second parameter being USB_TX. if any bit of INTRRX is set */
/* then this function will be called with the second parameter being USB_RX */
static void mt_epx_handler(int irq_src, USB_DIR dir)
{
    struct mt_req *req;
    struct mt_ep *ep;
    int count;
    u8 index;
    u16 csr;
    
    req = NULL;
  
    if(!udc){
        printk("[USB] mt_epx_handler, invalid *udc\n");
        return;
    }

    ep = &udc->ep[irq_src];
    req = container_of(ep->queue.next, struct mt_req, queue);

    if(!ep || !req){
        printk("[USB] mt_epx_handler, invalid *ep or *req\n");
        return;
    }

    if(!list_empty(&ep->queue)){
        index = __raw_readb(INDEX);
        __raw_writeb(irq_src, INDEX);

        if(dir == USB_TX){
            csr = __raw_readw(IECSR + TXCSR);
            if(csr & EPX_TX_TXPKTRDY){
                printk("[USB] TX Packet not transferred\n");
            }
            
            #if defined USB_USE_DMA_MODE0
                count = min((unsigned int)ep->ep.maxpacket, req->req.length - req->req.actual);
                if((0 < count)  && (count <= ep->ep.maxpacket)){
                    mt_usb_config_dma(irq_src, 3, irq_src, 0, 1, (u32)(req->req.buf + req->req.actual), count);
                }
                else if(count == 0){
                    /* this is the confirmation of the tx packet, nothing needs to 
                    be done */
                    done(ep, req, 0);
                    if(!list_empty(&ep->queue)){
                        mt_epx_handler(irq_src, USB_TX);
                    }
                }
            #elif defined USB_USE_DMA_MODE1
            
                done(ep, req, 0);
                
                if(!list_empty(&ep->queue)){
                    req = container_of(ep->queue.next, struct mt_req, queue);
                    csr |= EPX_TX_AUTOSET | EPX_TX_DMAREQEN | EPX_TX_DMAREQMODE;
                    __raw_writew(csr, IECSR + TXCSR);
                    mt_usb_config_dma(irq_src, 3, irq_src, 1, 1, (u32)req->req.buf, req->req.length);
                }
            #elif defined USB_USE_PIO_MODE
                count = mt_udc_write_fifo(ep);
                if((0 < count)  && (count <= ep->ep.maxpacket)){
                    csr |= EPX_TX_TXPKTRDY;
                    
                    __raw_writew(csr, IECSR + TXCSR);
                }
                else if(count == 0){
                    /* this is the confirmation of the tx packet, nothing needs to 
                    be done */
                    done(ep, req, 0);
                    
                    __raw_writeb(index, INDEX);
                    
                    if(!list_empty(&ep->queue)){
                        mt_epx_handler(irq_src, USB_TX);
                    }

                }
            #endif
        }
        else if(dir == USB_RX){

            csr = __raw_readw(IECSR + RXCSR);                 
            if(csr & EPX_RX_RXPKTRDY){

                #if defined USB_USE_DMA_MODE0
                if(!list_empty(&ep->queue)){
                    mt_usb_config_dma(irq_src, 3, irq_src, 0, 0, (u32)req->req.buf + req->req.actual, min((u32)__raw_readw(IECSR + RXCOUNT), req->req.length - req->req.actual));
                }
                else{
                    printk("[USB] SW buffer is not ready for rx transfer!!\n");
                }
                #elif defined USB_USE_DMA_MODE1
                if(!list_empty(&ep->queue)){
                    mt_usb_config_dma(irq_src, 3, irq_src, 0, 0, (u32)req->req.buf + req->req.actual, min((u32)__raw_readw(IECSR + RXCOUNT), req->req.length - req->req.actual));
                }
                else{
                    printk("[USB] SW buffer is not ready for rx transfer!!\n");
                }
                #elif defined USB_USE_PIO_MODE
                count = mt_udc_read_fifo(ep);
                
                if((count < ep->ep.maxpacket) || (req->req.length == req->req.actual)){
                    done(ep, req, 0);
                    if(!list_empty(&ep->queue)){
                        csr &= ~EPX_RX_RXPKTRDY;
                        __raw_writew(csr, IECSR + RXCSR);
                    }
                }
                else{
                    csr &= ~EPX_RX_RXPKTRDY;
                    __raw_writew(csr, IECSR + RXCSR);
                }
                #endif
            }
        }

        __raw_writeb(index, INDEX);
    }
    
    return;
}

/* reset the device */
static void mt_usb_reset()
{    
    int i;
    u8 tmp8;
    u16 swrst;

    if(!udc){
        printk("[USB] mt_usb_reset, invalid *udc\n");
        return;
    }

    #ifndef CONFIG_MT6516_EVB_BOARD
    if(!udc->test_mode){
        BAT_SetUSBState(USB_UNCONFIGURED);
        wake_up_bat();
    }
    #endif

    /* enable all system interrupts, but disable all endpoint interrupts */
    __raw_writeb(0, INTRTXE);
    __raw_writeb(0, INTRRXE);
    __raw_writeb(0, INTRUSBE);

    for(i = 0; i <= MT_EP_NUM; i++){
        mt_ep_fifo_flush_internal(&udc->ep[i].ep);
    }

     /* clear all dma channels */
    for(i = 1; i <= MT_CHAN_NUM; i++){
        __raw_writew(0, USB_DMA_CNTL(i));
        __raw_writel(0, USB_DMA_ADDR(i));
        __raw_writel(0, USB_DMA_COUNT(i));
    }

    __raw_readb(INTRTX);
    __raw_writeb(0, INTRRX);
    __raw_readb(INTRUSB);
    __raw_writeb(0, DMA_INTR);
    /* ip software reset */
    swrst = __raw_readw(SWRST);
    swrst |= (SWRST_DISUSBRESET | SWRST_SWRST);
    __raw_writew(swrst, SWRST);
    __raw_writeb((INTRUSB_SUSPEND | INTRUSB_RESUME | INTRUSB_RESET | 
    INTRUSB_DISCON), INTRUSBE);

    /* flush endpoint 0 fifo */
    __raw_writeb(0, INDEX);
    __raw_writew(EP0_FLUSH_FIFO, IECSR + CSR0);

    udc->ep0_state = EP0_IDLE;
    udc->faddr = 0;
    udc->fifo_addr = FIFO_START_ADDR;
    //udc->test_mode = 0;

    for(i = 0; i < MT_EP_NUM; i++)
    {
        struct mt_ep *ep;

        ep = &udc->ep[i];

        if(!ep){
            printk("[USB] mt_usb_reset, invalid *ep\n");
            return;
        }

        ep->desc = NULL;
        ep->busycompleting = 0;
    }
    
    tmp8 = __raw_readb(POWER);
    if(tmp8 & PWR_HS_MODE){
        udc->gadget.speed = USB_SPEED_HIGH;
    }
    else{
        udc->gadget.speed = USB_SPEED_FULL;
    }

    __raw_writeb(0x1, INTRTXE);

    return;
}

/* ========================================================= */
/* the handler which service interrupts generated by USB_DMA */ 
/* ========================================================= */

static irqreturn_t mt_udc_dma_handler(int chan)
{
    struct mt_req *req = NULL;
    struct mt_ep *ep;
    int count;
    u8 index;
    u16 csr;
    u8 irq_src;
    u32 usb_dma_cntl;
    //dma_addr_t dma_addr = 0;
    if(!udc){
        printk("[USB] mt_udc_dma_handler, invalid *udc\n");
        return IRQ_NONE;
    }

    usb_dma_cntl = __raw_readl(USB_DMA_CNTL(chan));
    /* for dma mode 0 temporarily */
    
    irq_src = (u8)((usb_dma_cntl & USB_DMA_CNTL_ENDPOINT_MASK) >> USB_DMA_CNTL_ENDPOINT_OFFSET);
    ep = &udc->ep[irq_src];
    req = container_of(ep->queue.next, struct mt_req, queue);
    count = __raw_readl(USB_DMA_REALCOUNT(chan));

    if(!ep || !req){
        printk("[USB] mt_udc_dma_handler, invalid *ep or *req\n");
        return IRQ_NONE;

    }

    index = __raw_readb(INDEX);
    __raw_writeb(irq_src, INDEX);
    
    if(usb_dma_cntl & USB_DMA_CNTL_DMADIR){ /* tx dma */
        #if defined USB_USE_DMA_MODE0
        
            /*dma_unmap_single(NULL, virt_to_dma(NULL, req->req.buf + \
            req->req.actual), count, DMA_TO_DEVICE);*/
            req->req.actual += count;

            csr = __raw_readw(IECSR + TXCSR);
            csr |= EPX_TX_TXPKTRDY;
            __raw_writew(csr, IECSR + TXCSR);
            
        #elif defined USB_USE_DMA_MODE1
           
            dma_addr = virt_to_dma(NULL, req->req.buf);
            count = __raw_readl(USB_DMA_REALCOUNT(chan));
            dma_unmap_single(NULL, dma_addr, req->req.length, DMA_TO_DEVICE);
            req->req.actual = count;

            if(count % (ep->ep.maxpacket) != 0){
                csr = __raw_readw(IECSR + TXCSR);
                csr &= ~EPX_TX_DMAREQEN;
                __raw_writew(csr, IECSR + TXCSR);
                csr |= EPX_TX_TXPKTRDY;
                __raw_writew(csr, IECSR + TXCSR);
            }
            else{    
                
                done(ep, req, 0);
                 
                if(!list_empty(&ep->queue)){
                    req = container_of(ep->queue.next, struct mt_req, queue);
                    csr = __raw_readw(IECSR + TXCSR);
                    csr |= EPX_TX_AUTOSET | EPX_TX_DMAREQEN | EPX_TX_DMAREQMODE;
                    __raw_writew(csr, IECSR + TXCSR);
                    mt_usb_config_dma(irq_src, 3, irq_src, 1, 1, (u32)req->req.buf, req->req.length);
                }
            }

        #endif

        dma_active[chan - 1] = 0;
    }
    else{ /* rx dma */
        /*dma_unmap_single(NULL, virt_to_dma(NULL, req->req.buf + \
        req->req.actual), count, DMA_FROM_DEVICE);*/

        /* if the transfer is done, then complete it  */
        req->req.actual += count;
         
        if((count < ep->ep.maxpacket) || (req->req.length == req->req.actual)){
            done(ep, req, 0);
        }

        /* enable the current fifo for the next packet */
        csr = __raw_readw(IECSR + RXCSR);
        csr &= ~EPX_RX_RXPKTRDY;
        __raw_writew(csr, IECSR + RXCSR);

        dma_active[chan - 1] = 0;

        /* if fifo is loaded and sw buffer is ready, then trigger the dma transfer(for double packet buffering) */
        if(!list_empty(&ep->queue)){
            csr = __raw_readw(IECSR + RXCSR);
            if(csr & EPX_RX_RXPKTRDY){
                req = container_of(ep->queue.next, struct mt_req, queue);
                count = __raw_readw(IECSR + RXCOUNT);

                if(count > 0){
                    mt_usb_config_dma(irq_src, 3, irq_src, 0, 0, (u32)req->req.buf + req->req.actual, \
                    min((unsigned)count, req->req.length - req->req.actual));
                }
            }
        }
    }

    __raw_writeb(index, INDEX);

    return IRQ_HANDLED;
}

static void mt_usb_config_dma(int channel, int burst_mode, int ep_num, int dma_mode, int 
dir, u32 addr, u32 count){

    u16 usb_dma_cntl = 0;
    dma_addr_t dma_addr;
    
    if(dma_active[channel - 1]){
        return;
    }

    dma_active[channel - 1] = 1;

    dma_addr = virt_to_phys((void*)addr);
    dmac_flush_range((void *)addr, (void *)(addr + count));
      
    __raw_writel(dma_addr, USB_DMA_ADDR(channel));
    __raw_writel(count, USB_DMA_COUNT(channel));
    
    usb_dma_cntl = (burst_mode << USB_DMA_CNTL_BURST_MODE_OFFSET) | (ep_num << USB_DMA_CNTL_ENDPOINT_OFFSET) | USB_DMA_CNTL_INTEN 
    | (dma_mode << USB_DMA_CNTL_DMAMODE_OFFSET) | (dir << USB_DMA_CNTL_DMADIR_OFFSET) | USB_DMA_CNTL_DMAEN;

    __raw_writew(usb_dma_cntl, USB_DMA_CNTL(channel));

    return;
}

/* ========================================================= */
/* USB Charger Type Detection                                */
/* ========================================================= */

CHARGER_TYPE mt6516_usb_charger_type_detection(){

    if(!udc)
        return CHARGER_UNKNOWN;

    if(!udc->ready)
        return CHARGER_UNKNOWN;

    if(udc->power){
        return udc->charger_type;
    }

    /* to avoid standard host being misrecognized as non-standard charger if */
    /* cable is halfly plugged */
    if(g_usb_power_saving){
        msleep(CHR_TYPE_DETECT_DEB);
    }
    else{
        mdelay(CHR_TYPE_DETECT_DEB);
    }
    
    USB_Charger_Detect_Init();
 
    mdelay(1);
    
    if(!(__raw_readl(USB_PHY_INTF1) & USB_PHY_INTF1_DM)){
        USB_Charger_Detect_Release();
        udc->charger_type = STANDARD_HOST;
	    g_USBStatus = STANDARD_HOST;
	    printk("[USB Charger] Charger Type Detection: ");
        printk("STANDARD HOST\n");
        return STANDARD_HOST;
    }

    USB_Charger_Detect_Release();

    mdelay(1);

    USB_Check_Standard_Charger();

    mdelay(1);
    
    if(__raw_readl(USB_PHY_INTF1) & USB_PHY_INTF1_DM){
        USB_Charger_Detect_Release();
        udc->charger_type = STANDARD_CHARGER;
	    g_USBStatus = STANDARD_CHARGER;
	    printk("[USB Charger] Charger Type Detection: ");
        printk("STANDARD CHARGER\n");
        return STANDARD_CHARGER;
    }
    else{
        USB_Charger_Detect_Release();
        udc->charger_type = NONSTANDARD_CHARGER;
	    g_USBStatus = NONSTANDARD_CHARGER;
	    printk("[USB Charger] Charger Type Detection: ");
        printk("NONSTANDARD CHARGER\n");
        return NONSTANDARD_CHARGER;
    }

    USB_Charger_Detect_Release();
    udc->charger_type = CHARGER_UNKNOWN;
    g_USBStatus = CHARGER_UNKNOWN;
    printk("[USB] Critical Error!! Should not be here!!\n");
    
    return CHARGER_UNKNOWN;
}

/* USB PHY Related functions */
static void USB_UART_Share(u8 usb_mode){
    u32 tmp;
    if(usb_mode){
        tmp = __raw_readl(USB_PHY_INTF2);
        tmp &= ~USB_BYTE3_MASK;
        tmp |= USB_PHY_INTF2_FORCE_USB_CLKON;
        __raw_writel(tmp, USB_PHY_INTF2);

        tmp = __raw_readl(USB_PHY_CON5);
        tmp &= ~USB_BYTE2_MASK;
        __raw_writel(tmp, USB_PHY_CON5);
    }
    else{
        tmp = __raw_readl(USB_PHY_INTF2);
        tmp &= ~USB_BYTE3_MASK;
        tmp |= USB_PHY_INTF2_FORCE_AUX_EN;
        __raw_writel(tmp, USB_PHY_INTF2);

        tmp = __raw_readl(USB_PHY_CON5);
        tmp &= ~USB_BYTE2_MASK;
        tmp |= USB_PHY_CON5_FORCE_SUSPENDM;
        __raw_writel(tmp, USB_PHY_CON5);
    }
}

static void USB_Charger_Detect_Init(void){
    u32 tmp;

    /* disable usb power down */
    hwEnableClock(MT6516_PDN_PERI_USB,"USB");
    
    /* Power On USB */
    hwPowerOn(MT6516_POWER_VUSB, VOL_3300,"USB");
    
    /* switch to USB mode */
    USB_UART_Share(1);

    /* Only for 38 E2 series add 100K resistor to D- */
    tmp = __raw_readl(USB_PHY_CON1);
    tmp &= ~USB_BYTE0_MASK;
    __raw_writel(tmp, USB_PHY_CON1);
    
    tmp = __raw_readl(USB_PHY_CON3);
    tmp |= USB_PHY_CON3_TEST_CTRL2 | USB_PHY_CON3_TEST_CTRL1;
    __raw_writel(tmp, USB_PHY_CON3);

    tmp = __raw_readl(USB_PHY_CON4);
    tmp &= ~USB_BYTE1_MASK;
    tmp |= USB_PHY_CON4_FORCE_BGR_ON;
    __raw_writel(tmp, USB_PHY_CON4);

    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_BYTE3_MASK;
    tmp |= USB_PHY_CON5_TERM_SELECT | USB_PHY_CON5_OP_MODE_0;
    __raw_writel(tmp, USB_PHY_CON5);

    tmp = __raw_readl(USB_PHY_INTF1);
    tmp &= ~USB_BYTE0_MASK;
    tmp |= USB_PHY_INTF1_TX_VALID;
    __raw_writel(tmp, USB_PHY_INTF1);

    /* force these initial value (non including UART part) */
    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_BYTE2_MASK;
    tmp |= USB_PHY_CON5_FORCE_OP_MODE | USB_PHY_CON5_FORCE_TERM_SELECT | USB_PHY_CON5_FORCE_SUSPENDM; 
    __raw_writel(tmp, USB_PHY_CON5);

    /* Remove 15K ohm pull down resistor */
    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~(USB_PHY_CON5_DP_PULL_DOWN | USB_PHY_CON5_DM_PULL_DOWN);
    __raw_writel(tmp, USB_PHY_CON5);

    tmp = __raw_readl(USB_PHY_CON2);
    tmp |= (USB_PHY_CON2_FORCE_DM_PULLDOWN | USB_PHY_CON2_FORCE_DP_PULLDOWN);
    __raw_writel(tmp, USB_PHY_CON2);
}

static void USB_Charger_Detect_Release(void){
    u32 tmp;

    /* Remove 100K resistor from D- */
    tmp = __raw_readl(USB_PHY_CON1);
    tmp &= ~USB_BYTE0_MASK;
    tmp |= USB_PHY_CON1_PLL_EN;
    __raw_writel(tmp, USB_PHY_CON1);

    tmp = __raw_readl(USB_PHY_CON3);
    tmp &= ~USB_PHY_CON3_TEST_CTRL_MASK;
    __raw_writel(tmp, USB_PHY_CON3);

    tmp = __raw_readl(USB_PHY_CON4);
    tmp &= ~USB_BYTE1_MASK;
    tmp |= USB_PHY_CON4_FORCE_BGR_ON;
    __raw_writel(tmp, USB_PHY_CON4);

    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_BYTE3_MASK;
    __raw_writel(tmp, USB_PHY_CON5);

    tmp = __raw_readl(USB_PHY_INTF1);
    tmp &= ~USB_BYTE0_MASK;
    __raw_writel(tmp, USB_PHY_INTF1);

    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_BYTE2_MASK;
    __raw_writel(tmp, USB_PHY_CON5);

    /* Remove 15K ohm pull down resistor */
    tmp = __raw_readl(USB_PHY_CON2);
    tmp &= ~(USB_PHY_CON2_FORCE_DP_PULLDOWN | USB_PHY_CON2_FORCE_DM_PULLDOWN);
    __raw_writel(tmp, USB_PHY_CON2);

    /* Remove D+ 1.5K ohm pull up resistor */
    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_PHY_CON5_FORCE_DP_HIGH;
    __raw_writel(tmp, USB_PHY_CON5);
    
    /* Disable usb power down */
    hwDisableClock(MT6516_PDN_PERI_USB,"USB");
    
    /* Power Down USB */
    hwPowerDown(MT6516_POWER_VUSB,"USB");

}

static void USB_Check_Standard_Charger(void){
    u32 tmp;
    
    /* disable usb power down */
    hwEnableClock(MT6516_PDN_PERI_USB,"USB");
    
    /* Power On USB */
    hwPowerOn(MT6516_POWER_VUSB, VOL_3300,"USB");
    
    /* Only for 38 E2, Remove 100K resistor from D- */
    tmp = __raw_readl(USB_PHY_CON1);
    tmp &= ~USB_BYTE0_MASK;
    tmp |= USB_PHY_CON1_PLL_EN;
    __raw_writel(tmp, USB_PHY_CON1);

    tmp = __raw_readl(USB_PHY_CON3);
    tmp &= ~USB_PHY_CON3_TEST_CTRL_MASK;
    __raw_writel(tmp, USB_PHY_CON3);

    tmp = __raw_readl(USB_PHY_CON3);
    tmp |= USB_PHY_CON3_TEST_CTRL1;
    __raw_writel(tmp, USB_PHY_CON3);

    tmp = __raw_readl(USB_PHY_CON4);
    tmp &= ~USB_BYTE1_MASK;
    tmp |= USB_PHY_CON4_FORCE_BGR_ON;
    __raw_writel(tmp, USB_PHY_CON4);

    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_BYTE3_MASK;
    __raw_writel(tmp, USB_PHY_CON5);

    tmp = __raw_readl(USB_PHY_INTF1);
    tmp &= ~USB_BYTE0_MASK;
    __raw_writel(tmp, USB_PHY_INTF1);

    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_BYTE2_MASK;
    __raw_writel(tmp, USB_PHY_CON5);

    /* Add D- 15K ohm pull down resistor */
    tmp = __raw_readl(USB_PHY_CON5);
    tmp |= USB_PHY_CON5_DM_PULL_DOWN;
    __raw_writel(tmp, USB_PHY_CON5);

    /* Add D+ 1.5K ohm pull up resistor */
    tmp = __raw_readl(USB_PHY_CON5);
    tmp &= ~USB_PHY_CON5_XCVR_SELECT_MASK;
    __raw_writel(tmp, USB_PHY_CON5);

    tmp = __raw_readl(USB_PHY_CON5);
    tmp |= (USB_PHY_CON5_XCVR_SELECT_0 | USB_PHY_CON5_TERM_SELECT);
    __raw_writel(tmp, USB_PHY_CON5);

    tmp = __raw_readl(USB_PHY_CON2);
    tmp |= USB_PHY_CON2_FORCE_DM_PULLDOWN | USB_PHY_CON2_FORCE_DP_PULLDOWN;
    __raw_writel(tmp, USB_PHY_CON2);

    tmp = __raw_readl(USB_PHY_CON5);
    tmp |= USB_PHY_CON5_FORCE_DP_HIGH;
    __raw_writel(tmp, USB_PHY_CON5);
}

/* ================================ */
/* connect and disconnect functions */
/* ================================ */

void mt6516_usb_connect(void){

    if(!udc || !udc->ready)
        return;

    mt_dev_connect();
}

void mt6516_usb_disconnect(void){

    if(!udc || !udc->ready)
        return;
    
    mt_dev_disconnect();
}

int mt6516_usb_is_charger_in(void){
    int charger_in = 0;
    
    mt_usb_phy_init(udc);
    mdelay(5);
    if(__raw_readl(USB_PHY_INTF2) & USB_PHY_INTF2_VBUSVALID)
        charger_in = 1;
    mt_usb_phy_deinit(udc);

    return charger_in;
}

/* =============================================== */
/* module initialization and destruction functions */
/* =============================================== */

static int __init mt_udc_init(void)
{
    //return driver_register(&mt_udc_driver);
    __raw_readb(INTRTX);
    __raw_writeb(0, INTRRX);
    __raw_readb(INTRUSB);
    __raw_writeb(0, INTRTXE);
    __raw_writeb(0, INTRRXE);
    __raw_writeb(0, INTRUSBE);
    __raw_writeb(0, DMA_INTR);
    MT6516_IRQSensitivity(MT6516_USB_IRQ_LINE, MT6516_LEVEL_SENSITIVE);
    MT6516_IRQSensitivity(MT6516_USBDMA_IRQ_LINE, MT6516_LEVEL_SENSITIVE);
    return platform_driver_register(&mt_udc_driver);
}

static void __exit mt_udc_exit(void)
{
    //driver_unregister(&mt_udc_driver);
    platform_driver_unregister(&mt_udc_driver);

    return;
}

module_init(mt_udc_init); 
module_exit(mt_udc_exit);

EXPORT_SYMBOL(usb_gadget_register_driver);
EXPORT_SYMBOL(usb_gadget_unregister_driver);

MODULE_AUTHOR("MediaTek");
