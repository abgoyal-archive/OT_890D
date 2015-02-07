
 
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>

#include <mach/dma.h>
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_pwm.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_gpt_sw.h>
#include <asm/tcm.h>

#include "mt6516_sd.h"

//#define CFG_PROFILING
//#define CFG_PROFILING_DUMP
#define DRV_NAME            "mt6516-sd"

#if defined(CONFIG_MT6516_CPU_416MHZ_MCU_104MHZ) || \
    defined(CONFIG_MT6516_CPU_208MHZ_MCU_104MHZ)
#define MCU_CLK_FREQ        (104000000)         /* 104MHz */
#else
#define MCU_CLK_FREQ        (52000000)          /* 52MHz */
#endif
#define MCPLL_CLK_FREQ      (91000000)          /* 91MHz */

#define DMA_MAX_COUNT       (32768)

#define HOST_MAX_NUM        (3)
#define HOST_MAX_SCLK       (26000000)          /* 26MHz  */
#define HOST_MIN_SCLK       (260000)            /* 260kHz */
#define HOST_INI_SCLK       (HOST_MIN_SCLK)
#define HOST_MAX_BLKSZ      (2048)

/* Tuning Parameter */
#define DEFAULT_DEBOUNCE    (8)       /* 8 cycles */
#define DEFAULT_DLT         (1)       /* data latch timing */
#define DEFAULT_DTOC        (40)      /* data timeout counter. 65536x40 sclk. */
#define DEFAULT_WDOD        (0)       /* write data output delay. no delay. */
#define DEFAULT_BSYDLY      (8)       /* card busy delay. 8 extend sclk */

#define CMD_TIMEOUT         (100/(1000/HZ)) /* 100ms */

#define MAX_FIFOTHD         (8)       /* the maximun fifo threshold */
#define PRD_FIFOTHD         (8)       /* default fifo threshold of pio read */
#define PWR_FIFOTHD         (1)       /* default fifo threshold of pio write */

/* Debug message event */
#define DBG_EVT_NONE        (0)       /* No event */
#define DBG_EVT_DMA         (1 << 0)  /* DMA related event */
#define DBG_EVT_CMD         (1 << 1)  /* MSDC CMD related event */
#define DBG_EVT_RSP         (1 << 2)  /* MSDC CMD RSP related event */
#define DBG_EVT_INT         (1 << 3)  /* MSDC INT event */
#define DBG_EVT_CFG         (1 << 4)  /* MSDC CFG event */
#define DBG_EVT_FUC         (1 << 5)  /* Function event */
#define DBG_EVT_OPS         (1 << 6)  /* Read/Write operation event */
#define DBG_EVT_WRN         (1 << 7)  /* Warning event */
#define DBG_EVT_ALL         (0xffffffff)

#define DBG_EVT_MASK        (DBG_EVT_ALL)

#ifdef CONFIG_MMC_DEBUG
#ifndef MT6516_SD_DEBUG
#define MT6516_SD_DEBUG
#endif

static struct mt6516_sd_regs *mt6516_sd_reg[HOST_MAX_NUM];

static unsigned long mt6516_sd_evt_mask[] = {
    DBG_EVT_MASK,
    DBG_EVT_NONE,
    DBG_EVT_NONE
};

