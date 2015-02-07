

#ifndef WAVELAN_P_H
#define WAVELAN_P_H

/************************** DOCUMENTATION ***************************/

/* ------------------------ SPECIFIC NOTES ------------------------ */

/* --------------------- WIRELESS EXTENSIONS --------------------- */

/* ---------------------------- FILES ---------------------------- */

/* --------------------------- HISTORY --------------------------- */



/* --------------------------- CREDITS --------------------------- */

/* ------------------------- IMPROVEMENTS ------------------------- */

/***************************** INCLUDES *****************************/

#include	<linux/module.h>

#include	<linux/kernel.h>
#include	<linux/sched.h>
#include	<linux/types.h>
#include	<linux/fcntl.h>
#include	<linux/interrupt.h>
#include	<linux/stat.h>
#include	<linux/ptrace.h>
#include	<linux/ioport.h>
#include	<linux/in.h>
#include	<linux/string.h>
#include	<linux/delay.h>
#include	<linux/bitops.h>
#include	<asm/system.h>
#include	<asm/io.h>
#include	<asm/dma.h>
#include	<asm/uaccess.h>
#include	<linux/errno.h>
#include	<linux/netdevice.h>
#include	<linux/etherdevice.h>
#include	<linux/skbuff.h>
#include	<linux/slab.h>
#include	<linux/timer.h>
#include	<linux/init.h>

#include <linux/wireless.h>		/* Wireless extensions */
#include <net/iw_handler.h>		/* Wireless handlers */

/* WaveLAN declarations */
#include	"i82586.h"
#include	"wavelan.h"

/************************** DRIVER OPTIONS **************************/
#undef SET_PSA_CRC		/* Calculate and set the CRC on PSA (slower) */
#define USE_PSA_CONFIG		/* Use info from the PSA. */
#undef EEPROM_IS_PROTECTED	/* doesn't seem to be necessary */
#define MULTICAST_AVOID		/* Avoid extra multicast (I'm sceptical). */
#undef SET_MAC_ADDRESS		/* Experimental */

/* Warning:  this stuff will slow down the driver. */
#define WIRELESS_SPY		/* Enable spying addresses. */
#undef HISTOGRAM		/* Enable histogram of signal level. */

/****************************** DEBUG ******************************/

#undef DEBUG_MODULE_TRACE	/* module insertion/removal */
#undef DEBUG_CALLBACK_TRACE	/* calls made by Linux */
#undef DEBUG_INTERRUPT_TRACE	/* calls to handler */
#undef DEBUG_INTERRUPT_INFO	/* type of interrupt and so on */
#define DEBUG_INTERRUPT_ERROR	/* problems */
#undef DEBUG_CONFIG_TRACE	/* Trace the config functions. */
#undef DEBUG_CONFIG_INFO	/* what's going on */
#define DEBUG_CONFIG_ERROR	/* errors on configuration */
#undef DEBUG_TX_TRACE		/* transmission calls */
#undef DEBUG_TX_INFO		/* header of the transmitted packet */
#undef DEBUG_TX_FAIL		/* Normal failure conditions */
#define DEBUG_TX_ERROR		/* Unexpected conditions */
#undef DEBUG_RX_TRACE		/* transmission calls */
#undef DEBUG_RX_INFO		/* header of the received packet */
#undef DEBUG_RX_FAIL		/* Normal failure conditions */
#define DEBUG_RX_ERROR		/* Unexpected conditions */

#undef DEBUG_PACKET_DUMP	/* Dump packet on the screen if defined to 32. */
#undef DEBUG_IOCTL_TRACE	/* misc. call by Linux */
#undef DEBUG_IOCTL_INFO		/* various debugging info */
#define DEBUG_IOCTL_ERROR	/* what's going wrong */
#define DEBUG_BASIC_SHOW	/* Show basic startup info. */
#undef DEBUG_VERSION_SHOW	/* Print version info. */
#undef DEBUG_PSA_SHOW		/* Dump PSA to screen. */
#undef DEBUG_MMC_SHOW		/* Dump mmc to screen. */
#undef DEBUG_SHOW_UNUSED	/* Show unused fields too. */
#undef DEBUG_I82586_SHOW	/* Show i82586 status. */
#undef DEBUG_DEVICE_SHOW	/* Show device parameters. */

/************************ CONSTANTS & MACROS ************************/

#ifdef DEBUG_VERSION_SHOW
static const char	*version	= "wavelan.c : v24 (SMP + wireless extensions) 11/12/01\n";
#endif

