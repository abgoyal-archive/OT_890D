

//MT6516_I2C2_IRQ_LINE

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <asm/scatterlist.h>
#include <linux/scatterlist.h>
#include <asm/io.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_reg_base.h>
//#include <mach/mt6516_pmu_sw.h>
//#include <mach/mt6516_pdn_hw.h>
//#include <mach/mt6516_pdn_sw.h>
//#include <mach/mt6516_wdt.h>
#include <mach/mt6516_devs.h>
#include <mach/dma.h>
#include <asm/tcm.h>


//#define CFG_FULL_SIZE_DMA

#define DRV_NAME                    "mt6516-i2c"

#if 0
#if defined(CONFIG_mt6516_CPU_416MHZ_MCU_104MHZ) || \
    defined(CONFIG_mt6516_CPU_208MHZ_MCU_104MHZ)
#define I2C_CLK_RATE			    13000			/* khz */
#elif defined(CONFIG_mt6516_CPU_468MHZ_MCU_117MHZ)
#define I2C_CLK_RATE			    14625			/* khz */
#else
#error "I2C clock rate not specified"
#endif
#endif

//#define mt6516_IRQ_I2C_CODE         19  /*I2C 2 controller*/

#define I2C_CLK_RATE			    13000			/* khz for CPU_416MHZ_MCU_104MHZ*/

#define I2C_FIFO_SIZE  	  			8

#define MAX_ST_MODE_SPEED			100	 /* khz */
#define MAX_FS_MODE_SPEED			400	 /* khz */
#define MAX_HS_MODE_SPEED	   		3400 /* khz */

#define MAX_DMA_TRANS_SIZE			252	/* Max(255) aligned to 4 bytes = 252 */
#define MAX_DMA_TRANS_NUM			256

#define MAX_SAMPLE_CNT_DIV			8
#define MAX_STEP_CNT_DIV			64
#define MAX_HS_STEP_CNT_DIV			8

#define mt6516_I2C_DATA_PORT		((base) + 0x0000)
#define mt6516_I2C_SLAVE_ADDR		((base) + 0x0004)
#define mt6516_I2C_INTR_MASK		((base) + 0x0008)
#define mt6516_I2C_INTR_STAT		((base) + 0x000c)
#define mt6516_I2C_CONTROL			((base) + 0x0010)
#define mt6516_I2C_TRANSFER_LEN	    ((base) + 0x0014)
#define mt6516_I2C_TRANSAC_LEN	    ((base) + 0x0018)
#define mt6516_I2C_DELAY_LEN		((base) + 0x001c)
#define mt6516_I2C_TIMING			((base) + 0x0020)
#define mt6516_I2C_START			((base) + 0x0024)
#define mt6516_I2C_FIFO_STAT		((base) + 0x0030)
#define mt6516_I2C_FIFO_THRESH	    ((base) + 0x0034)
#define mt6516_I2C_FIFO_ADDR_CLR	((base) + 0x0038)
#define mt6516_I2C_IO_CONFIG		((base) + 0x0040)
#define mt6516_I2C_DEBUG			((base) + 0x0044)
#define mt6516_I2C_HS				((base) + 0x0048)
#define mt6516_I2C_DEBUGSTAT		((base) + 0x0064)
#define mt6516_I2C_DEBUGCTRL		((base) + 0x0068)

#define mt6516_I2C_TRANS_LEN_MASK		(0xff)
#define mt6516_I2C_TRANS_AUX_LEN_MASK	(0x1f << 8)
#define mt6516_I2C_CONTROL_MASK			(0x3f << 1)

#define I2C_DEBUG					(1 << 3)
#define I2C_HS_NACKERR				(1 << 2)
#define I2C_ACKERR					(1 << 1)
#define I2C_TRANSAC_COMP			(1 << 0)

#define I2C_TX_THR_OFFSET			8
#define I2C_RX_THR_OFFSET			0

#define I2C_START_TRANSAC			__raw_writel(0x1,mt6516_I2C_START)
#define I2C_FIFO_CLR_ADDR			__raw_writel(0x1,mt6516_I2C_FIFO_ADDR_CLR)
#define I2C_FIFO_OFFSET				(__raw_readl(mt6516_I2C_FIFO_STAT)>>4&0xf)
#define I2C_FIFO_IS_EMPTY			(__raw_readw(mt6516_I2C_FIFO_STAT)>>0&0x1)

#define I2C_SET_BITS(BS,REG)       ((*(volatile u32*)(REG)) |= (u32)(BS))
#define I2C_CLR_BITS(BS,REG)       ((*(volatile u32*)(REG)) &= ~((u32)(BS)))

#define I2C_SET_FIFO_THRESH(tx,rx)	\
	do { u32 tmp = (((tx) & 0x7) << I2C_TX_THR_OFFSET) | \
	               (((rx) & 0x7) << I2C_RX_THR_OFFSET); \
		 __raw_writel(tmp, mt6516_I2C_FIFO_THRESH); \
	} while(0)

#define I2C_SET_INTR_MASK(mask)		__raw_writel(mask, mt6516_I2C_INTR_MASK)

#define I2C_CLR_INTR_MASK(mask)		\
	do { u32 tmp = __raw_readl(mt6516_I2C_INTR_MASK); \
		 tmp &= ~(mask); \
		 __raw_writel(tmp, mt6516_I2C_INTR_MASK); \
	} while(0)

#define I2C_SET_SLAVE_ADDR(addr)	__raw_writel((addr)&0xFF, mt6516_I2C_SLAVE_ADDR)

#define I2C_SET_TRANS_LEN(len)	 	\
	do { u32 tmp = __raw_readl(mt6516_I2C_TRANSFER_LEN) & \
	                          ~mt6516_I2C_TRANS_LEN_MASK; \
		 tmp |= ((len) & mt6516_I2C_TRANS_LEN_MASK); \
		 __raw_writel(tmp, mt6516_I2C_TRANSFER_LEN); \
	} while(0)

