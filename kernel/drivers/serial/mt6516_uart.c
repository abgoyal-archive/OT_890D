

//#define DEBUG
//#define VFIFO_DEBUG
#define ENABLE_DMA_VFIFO
#define POWER_FEATURE   //control power-on/power-off
#define ENABLE_VFIFO_LEVEL_SENSITIVE
#include <linux/autoconf.h>
#include <linux/platform_device.h>

#if defined(CONFIG_SERIAL_MT6516_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>


#include <asm/io.h>
#include <asm/irq.h>
#include <asm/scatterlist.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/board.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#include <mach/irqs.h>
#include <asm/tcm.h>
#include "mt6516_uart.h"

#define DBG_TAG                     "[UART] "
#define CFG_UART_AUTOBAUD           0

#define UART_VFIFO_SIZE             8192 //JK, 2048 -> 8192
#define UART_VFIFO_ALERT_LEN        0x3f

#define UART_MAX_TX_PENDING         1024

#define UART_MAJOR                  204
#define UART_MINOR                  209
#define UART_NR                     CFG_UART_PORTS

#define MT6516_SYSCLK_65            65000000
#define MT6516_SYSCLK_58_5          58500000
#define MT6516_SYSCLK_52            52000000
#define MT6516_SYSCLK_26            26000000
#define MT6516_SYSCLK_13            13000000

#if defined(CONFIG_MT6516_CPU_416MHZ_MCU_104MHZ) || \
    defined(CONFIG_MT6516_CPU_208MHZ_MCU_104MHZ)
#define UART_SYSCLK                 MT6516_SYSCLK_52
#elif defined(CONFIG_MT6516_CPU_468MHZ_MCU_117MHZ)
#define UART_SYSCLK                 MT6516_SYSCLK_58_5
#else
#define UART_SYSCLK                 MT6516_SYSCLK_52
#endif
#if (UART_SYSCLK != MT6516_SYSCLK_52)
#error "wrong colok setting"
#endif 

#define DRV_NAME            "mt6516-uart"

#ifdef DEBUG
/* Debug message event */
#define DBG_EVT_NONE        0x00000000    /* No event */
#define DBG_EVT_DMA         0x00000001    /* DMA related event */
#define DBG_EVT_INT         0x00000002    /* UART INT event */
#define DBG_EVT_CFG         0x00000004    /* UART CFG event */
#define DBG_EVT_FUC         0x00000008    /* Function event */
#define DBG_EVT_INFO        0x00000010    /* information event */
#define DBG_EVT_ERR         0x00000020  /* Error event */
#define DBG_EVT_ALL         0xffffffff

//#define DBG_EVT_MASK        (DBG_EVT_FUC|DBG_EVT_INT|DBG_EVT_DMA)
//#define DBG_EVT_MASK        (DBG_EVT_FUC|DBG_EVT_DMA|DBG_EVT_INFO)
//#define DBG_EVT_MASK        (DBG_EVT_CFG|DBG_EVT_DMA|DBG_EVT_INFO)
//#define DBG_EVT_MASK        (DBG_EVT_INT|DBG_EVT_DMA)
//#define DBG_EVT_MASK        (DBG_EVT_DMA|DBG_EVT_INFO|DBG_EVT_ERR|DBG_EVT_FUC)
//#define DBG_EVT_MASK        (DBG_EVT_NONE)
#define DBG_EVT_MASK          (DBG_EVT_INFO|DBG_EVT_DMA|DBG_EVT_CFG|DBG_EVT_FUC)
//#define DBG_EVT_MASK        (DBG_EVT_ALL & ~DBG_EVT_INT)
//#define DBG_EVT_MASK        (DBG_EVT_ALL)

static unsigned long mt6516_uart_evt_mask[] = {
    DBG_EVT_ALL,
    DBG_EVT_ALL,
    DBG_EVT_ALL,
    DBG_EVT_NONE
};

static int mt6516_uart_sysrq = 
  #if CONFIG_MAGIC_SYSRQ
    1
  #else
    0
  #endif 
    ;

#define MSG(evt, fmt, args...) \
do {    \
    if ((DBG_EVT_##evt) & mt6516_uart_evt_mask[uart->nport]) { \
        const char *s = #evt;                                  \
        if (DBG_EVT_##evt & DBG_EVT_ERR) \
            printk("    [UART%d]:<%c> in LINE %d(%s) -> " fmt , \
                   uart->nport, s[0], __LINE__, __FILE__, ##args); \
        else \
            printk("    [UART%d]:<%c> " fmt , uart->nport, s[0], ##args); \
    } \
} while(0)
#define MSG_FUNC_ENTRY(f)    MSG(FUC, "%s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)      do{}while(0)
#endif
#define MSG_FUNC_TRACE(f)   printk("    [UART] %s\n", __FUNCTION__)

// FIXME: in MT3351, only three uart ports allow hardware follow control; how about MT6516?
#define HW_FLOW_CTRL_PORT(uart)     (((struct mt6516_uart*)uart)->nport < UART_PORT3)   

#define UART_READ8(REG)             __raw_readb(REG)
#define UART_READ16(REG)            __raw_readw(REG)
#define UART_READ32(REG)            __raw_readl(REG)
#define UART_WRITE8(VAL, REG)       __raw_writeb(VAL, REG)
#define UART_WRITE16(VAL, REG)      __raw_writew(VAL, REG)
#define UART_WRITE32(VAL, REG)      __raw_writel(VAL, REG)

#define UART_SET_BITS(BS,REG)       ((*(volatile u32*)(REG)) |= (u32)(BS))
#define UART_CLR_BITS(BS,REG)       ((*(volatile u32*)(REG)) &= ~((u32)(BS)))

#define UART_RBR             (base+0x0)  /* Read only */
#define UART_THR             (base+0x0)  /* Write only */
#define UART_IER             (base+0x4)
#define UART_IIR             (base+0x8)  /* Read only */
#define UART_FCR             (base+0x8)  /* Write only */
#define UART_LCR             (base+0xc)
#define UART_MCR             (base+0x10)
#define UART_LSR             (base+0x14)
#define UART_MSR             (base+0x18)
#define UART_SCR             (base+0x1c)
#define UART_DLL             (base+0x0)  /* Only when LCR.DLAB = 1 */
#define UART_DLH             (base+0x4)  /* Only when LCR.DLAB = 1 */
#define UART_EFR             (base+0x8)  /* Only when LCR = 0xbf */
#define UART_XON1            (base+0x10) /* Only when LCR = 0xbf */
#define UART_XON2            (base+0x14) /* Only when LCR = 0xbf */
#define UART_XOFF1           (base+0x18) /* Only when LCR = 0xbf */
#define UART_XOFF2           (base+0x1c) /* Only when LCR = 0xbf */
#define UART_AUTOBAUD_EN     (base+0x20)
#define UART_HIGHSPEED       (base+0x24)
#define UART_SAMPLE_COUNT    (base+0x28) 
#define UART_SAMPLE_POINT    (base+0x2c) 
#define UART_AUTOBAUD_REG    (base+0x30)
#define UART_RATE_FIX_AD     (base+0x34)
#define UART_AUTOBAUD_SAMPLE (base+0x38)
#define UART_GUARD           (base+0x3c)
#define UART_ESCAPE_DAT      (base+0x40)
#define UART_ESCAPE_EN       (base+0x44)
#define UART_SLEEP_EN        (base+0x48)
#define UART_VFIFO_EN        (base+0x4c)
#define UART_RXTRI_AD        (base+0x50)

/* uart port ids */
enum {
    UART_PORT0 = 0,
    UART_PORT1,
    UART_PORT2,
    UART_PORT3,
    UART_PORT_NUM,
};

/* uart dma type */
enum {
    UART_NON_DMA,
    UART_TX_DMA,    
    UART_TX_VFIFO_DMA,
    UART_RX_VFIFO_DMA,
};

/* uart vfifo type */
enum {
    UART_TX_VFIFO,
    UART_RX_VFIFO,
};

/* uart dma mode */
enum {
    UART_DMA_MODE_0,
    UART_DMA_MODE_1,
};

/* flow control mode */
enum {
    UART_FC_NONE,
    UART_FC_SW,
    UART_FC_HW,
};

struct mt6516_uart_setting {
    int tx_mode;
    int rx_mode;
    int dma_mode;    
    int tx_trig_level;
    int rx_trig_level;
};

static struct mt6516_uart_setting mt6516_uart_default_settings[] = {
    //{UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_14B_TRI , UART_FCR_RXFIFO_1B_TRI},  /* ok */
    {UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    //{UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_1, UART_FCR_TXFIFO_14B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    //{UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_1, UART_FCR_TXFIFO_14B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    //{UART_TX_VFIFO_DMA     , UART_RX_VFIFO_DMA     ,UART_DMA_MODE_0, UART_FCR_TXFIFO_14B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    //{UART_TX_DMA      , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_1B_TRI}, /*ok */
    //{UART_TX_DMA      , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_12B_TRI}, /*ok */
    {UART_TX_VFIFO_DMA  , UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_8B_TRI ,  UART_FCR_RXFIFO_12B_TRI}, /* fail */
    
    //{UART_TX_VFIFO_DMA, UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_1B_TRI},     /* fail */
    //{UART_NON_DMA     , UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* tx abnormal */
    //{UART_NON_DMA     , UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_8B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* tx abnormal */
    //{UART_NON_DMA     , UART_RX_VFIFO_DMA, UART_DMA_MODE_1, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* tx abnormal */

    {UART_TX_VFIFO_DMA, UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_8B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    //{UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_12B_TRI},
    //{UART_TX_VFIFO_DMA, UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_12B_TRI},
    {UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_1B_TRI, UART_FCR_RXFIFO_12B_TRI},
};

struct mt6516_uart_hardware_setting
{
    u32 uart_base;
    u8  irq_num;
    u8  irq_sensitive;
    u8  power_on_reg_bit_shift;
    u8  power_off_reg_bit_shift;
    MT6516_PLL pll_id;    
    u32 vfifo_delay;
};
#define PDN_CLR0 (0xF0039340)   //80039340h
#define PDN_SET0 (0xF0039320)   //80039320h
static struct mt6516_uart_hardware_setting mt6516_uart_hwconfig[] = 
{   
    {UART1_BASE, MT6516_UART1_IRQ_LINE, MT6516_LEVEL_SENSITIVE,  7,  7,  MT6516_PDN_PERI_UART1, 10},
    {UART2_BASE, MT6516_UART2_IRQ_LINE, MT6516_LEVEL_SENSITIVE,  8,  8,  MT6516_PDN_PERI_UART2, 10},
    {UART3_BASE, MT6516_UART3_IRQ_LINE, MT6516_LEVEL_SENSITIVE,  9,  9,  MT6516_PDN_PERI_UART3, 10},
    {UART4_BASE, MT6516_UART4_IRQ_LINE, MT6516_LEVEL_SENSITIVE, 29, 29,  MT6516_PDN_PERI_UART4,10},
};
    
#ifdef DEBUG //================================================================
static struct mt6516_uart_regs mt6516_uart_debug[] =
{ 
    INI_REGS(UART1_BASE),
    INI_REGS(UART2_BASE),
    INI_REGS(UART3_BASE),
    INI_REGS(UART4_BASE),
};

#if defined(ENABLE_DMA_VFIFO)
/* MT6516 supports 8 Virtual FIFO DMA (Channel 17~24) */
static struct mt6516_dma_vfifo_reg mt6516_uart_vfifo_regs[] = {
    INI_DMA_VFIFO_REGS(DMA_BASE, 17),   /* dma vfifo channel 17 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 18),   /* dma vfifo channel 18 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 19),   /* dma vfifo channel 19*/
    INI_DMA_VFIFO_REGS(DMA_BASE, 20),   /* dma vfifo channel 20 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 21),   /* dma vfifo channel 21 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 22),   /* dma vfifo channel 22 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 23),   /* dma vfifo channel 23 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 24),   /* dma vfifo channel 24 */    
};
#endif /*ENABLE_DMA_VFIFO*/

#endif /*DEBUG*///=============================================================

struct mt6516_uart_vfifo
{
    unsigned short      id;     /* for dma vfifo request */
    unsigned short      used;   /* is used or not */
    unsigned short      size;   /* vfifo size */
    unsigned short      trig;   /* vfifo trigger level */
    void               *addr;   /* vfifo buffer addr */
    char               *port;   /* vfifo tx/rx port */
    struct mt_dma_conf *owner;  /* vfifo dma owner */
    struct timer_list   timer;  /* vfifo timer */
};

#if defined(ENABLE_DMA_VFIFO)
#define VFIFO_INIT(i,a)     {.id = (i), .port = (void*)(a)}
#define VFIFO_PORT_NUM      ARRAY_SIZE(mt6516_uart_vfifo_port)

static DEFINE_SPINLOCK(mt6516_uart_vfifo_port_lock);

enum
{
    DMA_VIRTUAL_FIFO_UART0 = 0,
    DMA_VIRTUAL_FIFO_UART1 = 1,
    DMA_VIRTUAL_FIFO_UART2 = 2,
    DMA_VIRTUAL_FIFO_UART3 = 3,
    DMA_VIRTUAL_FIFO_UART4 = 4,
    DMA_VIRTUAL_FIFO_UART5 = 5,
    DMA_VIRTUAL_FIFO_UART6 = 6,
    DMA_VIRTUAL_FIFO_UART7 = 7,
    
};
static struct mt6516_uart_vfifo mt6516_uart_vfifo_port[] = {
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART0, UART_VFIFO_PORT0),   /* vfifo port 0 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART1, UART_VFIFO_PORT1),   /* vfifo port 1 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART2, UART_VFIFO_PORT2),   /* vfifo port 2 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART3, UART_VFIFO_PORT3),   /* vfifo port 3 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART4, UART_VFIFO_PORT4),   /* vfifo port 4 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART5, UART_VFIFO_PORT5),   /* vfifo port 5 */                      
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART6, UART_VFIFO_PORT6),   /* vfifo port 6 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART7, UART_VFIFO_PORT7),   /* vfifo port 7 */                      
};
#endif /*ENABLE_DMA_VFIFO*/

struct mt6516_uart_dma {
    struct mt6516_uart       *uart;     /* dma uart */
    atomic_t                  free;        /* dma channel free */
    unsigned short            mode;        /* dma mode */
    unsigned short            dir;        /* dma transfer direction */
    struct mt_dma_conf       *cfg;        /* dma configuration */
    struct tasklet_struct     tasklet;    /* dma handling tasklet */
    struct completion         done;        /* dma transfer done */
    struct scatterlist        sg[3];
    unsigned long             sg_num;
    unsigned long             sg_idx;
    struct mt6516_uart_vfifo *vfifo;    /* dma vfifo */
};

struct mt6516_uart
{
    struct uart_port  port;
    unsigned long     base;
    int               nport;
    unsigned int      old_status;
    unsigned int      tx_stop;
    unsigned int      rx_stop;
    unsigned int      ms_enable;
    unsigned int      auto_baud;
    unsigned int      line_status;
    unsigned int      ignore_rx;
    unsigned int      flow_ctrl;
    unsigned long     pending_tx_reqs;
    unsigned long     tx_trig_level;  /* tx fifo trigger level */
    unsigned long     rx_trig_level;  /* rx fifo trigger level */
    unsigned long     sysclk;
    
    int               dma_mode;
    int               tx_mode;
    int               rx_mode;
    int               fctl_mode;     /*flow control*/

    struct mt6516_uart_dma dma_tx;
    struct mt6516_uart_dma dma_rx;
    struct mt6516_uart_vfifo *tx_vfifo;
    struct mt6516_uart_vfifo *rx_vfifo;

    struct scatterlist  sg_tx;
    struct scatterlist  sg_rx;
#ifdef DEBUG
    struct mt6516_uart_regs *debug;
#endif

    unsigned int (*write_allow)(struct mt6516_uart *uart);
    unsigned int (*read_allow)(struct mt6516_uart *uart);
    void         (*write_byte)(struct mt6516_uart *uart, unsigned int byte);
    unsigned int (*read_byte)(struct mt6516_uart *uart);
    unsigned int (*read_status)(struct mt6516_uart *uart);
};

struct mt6516_uart_dma_master{
    unsigned long tx_master;
    unsigned long rx_master;
};

/* uart control blocks */
static struct mt6516_uart mt6516_uarts[UART_NR];

/* uart dma master id */
static const struct mt6516_uart_dma_master mt6516_uart_dma_mas[] = {
    {DMA_CON_MASTER_UART0TX, DMA_CON_MASTER_UART0RX},
    {DMA_CON_MASTER_UART1TX, DMA_CON_MASTER_UART1RX},
    {DMA_CON_MASTER_UART2TX, DMA_CON_MASTER_UART2RX},
    {DMA_CON_MASTER_UART3TX, DMA_CON_MASTER_UART3RX}
};

extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);
static void mt6516_uart_init_ports(void);
static void mt6516_uart_start_tx(struct uart_port *port);
static void mt6516_uart_stop_tx(struct uart_port *port);
static void mt6516_uart_enable_intrs(struct mt6516_uart *uart, long mask);
static void mt6516_uart_disable_intrs(struct mt6516_uart *uart, long mask);
static unsigned int mt6516_uart_filter_line_status(struct mt6516_uart *uart);
static void mt6516_uart_dma_vfifo_tx_tasklet(unsigned long arg);
static void mt6516_uart_dma_vfifo_rx_tasklet(unsigned long arg);
extern bool is_meta_mode(void);
static void mt6516_uart_dma_vfifo_rx_intr_set(struct mt6516_uart *uart, bool bEnable);


#if defined(DEBUG)
/*define sysfs entry for configuring debug level and sysrq*/
ssize_t mt6516_uart_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer);
ssize_t mt6516_uart_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size);
ssize_t mt6516_uart_debug_show(struct kobject *kobj, char *page);
ssize_t mt6516_uart_debug_store(struct kobject *kobj, const char *page, size_t size);
ssize_t mt6516_uart_sysrq_show(struct kobject *kobj, char *page);
ssize_t mt6516_uart_sysrq_store(struct kobject *kobj, const char *page, size_t size);
struct sysfs_ops mt6516_uart_sysfs_ops = {
    .show   = mt6516_uart_attr_show,
    .store  = mt6516_uart_attr_store,
};
struct mtuart_entry {
    struct attribute attr;
    ssize_t (*show)(struct kobject *kobj, char *page);
    ssize_t (*store)(struct kobject *kobj, const char *page, size_t size);
};
struct mtuart_entry debug_entry = {
    {
        .name = "debug",
        .owner = NULL,
        .mode = S_IRUGO | S_IWUSR
    },
    mt6516_uart_debug_show,
    mt6516_uart_debug_store,
};
struct mtuart_entry sysrq_entry = {
    {
        .name = "sysrq",
        .owner = NULL,
        .mode = S_IRUGO | S_IWUSR
    },
    mt6516_uart_sysrq_show,
    mt6516_uart_sysrq_store,
};

struct attribute *mt6516_uart_attributes[] = {
    &debug_entry.attr,
    &sysrq_entry.attr,
    NULL,
};
struct kobj_type mt6516_uart_ktype = {
    .sysfs_ops = &mt6516_uart_sysfs_ops,
    .default_attrs = mt6516_uart_attributes,
};
struct kobject mt6516_uart_kobj;

static int mt6516_uart_sysfs(void) {
    memset(&mt6516_uart_kobj, 0, sizeof(mt6516_uart_kobj));
    mt6516_uart_kobj.parent = kernel_kobj;
    if (kobject_init_and_add(&mt6516_uart_kobj, &mt6516_uart_ktype, NULL, "mtuart")) {
        kobject_put(&mt6516_uart_kobj);
        return -ENOMEM;
    }
    kobject_uevent(&mt6516_uart_kobj, KOBJ_ADD);
    return 0;
}

ssize_t mt6516_uart_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer) {
    struct mtuart_entry *entry = container_of(attr, struct mtuart_entry, attr);
    return entry->show(kobj, buffer);
}

