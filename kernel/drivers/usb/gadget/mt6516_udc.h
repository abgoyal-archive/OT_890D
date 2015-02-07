
#ifndef MT6516_USBD_H
#define MT6516_USBD_H

#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/usb/ch9.h>
#include <asm/byteorder.h>
#include <mach/mt6516_reg_base.h>
#include <linux/usb/gadget.h>
#include <mach/mt6516_smart_battery.h>

/* ============= */
/* hardware spec */
/* ============= */

#define MT_EP_NUM 6
#define MT_CHAN_NUM 8
#define MT_EP0_FIFOSIZE 64

#define FIFO_START_ADDR  512

#define MT_BULK_MAXP 512
#define MT_INT_MAXP  1024

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

#define PWR_ISO_UPDATE       (1<<7)
#define PWR_SOFT_CONN        (1<<6)
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
#define INTRUSB_RESET      (1<<2)
#define INTRUSB_RESUME     (1<<1)
#define INTRUSB_SUSPEND    (1<<0)

/* ========== */
/* Test Modes */
/* ========== */

#define USB_TST_FORCE_HOST     (1<<7)
#define USB_TST_FIFO_ACCESS    (1<<6)
#define USB_TST_FORCE_FS       (1<<5)
#define USB_TST_FORCE_HS       (1<<4)
#define USB_TST_TEST_PACKET    (1<<3)
#define USB_TST_TEST_K         (1<<2)
#define USB_TST_TEST_J         (1<<1)
#define USB_TST_SE0_NAK        (1<<0)

/* ===================== */
/* DMA control registers */
/* ===================== */

#define DMA_INTR (USB_BASE + 0x0200)

#define USB_DMA_CNTL(chan)  (USB_BASE + 0x0204 + 0x10*(chan-1))
#define USB_DMA_ADDR(chan)  (USB_BASE + 0x0208 + 0x10*(chan-1))
#define USB_DMA_COUNT(chan) (USB_BASE + 0x020c + 0x10*(chan-1))
#define USB_DMA_REALCOUNT(chan) (USB_BASE + 0x0400 + 0x10*(chan-1))

/* ================================= */
/* Endpoint Control/Status Registers */
/* ================================= */

#define IECSR (USB_BASE + 0x0010)
/* for EP0 */
#define CSR0         0x2  /* EP0 Control Status Register */
                          /* For Host Mode, it would be 0x2 */
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
#define FIFOSIZE     0xf  /* configured FIFO size register */

/* ============================== */
/* control status register fields */
/* ============================== */

/* CSR0_DEV */
#define EP0_FLUSH_FIFO           (1<<8)
#define EP0_SERVICE_SETUP_END    (1<<7)
#define EP0_SERVICED_RXPKTRDY    (1<<6)
#define EP0_SENDSTALL            (1<<5)
#define EP0_SETUPEND             (1<<4)
#define EP0_DATAEND              (1<<3)
#define EP0_SENTSTALL            (1<<2)
#define EP0_TXPKTRDY             (1<<1)
#define EP0_RXPKTRDY             (1<<0)

/* TXCSR_DEV */
#define EPX_TX_AUTOSET           (1<<15)
#define EPX_TX_ISO               (1<<14)
#define EPX_TX_MODE              (1<<13)
#define EPX_TX_DMAREQEN          (1<<12)
#define EPX_TX_FRCDATATOG        (1<<11)
#define EPX_TX_DMAREQMODE        (1<<10)
#define EPX_TX_AUTOSETEN_SPKT    (1<<9)
#define EPX_TX_INCOMPTX          (1<<7)
#define EPX_TX_CLRDATATOG        (1<<6)
#define EPX_TX_SENTSTALL         (1<<5)
#define EPX_TX_SENDSTALL         (1<<4)
#define EPX_TX_FLUSHFIFO         (1<<3)
#define EPX_TX_UNDERRUN          (1<<2)
#define EPX_TX_FIFONOTEMPTY      (1<<1)
#define EPX_TX_TXPKTRDY          (1<<0)

#define EPX_TX_WZC_BITS  (EPX_TX_INCOMPTX | EPX_TX_SENTSTALL | EPX_TX_UNDERRUN \
| EPX_TX_FIFONOTEMPTY)

