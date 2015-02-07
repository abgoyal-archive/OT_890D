


#ifndef _P80211IOCTL_H
#define _P80211IOCTL_H

/*================================================================*/
/* Constants */

/*----------------------------------------------------------------*/
/* p80211 ioctl "request" codes.  See argument 2 of ioctl(2). */

#define P80211_IFTEST		(SIOCDEVPRIVATE + 0)
#define P80211_IFREQ		(SIOCDEVPRIVATE + 1)

/*----------------------------------------------------------------*/
/* Magic number, a quick test to see we're getting the desired struct */

#define P80211_IOCTL_MAGIC	(0x4a2d464dUL)

/*----------------------------------------------------------------*/
/* Netlink protocol numbers for the indication interface */

#define P80211_NL_SOCK_IND	NETLINK_USERSOCK

/*----------------------------------------------------------------*/
/* Netlink multicast bits for different types of messages */

#define P80211_NL_MCAST_GRP_MLME	BIT0	/* Local station messages */
#define P80211_NL_MCAST_GRP_SNIFF	BIT1	/* Sniffer messages */
#define P80211_NL_MCAST_GRP_DIST	BIT2	/* Distribution system messages */

/*================================================================*/
/* Macros */


/*================================================================*/
/* Types */

/*----------------------------------------------------------------*/
/* A ptr to the following structure type is passed as the third */
/*  argument to the ioctl system call when issuing a request to */
/*  the p80211 module. */

typedef struct p80211ioctl_req
{
	char 	name[WLAN_DEVNAMELEN_MAX];
	caddr_t data;
	u32	magic;
	u16	len;
	u32	result;
} __WLAN_ATTRIB_PACK__ p80211ioctl_req_t;


/*================================================================*/
/* Extern Declarations */


/*================================================================*/
/* Function Declarations */


#endif /* _P80211IOCTL_H */