ssize_t mt6516_uart_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size) {
    struct mtuart_entry *entry = container_of(attr, struct mtuart_entry, attr);
    return entry->store(kobj, buffer, size);
}

ssize_t mt6516_uart_debug_show(struct kobject *kobj, char *buffer) {
    int remain = PAGE_SIZE;
    int len;
    char *ptr = buffer;
    int idx;

    for (idx = 0; idx < UART_NR; idx++) {
        len = snprintf(ptr, remain, "0x%2x\n", (unsigned int)mt6516_uart_evt_mask[idx]);
        ptr += len;
        remain -= len;
    }
    return (PAGE_SIZE-remain);
}

ssize_t mt6516_uart_debug_store(struct kobject *kobj, const char *buffer, size_t size) {
    int a, b, c, d;
    int res = sscanf(buffer, "0x%x 0x%x 0x%x 0x%x", &a, &b, &c, &d);

    if (res != 4) {
        printk("%s: expect 4 numbers\n", __FUNCTION__);
    } else {
        mt6516_uart_evt_mask[0] = a;
        mt6516_uart_evt_mask[1] = b;
        mt6516_uart_evt_mask[2] = c;
        mt6516_uart_evt_mask[3] = d;
    }
    return size;
}

ssize_t mt6516_uart_sysrq_show(struct kobject *kobj, char *buffer) {
    return snprintf(buffer, PAGE_SIZE, "%d\n", mt6516_uart_sysrq);
}

ssize_t mt6516_uart_sysrq_store(struct kobject *kobj, const char *buffer, size_t size) {
    int a;
    int res = sscanf(buffer, "%d\n", &a);

    if (res != 1) {
        printk("%s: expect 1 number\n", __FUNCTION__);
    } else {
        mt6516_uart_sysrq = a;    
    }
    return size;
}

#endif 
static void dump_reg(struct mt6516_uart *uart, const char* caller)
{
#ifdef DEBUG
    u32 base = uart->base;
    u32 lcr = UART_READ32(UART_LCR);
    u32 uratefix = UART_READ32(UART_RATE_FIX_AD);
    u32 uhspeed  = UART_READ32(UART_HIGHSPEED);
    u32 usamplecnt = UART_READ32(UART_SAMPLE_COUNT);
    u32 usamplepnt = UART_READ32(UART_SAMPLE_POINT);
    u32 udll, udlh;
    u32 ier = UART_READ32(UART_IER);
    UART_WRITE32((lcr | UART_LCR_DLAB), UART_LCR);
    udll = UART_READ32(UART_DLL);
    udlh = UART_READ32(UART_DLH);
    UART_WRITE32(lcr, UART_LCR);     /* DLAB end */
    
    
    MSG(CFG, "%s: RATEFIX(%02X); HSPEED(%02X); CNT(%02X); PNT(%02X); DLH(%02X), DLL(%02X), IER(%02X)\n", 
            caller, uratefix, uhspeed, usamplecnt, usamplepnt, udlh, udll, ier);
#endif             
}

#ifdef CONFIG_SERIAL_MT6516_CONSOLE
static void mt6516_uart_console_write(struct console *co, const char *s,
    unsigned int count)
{
    /* Notice:
     * The function is called by printk, hence, spin lock can not be used
     */
    int i;
    struct mt6516_uart *uart;

    if (co->index >= UART_NR || !(co->flags & CON_ENABLED))
        return;

    uart = &mt6516_uarts[co->index];    
    for (i = 0; i < (int)count; i++) {
        while (!uart->write_allow(uart)) {
            barrier();
        }
        uart->write_byte(uart, s[i]);

        if (s[i] == '\n') {
            while (!uart->write_allow(uart)) {
                barrier();
            }
            uart->write_byte(uart, '\r');
        }
    }
}

static int __init mt6516_uart_console_setup(struct console *co, char *options)
{
    struct uart_port *port;
    int baud    = 115200;
    int bits    = 8;
    int parity  = 'n';
    int flow    = 'n';
    int ret;

    printk(KERN_ALERT DBG_TAG "mt6516 console setup : co->index %d options:%s\n",
        co->index, options);

    if (co->index >= UART_NR)
        co->index = 0;
    port = (struct uart_port *)&mt6516_uarts[co->index];

    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);

    ret = uart_set_options(port, co, baud, parity, bits, flow);
    printk(KERN_ALERT DBG_TAG "mt6516 console setup : uart_set_option port(%d) "
          "baud(%d) parity(%c) bits(%d) flow(%c) - ret(%d)\n",
           co->index, baud, parity, bits, flow, ret);
    
    printk(KERN_ALERT DBG_TAG "mt6516 setting: (%d, %d, %d, %lu, %lu)\n", 
           mt6516_uarts[co->index].tx_mode, mt6516_uarts[co->index].rx_mode,
           mt6516_uarts[co->index].dma_mode,mt6516_uarts[co->index].tx_trig_level,
           mt6516_uarts[co->index].rx_trig_level);
    return ret;
}

static struct uart_driver mt6516_uart_drv;
static struct console mt6516_uart_console =
{
    .name       = "ttyMT",
#if !defined(CONFIG_SERIAL_MT6516_MODEM_TEST)    
    /*don't configure UART4 as console*/
    .write      = mt6516_uart_console_write,
    .setup      = mt6516_uart_console_setup,
#endif     
    .device     = uart_console_device,
    .flags      = CON_PRINTBUFFER,
    .index      = -1,
    .data       = &mt6516_uart_drv,
};