/* RXCSR_DEV */
#define EPX_RX_AUTOCLEAR         (1<<15)
#define EPX_RX_ISO               (1<<14)
#define EPX_RX_DMAREQEN          (1<<13)
#define EPX_RX_DISNYET           (1<<12)
#define EPX_RX_PIDERR            (1<<12)
#define EPX_RX_DMAREQMODE        (1<<11)
#define EPX_RX_AUTOCLRENSPKT     (1<<10)
#define EPX_RX_INCOMPRXINTREN    (1<<9)
#define EPX_RX_INCOMPRX          (1<<8)
#define EPX_RX_CLRDATATOG        (1<<7)
#define EPX_RX_SENTSTALL         (1<<6)
#define EPX_RX_SENDSTALL         (1<<5)
#define EPX_RX_FLUSHFIFO         (1<<4)
#define EPX_RX_DATAERR           (1<<3)
#define EPX_RX_OVERRUN           (1<<2)
#define EPX_RX_FIFOFULL          (1<<1)
#define EPX_RX_RXPKTRDY          (1<<0)

#define EPX_RX_WZC_BITS  (EPX_RX_SENTSTALL | EPX_RX_OVERRUN | EPX_RX_RXPKTRDY)

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
#define EPINFO       (USB_BASE + 0x0078)  /* TX and RX Information Register */
#define RAM_DMAINFO  (USB_BASE + 0x0079)  /* RAM and DMA Information Register */
#define LINKINFO     (USB_BASE + 0x007a)  /* Delay Time Information Register */
#define VPLEN        (USB_BASE + 0x007b)  /* VBUS Pulse Charge Time Register */
#define HSEOF1       (USB_BASE + 0x007c)  /* High Speed EOF1 Register */
#define FSEOF1       (USB_BASE + 0x007d)  /* Full Speed EOF1 Register */
#define LSEOF1       (USB_BASE + 0x007e)  /* Low Speed EOF1 Register */
#define RSTINFO      (USB_BASE + 0x007f)  /* Reset Information Register */

/* ========================================================== */
/* FIFO size register fields and available packet size values */
/* ========================================================== */
#define DPB        0x10
#define PKTSZ      0x0f

#define PKTSZ_8    (1<<3)
#define PKTSZ_16   (1<<4)
#define PKTSZ_32   (1<<5)
#define PKTSZ_64   (1<<6)
#define PKTSZ_128  (1<<7)
#define PKTSZ_256  (1<<8)
#define PKTSZ_512  (1<<9)
#define PKTSZ_1024 (1<<10)

#define FIFOSZ_8      (0x0)
#define FIFOSZ_16     (0x1)
#define FIFOSZ_32     (0x2)
#define FIFOSZ_64     (0x3)
#define FIFOSZ_128    (0x4)
#define FIFOSZ_256    (0x5)
#define FIFOSZ_512    (0x6)
#define FIFOSZ_1024   (0x7)
#define FIFOSZ_2048   (0x8)
#define FIFOSZ_4096   (0x9)
#define FIFOSZ_3072   (0xF)

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

/* ================== */
/* Byte Masks         */
/* ================== */

#define USB_BYTE0_MASK      (0x000000FF)
#define USB_BYTE1_MASK      (0x0000FF00)
#define USB_BYTE2_MASK      (0x00FF0000)
#define USB_BYTE3_MASK      (0xFF000000)

/* ============== */
/* USB PHY Fields */
/* ============== */

/* USB_PHY_CON1 */
#define USB_PHY_CON1_PLL_EN          (1<<7)
#define USB_PHY_CON1_PLL_VCOG_MASK   (0xc0000)
#define USB_PHY_CON1_PLL_VCOG0       (0x40000)
#define USB_PHY_CON1_PLL_VCOG1       (0x80000)
#define USB_PHY_CON1_PLL_CCP_MASK    (0xf000)
#define USB_PHY_CON1_PLL_CCP_OFFSET  (12)

/* USB_PHY_CON2 */
#define USB_PHY_CON2_FORCE_DM_PULLDOWN  (1 << 30)
#define USB_PHY_CON2_FORCE_DP_PULLDOWN  (1 << 29)

/* USB_PHY_CON3 */
#define USB_PHY_CON3_TEST_CTRL0      (1 << 16)
#define USB_PHY_CON3_TEST_CTRL1      (1 << 17)
#define USB_PHY_CON3_TEST_CTRL2      (1 << 18)
#define USB_PHY_CON3_TEST_CTRL3      (1 << 19)
#define USB_PHY_CON3_TEST_CTRL_MASK  (0x000F0000)

