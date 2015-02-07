

#ifndef _P80211MSG_H
#define _P80211MSG_H

/*================================================================*/
/* System Includes */

/*================================================================*/
/* Project Includes */

#ifndef _WLAN_COMPAT_H
#include "wlan_compat.h"
#endif

/*================================================================*/
/* Constants */

#define MSG_BUFF_LEN		4000
#define WLAN_DEVNAMELEN_MAX	16

/*================================================================*/
/* Macros */

/*================================================================*/
/* Types */

/*--------------------------------------------------------------------*/
/*----- Message Structure Types --------------------------------------*/

/*--------------------------------------------------------------------*/
/* Prototype msg type */

typedef struct p80211msg
{
	u32	msgcode;
	u32	msglen;
	u8	devname[WLAN_DEVNAMELEN_MAX];
} __WLAN_ATTRIB_PACK__ p80211msg_t;

typedef struct p80211msgd
{
	u32	msgcode;
	u32	msglen;
	u8	devname[WLAN_DEVNAMELEN_MAX];
	u8	args[0];
} __WLAN_ATTRIB_PACK__ p80211msgd_t;

/*================================================================*/
/* Extern Declarations */


/*================================================================*/
/* Function Declarations */

#endif  /* _P80211MSG_H */