static int __init mt6516_uart_console_init(void)
{
    mt6516_uart_init_ports();
    register_console(&mt6516_uart_console);
    return 0;
}

console_initcall(mt6516_uart_console_init);

static int __init mt6516_late_console_init(void)
{
    if (!(mt6516_uart_console.flags & CON_ENABLED))
    {
        register_console(&mt6516_uart_console);
    }
    return 0;
}

late_initcall(mt6516_late_console_init);

#endif /* CONFIG_SERIAL_MT6516_CONSOLE */

#if defined(ENABLE_DMA_VFIFO)
static struct mt6516_uart_vfifo *mt6516_uart_vfifo_alloc(
                                 struct mt6516_uart *uart, int dir, int size)
{
    int i;
    void *buf;    
    struct mt6516_uart_vfifo *vfifo = NULL;

    spin_lock(&mt6516_uart_vfifo_port_lock);

    /* Notice:
     * In MT6516, the number of virtual FIFO is 8. And the number of UART is 4.
     * Hence, we apply VFIFO 0~3 to RX VFIFO; VFIFO 4~7 to TX VFIFO.
     * Please notice that it is not hardware limitation. The 8 VFIFO ports could
     * be used both for TX and RX.
     */
    if (dir == UART_RX_VFIFO) {
        if (mt6516_uart_vfifo_port[uart->nport].used) {
            dev_err(uart->port.dev, "No available vfifo port\n");
            goto end;
        } else {
            i = uart->nport;
        }
    } else {
        for (i = 4; i < VFIFO_PORT_NUM; i++) {
            if (!mt6516_uart_vfifo_port[i].used)
                break;        
        }
    }

    if (i == VFIFO_PORT_NUM) {
        dev_err(uart->port.dev, "No available vfifo port\n");
        goto end;
    }
    buf = (void*)kmalloc(size, GFP_ATOMIC);
    
    if (!buf) {
        dev_err(uart->port.dev, "Cannot allocate memory\n");
        goto end;
    }
    MSG(INFO, "alloc vfifo-%d[%d](%p) to uart-%d\n", i, size, buf, uart->nport);

    vfifo = &mt6516_uart_vfifo_port[i];
    vfifo->size = size;
    vfifo->addr = buf;
    vfifo->used = 1;
    
end:
    spin_unlock(&mt6516_uart_vfifo_port_lock);
    return vfifo;

}
static void mt6516_uart_vfifo_free(struct mt6516_uart *uart,
                                   struct mt6516_uart_vfifo *vfifo)
{
    if (vfifo) {
        spin_lock(&mt6516_uart_vfifo_port_lock);     
        kfree(vfifo->addr);
        vfifo->addr  = 0;
        vfifo->size  = 0;
        vfifo->used  = 0;
        vfifo->owner = 0;
        spin_unlock(&mt6516_uart_vfifo_port_lock);
    }
}

inline static void mt6516_uart_vfifo_enable(struct mt6516_uart *uart)
{
    u32 base = uart->base;
    
    UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR);
    UART_CLR_BITS(UART_FCR_DMA1, UART_FCR);     /* must be mode 0 */
    UART_SET_BITS(UART_VFIFO_ON, UART_VFIFO_EN);
}

inline static void mt6516_uart_vfifo_disable(struct mt6516_uart *uart)
{
    u32 base = uart->base;
    
    UART_CLR_BITS(UART_VFIFO_ON, UART_VFIFO_EN);
}

inline static int mt6516_uart_vfifo_is_full(struct mt6516_uart_vfifo *vfifo)
{
    INFO info;   
    DMA_STATUS err = mt_get_info(vfifo->owner, VF_FULL, &info);

    if (DMA_FAIL == err)
        printk(KERN_ERR DBG_TAG "mt_get_info err = %d\n", err);
    return (int)info;
}

inline static int mt6516_uart_vfifo_is_empty(struct mt6516_uart_vfifo *vfifo)
{
    INFO info;
    DMA_STATUS err = mt_get_info(vfifo->owner, VF_EMPTY, &info);

    if (DMA_FAIL == err)
        printk(KERN_ERR DBG_TAG "mt_get_info err = %d\n", err);
    return (int)info;
}

static unsigned int mt6516_uart_vfifo_write_allow(struct mt6516_uart *uart)
{
    return !mt6516_uart_vfifo_is_full(uart->tx_vfifo);
}

static unsigned int mt6516_uart_vfifo_read_allow(struct mt6516_uart *uart)
{
    return !mt6516_uart_vfifo_is_empty(uart->rx_vfifo);
}

static void mt6516_uart_vfifo_write_byte(struct mt6516_uart *uart, 
                                                unsigned int byte)
{
    UART_WRITE8((unsigned char)byte, uart->tx_vfifo->port);
}

static unsigned int mt6516_uart_vfifo_read_byte(struct mt6516_uart *uart)
{
    return (unsigned int)UART_READ8(uart->rx_vfifo->port);
}

static void mt6516_uart_vfifo_set_trig(struct mt6516_uart *uart, 
                                       struct mt6516_uart_vfifo *vfifo,
                                       u16 level)
{
    if (!vfifo || !vfifo->owner || !level) 
        return;

    vfifo->trig = level;
    vfifo->owner->count = level;
    if (DMA_FAIL == mt_config_dma(vfifo->owner, ALL))
        MSG(ERR, "mt_config_dma fail");
}

inline static unsigned short mt6516_uart_vfifo_get_trig(
                                        struct mt6516_uart *uart, 
                                        struct mt6516_uart_vfifo *vfifo)
{
    return vfifo->trig;
}

inline static void mt6516_uart_vfifo_set_owner(struct mt6516_uart_vfifo *vfifo,
                                               struct mt_dma_conf *owner)
{
    vfifo->owner = owner;
}

inline static int mt6516_uart_vfifo_get_counts(struct mt6516_uart_vfifo *vfifo)
{
    INFO info;
    DMA_STATUS err = mt_get_info(vfifo->owner, VF_FFCNT, &info);
    
    if (DMA_FAIL == err)
        printk(KERN_ERR DBG_TAG "mt_get_info err = %d\n", err);
    return (int)info;
}

static void mt6516_uart_dma_map(struct mt6516_uart *uart, 
                                struct scatterlist *sg, char *buf, 
                                unsigned long size, int dir)
{
    int res;
    sg_init_one(sg, (u8*)buf, size);
    res = dma_map_sg(uart->port.dev, sg, 1, dir);
    if (res != 1)
        MSG(ERR, "dma_map_sg fails\n");
}

static void mt6516_uart_dma_unmap(struct mt6516_uart *uart,
                                  struct scatterlist *sg, int dir)
{
    dma_unmap_sg(uart->port.dev, sg, 1, dir);
}

static void mt6516_uart_dma_setup(struct mt6516_uart *uart,
                                  struct mt6516_uart_dma *dma,
                                  struct scatterlist *sg)
{
    struct mt_dma_conf *cfg;
    
    if (!dma || !dma->cfg)
        return;

    cfg = dma->cfg;
    
    if (dma->mode == UART_TX_DMA) {      
        cfg->pgmaddr = sg_dma_address(sg);
        cfg->count = sg_dma_len(sg);
        cfg->b2w   = DMA_TRUE;
        cfg->size  = DMA_CON_SIZE_BYTE;
        cfg->burst = DMA_CON_BURST_SINGLE;

        memcpy(&dma->sg[0], sg, sizeof(struct scatterlist));
        dma->sg_idx = 0;
        dma->sg_num = 1;
    }
    if (DMA_FAIL == mt_config_dma(cfg, ALL))
        MSG(ERR, "mt_config_dma fails\n");
}

static void mt6516_uart_dma_vfifo_rx_intr_set(struct mt6516_uart *uart, bool bEnable)
{
    struct mt6516_uart_dma *dma_rx = &uart->dma_rx;    
    struct mt_dma_conf *cfg = dma_rx->cfg;
    if(bEnable)
        cfg->iten = DMA_TRUE;
    else
        cfg->iten = DMA_FALSE; 
               
    if (DMA_FAIL == mt_config_dma(cfg, ALL))
        MSG(ERR, "mt6516_uart_dma_vfifo_rx_intr_set fails\n");  
}

static int mt6516_uart_dma_start(struct mt6516_uart *uart,
                                 struct mt6516_uart_dma *dma)

{
    struct mt_dma_conf *cfg = dma->cfg;

    MSG_FUNC_ENTRY();
    if (!atomic_read(&dma->free))
        return -1;

    atomic_set(&dma->free, 0);
    init_completion(&dma->done);
    mt_start_dma(cfg);
    return 0;
}

static void mt6516_uart_dma_stop(struct mt6516_uart *uart, 
                                 struct mt6516_uart_dma *dma)
{
    MSG_FUNC_ENTRY();
    if (dma && dma->cfg) {
        mt_stop_dma(dma->cfg);
        atomic_set(&dma->free, 1);
        complete(&dma->done);
    }
}


static void mt6516_uart_dma_callback(void *data)
{
    struct mt6516_uart_dma *dma = (struct mt6516_uart_dma*)data;
    
    tasklet_schedule(&dma->tasklet);
}

static void mt6516_uart_dma_tx_tasklet(unsigned long arg)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)arg;
    struct uart_port   *port = &uart->port;
    struct mt6516_uart_dma *dma = &uart->dma_tx;
    struct scatterlist *sg   = &dma->sg[dma->sg_idx];
    struct circ_buf    *xmit = &port->info->xmit;
    int resched = 0;
    
    spin_lock_bh(&port->lock);
    
    mt6516_uart_dma_stop(uart, dma);    
    xmit->tail = (xmit->tail + sg_dma_len(sg)) & (UART_XMIT_SIZE - 1);
    
    if (dma->sg_num > ++dma->sg_idx) {
        resched = 1;
    } else {
        MSG(DMA, "TX DMA transfer %d bytes done!!\n", sg_dma_len(sg));    
        port->icount.tx++;
        mt6516_uart_dma_unmap(uart, &uart->sg_tx, DMA_TO_DEVICE);
    }
    
    if (uart_circ_empty(xmit)) {
        uart->pending_tx_reqs = 0;
    } else if (uart->pending_tx_reqs) {
        uart->pending_tx_reqs--;
        resched = 1;
    }
    if (resched)
        mt6516_uart_start_tx(port);

    spin_unlock_bh(&port->lock);

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);       
}

static void mt6516_uart_dma_vfifo_timeout(unsigned long data)
{
    /* FIXME: the mechanism could be removed:
     * This is a workaround when applying edge-sensitive IRQ,
     * After applying level-sensitive IRQ, the polling time is
     * replaced by RX timeout interrupt handler
     */
#if 0 
    struct mt6516_uart_dma *dma = (struct mt6516_uart_dma *)data;
    struct mt6516_uart *uart = dma->uart;
    int delay;

    //tasklet_schedule(&dma->tasklet);
    if (dma && uart) {
        if (!mt6516_uart_vfifo_is_empty(dma->vfifo)) {
            mt6516_uart_dma_vfifo_rx_tasklet((unsigned long)dma->uart);
        }
        
        if (dma->uart->port.line < UART_NR) {
            delay = mt6516_uart_hwconfig[dma->uart->port.line].vfifo_delay;
        } else {        
            MSG(ERR, "%s: invalid line: %d\n", __func__, dma->uart->port.line);
            delay = 10;
        }

        mod_timer(&dma->vfifo->timer, jiffies + delay/(1000/HZ));    
    } else {
        WARN_ON("Timer into wrong place\n");
        del_timer(&dma->vfifo->timer);
    }
#endif     
}

static void mt6516_uart_dma_vfifo_callback(void *data)
{
    struct mt6516_uart_dma *dma = (struct mt6516_uart_dma*)data;
    struct mt6516_uart *uart = dma->uart;
        
    MSG(DMA, "%s VFIFO CB: %d/%d\n", dma->dir == DMA_TO_DEVICE ? "TX" : "RX",
        mt6516_uart_vfifo_get_counts(dma->vfifo), dma->vfifo->size);

    if (dma->dir == DMA_FROM_DEVICE) {
        mt6516_uart_dma_vfifo_rx_tasklet((unsigned long)uart);
        return;
    }
    
    tasklet_schedule(&dma->tasklet);
}

static char  txbuf[4096];
static ulong txbuf_idx;
static char  rxbuf[4096];
static ulong rxbuf_idx;

