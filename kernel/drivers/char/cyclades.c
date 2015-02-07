
#undef	BLOCKMOVE
#define	Z_WAKE
#undef	Z_EXT_CHARS_IN_BUFFER


#define CY_VERSION	"2.5"


#define NR_CARDS	4


#define NR_PORTS	256

#define ZE_V1_NPORTS	64
#define ZO_V1	0
#define ZO_V2	1
#define ZE_V1	2

#define	SERIAL_PARANOIA_CHECK
#undef	CY_DEBUG_OPEN
#undef	CY_DEBUG_THROTTLE
#undef	CY_DEBUG_OTHER
#undef	CY_DEBUG_IO
#undef	CY_DEBUG_COUNT
#undef	CY_DEBUG_DTR
#undef	CY_DEBUG_WAIT_UNTIL_SENT
#undef	CY_DEBUG_INTERRUPTS
#undef	CY_16Y_HACK
#undef	CY_ENABLE_MONITORING
#undef	CY_PCI_DEBUG

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/cyclades.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/firmware.h>

#include <asm/system.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <linux/uaccess.h>

#include <linux/kernel.h>
#include <linux/pci.h>

#include <linux/stat.h>
#include <linux/proc_fs.h>

static void cy_throttle(struct tty_struct *tty);
static void cy_send_xchar(struct tty_struct *tty, char ch);

#define IS_CYC_Z(card) ((card).num_chips == (unsigned int)-1)

#define Z_FPGA_CHECK(card) \
	((readl(&((struct RUNTIME_9060 __iomem *) \
		((card).ctl_addr))->init_ctrl) & (1<<17)) != 0)

#define ISZLOADED(card)	(((ZO_V1 == readl(&((struct RUNTIME_9060 __iomem *) \
			((card).ctl_addr))->mail_box_0)) || \
			Z_FPGA_CHECK(card)) && \
			(ZFIRM_ID == readl(&((struct FIRM_ID __iomem *) \
			((card).base_addr+ID_ADDRESS))->signature)))

#ifndef SERIAL_XMIT_SIZE
#define	SERIAL_XMIT_SIZE	(min(PAGE_SIZE, 4096))
#endif
#define WAKEUP_CHARS		256

#define STD_COM_FLAGS (0)

/* firmware stuff */
#define ZL_MAX_BLOCKS	16
#define DRIVER_VERSION	0x02010203
#define RAM_SIZE 0x80000

#define Z_FPGA_LOADED(X)	((readl(&(X)->init_ctrl) & (1<<17)) != 0)

enum zblock_type {
	ZBLOCK_PRG = 0,
	ZBLOCK_FPGA = 1
};

struct zfile_header {
	char name[64];
	char date[32];
	char aux[32];
	u32 n_config;
	u32 config_offset;
	u32 n_blocks;
	u32 block_offset;
	u32 reserved[9];
} __attribute__ ((packed));

struct zfile_config {
	char name[64];
	u32 mailbox;
	u32 function;
	u32 n_blocks;
	u32 block_list[ZL_MAX_BLOCKS];
} __attribute__ ((packed));

struct zfile_block {
	u32 type;
	u32 file_offset;
	u32 ram_offset;
	u32 size;
} __attribute__ ((packed));

static struct tty_driver *cy_serial_driver;

#ifdef CONFIG_ISA

static unsigned int cy_isa_addresses[] = {
	0xD0000,
	0xD2000,
	0xD4000,
	0xD6000,
	0xD8000,
	0xDA000,
	0xDC000,
	0xDE000,
	0, 0, 0, 0, 0, 0, 0, 0
};

#define NR_ISA_ADDRS ARRAY_SIZE(cy_isa_addresses)

#ifdef MODULE
static long maddr[NR_CARDS];
static int irq[NR_CARDS];

module_param_array(maddr, long, NULL, 0);
module_param_array(irq, int, NULL, 0);
#endif

#endif				/* CONFIG_ISA */

static struct cyclades_card cy_card[NR_CARDS];

static int cy_next_channel;	/* next minor available */

static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200,
	1800, 2400, 4800, 9600, 19200, 38400, 57600, 76800, 115200, 150000,
	230400, 0
};

