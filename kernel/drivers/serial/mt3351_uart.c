

//#define DEBUG
//#define VFIFO_DEBUG
#include <linux/autoconf.h>

#if defined(CONFIG_SERIAL_MT3351_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
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


#include <asm/io.h>
#include <asm/irq.h>
#include <asm/scatterlist.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/mt3351_typedefs.h>
#include <mach/mt3351_devs.h>
#include <mach/mt3351_gpio.h>
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_pmu_sw.h>
#include "mt3351_uart.h"

#define CFG_UART_AUTOBAUD           0

#define UART_VFIFO_SIZE             2048
#define UART_VFIFO_ALERT_LEN        0x3f

#define UART_MAX_TX_PENDING         1024

#define UART_MAJOR                  204
#define UART_MINOR                  209
#define UART_NR                     CFG_UART_PORTS


#define MT3351_SYSCLK_65            65000000
#define MT3351_SYSCLK_58_5          58500000
#define MT3351_SYSCLK_52            52000000
#define MT3351_SYSCLK_26            26000000
#define MT3351_SYSCLK_13            13000000

#if defined(CONFIG_MT3351_CPU_416MHZ_MCU_104MHZ) || \
    defined(CONFIG_MT3351_CPU_208MHZ_MCU_104MHZ)
#define UART_SYSCLK                 MT3351_SYSCLK_52
#elif defined(CONFIG_MT3351_CPU_468MHZ_MCU_117MHZ)
#define UART_SYSCLK                 MT3351_SYSCLK_58_5
#else
#define UART_SYSCLK                 MT3351_SYSCLK_26
#endif

#define DRV_NAME            "mt3351-uart"

#ifdef DEBUG
/* Debug message event */
#define DBG_EVT_NONE		0x00000000	/* No event */
#define DBG_EVT_DMA			0x00000001	/* DMA related event */
#define DBG_EVT_INT			0x00000002	/* UART INT event */
#define DBG_EVT_CFG			0x00000004	/* UART CFG event */
#define DBG_EVT_FUC			0x00000008	/* Function event */
#define DBG_EVT_INFO		0x00000010	/* information event */
#define DBG_EVT_ERR         0x00000020  /* Error event */
#define DBG_EVT_ALL			0xffffffff

//#define DBG_EVT_MASK        (DBG_EVT_FUC|DBG_EVT_INT|DBG_EVT_DMA)
//#define DBG_EVT_MASK        (DBG_EVT_FUC|DBG_EVT_DMA|DBG_EVT_INFO)
//#define DBG_EVT_MASK        (DBG_EVT_CFG|DBG_EVT_DMA|DBG_EVT_INFO)
//#define DBG_EVT_MASK        (DBG_EVT_INT|DBG_EVT_DMA)
//#define DBG_EVT_MASK        (DBG_EVT_DMA|DBG_EVT_INFO|DBG_EVT_ERR|DBG_EVT_FUC)
//#define DBG_EVT_MASK        (DBG_EVT_NONE)
#define DBG_EVT_MASK          (DBG_EVT_INFO|DBG_EVT_DMA)
//#define DBG_EVT_MASK        (DBG_EVT_ALL & ~DBG_EVT_INT)
//#define DBG_EVT_MASK        (DBG_EVT_ALL)

static unsigned long mt3351_uart_evt_mask[] = {
    DBG_EVT_NONE,
    DBG_EVT_MASK,
    DBG_EVT_MASK,
    DBG_EVT_NONE
};

