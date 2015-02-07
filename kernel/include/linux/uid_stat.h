

#ifndef __uid_stat_h
#define __uid_stat_h

/* Contains definitions for resource tracking per uid. */

extern int update_tcp_snd(uid_t uid, int size);
extern int update_tcp_rcv(uid_t uid, int size);

#endif /* _LINUX_UID_STAT_H */
