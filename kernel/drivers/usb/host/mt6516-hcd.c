
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/types.h>
#include <asm/arch/irqs.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/usb.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include "../core/hcd.h"
#include "../core/hub.h"
#include "mt6516-hcd.h"

#define DRIVER_VERSION "2009 April 2"
#define DRIVER_AUTHOR "PangYen Chen"
#define DRIVER_DESC "USB 1.1 Embedded Host Controller Driver"

static const char hcd_name[] = "mt-hcd";

struct mt_hc *mt_host_controller;

static u8 mt_usb_tx_packet(struct mt_ep *pEnd);
static u8 mt_usb_rx_packet(struct mt_ep *pEnd, u16 wCount);
static void mt_usb_loadFifo(struct mt_ep *pEnd, u16 wCount, const u8 *pSource);
static USB_BOOL mt_complete_urb(struct mt_ep *pEnd, struct urb *pUrb);

/* log2(x) */
static u32 mt_log2(u32 x){

   u32 i;

   for (i = 0; x > 1; i++){
    	x = x / 2;
   }

   return i;
}

/* find a host endpoint that can service the usb device associated with the urb */
static struct mt_ep *mt_find_ep(struct urb *pUrb){
    
    struct mt_ep *pEnd = NULL;
    struct usb_device *dev = pUrb->dev;
    struct usb_host_endpoint *pHEnd;
    struct usb_device_descriptor *pDevDescriptor = &dev->descriptor;
    struct usb_interface_descriptor *pIntDescriptor;
    int iEnd = -1; /* the endpoint number of the endpoint found */
    unsigned int uiOut = usb_pipeout(pUrb->pipe);
    u16 wMaxPacketSize = usb_maxpacket(pUrb->dev, pUrb->pipe, uiOut);

    /* if the urb is for control transfer, then use default control endpoint */
    if(usb_pipecontrol(pUrb->pipe)){
        pEnd = &mt_host_controller->ep[0];
        return pEnd;
    }

    /* pHEnd: assciated with the endpoint in the usb device */
    pHEnd = (uiOut ? dev->ep_out : dev->ep_in)[usb_pipeendpoint(pUrb->pipe)];
    
    if(!pHEnd){
        return NULL;
    }

    if(pHEnd->hcpriv){
        pEnd = (struct mt_ep *)pHEnd->hcpriv;
        return pEnd;
    }

    /* use the reserved endpoint (ep1) for bulk transfer if any */
    if(usb_pipebulk(pUrb->pipe)){
        pIntDescriptor = &dev->actconfig->interface[0]->cur_altsetting->desc;
        if((pDevDescriptor->bDeviceClass == USB_CLASS_MASS_STORAGE) ||
           (pIntDescriptor->bInterfaceClass == USB_CLASS_MASS_STORAGE)){
            if(usb_pipeout(pUrb->pipe)){
                iEnd = 1;
            }
            else{
                iEnd = 2;
            }
            pEnd = &mt_host_controller->ep[iEnd];
            pHEnd->hcpriv = (void *)pEnd;
            return pEnd;
        }
    }

    /* find an endpoint that has not been used and can support the usb device */
    for(iEnd = 2; iEnd < MT_EP_NUM; iEnd++){
        pEnd = &mt_host_controller->ep[iEnd];
        if(pEnd->dev == NULL && pEnd->wPacketSize >= wMaxPacketSize){
            pEnd->dev = dev;
            pHEnd->hcpriv = (void *)pEnd;
            return pEnd;
        }
    }
    
    return NULL;
}

static USB_BOOL mt_ep_is_idle(struct mt_ep *pep){

    /* check whether the endpoint is serving an urb or an urb is waiting for its 
       service */

    if(pep->pCurrentUrb){
        return USB_FALSE;
    }
    else{
        return list_empty(&pep->urb_list);
    }
}

static void mt_hub_descriptor(struct usb_hub_descriptor *desc){

    u16 temp = 0;

    desc->bDescriptorType = 0x29;
    desc->bHubContrCurrent = 0;

    desc->bNbrPorts = 1;
    desc->bDescLength = 9;

    /* per-port power switching (gang of one!), or none */
    desc->bPwrOn2PwrGood = 10;

    /* no overcurrent errors detection/handling */
    temp = 0x0011;

    desc->wHubCharacteristics = (__force __u16) cpu_to_le16(temp);

    /* two bitmaps:  ports removable, and legacy PortPwrCtrlMask */
    desc->bitmap[0] = 1 << 1;
    desc->bitmap[1] = ~0;

    return;
}

static void mt_usb_setTimer(void (*func)(unsigned long), u32 millisecs)
{
    u32 delay = millisecs / 10;

    del_timer_sync(&(mt_host_controller->timer));

    init_timer(&(mt_host_controller->timer));
    mt_host_controller->timer.function = func;
    mt_host_controller->timer.expires = jiffies + (HZ * delay) / 100;
    add_timer(&(mt_host_controller->timer));
        
    return; 
}

static void mt_usb_delTimer(void){

    del_timer_sync(&(mt_host_controller->timer));

    return;
}

static void mt_usb_check_connect(unsigned long not_used){

    u8 tmp;
    u32 dwVirtualHubPortStatus = 0;

    //DBG("mt3351_usb_check_connect\n");

    /* check connect status */
    tmp = __raw_readb(DEVCTL);

    if(tmp & DEVCTL_LS_DEV){
        dwVirtualHubPortStatus = (1 << USB_PORT_FEAT_C_CONNECTION) | (1 << USB_PORT_FEAT_LOWSPEED) | (1 << USB_PORT_FEAT_POWER) | (1 << USB_PORT_FEAT_CONNECTION);
        mt_host_controller->bRootSpeed = USB_ROOT_SPEED_LOW;
    }
    else if(tmp & DEVCTL_FS_DEV){
        dwVirtualHubPortStatus = (1 << USB_PORT_FEAT_C_CONNECTION) | (1 << USB_PORT_FEAT_POWER) | (1 << USB_PORT_FEAT_CONNECTION);
        mt_host_controller->bRootSpeed = USB_ROOT_SPEED_FULL;
    }

    mt_host_controller->virtualHubPortStatus = dwVirtualHubPortStatus;
    /*
    DBG("CONNECT, VIRTUAL HUB PORT STATUS = \n");
	DBG("   PORT_CONNECTION : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_CONNECTION)) ? 1 : 0); 
	DBG("   PORT_ENABLE : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_ENABLE)) ? 1 : 0);
	DBG("   PORT_SUSPEND : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_SUSPEND)) ? 1 : 0);
    DBG("   PORT_OVERCURRENT : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_OVER_CURRENT)) ? 1 : 0);
    DBG("   PORT_RESET : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_RESET)) ? 1 : 0);
    DBG("   PORT_POWER : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_POWER)) ? 1 : 0);
    DBG("   PORT_LOW_SPEED : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_LOWSPEED)) ? 1 : 0);
    DBG("   PORT_HIGH_SPEED : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_HIGHSPEED)) ? 1 : 0);
    DBG("   PORT_TEST : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_TEST)) ? 1 : 0);
    DBG("   PORT_INDICATOR : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_INDICATOR)) ? 1 : 0);
    */
    return;
}

static struct urb *mt_get_next_urb(struct mt_ep *pEnd){

    struct mt_urb_list *urb_list;
    struct urb *pUrb;

    if(pEnd->pCurrentUrb){
        pUrb = pEnd->pCurrentUrb;
    }
    else{
        urb_list = list_entry(pEnd->urb_list.next, struct mt_urb_list, urb_list);
        pUrb = urb_list->pUrb;
        if(pUrb){
            pEnd->pCurrentUrb = pUrb;
            list_del(&urb_list->urb_list);
            kfree(urb_list);
        }
    }

    return pUrb;
}

static void mt_usb_starttx(u8 bEnd){

    u16 wCsr;

    __raw_writeb(bEnd, INDEX);

    if(bEnd){ /* not endpoint 0 */
        wCsr = __raw_readw(IECSR + TXCSR);
        wCsr |= EPX_TX_TXPKTRDY;
        __raw_writew(wCsr, IECSR + TXCSR);
    }
    else{ /* endpoint 0 */
        wCsr = __raw_readw(IECSR + CSR0);
        wCsr |= EP0_TXPKTRDY;
        if(mt_host_controller->ep0_state == EP0_IDLE)
            wCsr |= EP0_SETUPPKT;
        __raw_writew(wCsr, IECSR + CSR0);
    }

    return;
}

static void mt_usb_set_protocol(struct mt_ep *pEnd, struct urb *pUrb, u8 xmt){

    u8 protocol, ep_num, type;

    //DBG("mt3351_usb_set_protocol\n");

    protocol = 0;
    ep_num = pEnd->ep_num;

    __raw_writeb(ep_num, INDEX);

    if(usb_pipeisoc(pUrb->pipe)){
        //DBG("mt3351_usb_set_protocol: ISOCHRONOUS TRANSFER\n");
        protocol = PROTOCOL_ISOCHRONOUS;
    }
    else if(usb_pipebulk(pUrb->pipe)){
        //DBG("mt3351_usb_set_protocol: BULK TRANSFER\n");
        protocol = PROTOCOL_BULK;
    }
    else if(usb_pipeint(pUrb->pipe)){
        //DBG("mt3351_usb_set_protocol: INTERRUPT TRANSFER\n");
        protocol = PROTOCOL_INTERRUPT;
    }
    else{
        protocol = PROTOCOL_ILLEGAL;
    }

    type = protocol | ep_num;

    if(xmt){ /* tx endpoint */
        if(ep_num)
            __raw_writeb(type, IECSR + TXTYPE);
    }
    else{ /* rx endpoint */
        if(ep_num)
            __raw_writeb(type, IECSR + RXTYPE);
    }

    return;
}

static void mt_usb_set_address(struct mt_ep *pEnd, struct urb *pUrb){

    u8 bAddress = usb_pipedevice(pUrb->pipe);

    //DBG("mt3351_usb_set_address, FADDR = %d\n", bAddress);
    
    __raw_writeb(bAddress, FADDR);
    
    return;
}