static void mt6516_uart_dma_vfifo_tx_tasklet(unsigned long arg)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)arg;
    struct uart_port   *port = &uart->port;
    struct mt6516_uart_dma *dma = &uart->dma_tx;    
    struct mt6516_uart_vfifo *vfifo = uart->tx_vfifo;
    struct circ_buf    *xmit = &port->info->xmit;
    unsigned long flags;
    unsigned int size, left;

    /* stop tx if circular buffer is empty or this port is stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
        uart->pending_tx_reqs = 0;
        atomic_set(&dma->free, 1);
        complete(&dma->done);    
        mt6516_uart_stop_tx(port);
        return;    
    }

    spin_lock_irqsave(&port->lock, flags);

    size = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);
    left = vfifo->size - mt6516_uart_vfifo_get_counts(vfifo);

    //dump_reg(uart, __func__);
    MSG(DMA, "TxBuf[%d]: Write %d bytes to VFIFO[%d]\n", 
        size, left < size ? left : size, 
        vfifo->size - mt6516_uart_vfifo_get_counts(vfifo));

    size = left = left < size ? left : size;

    txbuf_idx = 0;
    while (size--) {
        txbuf[txbuf_idx++]=(char)xmit->buf[xmit->tail];
        uart->write_byte(uart, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
    }

    spin_unlock_irqrestore(&port->lock, flags);

#if defined(VFIFO_DEBUG)
    {
        int i;
        printk("[UART%d_TX] %ld bytes:", uart->nport, txbuf_idx);
        for (i = 0; i < txbuf_idx; i++) {
            if (i % 16 == 0)
                printk("\n");
            printk("%.2x ", (unsigned char)txbuf[i]);
        }
        printk("\n");
    }
#endif

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

    /* reschedule to write data to vfifo */
    tasklet_schedule(&uart->dma_tx.tasklet);
}

static void mt6516_uart_dma_vfifo_rx_tasklet(unsigned long arg)
{
    struct mt6516_uart *uart = (struct mt6516_uart*)arg;
    struct uart_port   *port = &uart->port;
    struct mt6516_uart_vfifo *vfifo = uart->rx_vfifo;
    struct tty_struct *tty = uart->port.info->port.tty;
    int count, left;
    unsigned int ch, flag, status;
    unsigned long flags;
#if defined(ENABLE_VFIFO_LEVEL_SENSITIVE)    
    unsigned long tmp;
    u32 base = uart->base;

    MSG_FUNC_ENTRY();
    /* IMPORTANT: this is a fix for HW Bug. 
     * Without the function call, the RX data timeout interrupt will be 
     * triggered again and again.Hence, the purpose of this function call
     * is to clear Rx data timeout interrupt
     */
    tmp = UART_READ32(UART_VFIFO_EN);
    MSG(FUC, "read VFIFO_EN\n");
#endif     

    if (mt6516_uart_vfifo_is_empty(vfifo)) {
        mt6516_uart_dma_stop(uart, &uart->dma_rx);
        if (mt6516_uart_dma_start(uart, &uart->dma_rx))
            MSG(ERR, "mt6516_uart_dma_start fail\n");
        return;
    }
    //dump_reg(uart, __func__);
    count = left = mt6516_uart_vfifo_get_counts(vfifo);
    
    spin_lock_irqsave(&port->lock, flags);
    rxbuf_idx = 0;
    while (!mt6516_uart_vfifo_is_empty(vfifo) && count > 0) {

        /* check status */
        status = uart->read_status(uart);
        status = mt6516_uart_filter_line_status(uart);
        
        ch = uart->read_byte(uart);
        flag = TTY_NORMAL;
        /* error handling routine */
        if (status & UART_LSR_BI) {
            MSG(INFO, "Break Interrupt!!!\n");
            port->icount.brk++;
            if (uart_handle_break(port))
                continue;
            flag = TTY_BREAK;
        } else if (status & UART_LSR_PE) {
            MSG(INFO, "Parity Error!!!\n");
            port->icount.parity++;
            flag = TTY_PARITY;
        } else if (status & UART_LSR_FE) {
            MSG(INFO, "Frame Error!!!\n");
            port->icount.frame++;
            flag = TTY_FRAME;
        } else if (status & UART_LSR_OE) {
            MSG(INFO, "Overrun!!!\n");
            port->icount.overrun++;
            flag = TTY_OVERRUN;        
        }        
        port->icount.rx++;
        count--;
        rxbuf[rxbuf_idx++]=(char)ch;
        if (!tty_insert_flip_char(tty, ch, flag))
            MSG(ERR, "tty_insert_flip_char: no space\n");
    }
    tty_flip_buffer_push(tty);

    MSG(DMA, "RxBuf[%d]: Read %d bytes from VFIFO[%d]\n", 
        left, left - count, mt6516_uart_vfifo_get_counts(vfifo));

    spin_unlock_irqrestore(&port->lock, flags);

#if defined(VFIFO_DEBUG)
    {
        int i;
        printk("[UART%d_RX] %ld bytes:", uart->nport, rxbuf_idx);
        
        for (i = 0; i < rxbuf_idx; i++) {        
            if (i % 16 == 0)
                printk("\n");
            printk("%.2x ", (unsigned char)rxbuf[i]);
        }
        printk("\n");
        
    }
#endif

    if (left) {
        if (mt6516_uart_vfifo_get_counts(vfifo) > 
            mt6516_uart_vfifo_get_trig(uart, vfifo))
        tasklet_schedule(&uart->dma_rx.tasklet);
    } 
}

static int mt6516_uart_dma_alloc(struct mt6516_uart *uart, 
                                 struct mt6516_uart_dma *dma, int mode,
                                 struct mt6516_uart_vfifo *vfifo)
{
    struct mt_dma_conf *cfg;
    int ret = 0;

    MSG_FUNC_ENTRY();

    if (mode == UART_NON_DMA || dma->cfg)
        return -1;
    
    switch (mode) {
    case UART_TX_DMA:
        cfg = mt_request_half_size_dma();
        if (!cfg) {
            printk(KERN_ERR "uart-%d: fail to alloc tx dma\n", uart->nport);
            ret = -1;
            break;
        }            
        /* set tx master*/
        mt_reset_dma(cfg);

        cfg->mas      = mt6516_uart_dma_mas[uart->nport].tx_master;
        cfg->iten     = DMA_TRUE;    /* interrupt mode */
        cfg->dreq     = DMA_TRUE;    /* hardware handshaking */
        cfg->limiter  = 0;
        cfg->dinc     = DMA_FALSE;   /* tx buffer     */
        cfg->sinc     = DMA_TRUE;    /* memory buffer */    
        cfg->dir      = DMA_FALSE;   /* memory buffer to tx buffer */
        cfg->data     = (void*)dma;
        cfg->callback = mt6516_uart_dma_callback;
        
        atomic_set(&dma->free, 1);
        tasklet_init(&dma->tasklet, mt6516_uart_dma_tx_tasklet, 
                    (unsigned long)uart);
        dma->dir  = DMA_TO_DEVICE;
        dma->mode = mode;
        dma->cfg  = cfg;
        dma->uart = uart;
        break;
    case UART_TX_VFIFO_DMA:
        /* only uart 0~2 support vfifo dma */
        if (uart->nport > 2) {
            ret = -1;
            break;
        }
        if (!vfifo) {
            printk(KERN_ERR "uart-%d: fail due to NULL tx_vfifo\n", uart->nport);
            ret = -1;
            break;
        }
        cfg = mt_request_virtual_fifo_dma(vfifo->id);
        if (!cfg) {
            printk(KERN_ERR "uart-%d: fail to alloc tx dma\n", uart->nport);
            ret = -1;
            break;
        }
        /* set tx master*/
        mt_reset_dma(cfg);

        cfg->mas      = mt6516_uart_dma_mas[uart->nport].tx_master;
        cfg->iten     = DMA_FALSE;    /* interrupt mode. CHECKME!!! */
        cfg->dreq     = DMA_TRUE;    /* hardware handshaking. CHECKME!!! */
        cfg->limiter  = 0;
        cfg->size     = DMA_CON_SIZE_BYTE;
        cfg->sinc     = DMA_TRUE;
        cfg->dinc     = DMA_FALSE;
        cfg->dir      = DMA_FALSE;  /* UART reads from vfifo to tx buffer */
        cfg->count    = 0;          /* default trigger level */
        cfg->ffsize   = UART_VFIFO_SIZE;
        cfg->altlen   = UART_VFIFO_ALERT_LEN;
        cfg->pgmaddr  = (u32)virt_to_phys(vfifo->addr);  /* tx vfifo address */
        cfg->data     = (void*)dma;
        cfg->callback = mt6516_uart_dma_vfifo_callback;

        mt6516_uart_vfifo_set_owner(vfifo, cfg);
        atomic_set(&dma->free, 1);
        init_completion(&dma->done);        
        tasklet_init(&dma->tasklet, mt6516_uart_dma_vfifo_tx_tasklet,
                    (unsigned long)uart);
        dma->dir   = DMA_TO_DEVICE;   
        dma->mode  = mode;
        dma->vfifo = vfifo;
        dma->cfg   = cfg;
        dma->uart  = uart;
        break;
    case UART_RX_VFIFO_DMA:
        /* only uart 0~2 support vfifo dma */
        if (uart->nport > 2) {
            ret = -1;
            break;
        }
        if (!vfifo) {
            printk(KERN_ERR "uart-%d: fail due to NULL rx_vfifo\n", uart->nport);
            ret = -1;
            break;
        }
        cfg = mt_request_virtual_fifo_dma(vfifo->id);
        if (!cfg) {
            printk(KERN_ERR "uart-%d: fail to alloc rx dma\n", uart->nport);
            ret = -1;
            break;
        }            
        /* set tx master*/
        mt_reset_dma(cfg);
    
        cfg->mas      = mt6516_uart_dma_mas[uart->nport].rx_master;
        cfg->iten     = DMA_TRUE;   /* interrupt mode */
        cfg->dreq     = DMA_TRUE;   /* hardware handshaking */
        cfg->limiter  = 0;
        cfg->size     = DMA_CON_SIZE_BYTE;
        cfg->sinc     = DMA_FALSE;
        cfg->dinc     = DMA_TRUE;            
        cfg->dir      = DMA_TRUE;   /* UART writes vfifo from rx buffer */          
        cfg->count    = 0;          /* default trigger level */
        cfg->ffsize   = UART_VFIFO_SIZE;
        cfg->altlen   = UART_VFIFO_ALERT_LEN;
        cfg->pgmaddr  = (u32)virt_to_phys(vfifo->addr); /* rx vfifo address */
        cfg->data     = (void*)dma;
        cfg->callback = mt6516_uart_dma_vfifo_callback;
        
        mt6516_uart_vfifo_set_owner(vfifo, cfg);  
        atomic_set(&dma->free, 1);
        init_completion(&dma->done);        
        tasklet_init(&dma->tasklet, mt6516_uart_dma_vfifo_rx_tasklet, 
                    (unsigned long)uart);
        dma->dir   = DMA_FROM_DEVICE;
        dma->mode  = mode;
        dma->vfifo = vfifo;
        dma->cfg   = cfg;
        dma->uart  = uart;        
        break;
    }
    return ret;
}

static void mt6516_uart_dma_free(struct mt6516_uart *uart, 
                                 struct mt6516_uart_dma *dma)
{
    unsigned long flags;
    
    MSG_FUNC_ENTRY();

    if (!dma || !dma->cfg)
        return;

    if (!dma->vfifo) {
        if (!atomic_read(&dma->free)) {            
            MSG(DMA, "wait for %s dma completed!!!\n", 
                dma->dir == DMA_TO_DEVICE ? "TX" : "RX");
            wait_for_completion(&dma->done);
        }
    } else if (dma->vfifo && !mt6516_uart_vfifo_is_empty(dma->vfifo)) {
        tasklet_schedule(&dma->tasklet);
        MSG(DMA, "wait for %s vfifo dma completed!!!\n", 
            dma->dir == DMA_TO_DEVICE ? "TX" : "RX");    
        wait_for_completion(&dma->done);          
    }
    spin_lock_irqsave(&uart->port.lock, flags);
    mt_stop_dma(dma->cfg);
    if (dma->vfifo && timer_pending(&dma->vfifo->timer))
        del_timer_sync(&dma->vfifo->timer);
    tasklet_kill(&dma->tasklet);
    mt_free_dma(dma->cfg);
    MSG(INFO, "free %s dma completed!!!\n", 
        dma->dir == DMA_TO_DEVICE ? "TX" : "RX");    
    memset(dma, 0, sizeof(struct mt6516_uart_dma));
    spin_unlock_irqrestore(&uart->port.lock, flags);    
}
#endif /*defined(ENABLE_DMA_VFIFO)*/

inline static void mt6516_uart_fifo_init(struct mt6516_uart *uart)
{
    u32 base = uart->base;

    UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR);
}

