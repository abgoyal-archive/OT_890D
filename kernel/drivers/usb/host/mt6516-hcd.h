
#include <linux/usb/ch9.h>
#include <asm/arch/mt6516_reg_base.h>
#include <asm/byteorder.h>

#define CONFIG_DEBUG_MSG

#ifdef CONFIG_DEBUG_MSG
#define DBG(fmt,args...) printk(fmt, ## args)
#define ASSERT(x)   if (!(x)) BUG()
#else
#define DBG(fmt,args...)	do {}  while (0)
#define ASSERT(x)
#endif

/* ============= */
/* hardware spec */
/* ============= */

#define MT_EP_NUM 3
#define MT_CHAN_NUM 4
#define MT_EP0_FIFOSIZE 64

#define FIFO_START_ADDR  512

#define MT_BULK_MAXP 512
#define MT_INT_MAXP  1024

#define MT_EP_TX_START 1
#define MT_EP_RX_START 2

/* connect debounce is 100ms */
#define CHECK_INSERT_DEBOUNCE 100

/* max of nak limit timeouts */
#define USB_MAX_RETRIES 8

/* =================== */
/* USB common register */
/* =================== */

#define FADDR    (USB_BASE + 0x0000)  /* Function Address Register */
#define POWER    (USB_BASE + 0x0001)  /* Power Management Register */
#define INTRTX   (USB_BASE + 0x0002)  /* TX Interrupt Status Register */
#define INTRRX   (USB_BASE + 0X0004)  /* RX Interrupt Status Register */
#define INTRTXE  (USB_BASE + 0x0006)  /* TX Interrupt Status Enable Register */
#define INTRRXE  (USB_BASE + 0x0008)  /* RX Interrupt Status Enable Register */
#define INTRUSB  (USB_BASE + 0x000a)  /* Common USB Interrupt Register */
#define INTRUSBE (USB_BASE + 0x000b)  /* Common USB Interrupt Enable Register */
#define FRAME    (USB_BASE + 0x000c)  /* Frame Number Register */
#define INDEX    (USB_BASE + 0x000e)  /* Endpoint Selecting Index Register */
#define TESTMODE (USB_BASE + 0x000f)  /* Test Mode Enable Register */

/* ============ */
/* POWER fields */
/* ============ */

#define PWR_HS_ENAB          (1<<5)
#define PWR_HS_MODE          (1<<4)
#define PWR_RESET            (1<<3)
#define PWR_RESUME           (1<<2)
#define PWR_SUSPEND_MODE     (1<<1)
#define PWR_ENABLE_SUSPENDM  (1<<0)

/* ============== */
/* INTRUSB fields */
/* ============== */

#define INTRUSB_VBUS_ERROR (1<<7)
#define INTRUSB_SESS_REQ   (1<<6)
#define INTRUSB_DISCON     (1<<5)
#define INTRUSB_CONN       (1<<4)
#define INTRUSB_SOF        (1<<3)
#define INTRUSB_BABBLE     (1<<2)
#define INTRUSB_RESET      (1<<2)
#define INTRUSB_RESUME     (1<<1)
#define INTRUSB_SUSPEND    (1<<0)

/* ===================== */
/* DMA control registers */
/* ===================== */

#define DMA_INTR (USB_BASE + 0x0200)

#define USB_DMA_CTNL(chan)  (USB_BASE + 0x0204 + 0x10*(chan-1))
#define USB_DMA_ADDR(chan)  (USB_BASE + 0x0208 + 0x10*(chan-1))
#define USB_DMA_COUNT(chan) (USB_BASE + 0x020c + 0x10*(chan-1))

/* ================================= */
/* Endpoint Control/Status Registers */
/* ================================= */

#define IECSR (USB_BASE + 0x0010)
/* for EP0 */
#define CSR0         0x2  /* EP0 Control Status Register */
#define COUNT0       0x8  /* EP0 Received Bytes Register */
#define NAKLIMIT0    0xB  /* NAK Limit Register */
#define CONFIGDATA   0xF  /* Core Configuration Register */
/* for other endpoints */
#define TXMAP        0x0  /* TXMAP Register: Max Packet Size for TX */
#define TXCSR        0x2  /* TXCSR Register: TX Control Status Register */
#define RXMAP        0x4  /* RXMAP Register: Max Packet Size for RX */
#define RXCSR        0x6  /* RXCSR Register: RX Control Status Register */
#define RXCOUNT      0x8  /* RXCOUNT Register */
#define TXTYPE       0xa  /* TX Type Register */
#define TXINTERVAL   0xb  /* TX Interval Register */
#define RXTYPE       0xc  /* RX Type Register */
#define RXINTERVAL   0xd  /* RX Interval Register */