static int mt_usb_ep_urb_enqueue(struct mt_ep *pEnd, struct urb *pUrb){

    struct mt_urb_list *pUrbList;

    //DBG("mt3351_usb_ep_urb_enqueue\n");

    if((pEnd->pCurrentUrb == NULL) && (list_empty(&pEnd->urb_list) == USB_TRUE)){
        pEnd->pCurrentUrb = pUrb;
    }
    else{
        pUrbList = kmalloc(sizeof(struct mt_urb_list), GFP_KERNEL);
        if(!pUrbList){
            return -ENOMEM;
        }
        INIT_LIST_HEAD(&pUrbList->urb_list);
        
        pUrbList->pUrb = pUrb;
        //DBG("pUrbList = %p\n", pUrbList);
        //DBG("pUrbList->urb_list = %p\n", &pUrbList->urb_list);
        //DBG("next = %p, prev = %p\n", pUrbList->urb_list.next, pUrbList->urb_list.prev);
        //DBG("pEnd = %p\n", pEnd);
        //DBG("pEnd->urb_list = %p\n", &pEnd->urb_list);
        //DBG("next = %p, prev = %p\n", pEnd->urb_list.next, pEnd->urb_list.prev);
        list_add_tail(&pUrbList->urb_list, &pEnd->urb_list);
    }

    pUrb->hcpriv = pEnd;

    return 0;   
}

static int mt_usb_ep_urb_dequeue(struct mt_ep *pEnd, struct urb *pUrb){

    struct mt_urb_list *urb_list, *next_urb_list;

    pUrb->hcpriv = NULL;

    if(pEnd->pCurrentUrb == pUrb){
        //DBG("mt3351_usb_ep_urb_dequeue, 1\n");
        pEnd->pCurrentUrb = NULL;
        pEnd->retries = 0;
        pEnd->traffic = 0;
        pEnd->dwOffset = 0;
        pEnd->dwRequestSize = 0;
        pEnd->dwIsoPacket = 0;
        pEnd->dwWaitFrame = 0;
    }
    else{
        //DBG("mt3351_usb_ep_urb_dequeue, 2\n");
        list_for_each_entry_safe(urb_list, next_urb_list, &pEnd->urb_list, urb_list){
            if(urb_list->pUrb == pUrb){
                list_del(&urb_list->urb_list);
                kfree(urb_list);
                break;
            }
        }
    }

    return 0;
}

static void mt_usb_set_interval(struct mt_ep *pEnd, struct urb *pUrb){

    unsigned int pipe = pUrb->pipe;
    u8 bInterval = 0;
    u8 bSpeed = mt_host_controller->bRootSpeed;

    if(usb_pipeint(pipe)){
        if(bSpeed == USB_ROOT_SPEED_HIGH){
            bInterval = mt_log2(pUrb->interval) + 1;
        }
        else{
            bInterval = pUrb->interval;
        }
    }
    else if(usb_pipeisoc(pipe)){
        if((bSpeed == USB_ROOT_SPEED_HIGH) || (bSpeed == USB_ROOT_SPEED_FULL)){
            bInterval = mt_log2(pUrb->interval) + 1;
        }
    }
    else if(usb_pipebulk(pipe)){
        if(bSpeed == USB_ROOT_SPEED_HIGH){
            if(pUrb->interval > 4096){
                bInterval = 16;
            }
            else{
                bInterval = mt_log2(pUrb->interval) + 1;
            }
        }
        else if(bSpeed == USB_ROOT_SPEED_FULL){
            if(pUrb->interval > 32768){
                bInterval = 16;
            }
            else{
                bInterval = mt_log2(pUrb->interval) + 1;
            }
        }
    }

    if(usb_pipeout(pipe)){
        __raw_writeb(bInterval, IECSR + TXINTERVAL);
        //DBG("TXINTERVAL = %d\n", bInterval);
    }
    else{
        __raw_writeb(bInterval, IECSR + RXINTERVAL);
        //DBG("RXINTERVAL = %d\n", bInterval);
    }
    
    return;
}

static void mt_usb_ep_setup(struct mt_ep *pEnd, struct urb *pUrb, u8 bXmt, void *pBuffer, u32 dwLength){

    u8 bEnd = pEnd->ep_num;
    unsigned int pipe = pUrb->pipe;
    u16 wPacketSize = usb_maxpacket(pUrb->dev, pipe, usb_pipeout(pipe));
    u8 bInterval;
    //u8 bIntrTxE;
    //u8 bIntrRxE;
    u8 bIntrUSBE;
    u8 bMode = MODE_TX; /* TX */
    u16 wCsr, wCount, wFrame;

    //static u8 counter = 0;

    __raw_writeb(bEnd, INDEX);

    /*
    if(bEnd){
        DBG("mt3351_usb_ep_setup:\n");
        DBG("    bEnd = %d\n", bEnd);
        DBG("    bXmt = %d\n", bXmt);
        DBG("    dwLength = %d\n", dwLength);
        if(bXmt){
            DBG("    fifoAdd = %d\n", __raw_readw(TXFIFOADD));
            DBG("    fifoSz = %d\n", __raw_readb(TXFIFOSZ));
            DBG("    MAXP = %d\n", __raw_readw(IECSR + TXMAP));
        }
        else{
            DBG("    fifoAdd = %d\n", __raw_readw(RXFIFOADD));
            DBG("    fifoSz = %d\n", __raw_readb(RXFIFOSZ));
            DBG("    MAXP = %d\n", __raw_readw(IECSR + RXMAP));
        }
    }
    */
    if(usb_pipecontrol(pUrb->pipe)){
        bXmt = 1;
    }
    
    mt_usb_set_protocol(pEnd, pUrb, bXmt);
    mt_usb_set_address(pEnd, pUrb);
    /*
    if(bXmt){
        DBG("SETUP: TX Transfer, EP %d\n", bEnd);
    }
    else{
        DBG("SETUP: RX Transfer, EP %d\n", bEnd);
    }
    */
    if(bXmt){ /* transmit, the transfer direction is from host to device */
        
        if(bEnd){
            
            __raw_writew(EPX_TX_FLUSHFIFO, IECSR + TXCSR);
            __raw_writew(EPX_TX_FLUSHFIFO, IECSR + TXCSR);

            /* set tx max packet size */
            __raw_writew(wPacketSize, IECSR + TXMAP);

            mt_usb_set_interval(pEnd, pUrb);
        }
        else{
            __raw_writew(EP0_FLUSH_FIFO, IECSR + CSR0);
            //__raw_writew(EP0_FLUSH_FIFO, IECSR + CSR0);

            bInterval = mt_log2(pUrb->interval) + 1;
            bInterval = (bInterval > 16) ? 16 : ((bInterval <= 1) ? 0 : bInterval);

            __raw_writeb(bInterval, IECSR + NAKLIMIT0);
        }

        if(bEnd){
            wCsr = __raw_readw(IECSR + TXCSR);
            if(bMode == MODE_TX){
                wCsr |= EPX_TX_MODE;
            }
            else{
                wCsr &= ~EPX_TX_MODE;
            }
            __raw_writew(wCsr, IECSR + TXCSR);
        }

        wCount = min((u32)wPacketSize, dwLength);

        if(bEnd){ /* not default control endpoint ep0 */
            mt_usb_tx_packet(pEnd);
        }
        else{
            if(wCount){
                pEnd->dwRequestSize = wCount;
                mt_usb_loadFifo(pEnd, wCount, pBuffer);
            }
        }

        if(usb_pipeisoc(pipe) || usb_pipeint(pipe)){
            wFrame = __raw_readw(FRAME);
            if ((pUrb->transfer_flags & USB_ISO_ASAP) || (wFrame >= pUrb->start_frame)){
    		    pEnd->dwWaitFrame = 0;
 
    		    mt_usb_starttx(bEnd);
    		}
    		else{
    		    pEnd->dwWaitFrame = pUrb->start_frame;

    		    /* enable SOF interrupt */
    		    bIntrUSBE = __raw_readb(INTRUSBE);
    		    bIntrUSBE |= INTRUSB_SOF;
    		    __raw_writeb(bIntrUSBE, INTRUSBE);
    		}
        }
        else{
            mt_usb_starttx(bEnd);
        }
    }
    else{/* bXmt = 0, this is an rx transfer */
        //printk("mt_usb_ep_setup: RX Transfer\n");
        wCsr = __raw_readw(IECSR + RXCSR);
        //DBG("mt_usb_ep_setup, RXCSR = %d\n", wCsr);
        if(wCsr & EPX_RX_RXPKTRDY){
            //printk("mt_usb_ep_setup: RX Packet Ready!!\n");
            wCount = __raw_readw(IECSR + RXCOUNT);
            printk("mt_usb_ep_setup: RX count = %d!!\n", wCount);
            mt_usb_rx_packet(pEnd, wCount);
        }

        if(bEnd){
            //DBG("mt_usb_ep_setup, wPacketSize = %d\n", wPacketSize);
            __raw_writew(wPacketSize, IECSR + RXMAP);
            mt_usb_set_interval(pEnd, pUrb);
            //printk("mt_usb_ep_setup: bEnd = %d!!\n", bEnd);
        }

        //DBG("mt_usb_ep_setup, RXCOUNT = %d\n", __raw_readw(IECSR + RXCOUNT));
        __raw_writew(EPX_RX_FLUSHFIFO, IECSR + RXCSR);
        __raw_writew(EPX_RX_FLUSHFIFO, IECSR + RXCSR);
        
        /* kick things off */
    	if (bEnd)
    	{
    	    wCsr = __raw_readw(IECSR + RXCSR);
    	    wCsr &= ~EPX_RX_RXPKTRDY;
    	    wCsr |= EPX_RX_REQPKT;
    	    /*
    	    if(bEnd == 2)
    	        counter++;
            */
    	    __raw_writew(wCsr, IECSR + RXCSR);
    	    
            /*
    	    if(counter == 2){

                while(1){
                    DBG("INDEX = %d\n", __raw_readb(INDEX));
                    DBG("TXCSR = %d\n", __raw_readw(IECSR + TXCSR));
                    DBG("TXFIFOSZ = %d\n", __raw_readb(TXFIFOSZ));
                    DBG("TXMAP = %d\n", __raw_readw(IECSR + TXMAP));
                    DBG("TXFIFOADDR = %d\n", __raw_readw(TXFIFOADD));
                    DBG("INTRTXE = %d\n", __raw_readb(INTRTXE));
                    DBG("TXTYPE = %d\n", __raw_readb(IECSR + TXTYPE));
                    DBG("RXCSR = %d\n", __raw_readw(IECSR + RXCSR));
                    DBG("RXFIFOSZ = %d\n", __raw_readb(RXFIFOSZ));
                    DBG("RXMAP = %d\n", __raw_readw(IECSR + RXMAP));
                    DBG("RXFIFOADDR = %d\n", __raw_readw(RXFIFOADD));
                    DBG("INTRRXE = %d\n", __raw_readb(INTRRXE));
                    DBG("RXTYPE = %d\n", __raw_readb(IECSR + RXTYPE));
                    DBG("RXCOUNT = %d\n", __raw_readw(IECSR + RXCOUNT));
                    DBG("\n");
                }
    	    }
            */
    	    /*
    	    DBG("RXCSR = %d\n", __raw_readw(IECSR + RXCSR));
    	    DBG("REQUEST IN PACKET\n");
    	    DBG("INTRRXE = %x\n", __raw_readb(INTRRXE));
    	    DBG("INTRTXE = %x\n", __raw_readb(INTRTXE));
    	    */
    	}
    }

    //DBG("mt_usb_ep_setup: end\n");

    return;
}