#define MSG(evt, fmt, args...) \
do {    \
    if ((DBG_EVT_##evt) & mt6516_sd_evt_mask[host->id]) { \
        printk(fmt, ##args); \
    }   \
} while(0)
#else
#define MSG(evt, fmt, args...)  do{}while(0)
#endif

#define msdc_clr_fifo()         sdr_set_bits(MSDC_STA, MSDC_STA_FIFOCLR)	
#define msdc_is_fifo_empty()   (sdr_read16(MSDC_STA) & MSDC_STA_BE)
#define msdc_is_fifo_full()    (sdr_read16(MSDC_STA) & MSDC_STA_BF)
#define msdc_is_data_req()     (sdr_read32(MSDC_STA) & MSDC_STA_DRQ)	
#define msdc_is_busy()         (sdr_read16(MSDC_STA) & MSDC_STA_BUSY)

#define msdc_get_fifo_cnt(c)    sdr_get_field(MSDC_STA, MSDC_STA_FIFOCNT, (c))
#define msdc_set_fifo_thd(v)    sdr_set_field(MSDC_CFG, MSDC_CFG_FIFOTHD, (v))
#define msdc_fifo_write(v)      sdr_write32(MSDC_DAT, (v))
#define msdc_fifo_read()        sdr_read32(MSDC_DAT)	

#define msdc_dma_on()           sdr_set_bits(MSDC_CFG, MSDC_CFG_DMAEN)
#define msdc_dma_off()          sdr_clr_bits(MSDC_CFG, MSDC_CFG_DMAEN)

#define msdc_fifo_irq_on()      sdr_set_bits(MSDC_CFG, MSDC_CFG_DIRQE)
#define msdc_fifo_irq_off()     sdr_clr_bits(MSDC_CFG, MSDC_CFG_DIRQE)

#define msdc_reset() \
    do { \
        sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
/*        while (sdr_read32(MSDC_CFG) & MSDC_CFG_RST){}; */	\
	mdelay(5); \
        sdr_clr_bits(MSDC_CFG, MSDC_CFG_RST); \
    } while(0)

#define msdc_lock_irqsave(val) \
    do { \
        sdr_get_field(MSDC_CFG, MSDC_CFG_INTEN, val); \
        sdr_clr_bits(MSDC_CFG, MSDC_CFG_INTEN); \
    } while(0)
	
#define msdc_unlock_irqrestore(val) \
    do { \
        sdr_set_field(MSDC_CFG, MSDC_CFG_INTEN, val); \
    } while(0)

#define msdc_set_bksz(sz)       sdr_set_field(SDC_CFG, SDC_CFG_BLKLEN, sz)

#define sdc_is_busy()          (sdr_read16(SDC_STA) & SDC_STA_BESDCBUSY)
#define sdc_is_cmd_busy()      (sdr_read16(SDC_STA) & SDC_STA_BECMDBUSY)
#define sdc_is_dat_busy()      (sdr_read16(SDC_STA) & SDC_STA_BEDATBUSY)
#define sdc_is_write_protect() (sdr_read16(SDC_STA) & SDC_STA_WP)
#define sdc_sclk_on()          sdr_set_bits(SDC_CFG, SDC_CFG_SIEN)
#define sdc_sclk_off()         sdr_clr_bits(SDC_CFG, SDC_CFG_SIEN)
#define sdc_send_cmd(cmd,arg) \
    do { \
        sdr_write32(SDC_ARG, (arg)); \
        sdr_write32(SDC_CMD, (cmd)); \
    } while(0)

#define is_card_present(h)     (((struct mt6516_sd_host*)(h))->card_inserted)

static int mt6516_sd_pio_read(struct mt6516_sd_host *host, struct mmc_data *data);
static int mt6516_sd_pio_write(struct mt6516_sd_host* host, struct mmc_data *data);
static void mt6516_sd_enable_cd_irq(struct mmc_host *mmc, int enable);

#ifdef CFG_PROFILING
struct mmc_op_report {
    unsigned long count;          /* the count of this operation */
    unsigned long min_time;       /* the min. time of this operation */
    unsigned long max_time;       /* the max. time of this operation */
    unsigned long total_time;     /* the total time of this operation */
    unsigned long total_size;     /* the total size of this operation */
};

struct mmc_op_perf {
    struct mt6516_sd_host *host;
    struct mmc_op_report   single_blk_read;
    struct mmc_op_report   single_blk_write;    
    struct mmc_op_report   multi_blks_read;
    struct mmc_op_report   multi_blks_write;
};

static struct mmc_op_perf sd_perf[HOST_MAX_NUM];
static int sd_perf_rpt[] = {1, 1, 0};

static void mmc_perf_report(struct mmc_op_report *rpt)
{
    printk(KERN_INFO "\tCount      : %ld\n", rpt->count);
    printk(KERN_INFO "\tMax. Time  : %ld counts\n", rpt->max_time);
    printk(KERN_INFO "\tMin. Time  : %ld counts\n", rpt->min_time);
    printk(KERN_INFO "\tTotal Size : %ld KB\n", rpt->total_size / 1024);
    printk(KERN_INFO "\tTotal Time : %ld counts\n", rpt->total_time);
    if (rpt->total_time) {
        printk(KERN_INFO "\tPerformance: %ld KB/sec\n", 
            ((rpt->total_size / 1024) * 32768) / rpt->total_time);
    }
}

static int mmc_perf_dump(int dev_id)
{
    struct mt6516_sd_host *host;
    struct mmc_op_perf *perf;
    u32 total_read_size, total_write_size;
    u32 total_read_time, total_write_time;
      
    perf = &sd_perf[dev_id];
    host = sd_perf[dev_id].host;

    total_read_size = total_write_size = 0;
    total_read_time = total_write_time = 0;

    printk(KERN_INFO "\n============== [SD Host %d] ==============\n", dev_id);
    printk(KERN_INFO " OP Clock Freq. : %d khz\n", host->hclk / 1000);    
    printk(KERN_INFO " SD Clock Freq. : %d khz\n", host->sclk / 1000);
    
    if (perf->multi_blks_read.count) {
        printk(KERN_INFO " Multi-Blks-Read:\n");
        mmc_perf_report(&perf->multi_blks_read);
        total_read_size += perf->multi_blks_read.total_size;
        total_read_time += perf->multi_blks_read.total_time;
    }
    if (perf->multi_blks_write.count) {
        printk(KERN_INFO " Multi-Blks-Write:\n");
        mmc_perf_report(&perf->multi_blks_write);
        total_write_size += perf->multi_blks_write.total_size;
        total_write_time += perf->multi_blks_write.total_time;
    }
    if (perf->single_blk_read.count) {
        printk(KERN_INFO " Single-Blk-Read:\n");
        mmc_perf_report(&perf->single_blk_read);
        total_read_size += perf->single_blk_read.total_size;
        total_read_time += perf->single_blk_read.total_time;
    }
    if (perf->single_blk_write.count) {
        printk(KERN_INFO " Single-Blk-Write:\n");
        mmc_perf_report(&perf->single_blk_write);
        total_write_size += perf->single_blk_write.total_size;
        total_write_time += perf->single_blk_write.total_time;
    }
    if (total_read_time) {
        printk(KERN_INFO " Performance Read : %d KB/sec\n",
            ((total_read_size / 1024) * 32768) / total_read_time);
    }
    if (total_write_time) {
        printk(KERN_INFO " Performance Write: %d KB/sec\n",
            ((total_write_size / 1024) * 32768) / total_write_time);
    }

    printk(KERN_INFO "========================================\n\n");
    
    return 0;
}

static ssize_t mmc_perf_show(struct device *dev, 
    struct device_attribute *attr, char *buf)
{
    struct platform_device *pdev;
    struct mt6516_sd_host *host;
    struct mmc_host *mmc;

	pdev = container_of(dev, struct platform_device, dev);
	mmc  = platform_get_drvdata(pdev);
    host = mmc_priv(mmc);
	return mmc_perf_dump(host->id);
}

static ssize_t mmc_perf_reset(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
    struct platform_device *pdev;
    struct mt6516_sd_host *host;    
    struct mmc_host *mmc;

	pdev = container_of(dev, struct platform_device, dev);
	mmc  = platform_get_drvdata(pdev);
	host = mmc_priv(mmc);

    host = sd_perf[host->id].host;
    memset(&sd_perf[host->id], 0, sizeof(struct mmc_op_perf));
    sd_perf[host->id].host = host;

	return count;
}

DEVICE_ATTR(perf, S_IWUSR | S_IRUGO, mmc_perf_show, mmc_perf_reset);

static int mmc_perf_init(struct mt6516_sd_host *host, struct platform_device *pdev)
{
    memset(&sd_perf[host->id], 0, sizeof(struct mmc_op_perf));
    sd_perf[host->id].host = host;
    return device_create_file(&pdev->dev, &dev_attr_perf);
}

static void mt6516_xgpt_timer_init(XGPT_NUM num)
{
    XGPT_CONFIG config;

    config.num = num; /* 32768Hz */
    config.mode = XGPT_KEEP_GO;
    config.bIrqEnable = FALSE;
    config.clkDiv = XGPT_CLK_DIV_1;
    config.u4Compare = 32768;
    XGPT_Config(config);
}

static void mt6516_xgpt_timer_start(XGPT_NUM num)
{
    XGPT_Start(num);
}
static void mt6516_xgpt_timer_stop(XGPT_NUM num)
{
    XGPT_Stop(num);
}
static void mt6516_xgpt_timer_stop_clear(XGPT_NUM num)
{
    XGPT_Stop(num);
    XGPT_ClearCount(num);
}
static unsigned int mt6516_xgpt_timer_get_count(XGPT_NUM num)
{
    return (unsigned int)XGPT_GetCounter(num);
}
#endif

#ifdef MT6516_SD_DEBUG
static void mt6516_sd_dump_card_status(struct mt6516_sd_host *host, u32 status)
{
	static char *state[] = {
		"Idle",			/* 0 */
		"Ready",		/* 1 */
		"Ident",		/* 2 */
		"Stby",			/* 3 */
		"Tran",			/* 4 */
		"Data",			/* 5 */
		"Rcv",			/* 6 */
		"Prg",			/* 7 */
		"Dis",			/* 8 */
		"Reserved",		/* 9 */
		"Reserved",		/* 10 */
		"Reserved",		/* 11 */
		"Reserved",		/* 12 */
		"Reserved",		/* 13 */
		"Reserved",		/* 14 */
		"I/O mode",		/* 15 */
	};
	if (status & R1_OUT_OF_RANGE)
		MSG(RSP, "\t[CARD_STATUS] Out of Range\n");
	if (status & R1_ADDRESS_ERROR)
		MSG(RSP, "\t[CARD_STATUS] Address Error\n");
	if (status & R1_BLOCK_LEN_ERROR)
		MSG(RSP, "\t[CARD_STATUS] Block Len Error\n");
	if (status & R1_ERASE_SEQ_ERROR)
		MSG(RSP, "\t[CARD_STATUS] Erase Seq Error\n");
	if (status & R1_ERASE_PARAM)
		MSG(RSP, "\t[CARD_STATUS] Erase Param\n");
	if (status & R1_WP_VIOLATION)
		MSG(RSP, "\t[CARD_STATUS] WP Violation\n");
	if (status & R1_CARD_IS_LOCKED)
		MSG(RSP, "\t[CARD_STATUS] Card is Locked\n");
	if (status & R1_LOCK_UNLOCK_FAILED)
		MSG(RSP, "\t[CARD_STATUS] Lock/Unlock Failed\n");
	if (status & R1_COM_CRC_ERROR)
		MSG(RSP, "\t[CARD_STATUS] Command CRC Error\n");
	if (status & R1_ILLEGAL_COMMAND)
		MSG(RSP, "\t[CARD_STATUS] Illegal Command\n");
	if (status & R1_CARD_ECC_FAILED)
		MSG(RSP, "\t[CARD_STATUS] Card ECC Failed\n");
	if (status & R1_CC_ERROR)
		MSG(RSP, "\t[CARD_STATUS] CC Error\n");
	if (status & R1_ERROR)
		MSG(RSP, "\t[CARD_STATUS] Error\n");
	if (status & R1_UNDERRUN)
		MSG(RSP, "\t[CARD_STATUS] Underrun\n");
	if (status & R1_OVERRUN)
		MSG(RSP, "\t[CARD_STATUS] Overrun\n");
	if (status & R1_CID_CSD_OVERWRITE)
		MSG(RSP, "\t[CARD_STATUS] CID/CSD Overwrite\n");
	if (status & R1_WP_ERASE_SKIP)
		MSG(RSP, "\t[CARD_STATUS] WP Eraser Skip\n");
	if (status & R1_CARD_ECC_DISABLED)
		MSG(RSP, "\t[CARD_STATUS] Card ECC Disabled\n");
	if (status & R1_ERASE_RESET)
		MSG(RSP, "\t[CARD_STATUS] Erase Reset\n");
	if (status & R1_READY_FOR_DATA)
		MSG(RSP, "\t[CARD_STATUS] Ready for Data\n");
	if (status & R1_APP_CMD)
		MSG(RSP, "\t[CARD_STATUS] App Command\n");

	MSG(RSP, "\t[CARD_STATUS] '%s' State\n", 
        state[R1_CURRENT_STATE(status)]);
}

static void mt6516_sd_dump_ocr_reg(struct mt6516_sd_host *host, u32 resp)
{
	if (resp & (1 << 7))
		MSG(RSP, "\t[OCR] Low Voltage Range\n");
	if (resp & (1 << 15))
		MSG(RSP, "\t[OCR] 2.7-2.8 volt\n");
	if (resp & (1 << 16))
		MSG(RSP, "\t[OCR] 2.8-2.9 volt\n");
	if (resp & (1 << 17))
		MSG(RSP, "\t[OCR] 2.9-3.0 volt\n");
	if (resp & (1 << 18))
		MSG(RSP, "\t[OCR] 3.0-3.1 volt\n");
	if (resp & (1 << 19))
		MSG(RSP, "\t[OCR] 3.1-3.2 volt\n");
	if (resp & (1 << 20))
		MSG(RSP, "\t[OCR] 3.2-3.3 volt\n");
	if (resp & (1 << 21))
		MSG(RSP, "\t[OCR] 3.3-3.4 volt\n");
	if (resp & (1 << 22))
		MSG(RSP, "\t[OCR] 3.4-3.5 volt\n");
	if (resp & (1 << 23))
		MSG(RSP, "\t[OCR] 3.5-3.6 volt\n");
	if (resp & (1 << 30))
		MSG(RSP, "\t[OCR] Card Capacity Status (CCS)\n");
	if (resp & (1 << 31))
		MSG(RSP, "\t[OCR] Card Power Up Status (Idle)\n");
	else
		MSG(RSP, "\t[OCR] Card Power Up Status (Busy)\n");
}

static void mt6516_sd_dump_rca_resp(struct mt6516_sd_host *host, u32 resp)
{
	u32 card_status = (((resp >> 15) & 0x1) << 23) |
	                  (((resp >> 14) & 0x1) << 22) |
	                  (((resp >> 13) & 0x1) << 19) |
	                    (resp & 0x1fff);

	MSG(RSP, "\t[RCA] 0x%.4x\n", resp >> 16);
	mt6516_sd_dump_card_status(host, card_status);	
}
#endif

static unsigned int mt6516_sd_send_command(struct mt6516_sd_host *host, 
                                           struct mmc_command    *cmd,
                                           int                    polling,
                                           unsigned long          timeout)
{
    u32 base = host->base;
    u32 opcode = cmd->opcode;
    u32 rawcmd;
    u32 resp;
    u32 status;
    unsigned long tmo;

    /* Protocol layer does not provide response type, but our hardware needs 
     * to know exact type, not just size!
     */

    if (opcode == MMC_SEND_OP_COND || opcode == SD_APP_OP_COND)
        resp = RESP_R3;
    else if (opcode == MMC_SET_RELATIVE_ADDR || opcode == SD_SEND_RELATIVE_ADDR)
        resp = (mmc_cmd_type(cmd) == MMC_CMD_BCR) ? RESP_R6 : RESP_R1;
    else if (opcode == MMC_FAST_IO)
        resp = RESP_R4;
    else if (opcode == MMC_GO_IRQ_STATE)
        resp = RESP_R5;
    else if (opcode == MMC_SELECT_CARD)
        resp = (cmd->arg != 0) ? RESP_R1B : RESP_NONE;
    else if (opcode == SD_IO_RW_DIRECT || opcode == SD_IO_RW_EXTENDED)
        resp = RESP_R1; /* SDIO workaround. */
    else if (opcode == SD_SEND_IF_COND && (mmc_cmd_type(cmd) == MMC_CMD_BCR))
        resp = RESP_R1;
    else {
        switch (mmc_resp_type(cmd)) {
        case MMC_RSP_R1:
            resp = RESP_R1;
            break;
        case MMC_RSP_R1B:
            resp = RESP_R1B;
            break;
        case MMC_RSP_R2:
            resp = RESP_R2;
            break;
        case MMC_RSP_R3:
            resp = RESP_R3;
            break;
        case MMC_RSP_NONE:          
        default:
            resp = RESP_NONE;
            break;
        }
    }

    cmd->error = 0;
    rawcmd = opcode | resp << 7;
    
    /* cmd = opc | rtyp << 7 | idt << 10 | dtype << 11 | rw << 13 | stp << 14 */

    if (opcode == MMC_READ_MULTIPLE_BLOCK) {
        rawcmd |= (2 << 11);
    } else if (opcode == MMC_READ_SINGLE_BLOCK) {
        rawcmd |= (1 << 11);
    } else if (opcode == SD_IO_RW_EXTENDED) {
        if (cmd->data->flags & MMC_DATA_WRITE)
            rawcmd |= (1 << 13);
        if (cmd->data->blocks > 1)
            rawcmd |= (2 << 11);
        else
            rawcmd |= (1 << 11);
    } else if (opcode == MMC_WRITE_MULTIPLE_BLOCK) {
        rawcmd |= ((2 << 11) | (1 << 13));
    } else if (opcode == MMC_WRITE_BLOCK) {
        rawcmd |= ((1 << 11) | (1 << 13));
    } else if (opcode == SD_IO_RW_DIRECT && cmd->flags == (unsigned int)-1) {
        rawcmd |= (1 << 14);
    } else if ((opcode == SD_APP_SEND_SCR) || 
        (opcode == SD_APP_SEND_NUM_WR_BLKS) ||
        (opcode == SD_SWITCH && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
        (opcode == SD_APP_SD_STATUS && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
        (opcode == MMC_SEND_EXT_CSD && (mmc_cmd_type(cmd) == MMC_CMD_ADTC))) {
        rawcmd |= (1 << 11);
    } else if (opcode == MMC_STOP_TRANSMISSION) {
        rawcmd |= (1 << 14);
    } else if (opcode == MMC_ALL_SEND_CID) {
        rawcmd |= (1 << 10);
    }

    MSG(CMD, "[SD%d] SND_CMD(%d): ARG(0x%.8x), RAW(0x%.8x), RSP(%d), Polling(%d)\n", 
        host->id, opcode, cmd->arg, rawcmd, resp, polling);

    tmo = jiffies + timeout;
    if ((resp == RESP_R1B || cmd->data) && (opcode != MMC_STOP_TRANSMISSION)) {
        /* command issued with data line */
        for (;;) {
            if (!sdc_is_busy())
                break;
            if (time_after(jiffies, tmo)) {
                printk(KERN_ERR "[SD%d] data line is busy\n", host->id);
                cmd->error = (unsigned int)-ETIMEDOUT;
                msdc_reset();
                goto end;
            }
        }
    } else {
        /* command issued without data line */
        for (;;) {
            if (!sdc_is_cmd_busy())
                break;
            if (time_after(jiffies, tmo)) {
                printk(KERN_ERR "[SD%d] cmd line is busy\n", host->id);
                cmd->error = (unsigned int)-ETIMEDOUT;
                msdc_reset();
                goto end;
            }
        }
    }

    if (resp != RESP_NONE && polling == 0) {
        BUG_ON(in_interrupt());
        init_completion(&host->cmd_done);
        host->cmd = cmd;
        host->cmd_rsp = resp;
        sdr_read32(SDC_CMDSTA);
        sdr_clr_bits(SDC_IRQMASK0,
            SDC_IRQMASK0_CMDRDY|SDC_IRQMASK0_CMDTO|SDC_IRQMASK0_RSPCRCERR);
    }
    sdc_send_cmd(rawcmd, cmd->arg);
        
    if (resp != RESP_NONE) {
        if (polling) {
            for (;;) {
                if ((status = sdr_read32(SDC_CMDSTA)) != 0)
                    break;
                if (time_after(jiffies, tmo)) {
                    printk(KERN_ERR "[SD%d] cmdsta timeout\n", host->id);
                    status = SDC_CMDSTA_CMDTO;
                    break;
                }
            }
            if (status & SDC_CMDSTA_CMDRDY) {
                u32 *rsp = &cmd->resp[0];
                cmd->error = 0;
                if (unlikely(resp == RESP_R2)) {
                    *rsp++ = sdr_read32(SDC_RESP3);
                    *rsp++ = sdr_read32(SDC_RESP2);
                    *rsp++ = sdr_read32(SDC_RESP1);
                    *rsp++ = sdr_read32(SDC_RESP0);        
                } else {
                    *rsp = sdr_read32(SDC_RESP0); /* Resp: 1, 3, 4, 5, 6, 7(1b) */
                }
            } else if (status & SDC_CMDSTA_RSPCRCERR) {
                printk(KERN_ERR "[SD%d] cmd resp crc error\n", host->id);
                cmd->error = (unsigned int)-EIO;
                msdc_reset();
            } else {
                cmd->error = (unsigned int)-ETIMEDOUT;
                msdc_reset();
            }
        } else {
			spin_unlock(&host->lock);
            wait_for_completion(&host->cmd_done);
			spin_lock(&host->lock);
            sdr_set_bits(SDC_IRQMASK0,
                SDC_IRQMASK0_CMDRDY|SDC_IRQMASK0_CMDTO|SDC_IRQMASK0_RSPCRCERR);
        }
    }

end:
#ifdef MT6516_SD_DEBUG
    switch (resp) {
    case RESP_NONE:
        MSG(RSP, "[SD%d] CMD_RSP(%d): %d RSP(%d)\n", host->id, 
            opcode, cmd->error, resp);
        break;
    case RESP_R2:
        MSG(RSP, "[SD%d] CMD_RSP(%d): %d RSP(%d)= %.8x %.8x %.8x %.8x\n", 
            host->id, opcode, cmd->error, resp, cmd->resp[0], cmd->resp[1], 
            cmd->resp[2], cmd->resp[3]);          
        break;
    default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
        MSG(RSP, "[SD%d] CMD_RSP(%d): %d RSP(%d)= 0x%.8x\n", 
            host->id, opcode, cmd->error, resp, cmd->resp[0]);
        if (cmd->error == 0) {
            switch (resp) {
            case RESP_R1:
            case RESP_R1B:
                mt6516_sd_dump_card_status(host, cmd->resp[0]);
                break;
            case RESP_R3:
                mt6516_sd_dump_ocr_reg(host, cmd->resp[0]);
                break;
            case RESP_R6:
                mt6516_sd_dump_rca_resp(host, cmd->resp[0]);
                break;
            }
        }
        break;
    }
#endif
    return cmd->error;
}

static int mt6516_sd_pio_read(struct mt6516_sd_host *host, struct mmc_data *data)
{
    struct scatterlist *sg = data->sg;
    u32  base = host->base;
    u32  num = data->sg_len;
    u32 *ptr;
    u32  left;
    u32  size = 0;
    u32  status;

    while (num) {
        left = sg->length;
        ptr = sg_virt(sg);
        while (left) {
            status = sdr_read32(MSDC_STA);
            if (status & MSDC_STA_DRQ) {
                if (likely(left > 3)) {
                    *ptr++ = msdc_fifo_read();
                    left -= 4;
                } else {
                    u32 val = msdc_fifo_read();
                    memcpy(ptr, &val, left);
                    left = 0;
                }
            }
            if (atomic_read(&host->abort))
                goto end;
        }
        size += sg->length;
        sg = sg_next(sg); num--;
    }
end:
    data->bytes_xfered += size;

    if (data->error)
        printk(KERN_ERR "[SD%d] pio read error %d\n", host->id, data->error);

    MSG(OPS, "[SD%d] DATA_RD: %d/%d bytes\n", host->id, size, host->xfer_size);

    return data->error;
}

static int mt6516_sd_pio_write(struct mt6516_sd_host* host, struct mmc_data *data)
{
    u32  base = host->base;
    struct scatterlist *sg = data->sg;
    u32  num = data->sg_len;
    u32 *ptr;
    u32  left;
    u32  size = 0;
    u32  status;

    while (num) {
        left = sg->length;
        ptr = sg_virt(sg);
        while (left) {
            status = sdr_read32(MSDC_STA);
            if (status & MSDC_STA_DRQ) {
                if (likely(left > 3)) {
                    msdc_fifo_write(*ptr); ptr++;
                    left -= 4;
                } else {
                    u32 val = 0;
                    memcpy(&val, ptr, left);
                    msdc_fifo_write(val);
                    left = 0;
                }
            } 
            if (atomic_read(&host->abort))
                goto end;
        }
        size += sg->length;
        sg = sg_next(sg); num--;
    }
end:    
    data->bytes_xfered = size;

    if (data->error)
        printk(KERN_ERR "[SD%d] pio read error %d\n", host->id, data->error);

    MSG(OPS, "[SD%d] DATA_WR: %d/%d bytes\n", host->id, size, host->xfer_size);

    return data->error;	
}

static void mt6516_sd_setup_dma(struct mt6516_sd_host *host,
                                struct scatterlist *sg)
{
    struct mt_dma_conf *dma = host->dma;
    u32 base = host->base;
    u32 left_size;
    u32 xfer_size;
    u32 dma_addr = host->dma_addr;

    MSG(DMA, "[SD%d] DMA_SET: Addr:0x%.8x Size:%d bytes\n", host->id, 
        host->dma_addr, host->dma_left_size);

    left_size = host->xfer_size - host->data->bytes_xfered;
    xfer_size = left_size > host->dma_left_size ? host->dma_left_size : left_size;

    if (likely((xfer_size & 0x3) == 0)) {
        if ((xfer_size >> 2) > DMA_MAX_COUNT)
            xfer_size = DMA_MAX_COUNT * sizeof(u32);
        dma->count = (u16)(xfer_size >> 2);
        dma->b2w   = DMA_FALSE;
        dma->size  = DMA_CON_SIZE_LONG;
        dma->burst = DMA_CON_BURST_4BEAT;
        dma->pgmaddr = dma_addr;
        msdc_set_fifo_thd(4);
    } else {
        if (xfer_size > DMA_MAX_COUNT)
            xfer_size = DMA_MAX_COUNT;
        dma->count = (u16)xfer_size;
        dma->b2w   = DMA_TRUE;
        dma->size  = DMA_CON_SIZE_BYTE;
        dma->burst = DMA_CON_BURST_SINGLE;
        dma->pgmaddr = dma_addr;
        msdc_set_fifo_thd(1);
    }

    host->dma_xfer_size = xfer_size;
}

static void mt6516_sd_prepare_dma(struct mt6516_sd_host *host,
                                  struct scatterlist *sg)
{
    struct mt_dma_conf *dma = host->dma;
    
    if (host->dma_dir == DMA_FROM_DEVICE) {
        dma->dir  = DMA_TRUE;  /* write to memory */
        dma->sinc = DMA_FALSE;
        dma->dinc = DMA_TRUE;
    } else {
        dma->dir  = DMA_FALSE; /* read from memory */       
        dma->sinc = DMA_TRUE;
        dma->dinc = DMA_FALSE;
    }
    
    host->dma_addr = sg_dma_address(sg);
    host->dma_left_size = sg_dma_len(sg);
    mt6516_sd_setup_dma(host, sg);
}

static void mt6516_sd_start_dma(struct mt6516_sd_host *host)
{
    if (mt_config_dma(host->dma, ALL) != DMA_OK) {
        BUG_ON(1);
    }
    mt_start_dma(host->dma);
}

static void mt6516_sd_stop_dma(struct mt6516_sd_host *host)
{
	mt_stop_dma(host->dma);
}

static void mt6516_sd_dma_callback(void *d)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host*)d;

    tasklet_hi_schedule(&host->dma_tasklet);
}

static void mt6516_sd_abort_data(struct mt6516_sd_host *host, struct mmc_data *data)
{
    u32 base = host->base;

    printk(KERN_ERR "[SD%d] abort data. dma(%d)\n", host->id, host->dma_xfer);

    atomic_set(&host->abort, 1);

    if (data->stop)
        (void)mt6516_sd_send_command(host, data->stop, 1, CMD_TIMEOUT);

    if (host->dma_xfer) {
        tasklet_hi_schedule(&host->dma_tasklet);
    } else {
        msdc_fifo_irq_off(); 
        complete(&host->xfer_done);
    }
}

static void mt6516_sd_set_timeout(struct mt6516_sd_host *host, u32 ns, u32 clks)
{
    u32 base = host->base;
    u32 timeout, clk_ns;

    host->timeout_ns   = ns;
    host->timeout_clks = clks;

    clk_ns  = 1000000000UL / host->sclk;
    timeout = ns / clk_ns + clks;
    timeout = timeout >> 16; /* in 65536 sclk cycle unit */
    timeout = timeout > 1 ? timeout - 1 : 0;
    timeout = timeout > 255 ? 255 : timeout;

    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, timeout);

    MSG(OPS, "[SD%d] Set read data timeout: %dns %dclks -> %d x 65536 cycles\n",
        host->id, ns, clks, timeout + 1);
}

static __tcmfunc irqreturn_t mt6516_sd_irq(int irq, void *dev_id)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host *)dev_id;
    struct mmc_data *data = host->data;
    struct mmc_command *cmd = host->cmd;
    int cmd_done = 0;
    u32 base = host->base;
    u32 intsts = sdr_read32(MSDC_INT);
    
    if (intsts & MSDC_INT_PINIRQ)
        tasklet_hi_schedule(&host->card_tasklet);

    if (intsts & MSDC_INT_DIRQ && data != NULL)
        tasklet_hi_schedule(&host->fifo_tasklet);

    if (intsts & MSDC_INT_SDIOIRQ)
        mmc_signal_sdio_irq(host->mmc);

    if (intsts & MSDC_INT_SDMCIRQ)
        printk(KERN_INFO "\n[SD%d] MCIRQ: SDC_CSTA=0x%.8x\n", 
            host->id, sdr_read32(SDC_CSTA));

    if (intsts & MSDC_INT_SDCMDIRQ && cmd != NULL) {
        u32 status = sdr_read32(SDC_CMDSTA);
        if (status & SDC_CMDSTA_CMDRDY) {
            u32 *rsp = &cmd->resp[0];
            cmd->error = 0;
            if (unlikely(host->cmd_rsp == RESP_R2)) {
                *rsp++ = sdr_read32(SDC_RESP3);
                *rsp++ = sdr_read32(SDC_RESP2);
                *rsp++ = sdr_read32(SDC_RESP1);
                *rsp++ = sdr_read32(SDC_RESP0);        
            } else {
                *rsp = sdr_read32(SDC_RESP0); /* Resp: 1, 3, 4, 5, 6, 7(1b) */
            }
        } else if (status & SDC_CMDSTA_RSPCRCERR) {
            printk(KERN_ERR "[SD%d] cmd resp crc error\n", host->id);
            cmd->error = (unsigned int)-EIO;
            msdc_reset();
        } else if (status & SDC_CMDSTA_CMDTO) {
            cmd->error = (unsigned int)-ETIMEDOUT;
            msdc_reset();
        }

        if (status) {
            host->cmd_rsp_done = 1;
            cmd_done = 1;
        }
    }

    if (intsts & MSDC_INT_SDR1BIRQ && cmd != NULL) {
        host->cmd_r1b_done = 1;
        cmd_done = 1;
    }

    if (cmd_done) {
        if (((host->cmd_rsp != RESP_R1B) && (host->cmd_rsp_done == 1)) ||
            ((host->cmd_rsp == RESP_R1B) && (host->cmd_rsp_done == 1) && 
             (host->cmd_r1b_done == 1))) {
             complete(&host->cmd_done);
             host->cmd = NULL;
             host->cmd_rsp_done = 0;
             host->cmd_r1b_done = 0;
        }
    }
    
    if (intsts & MSDC_INT_SDDATIRQ && data != NULL) {
        u32 status = sdr_read32(SDC_DATSTA);

        if (status & SDC_DATSTA_DATTO)
            data->error = (unsigned int)-ETIMEDOUT;
        else if (status & SDC_DATSTA_DATCRCERR)
            data->error = (unsigned int)-EIO;

        if (data->error) {
            printk(KERN_ERR "\n[SD%d] SDDATIRQ: data error(%d) flags(0x%x)\n", 
                host->id, data->error, data->flags);
            mt6516_sd_abort_data(host, data);
        }
    }
    
