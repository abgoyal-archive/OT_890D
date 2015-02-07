
#ifndef _ASM_FPU_EMULATOR_H
#define _ASM_FPU_EMULATOR_H

#include <asm/break.h>
#include <asm/inst.h>

struct mips_fpu_emulator_stats {
	unsigned int emulated;
	unsigned int loads;
	unsigned int stores;
	unsigned int cp1ops;
	unsigned int cp1xops;
	unsigned int errors;
};

extern struct mips_fpu_emulator_stats fpuemustats;

extern int mips_dsemul(struct pt_regs *regs, mips_instruction ir,
	unsigned long cpc);
extern int do_dsemulret(struct pt_regs *xcp);

#define BD_COOKIE 0x0000bd36	/* tne $0, $0 with baggage */

#define BREAK_MATH (0x0000000d | (BRK_MEMU << 16))

#endif /* _ASM_FPU_EMULATOR_H */