#define I2C_SET_TRANS_AUX_LEN(len)	\
	do { u32 tmp = __raw_readl(mt6516_I2C_TRANSFER_LEN) & \
	                         ~(mt6516_I2C_TRANS_AUX_LEN_MASK); \
		 tmp |= (((len) << 8) & mt6516_I2C_TRANS_AUX_LEN_MASK); \
		 __raw_writel(tmp, mt6516_I2C_TRANSFER_LEN); \
	} while(0)

#define I2C_SET_TRANSAC_LEN(len)	__raw_writel(len, mt6516_I2C_TRANSAC_LEN)
#define I2C_SET_TRANS_DELAY(delay)	__raw_writel(delay, mt6516_I2C_DELAY_LEN)

#define I2C_SET_TRANS_CTRL(ctrl)	\
	do { u32 tmp = __raw_readl(mt6516_I2C_CONTROL) & ~mt6516_I2C_CONTROL_MASK; \
		tmp |= ((ctrl) & mt6516_I2C_CONTROL_MASK); \
		__raw_writel(tmp, mt6516_I2C_CONTROL); \
	} while(0)

#define I2C_SET_HS_MODE(on_off) \
	do { u32 tmp = __raw_readl(mt6516_I2C_HS) & ~0x1; \
	tmp |= (on_off & 0x1); \
	__raw_writel(tmp, mt6516_I2C_HS); \
	} while(0)

#define I2C_READ_BYTE(byte)		\
	do { byte = __raw_readb(mt6516_I2C_DATA_PORT); } while(0)
	
#define I2C_WRITE_BYTE(byte)	\
	do { __raw_writeb(byte, mt6516_I2C_DATA_PORT); } while(0)

#define I2C_CLR_INTR_STATUS(status)	\
		do { __raw_writew(status, mt6516_I2C_INTR_STAT); } while(0)

#define I2C_INTR_STATUS				__raw_readw(mt6516_I2C_INTR_STAT)

/* mt6516 i2c control bits */
#define TRANS_LEN_CHG 				(1 << 6)
#define ACK_ERR_DET_EN				(1 << 5)
#define DIR_CHG						(1 << 4)
#define CLK_EXT						(1 << 3)
#define	DMA_EN						(1 << 2)
#define	REPEATED_START_FLAG 		(1 << 1)
#define	STOP_FLAG					(0 << 1)

enum {
	ST_MODE,
	FS_MODE,
	HS_MODE,
};

struct mt6516_i2c {
	struct i2c_adapter	*adap;		/* i2c host adapter */
	struct device		*dev;		/* the device object of i2c host adapter */
	u32					base;		/* i2c base addr */
	u16					id;
	u16					irqnr;		/* i2c interrupt number */
	u16					irq_stat;	/* i2c interrupt status */
	spinlock_t			lock;		/* for mt6516_i2c struct protection */
	wait_queue_head_t	wait;		/* i2c transfer wait queue */
	struct mt_dma_conf*	dma_tx;		/* dma tx config */
	struct mt_dma_conf*	dma_rx;		/* dma rx config */
	u32					dma_dir;	/* dma transfer direction */
	atomic_t			dma_in_progress;

	atomic_t			trans_err;	/* i2c transfer error */
	atomic_t			trans_comp;	/* i2c transfer completion */
	atomic_t			trans_stop;	/* i2c transfer stop */

	unsigned long		clk;		/* host clock speed in khz */
	unsigned long		sclk;		/* khz */

	unsigned char		master_code;/* master code in HS mode */
	unsigned char		mode;		/* ST/FS/HS mode */
};

typedef struct {
	u32 data:8;
	u32 reserved:24;
} i2c_data_reg;

typedef struct {
	u32 slave_addr:8;
	u32 reserved:24;
} i2c_slave_addr_reg;

typedef struct {
	u32 transac_comp:1;
	u32 ackerr:1;
	u32 hs_nackerr:1;
	u32 debug:1;
	u32 reserved:28;
} i2c_intr_mask_reg;

typedef struct {
	u32 transac_comp:1;
	u32 ackerr:1;
	u32 hs_nackerr:1;
	u32 reserved:29;
} i2c_intr_stat_reg;

typedef struct {
	u32 reserved1:1;
	u32 rs_stop:1;
	u32 dma_en:1;
	u32 clk_ext_en:1;
	u32 dir_chg:1;
	u32 ackerr_det_en:1;
	u32 trans_len_chg:1;
	u32 reserved2:25;
} i2c_control_reg;

typedef struct {
	u32 trans_len:8;
	u32 trans_aux_len:5;
	u32 reserved:19;
} i2c_trans_len_reg;

typedef struct {
	u32 transac_len:8;
	u32 reserved:24;
} i2c_transac_len_reg;

typedef struct {
	u32 delay_len:8;
	u32 reserved:24;
} i2c_delay_len_reg;

typedef struct {
	u32 step_cnt_div:6;
	u32 reserved1:2;
	u32 sample_cnt_div:3;
	u32 reserved2:1;
	u32 data_rd_time:3;
	u32 data_rd_adj:1;
	u32 reserved3:16;
} i2c_timing_reg;

typedef struct {
	u32 start:1;
	u32 reserved:31;
} i2c_start_reg;

typedef struct {
	u32 rd_empty:1;
	u32 wr_full:1;
	u32 reserved1:2;
	u32 fifo_offset:4;
	u32 wr_addr:4;
	u32 rd_addr:4;
	u32 reserved3:16;
} i2c_fifo_stat_reg;

typedef struct {
	u32 rx_trig:3;
	u32 reserved1:5;
	u32 tx_trig:3;
	u32 reserved2:21;
} i2c_fifo_thresh_reg;

typedef struct {
	u32 scl_io_cfg:1;
	u32 sda_io_cfg:1;
	u32 io_sync_en:1;
	u32 reserved2:29;
} i2c_io_config_reg;