static void mt_usb_ep_urb_start(struct mt_ep *pEnd, struct urb *pUrb){

    unsigned int pipe;
    u8 xmt; /* transfer direction, 1: out, 0: in */
    u16 maxPacketSize;
    u8 remoteAddress, remoteEnd;
    u32 length;
    void *pBuffer;

    //DBG("mt_usb_ep_urb_start, ep %d\n", pEnd->ep_num);

    pipe = pUrb->pipe;
    xmt = usb_pipeout(pipe);
    maxPacketSize = usb_maxpacket(pUrb->dev, pipe, xmt);
    remoteAddress = usb_pipedevice(pipe);
    remoteEnd = usb_pipeendpoint(pipe);

    pUrb->actual_length = 0;
    pUrb->error_count = 0;

    pEnd->remoteAddress = remoteAddress;
    pEnd->remoteEnd = remoteEnd;
    pEnd->traffic = (u8)usb_pipetype(pipe);

    /* init urb */
    pEnd->dwOffset = 0;
    pEnd->dwRequestSize = 0;
    pEnd->dwIsoPacket = 0;
    pEnd->dwWaitFrame = 0;
    pEnd->wPacketSize = maxPacketSize;

    /* if the request is a control transfer */
    if(usb_pipecontrol(pipe)){
        xmt = USB_TRUE;
        mt_host_controller->ep0_state = EP0_IDLE;
    }

    if(usb_pipeisoc(pipe)){
        length = pUrb->iso_frame_desc[0].length;
        pBuffer = (void *)(pUrb->transfer_buffer + \
        pUrb->iso_frame_desc[0].length);
    }
    else if(usb_pipecontrol(pipe)){
        length = 8;
        pBuffer = (void *)pUrb->setup_packet;
    }
    else{/* if the urb is a control transfer or interrupt transfer */

        length = pUrb->transfer_buffer_length;
        pBuffer = (void *)pUrb->transfer_buffer;
      
    }

    if(!pBuffer){
        DBG("in mt_usb_ep_urb_start, pBuffer = NULL!!\n");
        return;
    }

    mt_usb_ep_setup(pEnd, pUrb, xmt, pBuffer, length);
    
    return;
}

static int mt_usb_ep_urb_complete(struct mt_ep *pEnd, struct urb *pUrb){

    if(mt_usb_ep_urb_dequeue(pEnd, pUrb) == 0){
        if(mt_complete_urb(pEnd, pUrb) == 0){
            mt_usb_ep_urb_start(pEnd, mt_get_next_urb(pEnd));
        }
    }
    else{
        DBG("in mt_usb_ep_urb_complete, something is wrong\n");
    }
    
    return 0;
}

static int mt_schedule_urb(struct mt_ep *pEnd, struct urb *pUrb){

    u8 idle;

    //DBG("mt_schedule_urb\n");

    if(pUrb->status != (-EINPROGRESS)){
        //DBG("mt_schedule_urb, pUrb->status = %d\n", pUrb->status);
        spin_unlock(&pEnd->lock);
        mt_usb_ep_urb_complete(pEnd, pUrb);
        spin_lock(&pEnd->lock);
    }

    idle = mt_ep_is_idle(pEnd);
    //DBG("mt_schedule_urb, idle = %d\n", idle);
    if(mt_usb_ep_urb_enqueue(pEnd, pUrb) != 0){
        return -EBUSY;
    }

    if(idle && (pEnd->busycompleting == 0)){
        mt_usb_ep_urb_start(pEnd, pUrb);
    }
    
    return 0;
}

static USB_BOOL mt_complete_urb(struct mt_ep *pEnd, struct urb *pUrb){
    
    USB_BOOL status = 0;

    pEnd->busycompleting = 1;
    
    spin_unlock(&pEnd->lock);

    usb_hcd_giveback_urb(mt_host_controller->hcd, pUrb, 0);

    spin_lock(&pEnd->lock);
    
    status = mt_ep_is_idle(pEnd);
    
    pEnd->busycompleting = 0;
    
    return status;
}

static void mt_unlink_urb(struct urb *pUrb){

    struct mt_ep *pEnd;
    u8 bIndex;
    u8 wIntr;

    DBG("mt3351_unlink_urb\n");

    pEnd = (struct mt_ep *)pUrb->hcpriv;

    if(pEnd){
        spin_lock(&pEnd->lock);

        if(pEnd->pCurrentUrb == pUrb){
            mt_usb_ep_urb_dequeue(pEnd, pUrb);
            bIndex = __raw_readb(INDEX);
            __raw_writeb(pEnd->ep_num, INDEX);

            /* if pEnd is not default control endpoint ep0 */
            if(pEnd->ep_num){
                /* if it's a tx endpoint */
                if(usb_pipeout(pUrb->pipe)){
                    /* flush fifo twice in case of double buffering is enabled */
                    __raw_writew(EPX_TX_FLUSHFIFO, IECSR + TXCSR);
                    __raw_writew(EPX_TX_FLUSHFIFO, IECSR + TXCSR);
                    
                    wIntr = __raw_readb(INTRTX);
                    /* if interrupt is pending on the endpoint associated 
                    with pUrb */
                    /* check me!! intrtx should be read-only and read-clear */
                    if(wIntr & (1 << pEnd->ep_num)){
                        __raw_writeb(1 << pEnd->ep_num, INTRTX);
                    }
                }
                else{ /* rx endpoint */
                    /* flush fifo twice in case of double buffering is enabled */
                    __raw_writew(EPX_RX_FLUSHFIFO, IECSR + RXCSR);
                    __raw_writew(EPX_RX_FLUSHFIFO, IECSR + RXCSR);

                    wIntr = __raw_readb(INTRRX);
                    /* if interrupt is pending on the endpoint assiciated with 
                    pUrb */
                    if(wIntr & (1 << pEnd->ep_num)){
                        wIntr &= ~(1 << pEnd->ep_num);
                        __raw_writeb(wIntr, INTRRX);
                    }
                }
            }
            else{ /* if pEnd is ep0 */
                /* flush fifo twice in case of double buffering is enabled */
                /* does not clear data toggle */
                    __raw_writew(EP0_FLUSH_FIFO, IECSR + CSR0);
                    //__raw_writew(EP0_FLUSH_FIFO, IECSR + CSR0);

            }

            __raw_writeb(bIndex, INDEX);
        }
        else{
            mt_usb_ep_urb_dequeue(pEnd, pUrb);
        }

        spin_unlock(&pEnd->lock); 
    }

    //DBG("mt3351_unlink_urb, urb status = %d\n", pUrb->status);
    usb_hcd_giveback_urb(mt_host_controller->hcd, pUrb, 0);

    return;
}

static void mt_usb_intr(u8 intrusb){

    //DBG("INTRUSB\n");

    if(intrusb & INTRUSB_RESUME){
        /* RESUME routine */
        //DBG("INTRUSB: RESUME\n");
    }

    if(intrusb & INTRUSB_SESS_REQ){
        /* SESSION REQ routine */
        //DBG("INTRUSB: SESSION REQ\n");
    }

    if(intrusb & INTRUSB_VBUS_ERROR){
        /* VBUS ERROR routine */
        //DBG("INTRUSB: VBUS ERROR\n");
    }

    if(intrusb & INTRUSB_SUSPEND){
        /* SUSPEND routine */
        //DBG("INTRUSB: SUSPEND\n");
    }

    if(intrusb & INTRUSB_CONN){
        /* CONNECT routine */
        DBG("INTRUSB: CONNECT");
        mt_usb_delTimer();
        mt_usb_setTimer(mt_usb_check_connect, CHECK_INSERT_DEBOUNCE);
    }

    if(intrusb & INTRUSB_DISCON){
        /* DISCONNECT routine */
        DBG("INTRUSB: DISCONNECT\n");
    }

    if(intrusb & INTRUSB_BABBLE){
        /* BABBLE routine */
        //DBG("INTRUSB: BABBLE\n");
    }

    if(intrusb & INTRUSB_SOF){
        /* SOF routine */
        //DBG("INTRUSB: SOF\n");
    }

    return;
}

static void mt_usb_loadFifo(struct mt_ep *pEnd, u16 wCount, const u8 *pSource){

    u32 dwCount = (u32)wCount;
    u32 dwBufferSize = 4;

    //DBG("mt3351_usb_loadFifo, count = %d\n", wCount);

    if(dwCount == 0)
        return;

    /* unaligned word access */
    if((dwCount > 0) && ((u32)pSource & 3 )){
        /* if there's still data waiting to be transmitted */
        while(dwCount){
            if(dwCount < 4){
                dwBufferSize = dwCount;
            }

            memcpy((void *)FIFO(pEnd->ep_num), (const void *)pSource, dwBufferSize);
            dwCount -= dwBufferSize;
            pSource += dwBufferSize;            
        }
    }
    else{ /* aligned word access */
        while ((dwCount > 3) && !((u32)pSource & 3)){
            //DBG("pSource = %p, dwCount = %d\n", pSource, dwCount);
            memcpy((void *)FIFO(pEnd->ep_num), pSource, dwBufferSize);
    	    pSource += 4;
    	    dwCount -= 4;
    	}

        while(dwCount > 0){
            //DBG("pSource = %p, dwCount = %d\n", pSource, dwCount);
            memcpy((void *)FIFO(pEnd->ep_num), pSource, 1);
            dwCount--;
            pSource++;
        }
    }

    return;
}

