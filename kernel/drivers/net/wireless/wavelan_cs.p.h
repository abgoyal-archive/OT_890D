

#ifndef WAVELAN_CS_P_H
#define WAVELAN_CS_P_H

/************************** DOCUMENTATION **************************/

/* ------------------------ SPECIFIC NOTES ------------------------ */

/* --------------------- WIRELESS EXTENSIONS --------------------- */

/* ---------------------------- FILES ---------------------------- */

/* --------------------------- HISTORY --------------------------- */


/* --------------------------- CREDITS --------------------------- */

/* ------------------------- IMPROVEMENTS ------------------------- */

/***************************** INCLUDES *****************************/

/* Linux headers that we need */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/in.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/ethtool.h>
#include <linux/wireless.h>		/* Wireless extensions */
#include <net/iw_handler.h>		/* New driver API */

/* Pcmcia headers that we need */
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

/* Wavelan declarations */
#include "i82593.h"	/* Definitions for the Intel chip */

#include "wavelan_cs.h"	/* Others bits of the hardware */

/************************** DRIVER OPTIONS **************************/
#define WAVELAN_ROAMING		/* Include experimental roaming code */
#undef WAVELAN_ROAMING_EXT	/* Enable roaming wireless extensions */
#undef SET_PSA_CRC		/* Set the CRC in PSA (slower) */
#define USE_PSA_CONFIG		/* Use info from the PSA */
#undef EEPROM_IS_PROTECTED	/* Doesn't seem to be necessary */
#define MULTICAST_AVOID		/* Avoid extra multicast (I'm sceptical) */
#undef SET_MAC_ADDRESS		/* Experimental */

/* Warning : these stuff will slow down the driver... */
#define WIRELESS_SPY		/* Enable spying addresses */
#undef HISTOGRAM		/* Enable histogram of sig level... */

/****************************** DEBUG ******************************/

#undef DEBUG_MODULE_TRACE	/* Module insertion/removal */
#undef DEBUG_CALLBACK_TRACE	/* Calls made by Linux */
#undef DEBUG_INTERRUPT_TRACE	/* Calls to handler */
#undef DEBUG_INTERRUPT_INFO	/* type of interrupt & so on */
#define DEBUG_INTERRUPT_ERROR	/* problems */
#undef DEBUG_CONFIG_TRACE	/* Trace the config functions */
#undef DEBUG_CONFIG_INFO	/* What's going on... */
#define DEBUG_CONFIG_ERRORS	/* Errors on configuration */
#undef DEBUG_TX_TRACE		/* Transmission calls */
#undef DEBUG_TX_INFO		/* Header of the transmitted packet */
#undef DEBUG_TX_FAIL		/* Normal failure conditions */
#define DEBUG_TX_ERROR		/* Unexpected conditions */
#undef DEBUG_RX_TRACE		/* Transmission calls */
#undef DEBUG_RX_INFO		/* Header of the transmitted packet */
#undef DEBUG_RX_FAIL		/* Normal failure conditions */
#define DEBUG_RX_ERROR		/* Unexpected conditions */
#undef DEBUG_PACKET_DUMP	/* Dump packet on the screen */
#undef DEBUG_IOCTL_TRACE	/* Misc call by Linux */
#undef DEBUG_IOCTL_INFO		/* Various debug info */
#define DEBUG_IOCTL_ERROR	/* What's going wrong */
#define DEBUG_BASIC_SHOW	/* Show basic startup info */
#undef DEBUG_VERSION_SHOW	/* Print version info */
#undef DEBUG_PSA_SHOW		/* Dump psa to screen */
#undef DEBUG_MMC_SHOW		/* Dump mmc to screen */
#undef DEBUG_SHOW_UNUSED	/* Show also unused fields */
#undef DEBUG_I82593_SHOW	/* Show i82593 status */
#undef DEBUG_DEVICE_SHOW	/* Show device parameters */

/************************ CONSTANTS & MACROS ************************/

#ifdef DEBUG_VERSION_SHOW
static const char *version = "wavelan_cs.c : v24 (SMP + wireless extensions) 11/1/02\n";
#endif

/* Watchdog temporisation */
#define	WATCHDOG_JIFFIES	(256*HZ/100)

/* Fix a bug in some old wireless extension definitions */
#ifndef IW_ESSID_MAX_SIZE
#define IW_ESSID_MAX_SIZE	32
#endif

/* ------------------------ PRIVATE IOCTL ------------------------ */

#define SIOCSIPQTHR	SIOCIWFIRSTPRIV		/* Set quality threshold */
#define SIOCGIPQTHR	SIOCIWFIRSTPRIV + 1	/* Get quality threshold */
#define SIOCSIPROAM     SIOCIWFIRSTPRIV + 2	/* Set roaming state */
#define SIOCGIPROAM     SIOCIWFIRSTPRIV + 3	/* Get roaming state */

