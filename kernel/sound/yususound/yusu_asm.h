




 #ifndef _YUSU_ASM_H
 #define _YUSU_ASM_H

#define ASM_REGS_BASE                   (ASM_BASE) //this is physical address
#define ASM_REGS_END                    (ASM_BASE + 0x320)
#define ASM_OFFSET                      (0x320)
#define ASM_BLOCK_LENGTH        (0x20)
#define ASM_MEMORY_CONTROL      (CONFIG_BASE+0x30c) //this is physical address


#define ASM_MIXER_LINE_MAX                  (3)
#define ASM_EQ_NUM_MAX                      (45)
#define ASM_MASK_ALL                             (0xffffffff)

#define ASM_BLOCK_0 				(1<<0)
#define ASM_BLOCK_1				(1<<1)
#define ASM_BLOCK_2 				(1<<2)
#define EDI_BLOCK 				         (1<<3)
#define EDI_SHIFT_BITS 				 (3)


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum  {
	ASM_IBUF_BASE0  = 0x000,          // 0x000 ~ 0x004
	ASM_IBUF_END0   = 0x004,          // 0x004 ~ 0x008
	ASM_IBUF_CUR0   = 0x008,          // 0x008 ~ 0x00C

	ASM_SET0        = 0x010,          // 0x010 ~ 0x014
	ASM_GAIN0       = 0x014,          // 0x014 ~ 0x018
	ASM_IR_CNT0     = 0x018,          // 0x018 ~ 0x01C

	ASM_IBUF_BASE1  = 0x020,          // 0x020 ~ 0x024
	ASM_IBUF_END1   = 0x024,          // 0x024 ~ 0x028
	ASM_IBUF_CUR1   = 0x028,          // 0x028 ~ 0x02C

	ASM_SET1        = 0x030,          // 0x030 ~ 0x034
	ASM_GAIN1       = 0x034,          // 0x034 ~ 0x038
	ASM_IR_CNT1     = 0x038,          // 0x038 ~ 0x03C

	ASM_IBUF_BASE2  = 0x040,          // 0x040 ~ 0x044
	ASM_IBUF_END2   = 0x044,          // 0x044 ~ 0x048
	ASM_IBUF_CUR2   = 0x048,          // 0x048 ~ 0x04C

	ASM_SET2        = 0x050,          // 0x050 ~ 0x054
	ASM_GAIN2       = 0x054,          // 0x054 ~ 0x058
	ASM_IR_CNT2 	= 0x058,          // 0x058 ~ 0x05c

	ASM_CON0        = 0x100,          // 0x100 ~ 0x104
	ASM_CON1        = 0x104,          // 0x104 ~ 0x108

	ASM_IR_STATUS   = 0x108,          // 0x108 ~ 0x10c
	ASM_IR_CLR      = 0x10c,          // 0x10c ~ 0x110
	ASM_GMC_READ    = 0x110,          // 0x110 ~ 0x114

	ASM_CHECK_SUM   = 0x120,          // 0x120 ~ 0x124
	ASM_SIN_CON     = 0x124,          // 0x124 ~ 0x128

	ASM_AFE_CON0    = 0x200,          // 0x200 ~ 0x204
	ASM_AFE_CON1    = 0x204,          // 0x204 ~ 0x208
	ASM_AFE_CON2    = 0x208,          // 0x208 ~ 0x20c

	AFE_CHECKSUM    = 0x210,          // 0x210 ~ 0x214
	AFE_EQCOEF0    =  0x240,          // 0x240 ~ 0x243

	AFE_EDI_CON      = 0x300,          //  0x300 ~ 0x304
	ASM_EDI_BASE    = 0x304,          //  0x304 ~ 0x308
	ASM_EDI_END      = 0x308,          //  0x308 ~ 0x30c
	ASM_EDI_CON1    = 0x30c,          //  0x30c ~ 0x310
	ASM_EDI_IR_CNT  = 0x310,          //  0x310 ~ 0x314
	ASM_EBIT_RDATA  = 0x314,          //  0x314 ~ 0x318
	ASM_EDI_OUTPUT_MONITOR = 0x318,   //  0x318 ~ 0x31B
	ASM_EDI_INPUT_CURSU    = 0x31c    //  0x31C ~ 0x320
} ASM_REGS;

void Asm_Set(UINT32 offset,UINT32 value,UINT32 mask);
UINT32 Asm_Get(UINT32 offset);
void Asm_Enable_Memory_Power(void);
void Asm_Disable_Memory_Power(void);


 #endif