static void mt_usb_unloadFifo(struct mt_ep *pEnd, u16 wCount, u8 *pDest){

    u32 dwCount = (u32)wCount;
    u32 i;
    u8 ep_num = pEnd->ep_num;
    u32 dwData;

    if(dwCount == 0){
        return;
    }

    //DBG("mt3351_usb_unloadFifo\n");
    /* unaligned word access */
    if((u32)pDest & 3){
        while(dwCount > 0){
            if(dwCount < 4){

                dwData = __raw_readl(FIFO(ep_num));
                
                for (i = 0; i < dwCount; i++){
        		    *pDest++ = ((dwData >> (i * 8)) & 0xFF);
        		}
                
                dwCount = 0;
            }
            else{
                dwData = __raw_readl(FIFO(ep_num));
                
                for(i = 0; i < 4; i++){
                    *pDest++ = ((dwData >> (i * 8)) & 0xFF);
                }
                
                dwCount -= 4;
            }
        }
    }
    else{ /* aligned word access */
        /*
        if(ep_num != 0)
            printk("DATA: ");
        */
        while(dwCount >= 4){
            *((u32 *) ((void *) pDest)) = __raw_readl(FIFO(ep_num));
            /*
            if(ep_num != 0)
                printk("%x %x %x %x ", *pDest, *(pDest + 1), *(pDest + 2), *(pDest + 3));
            */
            pDest += 4;
            dwCount -= 4;
        }

        /* for the case 0 < dwCount < 4 */
        if(dwCount > 0){
            dwData = __raw_readl(FIFO(ep_num));
            for(i = 0; i < dwCount; i++){
                *pDest++ = ((dwData >> (i * 8)) & 0xFF);
                /*
                if(ep_num != 0)
                    printk("%x ", *(pDest - 1));
                */
            }
        }
        /*
        if(ep_num != 0)
            printk("\n");
        */
    }

    return;
}

static u8 mt_usb_ep0_packet(u16 wCount, struct urb *pUrb){

    u8 bMore = USB_FALSE;
    u8 *pFifoDest = NULL;
    u16 wFifoCount = 0;
    struct mt_ep *pEnd = &mt_host_controller->ep[0];
    mt_DeviceRequest *pRequest = (mt_DeviceRequest *)pUrb->setup_packet;

    if(mt_host_controller->ep0_state == EP0_RX){
        //DBG("mt3351_usb_ep0_packet: EP0_RX\n");
        /* receive data from USB device */
        pFifoDest = (u8 *)(pUrb->transfer_buffer + pUrb->actual_length);
        wFifoCount = min((int)wCount, pUrb->transfer_buffer_length - pUrb->actual_length);
        mt_usb_unloadFifo(pEnd, wFifoCount, pFifoDest);
        pUrb->actual_length += wFifoCount;
        if((pUrb->actual_length < pUrb->transfer_buffer_length) && \
           (wCount == pEnd->wPacketSize)){
            bMore = USB_TRUE;
            //DBG("More RX packets\n");
        }
    }
    else{
        if((mt_host_controller->ep0_state == EP0_IDLE) && (pRequest->bmRequestType & USB_DIR_IN)){
            mt_host_controller->ep0_state = EP0_RX;
            bMore = USB_TRUE;
        }
        else if((mt_host_controller->ep0_state == EP0_IDLE) && pRequest->wLength){
            mt_host_controller->ep0_state = EP0_TX;
        }

        if((mt_host_controller->ep0_state == EP0_TX) && (pRequest->wLength > pEnd->dwRequestSize)){
            pFifoDest = (u8 *)(pUrb->transfer_buffer + pUrb->actual_length);
            wFifoCount = min(pEnd->wPacketSize, (u16)(pUrb->transfer_buffer_length - pUrb->actual_length));
            mt_usb_loadFifo(pEnd, wFifoCount, pFifoDest);

            pEnd->dwRequestSize = wFifoCount;
            pUrb->actual_length += wFifoCount;

            if(wFifoCount){
                bMore = USB_TRUE;
            }
        }
    }

    return bMore;
}

static void mt_usb_ep0(void){

    struct mt_ep *pEnd = &mt_host_controller->ep[0];
    struct urb *pUrb = NULL;
    u8 count0;
    u16 csr0, value16, outval = 0;
    USB_BOOL complete = USB_FALSE, error = USB_FALSE;
    int status = USB_ST_NOERROR;

    //DBG("EP0 INTERRUPT\n");

    spin_lock(&pEnd->lock);
    
    pUrb = pEnd->pCurrentUrb;

    if(!pUrb){
        /* the endpoint is idle, has nothing to do */
        spin_unlock(&pEnd->lock);
        return;
    }

    __raw_writeb(0, INDEX);
    value16 = csr0 = __raw_readw(IECSR + CSR0);
    count0 = __raw_readw(IECSR + COUNT0);

    if(mt_host_controller->ep0_state == EP0_STATUS){
        complete = USB_TRUE;
    }

    if(csr0 & EP0_RXSTALL){
        status = USB_ST_STALL;
        error = USB_TRUE;
        DBG("ep0, stall\n");
    }
    else if(csr0 & EP0_ERROR){
        status = USB_ST_NORESPONSE;
        error = USB_TRUE;
        DBG("ep0, no response\n");
    }
    else if(csr0 & EP0_NAK_TIMEOUT){
        if(++pEnd->retries < USB_MAX_RETRIES){
            /* clear the content of CSR0 */
            __raw_writew(0, IECSR + CSR0);
        }
        else{
            pEnd->retries = 0;
            status = USB_ST_NORESPONSE;
            error = USB_TRUE;
            DBG("ep0, NAK LIMIT TIMEOUT\n");
        }
    }

    if(status == USB_ST_NORESPONSE){
        if(value16 & EP0_REQ_PKT){
            value16 &= ~EP0_REQ_PKT;
            __raw_writew(value16, IECSR + CSR0);
            value16 &= ~EP0_NAK_TIMEOUT;
            __raw_writew(value16, IECSR + CSR0);
        }
        else{
            value16 |= EP0_FLUSH_FIFO;
            /* flush twice in case of double buffering */
            __raw_writew(value16, IECSR + CSR0);
            __raw_writew(value16, IECSR + CSR0);
            value16 &= ~EP0_NAK_TIMEOUT;
            __raw_writew(value16, IECSR + CSR0);
        }

        /* check me!! doesn't know why we should do so */
        __raw_writew(0, IECSR + NAKLIMIT0);
    }

    if(error){
        DBG("in mt_usb_ep0, error\n");
        __raw_writew(0, IECSR + CSR0);
    }

    if(!complete && !error){
        if(mt_usb_ep0_packet(count0, pUrb)){
            /* more packets are required */
            outval = (EP0_RX == mt_host_controller->ep0_state) ? EP0_REQ_PKT : EP0_TXPKTRDY;
        }
        else{
            outval = EP0_STATUSPKT | (usb_pipeout(pUrb->pipe) ? EP0_REQ_PKT : EP0_TXPKTRDY);
            mt_host_controller->ep0_state = EP0_STATUS;
            //DBG("EP0: enter status stage\n");
        }
    }

    if(outval){
        __raw_writew(outval, IECSR + CSR0);
    }

    if(complete || error){
        if(mt_usb_ep_urb_dequeue(pEnd, pUrb) == 0){
            pUrb->status = status;
            if(mt_complete_urb(pEnd, pUrb) == 0){
                mt_usb_ep_urb_start(pEnd, mt_get_next_urb(pEnd));
            }
        }
    }

    spin_unlock(&pEnd->lock);
    
    return;
}

static u8 mt_usb_tx_packet(struct mt_ep *pEnd){

    u8 bDone = USB_FALSE;
    struct urb *pUrb;
    u16 wLength = 0;
    u8 *pBuffer;
    int pipe;
    u8 bEnd;
    int i;
    struct usb_iso_packet_descriptor *packet;

    if(!pEnd){
        return USB_TRUE;
    }

    pUrb = mt_get_next_urb(pEnd);
    if(!pUrb){
        return USB_TRUE;
    }

    pipe = pUrb ? pUrb->pipe : 0;
    bEnd = pEnd->ep_num;
    pBuffer = pUrb->transfer_buffer;
    if(!pBuffer){
        return USB_TRUE;
    }

    pEnd->dwOffset += pEnd->dwRequestSize;

    if(usb_pipeisoc(pipe)){
        /* isoch case */
    	if (pEnd->dwIsoPacket >= pUrb->number_of_packets)
    	{
            for (i = 0; i < pUrb->number_of_packets; i++) 
            {
                packet = &pUrb->iso_frame_desc[i];
                packet->status = 0;
                packet->actual_length = packet->length;
            }
	
    	    bDone = USB_TRUE;
    	}
    	else
    	{
    	    pBuffer += pUrb->iso_frame_desc[pEnd->dwIsoPacket].offset;
    	    wLength = pUrb->iso_frame_desc[pEnd->dwIsoPacket].length;
    	    pEnd->dwIsoPacket ++;
    	}
    }
    else{
        pBuffer += pEnd->dwOffset;

    	wLength = min(pEnd->wPacketSize, 
	    (uint16_t) (pUrb->transfer_buffer_length -pEnd->dwOffset));

    	if (pEnd->dwOffset >= pUrb->transfer_buffer_length)
    	{
    	    /* sent everything; see if we need to send a null */
    	    if (!((pEnd->dwRequestSize == pEnd->wPacketSize) && (pUrb->transfer_flags & USB_ZERO_PACKET)))
    	    {
        		bDone = USB_TRUE;
    	    }
    	    else
    	    {
        		// send null packet.
        		mt_usb_starttx(bEnd);

        		bDone = USB_FALSE;
    	    }
    	}
    }

    if (bDone)
    {
    	pUrb->status = 0;
    }
    else if (wLength)
    {				/* @assert bDone && !wLength */
        mt_usb_loadFifo(pEnd, wLength, pBuffer);

    	pEnd->dwRequestSize = wLength;
    }

    return bDone;
}

