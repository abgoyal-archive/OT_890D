

#ifndef __ASM_ARCH_NAND_H
#define __ASM_ARCH_NAND_H

struct mxc_nand_platform_data {
	int width;	/* data bus width in bytes */
	int hw_ecc;	/* 0 if supress hardware ECC */
};
#endif /* __ASM_ARCH_NAND_H */