inline static void mt6516_uart_fifo_flush(struct mt6516_uart *uart)
{
    u32 base = uart->base;

    UART_SET_BITS(UART_FCR_CLRR | UART_FCR_CLRT, UART_FCR);
}

static void mt6516_uart_fifo_set_trig(struct mt6516_uart *uart, int tx_level, 
                                      int rx_level)
{
    u32 base = uart->base;
    unsigned long tmp1;
    //unsigned long tmp2;

    tmp1 = UART_READ32(UART_LCR);
    UART_WRITE32(0xbf, UART_LCR);
    //tmp2 = UART_READ32(UART_EFR);
    UART_SET_BITS(UART_EFR_EN, UART_EFR);
    MSG(INFO, "%s(EFR) =  %04X\n", __func__, UART_READ32(UART_EFR));
    UART_WRITE32(tmp1, UART_LCR);

    UART_WRITE32(UART_FCR_FIFO_INIT|tx_level|rx_level, UART_FCR);

    //tmp1 = UART_READ32(UART_LCR);
    //UART_WRITE32(0xbf, UART_LCR);
    //UART_WRITE32(tmp2, UART_EFR);
    //UART_WRITE32(tmp1, UART_LCR);

}

static void mt6516_uart_set_mode(struct mt6516_uart *uart, int mode)
{
    u32 base = uart->base;

    if (mode == UART_DMA_MODE_0) {
        UART_CLR_BITS(UART_FCR_DMA1, UART_FCR);
    } else if (mode == UART_DMA_MODE_1) {
        UART_SET_BITS(UART_FCR_DMA1, UART_FCR);
    }
}

static void mt6516_uart_set_auto_baud(struct mt6516_uart *uart)
{
    u32 base = uart->base;

    MSG_FUNC_ENTRY();
        
    switch (uart->sysclk)
    {
    case MT6516_SYSCLK_13:
        UART_WRITE32(UART_AUTOBADUSAM_13M, UART_AUTOBAUD_SAMPLE);
        break;
    case MT6516_SYSCLK_26:
        UART_WRITE32(UART_AUTOBADUSAM_26M, UART_AUTOBAUD_SAMPLE);                    
        break;
    case MT6516_SYSCLK_52:
        UART_WRITE32(UART_AUTOBADUSAM_52M, UART_AUTOBAUD_SAMPLE);
        break;
    default:
        dev_err(uart->port.dev, "SYSCLK = %ldMHZ doesn't support autobaud\n",
                uart->sysclk);
        return;
    }
    UART_WRITE32(0x01, UART_AUTOBAUD_EN); /* Enable Auto Baud */
    return;
}

static void mt6516_uart_set_baud(struct mt6516_uart *uart , int baudrate)
{
    u32 base = uart->base;
    //unsigned int byte, highspeed, quot, remainder, ratefix;
    unsigned int divisor, uartclk;
    unsigned int lcr = UART_READ32(UART_LCR);

    uartclk = uart->sysclk;
    
    if (uart->port.flags & ASYNC_SPD_CUST) {
        /*the baud_base gotten in user space eqauls to sysclk/16.
          hence, we need to restore the difference when calculating custom baudrate */
        baudrate = uart->sysclk/16; 
        baudrate = baudrate/uart->port.custom_divisor;
        MSG(CFG, "CUSTOM, baudrate = %d, divisor = %d\n", baudrate, uart->port.custom_divisor);
    }

    if (uart->auto_baud)
        mt6516_uart_set_auto_baud(uart); 
#if 1 //apply new register setting for supporting high speed UART
    if (baudrate <= 115200) {
        uartclk = uart->sysclk;
        UART_WRITE32(0x00, UART_RATE_FIX_AD);
        UART_WRITE32(0x00, UART_HIGHSPEED); /*divider is 16*/
        divisor = (uartclk >> 4)/(unsigned int)baudrate;
    } else if (baudrate <= 460800) {
        uartclk = uart->sysclk;
        UART_WRITE32(0x00, UART_RATE_FIX_AD);
        UART_WRITE32(0x02, UART_HIGHSPEED); /*divider is 4*/
        divisor = (uartclk >> 2)/(unsigned int)baudrate;
    } else {
        uartclk = uart->sysclk;
        UART_WRITE32(0x00, UART_RATE_FIX_AD);
        UART_WRITE32(0x03, UART_HIGHSPEED);
        divisor = (uartclk)/(unsigned int)baudrate;
    }
    
    if (baudrate <= 460800) {
        UART_WRITE32(lcr|UART_LCR_DLAB, UART_LCR);
        UART_WRITE32((divisor&0xFF), UART_DLL);
        UART_WRITE32(((divisor>>8) & 0xFF), UART_DLH);
        UART_WRITE32(lcr, UART_LCR);
    } else {
        unsigned int sample_count, sample_point;
        sample_count = divisor - 1;
        sample_point = (sample_count-1) >> 1;
        UART_WRITE32(lcr|UART_LCR_DLAB, UART_LCR);
        UART_WRITE32(0x01, UART_DLL);
        UART_WRITE32(0x00, UART_DLH);
        UART_WRITE32(lcr, UART_LCR);
        UART_WRITE32(sample_count, UART_SAMPLE_COUNT);
        UART_WRITE32(sample_point, UART_SAMPLE_POINT);
        if (baudrate >= 3000000)
            UART_WRITE32(0x12, UART_GUARD);
    }
       
#else   
        
#if (UART_SYSCLK == MT6516_SYSCLK_13)
    ratefix = UART_READ32(UART_RATE_FIX_AD);
    if (ratefix & UART_RATE_FIX_ALL_13M) {
        if (ratefix & UART_FREQ_SEL_13M)
            uartclk = uart->sysclk / 4;
        else
            uartclk = uart->sysclk / 2;
    }
#else
    UART_WRITE32(0, UART_RATE_FIX_AD);
#endif
    
    uart->port.uartclk = uartclk;

    if (baudrate <= 115200 ) {
        highspeed = 0;
        quot = 16;
    } else {
        highspeed = 2;
        quot = 4;
    }

    /* Set divisor DLL and DLH  */             
    divisor   =  uartclk / (quot * baudrate);
    remainder =  uartclk % (quot * baudrate);
          
    if (remainder >= (quot / 2) * baudrate)
        divisor += 1;

    UART_WRITE16(highspeed, UART_HIGHSPEED);
    byte = UART_READ32(UART_LCR);     /* DLAB start */
    UART_WRITE32((byte | UART_LCR_DLAB), UART_LCR);
    UART_WRITE32((divisor & 0x00ff), UART_DLL);
    UART_WRITE32(((divisor >> 8)&0x00ff), UART_DLH);
    UART_WRITE32(byte, UART_LCR);     /* DLAB end */
#endif
    MSG(CFG, "BaudRate = %d, SysClk = %d, Divisor = %d, %04X/%04X\n", baudrate, uartclk, divisor, UART_READ32(UART_IER), UART_READ32(UART_LCR));
    dump_reg(uart, __func__);
}

#if defined(DEBUG)
static u32 UART_READ_EFR(struct mt6516_uart *uart)
{
    u32 base = uart->base;
    u32 efr, lcr = UART_READ32(UART_LCR);

    UART_WRITE32(0xbf, UART_LCR);
    efr = UART_READ32(UART_EFR);
    UART_WRITE32(lcr, UART_LCR);
    return efr;
}
#endif 
static void mt6516_uart_set_flow_ctrl(struct mt6516_uart *uart, int mode)
{
    u32 base = uart->base, old;
    unsigned int tmp = UART_READ32(UART_LCR);

    MSG(CFG, "%s: %04X\n", __func__, UART_READ_EFR(uart));    

    switch (mode) {
    case UART_FC_NONE:
        UART_WRITE32(0x00, UART_ESCAPE_EN);
        UART_SET_BITS(UART_IER_EDSSI, UART_IER);
        UART_WRITE32(0xbf, UART_LCR);        
        old = UART_READ32(UART_EFR);
        old &= ~(UART_EFR_AUTO_RTSCTS|UART_EFR_XON12_XOFF12);
        UART_WRITE32(old, UART_EFR);
        UART_WRITE32(tmp, UART_LCR);        
        mt6516_uart_disable_intrs(uart, UART_IER_XOFFI|UART_IER_RTSI|UART_IER_CTSI);
        break;
    case UART_FC_HW:
        UART_WRITE32(0x00, UART_ESCAPE_EN);
        UART_SET_BITS(UART_MCR_RTS, UART_MCR);
        UART_CLR_BITS(UART_IER_EDSSI, UART_IER);
        UART_WRITE32(0xbf, UART_LCR);        
        /*disable all flow control setting*/
        old = UART_READ32(UART_EFR);               
        old &= ~(UART_EFR_AUTO_RTSCTS | UART_EFR_XON12_XOFF12);
        UART_WRITE32(old, UART_EFR);
        /*enable hw flow control*/
        old = UART_READ32(UART_EFR);
        UART_WRITE32(old | UART_EFR_AUTO_RTSCTS, UART_EFR);
        UART_WRITE32(tmp, UART_LCR);    
        //mt6516_uart_enable_intrs(uart, UART_IER_RTSI|UART_IER_CTSI); /* CHECKME! HW BUG? */
        break;
    case UART_FC_SW:
        if (uart->nport != 3) {
            UART_WRITE32(UART_ESCAPE_CH, UART_ESCAPE_DAT);
            UART_WRITE32(0x01, UART_ESCAPE_EN);
        }
        UART_WRITE32(0xbf, UART_LCR);        
        /*dsiable all flow control setting*/
        old = UART_READ32(UART_EFR);               
        old &= ~(UART_EFR_AUTO_RTSCTS | UART_EFR_XON12_XOFF12);
        UART_WRITE32(old, UART_EFR);
        /*enable sw flow control*/
        if (uart->nport != 3) {
            old = UART_READ32(UART_EFR);
            UART_WRITE32(old | UART_EFR_XON1_XOFF1, UART_EFR);
            UART_WRITE32(START_CHAR(uart->port.info->port.tty), UART_XON1);
            UART_WRITE32(STOP_CHAR(uart->port.info->port.tty), UART_XOFF1);
        }
        UART_WRITE32(tmp, UART_LCR);        
        if (uart->nport != 3)   /*disable SW flow control in console*/
            mt6516_uart_enable_intrs(uart, UART_IER_XOFFI); 
        break;
    }
    uart->fctl_mode = mode;
}

inline static void mt6516_uart_power_up(struct mt6516_uart *uart)
{
    if (!uart || uart->nport >= UART_NR)
        return;

#ifdef POWER_FEATURE
    if (FALSE == hwEnableClock(mt6516_uart_hwconfig[uart->nport].pll_id,"UART"))
        MSG(ERR, "power on fail!!\n");
#else
    {
        unsigned int poweron;
        poweron = 1 << mt6516_uart_hwconfig[uart->nport].power_on_reg_bit_shift;
        UART_SET_BITS(poweron, PDN_CLR0);
    }
#endif     
}

inline static void mt6516_uart_power_down(struct mt6516_uart *uart)
{
    if (!uart || uart->nport >= UART_NR)
        return;

#ifdef POWER_FEATURE
    
    /*FIXME: 
     * (1) Because UART will be shutdown for a while during booting procedure,
     *     temporarily remove the function for fix the issues.
     * (2) The temporary fix will not affect suspend/resume because the power 
     *     will be checked by PM API during suspend/resume.
     * (3) The issue should be fixed in the future.
     */

    if (uart->nport == 3) /*don't power down log port*/
        return;
    if (FALSE == hwDisableClock(mt6516_uart_hwconfig[uart->nport].pll_id,"UART"))
        MSG(ERR, "power off fail!!\n");
#else
    {
        unsigned int poweroff;
        poweroff = 1 << mt6516_uart_hwconfig[uart->nport].power_off_reg_bit_shift;
        UART_SET_BITS(poweroff, PDN_SET0);
    }
#endif    
}

static void mt6516_uart_config(struct mt6516_uart *uart, 
                               int baud, int datalen, int stop, int parity)
{
    u32 base = uart->base;
    unsigned int val = 0;

    switch (datalen)
    {
    case 5:
        val |= UART_WLS_5;
        break;
    case 6:
        val |= UART_WLS_6;
        break;
    case 7:
        val |= UART_WLS_7;
        break;
    case 8:
    default:
        val |= UART_WLS_8;
        break;
    }
    
    if (stop == 2 || (datalen == 5 && stop == 1))
        val |= UART_2_STOP;

    if (parity == 1)
        val |= UART_ODD_PARITY;
    else if (parity == 2)
        val |= UART_EVEN_PARITY;

    UART_WRITE32(val, UART_LCR); 

    mt6516_uart_set_baud(uart, baud);
}