#define MSG(evt, fmt, args...) \
do {	\
	if ((DBG_EVT_##evt) & mt3351_uart_evt_mask[uart->nport]) { \
	    if (DBG_EVT_##evt & DBG_EVT_ERR) \
	        printk("[UART%d]:<%s> in LINE %d(%s) -> " fmt , \
	               uart->nport, #evt, __LINE__, __FILE__, ##args); \
        else \
    	    printk("[UART%d]:<%s> " fmt , uart->nport, #evt, ##args); \
	} \
} while(0)
#define MSG_FUNC_ENTRY(f)	MSG(FUC, "%s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)	   do{}while(0)
#endif

#define HW_FLOW_CTRL_PORT(uart)     (((struct mt3351_uart*)uart)->nport < UART_PORT3)

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
    UART_PORT4
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

struct mt3351_uart_setting {
    int tx_mode;
    int rx_mode;
    int dma_mode;    
    int tx_trig_level;
    int rx_trig_level;
};

static struct mt3351_uart_setting mt3351_uart_default_settings[] = {
    //{UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_14B_TRI , UART_FCR_RXFIFO_1B_TRI},  /* ok */
    {UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    //{UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_1, UART_FCR_TXFIFO_14B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    
    //{UART_TX_DMA      , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_1B_TRI}, /*ok */
    //{UART_TX_DMA      , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_12B_TRI}, /*ok */
    //{UART_TX_DMA      , UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_8B_TRI ,  UART_FCR_RXFIFO_12B_TRI}, /* fail */
    
    //{UART_TX_VFIFO_DMA, UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_1B_TRI},	 /* fail */
    //{UART_NON_DMA     , UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* tx abnormal */
    //{UART_NON_DMA     , UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_8B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* tx abnormal */
    //{UART_NON_DMA     , UART_RX_VFIFO_DMA, UART_DMA_MODE_1, UART_FCR_TXFIFO_1B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* tx abnormal */

    {UART_TX_VFIFO_DMA, UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_8B_TRI , UART_FCR_RXFIFO_12B_TRI}, /* ok */
    //{UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_12B_TRI},
    {UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_12B_TRI},
    //{UART_TX_VFIFO_DMA, UART_RX_VFIFO_DMA, UART_DMA_MODE_0, UART_FCR_TXFIFO_4B_TRI , UART_FCR_RXFIFO_12B_TRI},
    {UART_NON_DMA     , UART_NON_DMA     , UART_DMA_MODE_0, UART_FCR_TXFIFO_14B_TRI, UART_FCR_RXFIFO_12B_TRI},
};

extern BOOL hwEnableClock(MT3351_CLOCK clockId);
extern BOOL hwDisableClock(MT3351_CLOCK clockId);

#ifdef DEBUG
static struct mt3351_uart_regs mt3351_uart_debug[] =
{ 
    INI_REGS(UART1_BASE),
    INI_REGS(UART2_BASE),
    INI_REGS(UART3_BASE),
    INI_REGS(UART4_BASE),
    INI_REGS(UART5_BASE),
};

static struct mt3351_dma_vfifo_reg mt3351_uart_vfifo_regs[] = {
    INI_DMA_VFIFO_REGS(DMA_BASE, 14),   /* dma vfifo channel 14 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 15),   /* dma vfifo channel 15 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 16),   /* dma vfifo channel 16 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 17),   /* dma vfifo channel 17 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 18),   /* dma vfifo channel 18 */
    INI_DMA_VFIFO_REGS(DMA_BASE, 19),   /* dma vfifo channel 19 */
};
#endif

struct mt3351_uart_vfifo
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

#define VFIFO_INIT(i,a)     {.id = (i), .port = (void*)(a)}
#define VFIFO_PORT_NUM      ARRAY_SIZE(mt3351_uart_vfifo_port)

static DEFINE_SPINLOCK(mt3351_uart_vfifo_port_lock);

static struct mt3351_uart_vfifo mt3351_uart_vfifo_port[] = {
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART0, UART_VFIFO_PORT0),   /* vfifo port 0 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART1, UART_VFIFO_PORT1),   /* vfifo port 1 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART2, UART_VFIFO_PORT2),   /* vfifo port 2 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART3, UART_VFIFO_PORT3),   /* vfifo port 3 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART4, UART_VFIFO_PORT4),   /* vfifo port 4 */
    VFIFO_INIT(DMA_VIRTUAL_FIFO_UART5, UART_VFIFO_PORT5),   /* vfifo port 5 */                      
};


struct mt3351_uart_dma {
    struct mt3351_uart       *uart;     /* dma uart */
    atomic_t                  free;		/* dma channel free */
    unsigned short            mode;		/* dma mode */
    unsigned short            dir;		/* dma transfer direction */
    struct mt_dma_conf       *cfg;		/* dma configuration */
    struct tasklet_struct     tasklet;	/* dma handling tasklet */
    struct completion         done;		/* dma transfer done */
    struct scatterlist        sg[3];
    unsigned long             sg_num;
    unsigned long             sg_idx;
    struct mt3351_uart_vfifo *vfifo;	/* dma vfifo */
};

struct mt3351_uart
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

    struct mt3351_uart_dma dma_tx;
    struct mt3351_uart_dma dma_rx;
    struct mt3351_uart_vfifo *tx_vfifo;
    struct mt3351_uart_vfifo *rx_vfifo;

    struct scatterlist  sg_tx;
    struct scatterlist  sg_rx;
#ifdef DEBUG
    struct mt3351_uart_regs *debug;
#endif

    unsigned int (*write_allow)(struct mt3351_uart *uart);
    unsigned int (*read_allow)(struct mt3351_uart *uart);
    void         (*write_byte)(struct mt3351_uart *uart, unsigned int byte);
    unsigned int (*read_byte)(struct mt3351_uart *uart);
    unsigned int (*read_status)(struct mt3351_uart *uart);
};

struct mt3351_uart_dma_master{
    unsigned long tx_master;
    unsigned long rx_master;
};

/* uart control blocks */
static struct mt3351_uart mt3351_uarts[UART_NR];

/* uart dma master id */
static const struct mt3351_uart_dma_master mt3351_uart_dma_mas[] = {
    {DMA_CON_MASTER_UART0TX, DMA_CON_MASTER_UART0RX},
    {DMA_CON_MASTER_UART1TX, DMA_CON_MASTER_UART1RX},
    {DMA_CON_MASTER_UART2TX, DMA_CON_MASTER_UART2RX},
    {DMA_CON_MASTER_UART3TX, DMA_CON_MASTER_UART3RX},
    {DMA_CON_MASTER_UART4TX, DMA_CON_MASTER_UART4RX},    
};

static void mt3351_uart_init_ports(void);

static void mt3351_uart_start_tx(struct uart_port *port);
static void mt3351_uart_stop_tx(struct uart_port *port);
static void mt3351_uart_enable_intrs(struct mt3351_uart *uart, long mask);
static void mt3351_uart_disable_intrs(struct mt3351_uart *uart, long mask);
static unsigned int mt3351_uart_filter_line_status(struct mt3351_uart *uart);
static void mt3351_uart_dma_vfifo_tx_tasklet(unsigned long arg);
static void mt3351_uart_dma_vfifo_rx_tasklet(unsigned long arg);


#ifdef CONFIG_SERIAL_MT3351_CONSOLE
static void mt3351_uart_console_write(struct console *co, const char *s,
    unsigned int count)
{
    int i;
    struct mt3351_uart *uart;

    if (co->index >= UART_NR || !(co->flags & CON_ENABLED))
        return;

    uart = &mt3351_uarts[co->index];
    for (i = 0; i < count; i++) {
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

static int __init mt3351_uart_console_setup(struct console *co, char *options)
{
    struct uart_port *port;
    int baud    = 115200;
    int bits    = 8;
    int parity  = 'n';
    int flow    = 'n';
    int ret;

    printk(KERN_DEBUG "mt3351 console setup : co->index %d options:%s\n",
        co->index, options);

    if (co->index >= UART_NR)
        co->index = 0;
    port = (struct uart_port *)&mt3351_uarts[co->index];

    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);

    ret = uart_set_options(port, co, baud, parity, bits, flow);
    printk(KERN_DEBUG "mt3351 console setup : uart_set_option port(%d) "
          "baud(%d) parity(%c) bits(%d) flow(%c) - ret(%d)\n",
           co->index, baud, parity, bits, flow, ret);
    
    return ret;
}

static struct uart_driver mt3351_uart_drv;
static struct console mt3351_uart_console =
{
    .name       = "ttyMT",
    .write      = mt3351_uart_console_write,
    .device     = uart_console_device,
    .setup      = mt3351_uart_console_setup,
    .flags      = CON_PRINTBUFFER,
    .index      = -1,
    .data       = &mt3351_uart_drv,
};

static int __init mt3351_uart_console_init(void)
{
    mt3351_uart_init_ports();
    register_console(&mt3351_uart_console);
    return 0;
}

console_initcall(mt3351_uart_console_init);

static int __init mt3351_late_console_init(void)
{
    if (!(mt3351_uart_console.flags & CON_ENABLED))
    {
        register_console(&mt3351_uart_console);
    }
    return 0;
}

late_initcall(mt3351_late_console_init);

#endif /* CONFIG_SERIAL_MT3351_CONSOLE */

static struct mt3351_uart_vfifo *mt3351_uart_vfifo_alloc(
                                 struct mt3351_uart *uart, int dir, int size)
{
    int i;
    void *buf;    
    struct mt3351_uart_vfifo *vfifo = NULL;

    spin_lock(&mt3351_uart_vfifo_port_lock);

    if (dir == UART_RX_VFIFO) {
        if (mt3351_uart_vfifo_port[uart->nport].used) {
            dev_err(uart->port.dev, "No available vfifo port\n");
            goto end;
        } else {
            i = uart->nport;
        }
    } else {
        for (i = 3; i < VFIFO_PORT_NUM; i++) {
            if (!mt3351_uart_vfifo_port[i].used)
                break;        
        }
    }

    if (i == VFIFO_PORT_NUM) {
        dev_err(uart->port.dev, "No available vfifo port\n");
        goto end;
    }
    buf = (void*)kmalloc(size, GFP_KERNEL|GFP_DMA);
    
    if (!buf) {
        dev_err(uart->port.dev, "Cannot allocate memory\n");
        goto end;
    }
    MSG(INFO, "alloc vfifo-%d[%d](%p) to uart-%d\n", i, size, buf, uart->nport);

    vfifo = &mt3351_uart_vfifo_port[i];
    vfifo->size = size;
    vfifo->addr = buf;
    vfifo->used = 1;
    
end:
    spin_unlock(&mt3351_uart_vfifo_port_lock);
    return vfifo;

}
static void mt3351_uart_vfifo_free(struct mt3351_uart *uart,
                                   struct mt3351_uart_vfifo *vfifo)
{
    if (vfifo) {
        spin_lock(&mt3351_uart_vfifo_port_lock);     
        kfree(vfifo->addr);
        vfifo->addr  = 0;
        vfifo->size  = 0;
        vfifo->used  = 0;
        vfifo->owner = 0;
        spin_unlock(&mt3351_uart_vfifo_port_lock);
    }
}

inline static void mt3351_uart_vfifo_enable(struct mt3351_uart *uart)
{
    u32 base = uart->base;
    
    UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR);
    UART_CLR_BITS(UART_FCR_DMA1, UART_FCR);     /* must be mode 0 */
    UART_SET_BITS(UART_VFIFO_ON, UART_VFIFO_EN);
}

inline static void mt3351_uart_vfifo_disable(struct mt3351_uart *uart)
{
    u32 base = uart->base;
    
	UART_CLR_BITS(UART_VFIFO_ON, UART_VFIFO_EN);
}

inline static int mt3351_uart_vfifo_is_full(struct mt3351_uart_vfifo *vfifo)
{
    INFO info;

    mt_get_info(vfifo->owner, VF_FULL, &info);

    return (int)info;
}

inline static int mt3351_uart_vfifo_is_empty(struct mt3351_uart_vfifo *vfifo)
{
    INFO info;

    mt_get_info(vfifo->owner, VF_EMPTY, &info);

    return (int)info;
}

static unsigned int mt3351_uart_vfifo_write_allow(struct mt3351_uart *uart)
{
    return !mt3351_uart_vfifo_is_full(uart->tx_vfifo);
}

static unsigned int mt3351_uart_vfifo_read_allow(struct mt3351_uart *uart)
{
    return !mt3351_uart_vfifo_is_empty(uart->rx_vfifo);
}

static void mt3351_uart_vfifo_write_byte(struct mt3351_uart *uart, 
                                                unsigned int byte)
{
    UART_WRITE8((unsigned char)byte, uart->tx_vfifo->port);
}

static unsigned int mt3351_uart_vfifo_read_byte(struct mt3351_uart *uart)
{
    return (unsigned int)UART_READ8(uart->rx_vfifo->port);
}

static void mt3351_uart_vfifo_set_trig(struct mt3351_uart *uart, 
                                       struct mt3351_uart_vfifo *vfifo,
                                       u16 level)
{
    if (!vfifo || !vfifo->owner || !level) 
        return;

    vfifo->trig = level;
    vfifo->owner->count = level;
    mt_config_dma(vfifo->owner, ALL);
}

inline static unsigned short mt3351_uart_vfifo_get_trig(
                                        struct mt3351_uart *uart, 
                                        struct mt3351_uart_vfifo *vfifo)
{
    return vfifo->trig;
}

inline static void mt3351_uart_vfifo_set_owner(struct mt3351_uart_vfifo *vfifo,
                                               struct mt_dma_conf *owner)
{
	vfifo->owner = owner;
}

inline static int mt3351_uart_vfifo_get_counts(struct mt3351_uart_vfifo *vfifo)
{
    INFO info;

    mt_get_info(vfifo->owner, VF_FFCNT, &info);

    return (int)info;
}

static void mt3351_uart_dma_map(struct mt3351_uart *uart, 
                                struct scatterlist *sg, char *buf, 
                                unsigned long size, int dir)
{
    sg_init_one(sg, (u8*)buf, size);
    dma_map_sg(uart->port.dev, sg, 1, dir);
}

static void mt3351_uart_dma_unmap(struct mt3351_uart *uart,
                                  struct scatterlist *sg, int dir)
{
    dma_unmap_sg(uart->port.dev, sg, 1, dir);
}

static void mt3351_uart_dma_setup(struct mt3351_uart *uart,
                                  struct mt3351_uart_dma *dma,
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
    mt_config_dma(cfg, ALL);
}

static int mt3351_uart_dma_start(struct mt3351_uart *uart,
                                 struct mt3351_uart_dma *dma)

{
    struct mt_dma_conf *cfg = dma->cfg;

    if (!atomic_read(&dma->free))
        return -1;

    atomic_set(&dma->free, 0);
    init_completion(&dma->done);
    mt_start_dma(cfg);
    return 0;
}

static void mt3351_uart_dma_stop(struct mt3351_uart *uart, 
                                 struct mt3351_uart_dma *dma)
{
    if (dma && dma->cfg) {
        mt_stop_dma(dma->cfg);
        atomic_set(&dma->free, 1);
        complete(&dma->done);
    }
}


static void mt3351_uart_dma_callback(void *data)
{
    struct mt3351_uart_dma *dma = (struct mt3351_uart_dma*)data;
    
    tasklet_schedule(&dma->tasklet);
}

static void mt3351_uart_dma_tx_tasklet(unsigned long arg)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)arg;
    struct uart_port   *port = &uart->port;
    struct mt3351_uart_dma *dma = &uart->dma_tx;
    struct scatterlist *sg   = &dma->sg[dma->sg_idx];
    struct circ_buf    *xmit = &port->info->xmit;
    int resched = 0;
    
    spin_lock_bh(&port->lock);
    
    mt3351_uart_dma_stop(uart, dma);    
    xmit->tail = (xmit->tail + sg_dma_len(sg)) & (UART_XMIT_SIZE - 1);
    
    if (dma->sg_num > ++dma->sg_idx) {
        resched = 1;
    } else {
        MSG(DMA, "TX DMA transfer %d bytes done!!\n", sg_dma_len(sg));    
        port->icount.tx++;
        mt3351_uart_dma_unmap(uart, &uart->sg_tx, DMA_TO_DEVICE);
    }
    
    if (uart_circ_empty(xmit)) {
        uart->pending_tx_reqs = 0;
    } else if (uart->pending_tx_reqs) {
        uart->pending_tx_reqs--;
        resched = 1;
    }
    if (resched)
        mt3351_uart_start_tx(port);

    spin_unlock_bh(&port->lock);

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);       
}