typedef struct {
	u32 debug:3;
	u32 reserved2:29;
} i2c_debug_reg;

typedef struct {
	u32 hs_en:1;
	u32 hs_nackerr_det_en:1;
	u32 reserved1:2;
	u32 master_code:3;
	u32 reserved2:1;
	u32 hs_step_cnt_div:3;
	u32 reserved3:1;
	u32 hs_sample_cnt_div:3;
	u32 reserved4:17;
} i2c_hs_reg;

typedef struct {
	u32 master_stat:4;
	u32 master_rd:1;
	u32 master_wr:1;
	u32 bus_busy:1;	
	u32 reserved:25;
} i2c_dbg_stat_reg;

typedef struct {
	u32 fifo_apb_dbg:1;
	u32 apb_dbg_rd:1;
	u32 reserved:30;
} i2c_dbg_ctrl_reg;

typedef struct {
	i2c_data_reg		*data;
	i2c_slave_addr_reg  *slave_addr;
	i2c_intr_mask_reg   *intr_mask;
	i2c_intr_stat_reg   *intr_stat;
	i2c_control_reg     *control;
	i2c_trans_len_reg   *trans_len;
	i2c_transac_len_reg *transac_len;
	i2c_delay_len_reg   *delay_len;
	i2c_timing_reg      *timing;
	i2c_start_reg       *start;
	i2c_fifo_stat_reg   *fifo_stat;
	i2c_fifo_thresh_reg *fifo_thresh;
	i2c_io_config_reg   *io_config;
	i2c_debug_reg       *debug;
	i2c_hs_reg			*hs;
	i2c_dbg_stat_reg    *dbg_stat;
	i2c_dbg_ctrl_reg    *dbg_ctrl;
} i2c_regs;

//extern BOOL hwEnableClock(mt6516_CLOCK clockId);
//extern BOOL hwDisableClock(mt6516_CLOCK clockId);

inline static void mt6516_i2c_power_up(struct mt6516_i2c *i2c)
{
    //2009/05/04: MT6516, the I2C power on is controlled by APMCUSYS_PDN_CLR0
    //            Bit 3: I2C3
    //            Bit18: I2C2
    //            Bit21: I2C
    #define PDN_CLR0 (0xF0039340)   //80039340h
    u32 pwrbit[] = {21, 18, 3};
    unsigned int poweron = 1 << pwrbit[i2c->id];
    I2C_SET_BITS(poweron, PDN_CLR0);
}

inline static void mt6516_i2c_power_down(struct mt6516_i2c *i2c)
{
    //2009/05/04: MT6516, the I2C power on is controlled by APMCUSYS_PDN_SET0
    //            Bit 3: I2C3
    //            Bit18: I2C2
    //            Bit21: I2C    
    #define PDN_SET0 (0xF0039320)   //80039320h
    u32 pwrbit[] = {21, 18, 3};
    unsigned int poweroff = 1 << pwrbit[i2c->id];
    I2C_SET_BITS(poweroff, PDN_SET0);
}

static i2c_regs mt6516_i2c_regs[3];
static void mt6516_i2c_init_regs(struct mt6516_i2c *i2c)
{
    u32 base = i2c->base;
	i2c_regs *p = &mt6516_i2c_regs[i2c->id];
	p->data       = (i2c_data_reg*)mt6516_I2C_DATA_PORT;
	p->slave_addr = (i2c_slave_addr_reg*)mt6516_I2C_SLAVE_ADDR;
	p->intr_mask = (i2c_intr_mask_reg*)mt6516_I2C_INTR_MASK;
	p->intr_stat = (i2c_intr_stat_reg*)mt6516_I2C_INTR_STAT;
	p->control = (i2c_control_reg*)mt6516_I2C_CONTROL;
	p->trans_len = (i2c_trans_len_reg*)mt6516_I2C_TRANSFER_LEN;
	p->transac_len = (i2c_transac_len_reg*)mt6516_I2C_TRANSAC_LEN;
	p->delay_len = (i2c_delay_len_reg*)mt6516_I2C_DELAY_LEN;
	p->timing = (i2c_timing_reg*)mt6516_I2C_TIMING;
	p->start = (i2c_start_reg*)mt6516_I2C_START;
	p->fifo_stat = (i2c_fifo_stat_reg*)mt6516_I2C_FIFO_STAT;
	p->fifo_thresh = (i2c_fifo_thresh_reg*)mt6516_I2C_FIFO_THRESH;
	p->io_config = (i2c_io_config_reg*)mt6516_I2C_IO_CONFIG;
	p->debug = (i2c_debug_reg*)mt6516_I2C_DEBUG;
	p->hs = (i2c_hs_reg*)mt6516_I2C_HS;
	p->dbg_stat = (i2c_dbg_stat_reg*)mt6516_I2C_DEBUGSTAT;
	p->dbg_ctrl = (i2c_dbg_ctrl_reg*)mt6516_I2C_DEBUGCTRL;		
}

static u32 mt6516_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_EMUL;
}

static void mt6516_i2c_post_isr(struct mt6516_i2c *i2c, u16 addr)
{
	u32 base = i2c->base;

	if (i2c->irq_stat & I2C_TRANSAC_COMP) {
		atomic_set(&i2c->trans_err, 0);
		if ((atomic_read(&i2c->dma_in_progress) &&
		    I2C_FIFO_IS_EMPTY) ||
		    !atomic_read(&i2c->dma_in_progress))
		{
			atomic_set(&i2c->trans_comp, 1);
		}
		dev_dbg(i2c->dev, "I2C_TRANSAC_COMP\n");
	}
	
	if (i2c->irq_stat & I2C_HS_NACKERR) {
		if (likely(!(addr & I2C_A_FILTER_MSG)))
			dev_err(i2c->dev, "I2C_HS_NACKERR\n");
	}
	if (i2c->irq_stat & I2C_ACKERR) {
		if (likely(!(addr & I2C_A_FILTER_MSG)))
			dev_err(i2c->dev, "I2C_ACKERR\n");		
	}
	atomic_set(&i2c->trans_err, i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR));
}