static char baud_co_25[] = {	/* 25 MHz clock option table */
	/* value =>    00    01   02    03    04 */
	/* divide by    8    32   128   512  2048 */
	0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03, 0x02,
	0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static char baud_bpr_25[] = {	/* 25 MHz baud rate period table */
	0x00, 0xf5, 0xa3, 0x6f, 0x5c, 0x51, 0xf5, 0xa3, 0x51, 0xa3,
	0x6d, 0x51, 0xa3, 0x51, 0xa3, 0x51, 0x36, 0x29, 0x1b, 0x15
};

static char baud_co_60[] = {	/* 60 MHz clock option table (CD1400 J) */
	/* value =>    00    01   02    03    04 */
	/* divide by    8    32   128   512  2048 */
	0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x03, 0x03,
	0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00
};

static char baud_bpr_60[] = {	/* 60 MHz baud rate period table (CD1400 J) */
	0x00, 0x82, 0x21, 0xff, 0xdb, 0xc3, 0x92, 0x62, 0xc3, 0x62,
	0x41, 0xc3, 0x62, 0xc3, 0x62, 0xc3, 0x82, 0x62, 0x41, 0x32,
	0x21
};

static char baud_cor3[] = {	/* receive threshold */
	0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
	0x0a, 0x0a, 0x0a, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x07,
	0x07
};


static char rflow_thr[] = {	/* rflow threshold */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
	0x0a
};

static int cy_chip_offset[] = { 0x0000,
	0x0400,
	0x0800,
	0x0C00,
	0x0200,
	0x0600,
	0x0A00,
	0x0E00
};

/* PCI related definitions */

#ifdef CONFIG_PCI
static struct pci_device_id cy_pci_dev_id[] __devinitdata = {
	/* PCI < 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_Y_Lo) },
	/* PCI > 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_Y_Hi) },
	/* 4Y PCI < 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_4Y_Lo) },
	/* 4Y PCI > 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_4Y_Hi) },
	/* 8Y PCI < 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_8Y_Lo) },
	/* 8Y PCI > 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_8Y_Hi) },
	/* Z PCI < 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_Z_Lo) },
	/* Z PCI > 1Mb */
	{ PCI_DEVICE(PCI_VENDOR_ID_CYCLADES, PCI_DEVICE_ID_CYCLOM_Z_Hi) },
	{ }			/* end of table */
};
MODULE_DEVICE_TABLE(pci, cy_pci_dev_id);
#endif

static void cy_start(struct tty_struct *);
static void set_line_char(struct cyclades_port *);
static int cyz_issue_cmd(struct cyclades_card *, __u32, __u8, __u32);
#ifdef CONFIG_ISA
static unsigned detect_isa_irq(void __iomem *);
#endif				/* CONFIG_ISA */

static int cyclades_get_proc_info(char *, char **, off_t, int, int *, void *);

#ifndef CONFIG_CYZ_INTR
static void cyz_poll(unsigned long);

/* The Cyclades-Z polling cycle is defined by this variable */
static long cyz_polling_cycle = CZ_DEF_POLL;

static DEFINE_TIMER(cyz_timerlist, cyz_poll, 0, 0);

#else				/* CONFIG_CYZ_INTR */
static void cyz_rx_restart(unsigned long);
static struct timer_list cyz_rx_full_timer[NR_PORTS];
#endif				/* CONFIG_CYZ_INTR */

static inline int serial_paranoia_check(struct cyclades_port *info,
		char *name, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
	if (!info) {
		printk(KERN_WARNING "cyc Warning: null cyclades_port for (%s) "
				"in %s\n", name, routine);
		return 1;
	}

	if (info->magic != CYCLADES_MAGIC) {
		printk(KERN_WARNING "cyc Warning: bad magic number for serial "
				"struct (%s) in %s\n", name, routine);
		return 1;
	}
#endif
	return 0;
}				/* serial_paranoia_check */

/***********************************************************/
/********* Start of block of Cyclom-Y specific code ********/

static int cyy_issue_cmd(void __iomem *base_addr, u_char cmd, int index)
{
	unsigned int i;

	/* Check to see that the previous command has completed */
	for (i = 0; i < 100; i++) {
		if (readb(base_addr + (CyCCR << index)) == 0)
			break;
		udelay(10L);
	}
	/* if the CCR never cleared, the previous command
	   didn't finish within the "reasonable time" */
	if (i == 100)
		return -1;

	/* Issue the new command */
	cy_writeb(base_addr + (CyCCR << index), cmd);

	return 0;
}				/* cyy_issue_cmd */

#ifdef CONFIG_ISA
/* ISA interrupt detection code */
static unsigned detect_isa_irq(void __iomem *address)
{
	int irq;
	unsigned long irqs, flags;
	int save_xir, save_car;
	int index = 0;		/* IRQ probing is only for ISA */

	/* forget possible initially masked and pending IRQ */
	irq = probe_irq_off(probe_irq_on());

	/* Clear interrupts on the board first */
	cy_writeb(address + (Cy_ClrIntr << index), 0);
	/* Cy_ClrIntr is 0x1800 */

	irqs = probe_irq_on();
	/* Wait ... */
	udelay(5000L);

	/* Enable the Tx interrupts on the CD1400 */
	local_irq_save(flags);
	cy_writeb(address + (CyCAR << index), 0);
	cyy_issue_cmd(address, CyCHAN_CTL | CyENB_XMTR, index);

	cy_writeb(address + (CyCAR << index), 0);
	cy_writeb(address + (CySRER << index),
		  readb(address + (CySRER << index)) | CyTxRdy);
	local_irq_restore(flags);

	/* Wait ... */
	udelay(5000L);

	/* Check which interrupt is in use */
	irq = probe_irq_off(irqs);

	/* Clean up */
	save_xir = (u_char) readb(address + (CyTIR << index));
	save_car = readb(address + (CyCAR << index));
	cy_writeb(address + (CyCAR << index), (save_xir & 0x3));
	cy_writeb(address + (CySRER << index),
		  readb(address + (CySRER << index)) & ~CyTxRdy);
	cy_writeb(address + (CyTIR << index), (save_xir & 0x3f));
	cy_writeb(address + (CyCAR << index), (save_car));
	cy_writeb(address + (Cy_ClrIntr << index), 0);
	/* Cy_ClrIntr is 0x1800 */

	return (irq > 0) ? irq : 0;
}
#endif				/* CONFIG_ISA */

static void cyy_chip_rx(struct cyclades_card *cinfo, int chip,
		void __iomem *base_addr)
{
	struct cyclades_port *info;
	struct tty_struct *tty;
	int len, index = cinfo->bus_index;
	u8 save_xir, channel, save_car, data, char_count;

#ifdef CY_DEBUG_INTERRUPTS
	printk(KERN_DEBUG "cyy_interrupt: rcvd intr, chip %d\n", chip);
#endif
	/* determine the channel & change to that context */
	save_xir = readb(base_addr + (CyRIR << index));
	channel = save_xir & CyIRChannel;
	info = &cinfo->ports[channel + chip * 4];
	save_car = readb(base_addr + (CyCAR << index));
	cy_writeb(base_addr + (CyCAR << index), save_xir);

	/* if there is nowhere to put the data, discard it */
	if (info->port.tty == NULL) {
		if ((readb(base_addr + (CyRIVR << index)) & CyIVRMask) ==
				CyIVRRxEx) {	/* exception */
			data = readb(base_addr + (CyRDSR << index));
		} else {	/* normal character reception */
			char_count = readb(base_addr + (CyRDCR << index));
			while (char_count--)
				data = readb(base_addr + (CyRDSR << index));
		}
		goto end;
	}
	/* there is an open port for this data */
	tty = info->port.tty;
	if ((readb(base_addr + (CyRIVR << index)) & CyIVRMask) ==
			CyIVRRxEx) {	/* exception */
		data = readb(base_addr + (CyRDSR << index));

		/* For statistics only */
		if (data & CyBREAK)
			info->icount.brk++;
		else if (data & CyFRAME)
			info->icount.frame++;
		else if (data & CyPARITY)
			info->icount.parity++;
		else if (data & CyOVERRUN)
			info->icount.overrun++;

		if (data & info->ignore_status_mask) {
			info->icount.rx++;
			return;
		}
		if (tty_buffer_request_room(tty, 1)) {
			if (data & info->read_status_mask) {
				if (data & CyBREAK) {
					tty_insert_flip_char(tty,
						readb(base_addr + (CyRDSR <<
							index)), TTY_BREAK);
					info->icount.rx++;
					if (info->port.flags & ASYNC_SAK)
						do_SAK(tty);
				} else if (data & CyFRAME) {
					tty_insert_flip_char(tty,
						readb(base_addr + (CyRDSR <<
							index)), TTY_FRAME);
					info->icount.rx++;
					info->idle_stats.frame_errs++;
				} else if (data & CyPARITY) {
					/* Pieces of seven... */
					tty_insert_flip_char(tty,
						readb(base_addr + (CyRDSR <<
							index)), TTY_PARITY);
					info->icount.rx++;
					info->idle_stats.parity_errs++;
				} else if (data & CyOVERRUN) {
					tty_insert_flip_char(tty, 0,
							TTY_OVERRUN);
					info->icount.rx++;
					/* If the flip buffer itself is
					   overflowing, we still lose
					   the next incoming character.
					 */
					tty_insert_flip_char(tty,
						readb(base_addr + (CyRDSR <<
							index)), TTY_FRAME);
					info->icount.rx++;
					info->idle_stats.overruns++;
				/* These two conditions may imply */
				/* a normal read should be done. */
				/* } else if(data & CyTIMEOUT) { */
				/* } else if(data & CySPECHAR) { */
				} else {
					tty_insert_flip_char(tty, 0,
							TTY_NORMAL);
					info->icount.rx++;
				}
			} else {
				tty_insert_flip_char(tty, 0, TTY_NORMAL);
				info->icount.rx++;
			}
		} else {
			/* there was a software buffer overrun and nothing
			 * could be done about it!!! */
			info->icount.buf_overrun++;
			info->idle_stats.overruns++;
		}
	} else {	/* normal character reception */
		/* load # chars available from the chip */
		char_count = readb(base_addr + (CyRDCR << index));

#ifdef CY_ENABLE_MONITORING
		++info->mon.int_count;
		info->mon.char_count += char_count;
		if (char_count > info->mon.char_max)
			info->mon.char_max = char_count;
		info->mon.char_last = char_count;
#endif
		len = tty_buffer_request_room(tty, char_count);
		while (len--) {
			data = readb(base_addr + (CyRDSR << index));
			tty_insert_flip_char(tty, data, TTY_NORMAL);
			info->idle_stats.recv_bytes++;
			info->icount.rx++;
#ifdef CY_16Y_HACK
			udelay(10L);
#endif
		}
		info->idle_stats.recv_idle = jiffies;
	}
	tty_schedule_flip(tty);
end:
	/* end of service */
	cy_writeb(base_addr + (CyRIR << index), save_xir & 0x3f);
	cy_writeb(base_addr + (CyCAR << index), save_car);
}

static void cyy_chip_tx(struct cyclades_card *cinfo, unsigned int chip,
		void __iomem *base_addr)
{
	struct cyclades_port *info;
	int char_count, index = cinfo->bus_index;
	u8 save_xir, channel, save_car, outch;

	/* Since we only get here when the transmit buffer
	   is empty, we know we can always stuff a dozen
	   characters. */
#ifdef CY_DEBUG_INTERRUPTS
	printk(KERN_DEBUG "cyy_interrupt: xmit intr, chip %d\n", chip);
#endif

	/* determine the channel & change to that context */
	save_xir = readb(base_addr + (CyTIR << index));
	channel = save_xir & CyIRChannel;
	save_car = readb(base_addr + (CyCAR << index));
	cy_writeb(base_addr + (CyCAR << index), save_xir);

	/* validate the port# (as configured and open) */
	if (channel + chip * 4 >= cinfo->nports) {
		cy_writeb(base_addr + (CySRER << index),
			  readb(base_addr + (CySRER << index)) & ~CyTxRdy);
		goto end;
	}
	info = &cinfo->ports[channel + chip * 4];
	if (info->port.tty == NULL) {
		cy_writeb(base_addr + (CySRER << index),
			  readb(base_addr + (CySRER << index)) & ~CyTxRdy);
		goto end;
	}

	/* load the on-chip space for outbound data */
	char_count = info->xmit_fifo_size;

	if (info->x_char) {	/* send special char */
		outch = info->x_char;
		cy_writeb(base_addr + (CyTDR << index), outch);
		char_count--;
		info->icount.tx++;
		info->x_char = 0;
	}

	if (info->breakon || info->breakoff) {
		if (info->breakon) {
			cy_writeb(base_addr + (CyTDR << index), 0);
			cy_writeb(base_addr + (CyTDR << index), 0x81);
			info->breakon = 0;
			char_count -= 2;
		}
		if (info->breakoff) {
			cy_writeb(base_addr + (CyTDR << index), 0);
			cy_writeb(base_addr + (CyTDR << index), 0x83);
			info->breakoff = 0;
			char_count -= 2;
		}
	}

	while (char_count-- > 0) {
		if (!info->xmit_cnt) {
			if (readb(base_addr + (CySRER << index)) & CyTxMpty) {
				cy_writeb(base_addr + (CySRER << index),
					readb(base_addr + (CySRER << index)) &
						~CyTxMpty);
			} else {
				cy_writeb(base_addr + (CySRER << index),
					(readb(base_addr + (CySRER << index)) &
						~CyTxRdy) | CyTxMpty);
			}
			goto done;
		}
		if (info->port.xmit_buf == NULL) {
			cy_writeb(base_addr + (CySRER << index),
				readb(base_addr + (CySRER << index)) &
					~CyTxRdy);
			goto done;
		}
		if (info->port.tty->stopped || info->port.tty->hw_stopped) {
			cy_writeb(base_addr + (CySRER << index),
				readb(base_addr + (CySRER << index)) &
					~CyTxRdy);
			goto done;
		}
		/* Because the Embedded Transmit Commands have been enabled,
		 * we must check to see if the escape character, NULL, is being
		 * sent. If it is, we must ensure that there is room for it to
		 * be doubled in the output stream.  Therefore we no longer
		 * advance the pointer when the character is fetched, but
		 * rather wait until after the check for a NULL output
		 * character. This is necessary because there may not be room
		 * for the two chars needed to send a NULL.)
		 */
		outch = info->port.xmit_buf[info->xmit_tail];
		if (outch) {
			info->xmit_cnt--;
			info->xmit_tail = (info->xmit_tail + 1) &
					(SERIAL_XMIT_SIZE - 1);
			cy_writeb(base_addr + (CyTDR << index), outch);
			info->icount.tx++;
		} else {
			if (char_count > 1) {
				info->xmit_cnt--;
				info->xmit_tail = (info->xmit_tail + 1) &
					(SERIAL_XMIT_SIZE - 1);
				cy_writeb(base_addr + (CyTDR << index), outch);
				cy_writeb(base_addr + (CyTDR << index), 0);
				info->icount.tx++;
				char_count--;
			}
		}
	}

done:
	tty_wakeup(info->port.tty);
end:
	/* end of service */
	cy_writeb(base_addr + (CyTIR << index), save_xir & 0x3f);
	cy_writeb(base_addr + (CyCAR << index), save_car);
}

static void cyy_chip_modem(struct cyclades_card *cinfo, int chip,
		void __iomem *base_addr)
{
	struct cyclades_port *info;
	int index = cinfo->bus_index;
	u8 save_xir, channel, save_car, mdm_change, mdm_status;

	/* determine the channel & change to that context */
	save_xir = readb(base_addr + (CyMIR << index));
	channel = save_xir & CyIRChannel;
	info = &cinfo->ports[channel + chip * 4];
	save_car = readb(base_addr + (CyCAR << index));
	cy_writeb(base_addr + (CyCAR << index), save_xir);

	mdm_change = readb(base_addr + (CyMISR << index));
	mdm_status = readb(base_addr + (CyMSVR1 << index));

	if (!info->port.tty)
		goto end;

	if (mdm_change & CyANY_DELTA) {
		/* For statistics only */
		if (mdm_change & CyDCD)
			info->icount.dcd++;
		if (mdm_change & CyCTS)
			info->icount.cts++;
		if (mdm_change & CyDSR)
			info->icount.dsr++;
		if (mdm_change & CyRI)
			info->icount.rng++;

		wake_up_interruptible(&info->delta_msr_wait);
	}

	if ((mdm_change & CyDCD) && (info->port.flags & ASYNC_CHECK_CD)) {
		if (!(mdm_status & CyDCD)) {
			tty_hangup(info->port.tty);
			info->port.flags &= ~ASYNC_NORMAL_ACTIVE;
		}
		wake_up_interruptible(&info->port.open_wait);
	}
	if ((mdm_change & CyCTS) && (info->port.flags & ASYNC_CTS_FLOW)) {
		if (info->port.tty->hw_stopped) {
			if (mdm_status & CyCTS) {
				/* cy_start isn't used
				   because... !!! */
				info->port.tty->hw_stopped = 0;
				cy_writeb(base_addr + (CySRER << index),
					readb(base_addr + (CySRER << index)) |
						CyTxRdy);
				tty_wakeup(info->port.tty);
			}
		} else {
			if (!(mdm_status & CyCTS)) {
				/* cy_stop isn't used
				   because ... !!! */
				info->port.tty->hw_stopped = 1;
				cy_writeb(base_addr + (CySRER << index),
					readb(base_addr + (CySRER << index)) &
						~CyTxRdy);
			}
		}
	}
end:
	/* end of service */
	cy_writeb(base_addr + (CyMIR << index), save_xir & 0x3f);
	cy_writeb(base_addr + (CyCAR << index), save_car);
}

static irqreturn_t cyy_interrupt(int irq, void *dev_id)
{
	int status;
	struct cyclades_card *cinfo = dev_id;
	void __iomem *base_addr, *card_base_addr;
	unsigned int chip, too_many, had_work;
	int index;

	if (unlikely(cinfo == NULL)) {
#ifdef CY_DEBUG_INTERRUPTS
		printk(KERN_DEBUG "cyy_interrupt: spurious interrupt %d\n",
				irq);
#endif
		return IRQ_NONE;	/* spurious interrupt */
	}

	card_base_addr = cinfo->base_addr;
	index = cinfo->bus_index;

	/* card was not initialized yet (e.g. DEBUG_SHIRQ) */
	if (unlikely(card_base_addr == NULL))
		return IRQ_HANDLED;

	/* This loop checks all chips in the card.  Make a note whenever
	   _any_ chip had some work to do, as this is considered an
	   indication that there will be more to do.  Only when no chip
	   has any work does this outermost loop exit.
	 */
	do {
		had_work = 0;
		for (chip = 0; chip < cinfo->num_chips; chip++) {
			base_addr = cinfo->base_addr +
					(cy_chip_offset[chip] << index);
			too_many = 0;
			while ((status = readb(base_addr +
						(CySVRR << index))) != 0x00) {
				had_work++;
			/* The purpose of the following test is to ensure that
			   no chip can monopolize the driver.  This forces the
			   chips to be checked in a round-robin fashion (after
			   draining each of a bunch (1000) of characters).
			 */
				if (1000 < too_many++)
					break;
				spin_lock(&cinfo->card_lock);
				if (status & CySRReceive) /* rx intr */
					cyy_chip_rx(cinfo, chip, base_addr);
				if (status & CySRTransmit) /* tx intr */
					cyy_chip_tx(cinfo, chip, base_addr);
				if (status & CySRModem) /* modem intr */
					cyy_chip_modem(cinfo, chip, base_addr);
				spin_unlock(&cinfo->card_lock);
			}
		}
	} while (had_work);

	/* clear interrupts */
	spin_lock(&cinfo->card_lock);
	cy_writeb(card_base_addr + (Cy_ClrIntr << index), 0);
	/* Cy_ClrIntr is 0x1800 */
	spin_unlock(&cinfo->card_lock);
	return IRQ_HANDLED;
}				/* cyy_interrupt */

/***********************************************************/
/********* End of block of Cyclom-Y specific code **********/
/******** Start of block of Cyclades-Z specific code *******/
/***********************************************************/

static int
cyz_fetch_msg(struct cyclades_card *cinfo,
		__u32 *channel, __u8 *cmd, __u32 *param)
{
	struct FIRM_ID __iomem *firm_id;
	struct ZFW_CTRL __iomem *zfw_ctrl;
	struct BOARD_CTRL __iomem *board_ctrl;
	unsigned long loc_doorbell;

	firm_id = cinfo->base_addr + ID_ADDRESS;
	if (!ISZLOADED(*cinfo))
		return -1;
	zfw_ctrl = cinfo->base_addr + (readl(&firm_id->zfwctrl_addr) & 0xfffff);
	board_ctrl = &zfw_ctrl->board_ctrl;

	loc_doorbell = readl(&((struct RUNTIME_9060 __iomem *)
				  (cinfo->ctl_addr))->loc_doorbell);
	if (loc_doorbell) {
		*cmd = (char)(0xff & loc_doorbell);
		*channel = readl(&board_ctrl->fwcmd_channel);
		*param = (__u32) readl(&board_ctrl->fwcmd_param);
		cy_writel(&((struct RUNTIME_9060 __iomem *)(cinfo->ctl_addr))->
			  loc_doorbell, 0xffffffff);
		return 1;
	}
	return 0;
}				/* cyz_fetch_msg */

static int
cyz_issue_cmd(struct cyclades_card *cinfo,
		__u32 channel, __u8 cmd, __u32 param)
{
	struct FIRM_ID __iomem *firm_id;
	struct ZFW_CTRL __iomem *zfw_ctrl;
	struct BOARD_CTRL __iomem *board_ctrl;
	__u32 __iomem *pci_doorbell;
	unsigned int index;

	firm_id = cinfo->base_addr + ID_ADDRESS;
	if (!ISZLOADED(*cinfo))
		return -1;

	zfw_ctrl = cinfo->base_addr + (readl(&firm_id->zfwctrl_addr) & 0xfffff);
	board_ctrl = &zfw_ctrl->board_ctrl;

	index = 0;
	pci_doorbell =
	    &((struct RUNTIME_9060 __iomem *)(cinfo->ctl_addr))->pci_doorbell;
	while ((readl(pci_doorbell) & 0xff) != 0) {
		if (index++ == 1000)
			return (int)(readl(pci_doorbell) & 0xff);
		udelay(50L);
	}
	cy_writel(&board_ctrl->hcmd_channel, channel);
	cy_writel(&board_ctrl->hcmd_param, param);
	cy_writel(pci_doorbell, (long)cmd);

	return 0;
}				/* cyz_issue_cmd */

static void cyz_handle_rx(struct cyclades_port *info,
		struct BUF_CTRL __iomem *buf_ctrl)
{
	struct cyclades_card *cinfo = info->card;
	struct tty_struct *tty = info->port.tty;
	unsigned int char_count;
	int len;
#ifdef BLOCKMOVE
	unsigned char *buf;
#else
	char data;
#endif
	__u32 rx_put, rx_get, new_rx_get, rx_bufsize, rx_bufaddr;

	rx_get = new_rx_get = readl(&buf_ctrl->rx_get);
	rx_put = readl(&buf_ctrl->rx_put);
	rx_bufsize = readl(&buf_ctrl->rx_bufsize);
	rx_bufaddr = readl(&buf_ctrl->rx_bufaddr);
	if (rx_put >= rx_get)
		char_count = rx_put - rx_get;
	else
		char_count = rx_put - rx_get + rx_bufsize;

	if (char_count) {
#ifdef CY_ENABLE_MONITORING
		info->mon.int_count++;
		info->mon.char_count += char_count;
		if (char_count > info->mon.char_max)
			info->mon.char_max = char_count;
		info->mon.char_last = char_count;
#endif
		if (tty == NULL) {
			/* flush received characters */
			new_rx_get = (new_rx_get + char_count) &
					(rx_bufsize - 1);
			info->rflush_count++;
		} else {
#ifdef BLOCKMOVE
		/* we'd like to use memcpy(t, f, n) and memset(s, c, count)
		   for performance, but because of buffer boundaries, there
		   may be several steps to the operation */
			while (1) {
				len = tty_prepare_flip_string(tty, &buf,
						char_count);
				if (!len)
					break;

				len = min_t(unsigned int, min(len, char_count),
						rx_bufsize - new_rx_get);

				memcpy_fromio(buf, cinfo->base_addr +
						rx_bufaddr + new_rx_get, len);

				new_rx_get = (new_rx_get + len) &
						(rx_bufsize - 1);
				char_count -= len;
				info->icount.rx += len;
				info->idle_stats.recv_bytes += len;
			}
#else
			len = tty_buffer_request_room(tty, char_count);
			while (len--) {
				data = readb(cinfo->base_addr + rx_bufaddr +
						new_rx_get);
				new_rx_get = (new_rx_get + 1) &
							(rx_bufsize - 1);
				tty_insert_flip_char(tty, data, TTY_NORMAL);
				info->idle_stats.recv_bytes++;
				info->icount.rx++;
			}
#endif
#ifdef CONFIG_CYZ_INTR
		/* Recalculate the number of chars in the RX buffer and issue
		   a cmd in case it's higher than the RX high water mark */
			rx_put = readl(&buf_ctrl->rx_put);
			if (rx_put >= rx_get)
				char_count = rx_put - rx_get;
			else
				char_count = rx_put - rx_get + rx_bufsize;
			if (char_count >= readl(&buf_ctrl->rx_threshold) &&
					!timer_pending(&cyz_rx_full_timer[
							info->line]))
				mod_timer(&cyz_rx_full_timer[info->line],
						jiffies + 1);
#endif
			info->idle_stats.recv_idle = jiffies;
			tty_schedule_flip(tty);
		}
		/* Update rx_get */
		cy_writel(&buf_ctrl->rx_get, new_rx_get);
	}
}

static void cyz_handle_tx(struct cyclades_port *info,
		struct BUF_CTRL __iomem *buf_ctrl)
{
	struct cyclades_card *cinfo = info->card;
	struct tty_struct *tty = info->port.tty;
	u8 data;
	unsigned int char_count;
#ifdef BLOCKMOVE
	int small_count;
#endif
	__u32 tx_put, tx_get, tx_bufsize, tx_bufaddr;

	if (info->xmit_cnt <= 0)	/* Nothing to transmit */
		return;

	tx_get = readl(&buf_ctrl->tx_get);
	tx_put = readl(&buf_ctrl->tx_put);
	tx_bufsize = readl(&buf_ctrl->tx_bufsize);
	tx_bufaddr = readl(&buf_ctrl->tx_bufaddr);
	if (tx_put >= tx_get)
		char_count = tx_get - tx_put - 1 + tx_bufsize;
	else
		char_count = tx_get - tx_put - 1;

	if (char_count) {

		if (tty == NULL)
			goto ztxdone;

		if (info->x_char) {	/* send special char */
			data = info->x_char;

			cy_writeb(cinfo->base_addr + tx_bufaddr + tx_put, data);
			tx_put = (tx_put + 1) & (tx_bufsize - 1);
			info->x_char = 0;
			char_count--;
			info->icount.tx++;
		}
#ifdef BLOCKMOVE
		while (0 < (small_count = min_t(unsigned int,
				tx_bufsize - tx_put, min_t(unsigned int,
					(SERIAL_XMIT_SIZE - info->xmit_tail),
					min_t(unsigned int, info->xmit_cnt,
						char_count))))) {

			memcpy_toio((char *)(cinfo->base_addr + tx_bufaddr +
					tx_put),
					&info->port.xmit_buf[info->xmit_tail],
					small_count);

			tx_put = (tx_put + small_count) & (tx_bufsize - 1);
			char_count -= small_count;
			info->icount.tx += small_count;
			info->xmit_cnt -= small_count;
			info->xmit_tail = (info->xmit_tail + small_count) &
					(SERIAL_XMIT_SIZE - 1);
		}
#else
		while (info->xmit_cnt && char_count) {
			data = info->port.xmit_buf[info->xmit_tail];
			info->xmit_cnt--;
			info->xmit_tail = (info->xmit_tail + 1) &
					(SERIAL_XMIT_SIZE - 1);

			cy_writeb(cinfo->base_addr + tx_bufaddr + tx_put, data);
			tx_put = (tx_put + 1) & (tx_bufsize - 1);
			char_count--;
			info->icount.tx++;
		}
#endif
		tty_wakeup(tty);
ztxdone:
		/* Update tx_put */
		cy_writel(&buf_ctrl->tx_put, tx_put);
	}
}

static void cyz_handle_cmd(struct cyclades_card *cinfo)
{
	struct tty_struct *tty;
	struct cyclades_port *info;
	static struct FIRM_ID __iomem *firm_id;
	static struct ZFW_CTRL __iomem *zfw_ctrl;
	static struct BOARD_CTRL __iomem *board_ctrl;
	static struct CH_CTRL __iomem *ch_ctrl;
	static struct BUF_CTRL __iomem *buf_ctrl;
	__u32 channel;
	__u8 cmd;
	__u32 param;
	__u32 hw_ver, fw_ver;
	int special_count;
	int delta_count;

	firm_id = cinfo->base_addr + ID_ADDRESS;
	zfw_ctrl = cinfo->base_addr + (readl(&firm_id->zfwctrl_addr) & 0xfffff);
	board_ctrl = &zfw_ctrl->board_ctrl;
	fw_ver = readl(&board_ctrl->fw_version);
	hw_ver = readl(&((struct RUNTIME_9060 __iomem *)(cinfo->ctl_addr))->
			mail_box_0);

	while (cyz_fetch_msg(cinfo, &channel, &cmd, &param) == 1) {
		special_count = 0;
		delta_count = 0;
		info = &cinfo->ports[channel];
		tty = info->port.tty;
		if (tty == NULL)
			continue;

		ch_ctrl = &(zfw_ctrl->ch_ctrl[channel]);
		buf_ctrl = &(zfw_ctrl->buf_ctrl[channel]);

		switch (cmd) {
		case C_CM_PR_ERROR:
			tty_insert_flip_char(tty, 0, TTY_PARITY);
			info->icount.rx++;
			special_count++;
			break;
		case C_CM_FR_ERROR:
			tty_insert_flip_char(tty, 0, TTY_FRAME);
			info->icount.rx++;
			special_count++;
			break;
		case C_CM_RXBRK:
			tty_insert_flip_char(tty, 0, TTY_BREAK);
			info->icount.rx++;
			special_count++;
			break;
		case C_CM_MDCD:
			info->icount.dcd++;
			delta_count++;
			if (info->port.flags & ASYNC_CHECK_CD) {
				if ((fw_ver > 241 ? ((u_long) param) :
						readl(&ch_ctrl->rs_status)) &
						C_RS_DCD) {
					wake_up_interruptible(&info->port.open_wait);
				} else {
					tty_hangup(info->port.tty);
					wake_up_interruptible(&info->port.open_wait);
					info->port.flags &= ~ASYNC_NORMAL_ACTIVE;
				}
			}
			break;
		case C_CM_MCTS:
			info->icount.cts++;
			delta_count++;
			break;
		case C_CM_MRI:
			info->icount.rng++;
			delta_count++;
			break;
		case C_CM_MDSR:
			info->icount.dsr++;
			delta_count++;
			break;
#ifdef Z_WAKE
		case C_CM_IOCTLW:
			complete(&info->shutdown_wait);
			break;
#endif
#ifdef CONFIG_CYZ_INTR
		case C_CM_RXHIWM:
		case C_CM_RXNNDT:
		case C_CM_INTBACK2:
			/* Reception Interrupt */
#ifdef CY_DEBUG_INTERRUPTS
			printk(KERN_DEBUG "cyz_interrupt: rcvd intr, card %d, "
					"port %ld\n", info->card, channel);
#endif
			cyz_handle_rx(info, buf_ctrl);
			break;
		case C_CM_TXBEMPTY:
		case C_CM_TXLOWWM:
		case C_CM_INTBACK:
			/* Transmission Interrupt */
#ifdef CY_DEBUG_INTERRUPTS
			printk(KERN_DEBUG "cyz_interrupt: xmit intr, card %d, "
					"port %ld\n", info->card, channel);
#endif
			cyz_handle_tx(info, buf_ctrl);
			break;
#endif				/* CONFIG_CYZ_INTR */
		case C_CM_FATAL:
			/* should do something with this !!! */
			break;
		default:
			break;
		}
		if (delta_count)
			wake_up_interruptible(&info->delta_msr_wait);
		if (special_count)
			tty_schedule_flip(tty);
	}
}

#ifdef CONFIG_CYZ_INTR
static irqreturn_t cyz_interrupt(int irq, void *dev_id)
{
	struct cyclades_card *cinfo = dev_id;

	if (unlikely(cinfo == NULL)) {
#ifdef CY_DEBUG_INTERRUPTS
		printk(KERN_DEBUG "cyz_interrupt: spurious interrupt %d\n",
									irq);
#endif
		return IRQ_NONE;	/* spurious interrupt */
	}

	if (unlikely(!ISZLOADED(*cinfo))) {
#ifdef CY_DEBUG_INTERRUPTS
		printk(KERN_DEBUG "cyz_interrupt: board not yet loaded "
				"(IRQ%d).\n", irq);
#endif
		return IRQ_NONE;
	}

	/* Handle the interrupts */
	cyz_handle_cmd(cinfo);

	return IRQ_HANDLED;
}				/* cyz_interrupt */

static void cyz_rx_restart(unsigned long arg)
{
	struct cyclades_port *info = (struct cyclades_port *)arg;
	struct cyclades_card *card = info->card;
	int retval;
	__u32 channel = info->line - card->first_line;
	unsigned long flags;

	spin_lock_irqsave(&card->card_lock, flags);
	retval = cyz_issue_cmd(card, channel, C_CM_INTBACK2, 0L);
	if (retval != 0) {
		printk(KERN_ERR "cyc:cyz_rx_restart retval on ttyC%d was %x\n",
			info->line, retval);
	}
	spin_unlock_irqrestore(&card->card_lock, flags);
}

#else				/* CONFIG_CYZ_INTR */

static void cyz_poll(unsigned long arg)
{
	struct cyclades_card *cinfo;
	struct cyclades_port *info;
	struct tty_struct *tty;
	struct FIRM_ID __iomem *firm_id;
	struct ZFW_CTRL __iomem *zfw_ctrl;
	struct BOARD_CTRL __iomem *board_ctrl;
	struct BUF_CTRL __iomem *buf_ctrl;
	unsigned long expires = jiffies + HZ;
	unsigned int port, card;

	for (card = 0; card < NR_CARDS; card++) {
		cinfo = &cy_card[card];

		if (!IS_CYC_Z(*cinfo))
			continue;
		if (!ISZLOADED(*cinfo))
			continue;

		firm_id = cinfo->base_addr + ID_ADDRESS;
		zfw_ctrl = cinfo->base_addr +
				(readl(&firm_id->zfwctrl_addr) & 0xfffff);
		board_ctrl = &(zfw_ctrl->board_ctrl);

	/* Skip first polling cycle to avoid racing conditions with the FW */
		if (!cinfo->intr_enabled) {
			cinfo->nports = (int)readl(&board_ctrl->n_channel);
			cinfo->intr_enabled = 1;
			continue;
		}

		cyz_handle_cmd(cinfo);

		for (port = 0; port < cinfo->nports; port++) {
			info = &cinfo->ports[port];
			tty = info->port.tty;
			buf_ctrl = &(zfw_ctrl->buf_ctrl[port]);

			if (!info->throttle)
				cyz_handle_rx(info, buf_ctrl);
			cyz_handle_tx(info, buf_ctrl);
		}
		/* poll every 'cyz_polling_cycle' period */
		expires = jiffies + cyz_polling_cycle;
	}
	mod_timer(&cyz_timerlist, expires);
}				/* cyz_poll */

#endif				/* CONFIG_CYZ_INTR */

/********** End of block of Cyclades-Z specific code *********/
/***********************************************************/

static int startup(struct cyclades_port *info)
{
	struct cyclades_card *card;
	unsigned long flags;
	int retval = 0;
	void __iomem *base_addr;
	int chip, channel, index;
	unsigned long page;

	card = info->card;
	channel = info->line - card->first_line;

	page = get_zeroed_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

	spin_lock_irqsave(&card->card_lock, flags);

	if (info->port.flags & ASYNC_INITIALIZED) {
		free_page(page);
		goto errout;
	}

	if (!info->type) {
		if (info->port.tty)
			set_bit(TTY_IO_ERROR, &info->port.tty->flags);
		free_page(page);
		goto errout;
	}

	if (info->port.xmit_buf)
		free_page(page);
	else
		info->port.xmit_buf = (unsigned char *)page;

	spin_unlock_irqrestore(&card->card_lock, flags);

	set_line_char(info);

	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

#ifdef CY_DEBUG_OPEN
		printk(KERN_DEBUG "cyc startup card %d, chip %d, channel %d, "
				"base_addr %p\n",
				card, chip, channel, base_addr);
#endif
		spin_lock_irqsave(&card->card_lock, flags);

		cy_writeb(base_addr + (CyCAR << index), (u_char) channel);

		cy_writeb(base_addr + (CyRTPR << index),
			(info->default_timeout ? info->default_timeout : 0x02));
		/* 10ms rx timeout */

		cyy_issue_cmd(base_addr, CyCHAN_CTL | CyENB_RCVR | CyENB_XMTR,
				index);

		cy_writeb(base_addr + (CyCAR << index), (u_char) channel);
		cy_writeb(base_addr + (CyMSVR1 << index), CyRTS);
		cy_writeb(base_addr + (CyMSVR2 << index), CyDTR);

#ifdef CY_DEBUG_DTR
		printk(KERN_DEBUG "cyc:startup raising DTR\n");
		printk(KERN_DEBUG "     status: 0x%x, 0x%x\n",
			readb(base_addr + (CyMSVR1 << index)),
			readb(base_addr + (CyMSVR2 << index)));
#endif

		cy_writeb(base_addr + (CySRER << index),
			readb(base_addr + (CySRER << index)) | CyRxData);
		info->port.flags |= ASYNC_INITIALIZED;

		if (info->port.tty)
			clear_bit(TTY_IO_ERROR, &info->port.tty->flags);
		info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
		info->breakon = info->breakoff = 0;
		memset((char *)&info->idle_stats, 0, sizeof(info->idle_stats));
		info->idle_stats.in_use =
		info->idle_stats.recv_idle =
		info->idle_stats.xmit_idle = jiffies;

		spin_unlock_irqrestore(&card->card_lock, flags);

	} else {
		struct FIRM_ID __iomem *firm_id;
		struct ZFW_CTRL __iomem *zfw_ctrl;
		struct BOARD_CTRL __iomem *board_ctrl;
		struct CH_CTRL __iomem *ch_ctrl;

		base_addr = card->base_addr;

		firm_id = base_addr + ID_ADDRESS;
		if (!ISZLOADED(*card))
			return -ENODEV;

		zfw_ctrl = card->base_addr +
				(readl(&firm_id->zfwctrl_addr) & 0xfffff);
		board_ctrl = &zfw_ctrl->board_ctrl;
		ch_ctrl = zfw_ctrl->ch_ctrl;

#ifdef CY_DEBUG_OPEN
		printk(KERN_DEBUG "cyc startup Z card %d, channel %d, "
			"base_addr %p\n", card, channel, base_addr);
#endif
		spin_lock_irqsave(&card->card_lock, flags);

		cy_writel(&ch_ctrl[channel].op_mode, C_CH_ENABLE);
#ifdef Z_WAKE
#ifdef CONFIG_CYZ_INTR
		cy_writel(&ch_ctrl[channel].intr_enable,
			  C_IN_TXBEMPTY | C_IN_TXLOWWM | C_IN_RXHIWM |
			  C_IN_RXNNDT | C_IN_IOCTLW | C_IN_MDCD);
#else
		cy_writel(&ch_ctrl[channel].intr_enable,
			  C_IN_IOCTLW | C_IN_MDCD);
#endif				/* CONFIG_CYZ_INTR */
#else
#ifdef CONFIG_CYZ_INTR
		cy_writel(&ch_ctrl[channel].intr_enable,
			  C_IN_TXBEMPTY | C_IN_TXLOWWM | C_IN_RXHIWM |
			  C_IN_RXNNDT | C_IN_MDCD);
#else
		cy_writel(&ch_ctrl[channel].intr_enable, C_IN_MDCD);
#endif				/* CONFIG_CYZ_INTR */
#endif				/* Z_WAKE */

		retval = cyz_issue_cmd(card, channel, C_CM_IOCTL, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc:startup(1) retval on ttyC%d was "
				"%x\n", info->line, retval);
		}

		/* Flush RX buffers before raising DTR and RTS */
		retval = cyz_issue_cmd(card, channel, C_CM_FLUSH_RX, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc:startup(2) retval on ttyC%d was "
				"%x\n", info->line, retval);
		}

		/* set timeout !!! */
		/* set RTS and DTR !!! */
		cy_writel(&ch_ctrl[channel].rs_control,
			readl(&ch_ctrl[channel].rs_control) | C_RS_RTS |
			C_RS_DTR);
		retval = cyz_issue_cmd(card, channel, C_CM_IOCTLM, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc:startup(3) retval on ttyC%d was "
				"%x\n", info->line, retval);
		}