/* ============================== */
/* control status register fields */
/* ============================== */

/* CSR0_HST */
#define EP0_DISPING              (1<<11)
#define EP0_FLUSH_FIFO           (1<<8)
#define EP0_NAK_TIMEOUT          (1<<7)
#define EP0_STATUSPKT            (1<<6)
#define EP0_REQ_PKT              (1<<5)
#define EP0_ERROR                (1<<4)
#define EP0_SETUPPKT             (1<<3)
#define EP0_RXSTALL              (1<<2)
#define EP0_TXPKTRDY             (1<<1)
#define EP0_RXPKTRDY             (1<<0)

/* TXCSR_HST */
#define EPX_TX_AUTOSET           (1<<15)
#define EPX_TX_MODE              (1<<13)
#define EPX_TX_DMAREQEN          (1<<12)
#define EPX_TX_FRCDATATOG        (1<<11)
#define EPX_TX_DMAREQMODE        (1<<10)
#define EPX_TX_INCOMPTX          (1<<7)
#define EPX_TX_NAK_TIMEOUT       (1<<7)
#define EPX_TX_CLRDATATOG        (1<<6)
#define EPX_TX_RXSTALL           (1<<5)
#define EPX_TX_FLUSHFIFO         (1<<3)
#define EPX_TX_ERROR             (1<<2)
#define EPX_TX_FIFONOTEMPTY      (1<<1)
#define EPX_TX_TXPKTRDY          (1<<0)

/* RXCSR_HST */
#define EPX_RX_AUTOCLEAR         (1<<15)
#define EPX_RX_AUTOREQ           (1<<14)
#define EPX_RX_DMAREQEN          (1<<13)
#define EPX_RX_PIDERR            (1<<12)
#define EPX_RX_DMAREQMODE        (1<<11)
#define EPX_RX_INCOMPRX          (1<<8)
#define EPX_RX_CLRDATATOG        (1<<7)
#define EPX_RX_RXSTALL           (1<<6)
#define EPX_RX_REQPKT            (1<<5)
#define EPX_RX_FLUSHFIFO         (1<<4)
#define EPX_RX_DATAERR           (1<<3)
#define EPX_RX_NAKTIMEOUT        (1<<3)
#define EPX_RX_ERROR             (1<<2)
#define EPX_RX_FIFOFULL          (1<<1)
#define EPX_RX_RXPKTRDY          (1<<0)

/* for EPX_TX_MODE */
#define MODE_TX 1
#define MODE_RX 0

/* ================= */
/* CONFIGDATA fields */
/* ================= */

#define MP_RXE         (1<<7)
#define MP_TXE         (1<<6)
#define BIGENDIAN      (1<<5)
#define HBRXE          (1<<4)
#define HBTXE          (1<<3)
#define DYNFIFOSIZING  (1<<2)
#define SOFTCONE       (1<<1)
#define UTMIDATAWIDTH  (1<<0)

/* ============= */
/* FIFO register */
/* ============= */

/* for endpint 1 ~ 4, writing to these addresses = writing to the */
/* corresponding TX FIFO, reading from these addresses = reading from */ 
/* corresponding RX FIFO */

#define FIFO(ep_num)     (USB_BASE + 0x0020 + ep_num*0x0004)


/* ============================ */
/* additional control registers */
/* ============================ */

#define DEVCTL       (USB_BASE + 0x0060)  /* OTG Device Control Register */
#define PWRUPCNT     (USB_BASE + 0x0061)  /* Power Up Counter Register */
#define TXFIFOSZ     (USB_BASE + 0x0062)  /* TX FIFO Size Register */
#define RXFIFOSZ     (USB_BASE + 0x0063)  /* RX FIFO Size Register */
#define TXFIFOADD    (USB_BASE + 0x0064)  /* TX FIFO Address Register */
#define RXFIFOADD    (USB_BASE + 0x0066)  /* RX FIFO Address Register */
#define HWVERS       (USB_BASE + 0x006c)  /* H/W Version Register */
#define SWRST        (USB_BASE + 0x0070)  /* Software Reset Register */
//#define UTMI_FSMDBG  (USB_BASE + 0x0071)  /* UTMI state machine information */
#define EPINFO       (USB_BASE + 0x0078)  /* TX and RX Information Register */
#define RAM_DMAINFO  (USB_BASE + 0x0079)  /* RAM and DMA Information Register */
#define LINKINFO     (USB_BASE + 0x007a)  /* Delay Time Information Register */
#define VPLEN        (USB_BASE + 0x007b)  /* VBUS Pulse Charge Time Register */
#define HSEOF1       (USB_BASE + 0x007c)  /* High Speed EOF1 Register */
#define FSEOF1       (USB_BASE + 0x007d)  /* Full Speed EOF1 Register */
#define LSEOF1       (USB_BASE + 0x007e)  /* Low Speed EOF1 Register */
#define RSTINFO      (USB_BASE + 0x007f)  /* Reset Information Register */
#define SW_PATCH     (USB_BASE + 0x0080)  /* SW Patch Register */