/* Watchdog temporisation */
#define	WATCHDOG_JIFFIES	(512*HZ/100)

/* ------------------------ PRIVATE IOCTL ------------------------ */

#define SIOCSIPQTHR	SIOCIWFIRSTPRIV		/* Set quality threshold */
#define SIOCGIPQTHR	SIOCIWFIRSTPRIV + 1	/* Get quality threshold */

#define SIOCSIPHISTO	SIOCIWFIRSTPRIV + 2	/* Set histogram ranges */
#define SIOCGIPHISTO	SIOCIWFIRSTPRIV + 3	/* Get histogram values */

/****************************** TYPES ******************************/

/* Shortcuts */
typedef struct net_device_stats	en_stats;
typedef struct iw_statistics	iw_stats;
typedef struct iw_quality	iw_qual;
typedef struct iw_freq		iw_freq;
typedef struct net_local	net_local;
typedef struct timer_list	timer_list;

/* Basic types */
typedef u_char		mac_addr[WAVELAN_ADDR_SIZE];	/* Hardware address */

struct net_local
{
  net_local *	next;		/* linked list of the devices */
  struct net_device *	dev;		/* reverse link */
  spinlock_t	spinlock;	/* Serialize access to the hardware (SMP) */
  en_stats	stats;		/* Ethernet interface statistics */
  int		nresets;	/* number of hardware resets */
  u_char	reconfig_82586;	/* We need to reconfigure the controller. */
  u_char	promiscuous;	/* promiscuous mode */
  int		mc_count;	/* number of multicast addresses */
  u_short	hacr;		/* current host interface state */

  int		tx_n_in_use;
  u_short	rx_head;
  u_short	rx_last;
  u_short	tx_first_free;
  u_short	tx_first_in_use;

  iw_stats	wstats;		/* Wireless-specific statistics */

  struct iw_spy_data	spy_data;
  struct iw_public_data	wireless_data;

#ifdef HISTOGRAM
  int		his_number;		/* number of intervals */
  u_char	his_range[16];		/* boundaries of interval ]n-1; n] */
  u_long	his_sum[16];		/* sum in interval */
#endif	/* HISTOGRAM */
};

/**************************** PROTOTYPES ****************************/

/* ----------------------- MISC. SUBROUTINES ------------------------ */
static u_char
	wv_irq_to_psa(int);
static int
	wv_psa_to_irq(u_char);
/* ------------------- HOST ADAPTER SUBROUTINES ------------------- */
static inline u_short		/* data */
	hasr_read(u_long);	/* Read the host interface:  base address */
static inline void
	hacr_write(u_long,	/* Write to host interface:  base address */
		   u_short),	/* data */
	hacr_write_slow(u_long,
		   u_short),
	set_chan_attn(u_long,	/* ioaddr */
		      u_short),	/* hacr   */
	wv_hacr_reset(u_long),	/* ioaddr */
	wv_16_off(u_long,	/* ioaddr */
		  u_short),	/* hacr   */
	wv_16_on(u_long,	/* ioaddr */
		 u_short),	/* hacr   */
	wv_ints_off(struct net_device *),
	wv_ints_on(struct net_device *);
/* ----------------- MODEM MANAGEMENT SUBROUTINES ----------------- */
static void
	psa_read(u_long,	/* Read the Parameter Storage Area. */
		 u_short,	/* hacr */
		 int,		/* offset in PSA */
		 u_char *,	/* buffer to fill */
		 int),		/* size to read */
	psa_write(u_long, 	/* Write to the PSA. */
		  u_short,	/* hacr */
		  int,		/* offset in PSA */
		  u_char *,	/* buffer in memory */
		  int);		/* length of buffer */
static inline void
	mmc_out(u_long,		/* Write 1 byte to the Modem Manag Control. */
		u_short,
		u_char),
	mmc_write(u_long,	/* Write n bytes to the MMC. */
		  u_char,
		  u_char *,
		  int);
static inline u_char		/* Read 1 byte from the MMC. */
	mmc_in(u_long,
	       u_short);
static inline void
	mmc_read(u_long,	/* Read n bytes from the MMC. */
		 u_char,
		 u_char *,
		 int),
	fee_wait(u_long,	/* Wait for frequency EEPROM:  base address */
		 int,		/* base delay to wait for */
		 int);		/* time to wait */
static void
	fee_read(u_long,	/* Read the frequency EEPROM:  base address */
		 u_short,	/* destination offset */
		 u_short *,	/* data buffer */
		 int);		/* number of registers */