static u8 mt_usb_rx_packet(struct mt_ep *pEnd, u16 wCount){

    u16 wLength;
    u8 bDone = USB_FALSE;
    u8 bEnd;
    struct urb *pUrb;
    u8 *pBuffer;
    int pipe;
    u16 wPacketSize;
    u16 tmp;

    if(!pEnd){
        return USB_TRUE;
    }

    pUrb = mt_get_next_urb(pEnd);
    if(!pUrb){
        return USB_TRUE;
    }

    pipe = pUrb ? pUrb->pipe : 0;
    bEnd = pEnd->ep_num;
    pBuffer = pUrb->transfer_buffer;
    if(!pBuffer){
        return USB_TRUE;
    }

    __raw_writeb(bEnd, INDEX);

    if(usb_pipeisoc(pipe)){
        //DBG("mt3351_usb_rx_packet: ISOCHRONOUS\n");
        /* isoch case */
    	pBuffer += pUrb->iso_frame_desc[pEnd->dwIsoPacket].offset;
    	wLength = min((unsigned int) wCount, pUrb->iso_frame_desc[pEnd->dwIsoPacket].length);
    	pUrb->actual_length += wLength;

    	/* update actual & status */
    	pUrb->iso_frame_desc[pEnd->dwIsoPacket].actual_length = wLength;
        pUrb->iso_frame_desc[pEnd->dwIsoPacket].status = USB_ST_NOERROR;

    	/* see if we are done */
    	bDone = (++pEnd->dwIsoPacket >= pUrb->number_of_packets);

    	if (wLength)
    	{
    	    mt_usb_unloadFifo(pEnd, wLength, pBuffer);
    	}

    	if (bEnd && bDone)
    	{
    	    pUrb->status = 0;
    	}
    }
    else{
        //DBG("mt3351_usb_rx_packet: NOT ISOCHRONOUS\n");
        /* non-isoch */
    	pBuffer += pEnd->dwOffset;

    	wLength = min((unsigned int) wCount, pUrb->transfer_buffer_length - pEnd->dwOffset);

    	wPacketSize = usb_maxpacket(pUrb->dev, pUrb->pipe, USB_FALSE);

    	/*
        DBG("mt3351_usb_rx_packet: wLength = %d\n", wLength);
        DBG("                      wCount = %d\n", wCount);
        DBG("                      transfer_buffer_length - dwOffset = %d\n", \
        pUrb->transfer_buffer_length - pEnd->dwOffset);
        */
        
    	if ((usb_pipebulk(pipe)) && (wLength >= wPacketSize))
    	{
    	    // receive fifo data first.
    	    pUrb->actual_length += wLength;
    	    pEnd->dwOffset += wLength;
    	    mt_usb_unloadFifo(pEnd, wLength, pBuffer);

    	    // still need to receive.
    	    if (pUrb->transfer_buffer_length > pEnd->dwOffset)
    	    {
        		__raw_writew(0, IECSR + RXCSR);

        		wLength = pUrb->transfer_buffer_length - pEnd->dwOffset;        		
    	    }
    	}
    	else if (wLength > 0)
    	{
    	    pUrb->actual_length += wLength;
    	    pEnd->dwOffset += wLength;

            mt_usb_unloadFifo(pEnd, wLength, pBuffer);
    	}
    }

    /* see if we are done */
    bDone = (pEnd->dwOffset >= pUrb->transfer_buffer_length) || (wCount < pEnd->wPacketSize);
    
    if (!bDone)
    {				
       //DBG("mt3351_usb_rx_packet: MORE PACKETS\n");
       tmp = __raw_readw(IECSR + RXCSR);
       tmp |= EPX_RX_REQPKT;
       tmp &= ~EPX_RX_RXPKTRDY;
       __raw_writew(tmp, IECSR + RXCSR);
    }
    else{
        //DBG("mt3351_usb_rx_packet: DONE\n");
        tmp = __raw_readw(IECSR + RXCSR);
        tmp &= ~EPX_RX_RXPKTRDY;
        __raw_writew(tmp, IECSR + RXCSR);    
    }

    if (bEnd && bDone)
    {
    	pUrb->status = 0;
    }

    return 0;
}

static void mt_usb_tx(u8 intrtx){

    u8 ep_num = 1;
    u16 txcsr, value16;
    struct urb *pUrb;
    struct mt_ep *pEnd;
    u32 status = 0;
    USB_BOOL skip = USB_FALSE;

    //DBG("INTRTX\n");

    /* service all the tx endpoints */
    for(ep_num = MT_EP_TX_START; ep_num < MT_EP_RX_START; ep_num++){
        if(intrtx & (1 << ep_num)){
            
            /*
            DBG("mt3351_usb_tx, ep %d\n", ep_num);
            DBG("EP %d Status: \n", ep_num);
            DBG("    INDEX = %d\n", __raw_readb(INDEX));
            DBG("    TXFIFOADDR = %d\n", __raw_readw(TXFIFOADD));
            DBG("    TXFIFOSIZE = %d\n", __raw_readb(TXFIFOSZ));
            DBG("    TXMAP = %d\n", __raw_readw(IECSR + TXMAP));
            */
            pEnd = &mt_host_controller->ep[ep_num];

            spin_lock(&pEnd->lock);
            
            /* service tx endpoint number "ep_num" */
            __raw_writeb(ep_num, INDEX);
            value16 = txcsr = __raw_readw(IECSR + TXCSR);

            pUrb = pEnd->pCurrentUrb;
            
            if(txcsr & EPX_TX_RXSTALL){
                DBG("TX Stall\n");
                status = USB_ST_STALL;
            }
            else if(txcsr & EPX_TX_ERROR){
                DBG("TX Packet Error\n");
                status = USB_ST_NORESPONSE;

                value16 &= ~EPX_TX_FIFONOTEMPTY;
                value16 |= EPX_TX_FLUSHFIFO;

                __raw_writew(value16, IECSR + TXCSR);
                __raw_writew(value16, IECSR + TXCSR);
            }
            else if(txcsr & EPX_TX_NAK_TIMEOUT){
                DBG("TX NakLimit Timeout\n");
                if(pUrb->status == (-EINPROGRESS) \
                   && ++pEnd->retries < USB_MAX_RETRIES){
                    __raw_writew(EPX_TX_TXPKTRDY, IECSR + TXCSR);
                    spin_unlock(&pEnd->lock);
                    return;
                }

                if(pUrb->status == (-EINPROGRESS)){
                    status = -ECONNRESET;
                }

                value16 &= ~EPX_TX_FIFONOTEMPTY;
                value16 |= EPX_TX_FLUSHFIFO;
                __raw_writew(value16, IECSR + TXCSR);
                __raw_writew(value16, IECSR + TXCSR);

                __raw_writeb(0, IECSR + TXINTERVAL);
                pEnd->retries = 0;
            }
            else if(txcsr & EPX_TX_FIFONOTEMPTY){
                DBG("TX FIFO NOT EMPTY\n");
                skip = USB_TRUE;
            }

            if(status){
                DBG("Something is wrong with the TX transfer\n");
                pUrb->status = status;

                value16 &= ~(EPX_TX_ERROR | EPX_TX_RXSTALL | EPX_TX_NAK_TIMEOUT);
                value16 |= EPX_TX_FRCDATATOG;

                __raw_writew(value16, IECSR + TXCSR);
            }

            if(!skip && pUrb->status == (-EINPROGRESS)){
                mt_usb_tx_packet(pEnd);
            }

            if(pUrb->status != (-EINPROGRESS)){
                pUrb->actual_length = pEnd->dwOffset;
                mt_usb_ep_urb_complete(pEnd, pUrb);
            }
            else{
                if(!skip){
                    value16 = __raw_readw(IECSR + TXCSR);
                    value16 |= EPX_TX_TXPKTRDY;
                    __raw_writew(value16, IECSR + TXCSR);
                }
            }

            spin_unlock(&pEnd->lock);
        }
    }

    return;
}

