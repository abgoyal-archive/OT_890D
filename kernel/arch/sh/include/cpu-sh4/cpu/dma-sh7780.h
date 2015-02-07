
#ifndef __ASM_SH_CPU_SH4_DMA_SH7780_H
#define __ASM_SH_CPU_SH4_DMA_SH7780_H

#define REQ_HE	0x000000C0
#define REQ_H	0x00000080
#define REQ_LE	0x00000040
#define TM_BURST 0x0000020
#define TS_8	0x00000000
#define TS_16	0x00000008
#define TS_32	0x00000010
#define TS_16BLK	0x00000018
#define TS_32BLK	0x00100000

enum {
	XMIT_SZ_8BIT,
	XMIT_SZ_16BIT,
	XMIT_SZ_32BIT,
	XMIT_SZ_128BIT,
	XMIT_SZ_256BIT,
};

static unsigned int ts_shift[] __maybe_unused = {
	[XMIT_SZ_8BIT]		= 0,
	[XMIT_SZ_16BIT]		= 1,
	[XMIT_SZ_32BIT]		= 2,
	[XMIT_SZ_128BIT]	= 4,
	[XMIT_SZ_256BIT]	= 5,
};

#endif /* __ASM_SH_CPU_SH4_DMA_SH7780_H */