#define SIOCSIPHISTO	SIOCIWFIRSTPRIV + 4	/* Set histogram ranges */
#define SIOCGIPHISTO	SIOCIWFIRSTPRIV + 5	/* Get histogram values */

/*************************** WaveLAN Roaming  **************************/
#ifdef WAVELAN_ROAMING		/* Conditional compile, see above in options */

#define WAVELAN_ROAMING_DEBUG	 0	/* 1 = Trace of handover decisions */
					/* 2 = Info on each beacon rcvd... */
#define MAX_WAVEPOINTS		7	/* Max visible at one time */
#define WAVEPOINT_HISTORY	5	/* SNR sample history slow search */
#define WAVEPOINT_FAST_HISTORY	2	/* SNR sample history fast search */
#define SEARCH_THRESH_LOW	10	/* SNR to enter cell search */
#define SEARCH_THRESH_HIGH	13	/* SNR to leave cell search */
#define WAVELAN_ROAMING_DELTA	1	/* Hysteresis value (+/- SNR) */
#define CELL_TIMEOUT		2*HZ	/* in jiffies */

#define FAST_CELL_SEARCH	1	/* Boolean values... */
#define NWID_PROMISC		1	/* for code clarity. */

typedef struct wavepoint_beacon
{
  unsigned char		dsap,		/* Unused */
			ssap,		/* Unused */
			ctrl,		/* Unused */
			O,U,I,		/* Unused */
			spec_id1,	/* Unused */
			spec_id2,	/* Unused */
			pdu_type,	/* Unused */
			seq;		/* WavePoint beacon sequence number */
  __be16		domain_id,	/* WavePoint Domain ID */
			nwid;		/* WavePoint NWID */
} wavepoint_beacon;

typedef struct wavepoint_history
{
  unsigned short	nwid;		/* WavePoint's NWID */
  int			average_slow;	/* SNR running average */
  int			average_fast;	/* SNR running average */
  unsigned char	  sigqual[WAVEPOINT_HISTORY]; /* Ringbuffer of recent SNR's */
  unsigned char		qualptr;	/* Index into ringbuffer */
  unsigned char		last_seq;	/* Last seq. no seen for WavePoint */
  struct wavepoint_history *next;	/* Next WavePoint in table */
  struct wavepoint_history *prev;	/* Previous WavePoint in table */
  unsigned long		last_seen;	/* Time of last beacon recvd, jiffies */
} wavepoint_history;

struct wavepoint_table
{
  wavepoint_history	*head;		/* Start of ringbuffer */
  int			num_wavepoints;	/* No. of WavePoints visible */
  unsigned char		locked;		/* Table lock */
};

#endif	/* WAVELAN_ROAMING */

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
  dev_node_t 	node;		/* ???? What is this stuff ???? */
  struct net_device *	dev;		/* Reverse link... */
  spinlock_t	spinlock;	/* Serialize access to the hardware (SMP) */
  struct pcmcia_device *	link;		/* pcmcia structure */
  en_stats	stats;		/* Ethernet interface statistics */
  int		nresets;	/* Number of hw resets */
  u_char	configured;	/* If it is configured */
  u_char	reconfig_82593;	/* Need to reconfigure the controller */
  u_char	promiscuous;	/* Promiscuous mode */
  u_char	allmulticast;	/* All Multicast mode */
  int		mc_count;	/* Number of multicast addresses */

  int   	stop;		/* Current i82593 Stop Hit Register */
  int   	rfp;		/* Last DMA machine receive pointer */
  int		overrunning;	/* Receiver overrun flag */

  iw_stats	wstats;		/* Wireless specific stats */

  struct iw_spy_data	spy_data;
  struct iw_public_data	wireless_data;

#ifdef HISTOGRAM
  int		his_number;		/* Number of intervals */
  u_char	his_range[16];		/* Boundaries of interval ]n-1; n] */
  u_long	his_sum[16];		/* Sum in interval */
#endif	/* HISTOGRAM */
#ifdef WAVELAN_ROAMING
  u_long	domain_id;	/* Domain ID we lock on for roaming */
  int		filter_domains;	/* Check Domain ID of beacon found */
 struct wavepoint_table	wavepoint_table;	/* Table of visible WavePoints*/
  wavepoint_history *	curr_point;		/* Current wavepoint */
  int			cell_search;		/* Searching for new cell? */
  struct timer_list	cell_timer;		/* Garbage collection */
#endif	/* WAVELAN_ROAMING */
  void __iomem *mem;
};

/* ----------------- MODEM MANAGEMENT SUBROUTINES ----------------- */
static inline u_char		/* data */
	hasr_read(u_long);	/* Read the host interface : base address */
static void
	hacr_write(u_long,	/* Write to host interface : base address */
		   u_char),	/* data */
	hacr_write_slow(u_long,
		   u_char);
