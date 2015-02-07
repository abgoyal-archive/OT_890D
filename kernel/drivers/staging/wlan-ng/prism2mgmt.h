

#ifndef _PRISM2MGMT_H
#define _PRISM2MGMT_H


/*=============================================================*/
/*------ Constants --------------------------------------------*/

/*=============================================================*/
/*------ Macros -----------------------------------------------*/

/*=============================================================*/
/*------ Types and their related constants --------------------*/

/*=============================================================*/
/*------ Static variable externs ------------------------------*/

extern int	prism2_debug;
extern int      prism2_reset_holdtime;
extern int      prism2_reset_settletime;
/*=============================================================*/
/*--- Function Declarations -----------------------------------*/
/*=============================================================*/

u32
prism2sta_ifstate(wlandevice_t *wlandev, u32 ifstate);

void
prism2sta_ev_dtim(wlandevice_t *wlandev);
void
prism2sta_ev_infdrop(wlandevice_t *wlandev);
void
prism2sta_ev_info(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
void
prism2sta_ev_txexc(wlandevice_t *wlandev, u16 status);
void
prism2sta_ev_tx(wlandevice_t *wlandev, u16 status);
void
prism2sta_ev_rx(wlandevice_t *wlandev, struct sk_buff *skb);
void
prism2sta_ev_alloc(wlandevice_t *wlandev);

int prism2mgmt_mibset_mibget(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_scan(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_scan_results(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_start(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_wlansniff(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_readpda(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_ramdl_state(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_ramdl_write(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_flashdl_state(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_flashdl_write(wlandevice_t *wlandev, void *msgp);
int prism2mgmt_autojoin(wlandevice_t *wlandev, void *msgp);

/* byte area conversion functions*/
void prism2mgmt_pstr2bytearea(u8 *bytearea, p80211pstrd_t *pstr);
void prism2mgmt_bytearea2pstr(u8 *bytearea, p80211pstrd_t *pstr, int len);

/* byte string conversion functions*/
void prism2mgmt_pstr2bytestr(hfa384x_bytestr_t *bytestr, p80211pstrd_t *pstr);
void prism2mgmt_bytestr2pstr(hfa384x_bytestr_t *bytestr, p80211pstrd_t *pstr);

/* integer conversion functions */
void prism2mgmt_prism2int2p80211int(u16 *prism2int, u32 *wlanint);
void prism2mgmt_p80211int2prism2int(u16 *prism2int, u32 *wlanint);

/* enumerated integer conversion functions */
void prism2mgmt_prism2enum2p80211enum(u16 *prism2enum, u32 *wlanenum, u16 rid);
void prism2mgmt_p80211enum2prism2enum(u16 *prism2enum, u32 *wlanenum, u16 rid);

/* functions to convert a bit area to/from an Operational Rate Set */
void prism2mgmt_get_oprateset(u16 *rate, p80211pstrd_t *pstr);
void prism2mgmt_set_oprateset(u16 *rate, p80211pstrd_t *pstr);

/* functions to convert Group Addresses */
void prism2mgmt_get_grpaddr(u32 did,
	p80211pstrd_t *pstr, hfa384x_t *priv );
int prism2mgmt_set_grpaddr(u32 did,
	u8 *prism2buf, p80211pstrd_t *pstr, hfa384x_t *priv );
int prism2mgmt_get_grpaddr_index( u32 did );

void prism2sta_processing_defer(struct work_struct *data);

void prism2sta_commsqual_defer(struct work_struct *data);
void prism2sta_commsqual_timer(unsigned long data);

/*=============================================================*/
/*--- Inline Function Definitions (if supported) --------------*/
/*=============================================================*/



#endif