static int mt_usb_rx(u8 intrrx){

    u8 ep_num = 1;
    u16 rxcsr, value16, count;
    struct urb *pUrb;
    struct mt_ep *pEnd;
    u32 status = 0;
    //unsigned long flags;
    //u8 *pBuffer;
    //int pipe;
    USB_BOOL iso_error = USB_FALSE;//, done = USB_FALSE;

    //DBG("INTRRX\n");

    /* service all the rx endpoints */
    for(ep_num = MT_EP_RX_START; ep_num < MT_EP_NUM; ep_num++){
        if(intrrx & (1 << ep_num)){
            
            /*
            DBG("mt3351_usb_rx, ep %d\n", ep_num);
            DBG("EP %d Status: \n", ep_num);
            DBG("    INDEX = %d\n", __raw_readb(INDEX));
            DBG("    RXFIFOADDR = %d\n", __raw_readw(RXFIFOADD));
            DBG("    RXFIFOSIZE = %d\n", __raw_readb(RXFIFOSZ));
            DBG("    RXMAP = %d\n", __raw_readw(IECSR + RXMAP));
            */
            pEnd = &mt_host_controller->ep[ep_num];

            spin_lock(&pEnd->lock);
            
            /* service rx endpoint number "ep_num" */
            __raw_writeb(ep_num, INDEX);
            value16 = rxcsr = __raw_readw(IECSR + RXCSR);
            count = __raw_readw(IECSR + RXCOUNT);
            /*
            DBG("EP %d\n", ep_num);
            DBG("RXCSR = %d\n", rxcsr);
            DBG("RXCOUNT = %d\n", count);
            DBG("bEnd = %d\n", ep_num);    
            DBG("fifoAdd = %d\n", __raw_readw(RXFIFOADD));
            DBG("fifoSz = %d\n", __raw_readb(RXFIFOSZ));
            DBG("MAXP = %d\n", __raw_readw(IECSR + RXMAP));
            */

            pUrb = mt_host_controller->ep[ep_num].pCurrentUrb;
            
            if(rxcsr & EPX_RX_RXSTALL){
                //DBG("RX Stall\n");
                status = USB_ST_STALL;
            }
            else if(rxcsr & EPX_RX_ERROR){
                DBG("RX Packet Error\n");
                status = -ECONNRESET;

                value16 &= ~EPX_RX_REQPKT;
                __raw_writew(value16, IECSR + RXCSR);
                __raw_writeb(0, IECSR + RXINTERVAL);               
            }
            else if(rxcsr & EPX_RX_DATAERR){
                DBG("RX Data Error\n");
                if(pEnd->traffic == USB_BULK){
                    if(pUrb->status == (-EINPROGRESS) && \
                        pEnd->retries < USB_MAX_RETRIES){
                        value16 &= ~EPX_RX_DATAERR;
                        value16 &= ~EPX_RX_RXPKTRDY;
                        value16 |= EPX_RX_REQPKT;
                        __raw_writew(value16, IECSR + RXCSR);
                        spin_unlock(&pEnd->lock);
                        // check me!! replace with correct error code
                        return -1;
                    }

                    if(pUrb->status == (-EINPROGRESS)){
                        status = -ECONNRESET;
                    }

                    value16 &= ~EPX_RX_REQPKT;
                    __raw_writew(value16, IECSR + RXCSR);
                    __raw_writeb(0, IECSR + RXINTERVAL);
                    pEnd->retries = 0;
                }
                else if(pEnd->traffic == USB_ISOCHRONOUS){
                    iso_error = USB_TRUE;
                }
            }

            if(status){
                //DBG("Something is wrong\n");
                pUrb->status = status;

                if(status == USB_ST_STALL){
                    /* flush fifo and clear data toggle          *
                     * twice in case double buffering is enabled */
                    __raw_writew(EPX_RX_FLUSHFIFO | EPX_RX_CLRDATATOG, IECSR + RXCSR);
                    __raw_writew(EPX_RX_FLUSHFIFO | EPX_RX_CLRDATATOG, IECSR + RXCSR);
                }

                value16 &= ~(EPX_RX_RXSTALL | EPX_RX_DATAERR | EPX_RX_ERROR);
                value16 &= ~EPX_RX_RXPKTRDY;
                __raw_writew(value16, IECSR + RXCSR);
            }

            //DBG("pUrb->status = %d\n", pUrb->status);

            /* no error occurs, start unloading */
            if(pUrb->status == -EINPROGRESS){
                if(!(rxcsr & EPX_RX_RXPKTRDY)){
                    pUrb->status = USB_ST_INTERNALERROR;
                    value16 &= ~EPX_RX_REQPKT;
                    __raw_writew(value16, IECSR + RXCSR);
                }
                else{
                    mt_usb_rx_packet(pEnd, count);
                }
            }

            if(pUrb->status != -EINPROGRESS){
                mt_usb_ep_urb_complete(pEnd, pUrb);
            }

            spin_unlock(&pEnd->lock);
        }
    }

    return 0;
}

/* turn off reset signal after 80ms */
static void mt_usb_reset_off(unsigned long unused){

    u8 power;
    unsigned long flags;
    int ep_num;

    //DBG("mt3351_usb_reset_off\n");

    spin_lock_irqsave(&mt_host_controller->lock, flags);

    power = __raw_readb(POWER);
    power &= ~PWR_RESET;
    __raw_writeb(power, POWER);

    if(power & PWR_HS_MODE){
        //DBG("mt3351_usb_reset_off, high-speed\n");
        mt_host_controller->bRootSpeed = USB_ROOT_SPEED_HIGH;
    }

    /* disable reset status */
    mt_host_controller->virtualHubPortStatus &= ~(1 << USB_PORT_FEAT_RESET);

    /* reset complete, device still connected, set port enable */
    mt_host_controller->virtualHubPortStatus |= (1 << USB_PORT_FEAT_C_RESET) | (1 << USB_PORT_FEAT_ENABLE);

    if(mt_host_controller->bRootSpeed == USB_ROOT_SPEED_HIGH){
        mt_host_controller->virtualHubPortStatus &= ~(1 << USB_PORT_FEAT_LOWSPEED);
        mt_host_controller->virtualHubPortStatus |= (1 << USB_PORT_FEAT_HIGHSPEED);
        //DBG("New High-Speed USB Device Connected\n");
    }
    else if(mt_host_controller->bRootSpeed == USB_ROOT_SPEED_FULL){
        mt_host_controller->virtualHubPortStatus &= ~(1 << USB_PORT_FEAT_LOWSPEED);
        mt_host_controller->virtualHubPortStatus &= ~(1 << USB_PORT_FEAT_HIGHSPEED);

        /* Reset Endpoint Configuration According to USB Spec */
        for(ep_num = MT_EP_TX_START; ep_num < MT_EP_RX_START; ep_num++){
            mt_host_controller->ep[ep_num].wPacketSize = 64;
            __raw_writew(mt_host_controller->ep[ep_num].wPacketSize, IECSR + TXMAP);
        }

        for(ep_num = MT_EP_RX_START; ep_num < MT_EP_NUM; ep_num++){
            mt_host_controller->ep[ep_num].wPacketSize = 64;
            __raw_writew(mt_host_controller->ep[ep_num].wPacketSize, IECSR + RXMAP);
        }
        
        //DBG("New Full-Speed USB Device Connected\n");
    }
    else{/* low speed device */
        mt_host_controller->virtualHubPortStatus |= (1 << USB_PORT_FEAT_LOWSPEED);
        mt_host_controller->virtualHubPortStatus &= ~(1 << USB_PORT_FEAT_HIGHSPEED);
        //DBG("New Low-Speed USB Device Connected\n");
    }

    spin_unlock_irqrestore(&mt_host_controller->lock, flags);

    return;
}

irqreturn_t	mt_hcd_irq(struct usb_hcd *hcd){

    u8 intrtx, intrrx, intrusb;
    u8 devctl;
    //u8 index;

    //DBG("MT3351 HOST CONTROLLER INTERRUPT\n");

    devctl = __raw_readb(DEVCTL);
    if(!(devctl & DEVCTL_HOST_MODE)){ // MT6516 USB is acting as USB device
        printk("MT6516 USB Host ISR: Device Mode\n");
        return IRQ_HANDLED;
    }

    /* save interrupt status of tx, rx, and usb */
    intrtx = __raw_readb(INTRTX);
    intrrx = __raw_readb(INTRRX);
    intrusb = __raw_readb(INTRUSB);

    if(intrusb){
        mt_usb_intr(intrusb);
    }

    if(intrtx){
        if(intrtx & 1){
            mt_usb_ep0();
        }
        else{
            mt_usb_tx(intrtx);
        }
    }

    if(intrrx){
        mt_usb_rx(intrrx);
        __raw_writeb(0, INTRRX);
    }

    return IRQ_HANDLED;
}

int	 mt_hcd_start(struct usb_hcd *hcd){

    DBG("MT_HCD_START\n");

    hcd->state = HC_STATE_RUNNING;

    return 0;
}

void mt_hcd_stop(struct usb_hcd *hcd){

    DBG("MT_HCD_STOP\n");
    /* do nothing */

    return;
}

int	 mt_hcd_get_frame(struct usb_hcd *hcd){

    DBG("MT_HCD_GET_FRAME\n");
    
    return __raw_readw(FRAME);
}

int	 mt_hcd_urb_enqueue(struct usb_hcd *hcd,
					        struct urb *urb,
					        unsigned mem_flags){
	struct mt_ep *pEnd = NULL;
    //unsigned long flags;

    //DBG("mt3351_hcd_urb_enqueue\n");
    
    /* find the appropriate endpoint for servicing this urb */
    pEnd = mt_find_ep(urb);

    spin_lock(&pEnd->lock);

    //DBG("Before mt3351_schedule_urb, pEnd->ep_num = %d\n", pEnd->ep_num);
    /* schedule the urb to the endpoint */
    mt_schedule_urb(pEnd, urb);
    
    spin_unlock(&pEnd->lock);

    return 0;
}

int	 mt_hcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status){

    DBG("mt3351_hcd_urb_dequeue, status = %d\n", status);

    mt_unlink_urb(urb);
    
    return 0;
}

void mt_hcd_endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *ep){

    //DBG("mt3351_hcd_endpoint_disable\n");

    /* do nothing temporarily */

    return;
}

int  mt_hub_status_data(struct usb_hcd *hcd, char *buf){

    //DBG("MT3351_HUB_STATUS_DATA\n");

    unsigned long flags;
    s32 retval;
    //u8 tmp;

    spin_lock_irqsave(&mt_host_controller->lock, flags);
    /*
    tmp = __raw_readb(DEVCTL);
    
    if(tmp & DEVCTL_HOST_MODE){
        printk("HOST-MODE(default: device mode)\n");
    }
    else{
        printk("DEVICE-MODE(default: device mode)\n");
    }
    
    tmp &= DEVCTL_VBUS_MASK;
    tmp = tmp >> 3;

    switch(tmp){

        case 0:
            printk("VBUS: Below Session End\n");
            break;
        case 1:
            printk("VBUS: Above Session End, below AValid\n");
            break;
        case 2:
            printk("VBUS: Above AValid, below VBusValid\n");
            break;
        case 3:
            printk("VBUS: Above VBusValid\n");
            break;
        default:
            printk("VBUS: Undefined Status\n"); 
    }
    */
    if (!(mt_host_controller->virtualHubPortStatus & MTK_PORT_C_MASK))
    {
    	retval = 0;
    }
    else
    {
    	// Hub port status change. Port 1 change detected.
    	*buf = (1 << 1);
    	retval = 1;
    }

    spin_unlock_irqrestore(&mt_host_controller->lock, flags);

    return retval;
}