#ifdef CY_DEBUG_DTR
		printk(KERN_DEBUG "cyc:startup raising Z DTR\n");
#endif

		/* enable send, recv, modem !!! */

		info->port.flags |= ASYNC_INITIALIZED;
		if (info->port.tty)
			clear_bit(TTY_IO_ERROR, &info->port.tty->flags);
		info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
		info->breakon = info->breakoff = 0;
		memset((char *)&info->idle_stats, 0, sizeof(info->idle_stats));
		info->idle_stats.in_use =
		info->idle_stats.recv_idle =
		info->idle_stats.xmit_idle = jiffies;

		spin_unlock_irqrestore(&card->card_lock, flags);
	}

#ifdef CY_DEBUG_OPEN
	printk(KERN_DEBUG "cyc startup done\n");
#endif
	return 0;

errout:
	spin_unlock_irqrestore(&card->card_lock, flags);
	return retval;
}				/* startup */

static void start_xmit(struct cyclades_port *info)
{
	struct cyclades_card *card;
	unsigned long flags;
	void __iomem *base_addr;
	int chip, channel, index;

	card = info->card;
	channel = info->line - card->first_line;
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		spin_lock_irqsave(&card->card_lock, flags);
		cy_writeb(base_addr + (CyCAR << index), channel);
		cy_writeb(base_addr + (CySRER << index),
			readb(base_addr + (CySRER << index)) | CyTxRdy);
		spin_unlock_irqrestore(&card->card_lock, flags);
	} else {
#ifdef CONFIG_CYZ_INTR
		int retval;

		spin_lock_irqsave(&card->card_lock, flags);
		retval = cyz_issue_cmd(card, channel, C_CM_INTBACK, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc:start_xmit retval on ttyC%d was "
				"%x\n", info->line, retval);
		}
		spin_unlock_irqrestore(&card->card_lock, flags);
#else				/* CONFIG_CYZ_INTR */
		/* Don't have to do anything at this time */
#endif				/* CONFIG_CYZ_INTR */
	}
}				/* start_xmit */

static void shutdown(struct cyclades_port *info)
{
	struct cyclades_card *card;
	unsigned long flags;
	void __iomem *base_addr;
	int chip, channel, index;

	if (!(info->port.flags & ASYNC_INITIALIZED))
		return;

	card = info->card;
	channel = info->line - card->first_line;
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

#ifdef CY_DEBUG_OPEN
		printk(KERN_DEBUG "cyc shutdown Y card %d, chip %d, "
				"channel %d, base_addr %p\n",
				card, chip, channel, base_addr);
#endif

		spin_lock_irqsave(&card->card_lock, flags);

		/* Clear delta_msr_wait queue to avoid mem leaks. */
		wake_up_interruptible(&info->delta_msr_wait);

		if (info->port.xmit_buf) {
			unsigned char *temp;
			temp = info->port.xmit_buf;
			info->port.xmit_buf = NULL;
			free_page((unsigned long)temp);
		}
		cy_writeb(base_addr + (CyCAR << index), (u_char) channel);
		if (!info->port.tty || (info->port.tty->termios->c_cflag & HUPCL)) {
			cy_writeb(base_addr + (CyMSVR1 << index), ~CyRTS);
			cy_writeb(base_addr + (CyMSVR2 << index), ~CyDTR);
#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc shutdown dropping DTR\n");
			printk(KERN_DEBUG "     status: 0x%x, 0x%x\n",
				readb(base_addr + (CyMSVR1 << index)),
				readb(base_addr + (CyMSVR2 << index)));
#endif
		}
		cyy_issue_cmd(base_addr, CyCHAN_CTL | CyDIS_RCVR, index);
		/* it may be appropriate to clear _XMIT at
		   some later date (after testing)!!! */

		if (info->port.tty)
			set_bit(TTY_IO_ERROR, &info->port.tty->flags);
		info->port.flags &= ~ASYNC_INITIALIZED;
		spin_unlock_irqrestore(&card->card_lock, flags);
	} else {
		struct FIRM_ID __iomem *firm_id;
		struct ZFW_CTRL __iomem *zfw_ctrl;
		struct BOARD_CTRL __iomem *board_ctrl;
		struct CH_CTRL __iomem *ch_ctrl;
		int retval;

		base_addr = card->base_addr;
#ifdef CY_DEBUG_OPEN
		printk(KERN_DEBUG "cyc shutdown Z card %d, channel %d, "
			"base_addr %p\n", card, channel, base_addr);
#endif

		firm_id = base_addr + ID_ADDRESS;
		if (!ISZLOADED(*card))
			return;

		zfw_ctrl = card->base_addr +
				(readl(&firm_id->zfwctrl_addr) & 0xfffff);
		board_ctrl = &zfw_ctrl->board_ctrl;
		ch_ctrl = zfw_ctrl->ch_ctrl;

		spin_lock_irqsave(&card->card_lock, flags);

		if (info->port.xmit_buf) {
			unsigned char *temp;
			temp = info->port.xmit_buf;
			info->port.xmit_buf = NULL;
			free_page((unsigned long)temp);
		}

		if (!info->port.tty || (info->port.tty->termios->c_cflag & HUPCL)) {
			cy_writel(&ch_ctrl[channel].rs_control,
				(__u32)(readl(&ch_ctrl[channel].rs_control) &
					~(C_RS_RTS | C_RS_DTR)));
			retval = cyz_issue_cmd(info->card, channel,
					C_CM_IOCTLM, 0L);
			if (retval != 0) {
				printk(KERN_ERR"cyc:shutdown retval on ttyC%d "
					"was %x\n", info->line, retval);
			}
#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc:shutdown dropping Z DTR\n");
#endif
		}

		if (info->port.tty)
			set_bit(TTY_IO_ERROR, &info->port.tty->flags);
		info->port.flags &= ~ASYNC_INITIALIZED;

		spin_unlock_irqrestore(&card->card_lock, flags);
	}

#ifdef CY_DEBUG_OPEN
	printk(KERN_DEBUG "cyc shutdown done\n");
#endif
}				/* shutdown */


static int
block_til_ready(struct tty_struct *tty, struct file *filp,
		struct cyclades_port *info)
{
	DECLARE_WAITQUEUE(wait, current);
	struct cyclades_card *cinfo;
	unsigned long flags;
	int chip, channel, index;
	int retval;
	void __iomem *base_addr;

	cinfo = info->card;
	channel = info->line - cinfo->first_line;

	/*
	 * If the device is in the middle of being closed, then block
	 * until it's done, and then try again.
	 */
	if (tty_hung_up_p(filp) || (info->port.flags & ASYNC_CLOSING)) {
		wait_event_interruptible(info->port.close_wait,
				!(info->port.flags & ASYNC_CLOSING));
		return (info->port.flags & ASYNC_HUP_NOTIFY) ? -EAGAIN: -ERESTARTSYS;
	}

	/*
	 * If non-blocking mode is set, then make the check up front
	 * and then exit.
	 */
	if ((filp->f_flags & O_NONBLOCK) ||
					(tty->flags & (1 << TTY_IO_ERROR))) {
		info->port.flags |= ASYNC_NORMAL_ACTIVE;
		return 0;
	}

	/*
	 * Block waiting for the carrier detect and the line to become
	 * free (i.e., not in use by the callout).  While we are in
	 * this loop, info->port.count is dropped by one, so that
	 * cy_close() knows when to free things.  We restore it upon
	 * exit, either normal or abnormal.
	 */
	retval = 0;
	add_wait_queue(&info->port.open_wait, &wait);
#ifdef CY_DEBUG_OPEN
	printk(KERN_DEBUG "cyc block_til_ready before block: ttyC%d, "
		"count = %d\n", info->line, info->port.count);
#endif
	spin_lock_irqsave(&cinfo->card_lock, flags);
	if (!tty_hung_up_p(filp))
		info->port.count--;
	spin_unlock_irqrestore(&cinfo->card_lock, flags);
#ifdef CY_DEBUG_COUNT
	printk(KERN_DEBUG "cyc block_til_ready: (%d): decrementing count to "
		"%d\n", current->pid, info->port.count);
#endif
	info->port.blocked_open++;

	if (!IS_CYC_Z(*cinfo)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = cinfo->bus_index;
		base_addr = cinfo->base_addr + (cy_chip_offset[chip] << index);

		while (1) {
			spin_lock_irqsave(&cinfo->card_lock, flags);
			if ((tty->termios->c_cflag & CBAUD)) {
				cy_writeb(base_addr + (CyCAR << index),
					  (u_char) channel);
				cy_writeb(base_addr + (CyMSVR1 << index),
					  CyRTS);
				cy_writeb(base_addr + (CyMSVR2 << index),
					  CyDTR);
#ifdef CY_DEBUG_DTR
				printk(KERN_DEBUG "cyc:block_til_ready raising "
					"DTR\n");
				printk(KERN_DEBUG "     status: 0x%x, 0x%x\n",
					readb(base_addr + (CyMSVR1 << index)),
					readb(base_addr + (CyMSVR2 << index)));
#endif
			}
			spin_unlock_irqrestore(&cinfo->card_lock, flags);

			set_current_state(TASK_INTERRUPTIBLE);
			if (tty_hung_up_p(filp) ||
					!(info->port.flags & ASYNC_INITIALIZED)) {
				retval = ((info->port.flags & ASYNC_HUP_NOTIFY) ?
					  -EAGAIN : -ERESTARTSYS);
				break;
			}

			spin_lock_irqsave(&cinfo->card_lock, flags);
			cy_writeb(base_addr + (CyCAR << index),
				  (u_char) channel);
			if (!(info->port.flags & ASYNC_CLOSING) && (C_CLOCAL(tty) ||
					(readb(base_addr +
						(CyMSVR1 << index)) & CyDCD))) {
				spin_unlock_irqrestore(&cinfo->card_lock, flags);
				break;
			}
			spin_unlock_irqrestore(&cinfo->card_lock, flags);

			if (signal_pending(current)) {
				retval = -ERESTARTSYS;
				break;
			}
#ifdef CY_DEBUG_OPEN
			printk(KERN_DEBUG "cyc block_til_ready blocking: "
				"ttyC%d, count = %d\n",
				info->line, info->port.count);
#endif
			schedule();
		}
	} else {
		struct FIRM_ID __iomem *firm_id;
		struct ZFW_CTRL __iomem *zfw_ctrl;
		struct BOARD_CTRL __iomem *board_ctrl;
		struct CH_CTRL __iomem *ch_ctrl;

		base_addr = cinfo->base_addr;
		firm_id = base_addr + ID_ADDRESS;
		if (!ISZLOADED(*cinfo)) {
			__set_current_state(TASK_RUNNING);
			remove_wait_queue(&info->port.open_wait, &wait);
			return -EINVAL;
		}

		zfw_ctrl = base_addr + (readl(&firm_id->zfwctrl_addr)
								& 0xfffff);
		board_ctrl = &zfw_ctrl->board_ctrl;
		ch_ctrl = zfw_ctrl->ch_ctrl;

		while (1) {
			if ((tty->termios->c_cflag & CBAUD)) {
				cy_writel(&ch_ctrl[channel].rs_control,
					readl(&ch_ctrl[channel].rs_control) |
					C_RS_RTS | C_RS_DTR);
				retval = cyz_issue_cmd(cinfo,
					channel, C_CM_IOCTLM, 0L);
				if (retval != 0) {
					printk(KERN_ERR "cyc:block_til_ready "
						"retval on ttyC%d was %x\n",
						info->line, retval);
				}
#ifdef CY_DEBUG_DTR
				printk(KERN_DEBUG "cyc:block_til_ready raising "
					"Z DTR\n");
#endif
			}

			set_current_state(TASK_INTERRUPTIBLE);
			if (tty_hung_up_p(filp) ||
					!(info->port.flags & ASYNC_INITIALIZED)) {
				retval = ((info->port.flags & ASYNC_HUP_NOTIFY) ?
					  -EAGAIN : -ERESTARTSYS);
				break;
			}
			if (!(info->port.flags & ASYNC_CLOSING) && (C_CLOCAL(tty) ||
					(readl(&ch_ctrl[channel].rs_status) &
						C_RS_DCD))) {
				break;
			}
			if (signal_pending(current)) {
				retval = -ERESTARTSYS;
				break;
			}
#ifdef CY_DEBUG_OPEN
			printk(KERN_DEBUG "cyc block_til_ready blocking: "
				"ttyC%d, count = %d\n",
				info->line, info->port.count);
#endif
			schedule();
		}
	}
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&info->port.open_wait, &wait);
	if (!tty_hung_up_p(filp)) {
		info->port.count++;
#ifdef CY_DEBUG_COUNT
		printk(KERN_DEBUG "cyc:block_til_ready (%d): incrementing "
			"count to %d\n", current->pid, info->port.count);
#endif
	}
	info->port.blocked_open--;
#ifdef CY_DEBUG_OPEN
	printk(KERN_DEBUG "cyc:block_til_ready after blocking: ttyC%d, "
		"count = %d\n", info->line, info->port.count);
#endif
	if (retval)
		return retval;
	info->port.flags |= ASYNC_NORMAL_ACTIVE;
	return 0;
}				/* block_til_ready */

