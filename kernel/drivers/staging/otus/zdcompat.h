
/*                                                                      */
/*  Module Name : zdcompat.h                                            */
/*                                                                      */
/*  Abstract                                                            */
/*     This module contains function defintion for compatibility.       */
/*                                                                      */
/*  NOTES                                                               */
/*     Platform dependent.                                              */
/*                                                                      */
/************************************************************************/

#ifndef _ZDCOMPAT_H
#define _ZDCOMPAT_H


#ifndef DECLARE_TASKLET
#define tasklet_schedule(a)   schedule_task(a)
#endif

#undef netdevice_t
typedef struct net_device netdevice_t;

#ifdef WIRELESS_EXT
#if (WIRELESS_EXT < 13)
struct iw_request_info
{
        __u16           cmd;            /* Wireless Extension command */
        __u16           flags;          /* More to come ;-) */
};
#endif
#endif

#ifndef IRQ_NONE
typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#define IRQ_RETVAL(x)
#endif

#ifndef in_atomic
#define in_atomic()  0
#endif

#define USB_QUEUE_BULK 0


#endif
