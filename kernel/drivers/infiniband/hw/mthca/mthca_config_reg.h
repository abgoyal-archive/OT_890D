

#ifndef MTHCA_CONFIG_REG_H
#define MTHCA_CONFIG_REG_H

#include <asm/page.h>

#define MTHCA_HCR_BASE         0x80680
#define MTHCA_HCR_SIZE         0x0001c
#define MTHCA_ECR_BASE         0x80700
#define MTHCA_ECR_SIZE         0x00008
#define MTHCA_ECR_CLR_BASE     0x80708
#define MTHCA_ECR_CLR_SIZE     0x00008
#define MTHCA_MAP_ECR_SIZE     (MTHCA_ECR_SIZE + MTHCA_ECR_CLR_SIZE)
#define MTHCA_CLR_INT_BASE     0xf00d8
#define MTHCA_CLR_INT_SIZE     0x00008
#define MTHCA_EQ_SET_CI_SIZE   (8 * 32)

#endif /* MTHCA_CONFIG_REG_H */
