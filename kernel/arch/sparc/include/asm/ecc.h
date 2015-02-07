

#ifndef _SPARC_ECC_H
#define _SPARC_ECC_H

/* These registers are accessed through the SRMMU passthrough ASI 0x20 */
#define ECC_ENABLE     0x00000000       /* ECC enable register */
#define ECC_FSTATUS    0x00000008       /* ECC fault status register */
#define ECC_FADDR      0x00000010       /* ECC fault address register */
#define ECC_DIGNOSTIC  0x00000018       /* ECC diagnostics register */
#define ECC_MBAENAB    0x00000020       /* MBus arbiter enable register */
#define ECC_DMESG      0x00001000       /* Diagnostic message passing area */


#define ECC_MBAE_SBUS     0x00000010
#define ECC_MBAE_MOD3     0x00000008
#define ECC_MBAE_MOD2     0x00000004
#define ECC_MBAE_MOD1     0x00000002 

#define ECC_FCR_CHECK    0x00000002
#define ECC_FCR_INTENAB  0x00000001

#define ECC_FADDR0_MIDMASK   0xf0000000
#define ECC_FADDR0_S         0x08000000
#define ECC_FADDR0_VADDR     0x003fc000
#define ECC_FADDR0_BMODE     0x00002000
#define ECC_FADDR0_ATOMIC    0x00001000
#define ECC_FADDR0_CACHE     0x00000800
#define ECC_FADDR0_SIZE      0x00000700
#define ECC_FADDR0_TYPE      0x000000f0
#define ECC_FADDR0_PADDR     0x0000000f



#define ECC_FSR_C2ERR    0x00020000
#define ECC_FSR_MULT     0x00010000
#define ECC_FSR_SYND     0x0000ff00
#define ECC_FSR_DWORD    0x000000f0
#define ECC_FSR_UNC      0x00000008
#define ECC_FSR_TIMEO    0x00000004
#define ECC_FSR_BADSLOT  0x00000002
#define ECC_FSR_C        0x00000001

#endif /* !(_SPARC_ECC_H) */