int  mt_hub_control(struct usb_hcd *hcd,
        				u16 typeReq, u16 wValue, u16 wIndex,
		        		char *buf, u16 wLength){

    u8 power;

    //DBG("MT3351_HUB_CONTROL: ");

    switch(typeReq){
        case ClearHubFeature:
            //DBG("Clear Hub Feature\n");
        case SetHubFeature:
            //DBG("Set Hub Feature - ");
        	switch (wValue)
        	{
            	case C_HUB_OVER_CURRENT:
            	    //DBG("C_HUB_OVER_CURRENT\n");
            	case C_HUB_LOCAL_POWER:
            	    //DBG("C_HUB_LOCAL_POWER\n");
        	    break;

            	default:
            	    goto error;
        	}

        	break;

        case ClearPortFeature:
            //DBG("Clear Port Feature - ");
        	if (wIndex != 1 || wLength != 0)
        	    goto error;

        	switch (wValue)
        	{
            	case USB_PORT_FEAT_ENABLE:
            	    //DBG("USB_PORT_FEAT_ENABLE\n");
            	    mt_host_controller->virtualHubPortStatus &= (1 << USB_PORT_FEAT_POWER);
            	    break;

            	case USB_PORT_FEAT_SUSPEND:
            	    //DBG("USB_PORT_FEAT_SUSPEND\n");
            	    if (!(mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_SUSPEND)))
            		    break;

            		//DBG("START RESUME...\n");
            		break;

            	case USB_PORT_FEAT_POWER:
            	    //DBG("USB_PORT_FEAT_POWER\n");
            	    break;
            	case USB_PORT_FEAT_C_ENABLE:
            	    //DBG("USB_PORT_FEAT_C_ENABLE\n");
            	    break;
            	case USB_PORT_FEAT_C_SUSPEND:
            	    //DBG("USB_PORT_FEAT_C_SUSPEND\n");
            	    break;
            	case USB_PORT_FEAT_C_CONNECTION:
            	    //DBG("USB_PORT_FEAT_C_CONNECTION\n");
            	    break;
            	case USB_PORT_FEAT_C_OVER_CURRENT:
            	    //DBG("USB_PORT_FEAT_C_OVER_CURRENT\n");
            	    break;
            	case USB_PORT_FEAT_C_RESET:
            	    //DBG("USB_PORT_FEAT_C_RESET\n");
            	    break;

            	default:
            	    goto error;
        	}

        	mt_host_controller->virtualHubPortStatus &= ~(1 << wValue);
        	break;

        case GetHubDescriptor:
            //DBG("Get Hub Descriptor\n");
        	mt_hub_descriptor((struct usb_hub_descriptor *) buf);
        	break;

        case GetHubStatus:
            //DBG("Get Hub Status\n");
        	*(__le32 *) buf = cpu_to_le32(0);
        	break;

        case GetPortStatus:
            //DBG("Get Port Status\n");
        	if (wIndex != 1)
        	    goto error;

        	*(__le32 *) buf = cpu_to_le32(mt_host_controller->virtualHubPortStatus);
        	/*
        	DBG("VIRTUAL HUB PORT STATUS = \n");
        	DBG("   PORT_CONNECTION : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_CONNECTION)) ? 1 : 0); 
        	DBG("   PORT_ENABLE : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_ENABLE)) ? 1 : 0);
        	DBG("   PORT_SUSPEND : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_SUSPEND)) ? 1 : 0);
            DBG("   PORT_OVERCURRENT : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_OVER_CURRENT)) ? 1 : 0);
            DBG("   PORT_RESET : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_RESET)) ? 1 : 0);
            DBG("   PORT_POWER : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_POWER)) ? 1 : 0);
            DBG("   PORT_LOW_SPEED : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_LOWSPEED)) ? 1 : 0);
            DBG("   PORT_HIGH_SPEED : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_HIGHSPEED)) ? 1 : 0);
            DBG("   PORT_TEST : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_TEST)) ? 1 : 0);
            DBG("   PORT_INDICATOR : %d\n", (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_INDICATOR)) ? 1 : 0);
            */
        	break;

        case SetPortFeature:
            //DBG("Set Port Feature - ");
        	if (wIndex != 1 || wLength != 0)
        	    goto error;

        	switch (wValue)
        	{
            	case USB_PORT_FEAT_ENABLE:
            	    //DBG("USB_PORT_FEAT_ENABLE\n");
            	    break;

            	case USB_PORT_FEAT_SUSPEND:
            	    //DBG("USB_PORT_FEAT_SUSPEND\n");
            	    if (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_RESET))
                		goto error;

            	    if (!(mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_ENABLE)))
                		goto error;

            	    break;

            	case USB_PORT_FEAT_POWER:
            	    //DBG("USB_PORT_FEAT_POWER\n");
            	    //mt_host_controller->virtualHubPortStatus |= (1 << USB_PORT_FEAT_POWER);
            	    break;

            	case USB_PORT_FEAT_RESET:
            	    //DBG("USB_PORT_FEAT_RESET\n");
            	    if (mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_SUSPEND))
                		goto error;

            	    if (!(mt_host_controller->virtualHubPortStatus & (1 << USB_PORT_FEAT_POWER))){
            	        //DBG("RESET while POWER doesn't exist!!\n");
                		break;
            	    }

            	    // reset port.
            	    power = __raw_readb(POWER);
                    power &= ~(PWR_SUSPEND_MODE |PWR_ENABLE_SUSPENDM);
                    power |= PWR_RESET;
                    __raw_writeb(power, POWER);
                    mt_usb_setTimer(mt_usb_reset_off, 50);

            	    break;

            	default:
            	    goto error;
        	}

        	mt_host_controller->virtualHubPortStatus |= 1 << wValue;
        	break;

        }

    return 0;

    error:

    DBG("ERROR!!\n");
    return -1;
}

int	 mt_bus_suspend(struct usb_hcd *hcd){

    DBG("MT_BUS_SUSPEND\n");
    
    return 0;
}

int	 mt_bus_resume(struct usb_hcd *hcd){

    DBG("MT_BUS_RESUME\n");
    
    return 0;
}

void mt_hub_irq_enable(struct usb_hcd *hcd){

    //DBG("MT3351_HUB_IRQ_ENABLE\n");
    
    return;
}

static const struct hc_driver mt_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"MTK Host Controller Driver",
	.hcd_priv_size =	sizeof(struct mt_hc),

	/*
	 * generic hardware linkage
	 */
	.irq =			mt_hcd_irq,
	.flags =		HCD_USB2 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		mt_hcd_start,
	.suspend =      NULL,
	.resume =       NULL,
	.stop =			mt_hcd_stop,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		mt_hcd_urb_enqueue,
	.urb_dequeue =		mt_hcd_urb_dequeue,
	.endpoint_disable =	mt_hcd_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	mt_hcd_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	mt_hub_status_data,
	.hub_control =	    mt_hub_control,

	.bus_suspend =		mt_bus_suspend,
	.bus_resume =		mt_bus_resume,

	.hub_irq_enable =   mt_hub_irq_enable,
};

static void mt_hcd_hw_init(void)
{
    u8  tmpReg8;
    u16 tmpReg16;
    u32 tmpReg32;

    DBG("mt_hcd_hw_init\n");

    /* AP MCU Uses USB */
    tmpReg16 = __raw_readw(0xf0001020);
    tmpReg16 &= ~(1 << 0);
    __raw_writew(tmpReg16, 0xf0001020);

    /* disable usb power down */
    __raw_writel(0x00000002, 0xf0039340);
    
    /* Power On USB */
    /* Nothing is done */

    /* Turn on AHB clock(52MHz) */
    //__raw_writew(0x3, 0xf0001208);
   
    /* Turn on internal 48MHz PLL */
    //__raw_writew(0x40, 0xf0060014);
    
    /* Wait 50usec */
    udelay(50);

    /* Set FORCE_AUX_EN = 0 and FORCE_USB_CLKON = 1 */
    
    tmpReg32 = __raw_readl(USB_PHY_INTF2);
    tmpReg32 &= ~USB_PHY_INTF2_FORCE_AUX_EN;
    tmpReg32 |= USB_PHY_INTF2_FORCE_USB_CLKON;
    __raw_writel(tmpReg32, USB_PHY_INTF2);
    

    /* Turn on pll_vcog[1] to disable gated 48MHz PLL */
    tmpReg32 = __raw_readl(USB_PHY_CON1);
    tmpReg32 &= ~USB_PHY_CON1_PLL_VCOG0;
    tmpReg32 |= USB_PHY_CON1_PLL_VCOG1;
    __raw_writel(tmpReg32, USB_PHY_CON1);
    
    /* Turn on bandgap */
    tmpReg32 = __raw_readl(USB_PHY_CON4);
    tmpReg32 |= USB_PHY_CON4_BGR_BGR_EN;
    __raw_writel(tmpReg32, USB_PHY_CON4);
 
    /* Wait 10usec */
    udelay(10);

    /* Release force suspendm */
    tmpReg32 = __raw_readl(USB_PHY_CON5);
    tmpReg32 &= ~USB_PHY_CON5_FORCE_SUSPENDM;
    __raw_writel(tmpReg32, USB_PHY_CON5);
   
    /* Wait 20usec */
    udelay(20);

    /* pll_en = 1 */
    tmpReg32 = __raw_readl(USB_PHY_CON1);
    tmpReg32 |= USB_PHY_CON1_PLL_EN;
    __raw_writel(tmpReg32, USB_PHY_CON1);

    __raw_writeb(0x0, INTRTXE);
    __raw_writeb(0x0, INTRRXE);
    __raw_writeb(0x0, INTRUSBE);
    
    __raw_readb(INTRTX);
    __raw_readb(INTRRX);
    __raw_writeb(0, INTRRX);
    __raw_readb(INTRUSB);

    /* Use Full-speed first */
    tmpReg8 = __raw_readb(POWER);
    tmpReg8 |= PWR_HS_ENAB;
    //tmpReg8 &= ~PWR_HS_ENAB;
    __raw_writeb(tmpReg8, POWER);

    /* reset INTRUSBE */
    __raw_writeb(0x77, INTRUSBE);

    tmpReg8 = __raw_readb(DEVCTL);
    tmpReg8 |= DEVCTL_SESSION;
    __raw_writeb(tmpReg8, DEVCTL);

    return;
}

static void mt_hcd_hw_deinit(void)
{
    DBG("mt_hcd_hw_deinit, no-op currently \n");
    
    return;
}

