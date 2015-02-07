
#ifndef __ASMH8300_ELF_H
#define __ASMH8300_ELF_H


#include <asm/ptrace.h>
#include <asm/user.h>

typedef unsigned long elf_greg_t;

#define ELF_NGREG (sizeof(struct user_regs_struct) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];
typedef unsigned long elf_fpregset_t;

#define elf_check_arch(x) ((x)->e_machine == EM_H8_300)

#define ELF_CLASS	ELFCLASS32
#define ELF_DATA	ELFDATA2MSB
#define ELF_ARCH	EM_H8_300
#if defined(__H8300H__)
#define ELF_CORE_EFLAGS 0x810000
#endif
#if defined(__H8300S__)
#define ELF_CORE_EFLAGS 0x820000
#endif

#define ELF_PLAT_INIT(_r)	_r->er1 = 0

#define USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE	4096


#define ELF_ET_DYN_BASE         0xD0000000UL


#define ELF_HWCAP	(0)


#define ELF_PLATFORM  (NULL)

#define SET_PERSONALITY(ex) set_personality(PER_LINUX)

#define R_H8_NONE       0
#define R_H8_DIR32      1
#define R_H8_DIR32_28   2
#define R_H8_DIR32_24   3
#define R_H8_DIR32_16   4
#define R_H8_DIR32U     6
#define R_H8_DIR32U_28  7
#define R_H8_DIR32U_24  8
#define R_H8_DIR32U_20  9
#define R_H8_DIR32U_16 10
#define R_H8_DIR24     11
#define R_H8_DIR24_20  12
#define R_H8_DIR24_16  13
#define R_H8_DIR24U    14
#define R_H8_DIR24U_20 15
#define R_H8_DIR24U_16 16
#define R_H8_DIR16     17
#define R_H8_DIR16U    18
#define R_H8_DIR16S_32 19
#define R_H8_DIR16S_28 20
#define R_H8_DIR16S_24 21
#define R_H8_DIR16S_20 22
#define R_H8_DIR16S    23
#define R_H8_DIR8      24
#define R_H8_DIR8U     25
#define R_H8_DIR8Z_32  26
#define R_H8_DIR8Z_28  27
#define R_H8_DIR8Z_24  28
#define R_H8_DIR8Z_20  29
#define R_H8_DIR8Z_16  30
#define R_H8_PCREL16   31
#define R_H8_PCREL8    32
#define R_H8_BPOS      33
#define R_H8_PCREL32   34
#define R_H8_GOT32O    35
#define R_H8_GOT16O    36
#define R_H8_DIR16A8   59
#define R_H8_DIR16R8   60
#define R_H8_DIR24A8   61
#define R_H8_DIR24R8   62
#define R_H8_DIR32A16  63
#define R_H8_ABS32     65
#define R_H8_ABS32A16 127

#endif