#ifdef MT6516_SD_DEBUG
    {
        msdc_int_reg *int_reg = (msdc_int_reg*)&intsts;
        MSG(INT, "[SD%d] IRQ_EVT(0x%.8x): SDIO(%d) R1B(%d), DAT(%d), CMD(%d), PIN(%d), DIRQ(%d)\n", 
            host->id,
            intsts,
            int_reg->sdioirq,
            int_reg->sdr1b,
            int_reg->sddatirq,
            int_reg->sdcmdirq,
            int_reg->pinirq,
            int_reg->dirq);
    }
#endif
    return IRQ_HANDLED;
}

static void mt6516_sd_sdio_eirq(void *data)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host *)data;

    MSG(INT, "[SD%d] SDIO EINT(%d)\n", host->id);

    mmc_signal_sdio_irq(host->mmc);
}

static void mt6516_sd_cd_eirq(void *data)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host *)data;

    MSG(INT, "[SD%d] CD EINT(%d)\n", host->id);

    tasklet_hi_schedule(&host->card_tasklet);
}

static void mt6516_sd_init_dma(struct mt6516_sd_host *host)
{
    struct mt_dma_conf *dma;
    u32 dma_master[] = {DMA_CON_MASTER_MSDC0, 
                        DMA_CON_MASTER_MSDC1, 
                        DMA_CON_MASTER_MSDC2};

    dma = mt_request_half_size_dma();

    BUG_ON(!dma);

    mt_reset_dma(dma);
    dma->mas      = dma_master[host->id];
    dma->iten     = DMA_TRUE;
    dma->dreq     = DMA_TRUE;
    dma->limiter  = 0;
    dma->data     = (void*)host;
    dma->callback = mt6516_sd_dma_callback;
    host->dma     = dma;
}