static int cy_open(struct tty_struct *tty, struct file *filp)
{
	struct cyclades_port *info;
	unsigned int i, line;
	int retval;

	line = tty->index;
	if (tty->index < 0 || NR_PORTS <= line)
		return -ENODEV;

	for (i = 0; i < NR_CARDS; i++)
		if (line < cy_card[i].first_line + cy_card[i].nports &&
				line >= cy_card[i].first_line)
			break;
	if (i >= NR_CARDS)
		return -ENODEV;
	info = &cy_card[i].ports[line - cy_card[i].first_line];
	if (info->line < 0)
		return -ENODEV;

	/* If the card's firmware hasn't been loaded,
	   treat it as absent from the system.  This
	   will make the user pay attention.
	 */
	if (IS_CYC_Z(*info->card)) {
		struct cyclades_card *cinfo = info->card;
		struct FIRM_ID __iomem *firm_id = cinfo->base_addr + ID_ADDRESS;

		if (!ISZLOADED(*cinfo)) {
			if (((ZE_V1 == readl(&((struct RUNTIME_9060 __iomem *)
					 (cinfo->ctl_addr))->mail_box_0)) &&
					Z_FPGA_CHECK(*cinfo)) &&
					(ZFIRM_HLT == readl(
						&firm_id->signature))) {
				printk(KERN_ERR "cyc:Cyclades-Z Error: you "
					"need an external power supply for "
					"this number of ports.\nFirmware "
					"halted.\n");
			} else {
				printk(KERN_ERR "cyc:Cyclades-Z firmware not "
					"yet loaded\n");
			}
			return -ENODEV;
		}
#ifdef CONFIG_CYZ_INTR
		else {
		/* In case this Z board is operating in interrupt mode, its
		   interrupts should be enabled as soon as the first open
		   happens to one of its ports. */
			if (!cinfo->intr_enabled) {
				struct ZFW_CTRL __iomem *zfw_ctrl;
				struct BOARD_CTRL __iomem *board_ctrl;

				zfw_ctrl = cinfo->base_addr +
					(readl(&firm_id->zfwctrl_addr) &
					 0xfffff);

				board_ctrl = &zfw_ctrl->board_ctrl;

				/* Enable interrupts on the PLX chip */
				cy_writew(cinfo->ctl_addr + 0x68,
					readw(cinfo->ctl_addr + 0x68) | 0x0900);
				/* Enable interrupts on the FW */
				retval = cyz_issue_cmd(cinfo, 0,
						C_CM_IRQ_ENBL, 0L);
				if (retval != 0) {
					printk(KERN_ERR "cyc:IRQ enable retval "
						"was %x\n", retval);
				}
				cinfo->nports =
					(int)readl(&board_ctrl->n_channel);
				cinfo->intr_enabled = 1;
			}
		}
#endif				/* CONFIG_CYZ_INTR */
		/* Make sure this Z port really exists in hardware */
		if (info->line > (cinfo->first_line + cinfo->nports - 1))
			return -ENODEV;
	}
#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_open ttyC%d\n", info->line);
#endif
	tty->driver_data = info;
	info->port.tty = tty;
	if (serial_paranoia_check(info, tty->name, "cy_open"))
		return -ENODEV;

#ifdef CY_DEBUG_OPEN
	printk(KERN_DEBUG "cyc:cy_open ttyC%d, count = %d\n", info->line,
			info->port.count);
#endif
	info->port.count++;
#ifdef CY_DEBUG_COUNT
	printk(KERN_DEBUG "cyc:cy_open (%d): incrementing count to %d\n",
		current->pid, info->port.count);
#endif

	/*
	 * If the port is the middle of closing, bail out now
	 */
	if (tty_hung_up_p(filp) || (info->port.flags & ASYNC_CLOSING)) {
		wait_event_interruptible(info->port.close_wait,
				!(info->port.flags & ASYNC_CLOSING));
		return (info->port.flags & ASYNC_HUP_NOTIFY) ? -EAGAIN: -ERESTARTSYS;
	}

	/*
	 * Start up serial port
	 */
	retval = startup(info);
	if (retval)
		return retval;

	retval = block_til_ready(tty, filp, info);
	if (retval) {
#ifdef CY_DEBUG_OPEN
		printk(KERN_DEBUG "cyc:cy_open returning after block_til_ready "
			"with %d\n", retval);
#endif
		return retval;
	}

	info->throttle = 0;

#ifdef CY_DEBUG_OPEN
	printk(KERN_DEBUG "cyc:cy_open done\n");
#endif
	return 0;
}				/* cy_open */

static void cy_wait_until_sent(struct tty_struct *tty, int timeout)
{
	struct cyclades_card *card;
	struct cyclades_port *info = tty->driver_data;
	void __iomem *base_addr;
	int chip, channel, index;
	unsigned long orig_jiffies;
	int char_time;

	if (serial_paranoia_check(info, tty->name, "cy_wait_until_sent"))
		return;

	if (info->xmit_fifo_size == 0)
		return;		/* Just in case.... */

	orig_jiffies = jiffies;
	lock_kernel();
	/*
	 * Set the check interval to be 1/5 of the estimated time to
	 * send a single character, and make it at least 1.  The check
	 * interval should also be less than the timeout.
	 *
	 * Note: we have to use pretty tight timings here to satisfy
	 * the NIST-PCTS.
	 */
	char_time = (info->timeout - HZ / 50) / info->xmit_fifo_size;
	char_time = char_time / 5;
	if (char_time <= 0)
		char_time = 1;
	if (timeout < 0)
		timeout = 0;
	if (timeout)
		char_time = min(char_time, timeout);
	/*
	 * If the transmitter hasn't cleared in twice the approximate
	 * amount of time to send the entire FIFO, it probably won't
	 * ever clear.  This assumes the UART isn't doing flow
	 * control, which is currently the case.  Hence, if it ever
	 * takes longer than info->timeout, this is probably due to a
	 * UART bug of some kind.  So, we clamp the timeout parameter at
	 * 2*info->timeout.
	 */
	if (!timeout || timeout > 2 * info->timeout)
		timeout = 2 * info->timeout;
#ifdef CY_DEBUG_WAIT_UNTIL_SENT
	printk(KERN_DEBUG "In cy_wait_until_sent(%d) check=%d, jiff=%lu...",
		timeout, char_time, jiffies);
#endif
	card = info->card;
	channel = (info->line) - (card->first_line);
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);
		while (readb(base_addr + (CySRER << index)) & CyTxRdy) {
#ifdef CY_DEBUG_WAIT_UNTIL_SENT
			printk(KERN_DEBUG "Not clean (jiff=%lu)...", jiffies);
#endif
			if (msleep_interruptible(jiffies_to_msecs(char_time)))
				break;
			if (timeout && time_after(jiffies, orig_jiffies +
					timeout))
				break;
		}
	}
	/* Run one more char cycle */
	msleep_interruptible(jiffies_to_msecs(char_time * 5));
	unlock_kernel();
#ifdef CY_DEBUG_WAIT_UNTIL_SENT
	printk(KERN_DEBUG "Clean (jiff=%lu)...done\n", jiffies);
#endif
}

static void cy_flush_buffer(struct tty_struct *tty)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	int channel, retval;
	unsigned long flags;

#ifdef CY_DEBUG_IO
	printk(KERN_DEBUG "cyc:cy_flush_buffer ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_flush_buffer"))
		return;

	card = info->card;
	channel = info->line - card->first_line;

	spin_lock_irqsave(&card->card_lock, flags);
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
	spin_unlock_irqrestore(&card->card_lock, flags);

	if (IS_CYC_Z(*card)) {	/* If it is a Z card, flush the on-board
					   buffers as well */
		spin_lock_irqsave(&card->card_lock, flags);
		retval = cyz_issue_cmd(card, channel, C_CM_FLUSH_TX, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc: flush_buffer retval on ttyC%d "
				"was %x\n", info->line, retval);
		}
		spin_unlock_irqrestore(&card->card_lock, flags);
	}
	tty_wakeup(tty);
}				/* cy_flush_buffer */


static void cy_close(struct tty_struct *tty, struct file *filp)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	unsigned long flags;

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_close ttyC%d\n", info->line);
#endif

	if (!info || serial_paranoia_check(info, tty->name, "cy_close"))
		return;

	card = info->card;

	spin_lock_irqsave(&card->card_lock, flags);
	/* If the TTY is being hung up, nothing to do */
	if (tty_hung_up_p(filp)) {
		spin_unlock_irqrestore(&card->card_lock, flags);
		return;
	}
#ifdef CY_DEBUG_OPEN
	printk(KERN_DEBUG "cyc:cy_close ttyC%d, count = %d\n", info->line,
		info->port.count);
#endif
	if ((tty->count == 1) && (info->port.count != 1)) {
		/*
		 * Uh, oh.  tty->count is 1, which means that the tty
		 * structure will be freed.  Info->count should always
		 * be one in these conditions.  If it's greater than
		 * one, we've got real problems, since it means the
		 * serial port won't be shutdown.
		 */
		printk(KERN_ERR "cyc:cy_close: bad serial port count; "
			"tty->count is 1, info->port.count is %d\n", info->port.count);
		info->port.count = 1;
	}
#ifdef CY_DEBUG_COUNT
	printk(KERN_DEBUG  "cyc:cy_close at (%d): decrementing count to %d\n",
		current->pid, info->port.count - 1);
#endif
	if (--info->port.count < 0) {
#ifdef CY_DEBUG_COUNT
		printk(KERN_DEBUG "cyc:cyc_close setting count to 0\n");
#endif
		info->port.count = 0;
	}
	if (info->port.count) {
		spin_unlock_irqrestore(&card->card_lock, flags);
		return;
	}
	info->port.flags |= ASYNC_CLOSING;

	/*
	 * Now we wait for the transmit buffer to clear; and we notify
	 * the line discipline to only process XON/XOFF characters.
	 */
	tty->closing = 1;
	spin_unlock_irqrestore(&card->card_lock, flags);
	if (info->port.closing_wait != CY_CLOSING_WAIT_NONE)
		tty_wait_until_sent(tty, info->port.closing_wait);

	spin_lock_irqsave(&card->card_lock, flags);

	if (!IS_CYC_Z(*card)) {
		int channel = info->line - card->first_line;
		int index = card->bus_index;
		void __iomem *base_addr = card->base_addr +
			(cy_chip_offset[channel >> 2] << index);
		/* Stop accepting input */
		channel &= 0x03;
		cy_writeb(base_addr + (CyCAR << index), (u_char) channel);
		cy_writeb(base_addr + (CySRER << index),
			  readb(base_addr + (CySRER << index)) & ~CyRxData);
		if (info->port.flags & ASYNC_INITIALIZED) {
			/* Waiting for on-board buffers to be empty before
			   closing the port */
			spin_unlock_irqrestore(&card->card_lock, flags);
			cy_wait_until_sent(tty, info->timeout);
			spin_lock_irqsave(&card->card_lock, flags);
		}
	} else {
#ifdef Z_WAKE
		/* Waiting for on-board buffers to be empty before closing
		   the port */
		void __iomem *base_addr = card->base_addr;
		struct FIRM_ID __iomem *firm_id = base_addr + ID_ADDRESS;
		struct ZFW_CTRL __iomem *zfw_ctrl =
		    base_addr + (readl(&firm_id->zfwctrl_addr) & 0xfffff);
		struct CH_CTRL __iomem *ch_ctrl = zfw_ctrl->ch_ctrl;
		int channel = info->line - card->first_line;
		int retval;

		if (readl(&ch_ctrl[channel].flow_status) != C_FS_TXIDLE) {
			retval = cyz_issue_cmd(card, channel, C_CM_IOCTLW, 0L);
			if (retval != 0) {
				printk(KERN_DEBUG "cyc:cy_close retval on "
					"ttyC%d was %x\n", info->line, retval);
			}
			spin_unlock_irqrestore(&card->card_lock, flags);
			wait_for_completion_interruptible(&info->shutdown_wait);
			spin_lock_irqsave(&card->card_lock, flags);
		}
#endif
	}

	spin_unlock_irqrestore(&card->card_lock, flags);
	shutdown(info);
	cy_flush_buffer(tty);
	tty_ldisc_flush(tty);
	spin_lock_irqsave(&card->card_lock, flags);

	tty->closing = 0;
	info->port.tty = NULL;
	if (info->port.blocked_open) {
		spin_unlock_irqrestore(&card->card_lock, flags);
		if (info->port.close_delay) {
			msleep_interruptible(jiffies_to_msecs
						(info->port.close_delay));
		}
		wake_up_interruptible(&info->port.open_wait);
		spin_lock_irqsave(&card->card_lock, flags);
	}
	info->port.flags &= ~(ASYNC_NORMAL_ACTIVE | ASYNC_CLOSING);
	wake_up_interruptible(&info->port.close_wait);

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_close done\n");
#endif

	spin_unlock_irqrestore(&card->card_lock, flags);
}				/* cy_close */

static int cy_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
	struct cyclades_port *info = tty->driver_data;
	unsigned long flags;
	int c, ret = 0;

#ifdef CY_DEBUG_IO
	printk(KERN_DEBUG "cyc:cy_write ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_write"))
		return 0;

	if (!info->port.xmit_buf)
		return 0;

	spin_lock_irqsave(&info->card->card_lock, flags);
	while (1) {
		c = min(count, (int)(SERIAL_XMIT_SIZE - info->xmit_cnt - 1));
		c = min(c, (int)(SERIAL_XMIT_SIZE - info->xmit_head));

		if (c <= 0)
			break;

		memcpy(info->port.xmit_buf + info->xmit_head, buf, c);
		info->xmit_head = (info->xmit_head + c) &
			(SERIAL_XMIT_SIZE - 1);
		info->xmit_cnt += c;
		buf += c;
		count -= c;
		ret += c;
	}
	spin_unlock_irqrestore(&info->card->card_lock, flags);

	info->idle_stats.xmit_bytes += ret;
	info->idle_stats.xmit_idle = jiffies;

	if (info->xmit_cnt && !tty->stopped && !tty->hw_stopped)
		start_xmit(info);

	return ret;
}				/* cy_write */

static int cy_put_char(struct tty_struct *tty, unsigned char ch)
{
	struct cyclades_port *info = tty->driver_data;
	unsigned long flags;

#ifdef CY_DEBUG_IO
	printk(KERN_DEBUG "cyc:cy_put_char ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_put_char"))
		return 0;

	if (!info->port.xmit_buf)
		return 0;

	spin_lock_irqsave(&info->card->card_lock, flags);
	if (info->xmit_cnt >= (int)(SERIAL_XMIT_SIZE - 1)) {
		spin_unlock_irqrestore(&info->card->card_lock, flags);
		return 0;
	}

	info->port.xmit_buf[info->xmit_head++] = ch;
	info->xmit_head &= SERIAL_XMIT_SIZE - 1;
	info->xmit_cnt++;
	info->idle_stats.xmit_bytes++;
	info->idle_stats.xmit_idle = jiffies;
	spin_unlock_irqrestore(&info->card->card_lock, flags);
	return 1;
}				/* cy_put_char */

static void cy_flush_chars(struct tty_struct *tty)
{
	struct cyclades_port *info = tty->driver_data;

#ifdef CY_DEBUG_IO
	printk(KERN_DEBUG "cyc:cy_flush_chars ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_flush_chars"))
		return;

	if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
			!info->port.xmit_buf)
		return;

	start_xmit(info);
}				/* cy_flush_chars */

static int cy_write_room(struct tty_struct *tty)
{
	struct cyclades_port *info = tty->driver_data;
	int ret;

#ifdef CY_DEBUG_IO
	printk(KERN_DEBUG "cyc:cy_write_room ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_write_room"))
		return 0;
	ret = SERIAL_XMIT_SIZE - info->xmit_cnt - 1;
	if (ret < 0)
		ret = 0;
	return ret;
}				/* cy_write_room */

static int cy_chars_in_buffer(struct tty_struct *tty)
{
	struct cyclades_card *card;
	struct cyclades_port *info = tty->driver_data;
	int channel;

	if (serial_paranoia_check(info, tty->name, "cy_chars_in_buffer"))
		return 0;

	card = info->card;
	channel = (info->line) - (card->first_line);

#ifdef Z_EXT_CHARS_IN_BUFFER
	if (!IS_CYC_Z(cy_card[card])) {
#endif				/* Z_EXT_CHARS_IN_BUFFER */
#ifdef CY_DEBUG_IO
		printk(KERN_DEBUG "cyc:cy_chars_in_buffer ttyC%d %d\n",
			info->line, info->xmit_cnt);
#endif
		return info->xmit_cnt;
#ifdef Z_EXT_CHARS_IN_BUFFER
	} else {
		static struct FIRM_ID *firm_id;
		static struct ZFW_CTRL *zfw_ctrl;
		static struct CH_CTRL *ch_ctrl;
		static struct BUF_CTRL *buf_ctrl;
		int char_count;
		__u32 tx_put, tx_get, tx_bufsize;

		lock_kernel();
		firm_id = card->base_addr + ID_ADDRESS;
		zfw_ctrl = card->base_addr +
			(readl(&firm_id->zfwctrl_addr) & 0xfffff);
		ch_ctrl = &(zfw_ctrl->ch_ctrl[channel]);
		buf_ctrl = &(zfw_ctrl->buf_ctrl[channel]);

		tx_get = readl(&buf_ctrl->tx_get);
		tx_put = readl(&buf_ctrl->tx_put);
		tx_bufsize = readl(&buf_ctrl->tx_bufsize);
		if (tx_put >= tx_get)
			char_count = tx_put - tx_get;
		else
			char_count = tx_put - tx_get + tx_bufsize;
#ifdef CY_DEBUG_IO
		printk(KERN_DEBUG "cyc:cy_chars_in_buffer ttyC%d %d\n",
			info->line, info->xmit_cnt + char_count);
#endif
		unlock_kernel();
		return info->xmit_cnt + char_count;
	}
#endif				/* Z_EXT_CHARS_IN_BUFFER */
}				/* cy_chars_in_buffer */


static void cyy_baud_calc(struct cyclades_port *info, __u32 baud)
{
	int co, co_val, bpr;
	__u32 cy_clock = ((info->chip_rev >= CD1400_REV_J) ? 60000000 :
			25000000);

	if (baud == 0) {
		info->tbpr = info->tco = info->rbpr = info->rco = 0;
		return;
	}

	/* determine which prescaler to use */
	for (co = 4, co_val = 2048; co; co--, co_val >>= 2) {
		if (cy_clock / co_val / baud > 63)
			break;
	}

	bpr = (cy_clock / co_val * 2 / baud + 1) / 2;
	if (bpr > 255)
		bpr = 255;

	info->tbpr = info->rbpr = bpr;
	info->tco = info->rco = co;
}