static void
	psa_read(struct net_device *,	/* Read the Parameter Storage Area */
		 int,		/* offset in PSA */
		 u_char *,	/* buffer to fill */
		 int),		/* size to read */
	psa_write(struct net_device *,	/* Write to the PSA */
		  int,		/* Offset in psa */
		  u_char *,	/* Buffer in memory */
		  int);		/* Length of buffer */
static void
	mmc_out(u_long,		/* Write 1 byte to the Modem Manag Control */
		u_short,
		u_char),
	mmc_write(u_long,	/* Write n bytes to the MMC */
		  u_char,
		  u_char *,
		  int);
static u_char			/* Read 1 byte from the MMC */
	mmc_in(u_long,
	       u_short);
static void
	mmc_read(u_long,	/* Read n bytes from the MMC */
		 u_char,
		 u_char *,
		 int),
	fee_wait(u_long,	/* Wait for frequency EEprom : base address */
		 int,		/* Base delay to wait for */
		 int);		/* Number of time to wait */
static void
	fee_read(u_long,	/* Read the frequency EEprom : base address */
		 u_short,	/* destination offset */
		 u_short *,	/* data buffer */
		 int);		/* number of registers */
/* ---------------------- I82593 SUBROUTINES ----------------------- */
static int
	wv_82593_cmd(struct net_device *,	/* synchronously send a command to i82593 */ 
		     char *,
		     int,
		     int);
static inline int
	wv_diag(struct net_device *);	/* Diagnostique the i82593 */
static int
	read_ringbuf(struct net_device *,	/* Read a receive buffer */
		     int,
		     char *,
		     int);
static void
	wv_82593_reconfig(struct net_device *);	/* Reconfigure the controller */
/* ------------------- DEBUG & INFO SUBROUTINES ------------------- */
static void
	wv_init_info(struct net_device *);	/* display startup info */
/* ------------------- IOCTL, STATS & RECONFIG ------------------- */
static en_stats	*
	wavelan_get_stats(struct net_device *);	/* Give stats /proc/net/dev */
static iw_stats *
	wavelan_get_wireless_stats(struct net_device *);
/* ----------------------- PACKET RECEPTION ----------------------- */
static int
	wv_start_of_frame(struct net_device *,	/* Seek beggining of current frame */
			  int,	/* end of frame */
			  int);	/* start of buffer */
static void
	wv_packet_read(struct net_device *,	/* Read a packet from a frame */
		       int,
		       int),
	wv_packet_rcv(struct net_device *);	/* Read all packets waiting */
/* --------------------- PACKET TRANSMISSION --------------------- */
static void
	wv_packet_write(struct net_device *,	/* Write a packet to the Tx buffer */
			void *,
			short);
static int
	wavelan_packet_xmit(struct sk_buff *,	/* Send a packet */
			    struct net_device *);
/* -------------------- HARDWARE CONFIGURATION -------------------- */
static int
	wv_mmc_init(struct net_device *);	/* Initialize the modem */
static int
	wv_ru_stop(struct net_device *),	/* Stop the i82593 receiver unit */
	wv_ru_start(struct net_device *);	/* Start the i82593 receiver unit */
static int
	wv_82593_config(struct net_device *);	/* Configure the i82593 */
static int
	wv_pcmcia_reset(struct net_device *);	/* Reset the pcmcia interface */
static int
	wv_hw_config(struct net_device *);	/* Reset & configure the whole hardware */
static void
	wv_hw_reset(struct net_device *);	/* Same, + start receiver unit */
static int
	wv_pcmcia_config(struct pcmcia_device *);	/* Configure the pcmcia interface */
static void
	wv_pcmcia_release(struct pcmcia_device *);/* Remove a device */
/* ---------------------- INTERRUPT HANDLING ---------------------- */
static irqreturn_t
	wavelan_interrupt(int,	/* Interrupt handler */
			  void *);
static void
	wavelan_watchdog(struct net_device *);	/* Transmission watchdog */
/* ------------------- CONFIGURATION CALLBACKS ------------------- */
static int
	wavelan_open(struct net_device *),		/* Open the device */
	wavelan_close(struct net_device *);	/* Close the device */
static void
	wavelan_detach(struct pcmcia_device *p_dev);	/* Destroy a removed device */

/**************************** VARIABLES ****************************/


/* Shared memory speed, in ns */
static int	mem_speed = 0;

/* New module interface */
module_param(mem_speed, int, 0);

#ifdef WAVELAN_ROAMING		/* Conditional compile, see above in options */
/* Enable roaming mode ? No ! Please keep this to 0 */
static int	do_roaming = 0;
module_param(do_roaming, bool, 0);
#endif	/* WAVELAN_ROAMING */

MODULE_LICENSE("GPL");

#endif	/* WAVELAN_CS_P_H */

