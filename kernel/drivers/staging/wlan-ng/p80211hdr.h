

#ifndef _P80211HDR_H
#define _P80211HDR_H

/*================================================================*/
/* System Includes */

/*================================================================*/
/* Project Includes */

#ifndef  _WLAN_COMPAT_H
#include "wlan_compat.h"
#endif


/*================================================================*/
/* Constants */

/*--- Sizes -----------------------------------------------*/
#define WLAN_ADDR_LEN			6
#define WLAN_CRC_LEN			4
#define WLAN_BSSID_LEN			6
#define WLAN_BSS_TS_LEN			8
#define WLAN_HDR_A3_LEN			24
#define WLAN_HDR_A4_LEN			30
#define WLAN_SSID_MAXLEN		32
#define WLAN_DATA_MAXLEN		2312
#define WLAN_A3FR_MAXLEN		(WLAN_HDR_A3_LEN + WLAN_DATA_MAXLEN + WLAN_CRC_LEN)
#define WLAN_A4FR_MAXLEN		(WLAN_HDR_A4_LEN + WLAN_DATA_MAXLEN + WLAN_CRC_LEN)
#define WLAN_BEACON_FR_MAXLEN		(WLAN_HDR_A3_LEN + 334)
#define WLAN_ATIM_FR_MAXLEN		(WLAN_HDR_A3_LEN + 0)
#define WLAN_DISASSOC_FR_MAXLEN		(WLAN_HDR_A3_LEN + 2)
#define WLAN_ASSOCREQ_FR_MAXLEN		(WLAN_HDR_A3_LEN + 48)
#define WLAN_ASSOCRESP_FR_MAXLEN	(WLAN_HDR_A3_LEN + 16)
#define WLAN_REASSOCREQ_FR_MAXLEN	(WLAN_HDR_A3_LEN + 54)
#define WLAN_REASSOCRESP_FR_MAXLEN	(WLAN_HDR_A3_LEN + 16)
#define WLAN_PROBEREQ_FR_MAXLEN		(WLAN_HDR_A3_LEN + 44)
#define WLAN_PROBERESP_FR_MAXLEN	(WLAN_HDR_A3_LEN + 78)
#define WLAN_AUTHEN_FR_MAXLEN		(WLAN_HDR_A3_LEN + 261)
#define WLAN_DEAUTHEN_FR_MAXLEN		(WLAN_HDR_A3_LEN + 2)
#define WLAN_WEP_NKEYS			4
#define WLAN_WEP_MAXKEYLEN		13
#define WLAN_CHALLENGE_IE_LEN		130
#define WLAN_CHALLENGE_LEN		128
#define WLAN_WEP_IV_LEN			4
#define WLAN_WEP_ICV_LEN		4

/*--- Frame Control Field -------------------------------------*/
/* Frame Types */
#define WLAN_FTYPE_MGMT			0x00
#define WLAN_FTYPE_CTL			0x01
#define WLAN_FTYPE_DATA			0x02

/* Frame subtypes */
/* Management */
#define WLAN_FSTYPE_ASSOCREQ		0x00
#define WLAN_FSTYPE_ASSOCRESP		0x01
#define WLAN_FSTYPE_REASSOCREQ		0x02
#define WLAN_FSTYPE_REASSOCRESP		0x03
#define WLAN_FSTYPE_PROBEREQ		0x04
#define WLAN_FSTYPE_PROBERESP		0x05
#define WLAN_FSTYPE_BEACON		0x08
#define WLAN_FSTYPE_ATIM		0x09
#define WLAN_FSTYPE_DISASSOC		0x0a
#define WLAN_FSTYPE_AUTHEN		0x0b
#define WLAN_FSTYPE_DEAUTHEN		0x0c

/* Control */
#define WLAN_FSTYPE_BLOCKACKREQ		0x8
#define WLAN_FSTYPE_BLOCKACK  		0x9
#define WLAN_FSTYPE_PSPOLL		0x0a
#define WLAN_FSTYPE_RTS			0x0b
#define WLAN_FSTYPE_CTS			0x0c
#define WLAN_FSTYPE_ACK			0x0d
#define WLAN_FSTYPE_CFEND		0x0e
#define WLAN_FSTYPE_CFENDCFACK		0x0f