static unsigned int mt6516_uart_read_status(struct mt6516_uart *uart)
{
    u32 base = uart->base;

    uart->line_status = UART_READ32(UART_LSR);
    return uart->line_status;
}

static unsigned int mt6516_uart_read_allow(struct mt6516_uart *uart)
{
    return uart->line_status & UART_LSR_DR;
}

static unsigned int mt6516_uart_write_allow(struct mt6516_uart *uart)
{
    u32 base = uart->base;
    return UART_READ32(UART_LSR) & UART_LSR_THRE;
}

static void mt6516_uart_enable_intrs(struct mt6516_uart *uart, long mask)
{
    u32 base = uart->base;    

    UART_SET_BITS(mask, UART_IER);
    
#if defined(DEBUG) /*the UART_LCR_DLAB bit can't be set, or the DLM will be set instead of IER*/
{
    u32 post, prev = UART_READ32(UART_IER), efr = UART_READ_EFR(uart);
    post = UART_READ32(UART_IER);
    if (UART_READ32(UART_LCR) & UART_LCR_DLAB) {
        printk("ERROR: %02X\n", UART_READ32(UART_LCR));    
        WARN_ON(1);
    }
    MSG(FUC, "%s : %02X |  %02X = %02X, %02X/%02X\n", __FUNCTION__, prev, (unsigned int)mask, post, UART_READ32(UART_LCR), efr);
}
#endif     
}

static void mt6516_uart_disable_intrs(struct mt6516_uart *uart, long mask)
{
    u32 base = uart->base;

    UART_CLR_BITS(mask, UART_IER);

#if defined(DEBUG)  /*the UART_LCR_DLAB bit can't be set, or the DLM will be set instead of IER*/
{
    u32 post, prev = UART_READ32(UART_IER), efr = UART_READ_EFR(uart);    
    post = UART_READ32(UART_IER);
    if (UART_READ32(UART_LCR) & UART_LCR_DLAB) {
        printk("ERROR: %02X\n", UART_READ32(UART_LCR));
        WARN_ON(1);
    }
    MSG(FUC, "%s: %02X &= %02X = %02X, %02X/%02X\n", __FUNCTION__, prev, (unsigned int)mask, post, UART_READ32(UART_LCR), efr);
}
#endif    
}

static unsigned int mt6516_uart_read_byte(struct mt6516_uart *uart)
{
    u32 base = uart->base;
    return UART_READ32(UART_RBR);
}

static void mt6516_uart_write_byte(struct mt6516_uart *uart, unsigned int byte)
{
    u32 base = uart->base;
    UART_WRITE32(byte, UART_THR);
}

static unsigned int mt6516_uart_filter_line_status(struct mt6516_uart *uart)
{
    struct uart_port *port = &uart->port;
    unsigned int status;
    unsigned int lsr = uart->line_status;

    status = UART_LSR_BI|UART_LSR_PE|UART_LSR_FE|UART_LSR_OE;

#ifdef DEBUG
    if ((lsr & UART_LSR_BI) || (lsr & UART_LSR_PE) ||
        (lsr & UART_LSR_FE) || (lsr & UART_LSR_OE)) {
        printk(KERN_ALERT DBG_TAG "LSR: BI=%d, FE=%d, PE=%d, OE=%d, DR=%d\n",
            (lsr & UART_LSR_BI) >> 4, (lsr & UART_LSR_FE) >> 3, 
            (lsr & UART_LSR_PE) >> 2, (lsr & UART_LSR_OE) >> 1, 
             lsr & UART_LSR_DR); 
    }
#endif    
    status &= port->read_status_mask;
    status &= ~port->ignore_status_mask;
    status &= lsr;
    
    return status;
}

static inline bool mt6516_uart_enable_sysrq(void) {
#ifdef DEBUG
    return ((is_meta_mode() == false) && (mt6516_uart_sysrq == 1));
#else
    return (is_meta_mode() == false);
#endif        
}
static void mt6516_uart_rx_chars(struct mt6516_uart *uart)
{
    struct uart_port *port = &uart->port;
    struct tty_struct *tty = uart->port.info->port.tty;
    int max_count = UART_FIFO_SIZE;
    unsigned int data_byte, status;
    unsigned int flag;

    //MSG_FUNC_ENTRY();
    while (max_count-- > 0) {

        /* check status */
        if (!(uart->read_status(uart) & UART_LSR_DR))
            break;
#if 0
        if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
            if (tty->low_latency) {
                /*
                 * If this failed then we will throw away the
                 * bytes but must do so to clear interrupts
                 */
                tty_flip_buffer_push(tty);
            }
        }
#endif
        /* read the byte */
        data_byte = uart->read_byte(uart);
        port->icount.rx++;
        flag = TTY_NORMAL;

        status = mt6516_uart_filter_line_status(uart);
        
        /* error handling routine */
        if (status & UART_LSR_BI) {
            MSG(INFO, "Break interrupt!!\n");
            port->icount.brk++;
            if (uart_handle_break(port))
                continue;
            flag = TTY_BREAK;
        } else if (status & UART_LSR_PE) {
            MSG(INFO, "Parity Error!!\n");
            port->icount.parity++;
            flag = TTY_PARITY;
        } else if (status & UART_LSR_FE) {
            MSG(INFO, "Frame Error!!\n");
            port->icount.frame++;
            flag = TTY_FRAME;
        } else if (status & UART_LSR_OE) {
            MSG(INFO, "Overrun!!\n");
            port->icount.overrun++;
            flag = TTY_OVERRUN;
        }

#ifdef CONFIG_MAGIC_SYSRQ        
        if (mt6516_uart_enable_sysrq())
        {
            if (uart_handle_sysrq_char(port, data_byte))
                continue;

            /* FIXME. Infinity, 20081002, 'BREAK' char to enable sysrq handler { */
        #if defined(CONFIG_MAGIC_SYSRQ) && defined(CONFIG_SERIAL_CORE_CONSLE)
            if (data_byte == 0)
                uart->port.sysrq = 1;
        #endif
            /* FIXME. Infinity, 20081002, 'BREAK' char to enable sysrq handler } */
        }
#endif         

        if (!tty_insert_flip_char(tty, data_byte, flag))
            MSG(ERR, "tty_insert_flip_char: no space");
    }
    tty_flip_buffer_push(tty);
    MSG(FUC, "%s (%2d)\n", __FUNCTION__, UART_FIFO_SIZE - max_count - 1);
}

static void mt6516_uart_tx_chars(struct mt6516_uart *uart)
{
    /* Notice:
     * The function is called by uart_start, which is protected by spin lock,
     * Hence, no spin-lock is required in the functions
     */
    
    struct uart_port *port = &uart->port;
    struct circ_buf *xmit = &port->info->xmit;
    int count;

    /* deal with x_char first */
    if (unlikely(port->x_char)) {
        MSG(INFO, "detect x_char!!\n");
        uart->write_byte(uart, port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    /* stop tx if circular buffer is empty or this port is stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) 
    {
        struct tty_struct *tty = port->info->port.tty;
        if (!uart_circ_empty(xmit))
            printk(KERN_ALERT DBG_TAG "\t\tstopped: empty: %d %d %d\n", uart_circ_empty(xmit), tty->stopped, tty->hw_stopped);
        mt6516_uart_stop_tx(port);
        return;
    }

    count = port->fifosize - 1;

    do {
        if (uart_circ_empty(xmit))
            break;
        if (!uart->write_allow(uart)) {
            mt6516_uart_enable_intrs(uart, UART_IER_ETBEI);
            break;
        }
        uart->write_byte(uart, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;

    } while (--count > 0);

    MSG(INFO, "TX %d chars\n", port->fifosize - 1 - count);

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

    if (uart_circ_empty(xmit))
        mt6516_uart_stop_tx(port);
}

static void mt6516_uart_get_modem_status(struct mt6516_uart *uart)
{
    u32 base = uart->base;
    struct uart_port *port = &uart->port;
    unsigned int status, delta;

    status  = UART_READ32(UART_MSR);    
    status &= UART_MSR_DSR | UART_MSR_CTS | UART_MSR_DCD | UART_MSR_RI;

    MSG(INFO, "MSR: DCD(%d), RI(%d), DSR(%d), CTS(%d)\n",
        status & UART_MSR_DCD ? 1 : 0, 
        status & UART_MSR_RI ? 1 : 0, 
        status & UART_MSR_DSR ? 1 : 0,
        status & UART_MSR_CTS ? 1 : 0);

    delta = status ^ uart->old_status;

    if (!delta)
        return;

    if (uart->ms_enable) {
        if (delta & UART_MSR_DCD)
            uart_handle_dcd_change(port, status & UART_MSR_DCD);
        if (delta & UART_MSR_CTS)
            uart_handle_cts_change(port, status & UART_MSR_CTS);
        if (delta & UART_MSR_DSR)
            port->icount.dsr++;
        if (delta & UART_MSR_RI)
            port->icount.rng++;
    }

    uart->old_status = status;
}

static void mt6516_uart_rx_handler(struct mt6516_uart *uart, int timeout)
{
    if (uart->rx_mode == UART_NON_DMA) {
        mt6516_uart_rx_chars(uart);
    } else if (uart->rx_mode == UART_RX_VFIFO_DMA) {
#if defined(ENABLE_VFIFO_LEVEL_SENSITIVE)
        if (timeout)
#else
        if (timeout && uart->read_allow(uart))
#endif        
            mt6516_uart_dma_vfifo_rx_tasklet((unsigned long)uart);
    }
}

static void mt6516_uart_tx_handler(struct mt6516_uart *uart)
{
    if (uart->tx_mode == UART_NON_DMA) {
        mt6516_uart_tx_chars(uart);
    } else if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        tasklet_schedule(&uart->dma_tx.tasklet);
    }
}

static __tcmfunc irqreturn_t mt6516_uart_irq(int irq, void *dev_id)
{
    unsigned int intrs, timeout = 0;
    struct mt6516_uart *uart = (struct mt6516_uart *)dev_id;
    u32 base = uart->base;    

    intrs = UART_READ32(UART_IIR);

#ifdef DEBUG
    {
        static const char *fifo[] = {"No FIFO", "Unstable FIFO", 
                                     "Unknown", "FIFO Enabled"};
        static const char *intrrupt[] = {"Modem Status Chg", "Tx Buffer Empty",
                                         "Rx Data Received", "BI, FE, PE, or OE",
                                         "?", "?", "Rx Data Timeout", "?"
                                         "SW Flow Control", "?", "?", "?", "?",
                                         "?", "?", "?", "HW Flow Control"};
        UART_IIR_REG *iir = (UART_IIR_REG *)&intrs;
        if (iir->NINT)
            MSG(INT, "No interrupt (%s)\n", fifo[iir->FIFOE]);        
        else
            MSG(INT, "%s (%s)\n", intrrupt[iir->ID], fifo[iir->FIFOE]);
    }
#endif
    intrs &= UART_IIR_INT_MASK;
    
    if (intrs == UART_IIR_NO_INT_PENDING)
        return IRQ_HANDLED;

    if (intrs == UART_IIR_RLS) {
        /* BE, FE, PE, or OE occurs */
    } else if (intrs == UART_IIR_CTI) {
        timeout = 1;
    } else if (intrs == UART_IIR_MS) {
        mt6516_uart_get_modem_status(uart);
    } else if (intrs == UART_IIR_SW_FLOW_CTRL) {
        /* XOFF is received */
    } else if (intrs == UART_IIR_HW_FLOW_CTRL) {
        /* CTS or RTS is in rising edge */
    }

    mt6516_uart_rx_handler(uart, timeout);
    
    if (uart->write_allow(uart))
        mt6516_uart_tx_handler(uart);
    else /*sometimes, TX empty interrupt is enabled even when TX data available*/
        mt6516_uart_stop_tx(&uart->port);

    return IRQ_HANDLED;
}

/* test whether the transmitter fifo and shifter for the port is empty. */
static unsigned int mt6516_uart_tx_empty(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;

    MSG_FUNC_ENTRY();
    
    if (uart->tx_mode == UART_TX_VFIFO_DMA)
        return mt6516_uart_vfifo_is_empty(uart->dma_tx.vfifo) ? TIOCSER_TEMT : 0;
    else
        return uart->write_allow(uart) ? TIOCSER_TEMT : 0;
}

/* set the modem control lines. */
static void mt6516_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    u32 base = uart->base;    
    unsigned int val;

    val = UART_READ32(UART_MCR);

    if (mctrl & TIOCM_DTR)
        val |= UART_MCR_DTR;
    else
        val &= ~UART_MCR_DTR;

    if (mctrl & TIOCM_RTS)
        val |= UART_MCR_RTS;
    else
        val &= ~UART_MCR_RTS;

    if (mctrl & TIOCM_OUT1)
        val |= UART_MCR_OUT1;
    else
        val &= ~UART_MCR_OUT1;

    if (mctrl & TIOCM_OUT2)
        val |= UART_MCR_OUT2;
    else
        val &= ~UART_MCR_OUT2;

    if (mctrl & TIOCM_LOOP)
        val |= UART_MCR_LOOP;
    else
        val &= ~UART_MCR_LOOP;

    UART_WRITE32(val, UART_MCR);

    MSG(CFG, "MCR: DTR(%d), RTS(%d), OUT1(%d), OUT2(%d), LOOP(%d)\n",
        val & UART_MCR_DTR ? 1 : 0, 
        val & UART_MCR_RTS ? 1 : 0, 
        val & UART_MCR_OUT1 ? 1 : 0,
        val & UART_MCR_OUT2 ? 1 : 0, 
        val & UART_MCR_LOOP ? 1 : 0);
}