/* ========================================================== */
/* FIFO size register fields and available packet size values */
/* ========================================================== */
#define DPB        0x10
#define PKTSZ      0x0f

#define PKTSZ_8    0x00
#define PKTSZ_16   0x01
#define PKTSZ_32   0x02
#define PKTSZ_64   0x03
#define PKTSZ_128  0x04
#define PKTSZ_256  0x05
#define PKTSZ_512  0x06
#define PKTSZ_1024 0x07
#define PKTSZ_2048 0x08
#define PKTSZ_4096 0x09
#define PKTSZ_3072 0x0f

/* ============= */
/* DEVCTL fields */
/* ============= */
#define DEVCTL_B_DEVICE       (1<<7)
#define DEVCTL_FS_DEV         (1<<6)
#define DEVCTL_LS_DEV         (1<<5)
#define DEVCTL_VBUS           (1<<3)
#define DEVCTL_VBUS_MASK       0x18
#define DEVCTL_HOST_MODE      (1<<2)
#define DEVCTL_HOST_REQ       (1<<1)
#define DEVCTL_SESSION        (1<<0)

/* ============ */
/* SWRST fields */
/* ============ */

#define SWRST_PHY_RST         (1<<7)
#define SWRST_PHYSIG_GATE_HS  (1<<6)
#define SWRST_PHYSIG_GATE_EN  (1<<5)
#define SWRST_REDUCE_DLY      (1<<4)
#define SWRST_UNDO_SRPFIX     (1<<3)
#define SWRST_FRC_VBUSVALID   (1<<2)
#define SWRST_SWRST           (1<<1)
#define SWRST_DISUSBRESET     (1<<0)

/* =============== */
/* SW_PATCH fields */
/* =============== */
#define SW_PATCH_TM1                     (1<<6)
#define SW_PATCH_EN_PATCH_NOISESTILLSOF  (1<<5)
#define SW_PATCH_EN_PATCH_FLUSHFIFO      (1<<4)
#define SW_PATCH_EN_PATCH_CHGR_VALUE     (1<<1)
#define SW_PATCH_EN_PATCH_BABBLECLRSESS  (1<<0)

/* ================= */
/* USB PHY Registers */
/* ================= */

#define USB_PHY_CON1    (USB_BASE + 0x600)
#define USB_PHY_CON2    (USB_BASE + 0x604)
#define USB_PHY_CON3    (USB_BASE + 0x608)
#define USB_PHY_CON4    (USB_BASE + 0x60c)
#define USB_PHY_CON5    (USB_BASE + 0x610)
#define USB_PHY_INTF1   (USB_BASE + 0x614)
#define USB_PHY_INTF2   (USB_BASE + 0x618)
#define USB_PHY_INTF3   (USB_BASE + 0x61c)

/* ============== */
/* USB PHY Fields */
/* ============== */

/* ============== */
/* USB PHY Fields */
/* ============== */

/* USB_PHY_CON1 */
#define USB_PHY_CON1_PLL_EN          (1<<7)
#define USB_PHY_CON1_PLL_VCOG_MASK   (0xc0000)
#define USB_PHY_CON1_PLL_VCOG0       (0x40000)
#define USB_PHY_CON1_PLL_VCOG1       (0x80000)

/* USB_PHY_CON4 */
#define USB_PHY_CON4_BGR_BGR_EN      (0x100)
//#define USB_PHY_CON4_CHGER_AVAIL     (0x10000000)
//#define USB_PHY_CON4_LINE_STATE      (0x20000000)