/* Data */
#define WLAN_FSTYPE_DATAONLY		0x00
#define WLAN_FSTYPE_DATA_CFACK		0x01
#define WLAN_FSTYPE_DATA_CFPOLL		0x02
#define WLAN_FSTYPE_DATA_CFACK_CFPOLL	0x03
#define WLAN_FSTYPE_NULL		0x04
#define WLAN_FSTYPE_CFACK		0x05
#define WLAN_FSTYPE_CFPOLL		0x06
#define WLAN_FSTYPE_CFACK_CFPOLL	0x07


/*================================================================*/
/* Macros */

/*--- FC Macros ----------------------------------------------*/
/* Macros to get/set the bitfields of the Frame Control Field */
/*  GET_FC_??? - takes the host byte-order value of an FC     */
/*               and retrieves the value of one of the        */
/*               bitfields and moves that value so its lsb is */
/*               in bit 0.                                    */
/*  SET_FC_??? - takes a host order value for one of the FC   */
/*               bitfields and moves it to the proper bit     */
/*               location for ORing into a host order FC.     */
/*               To send the FC produced from SET_FC_???,     */
/*               one must put the bytes in IEEE order.        */
/*  e.g.                                                      */
/*     printf("the frame subtype is %x",                      */
/*                 GET_FC_FTYPE( ieee2host( rx.fc )))         */
/*                                                            */
/*     tx.fc = host2ieee( SET_FC_FTYPE(WLAN_FTYP_CTL) |       */
/*                        SET_FC_FSTYPE(WLAN_FSTYPE_RTS) );   */
/*------------------------------------------------------------*/

#define WLAN_GET_FC_PVER(n)	 (((u16)(n)) & (BIT0 | BIT1))
#define WLAN_GET_FC_FTYPE(n)	((((u16)(n)) & (BIT2 | BIT3)) >> 2)
#define WLAN_GET_FC_FSTYPE(n)	((((u16)(n)) & (BIT4|BIT5|BIT6|BIT7)) >> 4)
#define WLAN_GET_FC_TODS(n) 	((((u16)(n)) & (BIT8)) >> 8)
#define WLAN_GET_FC_FROMDS(n)	((((u16)(n)) & (BIT9)) >> 9)
#define WLAN_GET_FC_MOREFRAG(n) ((((u16)(n)) & (BIT10)) >> 10)
#define WLAN_GET_FC_RETRY(n)	((((u16)(n)) & (BIT11)) >> 11)
#define WLAN_GET_FC_PWRMGT(n)	((((u16)(n)) & (BIT12)) >> 12)
#define WLAN_GET_FC_MOREDATA(n) ((((u16)(n)) & (BIT13)) >> 13)
#define WLAN_GET_FC_ISWEP(n)	((((u16)(n)) & (BIT14)) >> 14)
#define WLAN_GET_FC_ORDER(n)	((((u16)(n)) & (BIT15)) >> 15)

#define WLAN_SET_FC_PVER(n)	((u16)(n))
#define WLAN_SET_FC_FTYPE(n)	(((u16)(n)) << 2)
#define WLAN_SET_FC_FSTYPE(n)	(((u16)(n)) << 4)
#define WLAN_SET_FC_TODS(n) 	(((u16)(n)) << 8)
#define WLAN_SET_FC_FROMDS(n)	(((u16)(n)) << 9)
#define WLAN_SET_FC_MOREFRAG(n) (((u16)(n)) << 10)
#define WLAN_SET_FC_RETRY(n)	(((u16)(n)) << 11)
#define WLAN_SET_FC_PWRMGT(n)	(((u16)(n)) << 12)
#define WLAN_SET_FC_MOREDATA(n) (((u16)(n)) << 13)
#define WLAN_SET_FC_ISWEP(n)	(((u16)(n)) << 14)
#define WLAN_SET_FC_ORDER(n)	(((u16)(n)) << 15)

/*--- Duration Macros ----------------------------------------*/
/* Macros to get/set the bitfields of the Duration Field      */
/*  - the duration value is only valid when bit15 is zero     */
/*  - the firmware handles these values, so I'm not going     */
/*    these macros right now.                                 */
/*------------------------------------------------------------*/