static void mt_hcd_ep_init(void){

    unsigned long flags;
    struct mt_ep *ep;
    unsigned int ep_num;
    u16 reg = 0;

    spin_lock_irqsave(&mt_host_controller->lock, flags);

    __raw_writeb(0, INTRTXE);
    __raw_writeb(0, INTRRXE);

    mt_host_controller->fifoAddr = 0;
    
    /* initialize endpoint 0 */
    ep = &mt_host_controller->ep[0];
    spin_lock_init(&ep->lock);

    INIT_LIST_HEAD(&ep->urb_list);

    ep->ep_num = 0;
    ep->dev = NULL;
    ep->wPacketSize = 64;
    ep->type = USB_CTRL;
    ep->traffic = USB_CONTROL;
    ep->pCurrentUrb = NULL;
    ep->retries = 0;
    ep->busycompleting = 0;
    ep->flags = 0;
    ep->remoteAddress = 0;
    ep->remoteEnd = 0;
    ep->dwOffset = 0;
    ep->dwRequestSize = 0;
    ep->dwIsoPacket = 0;
    ep->dwWaitFrame = 0;

    /* initialize FIFO */
    ep->fifoAddr = 0;

    mt_host_controller->fifoAddr = FIFO_START_ADDR;

    __raw_writeb(0, FADDR);

    reg = __raw_readb(INTRTXE);
    reg |= EPMASK(0);
    __raw_writeb(reg, INTRTXE);
    
    /* initialize Tx endpoints */
    for(ep_num = MT_EP_TX_START; ep_num < MT_EP_RX_START; ep_num++){    

        ep = &mt_host_controller->ep[ep_num];
        
        spin_lock_init(&ep->lock);

        INIT_LIST_HEAD(&ep->urb_list);
        
        ep->ep_num = ep_num;
        ep->dev = NULL;
        ep->wPacketSize = 512;
        ep->type = USB_TX;
        ep->traffic = USB_BULK;
        ep->pCurrentUrb = NULL;
        ep->retries = 0;
        ep->busycompleting = 0;
        ep->flags = 0;
        ep->remoteAddress = 0;
        ep->remoteEnd = 0;
        ep->dwOffset = 0;
        ep->dwRequestSize = 0;
        ep->dwIsoPacket = 0;
        ep->dwWaitFrame = 0;
        
        ep->fifoAddr = mt_host_controller->fifoAddr;

        __raw_writeb(ep_num, INDEX);
        __raw_writeb(ep_num | PROTOCOL_BULK, IECSR + TXTYPE);
        __raw_writew(ep->wPacketSize, IECSR + TXMAP);
        
        /* initialize TXCSR(Tx Control Status Register) */
        reg = __raw_readw(IECSR + TXCSR);
        reg |= (EPX_TX_CLRDATATOG | EPX_TX_FLUSHFIFO);
        __raw_writew(reg, IECSR + TXCSR);
        __raw_writew(reg, IECSR + RXCSR);
        
        /* initialize FIFO SIZE(Tx Max Packet Size) */
        switch(ep->wPacketSize){
            case 8:
                __raw_writeb(PKTSZ_8, TXFIFOSZ);
                break;
            case 16:
                __raw_writeb(PKTSZ_16, TXFIFOSZ);
                break;
            case 32:
                __raw_writeb(PKTSZ_32, TXFIFOSZ);
                break;
            case 64:
                __raw_writeb(PKTSZ_64, TXFIFOSZ);
                break;
            case 128:
                __raw_writeb(PKTSZ_128, TXFIFOSZ);
                break;
            case 256:
                __raw_writeb(PKTSZ_256, TXFIFOSZ);
                break;
            case 512:
                __raw_writeb(PKTSZ_512, TXFIFOSZ);
                break;
            case 1024:
                __raw_writeb(PKTSZ_1024, TXFIFOSZ);
                break;
            case 2048:
                __raw_writeb(PKTSZ_2048, TXFIFOSZ);
                break;
            default:
                DBG("TX FIFO Size Not Supported!!\n");
        }

        /* initialize TXFIFOADD(Tx FIFO Address) */
        __raw_writew((ep->fifoAddr), TXFIFOADD);

        /* initialize INTRTXE(TX Interrupt Enable) */
        reg = __raw_readb(INTRTXE);
        reg |= EPMASK(ep_num);
        __raw_writeb(reg, INTRTXE);
        
        mt_host_controller->fifoAddr += ep->wPacketSize;
    }

    /* initialize Rx endpoints */
    for(ep_num = MT_EP_RX_START; ep_num < MT_EP_NUM; ep_num++){
        
        ep = &mt_host_controller->ep[ep_num];
        
        spin_lock_init(&ep->lock);

        INIT_LIST_HEAD(&ep->urb_list);

        ep->ep_num = ep_num;
        ep->dev = NULL;
        ep->wPacketSize = 512;
        ep->type = USB_RX;
        ep->traffic = USB_BULK;
        ep->pCurrentUrb = NULL;
        ep->retries = 0;
        ep->busycompleting = 0;
        ep->flags = 0;
        ep->remoteAddress = 0;
        ep->remoteEnd = 0;
        ep->dwOffset = 0;
        ep->dwRequestSize = 0;
        ep->dwIsoPacket = 0;
        ep->dwWaitFrame = 0;
        
        ep->fifoAddr = mt_host_controller->fifoAddr;

        __raw_writeb(ep_num, INDEX);
        __raw_writeb(ep_num | PROTOCOL_BULK, IECSR + RXTYPE);

        __raw_writew(ep->wPacketSize, IECSR + RXMAP);

        /* initialize RXCSR(Rx Control Status Register) */
        reg = __raw_readw(IECSR + RXCSR);
        reg |= EPX_RX_CLRDATATOG | EPX_RX_FLUSHFIFO;
        __raw_writew(reg, IECSR + RXCSR);
        __raw_writew(reg, IECSR + RXCSR);
        
        /* initialize FIFO SIZE(Rx Max Packet Size) */
        switch(ep->wPacketSize){
            case 8:
                __raw_writeb(PKTSZ_8, RXFIFOSZ);
                break;
            case 16:
                __raw_writeb(PKTSZ_16, RXFIFOSZ);
                break;
            case 32:
                __raw_writeb(PKTSZ_32, RXFIFOSZ);
                break;
            case 64:
                __raw_writeb(PKTSZ_64, RXFIFOSZ);
                break;
            case 128:
                __raw_writeb(PKTSZ_128, RXFIFOSZ);
                break;
            case 256:
                __raw_writeb(PKTSZ_256, RXFIFOSZ);
                break;
            case 512:
                __raw_writeb(PKTSZ_512, RXFIFOSZ);
                break;
            case 1024:
                __raw_writeb(PKTSZ_1024, RXFIFOSZ);
                break;
            case 2048:
                __raw_writeb(PKTSZ_2048, RXFIFOSZ);
                break;
            default:
                DBG("RX FIFO Size Not Supported!!\n");
        }

        /* initialize RXFIFOADD(Rx FIFO Address) */
        __raw_writew((ep->fifoAddr), RXFIFOADD);

        /* initialize INTRRXE(RX Interrupt Enable) */
        reg = __raw_readb(INTRRXE);
        reg |= EPMASK(ep_num);
        __raw_writeb(reg, INTRRXE);

        mt_host_controller->fifoAddr += ep->wPacketSize;
    }
   
    spin_unlock_irqrestore(&mt_host_controller->lock, flags);
    
    return;
}

static int mt_hcd_probe(struct device *dev){

    struct usb_hcd *hcd = NULL;
    int retval = 0;
    u32 tmp;
    u16 tmp16;

    printk("MT HCD PROBE\n");

    if(!(mt_host_controller = kmalloc(sizeof(struct mt_hc), GFP_KERNEL)))
        return -ENOMEM;

    memset(mt_host_controller, 0, sizeof(struct mt_hc));
    
    hcd = usb_create_hcd(&mt_hc_driver, dev, "MTK HCD");
	if (hcd == NULL)
		return -ENOMEM;

	spin_lock_init(&mt_host_controller->lock);
    init_timer(&mt_host_controller->timer);
	mt_host_controller->hcd = hcd;
	mt_host_controller->fifoAddr = 0;
	mt_host_controller->virtualHubPortStatus = 0;
	mt_host_controller->ep0_state = EP0_IDLE;
	mt_host_controller->bRootSpeed = USB_ROOT_SPEED_FULL;

	retval = usb_add_hcd(hcd, MT6516_USB_IRQ_LINE, IRQF_SHARED);
	if (retval)
		goto error;
	
    /* host controller hardware initialization */
    mt_hcd_hw_init();
	/* initialize endpoints */
	mt_hcd_ep_init();

	/* set USB irq sensitivity to level triggered */
    tmp = __raw_readl(CIRQ_BASE + 0x60);
	tmp |= 0x00000300;
	__raw_writel(tmp, CIRQ_BASE + 0x60);

    /* Set GPIO32 direction */
	tmp16 = __raw_readw(0xf0002020);
	tmp16 |= 0x1;
	__raw_writew(tmp16, 0xf0002020);
#if 0
    /* Enable GPIO32 pull-up/pull-down function */
    tmp16 = __raw_readw(0xf0002120);
    tmp16 |= 0x1;
    __raw_writew(tmp16, 0xf0002120);

    /* Pull-up for GPIO32 */
    tmp16 = __raw_readw(0xf0002220);
    tmp16 |= 0x1;
    __raw_writew(tmp16, 0xf0002220);
#endif
    return 0;

error:

    mt_hcd_hw_deinit();
    return retval;
}

static int mt_hcd_remove(struct device *dev){
    
    DBG("MTK HCD REMOVE\n");
    
    return 0;
}

static int mt_hcd_suspend(struct device *dev, pm_message_t state){

    DBG("MTK HCD SUSPEND\n");
    
    return 0;
}

static int mt_hcd_resume(struct device *dev){

    DBG("MTK1 HCD RESUME\n");

    return 0;
}

static struct device_driver mt_hcd_driver = {
	.name		= "mt-hcd",
	.bus		= &platform_bus_type,
	.probe		= mt_hcd_probe,
	.remove		= mt_hcd_remove,
	.suspend	= mt_hcd_suspend, 
	.resume		= mt_hcd_resume, 
};

static int __init mt_hcd_init (void)
{
	return driver_register(&mt_hcd_driver);
}

static void __exit mt_hcd_exit (void)
{
	driver_unregister(&mt_hcd_driver);
}

module_init (mt_hcd_init);
module_exit (mt_hcd_exit);