static void set_line_char(struct cyclades_port *info)
{
	struct cyclades_card *card;
	unsigned long flags;
	void __iomem *base_addr;
	int chip, channel, index;
	unsigned cflag, iflag;
	unsigned short chip_number;
	int baud, baud_rate = 0;
	int i;

	if (!info->port.tty || !info->port.tty->termios)
		return;

	if (info->line == -1)
		return;

	cflag = info->port.tty->termios->c_cflag;
	iflag = info->port.tty->termios->c_iflag;

	/*
	 * Set up the tty->alt_speed kludge
	 */
	if (info->port.tty) {
		if ((info->port.flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI)
			info->port.tty->alt_speed = 57600;
		if ((info->port.flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI)
			info->port.tty->alt_speed = 115200;
		if ((info->port.flags & ASYNC_SPD_MASK) == ASYNC_SPD_SHI)
			info->port.tty->alt_speed = 230400;
		if ((info->port.flags & ASYNC_SPD_MASK) == ASYNC_SPD_WARP)
			info->port.tty->alt_speed = 460800;
	}

	card = info->card;
	channel = info->line - card->first_line;
	chip_number = channel / 4;

	if (!IS_CYC_Z(*card)) {

		index = card->bus_index;

		/* baud rate */
		baud = tty_get_baud_rate(info->port.tty);
		if (baud == 38400 && (info->port.flags & ASYNC_SPD_MASK) ==
				ASYNC_SPD_CUST) {
			if (info->custom_divisor)
				baud_rate = info->baud / info->custom_divisor;
			else
				baud_rate = info->baud;
		} else if (baud > CD1400_MAX_SPEED) {
			baud = CD1400_MAX_SPEED;
		}
		/* find the baud index */
		for (i = 0; i < 20; i++) {
			if (baud == baud_table[i])
				break;
		}
		if (i == 20)
			i = 19;	/* CD1400_MAX_SPEED */

		if (baud == 38400 && (info->port.flags & ASYNC_SPD_MASK) ==
				ASYNC_SPD_CUST) {
			cyy_baud_calc(info, baud_rate);
		} else {
			if (info->chip_rev >= CD1400_REV_J) {
				/* It is a CD1400 rev. J or later */
				info->tbpr = baud_bpr_60[i];	/* Tx BPR */
				info->tco = baud_co_60[i];	/* Tx CO */
				info->rbpr = baud_bpr_60[i];	/* Rx BPR */
				info->rco = baud_co_60[i];	/* Rx CO */
			} else {
				info->tbpr = baud_bpr_25[i];	/* Tx BPR */
				info->tco = baud_co_25[i];	/* Tx CO */
				info->rbpr = baud_bpr_25[i];	/* Rx BPR */
				info->rco = baud_co_25[i];	/* Rx CO */
			}
		}
		if (baud_table[i] == 134) {
			/* get it right for 134.5 baud */
			info->timeout = (info->xmit_fifo_size * HZ * 30 / 269) +
					2;
		} else if (baud == 38400 && (info->port.flags & ASYNC_SPD_MASK) ==
				ASYNC_SPD_CUST) {
			info->timeout = (info->xmit_fifo_size * HZ * 15 /
					baud_rate) + 2;
		} else if (baud_table[i]) {
			info->timeout = (info->xmit_fifo_size * HZ * 15 /
					baud_table[i]) + 2;
			/* this needs to be propagated into the card info */
		} else {
			info->timeout = 0;
		}
		/* By tradition (is it a standard?) a baud rate of zero
		   implies the line should be/has been closed.  A bit
		   later in this routine such a test is performed. */

		/* byte size and parity */
		info->cor5 = 0;
		info->cor4 = 0;
		/* receive threshold */
		info->cor3 = (info->default_threshold ?
				info->default_threshold : baud_cor3[i]);
		info->cor2 = CyETC;
		switch (cflag & CSIZE) {
		case CS5:
			info->cor1 = Cy_5_BITS;
			break;
		case CS6:
			info->cor1 = Cy_6_BITS;
			break;
		case CS7:
			info->cor1 = Cy_7_BITS;
			break;
		case CS8:
			info->cor1 = Cy_8_BITS;
			break;
		}
		if (cflag & CSTOPB)
			info->cor1 |= Cy_2_STOP;

		if (cflag & PARENB) {
			if (cflag & PARODD)
				info->cor1 |= CyPARITY_O;
			else
				info->cor1 |= CyPARITY_E;
		} else
			info->cor1 |= CyPARITY_NONE;

		/* CTS flow control flag */
		if (cflag & CRTSCTS) {
			info->port.flags |= ASYNC_CTS_FLOW;
			info->cor2 |= CyCtsAE;
		} else {
			info->port.flags &= ~ASYNC_CTS_FLOW;
			info->cor2 &= ~CyCtsAE;
		}
		if (cflag & CLOCAL)
			info->port.flags &= ~ASYNC_CHECK_CD;
		else
			info->port.flags |= ASYNC_CHECK_CD;

	 /***********************************************
	    The hardware option, CyRtsAO, presents RTS when
	    the chip has characters to send.  Since most modems
	    use RTS as reverse (inbound) flow control, this
	    option is not used.  If inbound flow control is
	    necessary, DTR can be programmed to provide the
	    appropriate signals for use with a non-standard
	    cable.  Contact Marcio Saito for details.
	 ***********************************************/

		chip = channel >> 2;
		channel &= 0x03;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		spin_lock_irqsave(&card->card_lock, flags);
		cy_writeb(base_addr + (CyCAR << index), (u_char) channel);

		/* tx and rx baud rate */

		cy_writeb(base_addr + (CyTCOR << index), info->tco);
		cy_writeb(base_addr + (CyTBPR << index), info->tbpr);
		cy_writeb(base_addr + (CyRCOR << index), info->rco);
		cy_writeb(base_addr + (CyRBPR << index), info->rbpr);

		/* set line characteristics  according configuration */

		cy_writeb(base_addr + (CySCHR1 << index),
			  START_CHAR(info->port.tty));
		cy_writeb(base_addr + (CySCHR2 << index), STOP_CHAR(info->port.tty));
		cy_writeb(base_addr + (CyCOR1 << index), info->cor1);
		cy_writeb(base_addr + (CyCOR2 << index), info->cor2);
		cy_writeb(base_addr + (CyCOR3 << index), info->cor3);
		cy_writeb(base_addr + (CyCOR4 << index), info->cor4);
		cy_writeb(base_addr + (CyCOR5 << index), info->cor5);

		cyy_issue_cmd(base_addr, CyCOR_CHANGE | CyCOR1ch | CyCOR2ch |
				CyCOR3ch, index);

		/* !!! Is this needed? */
		cy_writeb(base_addr + (CyCAR << index), (u_char) channel);
		cy_writeb(base_addr + (CyRTPR << index),
			(info->default_timeout ? info->default_timeout : 0x02));
		/* 10ms rx timeout */

		if (C_CLOCAL(info->port.tty)) {
			/* without modem intr */
			cy_writeb(base_addr + (CySRER << index),
				readb(base_addr + (CySRER << index)) | CyMdmCh);
			/* act on 1->0 modem transitions */
			if ((cflag & CRTSCTS) && info->rflow) {
				cy_writeb(base_addr + (CyMCOR1 << index),
					  (CyCTS | rflow_thr[i]));
			} else {
				cy_writeb(base_addr + (CyMCOR1 << index),
					  CyCTS);
			}
			/* act on 0->1 modem transitions */
			cy_writeb(base_addr + (CyMCOR2 << index), CyCTS);
		} else {
			/* without modem intr */
			cy_writeb(base_addr + (CySRER << index),
				  readb(base_addr +
					   (CySRER << index)) | CyMdmCh);
			/* act on 1->0 modem transitions */
			if ((cflag & CRTSCTS) && info->rflow) {
				cy_writeb(base_addr + (CyMCOR1 << index),
					  (CyDSR | CyCTS | CyRI | CyDCD |
					   rflow_thr[i]));
			} else {
				cy_writeb(base_addr + (CyMCOR1 << index),
					  CyDSR | CyCTS | CyRI | CyDCD);
			}
			/* act on 0->1 modem transitions */
			cy_writeb(base_addr + (CyMCOR2 << index),
				  CyDSR | CyCTS | CyRI | CyDCD);
		}

		if (i == 0) {	/* baud rate is zero, turn off line */
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  ~CyRTS);
			} else {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  ~CyDTR);
			}
#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc:set_line_char dropping DTR\n");
			printk(KERN_DEBUG "     status: 0x%x, 0x%x\n",
				readb(base_addr + (CyMSVR1 << index)),
				readb(base_addr + (CyMSVR2 << index)));
#endif
		} else {
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  CyRTS);
			} else {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  CyDTR);
			}
#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc:set_line_char raising DTR\n");
			printk(KERN_DEBUG "     status: 0x%x, 0x%x\n",
				readb(base_addr + (CyMSVR1 << index)),
				readb(base_addr + (CyMSVR2 << index)));
#endif
		}

		if (info->port.tty)
			clear_bit(TTY_IO_ERROR, &info->port.tty->flags);
		spin_unlock_irqrestore(&card->card_lock, flags);

	} else {
		struct FIRM_ID __iomem *firm_id;
		struct ZFW_CTRL __iomem *zfw_ctrl;
		struct BOARD_CTRL __iomem *board_ctrl;
		struct CH_CTRL __iomem *ch_ctrl;
		struct BUF_CTRL __iomem *buf_ctrl;
		__u32 sw_flow;
		int retval;

		firm_id = card->base_addr + ID_ADDRESS;
		if (!ISZLOADED(*card))
			return;

		zfw_ctrl = card->base_addr +
			(readl(&firm_id->zfwctrl_addr) & 0xfffff);
		board_ctrl = &zfw_ctrl->board_ctrl;
		ch_ctrl = &(zfw_ctrl->ch_ctrl[channel]);
		buf_ctrl = &zfw_ctrl->buf_ctrl[channel];

		/* baud rate */
		baud = tty_get_baud_rate(info->port.tty);
		if (baud == 38400 && (info->port.flags & ASYNC_SPD_MASK) ==
				ASYNC_SPD_CUST) {
			if (info->custom_divisor)
				baud_rate = info->baud / info->custom_divisor;
			else
				baud_rate = info->baud;
		} else if (baud > CYZ_MAX_SPEED) {
			baud = CYZ_MAX_SPEED;
		}
		cy_writel(&ch_ctrl->comm_baud, baud);

		if (baud == 134) {
			/* get it right for 134.5 baud */
			info->timeout = (info->xmit_fifo_size * HZ * 30 / 269) +
					2;
		} else if (baud == 38400 && (info->port.flags & ASYNC_SPD_MASK) ==
				ASYNC_SPD_CUST) {
			info->timeout = (info->xmit_fifo_size * HZ * 15 /
					baud_rate) + 2;
		} else if (baud) {
			info->timeout = (info->xmit_fifo_size * HZ * 15 /
					baud) + 2;
			/* this needs to be propagated into the card info */
		} else {
			info->timeout = 0;
		}

		/* byte size and parity */
		switch (cflag & CSIZE) {
		case CS5:
			cy_writel(&ch_ctrl->comm_data_l, C_DL_CS5);
			break;
		case CS6:
			cy_writel(&ch_ctrl->comm_data_l, C_DL_CS6);
			break;
		case CS7:
			cy_writel(&ch_ctrl->comm_data_l, C_DL_CS7);
			break;
		case CS8:
			cy_writel(&ch_ctrl->comm_data_l, C_DL_CS8);
			break;
		}
		if (cflag & CSTOPB) {
			cy_writel(&ch_ctrl->comm_data_l,
				  readl(&ch_ctrl->comm_data_l) | C_DL_2STOP);
		} else {
			cy_writel(&ch_ctrl->comm_data_l,
				  readl(&ch_ctrl->comm_data_l) | C_DL_1STOP);
		}
		if (cflag & PARENB) {
			if (cflag & PARODD)
				cy_writel(&ch_ctrl->comm_parity, C_PR_ODD);
			else
				cy_writel(&ch_ctrl->comm_parity, C_PR_EVEN);
		} else
			cy_writel(&ch_ctrl->comm_parity, C_PR_NONE);

		/* CTS flow control flag */
		if (cflag & CRTSCTS) {
			cy_writel(&ch_ctrl->hw_flow,
				readl(&ch_ctrl->hw_flow) | C_RS_CTS | C_RS_RTS);
		} else {
			cy_writel(&ch_ctrl->hw_flow, readl(&ch_ctrl->hw_flow) &
					~(C_RS_CTS | C_RS_RTS));
		}
		/* As the HW flow control is done in firmware, the driver
		   doesn't need to care about it */
		info->port.flags &= ~ASYNC_CTS_FLOW;

		/* XON/XOFF/XANY flow control flags */
		sw_flow = 0;
		if (iflag & IXON) {
			sw_flow |= C_FL_OXX;
			if (iflag & IXANY)
				sw_flow |= C_FL_OIXANY;
		}
		cy_writel(&ch_ctrl->sw_flow, sw_flow);

		retval = cyz_issue_cmd(card, channel, C_CM_IOCTL, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc:set_line_char retval on ttyC%d "
				"was %x\n", info->line, retval);
		}

		/* CD sensitivity */
		if (cflag & CLOCAL)
			info->port.flags &= ~ASYNC_CHECK_CD;
		else
			info->port.flags |= ASYNC_CHECK_CD;

		if (baud == 0) {	/* baud rate is zero, turn off line */
			cy_writel(&ch_ctrl->rs_control,
				  readl(&ch_ctrl->rs_control) & ~C_RS_DTR);
#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc:set_line_char dropping Z DTR\n");
#endif
		} else {
			cy_writel(&ch_ctrl->rs_control,
				  readl(&ch_ctrl->rs_control) | C_RS_DTR);
#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc:set_line_char raising Z DTR\n");
#endif
		}

		retval = cyz_issue_cmd(card, channel, C_CM_IOCTLM, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc:set_line_char(2) retval on ttyC%d "
				"was %x\n", info->line, retval);
		}

		if (info->port.tty)
			clear_bit(TTY_IO_ERROR, &info->port.tty->flags);
	}
}				/* set_line_char */

static int
get_serial_info(struct cyclades_port *info,
		struct serial_struct __user *retinfo)
{
	struct serial_struct tmp;
	struct cyclades_card *cinfo = info->card;

	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = info->type;
	tmp.line = info->line;
	tmp.port = (info->card - cy_card) * 0x100 + info->line -
		cinfo->first_line;
	tmp.irq = cinfo->irq;
	tmp.flags = info->port.flags;
	tmp.close_delay = info->port.close_delay;
	tmp.closing_wait = info->port.closing_wait;
	tmp.baud_base = info->baud;
	tmp.custom_divisor = info->custom_divisor;
	tmp.hub6 = 0;		/*!!! */
	return copy_to_user(retinfo, &tmp, sizeof(*retinfo)) ? -EFAULT : 0;
}				/* get_serial_info */

static int
set_serial_info(struct cyclades_port *info,
		struct serial_struct __user *new_info)
{
	struct serial_struct new_serial;
	struct cyclades_port old_info;

	if (copy_from_user(&new_serial, new_info, sizeof(new_serial)))
		return -EFAULT;
	old_info = *info;

	if (!capable(CAP_SYS_ADMIN)) {
		if (new_serial.close_delay != info->port.close_delay ||
				new_serial.baud_base != info->baud ||
				(new_serial.flags & ASYNC_FLAGS &
					~ASYNC_USR_MASK) !=
				(info->port.flags & ASYNC_FLAGS & ~ASYNC_USR_MASK))
			return -EPERM;
		info->port.flags = (info->port.flags & ~ASYNC_USR_MASK) |
				(new_serial.flags & ASYNC_USR_MASK);
		info->baud = new_serial.baud_base;
		info->custom_divisor = new_serial.custom_divisor;
		goto check_and_exit;
	}

	/*
	 * OK, past this point, all the error checking has been done.
	 * At this point, we start making changes.....
	 */

	info->baud = new_serial.baud_base;
	info->custom_divisor = new_serial.custom_divisor;
	info->port.flags = (info->port.flags & ~ASYNC_FLAGS) |
			(new_serial.flags & ASYNC_FLAGS);
	info->port.close_delay = new_serial.close_delay * HZ / 100;
	info->port.closing_wait = new_serial.closing_wait * HZ / 100;

check_and_exit:
	if (info->port.flags & ASYNC_INITIALIZED) {
		set_line_char(info);
		return 0;
	} else {
		return startup(info);
	}
}				/* set_serial_info */

static int get_lsr_info(struct cyclades_port *info, unsigned int __user *value)
{
	struct cyclades_card *card;
	int chip, channel, index;
	unsigned char status;
	unsigned int result;
	unsigned long flags;
	void __iomem *base_addr;

	card = info->card;
	channel = (info->line) - (card->first_line);
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		spin_lock_irqsave(&card->card_lock, flags);
		status = readb(base_addr + (CySRER << index)) &
				(CyTxRdy | CyTxMpty);
		spin_unlock_irqrestore(&card->card_lock, flags);
		result = (status ? 0 : TIOCSER_TEMT);
	} else {
		/* Not supported yet */
		return -EINVAL;
	}
	return put_user(result, (unsigned long __user *)value);
}

static int cy_tiocmget(struct tty_struct *tty, struct file *file)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	int chip, channel, index;
	void __iomem *base_addr;
	unsigned long flags;
	unsigned char status;
	unsigned long lstatus;
	unsigned int result;
	struct FIRM_ID __iomem *firm_id;
	struct ZFW_CTRL __iomem *zfw_ctrl;
	struct BOARD_CTRL __iomem *board_ctrl;
	struct CH_CTRL __iomem *ch_ctrl;

	if (serial_paranoia_check(info, tty->name, __func__))
		return -ENODEV;

	lock_kernel();

	card = info->card;
	channel = info->line - card->first_line;
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		spin_lock_irqsave(&card->card_lock, flags);
		cy_writeb(base_addr + (CyCAR << index), (u_char) channel);
		status = readb(base_addr + (CyMSVR1 << index));
		status |= readb(base_addr + (CyMSVR2 << index));
		spin_unlock_irqrestore(&card->card_lock, flags);

		if (info->rtsdtr_inv) {
			result = ((status & CyRTS) ? TIOCM_DTR : 0) |
				((status & CyDTR) ? TIOCM_RTS : 0);
		} else {
			result = ((status & CyRTS) ? TIOCM_RTS : 0) |
				((status & CyDTR) ? TIOCM_DTR : 0);
		}
		result |= ((status & CyDCD) ? TIOCM_CAR : 0) |
			((status & CyRI) ? TIOCM_RNG : 0) |
			((status & CyDSR) ? TIOCM_DSR : 0) |
			((status & CyCTS) ? TIOCM_CTS : 0);
	} else {
		base_addr = card->base_addr;
		firm_id = card->base_addr + ID_ADDRESS;
		if (ISZLOADED(*card)) {
			zfw_ctrl = card->base_addr +
				(readl(&firm_id->zfwctrl_addr) & 0xfffff);
			board_ctrl = &zfw_ctrl->board_ctrl;
			ch_ctrl = zfw_ctrl->ch_ctrl;
			lstatus = readl(&ch_ctrl[channel].rs_status);
			result = ((lstatus & C_RS_RTS) ? TIOCM_RTS : 0) |
				((lstatus & C_RS_DTR) ? TIOCM_DTR : 0) |
				((lstatus & C_RS_DCD) ? TIOCM_CAR : 0) |
				((lstatus & C_RS_RI) ? TIOCM_RNG : 0) |
				((lstatus & C_RS_DSR) ? TIOCM_DSR : 0) |
				((lstatus & C_RS_CTS) ? TIOCM_CTS : 0);
		} else {
			result = 0;
			unlock_kernel();
			return -ENODEV;
		}

	}
	unlock_kernel();
	return result;
}				/* cy_tiomget */

static int
cy_tiocmset(struct tty_struct *tty, struct file *file,
		unsigned int set, unsigned int clear)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	int chip, channel, index;
	void __iomem *base_addr;
	unsigned long flags;
	struct FIRM_ID __iomem *firm_id;
	struct ZFW_CTRL __iomem *zfw_ctrl;
	struct BOARD_CTRL __iomem *board_ctrl;
	struct CH_CTRL __iomem *ch_ctrl;
	int retval;

	if (serial_paranoia_check(info, tty->name, __func__))
		return -ENODEV;

	card = info->card;
	channel = (info->line) - (card->first_line);
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		if (set & TIOCM_RTS) {
			spin_lock_irqsave(&card->card_lock, flags);
			cy_writeb(base_addr + (CyCAR << index),
				  (u_char) channel);
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  CyDTR);
			} else {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  CyRTS);
			}
			spin_unlock_irqrestore(&card->card_lock, flags);
		}
		if (clear & TIOCM_RTS) {
			spin_lock_irqsave(&card->card_lock, flags);
			cy_writeb(base_addr + (CyCAR << index),
				  (u_char) channel);
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  ~CyDTR);
			} else {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  ~CyRTS);
			}
			spin_unlock_irqrestore(&card->card_lock, flags);
		}
		if (set & TIOCM_DTR) {
			spin_lock_irqsave(&card->card_lock, flags);
			cy_writeb(base_addr + (CyCAR << index),
				  (u_char) channel);
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  CyRTS);
			} else {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  CyDTR);
			}