/* return the current state of modem contrl inputs */
static unsigned int mt6516_uart_get_mctrl(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    u32 base = uart->base;    
    unsigned int status;
    unsigned int result = 0;

    status = UART_READ32(UART_MSR);

    MSG(INFO, "MSR: DCD(%d), RI(%d), DSR(%d), CTS(%d)\n",
        status & UART_MSR_DCD ? 1 : 0, 
        status & UART_MSR_RI ? 1 : 0, 
        status & UART_MSR_DSR ? 1 : 0,
        status & UART_MSR_CTS ? 1 : 0);

    if (status & UART_MSR_DCD)   
        result |= TIOCM_CAR;    /* DCD. (data carrier detect) */
    if (status & UART_MSR_RI)
        result |= TIOCM_RI;
    if (status & UART_MSR_DSR)
        result |= TIOCM_DSR;
    if (status & UART_MSR_CTS)
        result |= TIOCM_CTS;

    status = UART_READ32(UART_MCR);

    MSG(INFO, "MSR: OUT1(%d), OUT2(%d), LOOP(%d)\n",
        status & UART_MCR_OUT1 ? 1 : 0, 
        status & UART_MCR_OUT2 ? 1 : 0, 
        status & UART_MCR_LOOP ? 1 : 0);

    if (status & UART_MCR_OUT2)
        result |= TIOCM_OUT2;
    if (status & UART_MCR_OUT1)
        result |= TIOCM_OUT1;
    if (status & UART_MCR_LOOP)
        result |= TIOCM_LOOP;

    return result;
}

/* FIXME */
static void mt6516_uart_stop_tx(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;

    MSG_FUNC_ENTRY();
    if (uart->tx_mode == UART_TX_DMA) {
        mt6516_uart_dma_stop(uart, &uart->dma_tx);
    } else {
        /* disable tx interrupt */
        mt6516_uart_disable_intrs(uart, UART_IER_ETBEI);
    }
    uart->tx_stop = 1;
}

/* FIXME */
static void mt6516_uart_start_tx(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    struct mt6516_uart_dma *dma_tx = &uart->dma_tx;
    struct circ_buf    *xmit = &port->info->xmit;    
    unsigned long size;
    
    size = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);

    if (!size)
        return;

    uart->tx_stop = 0;

    if (uart->tx_mode == UART_TX_DMA) {
        if (atomic_read(&dma_tx->free)) {
            mt6516_uart_dma_map(uart, &uart->sg_tx, &xmit->buf[xmit->tail], 
                                size, DMA_TO_DEVICE);
            mt6516_uart_dma_setup(uart, dma_tx, &uart->sg_tx);
            if (mt6516_uart_dma_start(uart, dma_tx))
                MSG(ERR, "mt6516_uart_dma_start fails\n");
        } else if (uart->pending_tx_reqs < UART_MAX_TX_PENDING) {
            uart->pending_tx_reqs++;
        }
    } else if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        if (uart->write_allow(uart))
            mt6516_uart_dma_vfifo_tx_tasklet((unsigned long)uart);
            //tasklet_schedule(&dma_tx->tasklet);
    }
    else {
        if (uart->write_allow(uart))
            mt6516_uart_tx_chars(uart);
    }
}

/* FIXME */
static void mt6516_uart_stop_rx(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;

    MSG_FUNC_ENTRY();
    if (uart->rx_mode == UART_NON_DMA) {
        mt6516_uart_disable_intrs(uart, UART_IER_ERBFI);
    } else {
        mt6516_uart_dma_stop(uart, &uart->dma_rx);
    }
    uart->rx_stop = 1;
}

/* FIXME */
static void mt6516_uart_send_xchar(struct uart_port *port, char ch)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    unsigned long flags;

    MSG_FUNC_ENTRY();
    
    if (uart->tx_stop)
        return;

    spin_lock_irqsave(&port->lock, flags);
    while (!uart->write_allow(uart));
    uart->write_byte(uart, (unsigned char)ch);
    port->icount.tx++; 
    spin_unlock_irqrestore(&port->lock, flags);   
    return;
}

/* enable the modem status interrupts */
static void mt6516_uart_enable_ms(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;

    MSG_FUNC_ENTRY();
    uart->ms_enable = 1;
}

/* control the transmission of a break signal */
static void mt6516_uart_break_ctl(struct uart_port *port, int break_state)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    u32 base = uart->base;

    unsigned long flags;

    MSG_FUNC_ENTRY();
    
    spin_lock_irqsave(&port->lock, flags);
    
    if (break_state)
        UART_SET_BITS(UART_LCR_BREAK, UART_LCR);
    else
        UART_CLR_BITS(UART_LCR_BREAK, UART_LCR);

    spin_unlock_irqrestore(&port->lock, flags);
}

/* grab any interrupt resources and initialize any low level driver state */
static int mt6516_uart_startup(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    int ret;
    long mask = UART_IER_HW_NORMALINTS;
    
    MSG_FUNC_ENTRY();

    /*Reset default flag when the uart starts up, or the previous setting,
     *such as custom baudrate will be still applied even it is ever closed 
     */
    uart->port.flags = UPF_BOOT_AUTOCONF;
    uart->port.custom_divisor = 1;
    uart->fctl_mode     = UART_FC_NONE;
    
    /* power on the uart port */
    mt6516_uart_power_up(uart);

    /* disable interrupts */
    mt6516_uart_disable_intrs(uart, UART_IER_ALL_INTS);

    /* allocate irq line */
    ret = request_irq(port->irq, mt6516_uart_irq, 0, DRV_NAME, uart);
    if (ret)
        return ret;

    /* allocate vfifo */
    if (uart->rx_mode == UART_RX_VFIFO_DMA) {
        uart->rx_vfifo = mt6516_uart_vfifo_alloc(uart, UART_RX_VFIFO, 
                                                 UART_VFIFO_SIZE);       
    }
    if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        uart->tx_vfifo = mt6516_uart_vfifo_alloc(uart, UART_TX_VFIFO, 
                                                 UART_VFIFO_SIZE);
    }
    
    /* allocate dma channels */
    ret = mt6516_uart_dma_alloc(uart, &uart->dma_rx, uart->rx_mode, uart->rx_vfifo);
    if (ret) {
        uart->rx_mode = UART_NON_DMA;
    }
    ret = mt6516_uart_dma_alloc(uart, &uart->dma_tx, uart->tx_mode, uart->tx_vfifo);
    if (ret) {
        uart->tx_mode = UART_NON_DMA;
    }
    
    /* start vfifo dma */
    if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        uart->write_allow = mt6516_uart_vfifo_write_allow;
        uart->write_byte  = mt6516_uart_vfifo_write_byte;
        mt6516_uart_vfifo_enable(uart);
        mt6516_uart_vfifo_set_trig(uart, uart->tx_vfifo, UART_VFIFO_SIZE / 4 * 3);
        mt6516_uart_dma_setup(uart, &uart->dma_tx, NULL);
        if (mt6516_uart_dma_start(uart, &uart->dma_tx))
            MSG(ERR,"mt6516_uart_dma_start fails\n");
    }
    if (uart->rx_mode == UART_RX_VFIFO_DMA) {
        uart->read_allow = mt6516_uart_vfifo_read_allow;
        uart->read_byte  = mt6516_uart_vfifo_read_byte;  
        mt6516_uart_vfifo_enable(uart);
        //mt6516_uart_vfifo_set_trig(uart, uart->rx_vfifo, 1);
        mt6516_uart_vfifo_set_trig(uart, uart->rx_vfifo, UART_VFIFO_SIZE / 4 * 3);
        mt6516_uart_dma_setup(uart, &uart->dma_rx, NULL);
        if (mt6516_uart_dma_start(uart, &uart->dma_rx))
            MSG(ERR,"mt6516_uart_dma_start fails\n");        

        init_timer(&uart->rx_vfifo->timer);
        uart->rx_vfifo->timer.expires  = jiffies + 150/(1000/HZ);
        uart->rx_vfifo->timer.function = mt6516_uart_dma_vfifo_timeout;
        uart->rx_vfifo->timer.data     = (unsigned long)&uart->dma_rx;
        add_timer(&uart->rx_vfifo->timer);
    }

    uart->tx_stop = 0;
    uart->rx_stop = 0;

    /* After applying UART as Level-Triggered IRQ, the function must be called or
     * the interrupt will be incorrect activated. 
     */
    mt6516_uart_fifo_set_trig(uart, uart->tx_trig_level, uart->rx_trig_level);
    
    /* enable interrupts */
    mt6516_uart_enable_intrs(uart, mask);
    return 0;
}

static void mt6516_uart_shutdown(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;

    MSG_FUNC_ENTRY();

    /* disable interrupts */
    mt6516_uart_disable_intrs(uart, UART_IER_ALL_INTS);

    /* free dma channels and vfifo */
    mt6516_uart_dma_free(uart, &uart->dma_rx);
    mt6516_uart_dma_free(uart, &uart->dma_tx);
    mt6516_uart_vfifo_free(uart, uart->rx_vfifo);    
    mt6516_uart_vfifo_free(uart, uart->tx_vfifo);
    mt6516_uart_vfifo_disable(uart);
    mt6516_uart_fifo_flush(uart);

    /* release irq line */
    free_irq(port->irq, port);

    /* power off the uart port */
    mt6516_uart_power_down(uart);
}

static void mt6516_uart_set_termios(struct uart_port *port,
                                   struct ktermios *termios, struct ktermios *old)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    unsigned long flags;
    unsigned int baud;
    int datalen, mode;
    int parity = 0;
    int stopbit = 1;

    MSG_FUNC_ENTRY();

    /* datalen : default 8bits */
    switch (termios->c_cflag & CSIZE) {
    case CS5:
        datalen = 5;
        break;
    case CS6:
        datalen = 6;
        break;
    case CS7:
        datalen = 7;
        break;
    case CS8:
    default:
        datalen = 8;
        break;
    }
    
    /* stopbit : default 1 */
    if (termios->c_cflag & CSTOPB)
        stopbit = 2;

    /* parity : default none */
    if (termios->c_cflag & PARENB) {
        if (termios->c_cflag & PARODD)
            parity = 1; /* odd */
        else
            parity = 2; /* even */
    }

    spin_lock_irqsave(&port->lock, flags);
    
    /* read status mask */
    port->read_status_mask = 0;    
    if (termios->c_iflag & INPCK) {
        /* frame error, parity error */
        port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
    }
    if (termios->c_iflag & (BRKINT | PARMRK)) {
        /* break error */
        port->read_status_mask |= UART_LSR_BI;
    }
    
    port->ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR) {
        /* ignore parity and framing errors */
        port->ignore_status_mask |= UART_LSR_FE | UART_LSR_PE;
    }
    if (termios->c_iflag & IGNBRK) {
        /* ignore break errors. */
        port->ignore_status_mask |= UART_LSR_BI;
        if (termios->c_iflag & IGNPAR) {
            /* ignore overrun errors */
            port->ignore_status_mask |= UART_LSR_OE;
        }
    }

    /* ignore all characters if CREAD is not set */
    if ((termios->c_cflag & CREAD) == 0) {
        uart->ignore_rx = 1;
    }

    /* update per port timeout */
    baud = uart_get_baud_rate(port, termios, old, 0, uart->sysclk); /*when dividor is 1, baudrate = clock*/    
    uart_update_timeout(port, termios->c_cflag, baud);    
    mt6516_uart_config(uart, baud, datalen, stopbit, parity);

    /* setup fifo trigger level */
    mt6516_uart_fifo_set_trig(uart, uart->tx_trig_level, uart->rx_trig_level);

    /* setup hw flow control: only port 0 ~2 support hw rts/cts */
    MSG(CFG, "c_lflag=%02X, c_iflag=%02X, c_oflag=%02X\n", termios->c_lflag, termios->c_iflag, termios->c_oflag);
    if (HW_FLOW_CTRL_PORT(uart) && (termios->c_cflag & CRTSCTS) && (!(termios->c_iflag&0x80000000))) {
        MSG(CFG, "Hardware Flow Control\n");
        mode = UART_FC_HW;
    } else if (HW_FLOW_CTRL_PORT(uart) && (termios->c_cflag & CRTSCTS) && (termios->c_iflag&0x80000000)) {
        MSG(CFG, "MTK Software Flow Control\n");
        mode = UART_FC_SW;
    } else if (termios->c_iflag & (IXON | IXOFF | IXANY)) {
        MSG(CFG, "Linux default SW Flow Control\n");
        mode = UART_FC_NONE;    
    } else {
        MSG(CFG, "No Flow Control\n");
        mode = UART_FC_NONE;
    }
    mt6516_uart_set_flow_ctrl(uart, mode);

    /* determine if port should enable modem status interrupt */
    if (UART_ENABLE_MS(port, termios->c_cflag)) 
        uart->ms_enable = 1;
    else
        uart->ms_enable = 0;

    spin_unlock_irqrestore(&port->lock, flags);
}