static int mt6516_i2c_start_poll_xfer(struct mt6516_i2c *i2c, struct i2c_msg *msg)
{
    u32 base = i2c->base;
	u16 addr = msg->addr;
	u16 flags = msg->flags;
	u16 read = (flags & I2C_M_RD);
	u16 len = msg->len;
	u8 *ptr = msg->buf;
	long tmo = i2c->adap->timeout;
	int ret = len;

	BUG_ON(len > I2C_FIFO_SIZE);

	if (!len)	/* CHECKME. mt6516 doesn't support len = 0. */
		return 0;

	atomic_set(&i2c->trans_stop, 0);
	atomic_set(&i2c->trans_comp, 0);
	atomic_set(&i2c->trans_err, 0);

	addr = read ? (addr | 0x1) : (addr & ~0x1);

	I2C_SET_SLAVE_ADDR(addr);
	I2C_SET_TRANS_LEN(len);
	I2C_SET_TRANSAC_LEN(1);
	I2C_SET_INTR_MASK(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP);
	I2C_FIFO_CLR_ADDR;

	if (i2c->mode == HS_MODE)
		I2C_SET_TRANS_CTRL(ACK_ERR_DET_EN | CLK_EXT | REPEATED_START_FLAG);
	else
		I2C_SET_TRANS_CTRL(ACK_ERR_DET_EN | CLK_EXT | STOP_FLAG);
	
	if (read) {
		I2C_START_TRANSAC;
		tmo = wait_event_interruptible_timeout(
				i2c->wait,
				atomic_read(&i2c->trans_stop),
				tmo);
		mt6516_i2c_post_isr(i2c, addr);
		
		if (atomic_read(&i2c->trans_comp)) {
			while (len--) {
				I2C_READ_BYTE(*ptr);
				dev_dbg(i2c->dev, "read byte = 0x%.2X\n", *ptr);
				ptr++;
			}
		}
	}
	else {
		while (len--) {
			I2C_WRITE_BYTE(*ptr);
			dev_dbg(i2c->dev, "write byte = 0x%.2X\n", *ptr);
			ptr++;
		}
		I2C_START_TRANSAC;
		tmo = wait_event_interruptible_timeout(i2c->wait, 
		                               atomic_read(&i2c->trans_stop), tmo);
		mt6516_i2c_post_isr(i2c, addr);
	}
	I2C_CLR_INTR_MASK(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP);

	if (tmo == 0) {
		if (likely(!(addr & I2C_A_FILTER_MSG)))
			dev_err(i2c->dev, "addr: %.2x, transfer timeout\n", msg->addr);
		ret = -ETIMEDOUT;
	}
	else if (atomic_read(&i2c->trans_err)) {
		if (likely(!(addr & I2C_A_FILTER_MSG)))
			dev_err(i2c->dev, "addr: %.2x, transfer error\n", msg->addr);
		ret = -EREMOTEIO;
	}
	return ret;
}

#ifdef CFG_I2C_DMA_MODE
static void mt6516_i2c_start_dma(struct mt6516_i2c *i2c, struct scatterlist *sg, 
                                 enum dma_data_direction dir)
{
#ifdef CFG_FULL_SIZE_DMA
	struct mt_dma_conf *dma;
	u32 len  = sg_dma_len(sg);
	u32 addr = sg_dma_address(sg);	

	dma = (dir == DMA_FROM_DEVICE) ? i2c->dma_rx : i2c->dma_tx;

	if (likely((((u32)addr % sizeof(u32)) == 0) && (len % sizeof(u32) == 0))) {
		dma->count = len >> 2;
		dma->size  = DMA_CON_SIZE_LONG;
		dma->burst = DMA_CON_BURST_4BEAT;
	}
	else {
		dma->count = len;
		dma->size  = DMA_CON_SIZE_BYTE;
		dma->burst = DMA_CON_BURST_SINGLE;
	}

	if (dir == DMA_FROM_DEVICE) {
		dma->dir	 = DMA_TRUE;  /* write to memory */
		dma->sinc	 = DMA_FALSE;
		dma->dinc	 = DMA_TRUE;	
		dma->src	 = mt6516_I2C_DATA_PORT - IO_OFFSET;
		dma->dst	 = addr;
	}
	else {
		dma->dir	 = DMA_FALSE; /* read from memory */		
		dma->sinc	 = DMA_TRUE;
		dma->dinc	 = DMA_FALSE;	
		dma->src	 = addr;
		dma->dst	 = mt6516_I2C_DATA_PORT - IO_OFFSET;
	}
	atomic_set(&i2c->dma_in_progress, 1);	
	mt_config_dma(dma, ALL);
	mt_start_dma(dma);
#else
	struct mt_dma_conf *dma;
	u32 len  = sg_dma_len(sg);
	u32 addr = sg_dma_address(sg);

	dma = (dir == DMA_FROM_DEVICE) ? i2c->dma_rx : i2c->dma_tx;
    dma->pgmaddr = addr;
    
	if (likely((((u32)addr % sizeof(u32)) == 0) && (len % sizeof(u32) == 0))) {
		dma->count = len >> 2;
		dma->b2w   = DMA_FALSE;
		dma->size  = DMA_CON_SIZE_LONG;
		dma->burst = DMA_CON_BURST_4BEAT;
	}
	else {
		dma->count = len;
		dma->b2w   = DMA_TRUE;
		dma->size  = DMA_CON_SIZE_BYTE;
		dma->burst = DMA_CON_BURST_SINGLE;
	}

