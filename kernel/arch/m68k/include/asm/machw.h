

#ifndef _ASM_MACHW_H_
#define _ASM_MACHW_H_


#define VIDEOMEMBASE	0xf0000000
#define VIDEOMEMSIZE	(4096*1024)
#define VIDEOMEMMASK	(-4096*1024)

#ifndef __ASSEMBLY__

#include <linux/types.h>

#if 0

#define MAC_SCC_BAS (0x50F04000)
struct MAC_SCC
 {
  u_char cha_a_ctrl;
  u_char char_dummy1;
  u_char cha_a_data;
  u_char char_dummy2;
  u_char cha_b_ctrl;
  u_char char_dummy3;
  u_char cha_b_data;
 };
# define mac_scc ((*(volatile struct SCC*)MAC_SCC_BAS))
#endif

#endif /* __ASSEMBLY__ */

#endif /* linux/machw.h */