static void mt3351_uart_dma_vfifo_timeout(unsigned long data)
{
    struct mt3351_uart_dma *dma = (struct mt3351_uart_dma *)data;

    //tasklet_schedule(&dma->tasklet);
    if (dma->uart) {
        if (!mt3351_uart_vfifo_is_empty(dma->vfifo)) {
            mt3351_uart_dma_vfifo_rx_tasklet((unsigned long)dma->uart);
        }
        mod_timer(&dma->vfifo->timer, jiffies + 200/(1000/HZ));
    } else {
        WARN_ON("Timer into wrong place\n");
        del_timer(&dma->vfifo->timer);
    }
}

static void mt3351_uart_dma_vfifo_callback(void *data)
{
    struct mt3351_uart_dma *dma = (struct mt3351_uart_dma*)data;
    struct mt3351_uart *uart = dma->uart;
        
    MSG(DMA, "%s VFIFO CB: %d/%d\n", dma->dir == DMA_TO_DEVICE ? "TX" : "RX",
        mt3351_uart_vfifo_get_counts(dma->vfifo), dma->vfifo->size);

    if (dma->dir == DMA_FROM_DEVICE) {
        mt3351_uart_dma_vfifo_rx_tasklet((unsigned long)uart);
        return;
    }
    
    tasklet_schedule(&dma->tasklet);
}