	if (dir == DMA_FROM_DEVICE) {
		dma->dir     = DMA_TRUE; /* write to memory */
		dma->sinc    = DMA_FALSE;
		dma->dinc    = DMA_TRUE;
	}
	else {
		dma->dir  = DMA_FALSE; /* read from memory */		
		dma->sinc = DMA_TRUE;
		dma->dinc = DMA_FALSE;
	}
	atomic_set(&i2c->dma_in_progress, 1);	
	mt_config_dma(dma, ALL);
	mt_start_dma(dma);
#endif	
}

static void mt6516_i2c_stop_dma(struct mt6516_i2c *i2c, 
                                enum dma_data_direction dir)
{
	struct mt_dma_conf *dma;

	dma = (dir == DMA_FROM_DEVICE) ? i2c->dma_rx : i2c->dma_tx;
	mt_stop_dma(dma);
	atomic_set(&i2c->dma_in_progress, 0);
}

#if 0 /* reference */
static void mt6516_uart_dma_setup(struct mt6516_uart *uart,
                                  struct mt6516_uart_dma *dma,
                                  struct scatterlist *sg)
{
    struct scatterlist *cur_sg = &dma->sg[0];
    unsigned int addr, start_addr;
    unsigned int total_len, len = 0;

    start_addr = sg_dma_address(sg);
    total_len  = sg_dma_len(sg);
    
    MSG(DMA, "setup dma (addr = 0x%.8x, size = %d)\n", start_addr, total_len);
    while (len < total_len) {
        addr = start_addr + len;
        sg_dma_address(cur_sg) = addr;

        if (total_len - len < 4) {
            sg_dma_len(cur_sg) = total_len - len;
        } else {
            sg_dma_len(cur_sg) = addr % 4 ? 
                                 (4 - (addr % 4)) : ((total_len - len) & ~3);
        }        
        MSG(DMA, "dma->sg[%d] (addr = 0x%.8x, size = %d)\n", 
            cur_sg - &dma->sg[0], sg_dma_address(cur_sg), sg_dma_len(cur_sg));
        
        len += sg_dma_len(cur_sg++);
    }
    dma->sg_idx = 0;
    dma->sg_num = cur_sg - &dma->sg[0];    
}
#endif

static int mt6516_i2c_start_dma_xfer(struct mt6516_i2c *i2c, struct i2c_msg *msg)
{
    u32 base = i2c->base;
	u16 trans_num;
	u16 flags = msg->flags, addr = msg->addr;
	u16 last_trans_len;
	u8 *buf = msg->buf;
	int ret = msg->len;	
	long tmo;
	struct scatterlist sg;
	enum dma_data_direction dir;

	BUG_ON(msg->len > MAX_DMA_TRANS_SIZE * MAX_DMA_TRANS_NUM);

	if (!msg->len)	/* CHECKME. mt6516 doesn't support len = 0. */
		return 0;

	dir  = (flags & I2C_M_RD) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
	addr = (flags & I2C_M_RD) ? (addr | 0x1) : (addr & ~0x1);
	
	I2C_SET_SLAVE_ADDR(addr);
	I2C_SET_INTR_MASK(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP);
	I2C_SET_FIFO_THRESH(7, 0);	
	I2C_FIFO_CLR_ADDR;

	if (i2c->mode == HS_MODE)
		I2C_SET_TRANS_CTRL(ACK_ERR_DET_EN | CLK_EXT | DMA_EN | REPEATED_START_FLAG);
	else
		I2C_SET_TRANS_CTRL(ACK_ERR_DET_EN | CLK_EXT | DMA_EN | STOP_FLAG);

	trans_num      = (msg->len + MAX_DMA_TRANS_SIZE + 1) / MAX_DMA_TRANS_SIZE;
	last_trans_len = msg->len % MAX_DMA_TRANS_SIZE;
	

	if (trans_num > 1) {
		if (last_trans_len)
			trans_num--;
		I2C_SET_TRANS_LEN(MAX_DMA_TRANS_SIZE);
		I2C_SET_TRANSAC_LEN(trans_num);
		sg_init_one(&sg, buf, trans_num * MAX_DMA_TRANS_SIZE);
		dma_map_sg(i2c->dev, &sg, 1, dir);
		mt6516_i2c_start_dma(i2c, &sg, dir);
		atomic_set(&i2c->trans_stop, 0);
		atomic_set(&i2c->trans_comp, 0);
		atomic_set(&i2c->trans_err, 0);
		tmo = i2c->adap->timeout;
		I2C_START_TRANSAC;
		tmo = wait_event_interruptible_timeout(i2c->wait, 
		                               atomic_read(&i2c->trans_stop), tmo);		
		mt6516_i2c_stop_dma(i2c, dir);

		mt6516_i2c_post_isr(i2c, addr);
		
		dma_unmap_sg(i2c->dev, &sg, 1, dir);
		if (tmo == 0) {
			dev_err(i2c->dev, "addr: %.2x, transfer timeout\n", msg->addr);
			ret = -ETIMEDOUT;
			goto end;
		}
		else if (atomic_read(&i2c->trans_err)) {
			dev_err(i2c->dev, "addr: %.2x, transfer error\n", msg->addr);
			ret = -EREMOTEIO;
			goto end;
		}
		buf  = buf + trans_num * MAX_DMA_TRANS_SIZE;
	}
	if (last_trans_len) {
		I2C_SET_TRANS_LEN(last_trans_len);
		I2C_SET_TRANSAC_LEN(1);
		sg_init_one(&sg, buf, last_trans_len);
		dma_map_sg(i2c->dev, &sg, 1, dir);
		mt6516_i2c_start_dma(i2c, &sg, dir);
		atomic_set(&i2c->trans_comp, 0);
		atomic_set(&i2c->trans_err, 0);
		tmo = i2c->adap->timeout;
		I2C_START_TRANSAC;
		tmo = wait_event_interruptible_timeout(i2c->wait, 
		                               atomic_read(&i2c->trans_stop), tmo);
		mt6516_i2c_stop_dma(i2c, dir);

		mt6516_i2c_post_isr(i2c, addr);
		
		dma_unmap_sg(i2c->dev, &sg, 1, dir);
		if (tmo == 0) {
			dev_err(i2c->dev, "addr: %.2x, transfer timeout\n", msg->addr);
			ret = -ETIMEDOUT;
			goto end;
		}
		else if (atomic_read(&i2c->trans_err)) {
			dev_err(i2c->dev, "addr: %.2x, transfer error\n", msg->addr);
			ret = -EREMOTEIO;
			goto end;
		}		
	}
	
end:
	I2C_CLR_INTR_MASK(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP);
	return ret;
}
#endif