static void mt6516_sd_deinit_dma(struct mt6516_sd_host *host)
{
    mt_stop_dma(host->dma);
    mt_free_dma(host->dma);
}

static void mt6516_sd_tasklet_card(unsigned long arg)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host *)arg;
    struct mt6516_sd_host_hw *hw = host->hw;
    u32 base = host->base;
    u32 inserted;	
    u32 status;
    u32 change = 0;

    spin_lock(&host->lock);

    if (hw->get_cd_status) {
        inserted = hw->get_cd_status();
    } else {
        status = sdr_read32(MSDC_PS);
        inserted = status & MSDC_PS_PIN0 ? 0 : 1;
    }

    change = host->card_inserted ^ inserted;
    host->card_inserted = inserted;
	
    if (change && !host->suspend)
        mmc_detect_change(host->mmc, msecs_to_jiffies(20));
    spin_unlock(&host->lock);

    MSG(OPS, "[SD%d] PS(0x%.8x), inserted(%d), change(%d)\n", 
        host->id, status, host->card_inserted, change);
}

static void mt6516_sd_tasklet_fifo(unsigned long arg)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host *)arg;
    struct mmc_data *data = host->data;
    u32 base = host->base;

    if (!data) return;

    spin_lock(&host->lock);

    /* disable DREQ interrupt to avoid disturb pio ops. */
    msdc_fifo_irq_off(); 

    if (data->flags & MMC_DATA_READ) {
        (void)mt6516_sd_pio_read(host, data);
    } else {
        (void)mt6516_sd_pio_write(host, data);
    }
    complete(&host->xfer_done);

    spin_unlock(&host->lock);
}