static char  txbuf[4096];
static ulong txbuf_idx;
static char  rxbuf[4096];
static ulong rxbuf_idx;

static void mt3351_uart_dma_vfifo_tx_tasklet(unsigned long arg)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)arg;
    struct uart_port   *port = &uart->port;
    struct mt3351_uart_dma *dma = &uart->dma_tx;    
    struct mt3351_uart_vfifo *vfifo = uart->tx_vfifo;
    struct circ_buf    *xmit = &port->info->xmit;
    unsigned long flags;
    unsigned int size, left;

    /* stop tx if circular buffer is empty or this port is stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
        uart->pending_tx_reqs = 0;
        atomic_set(&dma->free, 1);
        complete(&dma->done);    
        mt3351_uart_stop_tx(port);
        return;    
    }

    spin_lock_irqsave(&port->lock, flags);

    size = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);
    left = vfifo->size - mt3351_uart_vfifo_get_counts(vfifo);

    MSG(DMA, "TxBuf[%d]: Write %d bytes to VFIFO[%d]\n", 
        size, left < size ? left : size, 
        vfifo->size - mt3351_uart_vfifo_get_counts(vfifo));

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

static void mt3351_uart_dma_vfifo_rx_tasklet(unsigned long arg)
{
    struct mt3351_uart *uart = (struct mt3351_uart*)arg;
    struct uart_port   *port = &uart->port;
    struct mt3351_uart_vfifo *vfifo = uart->rx_vfifo;
    struct tty_struct *tty = uart->port.info->port.tty;
    struct tty_buffer *tb = tty->buf.tail;
    int count, left;
    unsigned int ch, flag, status;
    unsigned long flags;

    if (mt3351_uart_vfifo_is_empty(vfifo)) {
        mt3351_uart_dma_stop(uart, &uart->dma_rx);
        mt3351_uart_dma_start(uart, &uart->dma_rx);
        return;
    }
    
    count = left = tb->size - tb->used;

    spin_lock_irqsave(&port->lock, flags);
    rxbuf_idx = 0;
    while (!mt3351_uart_vfifo_is_empty(vfifo) && count > 0) {

        if (unlikely(tb->used >= tb->size)) {
            if (tty->low_latency) {
                tty_flip_buffer_push(tty);
            }
        }

        /* check status */
        uart->read_status(uart);
        status = mt3351_uart_filter_line_status(uart);
        
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
        tty_insert_flip_char(tty, ch, flag);
    }
    tty_flip_buffer_push(tty);

    MSG(DMA, "RxBuf[%d]: Read %d bytes from VFIFO[%d]\n", 
        left, left - count, mt3351_uart_vfifo_get_counts(vfifo));

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
        if (mt3351_uart_vfifo_get_counts(vfifo) > 
            mt3351_uart_vfifo_get_trig(uart, vfifo))
        tasklet_schedule(&uart->dma_rx.tasklet);
    } 
}

static int mt3351_uart_dma_alloc(struct mt3351_uart *uart, 
                                 struct mt3351_uart_dma *dma, int mode,
                                 struct mt3351_uart_vfifo *vfifo)
{
    struct mt_dma_conf *cfg;
    int ret = 0;

    MSG_FUNC_ENTRY();

    if (mode == UART_NON_DMA || dma->cfg)
        return -1;
    