#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc:set_modem_info raising DTR\n");
			printk(KERN_DEBUG "     status: 0x%x, 0x%x\n",
				readb(base_addr + (CyMSVR1 << index)),
				readb(base_addr + (CyMSVR2 << index)));
#endif
			spin_unlock_irqrestore(&card->card_lock, flags);
		}
		if (clear & TIOCM_DTR) {
			spin_lock_irqsave(&card->card_lock, flags);
			cy_writeb(base_addr + (CyCAR << index),
				  (u_char) channel);
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  ~CyRTS);
			} else {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  ~CyDTR);
			}

#ifdef CY_DEBUG_DTR
			printk(KERN_DEBUG "cyc:set_modem_info dropping DTR\n");
			printk(KERN_DEBUG "     status: 0x%x, 0x%x\n",
				readb(base_addr + (CyMSVR1 << index)),
				readb(base_addr + (CyMSVR2 << index)));
#endif
			spin_unlock_irqrestore(&card->card_lock, flags);
		}
	} else {
		base_addr = card->base_addr;

		firm_id = card->base_addr + ID_ADDRESS;
		if (ISZLOADED(*card)) {
			zfw_ctrl = card->base_addr +
				(readl(&firm_id->zfwctrl_addr) & 0xfffff);
			board_ctrl = &zfw_ctrl->board_ctrl;
			ch_ctrl = zfw_ctrl->ch_ctrl;

			if (set & TIOCM_RTS) {
				spin_lock_irqsave(&card->card_lock, flags);
				cy_writel(&ch_ctrl[channel].rs_control,
					readl(&ch_ctrl[channel].rs_control) |
					C_RS_RTS);
				spin_unlock_irqrestore(&card->card_lock, flags);
			}
			if (clear & TIOCM_RTS) {
				spin_lock_irqsave(&card->card_lock, flags);
				cy_writel(&ch_ctrl[channel].rs_control,
					readl(&ch_ctrl[channel].rs_control) &
					~C_RS_RTS);
				spin_unlock_irqrestore(&card->card_lock, flags);
			}
			if (set & TIOCM_DTR) {
				spin_lock_irqsave(&card->card_lock, flags);
				cy_writel(&ch_ctrl[channel].rs_control,
					readl(&ch_ctrl[channel].rs_control) |
					C_RS_DTR);
#ifdef CY_DEBUG_DTR
				printk(KERN_DEBUG "cyc:set_modem_info raising "
					"Z DTR\n");
#endif
				spin_unlock_irqrestore(&card->card_lock, flags);
			}
			if (clear & TIOCM_DTR) {
				spin_lock_irqsave(&card->card_lock, flags);
				cy_writel(&ch_ctrl[channel].rs_control,
					readl(&ch_ctrl[channel].rs_control) &
					~C_RS_DTR);
#ifdef CY_DEBUG_DTR
				printk(KERN_DEBUG "cyc:set_modem_info clearing "
					"Z DTR\n");
#endif
				spin_unlock_irqrestore(&card->card_lock, flags);
			}
		} else {
			return -ENODEV;
		}
		spin_lock_irqsave(&card->card_lock, flags);
		retval = cyz_issue_cmd(card, channel, C_CM_IOCTLM, 0L);
		if (retval != 0) {
			printk(KERN_ERR "cyc:set_modem_info retval on ttyC%d "
				"was %x\n", info->line, retval);
		}
		spin_unlock_irqrestore(&card->card_lock, flags);
	}
	return 0;
}				/* cy_tiocmset */

static int cy_break(struct tty_struct *tty, int break_state)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	unsigned long flags;
	int retval = 0;

	if (serial_paranoia_check(info, tty->name, "cy_break"))
		return -EINVAL;

	card = info->card;

	spin_lock_irqsave(&card->card_lock, flags);
	if (!IS_CYC_Z(*card)) {
		/* Let the transmit ISR take care of this (since it
		   requires stuffing characters into the output stream).
		 */
		if (break_state == -1) {
			if (!info->breakon) {
				info->breakon = 1;
				if (!info->xmit_cnt) {
					spin_unlock_irqrestore(&card->card_lock, flags);
					start_xmit(info);
					spin_lock_irqsave(&card->card_lock, flags);
				}
			}
		} else {
			if (!info->breakoff) {
				info->breakoff = 1;
				if (!info->xmit_cnt) {
					spin_unlock_irqrestore(&card->card_lock, flags);
					start_xmit(info);
					spin_lock_irqsave(&card->card_lock, flags);
				}
			}
		}
	} else {
		if (break_state == -1) {
			retval = cyz_issue_cmd(card,
				info->line - card->first_line,
				C_CM_SET_BREAK, 0L);
			if (retval != 0) {
				printk(KERN_ERR "cyc:cy_break (set) retval on "
					"ttyC%d was %x\n", info->line, retval);
			}
		} else {
			retval = cyz_issue_cmd(card,
				info->line - card->first_line,
				C_CM_CLR_BREAK, 0L);
			if (retval != 0) {
				printk(KERN_DEBUG "cyc:cy_break (clr) retval "
					"on ttyC%d was %x\n", info->line,
					retval);
			}
		}
	}
	spin_unlock_irqrestore(&card->card_lock, flags);
	return retval;
}				/* cy_break */

static int get_mon_info(struct cyclades_port *info,
				struct cyclades_monitor __user *mon)
{

	if (copy_to_user(mon, &info->mon, sizeof(struct cyclades_monitor)))
		return -EFAULT;
	info->mon.int_count = 0;
	info->mon.char_count = 0;
	info->mon.char_max = 0;
	info->mon.char_last = 0;
	return 0;
}				/* get_mon_info */

static int set_threshold(struct cyclades_port *info, unsigned long value)
{
	struct cyclades_card *card;
	void __iomem *base_addr;
	int channel, chip, index;
	unsigned long flags;

	card = info->card;
	channel = info->line - card->first_line;
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr =
		    card->base_addr + (cy_chip_offset[chip] << index);

		info->cor3 &= ~CyREC_FIFO;
		info->cor3 |= value & CyREC_FIFO;

		spin_lock_irqsave(&card->card_lock, flags);
		cy_writeb(base_addr + (CyCOR3 << index), info->cor3);
		cyy_issue_cmd(base_addr, CyCOR_CHANGE | CyCOR3ch, index);
		spin_unlock_irqrestore(&card->card_lock, flags);
	}
	return 0;
}				/* set_threshold */

static int get_threshold(struct cyclades_port *info,
						unsigned long __user *value)
{
	struct cyclades_card *card;
	void __iomem *base_addr;
	int channel, chip, index;
	unsigned long tmp;

	card = info->card;
	channel = info->line - card->first_line;
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		tmp = readb(base_addr + (CyCOR3 << index)) & CyREC_FIFO;
		return put_user(tmp, value);
	}
	return 0;
}				/* get_threshold */

static int set_default_threshold(struct cyclades_port *info,
							unsigned long value)
{
	info->default_threshold = value & 0x0f;
	return 0;
}				/* set_default_threshold */

static int get_default_threshold(struct cyclades_port *info,
						unsigned long __user *value)
{
	return put_user(info->default_threshold, value);
}				/* get_default_threshold */

static int set_timeout(struct cyclades_port *info, unsigned long value)
{
	struct cyclades_card *card;
	void __iomem *base_addr;
	int channel, chip, index;
	unsigned long flags;

	card = info->card;
	channel = info->line - card->first_line;
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		spin_lock_irqsave(&card->card_lock, flags);
		cy_writeb(base_addr + (CyRTPR << index), value & 0xff);
		spin_unlock_irqrestore(&card->card_lock, flags);
	}
	return 0;
}				/* set_timeout */

static int get_timeout(struct cyclades_port *info,
						unsigned long __user *value)
{
	struct cyclades_card *card;
	void __iomem *base_addr;
	int channel, chip, index;
	unsigned long tmp;

	card = info->card;
	channel = info->line - card->first_line;
	if (!IS_CYC_Z(*card)) {
		chip = channel >> 2;
		channel &= 0x03;
		index = card->bus_index;
		base_addr = card->base_addr + (cy_chip_offset[chip] << index);

		tmp = readb(base_addr + (CyRTPR << index));
		return put_user(tmp, value);
	}
	return 0;
}				/* get_timeout */

static int set_default_timeout(struct cyclades_port *info, unsigned long value)
{
	info->default_timeout = value & 0xff;
	return 0;
}				/* set_default_timeout */

static int get_default_timeout(struct cyclades_port *info,
					unsigned long __user *value)
{
	return put_user(info->default_timeout, value);
}				/* get_default_timeout */

static int
cy_ioctl(struct tty_struct *tty, struct file *file,
	 unsigned int cmd, unsigned long arg)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_icount cprev, cnow;	/* kernel counter temps */
	struct serial_icounter_struct __user *p_cuser;	/* user space */
	int ret_val = 0;
	unsigned long flags;
	void __user *argp = (void __user *)arg;

	if (serial_paranoia_check(info, tty->name, "cy_ioctl"))
		return -ENODEV;

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_ioctl ttyC%d, cmd = %x arg = %lx\n",
		info->line, cmd, arg);
#endif
	lock_kernel();

	switch (cmd) {
	case CYGETMON:
		ret_val = get_mon_info(info, argp);
		break;
	case CYGETTHRESH:
		ret_val = get_threshold(info, argp);
		break;
	case CYSETTHRESH:
		ret_val = set_threshold(info, arg);
		break;
	case CYGETDEFTHRESH:
		ret_val = get_default_threshold(info, argp);
		break;
	case CYSETDEFTHRESH:
		ret_val = set_default_threshold(info, arg);
		break;
	case CYGETTIMEOUT:
		ret_val = get_timeout(info, argp);
		break;
	case CYSETTIMEOUT:
		ret_val = set_timeout(info, arg);
		break;
	case CYGETDEFTIMEOUT:
		ret_val = get_default_timeout(info, argp);
		break;
	case CYSETDEFTIMEOUT:
		ret_val = set_default_timeout(info, arg);
		break;
	case CYSETRFLOW:
		info->rflow = (int)arg;
		ret_val = 0;
		break;
	case CYGETRFLOW:
		ret_val = info->rflow;
		break;
	case CYSETRTSDTR_INV:
		info->rtsdtr_inv = (int)arg;
		ret_val = 0;
		break;
	case CYGETRTSDTR_INV:
		ret_val = info->rtsdtr_inv;
		break;
	case CYGETCD1400VER:
		ret_val = info->chip_rev;
		break;
#ifndef CONFIG_CYZ_INTR
	case CYZSETPOLLCYCLE:
		cyz_polling_cycle = (arg * HZ) / 1000;
		ret_val = 0;
		break;
	case CYZGETPOLLCYCLE:
		ret_val = (cyz_polling_cycle * 1000) / HZ;
		break;
#endif				/* CONFIG_CYZ_INTR */
	case CYSETWAIT:
		info->port.closing_wait = (unsigned short)arg * HZ / 100;
		ret_val = 0;
		break;
	case CYGETWAIT:
		ret_val = info->port.closing_wait / (HZ / 100);
		break;
	case TIOCGSERIAL:
		ret_val = get_serial_info(info, argp);
		break;
	case TIOCSSERIAL:
		ret_val = set_serial_info(info, argp);
		break;
	case TIOCSERGETLSR:	/* Get line status register */
		ret_val = get_lsr_info(info, argp);
		break;
		/*
		 * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
		 * - mask passed in arg for lines of interest
		 *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
		 * Caller should use TIOCGICOUNT to see which one it was
		 */
	case TIOCMIWAIT:
		spin_lock_irqsave(&info->card->card_lock, flags);
		/* note the counters on entry */
		cnow = info->icount;
		spin_unlock_irqrestore(&info->card->card_lock, flags);
		ret_val = wait_event_interruptible(info->delta_msr_wait, ({
			cprev = cnow;
			spin_lock_irqsave(&info->card->card_lock, flags);
			cnow = info->icount;	/* atomic copy */
			spin_unlock_irqrestore(&info->card->card_lock, flags);

			((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
			((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
			((arg & TIOCM_CD)  && (cnow.dcd != cprev.dcd)) ||
			((arg & TIOCM_CTS) && (cnow.cts != cprev.cts));
		}));
		break;

		/*
		 * Get counter of input serial line interrupts (DCD,RI,DSR,CTS)
		 * Return: write counters to the user passed counter struct
		 * NB: both 1->0 and 0->1 transitions are counted except for
		 *     RI where only 0->1 is counted.
		 */
	case TIOCGICOUNT:
		spin_lock_irqsave(&info->card->card_lock, flags);
		cnow = info->icount;
		spin_unlock_irqrestore(&info->card->card_lock, flags);
		p_cuser = argp;
		ret_val = put_user(cnow.cts, &p_cuser->cts);
		if (ret_val)
			break;
		ret_val = put_user(cnow.dsr, &p_cuser->dsr);
		if (ret_val)
			break;
		ret_val = put_user(cnow.rng, &p_cuser->rng);
		if (ret_val)
			break;
		ret_val = put_user(cnow.dcd, &p_cuser->dcd);
		if (ret_val)
			break;
		ret_val = put_user(cnow.rx, &p_cuser->rx);
		if (ret_val)
			break;
		ret_val = put_user(cnow.tx, &p_cuser->tx);
		if (ret_val)
			break;
		ret_val = put_user(cnow.frame, &p_cuser->frame);
		if (ret_val)
			break;
		ret_val = put_user(cnow.overrun, &p_cuser->overrun);
		if (ret_val)
			break;
		ret_val = put_user(cnow.parity, &p_cuser->parity);
		if (ret_val)
			break;
		ret_val = put_user(cnow.brk, &p_cuser->brk);
		if (ret_val)
			break;
		ret_val = put_user(cnow.buf_overrun, &p_cuser->buf_overrun);
		if (ret_val)
			break;
		ret_val = 0;
		break;
	default:
		ret_val = -ENOIOCTLCMD;
	}
	unlock_kernel();

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_ioctl done\n");
#endif
	return ret_val;
}				/* cy_ioctl */

static void cy_set_termios(struct tty_struct *tty, struct ktermios *old_termios)
{
	struct cyclades_port *info = tty->driver_data;

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_set_termios ttyC%d\n", info->line);
#endif

	set_line_char(info);

	if ((old_termios->c_cflag & CRTSCTS) &&
			!(tty->termios->c_cflag & CRTSCTS)) {
		tty->hw_stopped = 0;
		cy_start(tty);
	}
#if 0
	/*
	 * No need to wake up processes in open wait, since they
	 * sample the CLOCAL flag once, and don't recheck it.
	 * XXX  It's not clear whether the current behavior is correct
	 * or not.  Hence, this may change.....
	 */
	if (!(old_termios->c_cflag & CLOCAL) &&
	    (tty->termios->c_cflag & CLOCAL))
		wake_up_interruptible(&info->port.open_wait);
#endif
}				/* cy_set_termios */

static void cy_send_xchar(struct tty_struct *tty, char ch)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	int channel;

	if (serial_paranoia_check(info, tty->name, "cy_send_xchar"))
		return;

	info->x_char = ch;

	if (ch)
		cy_start(tty);

	card = info->card;
	channel = info->line - card->first_line;

	if (IS_CYC_Z(*card)) {
		if (ch == STOP_CHAR(tty))
			cyz_issue_cmd(card, channel, C_CM_SENDXOFF, 0L);
		else if (ch == START_CHAR(tty))
			cyz_issue_cmd(card, channel, C_CM_SENDXON, 0L);
	}
}

static void cy_throttle(struct tty_struct *tty)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	unsigned long flags;
	void __iomem *base_addr;
	int chip, channel, index;

#ifdef CY_DEBUG_THROTTLE
	char buf[64];

	printk(KERN_DEBUG "cyc:throttle %s: %ld...ttyC%d\n", tty_name(tty, buf),
			tty->ldisc.chars_in_buffer(tty), info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_throttle"))
		return;

	card = info->card;

	if (I_IXOFF(tty)) {
		if (!IS_CYC_Z(*card))
			cy_send_xchar(tty, STOP_CHAR(tty));
		else
			info->throttle = 1;
	}

	if (tty->termios->c_cflag & CRTSCTS) {
		channel = info->line - card->first_line;
		if (!IS_CYC_Z(*card)) {
			chip = channel >> 2;
			channel &= 0x03;
			index = card->bus_index;
			base_addr = card->base_addr +
				(cy_chip_offset[chip] << index);

			spin_lock_irqsave(&card->card_lock, flags);
			cy_writeb(base_addr + (CyCAR << index),
				  (u_char) channel);
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  ~CyDTR);
			} else {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  ~CyRTS);
			}
			spin_unlock_irqrestore(&card->card_lock, flags);
		} else {
			info->throttle = 1;
		}
	}
}				/* cy_throttle */