static void mt6516_sd_tasklet_dma(unsigned long arg)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host*)arg;
    struct mmc_data *data = host->data;
    u32 base = host->base;

    if (!data) return;

    spin_lock(&host->lock);
    mt6516_sd_stop_dma(host);

    data->bytes_xfered += host->dma_xfer_size;

    if (data->error || data->bytes_xfered == host->xfer_size) {
        msdc_dma_off();
        complete(&host->xfer_done);
        spin_unlock(&host->lock);
        return;
    }

    host->dma_addr += host->dma_xfer_size;    

    if (host->dma_addr == (sg_dma_address(host->cur_sg) + sg_dma_len(host->cur_sg))) {
        host->cur_sg++;
        host->num_sg--;
        host->dma_addr = sg_dma_address(host->cur_sg);  
        host->dma_left_size = sg_dma_len(host->cur_sg);
    } else {
        host->dma_left_size -= host->dma_xfer_size;
    }

    mt6516_sd_setup_dma(host, host->cur_sg);
    mt6516_sd_start_dma(host);
    spin_unlock(&host->lock);
}

static void mt6516_sd_card_clock(struct mt6516_sd_host *host, int clksrc, int on)
{
    u32 base = host->base;

    MSG(CFG, "[SD%d] Turn %s %s clock (current clksrc: %d, clk_on: %d -> %d)\n", 
        host->id, on ? "on" : "off", "card", clksrc, host->card_clkon, on);

    if (on && !host->card_clkon) {
        host->card_clkon = 1;
        if (clksrc == MSDC_CLKSRC_MCPLL) {
            sdr_set_bits(MSDC_CFG, MSDC_CFG_CLKSRC); /* MCPLL */
            (void)hwEnablePLL(MT6516_PLL_MCPLL,"SD");            
        } else {
            sdr_clr_bits(MSDC_CFG, MSDC_CFG_CLKSRC); /* MCU */ 
        }
        sdc_sclk_on();
        mdelay(1);
    } else if (!on && host->card_clkon) {
        host->card_clkon = 0;
        if (clksrc == MSDC_CLKSRC_MCPLL)
            (void)hwDisablePLL(MT6516_PLL_MCPLL, FALSE, "SD");
        sdc_sclk_off();        
    }
}

static void mt6516_sd_host_clock(struct mt6516_sd_host *host, int on)
{
    u32 pwr_bit[] = 
        {MT6516_PDN_PERI_MSDC, MT6516_PDN_PERI_MSDC2, MT6516_PDN_PERI_MSDC3};

    MSG(CFG, "[SD%d] Turn %s %s clock \n", host->id, on ? "on" : "off", "host");

    if (on) {
        (void)hwEnableClock(pwr_bit[host->id],"SD");
    } else {
        (void)hwDisableClock(pwr_bit[host->id],"SD");
    }
}

static void mt6516_sd_config_clock(struct mt6516_sd_host *host, u32 hz)
{
    struct mt6516_sd_host_hw *hw = host->hw;
    u32 base = host->base;
    u32 flags;
    u32 dlt = DEFAULT_DLT;
    u32 cred = hw->cmd_edge;
    u32 dred = hw->data_edge;
    u32 sclk_div;
    u32 sclk;
    u32 hclk = host->hclk;
    u8  clksrc = hw->clk_src;

    if (!hz) {
        msdc_reset();
        mt6516_sd_card_clock(host, clksrc, 0);
        return;
    }

    msdc_lock_irqsave(flags);
    mt6516_sd_card_clock(host, clksrc, 0);

    if ((hz >= HOST_MAX_SCLK) && (hw->flags & MSDC_HIGHSPEED)) {
        hclk   = MCU_CLK_FREQ;
        clksrc = MSDC_CLKSRC_MCU;
    }
    
    if (hz >= (hclk >> 1)) {
        sclk_div = 0;         /* mean div = 1/2 */
        sclk     = hclk >> 1; /* sclk = hclk / 2 */
    } else {
        sclk_div = (hclk + ((hz << 2) - 1)) / (hz << 2);
        sclk     = (hclk >> 2) / sclk_div;
    }

    host->sclk  = sclk;
    host->hclk  = hclk;
    hw->clk_src = clksrc;

    sdr_set_field(MSDC_CFG, MSDC_CFG_SCLKF, sclk_div);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DLT, dlt);  
    sdr_set_field(MSDC_CFG, MSDC_CFG_RED, dred); 
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_CMDRE, cred);

    mt6516_sd_card_clock(host, clksrc, 1);
    mt6516_sd_set_timeout(host, host->timeout_ns, host->timeout_clks);
    
    msdc_unlock_irqrestore(flags);

    MSG(CFG, "[SD%d] SET_CLK(%d): SCLK(%dkHz) DIV(%d) DLAT(%d) CRED(%d) DRED(%d)\n", 
        host->id, clksrc, sclk/1000, sclk_div, dlt, cred, dred);   
}

static void mt6516_sd_config_bus(struct mt6516_sd_host *host, u32 width)
{
    u32 base = host->base;
    u32 flags;

    msdc_lock_irqsave(flags);

    switch (width) {
    default:
    case 1:
        width = 1;
        sdr_clr_bits(SDC_CFG, SDC_CFG_MDLEN | SDC_CFG_MDLEN8);
        break;
    case 4:
        sdr_set_bits(SDC_CFG, SDC_CFG_MDLEN);
        sdr_clr_bits(SDC_CFG, SDC_CFG_MDLEN8);
        break;
    case 8:
        sdr_set_bits(SDC_CFG, SDC_CFG_MDLEN | SDC_CFG_MDLEN8);
        break;
    }

    msdc_unlock_irqrestore(flags);

    MSG(CFG, "[SD%d] Bus Width = %d\n", host->id, width);
}

static void mt6516_sd_config_pin(struct mt6516_sd_host *host, int mode)
{
    struct mt6516_sd_host_hw *hw = host->hw;
    u32 base = host->base;
    int pull = (mode == PIN_PULL_UP) ? GPIO_PULL_UP : GPIO_PULL_DOWN;

    /* Config WP pin */
    if (hw->flags & MSDC_WP_PIN_EN) {
        if (hw->config_gpio_pin)
            hw->config_gpio_pin(MSDC_WP_PIN, pull);
        sdr_set_field(MSDC_CFG, MSDC_CFG_PRCFG0, mode);
    }

    if (hw->config_gpio_pin) {
        hw->config_gpio_pin(MSDC_CMD_PIN, pull);
        hw->config_gpio_pin(MSDC_DAT_PIN, pull);    
    }
    sdr_set_field(MSDC_CFG, MSDC_CFG_PRCFG1, mode); /* Config CMD pin */
    sdr_set_field(MSDC_CFG, MSDC_CFG_PRCFG2, mode); /* Config DATA pins */

    MSG(CFG, "[SD%d] Pins mode(%d), none(0), down(1), up(2), keep(3)\n", 
        host->id, mode);
}

static void mt6516_sd_core_power(struct mt6516_sd_host *host, int on)
{
    MSG(CFG, "[SD%d] Turn %s %s power (copower: %d -> %d)\n", 
        host->id, on ? "on" : "off", "core", host->core_power, on);

    if (on && host->core_power == 0) {
        (void)hwPowerOn(MT6516_POWER_VSDIO, VOL_3300, "SD");
        host->core_power = 1;
    } else if (!on && host->core_power == 1) {
        (void)hwPowerDown(MT6516_POWER_VSDIO, "SD");
        host->core_power = 0;
    }

    msleep(1); 
}

