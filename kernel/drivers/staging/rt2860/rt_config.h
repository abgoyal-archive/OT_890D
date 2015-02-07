
#ifndef	__RT_CONFIG_H__
#define	__RT_CONFIG_H__

#include    "rtmp_type.h"
#ifdef UCOS
#include "includes.h"
#include <stdio.h>
#include 	"rt_ucos.h"
#endif

#ifdef LINUX
#include	"rt_linux.h"
#endif
#include    "rtmp_def.h"
#include    "rt28xx.h"

#ifdef RT2860
#include	"rt2860.h"
#endif // RT2860 //


#include    "oid.h"
#include    "mlme.h"
#include    "wpa.h"
#include    "md5.h"
#include    "rtmp.h"
#include	"ap.h"
#include	"dfs.h"
#include	"chlist.h"
#include	"spectrum.h"

#ifdef LEAP_SUPPORT
#include    "leap.h"
#endif // LEAP_SUPPORT //

#ifdef BLOCK_NET_IF
#include "netif_block.h"
#endif // BLOCK_NET_IF //

#ifdef IGMP_SNOOP_SUPPORT
#include "igmp_snoop.h"
#endif // IGMP_SNOOP_SUPPORT //

#ifdef RALINK_ATE
#include "rt_ate.h"
#endif // RALINK_ATE //

#ifdef CONFIG_STA_SUPPORT
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
#ifndef WPA_SUPPLICANT_SUPPORT
#error "Build for being controlled by NetworkManager or wext, please set HAS_WPA_SUPPLICANT=y and HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=y"
#endif // WPA_SUPPLICANT_SUPPORT //
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //

#endif // CONFIG_STA_SUPPORT //

#ifdef IKANOS_VX_1X0
#include	"vr_ikans.h"
#endif // IKANOS_VX_1X0 //

#endif	// __RT_CONFIG_H__

