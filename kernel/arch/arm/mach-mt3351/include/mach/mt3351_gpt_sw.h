

#ifndef _MTK_GPT_SW_H
#define _MTK_GPT_SW_H

#include <mach/mt3351_typedefs.h>

//The operation mode of GPT n
typedef enum
{
    GPT_ONE_SHOT = 0x0000,
    GPT_REPEAT   = 0x0010,
    GPT_KEEP_GO  = 0x0020,
    GPT_FREE_RUN = 0x0030
} GPT_CON_MODE;

//GPT n input clock frequency.
typedef enum
{
    GPT_CLK_DIV_1   = 0x0000,
    GPT_CLK_DIV_2   = 0x0001,
    GPT_CLK_DIV_4   = 0x0002,
    GPT_CLK_DIV_8   = 0x0003,
    GPT_CLK_DIV_16  = 0x0004,
    GPT_CLK_DIV_32  = 0x0005,
    GPT_CLK_DIV_64  = 0x0006,
    GPT_CLK_DIV_128 = 0x0007
} GPT_CLK_DIV;

//The clock source of GPT n
typedef enum
{
    GPT_CLK_NONE = 0,     // turn off the gpt clcok
    GPT_CLK_SYS_CRYSTAL,  // system clock : 13M or 26M Hz
    GPT_CLK_APB_BUS,      // apb bus
    GPT_CLK_RTC           // 32768Hz
} GPT_CLK_SOURCE;

typedef enum
{
    GPT0 = 0,
    GPT1,
    GPT2,
    GPT3,
    GPT4,
    GPT5,
    GPT_TOTAL_COUNT
}GPT_NUM;

typedef struct 
{
    void (*gpt0_func)(UINT16);
    void (*gpt1_func)(UINT16); 
    void (*gpt2_func)(UINT16); 
    void (*gpt3_func)(UINT16);
    void (*gpt4_func)(UINT16); 
    void (*gpt5_func)(UINT16);    
}GPT_Func;

typedef struct
{
    GPT_NUM num;          //GPT1~5
    GPT_CON_MODE mode;    //
    GPT_CLK_SOURCE clkSrc;
    GPT_CLK_DIV clkDiv;
    BOOL bIrqEnable;
    UINT32 u4CompareL;
    UINT32 u4CompareH;
} GPT_CONFIG;


BOOL GPT_Config(GPT_CONFIG config);
void GPT_EnableIRQ(GPT_NUM);
void GPT_DisableIRQ(GPT_NUM);
GPT_NUM GPT_Get_IRQSTA(void);
BOOL GPT_Check_IRQSTA(GPT_NUM numGPT);
void GPT_Init(GPT_NUM timerNum, void (*GPT_Callback)(UINT16));
void GPT_Reset(GPT_NUM numGPT);
void GPT_LISR(void);
void GPT_AckIRQ(GPT_NUM);
void GPT_Start(GPT_NUM);
void GPT_Stop(GPT_NUM);
void GPT_ClearCount(GPT_NUM);
void GPT_SetOpMode(GPT_NUM, GPT_CON_MODE); // set operation mode
GPT_CON_MODE GPT_GetOpMode(GPT_NUM);
void GPT_SetClkDivisor(GPT_NUM, GPT_CLK_DIV);
GPT_CLK_DIV GPT_GetClkDivisor(GPT_NUM);
void GPT_SetClkSource(GPT_NUM, GPT_CLK_SOURCE);
GPT_CLK_SOURCE GPT_GetClkSource(GPT_NUM);
UINT32 GPT_GetCounterL32(GPT_NUM);
UINT32 GPT_GetCounterH32(GPT_NUM);
void GPT_SetCompareL32(GPT_NUM, UINT32);
void GPT_SetCompareH32(GPT_NUM, UINT32);
UINT32 GPT_GetCompareL32(GPT_NUM);
UINT32 GPT_GetCompareH32(GPT_NUM);

#endif
