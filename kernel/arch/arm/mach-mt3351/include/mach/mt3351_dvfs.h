


#ifndef _DVFS_H
#define _DVFS_H


#define 	DVFS_CON   	                  (DVFS_BASE+0x0000)
#define 	DVFS_STA 	                  (DVFS_BASE+0x0004)
#define 	DVFS_INT 	                  (DVFS_BASE+0x0008)
#define 	DVFS_CLK_TMR  	              (DVFS_BASE+0x000C)
#define 	DVFS_PMU_TMR                  (DVFS_BASE+0x0010)
                                         
/*DVFS_CON*/
#define 	DVFS_UP	                      0x0001
#define 	DVFS_DOWN		              0x0002
#define 	DVFS_IRQ_EN                   0x0004
#define 	DVFS_ASYNC                    0x0010
#define 	DVFS_CKDIV_AU		          0x0040
#define 	DVFS_PMU_AU		              0x0080


/*DVFS_STA*/
#define 	DVFS_INT_status               0x0001
#define 	DVFS_BUSY	                  0x0100

/*DVFS_INT*/
#define 	DVFS_IRQ_ACK	              0x0001

/*DVFS_CLK_TMR*/
#define 	DVFS_CLK_TIMEOUT_MASK	      0xFF

/*DVFS_PMIC_TMR*/
#define 	DVFS_PMU_TIMEOUT_MASK	      0xFFFF

#define SCALE_UP		TRUE
#define SCALE_DOWN		FALSE

// CONFIG Register for DVFS : CLKCON
#define DVFS_UPD			              0x80000000
#define CLK_DLY_MASK		              0x700000
#define EMI_CLKDIV_MASK		              0xC0000		
#define ARM_CLKDIV_MASK                   0x30000
#define MMCKSRC_MASK				      0xC0
#define ARMCKSRC_MASK                     0x30



typedef enum
{
	ARM_104_or_117 = 10,
	ARM_208_or_234,
	ARM_416_or_468,
}ARM_FREQUENCY;

typedef enum
{
	CLK_DLY_0 = 0x00000000,
	CLK_DLY_1 = 0x00100000,
	CLK_DLY_2 = 0x00200000,
	CLK_DLY_3 = 0x00300000,
	CLK_DLY_4 = 0x00400000,
	CLK_DLY_5 = 0x00500000,	
	CLK_DLY_6 = 0x00600000,	
	CLK_DLY_7 = 0x00700000,
}CLK_DLY;



typedef enum 
{
	DVFS_STATUS_OK = 0,
	DVFS_STATUS_FAIL
}DVFS_STATUS;

#endif  