static __tcmfunc irqreturn_t mt6516_i2c_irq(int irqno, void *dev_id)
{
	struct mt6516_i2c *i2c;
	u32 base;

	i2c = (struct mt6516_i2c*)dev_id;
	base = i2c->base;

	i2c->irq_stat = I2C_INTR_STATUS;
//	dev_dbg(i2c->dev, "I2C interrupt status 0x%04X\n", i2c->irq_stat);

	// for debugging
	// beside I2C_HS_NACKERR, I2C_ACKERR, and I2C_TRANSAC_COMP,
	// all other bits of INTR_STAT should be all zero.
//	BUG_ON(i2c->irq_stat & ~(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));

	atomic_set(&i2c->trans_stop, 1);
	wake_up_interruptible(&i2c->wait);

	I2C_CLR_INTR_STATUS(I2C_HS_NACKERR|I2C_ACKERR|I2C_TRANSAC_COMP);

	return IRQ_HANDLED;
}

static int mt6516_i2c_do_transfer(struct mt6516_i2c *i2c, struct i2c_msg *msgs, int num)
{
	int ret = 0;
	int left_num = num;

	while (left_num--) {
#ifdef CFG_I2C_DMA_MODE	
		if (!atomic_read(&i2c->dma_in_progress))
			ret = mt6516_i2c_start_dma_xfer(i2c, msgs++);
		else
#endif		
			ret = mt6516_i2c_start_poll_xfer(i2c, msgs++);
		if (ret < 0)
			return -EAGAIN;
	}
    /*the return value is number of executed messages*/
	return num;
}

static int mt6516_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct mt6516_i2c *i2c = i2c_get_adapdata(adap);
	
	int	retry;
	int	ret;
	
	long	old_timing;
	long	new_timing;
	long	old_hs;
	long	new_hs;
	long	tmp;
	u32		base;

#define MT6516_I2C_TIMING_MASK		(0x073F)
#define MT6516_I2C_HS_MASK		(0x7700)

	// some slaves need a special timing, so we use a black list to
	// check if SAMPLE_CNT_DIV and STEP_CNT_DIV need a adjustment.
	base = i2c->base;
	if (unlikely(msgs->addr & I2C_A_CHANGE_TIMING)) {
		new_timing = msgs->timing & MT6516_I2C_TIMING_MASK;
		new_hs = (msgs->timing >> 16) & MT6516_I2C_HS_MASK;
		spin_lock(&i2c->lock);	
		old_timing = __raw_readw(mt6516_I2C_TIMING);
		old_hs     = __raw_readw(mt6516_I2C_HS    );
		tmp  = old_timing & ~(MT6516_I2C_TIMING_MASK);
		tmp |= new_timing;
		__raw_writew(tmp, mt6516_I2C_TIMING);
		tmp  = old_hs & ~(MT6516_I2C_HS_MASK);
		tmp |= new_hs;
		__raw_writew(tmp, mt6516_I2C_HS);
		spin_unlock(&i2c->lock);
	}
	

    //hwEnableClock(mt6516_CLOCK_I2C,"I2C");
    mt6516_i2c_power_up(i2c);

	for (retry = 0; retry < adap->retries; retry++) {

		ret = mt6516_i2c_do_transfer(i2c, msgs, num);
		if (ret != -EAGAIN) {
		    //hwDisableClock(mt6516_CLOCK_I2C,"I2C");
//		    mt6516_i2c_power_down(i2c);
//			return ret;
			break;
		}

		dev_dbg(i2c->dev, "Retrying transmission (%d)\n", retry);
		udelay(100);
	}
	
	//hwDisableClock(mt6516_CLOCK_I2C,"I2C");
	mt6516_i2c_power_down(i2c);
	
	// restore the previous timing settings
	if (unlikely(msgs->addr & I2C_A_CHANGE_TIMING)) {
		spin_lock(&i2c->lock);
		__raw_writew(old_timing, mt6516_I2C_TIMING);
		__raw_writew(old_hs, mt6516_I2C_HS);
		spin_unlock(&i2c->lock);
	}

	if (ret != -EAGAIN)
		return ret;
	else
		return -EREMOTEIO;
}


static void mt6516_i2c_free(struct mt6516_i2c *i2c)
{
	if (!i2c)
		return;

	free_irq(i2c->irqnr, i2c);
#if 0	
	if (i2c->dma_tx) {
		mt_stop_dma(i2c->dma_tx);
		mt_free_dma(i2c->dma_tx);		
	}
	if (i2c->dma_rx) {
		mt_stop_dma(i2c->dma_rx);
		mt_free_dma(i2c->dma_rx);
	}
#endif	
	if (i2c->adap)
		i2c_del_adapter(i2c->adap);
	kfree(i2c);
}