/* perform any power management related activities on the port */
static void mt6516_uart_power_mgnt(struct uart_port *port, unsigned int state,
                                   unsigned int oldstate)
{
    return; /* TODO */
}

static int mt6516_uart_set_wake(struct uart_port *port, unsigned int state)
{
    return 0; /* Not used in current kernel version */
}

/* return a pointer to a string constant describing the port */
static const char *mt6516_uart_type(struct uart_port *port)
{
    return "MT6516 UART";
}

/* release any memory and io region resources currently in used by the port */
static void mt6516_uart_release_port(struct uart_port *port)
{
    return;
}

/* request any memory and io region resources required by the port */
static int mt6516_uart_request_port(struct uart_port *port)
{
    return 0;
}

static void mt6516_uart_config_port(struct uart_port *port, int flags)
{ 
    if (flags & UART_CONFIG_TYPE) {
        if (mt6516_uart_request_port(port))
            printk(KERN_ERR DBG_TAG "mt6516_uart_request_port fail\n");
        port->type = PORT_MT6516;
    }
}

/* verify if the new serial information contained within 'ser' is suitable */ 
static int mt6516_uart_verify_port(struct uart_port *port,
    struct serial_struct *ser)
{
    int ret = 0;
    if (ser->type != PORT_UNKNOWN && ser->type != PORT_MT6516)
        ret = -EINVAL;
    if (ser->irq != port->irq)
        ret = -EINVAL;
    if (ser->baud_base < 110)
        ret = -EINVAL;
    return ret;
}

/* perform any port specific IOCTLs */
static int mt6516_uart_ioctl(struct uart_port *port, unsigned int cmd, 
                             unsigned long arg)
{
    return -ENOIOCTLCMD;
}

#ifdef CONFIG_CONSOLE_POLL
static int mt6516_uart_get_poll_char(struct uart_port *port)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    
    while (!(uart->read_status(uart) & UART_LSR_DR))
        cpu_relax();
    /* read the byte */
    return uart->read_byte(uart);
}
static void mt6516_uart_put_poll_char(struct uart_port *port, unsigned char c)
{
    struct mt6516_uart *uart = (struct mt6516_uart *)port;
    while (!uart->write_allow(uart)) 
        barrier();
    uart->write_byte(uart, c);    
}
#endif 
static struct uart_ops mt6516_uart_ops =
{
    .tx_empty       = mt6516_uart_tx_empty,
    .set_mctrl      = mt6516_uart_set_mctrl,
    .get_mctrl      = mt6516_uart_get_mctrl,
    .stop_tx        = mt6516_uart_stop_tx,
    .start_tx       = mt6516_uart_start_tx,
    .stop_rx        = mt6516_uart_stop_rx,
    .send_xchar     = mt6516_uart_send_xchar,
    .enable_ms      = mt6516_uart_enable_ms,
    .break_ctl      = mt6516_uart_break_ctl,
    .startup        = mt6516_uart_startup,
    .shutdown       = mt6516_uart_shutdown,
    .set_termios    = mt6516_uart_set_termios,
    .pm             = mt6516_uart_power_mgnt,
    .set_wake       = mt6516_uart_set_wake,
    .type           = mt6516_uart_type,
    .release_port   = mt6516_uart_release_port,
    .request_port   = mt6516_uart_request_port,
    .config_port    = mt6516_uart_config_port,
    .verify_port    = mt6516_uart_verify_port,
    .ioctl          = mt6516_uart_ioctl,
#ifdef CONFIG_CONSOLE_POLL
    .poll_get_char  = mt6516_uart_get_poll_char,
    .poll_put_char  = mt6516_uart_put_poll_char,
#endif    
};

static struct uart_driver mt6516_uart_drv =
{
    .owner          = THIS_MODULE,
    .driver_name    = DRV_NAME,
    .dev_name       = "ttyMT",
    .major          = UART_MAJOR,
    .minor          = UART_MINOR,
    .nr             = UART_NR,
#if defined(CONFIG_SERIAL_MT6516_CONSOLE) && !defined(CONFIG_SERIAL_MT6516_MODEM_TEST)
    .cons           = &mt6516_uart_console,
#endif
};

static int mt6516_uart_probe(struct platform_device *pdev)
{
    struct mt6516_uart     *uart = &mt6516_uarts[pdev->id];
    int err;

    uart->port.dev = &pdev->dev;
    err = uart_add_one_port(&mt6516_uart_drv, &uart->port);
    if (!err)
        platform_set_drvdata(pdev, uart);

    return err;
}

static int mt6516_uart_remove(struct platform_device *pdev)
{
    struct mt6516_uart *uart = platform_get_drvdata(pdev);

    platform_set_drvdata(pdev, NULL);

    if (uart)
        return uart_remove_one_port(&mt6516_uart_drv, &uart->port);
    
    return 0;
}

#ifdef CONFIG_PM 
static int mt6516_uart_suspend(struct platform_device *pdev, pm_message_t state)
{
    int ret = 0;
    struct mt6516_uart *uart = platform_get_drvdata(pdev);

    /*temporary solution for BT: no suspend/resume in UART3*/
    if (uart && (uart->nport != 2)) {
        ret = uart_suspend_port(&mt6516_uart_drv, &uart->port);
        MSG(INFO, "Suspend(%d)!\n", ret);
    }

    return ret;
}

static int mt6516_uart_resume(struct platform_device *pdev)
{
    int ret = 0;
    struct mt6516_uart *uart = platform_get_drvdata(pdev);

    /*temporary solution for BT: no suspend/resume in UART3*/
    if (uart && (uart->nport != 2)) {
        ret = uart_resume_port(&mt6516_uart_drv, &uart->port);
        MSG(INFO, "Resume(%d)!\n", ret);
    }
    return 0;
}
#endif

static void mt6516_uart_init_ports(void)
{
    int i;
    struct mt6516_uart *uart;
    unsigned long base;
#if defined(CONFIG_SERIAL_MT6516_MODEM_TEST)
    #define HW_MISC     (CONFIG_BASE+0x0020)
    unsigned char mask[UART_NR] = { 1 << 3, 1 << 4, 1 << 5, 1 << 6};           
    unsigned char modem_uart[UART_NR] = {1, 0, 0, 1};
#endif 

    for (i = 0; i < UART_NR; i++) {
        uart = &mt6516_uarts[i];        
        base = mt6516_uart_hwconfig[i].uart_base;        
        uart->port.iotype   = UPIO_MEM;
        uart->port.mapbase  = base - IO_OFFSET;   /* for ioremap */
        uart->port.membase  = (unsigned char __iomem *)base;
        uart->port.irq      = mt6516_uart_hwconfig[i].irq_num;
        uart->port.fifosize = UART_FIFO_SIZE;
        uart->port.ops      = &mt6516_uart_ops;
        uart->port.flags    = UPF_BOOT_AUTOCONF;
        uart->port.line     = i;
        uart->port.uartclk  = UART_SYSCLK;
        spin_lock_init(&uart->port.lock);
        uart->base          = base;
        uart->auto_baud     = CFG_UART_AUTOBAUD;
        uart->nport         = i;
        uart->sysclk        = UART_SYSCLK; /* FIXME */
        uart->dma_mode      = mt6516_uart_default_settings[i].dma_mode;
        uart->tx_mode       = mt6516_uart_default_settings[i].tx_mode;
        uart->rx_mode       = mt6516_uart_default_settings[i].rx_mode;
        uart->tx_trig_level = mt6516_uart_default_settings[i].tx_trig_level;
        uart->rx_trig_level = mt6516_uart_default_settings[i].rx_trig_level;
        uart->write_allow   = mt6516_uart_write_allow;
        uart->read_allow    = mt6516_uart_read_allow;
        uart->write_byte    = mt6516_uart_write_byte;
        uart->read_byte     = mt6516_uart_read_byte;
        uart->read_status   = mt6516_uart_read_status;
#ifdef DEBUG
        uart->debug         = &mt6516_uart_debug[i];
#endif
#if defined(CONFIG_SERIAL_MT6516_MODEM_TEST)
        if (modem_uart[i]) {            
            u32 dat = UART_READ32(HW_MISC);
            mt6516_uart_power_up(uart); //power up
            UART_WRITE32(dat | mask[i], HW_MISC);
            continue;
        }
#endif 
        //mt6516_uart_power_down(uart);
        //mt6516_uart_config_pinmux(uart);
        //mt6516_uart_power_up(uart);
        mt6516_uart_disable_intrs(uart, UART_IER_ALL_INTS);
        MT6516_IRQSensitivity(mt6516_uart_hwconfig[i].irq_num, mt6516_uart_hwconfig[i].irq_sensitive);
        mt6516_uart_fifo_init(uart);
        mt6516_uart_set_mode(uart, uart->dma_mode);
    }
#if defined(CONFIG_SERIAL_MT6516_MODEM_TEST)
    /*NOTICE: for enabling modem test, UART4 needs to be disabled. Howerver, if CONFIG_SERIAL_MT6516_CONSOLE
              is defined, resume will fail. Since the root cause is not clear, only disable the console-related
              function.*/
    printk("HW_MISC: 0x%08X\n", UART_READ32(HW_MISC));
    {
        extern void mt_gpio_dump(GPIO_REGS *regs);
        mt_gpio_dump(NULL);
    }
    printk("dump done\n");

    printk("%d %d %d %d %d %d\n", mt_get_gpio_dir(GPIO70), mt_get_gpio_pull_enable(GPIO70), mt_get_gpio_pull_select(GPIO70), 
                                  mt_get_gpio_inversion(GPIO70), mt_get_gpio_out(GPIO70), mt_get_gpio_mode(GPIO70));
    printk("%d %d %d %d %d %d\n", mt_get_gpio_dir(GPIO69), mt_get_gpio_pull_enable(GPIO69), mt_get_gpio_pull_select(GPIO69), 
                                  mt_get_gpio_inversion(GPIO69), mt_get_gpio_out(GPIO69), mt_get_gpio_mode(GPIO69));

#endif 
}
static struct platform_driver mt6516_uart_dev_drv =
{
    .probe   = mt6516_uart_probe,
    .remove  = mt6516_uart_remove,
#ifdef CONFIG_PM    
    .suspend = mt6516_uart_suspend,
    .resume  = mt6516_uart_resume,
#endif    
    .driver = {
        .name    = DRV_NAME,
        .owner   = THIS_MODULE,    
    }
};


static int __init mt6516_uart_init(void)
{
    int ret = 0;

    printk(KERN_ALERT DBG_TAG "is_meta_mode() = %d\n", is_meta_mode());
#ifndef CONFIG_SERIAL_MT6516_CONSOLE
    mt6516_uart_init_ports();
#endif

#if defined(DEBUG)
    mt6516_uart_sysfs();
#endif 

    ret = uart_register_driver(&mt6516_uart_drv);

    if (ret) return ret;
    
    ret = platform_driver_register(&mt6516_uart_dev_drv);

    if (ret) {
        uart_unregister_driver(&mt6516_uart_drv);
        return ret;
    }

    return ret;
}

static void __exit mt6516_uart_exit(void)
{
    platform_driver_unregister(&mt6516_uart_dev_drv);
    uart_unregister_driver(&mt6516_uart_drv);
}

module_init(mt6516_uart_init);
module_exit(mt6516_uart_exit);

MODULE_AUTHOR("MingHsien Hsieh <minghsien.hsieh@mediatek.com>");
MODULE_DESCRIPTION("MT6516 Serial Port Driver $Revision$");
MODULE_LICENSE("GPL");