/*--- Sequence Control  Macros -------------------------------*/
/* Macros to get/set the bitfields of the Sequence Control    */
/* Field.                                                     */
/*------------------------------------------------------------*/
#define WLAN_GET_SEQ_FRGNUM(n) (((u16)(n)) & (BIT0|BIT1|BIT2|BIT3))
#define WLAN_GET_SEQ_SEQNUM(n) ((((u16)(n)) & (~(BIT0|BIT1|BIT2|BIT3))) >> 4)

/*--- Data ptr macro -----------------------------------------*/
/* Creates a u8* to the data portion of a frame            */
/* Assumes you're passing in a ptr to the beginning of the hdr*/
/*------------------------------------------------------------*/
#define WLAN_HDR_A3_DATAP(p) (((u8*)(p)) + WLAN_HDR_A3_LEN)
#define WLAN_HDR_A4_DATAP(p) (((u8*)(p)) + WLAN_HDR_A4_LEN)

#define DOT11_RATE5_ISBASIC_GET(r)     (((u8)(r)) & BIT7)

/*================================================================*/
/* Types */

/* BSS Timestamp */
typedef u8 wlan_bss_ts_t[WLAN_BSS_TS_LEN];

/* Generic 802.11 Header types */

typedef struct p80211_hdr_a3
{
	u16	fc;
	u16	dur;
	u8	a1[WLAN_ADDR_LEN];
	u8	a2[WLAN_ADDR_LEN];
	u8	a3[WLAN_ADDR_LEN];
	u16	seq;
} __WLAN_ATTRIB_PACK__ p80211_hdr_a3_t;

typedef struct p80211_hdr_a4
{
	u16	fc;
	u16	dur;
	u8	a1[WLAN_ADDR_LEN];
	u8	a2[WLAN_ADDR_LEN];
	u8	a3[WLAN_ADDR_LEN];
	u16	seq;
	u8	a4[WLAN_ADDR_LEN];
} __WLAN_ATTRIB_PACK__ p80211_hdr_a4_t;

typedef union p80211_hdr
{
	p80211_hdr_a3_t		a3;
	p80211_hdr_a4_t		a4;
} __WLAN_ATTRIB_PACK__ p80211_hdr_t;


/*================================================================*/
/* Extern Declarations */


/*================================================================*/
/* Function Declarations */

/* Frame and header lenght macros */

#define WLAN_CTL_FRAMELEN(fstype) (\
	(fstype) == WLAN_FSTYPE_BLOCKACKREQ	? 24 : \
	(fstype) == WLAN_FSTYPE_BLOCKACK   	? 152 : \
	(fstype) == WLAN_FSTYPE_PSPOLL		? 20 : \
	(fstype) == WLAN_FSTYPE_RTS		? 20 : \
	(fstype) == WLAN_FSTYPE_CTS		? 14 : \
	(fstype) == WLAN_FSTYPE_ACK		? 14 : \
	(fstype) == WLAN_FSTYPE_CFEND		? 20 : \
	(fstype) == WLAN_FSTYPE_CFENDCFACK	? 20 : 4)

#define WLAN_FCS_LEN			4

/* ftcl in HOST order */
inline static u16 p80211_headerlen(u16 fctl)
{
	u16 hdrlen = 0;

	switch ( WLAN_GET_FC_FTYPE(fctl) ) {
	case WLAN_FTYPE_MGMT:
		hdrlen = WLAN_HDR_A3_LEN;
		break;
	case WLAN_FTYPE_DATA:
		hdrlen = WLAN_HDR_A3_LEN;
		if ( WLAN_GET_FC_TODS(fctl) && WLAN_GET_FC_FROMDS(fctl) ) {
			hdrlen += WLAN_ADDR_LEN;
		}
		break;
	case WLAN_FTYPE_CTL:
		hdrlen = WLAN_CTL_FRAMELEN(WLAN_GET_FC_FSTYPE(fctl)) -
			WLAN_FCS_LEN;
		break;
	default:
		hdrlen = WLAN_HDR_A3_LEN;
	}

	return hdrlen;
}

#endif /* _P80211HDR_H */