static int mt6516_i2c_set_speed(struct mt6516_i2c *i2c, int mode, unsigned long khz)
{
	u32 base = i2c->base;
	int ret = 0;
	unsigned short sample_cnt_div, step_cnt_div;
	unsigned short max_step_cnt_div = (mode == HS_MODE) ? 
	                                  MAX_HS_STEP_CNT_DIV : MAX_STEP_CNT_DIV;
	unsigned long tmp, sclk, hclk = i2c->clk;
	
	BUG_ON((mode == FS_MODE && khz > MAX_FS_MODE_SPEED) || 
		   (mode == ST_MODE && khz > MAX_ST_MODE_SPEED) ||
		   (mode == HS_MODE && khz > MAX_HS_MODE_SPEED));

	spin_lock(&i2c->lock);
#if 1
	{
		unsigned long diff, min_diff = I2C_CLK_RATE;
		unsigned short sample_div = MAX_SAMPLE_CNT_DIV;
		unsigned short step_div = max_step_cnt_div;
		for (sample_cnt_div = 1; sample_cnt_div <= MAX_SAMPLE_CNT_DIV; sample_cnt_div++) {
			for (step_cnt_div = 1; step_cnt_div <= max_step_cnt_div; step_cnt_div++) {
				sclk = (hclk >> 1) / (sample_cnt_div * step_cnt_div);
				if (sclk > khz) 
					continue;
				diff = khz - sclk;
				if (diff < min_diff) {
					min_diff = diff;
					sample_div = sample_cnt_div;
					step_div   = step_cnt_div;
				}											
			}
		}
		sample_cnt_div = sample_div;
		step_cnt_div   = step_div;
	}
#else
	for (sample_cnt_div = 1; sample_cnt_div <= MAX_SAMPLE_CNT_DIV; sample_cnt_div++) {
		tmp = khz * 2 * sample_cnt_div;
		step_cnt_div = (hclk + tmp - 1) / tmp;
		if (step_cnt_div <= max_step_cnt_div)
			break;
	}

	if (sample_cnt_div > MAX_SAMPLE_CNT_DIV)
		sample_cnt_div = MAX_SAMPLE_CNT_DIV;
	if (step_cnt_div > max_step_cnt_div)
		step_cnt_div = max_step_cnt_div;
#endif

	sclk = hclk / (2 * sample_cnt_div * step_cnt_div);
	if (sclk > khz) {
		dev_err(i2c->dev, "%s mode: unsupported speed (%ldkhz)\n", 
	           (mode == HS_MODE) ? "HS" : "ST/FT", khz);
		ret = -ENOTSUPP;
		goto end;
	}
	step_cnt_div--;
	sample_cnt_div--;

	if (mode == HS_MODE) {
		tmp  = __raw_readw(mt6516_I2C_HS) & ~((0x7 << 12) | (0x7 << 8));
		tmp  = (sample_cnt_div & 0x7) << 12 | (step_cnt_div & 0x7) << 8 | tmp;
		__raw_writew(tmp, mt6516_I2C_HS);
		I2C_SET_HS_MODE(1);
	}
	else {
		tmp  = __raw_readw(mt6516_I2C_TIMING) & ~((0x7 << 8) | (0x1f << 0));
		tmp  = (sample_cnt_div & 0x7) << 8 | (step_cnt_div & 0x1f) << 0 | tmp;
		__raw_writew(tmp, mt6516_I2C_TIMING);
		I2C_SET_HS_MODE(0);
	}
	i2c->mode = mode;
	i2c->sclk = sclk;
	dev_dbg(i2c->dev, "mt6516-i2c: set sclk to %ldkhz (orig: %ldkhz)\n", sclk, khz);
end:
	spin_unlock(&i2c->lock);
	return ret;
}

static void mt6516_i2c_init_hw(struct mt6516_i2c *i2c)
{
    u32 base = i2c->base;
    /* reset i2c */
    //mt6516_wdt_SW_MCUPeripheralReset(mt6516_MCU_PERI_I2C);

    /* set clock timing */
    mt6516_i2c_set_speed(i2c, ST_MODE, MAX_ST_MODE_SPEED);
    //mt6516_i2c_set_speed(i2c, FS_MODE, MAX_FS_MODE_SPEED);
    if (i2c->id == 2) {
	dev_dbg(i2c->dev, "mt6516_i2c_init_hw(2):setup SCL_IO_CONFIG\n");
	__raw_writew(0x0001, mt6516_I2C_IO_CONFIG);     //setup SCL_IO_CONFIG
    }
#ifdef CFG_I2C_HIGH_SPEED_MODE
    mt6516_i2c_set_speed(i2c, HS_MODE, MAX_HS_MODE_SPEED);
#endif

    I2C_SET_TRANS_CTRL(ACK_ERR_DET_EN | CLK_EXT);
}

static struct i2c_algorithm mt6516_i2c_algorithm = {    
    .master_xfer   = mt6516_i2c_transfer,
	.smbus_xfer    = NULL,
	.functionality = mt6516_i2c_functionality,
};

static struct i2c_adapter mt6516_i2c_adaptor[] = {
    {
        .id                = 0,
     	.owner             = THIS_MODULE,
    	.name			   = "mt6516-i2c",
    	.algo			   = &mt6516_i2c_algorithm,
    	.algo_data         = NULL,
    	.client_register   = NULL,
    	.client_unregister = NULL,
    	.timeout           = 0.5 * HZ,
    	.retries		   = 1, 
    },
    {
        .id                = 1,
     	.owner             = THIS_MODULE,
    	.name			   = "mt6516-i2c",
    	.algo			   = &mt6516_i2c_algorithm,
    	.algo_data         = NULL,
    	.client_register   = NULL,
    	.client_unregister = NULL,
    	.timeout           = 0.5 * HZ,
    	.retries		   = 1, 
    },
    {
        .id                = 2,
     	.owner             = THIS_MODULE,
    	.name			   = "mt6516-i2c",
    	.algo			   = &mt6516_i2c_algorithm,
    	.algo_data         = NULL,
    	.client_register   = NULL,
    	.client_unregister = NULL,
    	.timeout           = 0.5 * HZ,
    	.retries		   = 1, 
    },    
};

