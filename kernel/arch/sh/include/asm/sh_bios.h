
#ifndef __ASM_SH_BIOS_H
#define __ASM_SH_BIOS_H



extern void sh_bios_console_write(const char *buf, unsigned int len);
extern void sh_bios_char_out(char ch);
extern void sh_bios_gdb_detach(void);

extern void sh_bios_get_node_addr(unsigned char *node_addr);
extern void sh_bios_shutdown(unsigned int how);

#endif /* __ASM_SH_BIOS_H */
