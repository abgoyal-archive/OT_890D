

#ifndef __ASM_ARCH_MXC_BOARD_PCM038_H__
#define __ASM_ARCH_MXC_BOARD_PCM038_H__

/* mandatory for CONFIG_LL_DEBUG */

#define MXC_LL_UART_PADDR	UART1_BASE_ADDR
#define MXC_LL_UART_VADDR	(AIPI_BASE_ADDR_VIRT + 0x0A000)

#ifndef __ASSEMBLY__

extern void pcm970_baseboard_init(void);

#endif

#endif /* __ASM_ARCH_MXC_BOARD_PCM038_H__ */