static int mt6516_i2c_probe(struct device *dev)
{
	int ret, irq;
	struct platform_device *pdev = to_platform_device(dev);
	//struct mt_dma_conf *dma_tx, *dma_rx;
	struct mt6516_i2c *i2c = NULL;

	dev_dbg(i2c->dev, "mt6516-i2c : probe !!\n");

	/* Request IO memory */
	if (!request_mem_region(pdev->resource[0].start,
				            pdev->resource[0].end - pdev->resource[0].start + 1, 
				            pdev->name)) {
		return -EBUSY;
	}
	
	if (NULL == (i2c = kmalloc(sizeof(struct mt6516_i2c), GFP_KERNEL))) 
		return -ENOMEM;

	/* initialize i2c tx and rx dma */
#if 0
#ifdef CFG_FULL_SIZE_DMA
    dma_tx = mt_request_dma(DMA_FULL_CHANNEL);
    dma_rx = mt_request_dma(DMA_FULL_CHANNEL);
#else
	dma_tx = mt_request_dma(DMA_HALF_CHANNEL);
	dma_rx = mt_request_dma(DMA_HALF_CHANNEL);
#endif
#endif
	//BUG_ON(!dma_tx || !dma_rx);

#if 0	
	mt_reset_dma(dma_tx);
	mt_reset_dma(dma_rx);
#endif	

#if 0	
	dma_tx->mas      = DMA_CON_MASTER_I2CTX;
	dma_tx->iten     = DMA_FALSE;	 /* interrupt mode */
	dma_tx->dreq     = DMA_TRUE;	 /* enable hardware handshaking */
	dma_tx->limiter  = 0;
	dma_tx->data     = NULL;
	dma_tx->callback = NULL;

	dma_rx->mas      = DMA_CON_MASTER_I2CRX;
	dma_rx->iten     = DMA_FALSE;	 /* interrupt mode */
	dma_rx->dreq     = DMA_TRUE;	 /* enable hardware handshaking */
	dma_rx->limiter  = 0;
	dma_rx->data     = NULL;
	dma_rx->callback = NULL;
#endif

	/* initialize mt6516_i2c structure */
	irq = pdev->resource[1].start;
	i2c->id = pdev->id;
	i2c->base = pdev->resource[0].start;
	i2c->irqnr = irq;
	i2c->clk    = I2C_CLK_RATE;	
	i2c->adap   = &mt6516_i2c_adaptor[pdev->id];
	i2c->dev    = &mt6516_i2c_adaptor[pdev->id].dev;
	//i2c->dma_tx = dma_tx;
	//i2c->dma_rx = dma_rx;
	spin_lock_init(&i2c->lock);	
	init_waitqueue_head(&i2c->wait);
	atomic_set(&i2c->dma_in_progress, 0);

	ret = request_irq(irq, mt6516_i2c_irq, 0, DRV_NAME, i2c);

	if (ret) goto free;

	mt6516_i2c_init_regs(i2c);
	mt6516_i2c_init_hw(i2c);

	i2c_set_adapdata(i2c->adap, i2c);
	if (2 == i2c->id) {
		i2c->adap->nr = i2c->id;
		ret = i2c_add_numbered_adapter(i2c->adap);
	}
	else {
		ret = i2c_add_adapter(i2c->adap);
	}

	if (ret) goto free;

	dev_set_drvdata(dev, i2c);

	return ret;
	
free:
	mt6516_i2c_free(i2c);
	return ret;
}

static int mt6516_i2c_remove(struct device *dev)
{
	struct mt6516_i2c *i2c = dev_get_drvdata(dev);
	
	if (i2c) {
		dev_set_drvdata(dev, NULL);
		mt6516_i2c_free(i2c);
	}

	return 0;
}

#ifdef CONFIG_PM
static int mt6516_i2c_suspend(struct device *dev, pm_message_t state)
{
    struct mt6516_i2c *i2c = dev_get_drvdata(dev); 
    dev_dbg(i2c->dev,"[I2C %d] Suspend!\n", i2c->id);
    
    #if 0
    struct mt6516_i2c *i2c = dev_get_drvdata(dev);    
    
    if (i2c) {
        dev_dbg(i2c->dev,"[I2C] Suspend!\n");
        /* Check if i2c bus is already in used by other modules or not.
         * Parent module should be stopped or wait for i2c bus completed before
         * entering i2c suspend mode.
         */
        WARN_ON(PDN_Get_Peri_Status(PDN_PERI_I2C) == KAL_FALSE);        
		while (PDN_Get_Peri_Status(PDN_PERI_I2C) == KAL_FALSE)
			msleep(10);
    }
    #endif
    return 0;
}

static int mt6516_i2c_resume(struct device *dev)
{
    struct mt6516_i2c *i2c = dev_get_drvdata(dev); 
    dev_dbg(i2c->dev,"[I2C %d] Resume!\n", i2c->id);
    
    #if 0
    struct mt6516_i2c *i2c = dev_get_drvdata(dev);

    if (i2c) {
        dev_dbg(i2c->dev,"[I2C] Resume!\n");
        mt6516_i2c_init_hw(i2c);
    }
    #endif
    return 0;
}
#endif

/* device driver for platform bus bits */
static struct device_driver mt6516_i2c_driver = {
	.name	 = DRV_NAME,
	.bus	 = &platform_bus_type,
	.probe	 = mt6516_i2c_probe,
	.remove	 = mt6516_i2c_remove,
#ifdef CONFIG_PM
	.suspend = mt6516_i2c_suspend,
	.resume	 = mt6516_i2c_resume,
#endif
};

static int __init mt6516_i2c_init(void)
{
	int ret;

	ret = driver_register(&mt6516_i2c_driver);

	return ret;
}

static void __exit mt6516_i2c_exit(void)
{
	driver_unregister(&mt6516_i2c_driver);
}

module_init(mt6516_i2c_init);
module_exit(mt6516_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek mt6516 I2C Bus Driver");
MODULE_AUTHOR("Infinity Chen <infinity.chen@mediatek.com>");