/* USB_PHY_CON5 */
#define USB_PHY_CON5_CDR_FILT              (0x0000000f)
#define USB_PHY_CON5_CLK_DIV_CNT           (0x00000070)
#define USB_PHY_CON5_VBUS_CMP_EN           (1 << 7)
#define USB_PHY_CON5_PROBE_SEL             (0x0000ff00)
#define USB_PHY_CON5_FORCE_OP_MODE         (1 << 16)
#define USB_PHY_CON5_FORCE_TERM_SELECT     (1 << 17)
#define USB_PHY_CON5_FORCE_SUSPENDM        (1 << 18)
#define USB_PHY_CON5_XCVR_SELECT           (1 << 19)
#define USB_PHY_CON5_USB_MODE_0            (1 << 20)
#define USB_PHY_CON5_USB_MODE_1            (1 << 21)
#define USB_PHY_CON5_UTMI_MUXSEL           (1 << 22)
#define USB_PHY_CON5_FORCE_IDPULLUP        (1 << 23)
#define USB_PHY_CON5_OP_MODE_0             (1 << 24)
#define USB_PHY_CON5_OP_MODE_1             (1 << 25)
#define USB_PHY_CON5_TERM_SELECT           (1 << 26)
#define USB_PHY_CON5_SUSPENDM              (1 << 27)
#define USB_PHY_CON5_XCVR_SELECT_0         (1 << 28)
#define USB_PHY_CON5_XCVR_SELECT_1         (1 << 29)
#define USB_PHY_CON5_DP_PULL_DOWN          (1 << 30)
#define USB_PHY_CON5_DM_PULL_DOWN          (1 << 31)

/* USB_PHY_INTF1 */
#define USB_PHY_INTF1_LINESTATE            (0xc0000000)
#define USB_PHY_INTF1_HOSTDISCON           (1<<29)
#define USB_PHY_INTF1_TXREADY              (1<<28)
#define USB_PHY_INTF1_RXERROR              (1<<27)
#define USB_PHY_INTF1_RXACTIVE             (1<<26)
#define USB_PHY_INTF1_RXVALIDH             (1<<25)
#define USB_PHY_INTF1_RXVALID              (1<<24)
#define USB_PHY_INTF1_XDATA_OUT            (0x00ffff00)
#define USB_PHY_INTF1_XDATA_IN             (0x000000f0)
#define USB_PHY_INTF1_TX_VALIDH            (1<<3)
#define USB_PHY_INTF1_TX_VALID             (1<<2)
#define USB_PHY_INTF1_DRVVBUS              (1<<1)
#define USB_PHY_INTF1_IDPULLUP             (1<<0)

/* USB_PHY_INTF2 */
#define USB_PHY_INTF2_FORCE_AUX_EN         (1<<31)
#define USB_PHY_INTF2_FORCE_OTG_PROBE      (1<<30)
#define USB_PHY_INTF2_FORCE_USB_CLKON      (1<<29)
#define USB_PHY_INTF2_FORCE_BVALID         (1<<28)
#define USB_PHY_INTF2_FORCE_IDDIG          (1<<27)
#define USB_PHY_INTF2_FORCE_VBUS_VALID     (1<<26)
#define USB_PHY_INTF2_FORCE_SESSEND        (1<<25)
#define USB_PHY_INTF2_FORCE_AVALID         (1<<24)
#define USB_PHY_INTF2_IDDIG                (1<<3)
#define USB_PHY_INTF2_VBUSVALID            (1<<2)
#define USB_PHY_INTF2_SESSEND              (1<<1)
#define USB_PHY_INTF2_AVALID               (1<<0)

/* USB_PHY_INTF3 */
#define USB_PHY_INTF3_AUX_EN               (1<<7)
#define USB_PHY_INTF3_OTG_PROBE            (1<<6)
#define USB_PHY_INTF3_USB_CLKON            (1<<5)
#define USB_PHY_INTF3_BVALID_W             (1<<4)
#define USB_PHY_INTF3_IDDIG_W              (1<<3)
#define USB_PHY_INTF3_VBUSVALID_W          (1<<2)
#define USB_PHY_INTF3_SESSEND_W            (1<<1)
#define USB_PHY_INTF3_AVALID_W             (1<<0)

/* =============== */
/* Protocol fields */
/* =============== */

#define PROTOCOL_MASK        (0x30)
#define EP_NUM_MASK          (0x0f)

#define PROTOCOL_ILLEGAL     (0x00)
#define PROTOCOL_ISOCHRONOUS (0x10)
#define PROTOCOL_BULK        (0x20)
#define PROTOCOL_INTERRUPT   (0x30)

/* ========== */
/* USB STATUS */
/* ========== */