static void mt6516_sd_host_power(struct mt6516_sd_host *host, int on)
{
    MSG(CFG, "[SD%d] Turn %s %s power \n", host->id, on ? "on" : "off", "host");

    if (on) {
        mt6516_sd_host_clock(host, 1);
    } else {
        mt6516_sd_host_clock(host, 0);
    }
}

static void mt6516_sd_card_power(struct mt6516_sd_host *host, int on)
{
    u32 base = host->base;

    MSG(CFG, "[SD%d] Turn %s %s power \n", host->id, on ? "on" : "off", "card");

    if (on) {
        mt6516_sd_config_pin(host, PIN_PULL_UP);    
        if (host->hw->ext_power_on) {
            host->hw->ext_power_on();
        } else {
            sdr_set_bits(MSDC_CFG, MSDC_CFG_VDDPD);
        }
    } else {
        if (host->hw->ext_power_off) {
            host->hw->ext_power_off();
        } else {
            sdr_clr_bits(MSDC_CFG, MSDC_CFG_VDDPD);
        }
        mt6516_sd_config_pin(host, PIN_PULL_DOWN);
    }
}

static void mt6516_sd_set_power(struct mt6516_sd_host *host, u8 mode)
{
    MSG(CFG, "[SD%d] Set power mode(%d)\n", host->id, mode);

    if (host->power_mode == MMC_POWER_OFF && mode != MMC_POWER_OFF) {
        mt6516_sd_core_power(host, 1);
        mt6516_sd_host_power(host, 1);
        mt6516_sd_card_power(host, 1);
    } else if (host->power_mode != MMC_POWER_OFF && mode == MMC_POWER_OFF) {
        mt6516_sd_card_power(host, 0);    
        mt6516_sd_host_power(host, 0);
        mt6516_sd_core_power(host, 0);
    }
    host->power_mode = mode;
}

static void mt6516_sd_init_hw(struct mt6516_sd_host *host)
{
    struct mt6516_sd_host_hw *hw = host->hw;
    u32 base = host->base;

#ifdef MT6516_SD_DEBUG	
    mt6516_sd_reg[host->id] = (struct mt6516_sd_regs *)host->base;
#endif

    /* Reset and power on */
    mt6516_sd_core_power(host, 1);
    mt6516_sd_core_power(host, 0);
    mt6516_sd_core_power(host, 1);

    msdc_reset();

    /* Turn off card vdd */
    sdr_clr_bits(MSDC_CFG, MSDC_CFG_VDDPD);

    /* Disable card detection */
    sdr_clr_bits(MSDC_PS, MSDC_PS_PIEN0|MSDC_PS_POEN0|MSDC_PS_CDEN);

    /* Disable all interrupts */
    sdr_clr_bits(MSDC_CFG, MSDC_CFG_RCDEN|MSDC_CFG_DIRQE|MSDC_CFG_PINEN|
        MSDC_CFG_DMAEN|MSDC_CFG_INTEN);

    sdr_set_bits(MSDC_CFG, MSDC_CFG_MSDC);   /* SD mode */

    /* Clear interrupts and status bits */
    sdr_read32(MSDC_INT);
    sdr_read32(SDC_CMDSTA);
    sdr_read32(SDC_DATSTA);

    /* Mask command interrups since we use pio mode for command req/rsp. */
    sdr_set_bits(SDC_IRQMASK0,
        SDC_IRQMASK0_CMDRDY|SDC_IRQMASK0_CMDTO|SDC_IRQMASK0_RSPCRCERR);

    /* Mask data done interrupt to reduce interrupt latency */
    sdr_set_bits(SDC_IRQMASK0, SDC_IRQMASK0_BLKDONE);

    sdr_set_field(MSDC_IOCON, MSDC_IOCON_ODCCFG0, hw->cmd_odc);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_ODCCFG1, hw->data_odc);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_SRCFG0, hw->cmd_slew_rate);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_SRCFG1, hw->data_slew_rate);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSW, MSDC_DSW_NODELAY);

    sdr_set_bits(MSDC_STA, MSDC_STA_FIFOCLR);

    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, DEFAULT_DTOC);
    sdr_set_field(SDC_CFG, SDC_CFG_WDOD, DEFAULT_WDOD);
    sdr_set_field(SDC_CFG, SDC_CFG_BSYDLY, DEFAULT_BSYDLY);

    mt6516_sd_config_bus(host, 1);
    mt6516_sd_init_dma(host);

    MSG(FUC, "[SD%d] init hardware done!\n", host->id);
}

static void mt6516_sd_deinit_hw(struct mt6516_sd_host *host)
{
    struct mt6516_sd_host_hw *hw = host->hw;
    u32 base = host->base;

    /* Disable all interrupts */
    sdr_clr_bits(MSDC_CFG, MSDC_CFG_RCDEN|MSDC_CFG_DIRQE|MSDC_CFG_PINEN|
        MSDC_CFG_DMAEN|MSDC_CFG_INTEN);

    /* Disable card detection */
    mt6516_sd_enable_cd_irq(host->mmc, 0);
    mt6516_sd_deinit_dma(host);
    mt6516_sd_set_power(host, MMC_POWER_OFF);   /* make sure power down */
}

static void mt6516_sd_pm_change(pm_message_t state, void *data)
{
    struct mt6516_sd_host *host = (struct mt6516_sd_host *)data;
    int evt = state.event;

    if (evt == PM_EVENT_SUSPEND || evt == PM_EVENT_USER_SUSPEND) {

        if (host->suspend)
            return;

        if (evt == PM_EVENT_SUSPEND && host->power_mode == MMC_POWER_OFF)
            return;

        (void)mmc_suspend_host(host->mmc, state);
        host->suspend = 1;
        host->pm_state = state;

        printk(KERN_INFO "[SD%d] %s Suspend\n", host->id, 
            evt == PM_EVENT_SUSPEND ? "PM" : "USR");

    } else if (evt == PM_EVENT_RESUME || evt == PM_EVENT_USER_RESUME) {

        if (!host->suspend)
            return;

        /* don't resume from system when it's suspended by user */
        if (evt == PM_EVENT_RESUME && host->pm_state.event == PM_EVENT_USER_SUSPEND) {
            printk(KERN_INFO "[SD%d] No PM Resume(USR Suspend)\n", host->id);
            return;
        }
        printk(KERN_INFO "[SD%d] %s Resume\n", host->id,
            evt == PM_EVENT_RESUME ? "PM" : "USR");

        (void)mmc_resume_host(host->mmc);
        host->suspend = 0;
        host->pm_state = state;
    }
}

static void mt6516_sd_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
    struct mt6516_sd_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;
    u32 base = host->base;
    int dma = 0, read = 1, polling = 0;

    spin_lock(&host->lock);

    cmd  = mrq->cmd;
    data = mrq->cmd->data;

    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        MSG(WRN, "[SD%d] REQ fail. no present/power off\n", host->id);
        cmd->error = (unsigned int)-ENOMEDIUM;
        mrq->done(mrq);
        spin_unlock(&host->lock);
        return;
    }

    if (data != NULL) {
        BUG_ON(data->blksz > HOST_MAX_BLKSZ);
        MSG(OPS, "[SD%d] PRE_DAT: blksz(%d), blks(%d), size(%d)\n", 
            host->id, data->blksz, data->blocks, data->blocks * data->blksz);

#ifdef CFG_PROFILING
        if (sd_perf_rpt[host->id]) {
            mt6516_xgpt_timer_init(host->id + XGPT5);
            mt6516_xgpt_timer_start(host->id + XGPT5);
        }
#endif

        host->data = data;
        host->xfer_size = data->blocks * data->blksz;
        host->cur_sg = data->sg;
        host->num_sg = data->sg_len;

        /* NOTE:
         * For performance consideration, may use pio mode for smaller size
         * transferring and use dma mode for larger size one.
         */
        if (cmd->opcode == MMC_READ_SINGLE_BLOCK ||
            cmd->opcode == MMC_WRITE_BLOCK ||
            cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
            cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) {
            host->dma_xfer = dma = 1;
        }

        init_completion(&host->xfer_done);
        read = data->flags & MMC_DATA_READ ? 1 : 0;

        if (read) {
            polling = 1;
            if ((host->timeout_ns != data->timeout_ns) ||
                (host->timeout_clks != data->timeout_clks)) {
                mt6516_sd_set_timeout(host, data->timeout_ns, data->timeout_clks);
            }
        }

        msdc_set_bksz(data->blksz);
        msdc_clr_fifo();

        if (dma) {
            msdc_dma_on();
            msdc_fifo_irq_off();
            host->dma_dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
            (void)dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len, host->dma_dir);
            mt6516_sd_prepare_dma(host, host->cur_sg);
            mt6516_sd_start_dma(host);
        } else {
            msdc_dma_off();
            msdc_fifo_irq_on();
            if (read) {
                u32 xfersz = host->xfer_size;
                if ((xfersz >> 2) < MAX_FIFOTHD) {
                    msdc_set_fifo_thd(xfersz >> 2);
                } else {
                    msdc_set_fifo_thd(PRD_FIFOTHD);
                }
            } else {
                msdc_set_fifo_thd(PWR_FIFOTHD);
            }
        }
    }

    if (mt6516_sd_send_command(host, cmd, polling, CMD_TIMEOUT) != 0)
        goto done;

    if (data != NULL) {
        spin_unlock(&host->lock);
        wait_for_completion(&host->xfer_done);
        spin_lock(&host->lock);
        if (!data->error) {
            /* make sure contents in fifo flushed to device after a dma write */
            if (dma && !read) {
                int count;
                do {
                    count = (sdr_read32(MSDC_STA) >> 4) & 0xf;
                    if (count)
                        printk(KERN_WARNING "[SD%d] FIFOCNT: %d\n", host->id, count);
                } while(0 != count);
            }
            if (data->stop)
                (void)mt6516_sd_send_command(host, data->stop, 0, CMD_TIMEOUT);
        }
        if (cmd->opcode == SD_IO_RW_EXTENDED) {
            if (cmd->arg & 0x08000000) {
                /* SDIO workaround for CMD53 multiple block transfer */
                if (data->blocks > 1) {
                    struct mmc_command abort;
                    memset(&abort, 0, sizeof(struct mmc_command));
                    abort.opcode = SD_IO_RW_DIRECT;
                    abort.arg    = 0x80000000;            /* write */
                    abort.arg   |= 0 << 28;               /* function 0 */
                    abort.arg   |= SDIO_CCCR_ABORT << 9;  /* address */
                    abort.arg   |= 0;                     /* abort function 0 */
                    abort.flags  = (unsigned int)-1;
                    (void)mt6516_sd_send_command(host, &abort, 1, CMD_TIMEOUT);
                }
            } else {
                /* SDIO workaround for CMD53 multiple byte transfer, which
                 * is not 4 byte-alignment
                 */
                if (data->blksz % 4) {
                    /* The delay is required and tunable. The delay time must 
                     * be not too small. Currently, it is tuned to 25us.(CHECKME)
                     */
                    udelay(25);
                    msdc_reset();
                }
            }
        }       
    }
