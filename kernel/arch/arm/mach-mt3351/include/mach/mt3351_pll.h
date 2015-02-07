
#ifndef _PLL_H
#define _PLL_H

#include <mach/mt3351_typedefs.h>


#define 	MPLL_CON   	                     (PLL_BASE+0x0000)
#define 	APLL_CON0 	                     (PLL_BASE+0x0018)
#define 	APLL_CON1 	                  (PLL_BASE+0x001C)
#define 	LPLL_CON0  	                  (PLL_BASE+0x0020)
#define 	LPLL_CON1                 (PLL_BASE+0x0024)
#define 	CLK_CON                 (PLL_BASE+0x0028)
#define 	PDN_CON                 (PLL_BASE+0x002C)
#define 	UPLL_CON                 (PLL_BASE+0x0060)

                                         
/*MPLL_CON*/
#define MPLL_TESTMODE_MASK			0xE000
#define MPLL_CALI_MASK				0x1F00
#define MPLL_RST				0x0080
#define LDO_TUNE_VOLT_MASK		0x0060
#define LDO_FORCE_DIS			0x0010
#define MPLL_DIV_CTRL_MASK		0x000F
typedef enum
{
	LDO_OUT_1200 = 0,
	LDO_OUT_1250,
	LDO_OUT_1300,
	LDO_OUT_1150
}LDO_OUT_VOL;

/*APLL_CON0*/
#define APPLL_RST				0x0800
#define APPLL_DIV_CTRL_MASK		0x07E0
#define APPLL_CALI_MASK			0x001F

typedef enum
{
	APLL_312 = 10,
	APLL_338,
	APLL_364,
	APLL_390,
	APLL_416,
	APLL_442,
	APLL_468,
	APLL_494,
	APLL_520
}APLL_FREQ;

#define EXT_26MHZ 			26000000
#define MPLL_208MHZ 		208000000
#define UPLL_48MHZ 			48000000
#define APLL_312MHZ			312000000
#define APLL_338MHZ			338000000
#define APLL_364MHZ			364000000
#define APLL_390MHZ			390000000
#define APLL_416MHZ			416000000
#define APLL_442MHZ			442000000
#define APLL_468MHZ			468000000
#define APLL_494MHZ			494000000
#define APLL_520MHZ			520000000



/*APLL_CON1*/
#define APPLL_D2SPWX2			0x8000
#define APPLL_DCCPWX2			0x4000
#define APPLL_DIRECT_EN			0x2000
#define APPLL_DUTY_CHECK_MASK		0x0600
typedef enum
{
	CHECK_DISABLE = 0,
	CHECK_POSITIVE,
	CHECK_NEGAIVE,
	CHECK_RESERVED
}APLL_DUTY_CHECK;

#define APLL_DCC_EN			0x0100
#define APLL_TESTMODE_MASK		0x000E			
#define APLL_PWDB				0x0001

/*LPLL_CON0*/
#define LPLL_DIGDIV_MASK		0xE000
#define LPLL_CALI				0x1F00
#define LPLL_RST				0x0080
#define LPLL_DIV_CTRL_MASK		0x003F


/*LPLL_CON1*/
#define LPLL_TESTMODE			0xE000
#define LPLL_OUT_DIV_MASK		0x0700
#define LPLL_OUT_SEL			0x0070
#define LPLL_PWDB				0x0001


/*CLK_CON*/
#define TESTMODE_EN				0x8000
#define PLL_CLKTEST				0x0300
#define XTAL_PW_CTR				0x0030
#define XTAL_DIV2_SEL			0x0001

/*PDN_CON*/
#define UPLL_PWDB				0x8000
#define MPLL_PWDB				0x2000
#define XTAL_DIV2_PWDB			0x1000
#define XTAL_PWDB				0x0800

/*UPLL_CON*/
#define UPLL_TESTMODE_MASK		0xE000	
#define UPLL_CALI_MASK			0x1F00
#define UPLL_RST				0x0800
#define UPLL_CK2_SEL			0x0010
#define UPLL_DIV_CTRL_MASK		0x000F

/* Pangyen, 20090131 { */
/* UPLL_DIV_CTRL: for UPLL division ratio control*/
#define UPLL_DIV_CTRL_48MHZ     0x000a
/* Pangyen, 20090131 } */

typedef enum
{
    EMI_1X_PLL_13MHZ = 13,
    EMI_1X_PLL_52MHZ = 52,
    EMI_1X_PLL_78MHZ = 78,
    EMI_1X_PLL_104MHZ = 104,
/* Infinity, 20080905 { */
    EMI_1X_PLL_117MHZ = 117,
    EMI_1X_PLL_208MHZ = 208,
    EMI_1X_PLL_416MHZ = 416,
    EMI_1X_PLL_468MHZ = 468
/* Infinity, 20080905 } */
} EMI_1X_PLL;

// Functions

void APLL_Power_UP(kal_bool enable);
void APLL_Reset(void);
void LPLL_Power_UP(kal_bool enable);
void LPLL_Reset(void);
void MPLL_Power_UP(kal_bool enable);
void MPLL_Reset(void);
void UPLL_Power_UP(kal_bool enable);
void UPLL_Reset(void);

void MT3351_PLL_Init(void);

#endif  