/* ---------------------- I82586 SUBROUTINES ----------------------- */
static /*inline*/ void
	obram_read(u_long,	/* ioaddr */
		   u_short,	/* o */
		   u_char *,	/* b */
		   int);	/* n */
static inline void
	obram_write(u_long,	/* ioaddr */
		    u_short,	/* o */
		    u_char *,	/* b */
		    int);	/* n */
static void
	wv_ack(struct net_device *);
static inline int
	wv_synchronous_cmd(struct net_device *,
			   const char *),
	wv_config_complete(struct net_device *,
			   u_long,
			   net_local *);
static int
	wv_complete(struct net_device *,
		    u_long,
		    net_local *);
static inline void
	wv_82586_reconfig(struct net_device *);
/* ------------------- DEBUG & INFO SUBROUTINES ------------------- */
#ifdef DEBUG_I82586_SHOW
static void
	wv_scb_show(unsigned short);
#endif
static inline void
	wv_init_info(struct net_device *);	/* display startup info */
/* ------------------- IOCTL, STATS & RECONFIG ------------------- */
static en_stats	*
	wavelan_get_stats(struct net_device *);	/* Give stats /proc/net/dev */
static iw_stats *
	wavelan_get_wireless_stats(struct net_device *);
static void
	wavelan_set_multicast_list(struct net_device *);
/* ----------------------- PACKET RECEPTION ----------------------- */
static inline void
	wv_packet_read(struct net_device *,	/* Read a packet from a frame. */
		       u_short,
		       int),
	wv_receive(struct net_device *);	/* Read all packets waiting. */
/* --------------------- PACKET TRANSMISSION --------------------- */
static inline int
	wv_packet_write(struct net_device *,	/* Write a packet to the Tx buffer. */
			void *,
			short);
static int
	wavelan_packet_xmit(struct sk_buff *,	/* Send a packet. */
			    struct net_device *);
/* -------------------- HARDWARE CONFIGURATION -------------------- */
static inline int
	wv_mmc_init(struct net_device *),	/* Initialize the modem. */
	wv_ru_start(struct net_device *),	/* Start the i82586 receiver unit. */
	wv_cu_start(struct net_device *),	/* Start the i82586 command unit. */
	wv_82586_start(struct net_device *);	/* Start the i82586. */
static void
	wv_82586_config(struct net_device *);	/* Configure the i82586. */
static inline void
	wv_82586_stop(struct net_device *);
static int
	wv_hw_reset(struct net_device *),	/* Reset the WaveLAN hardware. */
	wv_check_ioaddr(u_long,		/* ioaddr */
			u_char *);	/* mac address (read) */
/* ---------------------- INTERRUPT HANDLING ---------------------- */
static irqreturn_t
	wavelan_interrupt(int,		/* interrupt handler */
			  void *);
static void
	wavelan_watchdog(struct net_device *);	/* transmission watchdog */
/* ------------------- CONFIGURATION CALLBACKS ------------------- */
static int
	wavelan_open(struct net_device *),	/* Open the device. */
	wavelan_close(struct net_device *),	/* Close the device. */
	wavelan_config(struct net_device *, unsigned short);/* Configure one device. */
extern struct net_device *wavelan_probe(int unit);	/* See Space.c. */

/**************************** VARIABLES ****************************/

static net_local *	wavelan_list	= (net_local *) NULL;

static u_char	irqvals[]	=
{
	   0,    0,    0, 0x01,
	0x02, 0x04,    0, 0x08,
	   0,    0, 0x10, 0x20,
	0x40,    0,    0, 0x80,
};

static unsigned short	iobase[]	=
{
#if	0
  /* Leave out 0x3C0 for now -- seems to clash with some video
   * controllers.
   * Leave out the others too -- we will always use 0x390 and leave
   * 0x300 for the Ethernet device.
   * Jean II:  0x3E0 is fine as well.
   */
  0x300, 0x390, 0x3E0, 0x3C0
#endif	/* 0 */
  0x390, 0x3E0
};

#ifdef	MODULE
/* Parameters set by insmod */
static int	io[4];
static int	irq[4];
static char	*name[4];
module_param_array(io, int, NULL, 0);
module_param_array(irq, int, NULL, 0);
module_param_array(name, charp, NULL, 0);

MODULE_PARM_DESC(io, "WaveLAN I/O base address(es),required");
MODULE_PARM_DESC(irq, "WaveLAN IRQ number(s)");
MODULE_PARM_DESC(name, "WaveLAN interface neme(s)");
#endif	/* MODULE */

#endif	/* WAVELAN_P_H */