done:
#ifdef CFG_PROFILING
    if (data && (data->error == 0) && sd_perf_rpt[host->id]) {
        struct mmc_op_report *rpt;
        unsigned int counts = mt6516_xgpt_timer_get_count(host->id + XGPT5);
        mt6516_xgpt_timer_stop_clear(host->id + XGPT5);

        if (data->stop) {
            if (read)
                rpt = &sd_perf[host->id].multi_blks_read;
            else
                rpt = &sd_perf[host->id].multi_blks_write;
        } else {
            if (read)
                rpt = &sd_perf[host->id].single_blk_read;
            else
                rpt = &sd_perf[host->id].single_blk_write;
        }
        rpt->count++;
        rpt->total_size += host->xfer_size;
        rpt->total_time += counts;

        if (counts < rpt->min_time || rpt->min_time == 0)
            rpt->min_time = counts;
        if (counts > rpt->max_time || rpt->max_time == 0)
            rpt->max_time = counts;

        #ifdef CFG_PROFILING_DUMP
        printk(KERN_INFO "[SD%d] %s(%.8x): %6d bytes, %6d counts, %6d us, %5d KB/s\n", 
            host->id, read ? "READ " : "WRITE", 
            cmd->arg, host->xfer_size, 
            counts, counts * 30 + counts * 16960 / 32768, 
            host->xfer_size * 32 / (counts ? counts : 1));
        #endif
    }
#endif

    host->data = NULL;

    if (dma != 0 && data != NULL) {
        host->dma_xfer = 0;
        mt6516_sd_stop_dma(host);
        dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, host->dma_dir);
    }
    if (atomic_read(&host->abort)) {
        msdc_clr_fifo();
        atomic_set(&host->abort, 0);
    }
    spin_unlock(&host->lock);
    mmc_request_done(mmc, mrq);

    return;
}

static void mt6516_sd_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct mt6516_sd_host *host = mmc_priv(mmc);
    u32 base = host->base;
#ifdef MT6516_SD_DEBUG
    static char *vdd[] = {
        "1.50v", "1.55v", "1.60v", "1.65v", "1.70v", "1.80v", "1.90v",
        "2.00v", "2.10v", "2.20v", "2.30v", "2.40v", "2.50v", "2.60v",
        "2.70v", "2.80v", "2.90v", "3.00v", "3.10v", "3.20v", "3.30v",
        "3.40v", "3.50v", "3.60v"		
    };
    static char *power_mode[] = {
        "OFF", "UP", "ON"
    };
    static char *bus_mode[] = {
        "UNKNOWN", "OPENDRAIN", "PUSHPULL"
    };
    static char *timing[] = {
        "LEGACY", "MMC_HS", "SD_HS"
    };

    MSG(CFG, "[SD%d] SET_IOS: CLK(%dkHz), BUS(%s), BW(%u), PWR(%s), VDD(%s), TIMING(%s)\n",
        host->id, ios->clock / 1000, bus_mode[ios->bus_mode],
        (ios->bus_width == MMC_BUS_WIDTH_4) ? 4 : 1,
        power_mode[ios->power_mode], vdd[ios->vdd], timing[ios->timing]);
#endif
    /* Bus width select */
    switch (ios->bus_width) {
    case MMC_BUS_WIDTH_1:
        mt6516_sd_config_bus(host, 1);
        break;
    case MMC_BUS_WIDTH_4:
        mt6516_sd_config_bus(host, 4);	
        break;
    case MMC_BUS_WIDTH_8:
        mt6516_sd_config_bus(host, 8); 
        break;
    default:
        break;
    }

    /* Power control */
    switch (ios->power_mode) {
    case MMC_POWER_OFF:
    case MMC_POWER_UP:
        mt6516_sd_set_power(host, ios->power_mode);
        break;
    case MMC_POWER_ON:
        host->power_mode = MMC_POWER_ON;
        break;
    default:
        break;
    }

    /* Clock control */
    if (host->mclk != ios->clock) {
        host->mclk = ios->clock;
        mt6516_sd_config_clock(host, ios->clock);
    }
}

static int mt6516_sd_card_readonly(struct mmc_host *mmc)
{
    struct mt6516_sd_host *host = mmc_priv(mmc);
    u32 base = host->base;
    unsigned long flags;
    int ro = 0;

    if (host->hw->flags & MSDC_WP_PIN_EN) {
        spin_lock_irqsave(&host->lock, flags);
        ro = sdc_is_write_protect();
        spin_unlock_irqrestore(&host->lock, flags);
    }
    return ro;
}

static int mt6516_sd_card_inserted(struct mmc_host *mmc)
{
    struct mt6516_sd_host *host = mmc_priv(mmc);
    unsigned long flags;
    int present = 1;

    if (!(host->hw->flags & MSDC_REMOVABLE))
        return 1;

    if (host->hw->flags & MSDC_CD_PIN_EN) {
        spin_lock_irqsave(&host->lock, flags);
        present = host->card_inserted;
        spin_unlock_irqrestore(&host->lock, flags);
    } else {
        present = 0; /* TODO? Check DAT3 pins for card detection */
    }

    return present;
}

static void mt6516_sd_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
    struct mt6516_sd_host *host = mmc_priv(mmc);
    u32 base = host->base;
    u32 tmp;

    if (host->hw->flags & MSDC_EXT_SDIO_IRQ) {
        if (enable)
            host->hw->enable_sdio_eirq();
        else
            host->hw->disable_sdio_eirq();
    } else {
        tmp = sdr_read32(SDIO_CFG);
        if (enable) {
            tmp |= SDIO_CFG_INTEN;
            sdr_set_bits(SDC_CFG, SDC_CFG_SDIO);
            sdr_write32(SDIO_CFG, tmp);
        } else {
            tmp &= ~SDIO_CFG_INTEN;
            sdr_write32(SDIO_CFG, tmp);
            sdr_clr_bits(SDC_CFG, SDC_CFG_SDIO);
        }
    }
}

static void mt6516_sd_enable_cd_irq(struct mmc_host *mmc, int enable)
{
    struct mt6516_sd_host *host = mmc_priv(mmc);
    struct mt6516_sd_host_hw *hw = host->hw;
    u32 base = host->base;
    u32 tmp;

    if ((hw->flags & MSDC_CD_PIN_EN) == 0) {
        /* Pull down card detection pin since it is not avaiable */
        if (hw->config_gpio_pin)
            hw->config_gpio_pin(MSDC_CD_PIN, GPIO_PULL_DOWN);    
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_PRCFG3, PIN_PULL_DOWN);
        sdr_clr_bits(MSDC_PS, MSDC_PS_PIEN0|MSDC_PS_POEN0|MSDC_PS_CDEN);
        sdr_clr_bits(MSDC_CFG, MSDC_CFG_PINEN);    
        return;
    }

    MSG(CFG, "[SD%d] CD IRQ Eanable(%d)\n", host->id, enable);

    if (enable) {
        if (hw->enable_cd_eirq) {
            hw->enable_cd_eirq();
        } else {
            /* card detection circuit relies on the core power so that the core power 
             * shouldn't be turned off. Here adds a reference count to keep 
             * the core power alive.
             */
            MSG(CFG, "[SD%d] Add ref. count of core power\n", host->id);
            (void)hwPowerOn(MT6516_POWER_VSDIO, VOL_3300, "SD");

            if (hw->config_gpio_pin)
                hw->config_gpio_pin(MSDC_CD_PIN, GPIO_PULL_UP);
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_PRCFG3, PIN_PULL_UP);
            sdr_set_field(MSDC_PS, MSDC_PS_DEBOUNCE, DEFAULT_DEBOUNCE);
            sdr_set_bits(MSDC_PS, MSDC_PS_PIEN0|MSDC_PS_POEN0|MSDC_PS_CDEN);
            sdr_set_bits(MSDC_CFG, MSDC_CFG_PINEN);
        }
    } else {
        if (hw->disable_cd_eirq) {
            hw->disable_cd_eirq();
        } else {
            if (hw->config_gpio_pin)
                hw->config_gpio_pin(MSDC_CD_PIN, GPIO_PULL_DOWN);    
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_PRCFG3, PIN_PULL_DOWN);
            sdr_clr_bits(MSDC_PS, MSDC_PS_PIEN0|MSDC_PS_POEN0|MSDC_PS_CDEN);
            sdr_clr_bits(MSDC_CFG, MSDC_CFG_PINEN);

            /* Here decreases a reference count to core power since card 
             * detection circuit is shutdown.
             */
            MSG(CFG, "[SD%d] Decrease ref. count of core power\n", host->id);
            (void)hwPowerDown(MT6516_POWER_VSDIO, "SD");
        }
    }
}