/* USB_PHY_CON4 */
#define USB_PHY_CON4_BGR_BGR_EN      (0x100)
#define USB_PHY_CON4_FORCE_BGR_ON    (0x4F00)

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

#define USB_PHY_CON5_FORCE_DP_HIGH         (0x000B0000)
#define USB_PHY_CON5_XCVR_SELECT_MASK      (0x30000000)

/* USB_PHY_INTF1 */
#define USB_PHY_INTF1_LINESTATE            (0xc0000000)
#define USB_PHY_INTF1_DM                   (1<<31)
#define USB_PHY_INTF1_DP                   (1<<30)
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

/* ============= */
/* DMA Registers */
/* ============= */

/* DMA_CNTL */
#define USB_DMA_CNTL_ENDMAMODE2             (1 << 13)
#define USB_DMA_CNTL_PP_RST                 (1 << 12)
#define USB_DMA_CNTL_PP_EN                  (1 << 11)
#define USB_DMA_CNTL_BURST_MODE_MASK        (0x3 << 9)
#define USB_DMA_CNTL_BURST_MODE_OFFSET      (9)
#define USB_DMA_CNTL_BURST_MODE0            (0x0 << 9)
#define USB_DMA_CNTL_BURST_MODE1            (0x1 << 9)
#define USB_DMA_CNTL_BURST_MODE2            (0x2 << 9)
#define USB_DMA_CNTL_BURST_MODE3            (0x3 << 9)
#define USB_DMA_CNTL_BUSERR                 (1 << 8)
#define USB_DMA_CNTL_ENDPOINT_MASK          (0xf << 4)
#define USB_DMA_CNTL_ENDPOINT_OFFSET        (4)
#define USB_DMA_CNTL_INTEN                  (1 << 3)
#define USB_DMA_CNTL_DMAMODE                (1 << 2)
#define USB_DMA_CNTL_DMAMODE_OFFSET         (2)
#define USB_DMA_CNTL_DMADIR                 (1 << 1)
#define USB_DMA_CNTL_DMADIR_OFFSET          (1)
#define USB_DMA_CNTL_DMAEN                  (1 << 0)

/* DMA_LIMITER */
#define USB_DMA_LIMITER_MASK                (0xff00)
#define USB_DMA_LIMITER_OFFSET              (8)

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
}USB_DIR;

typedef enum
{
    EP0_IDLE = 0,
    EP0_RX,
    EP0_TX,
    EP0_RX_STATUS,
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
#if 0
typedef enum
{
    CHARGER_UNKNOWN = 0,
    STANDARD_HOST,
    NONSTANDARD_CHARGER,
    STANDARD_CHARGER,
}USB_CHARGER_TYPE;
#endif
/* =========== */
/* some macros */
/* =========== */

#define EP_IS_IN(EP)      ((EP)->desc->bEndpointAddress & USB_DIR_IN)
#define EP_INDEX(EP)      ((EP)->desc->bEndpointAddress & 0xF)
#define EP_MAXPACKET(EP)  ((EP)->ep.maxpacket)

#define EPMASK(x) (1<<x)
#define CHANMASK(x) (1<<(x-1))

/* ========== */
/* structures */
/* ========== */


struct mt_ep
{
    unsigned int ep_num;
    struct usb_ep ep;
    struct list_head queue;
    const struct usb_endpoint_descriptor *desc;
    unsigned int busycompleting;
};

struct mt_req
{
    struct usb_request req;
    struct list_head queue;
    int queued;
};

struct mt_udc
{
    u8                         faddr;
    struct usb_gadget          gadget;
    struct usb_gadget_driver   *driver;
    spinlock_t                 lock;
    EP0_STATE                  ep0_state;
    struct mt_ep               ep[MT_EP_NUM];
    u16                        fifo_addr;
    unsigned long flags;
    u8 set_address;
    u8 test_mode;
    u8 test_mode_nr;
    u8                         power;
    u8                         ready;
    CHARGER_TYPE               charger_type;
    struct mt_req              *udc_req;
};

CHARGER_TYPE mt6516_usb_charger_type_detection(void);
void mt6516_usb_connect(void);
void mt6516_usb_disconnect(void);
int mt6516_usb_is_charger_in(void);
kal_bool is_mp_ic(void);

#endif //MT6516_USBD_H