#define USB_ST_NOERROR          (0)
#define USB_ST_CRC              (-EILSEQ)
#define USB_ST_BITSTUFF         (-EPROTO)
#define USB_ST_NORESPONSE       (-ETIMEDOUT)    /* device not responding/handshaking */
#define USB_ST_DATAOVERRUN      (-EOVERFLOW)
#define USB_ST_DATAUNDERRUN     (-EREMOTEIO)
#define USB_ST_BUFFEROVERRUN    (-ECOMM)
#define USB_ST_BUFFERUNDERRUN   (-ENOSR)
#define USB_ST_INTERNALERROR    (-EPROTO)       /* unknown error */
#define USB_ST_SHORT_PACKET     (-EREMOTEIO)
#define USB_ST_PARTIAL_ERROR    (-EXDEV)        /* ISO transfer only partially completed */
#define USB_ST_URB_KILLED       (-ENOENT)       /* URB canceled by user */
#define USB_ST_URB_PENDING      (-EINPROGRESS)
#define USB_ST_REMOVED          (-ENODEV)       /* device not existing or removed */
#define USB_ST_TIMEOUT          (-ETIMEDOUT)    /* communication timed out, also in urb->status**/
#define USB_ST_NOTSUPPORTED     (-ENOSYS)
#define USB_ST_BANDWIDTH_ERROR  (-ENOSPC)       /* too much bandwidth used */
#define USB_ST_URB_INVALID_ERROR  (-EINVAL)     /* invalid value/transfer type */
#define USB_ST_URB_REQUEST_ERROR  (-ENXIO)      /* invalid endpoint */
#define USB_ST_STALL            (-EPIPE)        /* pipe stalled, also in urb->status*/

/* ======================================================= */
/* the mask contains all supported virutal hub port status */
/* ======================================================= */

#define MTK_PORT_C_MASK               \
((1 << USB_PORT_FEAT_C_CONNECTION)    \
| (1 << USB_PORT_FEAT_C_ENABLE)       \
| (1 << USB_PORT_FEAT_C_SUSPEND)      \
| (1 << USB_PORT_FEAT_C_OVER_CURRENT) \
| (1 << USB_PORT_FEAT_C_RESET))

/* ======= */
/* typedef */
/* ======= */

typedef enum
{
    USB_FALSE = 0,
    USB_TRUE,
}USB_BOOL;

typedef enum
{
    USB_RX = 0,
    USB_TX,
    USB_CTRL,
}USB_TYPE;

typedef enum
{
    EP0_IDLE = 0,
    EP0_RX,
    EP0_TX,
    EP0_STATUS,
}EP0_STATE;

typedef enum
{
    EP0_STATE_READ_END = 0,
    EP0_STATE_WRITE_RDY,
    EP0_STATE_TRANSACTION_END,
    EP0_STATE_CLEAR_SENT_STALL,
}EP0_DRV_STATE;

typedef enum
{
    DEVSTATE_DEFAULT = 0,
    DEVSTATE_SET_ADDRESS,
    DEVSTATE_ADDRESS,
    DEVSTATE_CONFIG,
}DEVICE_STATE;

/* =========== */
/* some macros */
/* =========== */

#define EP_IS_IN(EP)      ((EP)->bEndpointAddress & USB_DIR_IN)
#define EP_INDEX(EP)      ((EP)->bEndpointAddress & 0xF)
#define EP_MAXPACKET(EP)  ((EP)->ep.maxpacket)

#define EPMASK(x) (1<<x)
#define CHANMASK(x) (1<<x)

#define USB_ISO_ASAP       0x0002
#define USB_ASYNC_UNLINK   0x0008

#define USB_ZERO_PACKET         0x0040

#define USB_INT		    0
#define USB_BULK 	    1
#define USB_ISOCHRONOUS 2
#define USB_CONTROL     3

#define USB_ROOT_SPEED_LOW  0
#define USB_ROOT_SPEED_FULL 1
#define USB_ROOT_SPEED_HIGH 2


typedef struct __attribute__((packed)) {
    u8  bmRequestType;
    u8  bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} mt_DeviceRequest;

struct mt_urb_list{
    struct list_head urb_list;
    struct urb *pUrb;
};

struct mt_ep{
    u8 ep_num;
    spinlock_t lock;        /* spin lock */ 
    struct usb_device *dev; /* the device which this endpoint services */
    USB_TYPE type;
    u8 traffic;
    u16 wPacketSize;     /* maxpacket size */
    u32 fifoAddr;
    struct list_head urb_list;
    struct urb *pCurrentUrb;
    u8 remoteAddress;
    u8 remoteEnd;
    u8 retries;             /* number of NAK LIMIT TIMEOUTS before giving up */
    u8 busycompleting;
    u32 dwOffset;
    u32 dwRequestSize;
    u32 dwIsoPacket;
    u32 dwWaitFrame;
    unsigned long flags;
};

struct mt_hc{
    spinlock_t lock;
    struct timer_list timer;
    struct mt_ep ep[MT_EP_NUM];
    struct usb_hcd *hcd; 
    u32 fifoAddr;
    u32 virtualHubPortStatus;
    EP0_STATE ep0_state;
    u8 bRootSpeed;
};