static struct mmc_host_ops mt6516_sd_ops = {
    .request         = mt6516_sd_request,
    .set_ios         = mt6516_sd_set_ios,
    .get_ro          = mt6516_sd_card_readonly,
    .get_cd          = mt6516_sd_card_inserted,
    .enable_sdio_irq = mt6516_sd_enable_sdio_irq,
};

static int mt6516_sd_probe(struct platform_device *pdev)
{
    struct mmc_host *mmc;
    struct resource *res;
    struct mt6516_sd_host *host;
    struct mt6516_sd_host_hw *hw;
    unsigned long base;
    int ret, irq, cirq;

    /* Allocate MMC host for this device */
    mmc = mmc_alloc_host(sizeof(struct mt6516_sd_host), &pdev->dev);
    if (!mmc)
        return -ENOMEM;

    hw   = (struct mt6516_sd_host_hw*)pdev->dev.platform_data;
    res  = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    irq  = platform_get_irq(pdev, 0);
    cirq = platform_get_irq(pdev, 1);
    base = res->start;

    BUG_ON(!hw);
    BUG_ON(irq < 0);
    BUG_ON(cirq < 0);

    res = request_mem_region(res->start, res->end - res->start + 1, DRV_NAME);

    if (res == NULL) {
        mmc_free_host(mmc);
        return -EBUSY;
    }

    /* Set host parameters */
    mmc->ops        = &mt6516_sd_ops;
    mmc->f_min      = HOST_MIN_SCLK;
    mmc->f_max      = HOST_MAX_SCLK;
    mmc->ocr_avail  = MMC_VDD_32_33 | MMC_VDD_33_34;
    if (hw->flags & MSDC_HIGHSPEED) {
        mmc->caps   = MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
    }
    if (hw->data_pins == 4) {
        mmc->caps  |= MMC_CAP_4_BIT_DATA;
    } else if (hw->data_pins == 8) {
        mmc->caps  |= MMC_CAP_8_BIT_DATA;
    }
    if ((hw->flags & MSDC_SDIO_IRQ) || (hw->flags & MSDC_EXT_SDIO_IRQ))
        mmc->caps |= MMC_CAP_SDIO_IRQ;

    /* MMC core transfer sizes tunable parameters */
    mmc->max_hw_segs   = 128;
    mmc->max_phys_segs = 128;
    mmc->max_seg_size  = 128 * 512;
    mmc->max_blk_size  = HOST_MAX_BLKSZ;
    mmc->max_blk_count = 65535;
    /* MTK, 20100527, Infinity, ALPS00005897 & ALPS00006942 { */
    //mmc->max_req_size  = mmc->max_phys_segs *  mmc->max_seg_size;
    mmc->max_req_size  = 65536;
    /* MTK, 20100527, Infinity, ALPS00005897 & ALPS00006942 } */

    host = mmc_priv(mmc);
    host->hw        = hw;
    host->mmc       = mmc;
    host->id        = pdev->id;
    host->base      = base;
    host->sd_irq    = irq;
    host->cd_irq    = cirq;
    host->mclk      = 0;
    host->hclk      = hw->clk_src == MSDC_CLKSRC_MCU ? MCU_CLK_FREQ : MCPLL_CLK_FREQ;
    host->sclk      = HOST_INI_SCLK;
    host->pm_state  = PMSG_RESUME;
    host->suspend   = 0;
    host->card_clkon = 0;    
    host->core_power = 0;
    host->power_mode = MMC_POWER_OFF;
    host->card_inserted = hw->flags & MSDC_REMOVABLE ? 0 : 1;
    host->timeout_ns = 0;
    host->timeout_clks = DEFAULT_DTOC * 65536;

    spin_lock_init(&host->lock);

    tasklet_init(&host->card_tasklet, mt6516_sd_tasklet_card, (ulong)host);
    tasklet_init(&host->fifo_tasklet, mt6516_sd_tasklet_fifo, (ulong)host);
    tasklet_init(&host->dma_tasklet, mt6516_sd_tasklet_dma, (ulong)host);

    mt6516_sd_init_hw(host);

    ret = request_irq((unsigned int)irq, mt6516_sd_irq, 0, DRV_NAME, host);
    if (ret)
        goto release;

    if (hw->flags & MSDC_CD_PIN_EN) {
        if (hw->request_cd_eirq) {
            hw->request_cd_eirq(mt6516_sd_cd_eirq, (void*)host);
        } else {
            ret = request_irq((unsigned int)cirq, mt6516_sd_irq, 0, DRV_NAME, host);
            if (ret)
                goto free_irq;
        }
    }

    if (hw->request_sdio_eirq)
        hw->request_sdio_eirq(mt6516_sd_sdio_eirq, (void*)host);

    if (hw->register_pm)
        hw->register_pm(mt6516_sd_pm_change, (void*)host);

    platform_set_drvdata(pdev, mmc);

#ifdef CFG_PROFILING
    mmc_perf_init(host, pdev);
#endif

    ret = mmc_add_host(mmc);
    if (ret)
        goto free_cirq;

    /* Config card detection pin and enable interrupts */
    if (hw->flags & MSDC_CD_PIN_EN) {
        mt6516_sd_enable_cd_irq(mmc, 1);
    } else {
        mt6516_sd_enable_cd_irq(mmc, 0);
    }
    sdr_set_bits(MSDC_CFG, MSDC_CFG_INTEN);

    return 0;

free_cirq:
    free_irq(cirq, host);
free_irq:
    free_irq(irq, host);
release:
    platform_set_drvdata(pdev, NULL);
    mmc_free_host(mmc);
    mt6516_sd_deinit_hw(host);

    tasklet_kill(&host->card_tasklet);
    tasklet_kill(&host->fifo_tasklet);
    tasklet_kill(&host->dma_tasklet);

    if (res)
        release_mem_region(res->start, res->end - res->start + 1);

    return ret;
}

static int mt6516_sd_remove(struct platform_device *pdev)
{
    struct mmc_host *mmc  = platform_get_drvdata(pdev);
    struct mt6516_sd_host *host = mmc_priv(mmc);
    struct mt6516_sd_host_hw *hw = host->hw;
    struct resource *res;
    
    BUG_ON(!mmc || !host);

    printk(KERN_INFO "[SD%d] removed !!!\n", host->id);

    platform_set_drvdata(pdev, NULL);
    mmc_remove_host(host->mmc);
    mmc_free_host(host->mmc);
    mt6516_sd_deinit_hw(host);

    tasklet_kill(&host->card_tasklet);
    tasklet_kill(&host->fifo_tasklet);
    tasklet_kill(&host->dma_tasklet);
    free_irq(host->sd_irq, host);
    if ((hw->flags & MSDC_CD_PIN_EN) && !hw->request_cd_eirq)
        free_irq(host->cd_irq, host);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    if (res)
        release_mem_region(res->start, res->end - res->start + 1);

    return 0;
}

#ifdef CONFIG_PM
static int mt6516_sd_suspend(struct platform_device *pdev, pm_message_t state)
{
    int ret = 0;
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    struct mt6516_sd_host *host = mmc_priv(mmc);

    if (mmc && state.event == PM_EVENT_SUSPEND && 
       (host->hw->flags & MSDC_SYS_SUSPEND))
        mt6516_sd_pm_change(state, (void*)host);

    return ret;
}

static int mt6516_sd_resume(struct platform_device *pdev)
{
    int ret = 0;
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    struct mt6516_sd_host *host = mmc_priv(mmc);
    struct pm_message state;

    state.event = PM_EVENT_RESUME;
    if (mmc && (host->hw->flags & MSDC_SYS_SUSPEND))
        mt6516_sd_pm_change(state, (void*)host);

    return ret;
}
#endif

static struct platform_driver mt6516_sd_driver = {
    .probe   = mt6516_sd_probe,
    .remove  = mt6516_sd_remove,
#ifdef CONFIG_PM
    .suspend = mt6516_sd_suspend,
    .resume  = mt6516_sd_resume,
#endif
    .driver  = {
        .name  = DRV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init mt6516_sd_init(void)
{
    int ret;

    ret = platform_driver_register(&mt6516_sd_driver);
    if (ret) {
        printk(KERN_ERR DRV_NAME ": Can't register driver\n");
        return ret;
    }
    printk(KERN_INFO DRV_NAME ": MediaTek MT6516 SD/MMC Card Driver\n");

    return 0;
}

static void __exit mt6516_sd_exit(void)
{
    platform_driver_unregister(&mt6516_sd_driver);
}

module_init(mt6516_sd_init);
module_exit(mt6516_sd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek MT6516 SD/MMC Card Driver");
MODULE_AUTHOR("Infinity Chen <infinity.chen@mediatek.com>");