    switch (mode) {
    case UART_TX_DMA:
        cfg = mt_request_dma(DMA_HALF_CHANNEL);
        if (!cfg) {
            printk(KERN_ERR "uart-%d: fail to alloc tx dma\n", uart->nport);
            ret = -1;
            break;
        }            
        /* set tx master*/
        mt_reset_dma(cfg);

    	cfg->mas      = mt3351_uart_dma_mas[uart->nport].tx_master;
    	cfg->iten     = DMA_TRUE;	/* interrupt mode */
    	cfg->dreq     = DMA_TRUE;	/* hardware handshaking */
        cfg->limiter  = 0;
    	cfg->dinc     = DMA_FALSE;   /* tx buffer     */
    	cfg->sinc     = DMA_TRUE;    /* memory buffer */    
        cfg->dir      = DMA_FALSE;   /* memory buffer to tx buffer */
    	cfg->data     = (void*)dma;
    	cfg->callback = mt3351_uart_dma_callback;
    	
    	atomic_set(&dma->free, 1);
        tasklet_init(&dma->tasklet, mt3351_uart_dma_tx_tasklet, 
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
        cfg = mt_request_dma(vfifo->id);
        if (!cfg) {
            printk(KERN_ERR "uart-%d: fail to alloc tx dma\n", uart->nport);
            ret = -1;
            break;
        }
        /* set tx master*/
        mt_reset_dma(cfg);

    	cfg->mas      = mt3351_uart_dma_mas[uart->nport].tx_master;
    	cfg->iten     = DMA_FALSE;	/* interrupt mode. CHECKME!!! */
    	cfg->dreq     = DMA_TRUE;	/* hardware handshaking. CHECKME!!! */
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
    	cfg->callback = mt3351_uart_dma_vfifo_callback;

        mt3351_uart_vfifo_set_owner(vfifo, cfg);
        atomic_set(&dma->free, 1);
        init_completion(&dma->done);        
        tasklet_init(&dma->tasklet, mt3351_uart_dma_vfifo_tx_tasklet,
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
        cfg = mt_request_dma(vfifo->id);
        if (!cfg) {
            printk(KERN_ERR "uart-%d: fail to alloc rx dma\n", uart->nport);
            ret = -1;
            break;
        }            
        /* set tx master*/
        mt_reset_dma(cfg);
    
        cfg->mas      = mt3351_uart_dma_mas[uart->nport].rx_master;
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
        cfg->callback = mt3351_uart_dma_vfifo_callback;
        
        mt3351_uart_vfifo_set_owner(vfifo, cfg);  
        atomic_set(&dma->free, 1);
        init_completion(&dma->done);        
        tasklet_init(&dma->tasklet, mt3351_uart_dma_vfifo_rx_tasklet, 
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

static void mt3351_uart_dma_free(struct mt3351_uart *uart, 
                                 struct mt3351_uart_dma *dma)
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
    } else if (dma->vfifo && !mt3351_uart_vfifo_is_empty(dma->vfifo)) {
        tasklet_schedule(&dma->tasklet);
        MSG(DMA, "wait for %s vfifo dma completed!!!\n", 
            dma->dir == DMA_TO_DEVICE ? "TX" : "RX");    
        wait_for_completion(&dma->done);          
    }
    spin_lock_irqsave(&uart->port.lock, flags);
    mt_stop_dma(dma->cfg);
    if (timer_pending(&dma->vfifo->timer))
        del_timer_sync(&dma->vfifo->timer);
    tasklet_kill(&dma->tasklet);
    mt_free_dma(dma->cfg);
    MSG(INFO, "free %s dma completed!!!\n", 
        dma->dir == DMA_TO_DEVICE ? "TX" : "RX");    
    memset(dma, 0, sizeof(struct mt3351_uart_dma));
    spin_unlock_irqrestore(&uart->port.lock, flags);    
}

inline static void mt3351_uart_fifo_init(struct mt3351_uart *uart)
{
    u32 base = uart->base;

    UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR);
}

inline static void mt3351_uart_fifo_flush(struct mt3351_uart *uart)
{
    u32 base = uart->base;

    UART_SET_BITS(UART_FCR_CLRR | UART_FCR_CLRT, UART_FCR);
}

static void mt3351_uart_fifo_set_trig(struct mt3351_uart *uart, int tx_level, 
                                      int rx_level)
{
    u32 base = uart->base;
    unsigned long tmp1, tmp2;

    tmp1 = UART_READ32(UART_LCR);
    UART_WRITE32(0xbf, UART_LCR);
    tmp2 = UART_READ32(UART_EFR);
    UART_SET_BITS(UART_EFR_EN, UART_EFR);
    UART_WRITE32(tmp1, UART_LCR);

    UART_WRITE32(UART_FCR_FIFO_INIT|tx_level|rx_level, UART_FCR);

    tmp1 = UART_READ32(UART_LCR);
    UART_WRITE32(0xbf, UART_LCR);
    UART_WRITE32(tmp2, UART_EFR);
    UART_WRITE32(tmp1, UART_LCR);

}

static void mt3351_uart_set_mode(struct mt3351_uart *uart, int mode)
{
    u32 base = uart->base;

    if (mode == UART_DMA_MODE_0) {
        UART_CLR_BITS(UART_FCR_DMA1, UART_FCR);
    } else if (mode == UART_DMA_MODE_1) {
        UART_SET_BITS(UART_FCR_DMA1, UART_FCR);
    }
}

static void mt3351_uart_set_auto_baud(struct mt3351_uart *uart)
{
    u32 base = uart->base;

    MSG_FUNC_ENTRY();
        
    switch (uart->sysclk)
    {
    case MT3351_SYSCLK_13:
        UART_WRITE32(UART_AUTOBADUSAM_13M, UART_AUTOBAUD_SAMPLE);
        break;
    case MT3351_SYSCLK_26:
        UART_WRITE32(UART_AUTOBADUSAM_26M, UART_AUTOBAUD_SAMPLE);                    
        break;
    case MT3351_SYSCLK_52:
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

static void mt3351_uart_set_baud(struct mt3351_uart *uart , int baudrate)
{
    u32 base = uart->base;
    unsigned int byte;
    unsigned int highspeed;
    unsigned int quot, divisor, remainder;
    unsigned int ratefix; 
    unsigned int uartclk;

    uartclk = uart->sysclk;
    
    if (uart->auto_baud)
        mt3351_uart_set_auto_baud(uart); 

    ratefix = UART_READ32(UART_RATE_FIX_AD);
    if (ratefix & UART_RATE_FIX_ALL_13M) {
        if (ratefix & UART_FREQ_SEL_13M)
            uartclk = uart->sysclk / 4;
        else
            uartclk = uart->sysclk / 2;
    }
    
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
}

static void mt3351_uart_set_flow_ctrl(struct mt3351_uart *uart, int mode)
{
    u32 base = uart->base;
    unsigned int tmp = UART_READ32(UART_LCR);
    
    UART_WRITE32(0xbf, UART_LCR);
    
    switch (mode) {
    case UART_FC_NONE:
        UART_WRITE16(UART_EFR_NO_FLOW_CTRL, UART_EFR);
        mt3351_uart_disable_intrs(uart, UART_IER_XOFFI|UART_IER_RTSI|UART_IER_CTSI);
        break;
    case UART_FC_HW:
        UART_WRITE16(UART_EFR_AUTO_RTSCTS, UART_EFR);
        //mt3351_uart_enable_intrs(uart, UART_IER_RTSI|UART_IER_CTSI); /* CHECKME! HW BUG? */
        break;
    case UART_FC_SW:
        UART_WRITE16(UART_EFR_XON1_XOFF1, UART_EFR);
        UART_WRITE32(START_CHAR(uart->port.info->port.tty), UART_XON1);
        UART_WRITE32(STOP_CHAR(uart->port.info->port.tty), UART_XOFF1);
        //mt3351_uart_enable_intrs(uart, UART_IER_XOFFI); /* CHECKME! HW BUG? */
        break;
    }

    UART_WRITE32(tmp, UART_LCR);
}

inline static void mt3351_uart_power_up(struct mt3351_uart *uart)
{
    //2009/02/02, Kelvin modify for power management
    hwEnableClock(uart->nport + PDN_PERI_UART0);
    //PDN_Power_CONA_DOWN(uart->nport + PDN_PERI_UART0, 0); /* power on port */ 
}

inline static void mt3351_uart_power_down(struct mt3351_uart *uart)
{
    hwDisableClock(uart->nport + PDN_PERI_UART0);
    //PDN_Power_CONA_DOWN(uart->nport + PDN_PERI_UART0, 1); /* power off port */ 
}

static void mt3351_uart_config(struct mt3351_uart *uart, 
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

    mt3351_uart_set_baud(uart, baud);
}

static unsigned int mt3351_uart_read_status(struct mt3351_uart *uart)
{
    u32 base = uart->base;

    uart->line_status = UART_READ32(UART_LSR);
    return uart->line_status;
}

static unsigned int mt3351_uart_read_allow(struct mt3351_uart *uart)
{
    return uart->line_status & UART_LSR_DR;
}

static unsigned int mt3351_uart_write_allow(struct mt3351_uart *uart)
{
    u32 base = uart->base;
    return UART_READ32(UART_LSR) & UART_LSR_THRE;
}

static void mt3351_uart_enable_intrs(struct mt3351_uart *uart, long mask)
{
    u32 base = uart->base;
    UART_SET_BITS(mask, UART_IER);
}

static void mt3351_uart_disable_intrs(struct mt3351_uart *uart, long mask)
{
    u32 base = uart->base;
    UART_CLR_BITS(mask, UART_IER);
}

static unsigned int mt3351_uart_read_byte(struct mt3351_uart *uart)
{
    u32 base = uart->base;
    return UART_READ32(UART_RBR);
}

static void mt3351_uart_write_byte(struct mt3351_uart *uart, unsigned int byte)
{
    u32 base = uart->base;
    UART_WRITE32(byte, UART_THR);
}

static unsigned int mt3351_uart_filter_line_status(struct mt3351_uart *uart)
{
    struct uart_port *port = &uart->port;
    unsigned int status;
    unsigned int lsr = uart->line_status;

    status = UART_LSR_BI|UART_LSR_PE|UART_LSR_FE|UART_LSR_OE;

#ifdef DEBUG
    if ((lsr & UART_LSR_BI) || (lsr & UART_LSR_PE) ||
        (lsr & UART_LSR_FE) || (lsr & UART_LSR_OE)) {
        MSG(INFO, "LSR: BI=%d, FE=%d, PE=%d, OE=%d, DR=%d\n",
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

static void mt3351_uart_rx_chars(struct mt3351_uart *uart)
{
    struct uart_port *port = &uart->port;
    struct tty_struct *tty = uart->port.info->port.tty;
    int max_count = UART_FIFO_SIZE;
    unsigned int data_byte, status;
    unsigned int flag;

    MSG_FUNC_ENTRY();
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

        status = mt3351_uart_filter_line_status(uart);
        
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

        if (uart_handle_sysrq_char(port, data_byte))
            continue;

        /* FIXME. Infinity, 20081002, 'BREAK' char to enable sysrq handler { */
#ifdef CONFIG_MAGIC_SYSRQ
        if (data_byte == 0)
            uart->port.sysrq = 1;
#endif
        /* FIXME. Infinity, 20081002, 'BREAK' char to enable sysrq handler } */

        tty_insert_flip_char(tty, data_byte, flag);
    }
    tty_flip_buffer_push(tty);
}

static void mt3351_uart_tx_chars(struct mt3351_uart *uart)
{
    struct uart_port *port = &uart->port;
    struct circ_buf *xmit = &port->info->xmit;
    int count;

    /* deal with x_char first */
    if (unlikely(port->x_char)) {
        uart->write_byte(uart, port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    /* stop tx if circular buffer is empty or this port is stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
        mt3351_uart_stop_tx(port);
        return;
    }

    count = port->fifosize - 1;

    do {
        if (uart_circ_empty(xmit))
            break;
        if (!uart->write_allow(uart)) {
            mt3351_uart_enable_intrs(uart, UART_IER_ETBEI);
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
        mt3351_uart_stop_tx(port);
}

static void mt3351_uart_get_modem_status(struct mt3351_uart *uart)
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

static void mt3351_uart_rx_handler(struct mt3351_uart *uart, int timeout)
{
    if (uart->rx_mode == UART_NON_DMA) {
        mt3351_uart_rx_chars(uart);
    } else if (uart->rx_mode == UART_RX_VFIFO_DMA) {
        if (timeout && uart->read_allow(uart))
            mt3351_uart_dma_vfifo_rx_tasklet((unsigned long)uart);
    }
}

static void mt3351_uart_tx_handler(struct mt3351_uart *uart)
{
    if (uart->tx_mode == UART_NON_DMA) {
        mt3351_uart_tx_chars(uart);
    } else if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        tasklet_schedule(&uart->dma_tx.tasklet);
    }
}

/* FIXME */
static irqreturn_t mt3351_uart_irq(int irq, void *dev_id)
{
    unsigned int intrs, timeout = 0;
    struct mt3351_uart *uart = (struct mt3351_uart *)dev_id;
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
        mt3351_uart_get_modem_status(uart);
    } else if (intrs == UART_IIR_SW_FLOW_CTRL) {
        /* XOFF is received */
    } else if (intrs == UART_IIR_HW_FLOW_CTRL) {
        /* CTS or RTS is in rising edge */
    }

    mt3351_uart_rx_handler(uart, timeout);
    
    if (uart->write_allow(uart))
        mt3351_uart_tx_handler(uart);

    return IRQ_HANDLED;
}

/* test whether the transmitter fifo and shifter for the port is empty. */
static unsigned int mt3351_uart_tx_empty(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;

    MSG_FUNC_ENTRY();
    
    if (uart->tx_mode == UART_TX_VFIFO_DMA)
        return mt3351_uart_vfifo_is_empty(uart->dma_tx.vfifo) ? TIOCSER_TEMT : 0;
    else
        return uart->write_allow(uart) ? TIOCSER_TEMT : 0;
}

/* set the modem control lines. */
static void mt3351_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;
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
static unsigned int mt3351_uart_get_mctrl(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;
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
static void mt3351_uart_stop_tx(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;

    if (uart->tx_mode == UART_TX_DMA) {
        mt3351_uart_dma_stop(uart, &uart->dma_tx);
    } else {
        /* disable tx interrupt */
        mt3351_uart_disable_intrs(uart, UART_IER_ETBEI);
    }
    uart->tx_stop = 1;
}

/* FIXME */
static void mt3351_uart_start_tx(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;
    struct mt3351_uart_dma *dma_tx = &uart->dma_tx;
    struct circ_buf    *xmit = &port->info->xmit;    
    unsigned long size;
    
    size = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);

    if (!size)
        return;

    uart->tx_stop = 0;

    if (uart->tx_mode == UART_TX_DMA) {
        if (atomic_read(&dma_tx->free)) {
            mt3351_uart_dma_map(uart, &uart->sg_tx, (u8*)&xmit->buf[xmit->tail], 
                                size, DMA_TO_DEVICE);
            mt3351_uart_dma_setup(uart, dma_tx, &uart->sg_tx);
            mt3351_uart_dma_start(uart, dma_tx);
        } else if (uart->pending_tx_reqs < UART_MAX_TX_PENDING) {
            uart->pending_tx_reqs++;
        }
    } else if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        if (uart->write_allow(uart))
            mt3351_uart_dma_vfifo_tx_tasklet((unsigned long)uart);
            //tasklet_schedule(&dma_tx->tasklet);
    }
    else {
        if (uart->write_allow(uart))
            mt3351_uart_tx_chars(uart);
    }
}

/* FIXME */
static void mt3351_uart_stop_rx(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;

    MSG_FUNC_ENTRY();
    if (uart->rx_mode == UART_NON_DMA) {
        mt3351_uart_disable_intrs(uart, UART_IER_ERBFI);
    } else {
        mt3351_uart_dma_stop(uart, &uart->dma_rx);
    }
    uart->rx_stop = 1;
}

/* FIXME */
static void mt3351_uart_send_xchar(struct uart_port *port, char ch)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;
    unsigned long flags;

    MSG_FUNC_ENTRY();
    
    if (uart->tx_stop)
        return;

    spin_lock_irqsave(&port->lock, flags);
    while (!uart->write_allow(uart));
    uart->write_byte(uart, (unsigned int)ch);
    port->icount.tx++; 
    spin_unlock_irqrestore(&port->lock, flags);   
    return;
}

/* enable the modem status interrupts */
static void mt3351_uart_enable_ms(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;

    MSG_FUNC_ENTRY();
    uart->ms_enable = 1;
}

/* control the transmission of a break signal */
static void mt3351_uart_break_ctl(struct uart_port *port, int break_state)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;
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
static int mt3351_uart_startup(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;
    int ret;
    long mask = UART_IER_HW_NORMALINTS;
    
    MSG_FUNC_ENTRY();

    /* power on the uart port */
    mt3351_uart_power_up(uart);

    /* disable interrupts */
    mt3351_uart_disable_intrs(uart, UART_IER_ALL_INTS);

    /* allocate irq line */
    ret = request_irq(port->irq, mt3351_uart_irq, 0, DRV_NAME, uart);
    if (ret)
        return ret;

    /* allocate vfifo */
    if (uart->rx_mode == UART_RX_VFIFO_DMA) {
        uart->rx_vfifo = mt3351_uart_vfifo_alloc(uart, UART_RX_VFIFO, 
                                                 UART_VFIFO_SIZE);       
    }
    if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        uart->tx_vfifo = mt3351_uart_vfifo_alloc(uart, UART_TX_VFIFO, 
                                                 UART_VFIFO_SIZE);
    }
    
    /* allocate dma channels */
    ret = mt3351_uart_dma_alloc(uart, &uart->dma_rx, uart->rx_mode, uart->rx_vfifo);
    if (ret) {
        uart->rx_mode = UART_NON_DMA;
    }
    ret = mt3351_uart_dma_alloc(uart, &uart->dma_tx, uart->tx_mode, uart->tx_vfifo);
    if (ret) {
        uart->tx_mode = UART_NON_DMA;
    }
    
    /* start vfifo dma */
    if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        uart->write_allow = mt3351_uart_vfifo_write_allow;
        uart->write_byte  = mt3351_uart_vfifo_write_byte;
        mt3351_uart_vfifo_enable(uart);
        mt3351_uart_vfifo_set_trig(uart, uart->tx_vfifo, UART_VFIFO_SIZE / 4);
        mt3351_uart_dma_setup(uart, &uart->dma_tx, NULL);
        mt3351_uart_dma_start(uart, &uart->dma_tx);
    }
    if (uart->rx_mode == UART_RX_VFIFO_DMA) {
        uart->read_allow = mt3351_uart_vfifo_read_allow;
        uart->read_byte  = mt3351_uart_vfifo_read_byte;  
        mt3351_uart_vfifo_enable(uart);
        //mt3351_uart_vfifo_set_trig(uart, uart->rx_vfifo, 0);
        mt3351_uart_vfifo_set_trig(uart, uart->rx_vfifo, UART_VFIFO_SIZE / 4 * 3);
        mt3351_uart_dma_setup(uart, &uart->dma_rx, NULL);
        mt3351_uart_dma_start(uart, &uart->dma_rx);

        init_timer(&uart->rx_vfifo->timer);
        uart->rx_vfifo->timer.expires  = jiffies + 150/(1000/HZ);
        uart->rx_vfifo->timer.function = mt3351_uart_dma_vfifo_timeout;
        uart->rx_vfifo->timer.data     = (unsigned long)&uart->dma_rx;
        add_timer(&uart->rx_vfifo->timer);
    }

    uart->tx_stop = 0;
    uart->rx_stop = 0;

    /* enable interrupts */
    mt3351_uart_enable_intrs(uart, mask);
    
    return 0;
}

static void mt3351_uart_shutdown(struct uart_port *port)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;

    MSG_FUNC_ENTRY();

    /* disable interrupts */
    mt3351_uart_disable_intrs(uart, UART_IER_ALL_INTS);

    /* free dma channels and vfifo */
    mt3351_uart_dma_free(uart, &uart->dma_rx);
    mt3351_uart_dma_free(uart, &uart->dma_tx);
    mt3351_uart_vfifo_free(uart, uart->rx_vfifo);    
    mt3351_uart_vfifo_free(uart, uart->tx_vfifo);
    mt3351_uart_vfifo_disable(uart);
    mt3351_uart_fifo_flush(uart);

    /* release irq line */
    free_irq(port->irq, port);

    /* power off the uart port */
    mt3351_uart_power_down(uart);
}

static void mt3351_uart_set_termios(struct uart_port *port,
                                   struct ktermios *termios, struct ktermios *old)
{
    struct mt3351_uart *uart = (struct mt3351_uart *)port;
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
    baud = uart_get_baud_rate(port, termios, old, 0, uart->sysclk / 16);    
    uart_update_timeout(port, termios->c_cflag, baud);	
    mt3351_uart_config(uart, baud, datalen, stopbit, parity);

    /* setup fifo trigger level */
    mt3351_uart_fifo_set_trig(uart, uart->tx_trig_level, uart->rx_trig_level);

    /* setup hw flow control: only port 0 ~2 support hw rts/cts */
    if (HW_FLOW_CTRL_PORT(uart) && (termios->c_cflag & CRTSCTS)) {
        MSG(CFG, "Hardware Flow Control\n");
        mode = UART_FC_HW;
    } else if (termios->c_iflag & (IXON | IXOFF | IXANY)) {
        MSG(CFG, "Software Flow Control\n");
        mode = UART_FC_SW;
    } else {
        MSG(CFG, "No Flow Control\n");
        mode = UART_FC_NONE;
    }
    mt3351_uart_set_flow_ctrl(uart, mode);

    /* determine if port should enable modem status interrupt */
    if (UART_ENABLE_MS(port, termios->c_cflag)) 
        uart->ms_enable = 1;
    else
        uart->ms_enable = 0;

    spin_unlock_irqrestore(&port->lock, flags);
}

/* perform any power management related activities on the port */
static void mt3351_uart_power_mgnt(struct uart_port *port, unsigned int state,
			                       unsigned int oldstate)
{
    return; /* TODO */
}

static int mt3351_uart_set_wake(struct uart_port *port, unsigned int state)
{
    return 0; /* Not used in current kernel version */
}

/* return a pointer to a string constant describing the port */
static const char *mt3351_uart_type(struct uart_port *port)
{
    return "MT3351 UART";
}

/* release any memory and io region resources currently in used by the port */
static void mt3351_uart_release_port(struct uart_port *port)
{
    return;
}

/* request any memory and io region resources required by the port */
static int mt3351_uart_request_port(struct uart_port *port)
{
    return 0;
}

static void mt3351_uart_config_port(struct uart_port *port, int flags)
{ 
    if (flags & UART_CONFIG_TYPE) {
        mt3351_uart_request_port(port);
        port->type = PORT_MT3351;
    }
}

/* verify if the new serial information contained within 'ser' is suitable */ 
static int mt3351_uart_verify_port(struct uart_port *port,
    struct serial_struct *ser)
{
	int ret = 0;
    if (ser->type != PORT_UNKNOWN && ser->type != PORT_MT3351)
		ret = -EINVAL;
	if (ser->irq != port->irq)
		ret = -EINVAL;
	if (ser->baud_base < 110)
		ret = -EINVAL;
	return ret;
}

/* perform any port specific IOCTLs */
static int mt3351_uart_ioctl(struct uart_port *port, unsigned int cmd, 
                             unsigned long arg)
{
    return -ENOIOCTLCMD;
}

static struct uart_ops mt3351_uart_ops =
{
    .tx_empty       = mt3351_uart_tx_empty,
    .set_mctrl      = mt3351_uart_set_mctrl,
    .get_mctrl      = mt3351_uart_get_mctrl,
    .stop_tx        = mt3351_uart_stop_tx,
    .start_tx       = mt3351_uart_start_tx,
    .stop_rx        = mt3351_uart_stop_rx,
    .send_xchar     = mt3351_uart_send_xchar,
    .enable_ms      = mt3351_uart_enable_ms,
    .break_ctl      = mt3351_uart_break_ctl,
    .startup        = mt3351_uart_startup,
    .shutdown       = mt3351_uart_shutdown,
    .set_termios    = mt3351_uart_set_termios,
    .pm             = mt3351_uart_power_mgnt,
    .set_wake       = mt3351_uart_set_wake,
    .type           = mt3351_uart_type,
    .release_port   = mt3351_uart_release_port,
    .request_port   = mt3351_uart_request_port,
    .config_port    = mt3351_uart_config_port,
    .verify_port    = mt3351_uart_verify_port,
    .ioctl          = mt3351_uart_ioctl,
};

static struct uart_driver mt3351_uart_drv =
{
    .owner          = THIS_MODULE,
    .driver_name    = DRV_NAME,
    .dev_name       = "ttyMT",
    .major          = UART_MAJOR,
    .minor          = UART_MINOR,
    .nr             = UART_NR,
#ifdef CONFIG_SERIAL_MT3351_CONSOLE
    .cons           = &mt3351_uart_console,
#endif
};

static int mt3351_uart_probe(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct mt3351_uart     *uart = &mt3351_uarts[pdev->id];

    uart->port.dev = dev;
    uart_add_one_port(&mt3351_uart_drv, &uart->port);
    dev_set_drvdata(dev, uart);

    return 0;
}

static int mt3351_uart_remove(struct device *dev)
{
    struct mt3351_uart *uart = dev_get_drvdata(dev);

    dev_set_drvdata(dev, NULL);

    if (uart)
        uart_remove_one_port(&mt3351_uart_drv, &uart->port);
    
    return 0;
}

#ifdef CONFIG_PM 
static int mt3351_uart_suspend(struct device *dev, pm_message_t state)
{
    int ret = 0;
    struct mt3351_uart *uart = dev_get_drvdata(dev);

    if (uart) {
        ret = uart_suspend_port(&mt3351_uart_drv, &uart->port);
        printk(KERN_INFO "[UART%d] Suspend(%d)!\n", uart->nport, ret);
    }

    return ret;
}

static int mt3351_uart_resume(struct device *dev)
{
    int ret = 0;
    struct mt3351_uart *uart = dev_get_drvdata(dev);

    if (uart) {
        ret = uart_resume_port(&mt3351_uart_drv, &uart->port);
        printk(KERN_INFO "[UART%d] Resume(%d)!\n", uart->nport, ret);
    }
    return 0;
}
#endif

static void mt3351_uart_init_ports(void)
{
    int i;
    struct mt3351_uart *uart;
    unsigned long base;

    for (i = 0; i < UART_NR; i++) {
        uart = &mt3351_uarts[i];        
        base = UART1_BASE + i * 0x1000;        
        uart->port.iotype   = UPIO_MEM;
        uart->port.mapbase  = base - IO_OFFSET;   /* for ioremap */
        uart->port.membase  = (unsigned char __iomem *)base;
        uart->port.irq      = MT3351_IRQ_UART1_CODE + i;
        uart->port.fifosize = UART_FIFO_SIZE;
        uart->port.ops      = &mt3351_uart_ops;
        uart->port.flags    = UPF_BOOT_AUTOCONF;
        uart->port.line     = i;
        uart->port.lock     = SPIN_LOCK_UNLOCKED;
        uart->base          = base;
        uart->auto_baud     = CFG_UART_AUTOBAUD;
        uart->nport         = i;
        uart->sysclk        = UART_SYSCLK; /* FIXME */
        uart->dma_mode      = mt3351_uart_default_settings[i].dma_mode;
        uart->tx_mode       = mt3351_uart_default_settings[i].tx_mode;
        uart->rx_mode       = mt3351_uart_default_settings[i].rx_mode;
        uart->tx_trig_level = mt3351_uart_default_settings[i].tx_trig_level;
        uart->rx_trig_level = mt3351_uart_default_settings[i].rx_trig_level;
        uart->write_allow   = mt3351_uart_write_allow;
        uart->read_allow    = mt3351_uart_read_allow;
        uart->write_byte    = mt3351_uart_write_byte;
        uart->read_byte     = mt3351_uart_read_byte;
        uart->read_status   = mt3351_uart_read_status;
#ifdef DEBUG
        uart->debug         = &mt3351_uart_debug[i];
#endif
        //mt3351_uart_power_down(uart);
        //mt3351_uart_config_pinmux(uart);
        //mt3351_uart_power_up(uart);
        mt3351_uart_fifo_init(uart);
        mt3351_uart_set_mode(uart, uart->dma_mode);
    }
}

static struct device_driver mt3351_uart_dev_drv =
{
    .name    = DRV_NAME,
    .bus     = &platform_bus_type,
    .probe   = mt3351_uart_probe,
    .remove  = mt3351_uart_remove,
#ifdef CONFIG_PM    
    .suspend = mt3351_uart_suspend,
    .resume  = mt3351_uart_resume,
#endif    
};

static int __init mt3351_uart_init(void)
{
    int ret = 0;

#ifndef CONFIG_SERIAL_MT3351_CONSOLE
    mt3351_uart_init_ports();
#endif

    ret = uart_register_driver(&mt3351_uart_drv);

    if (ret) return ret;
    
    ret = driver_register(&mt3351_uart_dev_drv);

    if (ret) {
        uart_unregister_driver(&mt3351_uart_drv);
        return ret;
    }

    return ret;
}

static void __exit mt3351_uart_exit(void)
{
    driver_unregister(&mt3351_uart_dev_drv);
    uart_unregister_driver(&mt3351_uart_drv);
}

module_init(mt3351_uart_init);
module_exit(mt3351_uart_exit);

MODULE_AUTHOR("Infinity Chen <infinity.chen@mediatek.com>");
MODULE_DESCRIPTION("MT3351 Serial Port Driver $Revision$");
MODULE_LICENSE("MKL");