static void cy_unthrottle(struct tty_struct *tty)
{
	struct cyclades_port *info = tty->driver_data;
	struct cyclades_card *card;
	unsigned long flags;
	void __iomem *base_addr;
	int chip, channel, index;

#ifdef CY_DEBUG_THROTTLE
	char buf[64];

	printk(KERN_DEBUG "cyc:unthrottle %s: %ld...ttyC%d\n",
		tty_name(tty, buf), tty_chars_in_buffer(tty), info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_unthrottle"))
		return;

	if (I_IXOFF(tty)) {
		if (info->x_char)
			info->x_char = 0;
		else
			cy_send_xchar(tty, START_CHAR(tty));
	}

	if (tty->termios->c_cflag & CRTSCTS) {
		card = info->card;
		channel = info->line - card->first_line;
		if (!IS_CYC_Z(*card)) {
			chip = channel >> 2;
			channel &= 0x03;
			index = card->bus_index;
			base_addr = card->base_addr +
				(cy_chip_offset[chip] << index);

			spin_lock_irqsave(&card->card_lock, flags);
			cy_writeb(base_addr + (CyCAR << index),
				  (u_char) channel);
			if (info->rtsdtr_inv) {
				cy_writeb(base_addr + (CyMSVR2 << index),
					  CyDTR);
			} else {
				cy_writeb(base_addr + (CyMSVR1 << index),
					  CyRTS);
			}
			spin_unlock_irqrestore(&card->card_lock, flags);
		} else {
			info->throttle = 0;
		}
	}
}				/* cy_unthrottle */

static void cy_stop(struct tty_struct *tty)
{
	struct cyclades_card *cinfo;
	struct cyclades_port *info = tty->driver_data;
	void __iomem *base_addr;
	int chip, channel, index;
	unsigned long flags;

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_stop ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_stop"))
		return;

	cinfo = info->card;
	channel = info->line - cinfo->first_line;
	if (!IS_CYC_Z(*cinfo)) {
		index = cinfo->bus_index;
		chip = channel >> 2;
		channel &= 0x03;
		base_addr = cinfo->base_addr + (cy_chip_offset[chip] << index);

		spin_lock_irqsave(&cinfo->card_lock, flags);
		cy_writeb(base_addr + (CyCAR << index),
			(u_char)(channel & 0x0003)); /* index channel */
		cy_writeb(base_addr + (CySRER << index),
			  readb(base_addr + (CySRER << index)) & ~CyTxRdy);
		spin_unlock_irqrestore(&cinfo->card_lock, flags);
	}
}				/* cy_stop */

static void cy_start(struct tty_struct *tty)
{
	struct cyclades_card *cinfo;
	struct cyclades_port *info = tty->driver_data;
	void __iomem *base_addr;
	int chip, channel, index;
	unsigned long flags;

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_start ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_start"))
		return;

	cinfo = info->card;
	channel = info->line - cinfo->first_line;
	index = cinfo->bus_index;
	if (!IS_CYC_Z(*cinfo)) {
		chip = channel >> 2;
		channel &= 0x03;
		base_addr = cinfo->base_addr + (cy_chip_offset[chip] << index);

		spin_lock_irqsave(&cinfo->card_lock, flags);
		cy_writeb(base_addr + (CyCAR << index),
			(u_char) (channel & 0x0003));	/* index channel */
		cy_writeb(base_addr + (CySRER << index),
			  readb(base_addr + (CySRER << index)) | CyTxRdy);
		spin_unlock_irqrestore(&cinfo->card_lock, flags);
	}
}				/* cy_start */

static void cy_hangup(struct tty_struct *tty)
{
	struct cyclades_port *info = tty->driver_data;

#ifdef CY_DEBUG_OTHER
	printk(KERN_DEBUG "cyc:cy_hangup ttyC%d\n", info->line);
#endif

	if (serial_paranoia_check(info, tty->name, "cy_hangup"))
		return;

	cy_flush_buffer(tty);
	shutdown(info);
	info->port.count = 0;
#ifdef CY_DEBUG_COUNT
	printk(KERN_DEBUG "cyc:cy_hangup (%d): setting count to 0\n",
		current->pid);
#endif
	info->port.tty = NULL;
	info->port.flags &= ~ASYNC_NORMAL_ACTIVE;
	wake_up_interruptible(&info->port.open_wait);
}				/* cy_hangup */


static int __devinit cy_init_card(struct cyclades_card *cinfo)
{
	struct cyclades_port *info;
	u32 uninitialized_var(mailbox);
	unsigned int nports, port;
	unsigned short chip_number;
	int uninitialized_var(index);

	spin_lock_init(&cinfo->card_lock);

	if (IS_CYC_Z(*cinfo)) {	/* Cyclades-Z */
		mailbox = readl(&((struct RUNTIME_9060 __iomem *)
				     cinfo->ctl_addr)->mail_box_0);
		nports = (mailbox == ZE_V1) ? ZE_V1_NPORTS : 8;
		cinfo->intr_enabled = 0;
		cinfo->nports = 0;	/* Will be correctly set later, after
					   Z FW is loaded */
	} else {
		index = cinfo->bus_index;
		nports = cinfo->nports = CyPORTS_PER_CHIP * cinfo->num_chips;
	}

	cinfo->ports = kzalloc(sizeof(*cinfo->ports) * nports, GFP_KERNEL);
	if (cinfo->ports == NULL) {
		printk(KERN_ERR "Cyclades: cannot allocate ports\n");
		cinfo->nports = 0;
		return -ENOMEM;
	}

	for (port = cinfo->first_line; port < cinfo->first_line + nports;
			port++) {
		info = &cinfo->ports[port - cinfo->first_line];
		tty_port_init(&info->port);
		info->magic = CYCLADES_MAGIC;
		info->card = cinfo;
		info->line = port;

		info->port.closing_wait = CLOSING_WAIT_DELAY;
		info->port.close_delay = 5 * HZ / 10;
		info->port.flags = STD_COM_FLAGS;
		init_completion(&info->shutdown_wait);
		init_waitqueue_head(&info->delta_msr_wait);

		if (IS_CYC_Z(*cinfo)) {
			info->type = PORT_STARTECH;
			if (mailbox == ZO_V1)
				info->xmit_fifo_size = CYZ_FIFO_SIZE;
			else
				info->xmit_fifo_size = 4 * CYZ_FIFO_SIZE;
#ifdef CONFIG_CYZ_INTR
			setup_timer(&cyz_rx_full_timer[port],
				cyz_rx_restart, (unsigned long)info);
#endif
		} else {
			info->type = PORT_CIRRUS;
			info->xmit_fifo_size = CyMAX_CHAR_FIFO;
			info->cor1 = CyPARITY_NONE | Cy_1_STOP | Cy_8_BITS;
			info->cor2 = CyETC;
			info->cor3 = 0x08;	/* _very_ small rcv threshold */

			chip_number = (port - cinfo->first_line) / 4;
			info->chip_rev = readb(cinfo->base_addr +
				      (cy_chip_offset[chip_number] << index) +
				      (CyGFRCR << index));

			if (info->chip_rev >= CD1400_REV_J) {
				/* It is a CD1400 rev. J or later */
				info->tbpr = baud_bpr_60[13];	/* Tx BPR */
				info->tco = baud_co_60[13];	/* Tx CO */
				info->rbpr = baud_bpr_60[13];	/* Rx BPR */
				info->rco = baud_co_60[13];	/* Rx CO */
				info->rtsdtr_inv = 1;
			} else {
				info->tbpr = baud_bpr_25[13];	/* Tx BPR */
				info->tco = baud_co_25[13];	/* Tx CO */
				info->rbpr = baud_bpr_25[13];	/* Rx BPR */
				info->rco = baud_co_25[13];	/* Rx CO */
				info->rtsdtr_inv = 0;
			}
			info->read_status_mask = CyTIMEOUT | CySPECHAR |
				CyBREAK | CyPARITY | CyFRAME | CyOVERRUN;
		}

	}

#ifndef CONFIG_CYZ_INTR
	if (IS_CYC_Z(*cinfo) && !timer_pending(&cyz_timerlist)) {
		mod_timer(&cyz_timerlist, jiffies + 1);
#ifdef CY_PCI_DEBUG
		printk(KERN_DEBUG "Cyclades-Z polling initialized\n");
#endif
	}
#endif
	return 0;
}

static unsigned short __devinit cyy_init_card(void __iomem *true_base_addr,
		int index)
{
	unsigned int chip_number;
	void __iomem *base_addr;

	cy_writeb(true_base_addr + (Cy_HwReset << index), 0);
	/* Cy_HwReset is 0x1400 */
	cy_writeb(true_base_addr + (Cy_ClrIntr << index), 0);
	/* Cy_ClrIntr is 0x1800 */
	udelay(500L);

	for (chip_number = 0; chip_number < CyMAX_CHIPS_PER_CARD;
							chip_number++) {
		base_addr =
		    true_base_addr + (cy_chip_offset[chip_number] << index);
		mdelay(1);
		if (readb(base_addr + (CyCCR << index)) != 0x00) {
			/*************
			printk(" chip #%d at %#6lx is never idle (CCR != 0)\n",
			chip_number, (unsigned long)base_addr);
			*************/
			return chip_number;
		}

		cy_writeb(base_addr + (CyGFRCR << index), 0);
		udelay(10L);

		/* The Cyclom-16Y does not decode address bit 9 and therefore
		   cannot distinguish between references to chip 0 and a non-
		   existent chip 4.  If the preceding clearing of the supposed
		   chip 4 GFRCR register appears at chip 0, there is no chip 4
		   and this must be a Cyclom-16Y, not a Cyclom-32Ye.
		 */
		if (chip_number == 4 && readb(true_base_addr +
				(cy_chip_offset[0] << index) +
				(CyGFRCR << index)) == 0) {
			return chip_number;
		}

		cy_writeb(base_addr + (CyCCR << index), CyCHIP_RESET);
		mdelay(1);

		if (readb(base_addr + (CyGFRCR << index)) == 0x00) {
			/*
			   printk(" chip #%d at %#6lx is not responding ",
			   chip_number, (unsigned long)base_addr);
			   printk("(GFRCR stayed 0)\n",
			 */
			return chip_number;
		}
		if ((0xf0 & (readb(base_addr + (CyGFRCR << index)))) !=
				0x40) {
			/*
			printk(" chip #%d at %#6lx is not valid (GFRCR == "
					"%#2x)\n",
					chip_number, (unsigned long)base_addr,
					base_addr[CyGFRCR<<index]);
			 */
			return chip_number;
		}
		cy_writeb(base_addr + (CyGCR << index), CyCH0_SERIAL);
		if (readb(base_addr + (CyGFRCR << index)) >= CD1400_REV_J) {
			/* It is a CD1400 rev. J or later */
			/* Impossible to reach 5ms with this chip.
			   Changed to 2ms instead (f = 500 Hz). */
			cy_writeb(base_addr + (CyPPR << index), CyCLOCK_60_2MS);
		} else {
			/* f = 200 Hz */
			cy_writeb(base_addr + (CyPPR << index), CyCLOCK_25_5MS);
		}

		/*
		   printk(" chip #%d at %#6lx is rev 0x%2x\n",
		   chip_number, (unsigned long)base_addr,
		   readb(base_addr+(CyGFRCR<<index)));
		 */
	}
	return chip_number;
}				/* cyy_init_card */

static int __init cy_detect_isa(void)
{
#ifdef CONFIG_ISA
	unsigned short cy_isa_irq, nboard;
	void __iomem *cy_isa_address;
	unsigned short i, j, cy_isa_nchan;
#ifdef MODULE
	int isparam = 0;
#endif

	nboard = 0;

#ifdef MODULE
	/* Check for module parameters */
	for (i = 0; i < NR_CARDS; i++) {
		if (maddr[i] || i) {
			isparam = 1;
			cy_isa_addresses[i] = maddr[i];
		}
		if (!maddr[i])
			break;
	}
#endif

	/* scan the address table probing for Cyclom-Y/ISA boards */
	for (i = 0; i < NR_ISA_ADDRS; i++) {
		unsigned int isa_address = cy_isa_addresses[i];
		if (isa_address == 0x0000)
			return nboard;

		/* probe for CD1400... */
		cy_isa_address = ioremap_nocache(isa_address, CyISA_Ywin);
		if (cy_isa_address == NULL) {
			printk(KERN_ERR "Cyclom-Y/ISA: can't remap base "
					"address\n");
			continue;
		}
		cy_isa_nchan = CyPORTS_PER_CHIP *
			cyy_init_card(cy_isa_address, 0);
		if (cy_isa_nchan == 0) {
			iounmap(cy_isa_address);
			continue;
		}
#ifdef MODULE
		if (isparam && irq[i])
			cy_isa_irq = irq[i];
		else
#endif
			/* find out the board's irq by probing */
			cy_isa_irq = detect_isa_irq(cy_isa_address);
		if (cy_isa_irq == 0) {
			printk(KERN_ERR "Cyclom-Y/ISA found at 0x%lx, but the "
				"IRQ could not be detected.\n",
				(unsigned long)cy_isa_address);
			iounmap(cy_isa_address);
			continue;
		}

		if ((cy_next_channel + cy_isa_nchan) > NR_PORTS) {
			printk(KERN_ERR "Cyclom-Y/ISA found at 0x%lx, but no "
				"more channels are available. Change NR_PORTS "
				"in cyclades.c and recompile kernel.\n",
				(unsigned long)cy_isa_address);
			iounmap(cy_isa_address);
			return nboard;
		}
		/* fill the next cy_card structure available */
		for (j = 0; j < NR_CARDS; j++) {
			if (cy_card[j].base_addr == NULL)
				break;
		}
		if (j == NR_CARDS) {	/* no more cy_cards available */
			printk(KERN_ERR "Cyclom-Y/ISA found at 0x%lx, but no "
				"more cards can be used. Change NR_CARDS in "
				"cyclades.c and recompile kernel.\n",
				(unsigned long)cy_isa_address);
			iounmap(cy_isa_address);
			return nboard;
		}

		/* allocate IRQ */
		if (request_irq(cy_isa_irq, cyy_interrupt,
				IRQF_DISABLED, "Cyclom-Y", &cy_card[j])) {
			printk(KERN_ERR "Cyclom-Y/ISA found at 0x%lx, but "
				"could not allocate IRQ#%d.\n",
				(unsigned long)cy_isa_address, cy_isa_irq);
			iounmap(cy_isa_address);
			return nboard;
		}

		/* set cy_card */
		cy_card[j].base_addr = cy_isa_address;
		cy_card[j].ctl_addr = NULL;
		cy_card[j].irq = (int)cy_isa_irq;
		cy_card[j].bus_index = 0;
		cy_card[j].first_line = cy_next_channel;
		cy_card[j].num_chips = cy_isa_nchan / 4;
		if (cy_init_card(&cy_card[j])) {
			cy_card[j].base_addr = NULL;
			free_irq(cy_isa_irq, &cy_card[j]);
			iounmap(cy_isa_address);
			continue;
		}
		nboard++;

		printk(KERN_INFO "Cyclom-Y/ISA #%d: 0x%lx-0x%lx, IRQ%d found: "
			"%d channels starting from port %d\n",
			j + 1, (unsigned long)cy_isa_address,
			(unsigned long)(cy_isa_address + (CyISA_Ywin - 1)),
			cy_isa_irq, cy_isa_nchan, cy_next_channel);

		for (j = cy_next_channel;
				j < cy_next_channel + cy_isa_nchan; j++)
			tty_register_device(cy_serial_driver, j, NULL);
		cy_next_channel += cy_isa_nchan;
	}
	return nboard;
#else
	return 0;
#endif				/* CONFIG_ISA */
}				/* cy_detect_isa */

#ifdef CONFIG_PCI
static inline int __devinit cyc_isfwstr(const char *str, unsigned int size)
{
	unsigned int a;

	for (a = 0; a < size && *str; a++, str++)
		if (*str & 0x80)
			return -EINVAL;

	for (; a < size; a++, str++)
		if (*str)
			return -EINVAL;

	return 0;
}

static inline void __devinit cyz_fpga_copy(void __iomem *fpga, const u8 *data,
		unsigned int size)
{
	for (; size > 0; size--) {
		cy_writel(fpga, *data++);
		udelay(10);
	}
}

static void __devinit plx_init(struct pci_dev *pdev, int irq,
		struct RUNTIME_9060 __iomem *addr)
{
	/* Reset PLX */
	cy_writel(&addr->init_ctrl, readl(&addr->init_ctrl) | 0x40000000);
	udelay(100L);
	cy_writel(&addr->init_ctrl, readl(&addr->init_ctrl) & ~0x40000000);

	/* Reload Config. Registers from EEPROM */
	cy_writel(&addr->init_ctrl, readl(&addr->init_ctrl) | 0x20000000);
	udelay(100L);
	cy_writel(&addr->init_ctrl, readl(&addr->init_ctrl) & ~0x20000000);

	/* For some yet unknown reason, once the PLX9060 reloads the EEPROM,
	 * the IRQ is lost and, thus, we have to re-write it to the PCI config.
	 * registers. This will remain here until we find a permanent fix.
	 */
	pci_write_config_byte(pdev, PCI_INTERRUPT_LINE, irq);
}

static int __devinit __cyz_load_fw(const struct firmware *fw,
		const char *name, const u32 mailbox, void __iomem *base,
		void __iomem *fpga)
{
	const void *ptr = fw->data;
	const struct zfile_header *h = ptr;
	const struct zfile_config *c, *cs;
	const struct zfile_block *b, *bs;
	unsigned int a, tmp, len = fw->size;
#define BAD_FW KERN_ERR "Bad firmware: "
	if (len < sizeof(*h)) {
		printk(BAD_FW "too short: %u<%zu\n", len, sizeof(*h));
		return -EINVAL;
	}

	cs = ptr + h->config_offset;
	bs = ptr + h->block_offset;

	if ((void *)(cs + h->n_config) > ptr + len ||
			(void *)(bs + h->n_blocks) > ptr + len) {
		printk(BAD_FW "too short");
		return  -EINVAL;
	}

	if (cyc_isfwstr(h->name, sizeof(h->name)) ||
			cyc_isfwstr(h->date, sizeof(h->date))) {
		printk(BAD_FW "bad formatted header string\n");
		return -EINVAL;
	}

	if (strncmp(name, h->name, sizeof(h->name))) {
		printk(BAD_FW "bad name '%s' (expected '%s')\n", h->name, name);
		return -EINVAL;
	}

	tmp = 0;
	for (c = cs; c < cs + h->n_config; c++) {
		for (a = 0; a < c->n_blocks; a++)
			if (c->block_list[a] > h->n_blocks) {
				printk(BAD_FW "bad block ref number in cfgs\n");
				return -EINVAL;
			}
		if (c->mailbox == mailbox && c->function == 0) /* 0 is normal */
			tmp++;
	}
	if (!tmp) {
		printk(BAD_FW "nothing appropriate\n");
		return -EINVAL;
	}

	for (b = bs; b < bs + h->n_blocks; b++)
		if (b->file_offset + b->size > len) {
			printk(BAD_FW "bad block data offset\n");
			return -EINVAL;
		}

	/* everything is OK, let's seek'n'load it */
	for (c = cs; c < cs + h->n_config; c++)
		if (c->mailbox == mailbox && c->function == 0)
			break;

	for (a = 0; a < c->n_blocks; a++) {
		b = &bs[c->block_list[a]];
		if (b->type == ZBLOCK_FPGA) {
			if (fpga != NULL)
				cyz_fpga_copy(fpga, ptr + b->file_offset,
						b->size);
		} else {
			if (base != NULL)
				memcpy_toio(base + b->ram_offset,
					       ptr + b->file_offset, b->size);
		}
	}
#undef BAD_FW
	return 0;
}

static int __devinit cyz_load_fw(struct pci_dev *pdev, void __iomem *base_addr,
		struct RUNTIME_9060 __iomem *ctl_addr, int irq)
{
	const struct firmware *fw;
	struct FIRM_ID __iomem *fid = base_addr + ID_ADDRESS;
	struct CUSTOM_REG __iomem *cust = base_addr;
	struct ZFW_CTRL __iomem *pt_zfwctrl;
	void __iomem *tmp;
	u32 mailbox, status;
	unsigned int i;
	int retval;

	retval = request_firmware(&fw, "cyzfirm.bin", &pdev->dev);
	if (retval) {
		dev_err(&pdev->dev, "can't get firmware\n");
		goto err;
	}

	/* Check whether the firmware is already loaded and running. If
	   positive, skip this board */
	if (Z_FPGA_LOADED(ctl_addr) && readl(&fid->signature) == ZFIRM_ID) {
		u32 cntval = readl(base_addr + 0x190);

		udelay(100);
		if (cntval != readl(base_addr + 0x190)) {
			/* FW counter is working, FW is running */
			dev_dbg(&pdev->dev, "Cyclades-Z FW already loaded. "
					"Skipping board.\n");
			retval = 0;
			goto err_rel;
		}
	}

	/* start boot */
	cy_writel(&ctl_addr->intr_ctrl_stat, readl(&ctl_addr->intr_ctrl_stat) &
			~0x00030800UL);

	mailbox = readl(&ctl_addr->mail_box_0);

	if (mailbox == 0 || Z_FPGA_LOADED(ctl_addr)) {
		/* stops CPU and set window to beginning of RAM */
		cy_writel(&ctl_addr->loc_addr_base, WIN_CREG);
		cy_writel(&cust->cpu_stop, 0);
		cy_writel(&ctl_addr->loc_addr_base, WIN_RAM);
		udelay(100);
	}

	plx_init(pdev, irq, ctl_addr);

	if (mailbox != 0) {
		/* load FPGA */
		retval = __cyz_load_fw(fw, "Cyclom-Z", mailbox, NULL,
				base_addr);
		if (retval)
			goto err_rel;
		if (!Z_FPGA_LOADED(ctl_addr)) {
			dev_err(&pdev->dev, "fw upload successful, but fw is "
					"not loaded\n");
			goto err_rel;
		}
	}

	/* stops CPU and set window to beginning of RAM */
	cy_writel(&ctl_addr->loc_addr_base, WIN_CREG);
	cy_writel(&cust->cpu_stop, 0);
	cy_writel(&ctl_addr->loc_addr_base, WIN_RAM);
	udelay(100);

	/* clear memory */
	for (tmp = base_addr; tmp < base_addr + RAM_SIZE; tmp++)
		cy_writeb(tmp, 255);
	if (mailbox != 0) {
		/* set window to last 512K of RAM */
		cy_writel(&ctl_addr->loc_addr_base, WIN_RAM + RAM_SIZE);
		for (tmp = base_addr; tmp < base_addr + RAM_SIZE; tmp++)
			cy_writeb(tmp, 255);
		/* set window to beginning of RAM */
		cy_writel(&ctl_addr->loc_addr_base, WIN_RAM);
	}

	retval = __cyz_load_fw(fw, "Cyclom-Z", mailbox, base_addr, NULL);
	release_firmware(fw);
	if (retval)
		goto err;

	/* finish boot and start boards */
	cy_writel(&ctl_addr->loc_addr_base, WIN_CREG);
	cy_writel(&cust->cpu_start, 0);
	cy_writel(&ctl_addr->loc_addr_base, WIN_RAM);
	i = 0;
	while ((status = readl(&fid->signature)) != ZFIRM_ID && i++ < 40)
		msleep(100);
	if (status != ZFIRM_ID) {
		if (status == ZFIRM_HLT) {
			dev_err(&pdev->dev, "you need an external power supply "
				"for this number of ports. Firmware halted and "
				"board reset.\n");
			retval = -EIO;
			goto err;
		}
		dev_warn(&pdev->dev, "fid->signature = 0x%x... Waiting "
				"some more time\n", status);
		while ((status = readl(&fid->signature)) != ZFIRM_ID &&
				i++ < 200)
			msleep(100);
		if (status != ZFIRM_ID) {
			dev_err(&pdev->dev, "Board not started in 20 seconds! "
					"Giving up. (fid->signature = 0x%x)\n",
					status);
			dev_info(&pdev->dev, "*** Warning ***: if you are "
				"upgrading the FW, please power cycle the "
				"system before loading the new FW to the "
				"Cyclades-Z.\n");

			if (Z_FPGA_LOADED(ctl_addr))
				plx_init(pdev, irq, ctl_addr);

			retval = -EIO;
			goto err;
		}
		dev_dbg(&pdev->dev, "Firmware started after %d seconds.\n",
				i / 10);
	}
	pt_zfwctrl = base_addr + readl(&fid->zfwctrl_addr);

	dev_dbg(&pdev->dev, "fid=> %p, zfwctrl_addr=> %x, npt_zfwctrl=> %p\n",
			base_addr + ID_ADDRESS, readl(&fid->zfwctrl_addr),
			base_addr + readl(&fid->zfwctrl_addr));

	dev_info(&pdev->dev, "Cyclades-Z FW loaded: version = %x, ports = %u\n",
		readl(&pt_zfwctrl->board_ctrl.fw_version),
		readl(&pt_zfwctrl->board_ctrl.n_channel));

	if (readl(&pt_zfwctrl->board_ctrl.n_channel) == 0) {
		dev_warn(&pdev->dev, "no Cyclades-Z ports were found. Please "
			"check the connection between the Z host card and the "
			"serial expanders.\n");

		if (Z_FPGA_LOADED(ctl_addr))
			plx_init(pdev, irq, ctl_addr);

		dev_info(&pdev->dev, "Null number of ports detected. Board "
				"reset.\n");
		retval = 0;
		goto err;
	}

	cy_writel(&pt_zfwctrl->board_ctrl.op_system, C_OS_LINUX);
	cy_writel(&pt_zfwctrl->board_ctrl.dr_version, DRIVER_VERSION);

	/*
	   Early firmware failed to start looking for commands.
	   This enables firmware interrupts for those commands.
	 */
	cy_writel(&ctl_addr->intr_ctrl_stat, readl(&ctl_addr->intr_ctrl_stat) |
			(1 << 17));
	cy_writel(&ctl_addr->intr_ctrl_stat, readl(&ctl_addr->intr_ctrl_stat) |
			0x00030800UL);

	plx_init(pdev, irq, ctl_addr);

	return 0;
err_rel:
	release_firmware(fw);
err:
	return retval;
}

static int __devinit cy_pci_probe(struct pci_dev *pdev,
		const struct pci_device_id *ent)
{
	void __iomem *addr0 = NULL, *addr2 = NULL;
	char *card_name = NULL;
	u32 mailbox;
	unsigned int device_id, nchan = 0, card_no, i;
	unsigned char plx_ver;
	int retval, irq;

	retval = pci_enable_device(pdev);
	if (retval) {
		dev_err(&pdev->dev, "cannot enable device\n");
		goto err;
	}

	/* read PCI configuration area */
	irq = pdev->irq;
	device_id = pdev->device & ~PCI_DEVICE_ID_MASK;

#if defined(__alpha__)
	if (device_id == PCI_DEVICE_ID_CYCLOM_Y_Lo) {	/* below 1M? */
		dev_err(&pdev->dev, "Cyclom-Y/PCI not supported for low "
			"addresses on Alpha systems.\n");
		retval = -EIO;
		goto err_dis;
	}
#endif
	if (device_id == PCI_DEVICE_ID_CYCLOM_Z_Lo) {
		dev_err(&pdev->dev, "Cyclades-Z/PCI not supported for low "
			"addresses\n");
		retval = -EIO;
		goto err_dis;
	}

	if (pci_resource_flags(pdev, 2) & IORESOURCE_IO) {
		dev_warn(&pdev->dev, "PCI I/O bit incorrectly set. Ignoring "
				"it...\n");
		pdev->resource[2].flags &= ~IORESOURCE_IO;
	}

	retval = pci_request_regions(pdev, "cyclades");
	if (retval) {
		dev_err(&pdev->dev, "failed to reserve resources\n");
		goto err_dis;
	}

	retval = -EIO;
	if (device_id == PCI_DEVICE_ID_CYCLOM_Y_Lo ||
			device_id == PCI_DEVICE_ID_CYCLOM_Y_Hi) {
		card_name = "Cyclom-Y";

		addr0 = ioremap_nocache(pci_resource_start(pdev, 0),
				CyPCI_Yctl);
		if (addr0 == NULL) {
			dev_err(&pdev->dev, "can't remap ctl region\n");
			goto err_reg;
		}
		addr2 = ioremap_nocache(pci_resource_start(pdev, 2),
				CyPCI_Ywin);
		if (addr2 == NULL) {
			dev_err(&pdev->dev, "can't remap base region\n");
			goto err_unmap;
		}

		nchan = CyPORTS_PER_CHIP * cyy_init_card(addr2, 1);
		if (nchan == 0) {
			dev_err(&pdev->dev, "Cyclom-Y PCI host card with no "
					"Serial-Modules\n");
			goto err_unmap;
		}
	} else if (device_id == PCI_DEVICE_ID_CYCLOM_Z_Hi) {
		struct RUNTIME_9060 __iomem *ctl_addr;

		ctl_addr = addr0 = ioremap_nocache(pci_resource_start(pdev, 0),
				CyPCI_Zctl);
		if (addr0 == NULL) {
			dev_err(&pdev->dev, "can't remap ctl region\n");
			goto err_reg;
		}

		/* Disable interrupts on the PLX before resetting it */
		cy_writew(addr0 + 0x68, readw(addr0 + 0x68) & ~0x0900);

		plx_init(pdev, irq, addr0);

		mailbox = (u32)readl(&ctl_addr->mail_box_0);

		addr2 = ioremap_nocache(pci_resource_start(pdev, 2),
				mailbox == ZE_V1 ? CyPCI_Ze_win : CyPCI_Zwin);
		if (addr2 == NULL) {
			dev_err(&pdev->dev, "can't remap base region\n");
			goto err_unmap;
		}

		if (mailbox == ZE_V1) {
			card_name = "Cyclades-Ze";

			readl(&ctl_addr->mail_box_0);
			nchan = ZE_V1_NPORTS;
		} else {
			card_name = "Cyclades-8Zo";

#ifdef CY_PCI_DEBUG
			if (mailbox == ZO_V1) {
				cy_writel(&ctl_addr->loc_addr_base, WIN_CREG);
				dev_info(&pdev->dev, "Cyclades-8Zo/PCI: FPGA "
					"id %lx, ver %lx\n", (ulong)(0xff &
					readl(&((struct CUSTOM_REG *)addr2)->
						fpga_id)), (ulong)(0xff &
					readl(&((struct CUSTOM_REG *)addr2)->
						fpga_version)));
				cy_writel(&ctl_addr->loc_addr_base, WIN_RAM);
			} else {
				dev_info(&pdev->dev, "Cyclades-Z/PCI: New "
					"Cyclades-Z board.  FPGA not loaded\n");
			}
#endif
			/* The following clears the firmware id word.  This
			   ensures that the driver will not attempt to talk to
			   the board until it has been properly initialized.
			 */
			if ((mailbox == ZO_V1) || (mailbox == ZO_V2))
				cy_writel(addr2 + ID_ADDRESS, 0L);

			retval = cyz_load_fw(pdev, addr2, addr0, irq);
			if (retval)
				goto err_unmap;
			/* This must be a Cyclades-8Zo/PCI.  The extendable
			   version will have a different device_id and will
			   be allocated its maximum number of ports. */
			nchan = 8;
		}
	}

	if ((cy_next_channel + nchan) > NR_PORTS) {
		dev_err(&pdev->dev, "Cyclades-8Zo/PCI found, but no "
			"channels are available. Change NR_PORTS in "
			"cyclades.c and recompile kernel.\n");
		goto err_unmap;
	}
	/* fill the next cy_card structure available */
	for (card_no = 0; card_no < NR_CARDS; card_no++) {
		if (cy_card[card_no].base_addr == NULL)
			break;
	}
	if (card_no == NR_CARDS) {	/* no more cy_cards available */
		dev_err(&pdev->dev, "Cyclades-8Zo/PCI found, but no "
			"more cards can be used. Change NR_CARDS in "
			"cyclades.c and recompile kernel.\n");
		goto err_unmap;
	}

	if (device_id == PCI_DEVICE_ID_CYCLOM_Y_Lo ||
			device_id == PCI_DEVICE_ID_CYCLOM_Y_Hi) {
		/* allocate IRQ */
		retval = request_irq(irq, cyy_interrupt,
				IRQF_SHARED, "Cyclom-Y", &cy_card[card_no]);
		if (retval) {
			dev_err(&pdev->dev, "could not allocate IRQ\n");
			goto err_unmap;
		}
		cy_card[card_no].num_chips = nchan / 4;
	} else {
#ifdef CONFIG_CYZ_INTR
		/* allocate IRQ only if board has an IRQ */
		if (irq != 0 && irq != 255) {
			retval = request_irq(irq, cyz_interrupt,
					IRQF_SHARED, "Cyclades-Z",
					&cy_card[card_no]);
			if (retval) {
				dev_err(&pdev->dev, "could not allocate IRQ\n");
				goto err_unmap;
			}
		}
#endif				/* CONFIG_CYZ_INTR */
		cy_card[card_no].num_chips = (unsigned int)-1;
	}

	/* set cy_card */
	cy_card[card_no].base_addr = addr2;
	cy_card[card_no].ctl_addr = addr0;
	cy_card[card_no].irq = irq;
	cy_card[card_no].bus_index = 1;
	cy_card[card_no].first_line = cy_next_channel;
	retval = cy_init_card(&cy_card[card_no]);
	if (retval)
		goto err_null;

	pci_set_drvdata(pdev, &cy_card[card_no]);

	if (device_id == PCI_DEVICE_ID_CYCLOM_Y_Lo ||
			device_id == PCI_DEVICE_ID_CYCLOM_Y_Hi) {
		/* enable interrupts in the PCI interface */
		plx_ver = readb(addr2 + CyPLX_VER) & 0x0f;
		switch (plx_ver) {
		case PLX_9050:

			cy_writeb(addr0 + 0x4c, 0x43);
			break;

		case PLX_9060:
		case PLX_9080:
		default:	/* Old boards, use PLX_9060 */
			plx_init(pdev, irq, addr0);
			cy_writew(addr0 + 0x68, readw(addr0 + 0x68) | 0x0900);
			break;
		}
	}

	dev_info(&pdev->dev, "%s/PCI #%d found: %d channels starting from "
		"port %d.\n", card_name, card_no + 1, nchan, cy_next_channel);
	for (i = cy_next_channel; i < cy_next_channel + nchan; i++)
		tty_register_device(cy_serial_driver, i, &pdev->dev);
	cy_next_channel += nchan;

	return 0;
err_null:
	cy_card[card_no].base_addr = NULL;
	free_irq(irq, &cy_card[card_no]);
err_unmap:
	iounmap(addr0);
	if (addr2)
		iounmap(addr2);
err_reg:
	pci_release_regions(pdev);
err_dis:
	pci_disable_device(pdev);
err:
	return retval;
}

static void __devexit cy_pci_remove(struct pci_dev *pdev)
{
	struct cyclades_card *cinfo = pci_get_drvdata(pdev);
	unsigned int i;

	/* non-Z with old PLX */
	if (!IS_CYC_Z(*cinfo) && (readb(cinfo->base_addr + CyPLX_VER) & 0x0f) ==
			PLX_9050)
		cy_writeb(cinfo->ctl_addr + 0x4c, 0);
	else
#ifndef CONFIG_CYZ_INTR
		if (!IS_CYC_Z(*cinfo))
#endif
		cy_writew(cinfo->ctl_addr + 0x68,
				readw(cinfo->ctl_addr + 0x68) & ~0x0900);

	iounmap(cinfo->base_addr);
	if (cinfo->ctl_addr)
		iounmap(cinfo->ctl_addr);
	if (cinfo->irq
#ifndef CONFIG_CYZ_INTR
		&& !IS_CYC_Z(*cinfo)
#endif /* CONFIG_CYZ_INTR */
		)
		free_irq(cinfo->irq, cinfo);
	pci_release_regions(pdev);

	cinfo->base_addr = NULL;
	for (i = cinfo->first_line; i < cinfo->first_line +
			cinfo->nports; i++)
		tty_unregister_device(cy_serial_driver, i);
	cinfo->nports = 0;
	kfree(cinfo->ports);
}

static struct pci_driver cy_pci_driver = {
	.name = "cyclades",
	.id_table = cy_pci_dev_id,
	.probe = cy_pci_probe,
	.remove = __devexit_p(cy_pci_remove)
};
#endif

static int
cyclades_get_proc_info(char *buf, char **start, off_t offset, int length,
		int *eof, void *data)
{
	struct cyclades_port *info;
	unsigned int i, j;
	int len = 0;
	off_t begin = 0;
	off_t pos = 0;
	int size;
	__u32 cur_jifs = jiffies;

	size = sprintf(buf, "Dev TimeOpen   BytesOut  IdleOut    BytesIn   "
			"IdleIn  Overruns  Ldisc\n");

	pos += size;
	len += size;

	/* Output one line for each known port */
	for (i = 0; i < NR_CARDS; i++)
		for (j = 0; j < cy_card[i].nports; j++) {
			info = &cy_card[i].ports[j];

			if (info->port.count)
				size = sprintf(buf + len, "%3d %8lu %10lu %8lu "
					"%10lu %8lu %9lu %6ld\n", info->line,
					(cur_jifs - info->idle_stats.in_use) /
					HZ, info->idle_stats.xmit_bytes,
					(cur_jifs - info->idle_stats.xmit_idle)/
					HZ, info->idle_stats.recv_bytes,
					(cur_jifs - info->idle_stats.recv_idle)/
					HZ, info->idle_stats.overruns,
					/* FIXME: double check locking */
					(long)info->port.tty->ldisc.ops->num);
			else
				size = sprintf(buf + len, "%3d %8lu %10lu %8lu "
					"%10lu %8lu %9lu %6ld\n",
					info->line, 0L, 0L, 0L, 0L, 0L, 0L, 0L);
			len += size;
			pos = begin + len;

			if (pos < offset) {
				len = 0;
				begin = pos;
			}
			if (pos > offset + length)
				goto done;
		}
	*eof = 1;
done:
	*start = buf + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	if (len < 0)
		len = 0;
	return len;
}


static const struct tty_operations cy_ops = {
	.open = cy_open,
	.close = cy_close,
	.write = cy_write,
	.put_char = cy_put_char,
	.flush_chars = cy_flush_chars,
	.write_room = cy_write_room,
	.chars_in_buffer = cy_chars_in_buffer,
	.flush_buffer = cy_flush_buffer,
	.ioctl = cy_ioctl,
	.throttle = cy_throttle,
	.unthrottle = cy_unthrottle,
	.set_termios = cy_set_termios,
	.stop = cy_stop,
	.start = cy_start,
	.hangup = cy_hangup,
	.break_ctl = cy_break,
	.wait_until_sent = cy_wait_until_sent,
	.read_proc = cyclades_get_proc_info,
	.tiocmget = cy_tiocmget,
	.tiocmset = cy_tiocmset,
};

static int __init cy_init(void)
{
	unsigned int nboards;
	int retval = -ENOMEM;

	cy_serial_driver = alloc_tty_driver(NR_PORTS);
	if (!cy_serial_driver)
		goto err;

	printk(KERN_INFO "Cyclades driver " CY_VERSION " (built %s %s)\n",
			__DATE__, __TIME__);

	/* Initialize the tty_driver structure */

	cy_serial_driver->owner = THIS_MODULE;
	cy_serial_driver->driver_name = "cyclades";
	cy_serial_driver->name = "ttyC";
	cy_serial_driver->major = CYCLADES_MAJOR;
	cy_serial_driver->minor_start = 0;
	cy_serial_driver->type = TTY_DRIVER_TYPE_SERIAL;
	cy_serial_driver->subtype = SERIAL_TYPE_NORMAL;
	cy_serial_driver->init_termios = tty_std_termios;
	cy_serial_driver->init_termios.c_cflag =
	    B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	cy_serial_driver->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	tty_set_operations(cy_serial_driver, &cy_ops);

	retval = tty_register_driver(cy_serial_driver);
	if (retval) {
		printk(KERN_ERR "Couldn't register Cyclades serial driver\n");
		goto err_frtty;
	}

	/* the code below is responsible to find the boards. Each different
	   type of board has its own detection routine. If a board is found,
	   the next cy_card structure available is set by the detection
	   routine. These functions are responsible for checking the
	   availability of cy_card and cy_port data structures and updating
	   the cy_next_channel. */

	/* look for isa boards */
	nboards = cy_detect_isa();

#ifdef CONFIG_PCI
	/* look for pci boards */
	retval = pci_register_driver(&cy_pci_driver);
	if (retval && !nboards) {
		tty_unregister_driver(cy_serial_driver);
		goto err_frtty;
	}
#endif

	return 0;
err_frtty:
	put_tty_driver(cy_serial_driver);
err:
	return retval;
}				/* cy_init */

static void __exit cy_cleanup_module(void)
{
	struct cyclades_card *card;
	unsigned int i, e1;

#ifndef CONFIG_CYZ_INTR
	del_timer_sync(&cyz_timerlist);
#endif /* CONFIG_CYZ_INTR */

	e1 = tty_unregister_driver(cy_serial_driver);
	if (e1)
		printk(KERN_ERR "failed to unregister Cyclades serial "
				"driver(%d)\n", e1);

#ifdef CONFIG_PCI
	pci_unregister_driver(&cy_pci_driver);
#endif

	for (i = 0; i < NR_CARDS; i++) {
		card = &cy_card[i];
		if (card->base_addr) {
			/* clear interrupt */
			cy_writeb(card->base_addr + Cy_ClrIntr, 0);
			iounmap(card->base_addr);
			if (card->ctl_addr)
				iounmap(card->ctl_addr);
			if (card->irq
#ifndef CONFIG_CYZ_INTR
				&& !IS_CYC_Z(*card)
#endif /* CONFIG_CYZ_INTR */
				)
				free_irq(card->irq, card);
			for (e1 = card->first_line; e1 < card->first_line +
					card->nports; e1++)
				tty_unregister_device(cy_serial_driver, e1);
			kfree(card->ports);
		}
	}

	put_tty_driver(cy_serial_driver);
} /* cy_cleanup_module */

module_init(cy_init);
module_exit(cy_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_VERSION(CY_VERSION);
