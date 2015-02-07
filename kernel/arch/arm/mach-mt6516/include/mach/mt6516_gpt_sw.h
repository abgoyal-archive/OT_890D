

#ifndef _MTK_MT6516_XGPT_SW_H
#define _MTK_MT6516_XGPT_SW_H

#include <mach/mt6516_typedefs.h>

#include <asm/tcm.h>

//The operation mode of XGPT n
typedef enum
{
    XGPT_ONE_SHOT = 0x0000,
    XGPT_REPEAT   = 0x0010,
    XGPT_KEEP_GO  = 0x0020,
    XGPT_FREE_RUN = 0x0030
} XGPT_CON_MODE;

//The operation mode of GPT n
typedef enum
{
    GPT_ONE_SHOT = 0x0000,
    GPT_REPEAT   = 0x4000,
} GPT_CON_MODE;

//XGPT n input clock frequency.
typedef enum
{
    XGPT_CLK_DIV_1   = 0x0000,
    XGPT_CLK_DIV_2   = 0x0001,
    XGPT_CLK_DIV_4   = 0x0002,
    XGPT_CLK_DIV_8   = 0x0003,
    XGPT_CLK_DIV_16  = 0x0004,
    XGPT_CLK_DIV_32  = 0x0005,
    XGPT_CLK_DIV_64  = 0x0006,
    XGPT_CLK_DIV_128 = 0x0007
} XGPT_CLK_DIV;

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

typedef enum
{
    XGPT1 = 0,
    XGPT2,
    XGPT3,
    XGPT4,
    XGPT5,
    XGPT6,
    XGPT7,    
    XGPT_TOTAL_COUNT
} XGPT_NUM;

typedef enum
{
    GPT1 = 0,
    GPT2,
    GPT3,
    GPT_TOTAL_COUNT
} GPT_NUM;

typedef enum
{
    USED = 0,
    NOT_USED
} Status;

typedef struct 
{
    void (*xgpt2_func)(UINT16);
    void (*xgpt3_func)(UINT16);
    void (*xgpt4_func)(UINT16);
    void (*xgpt5_func)(UINT16);
    void (*xgpt6_func)(UINT16);
    void (*xgpt7_func)(UINT16);       
} XGPT_Func;

typedef struct 
{
    void (*gpt1_func)(UINT16);
    void (*gpt2_func)(UINT16);
} GPT_Func;

typedef struct
{
    XGPT_NUM num;          //XGPT2~7
    XGPT_CON_MODE mode;    //
    XGPT_CLK_DIV clkDiv;
    BOOL bIrqEnable;
    UINT32 u4Compare;
} XGPT_CONFIG;

typedef struct
{
    GPT_NUM num;          //GPT1~2
    GPT_CON_MODE mode;    //
    GPT_CLK_DIV clkDiv;
    UINT32 u4Timeout;
} GPT_CONFIG;

  
void XGPT_EnableIRQ(XGPT_NUM);
void XGPT_DisableIRQ(XGPT_NUM);
XGPT_NUM __tcmfunc XGPT_Get_IRQSTA(void);
BOOL __tcmfunc XGPT_Check_IRQSTA(XGPT_NUM numGPT);
void XGPT_Init(XGPT_NUM timerNum, void (*XGPT_Callback)(UINT16));
void XGPT_Reset(XGPT_NUM numGPT);
void __tcmfunc XGPT_LISR(void);
void __tcmfunc XGPT_AckIRQ(XGPT_NUM);
void XGPT_Start(XGPT_NUM);
void XGPT_Restart(XGPT_NUM);
void XGPT_Stop(XGPT_NUM);
void XGPT_ClearCount(XGPT_NUM);
void XGPT_SetOpMode(XGPT_NUM, XGPT_CON_MODE);
XGPT_CON_MODE XGPT_GetOpMode(XGPT_NUM);
void XGPT_SetClkDivisor(XGPT_NUM, XGPT_CLK_DIV);
XGPT_CLK_DIV XGPT_GetClkDivisor(XGPT_NUM);
UINT32 XGPT_GetCounter(XGPT_NUM);
void XGPT_SetCompare(XGPT_NUM, UINT32);
BOOL XGPT_Config(XGPT_CONFIG config);


void GPT_Init(GPT_NUM timerNum, void (*GPT_Callback)(UINT16));
void GPT_Start(GPT_NUM);
void GPT_Stop(GPT_NUM);
void GPT_SetOpMode(GPT_NUM, GPT_CON_MODE);
GPT_CON_MODE GPT_GetOpMode(GPT_NUM);
void GPT_SetClkDivisor(GPT_NUM, GPT_CLK_DIV);
GPT_CLK_DIV GPT_GetClkDivisor(GPT_NUM);
UINT32 GPT_GetTimeout(GPT_NUM);
void GPT_SetTimeout(GPT_NUM, UINT32);
BOOL GPT_Config(GPT_CONFIG config);

BOOL XGPT_IsStart(XGPT_NUM numXGPT);

#endif
