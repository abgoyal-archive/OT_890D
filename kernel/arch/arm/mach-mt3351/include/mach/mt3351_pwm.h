

#include <linux/fs.h>
#include <linux/ioctl.h>

#define RSUCCESS                        0
#define ESETPWMPINMUX                   1
#define ESETPWMDIR                      2
#define ESETPWMOCTL                     3
#define EMEMMODEHLDUR                   4
#define EPWMNODEV                       5
#define EPWMINVAL                       6

#define PWM_BACKLIGHT_MIN               0
#define PWM_BACKLIGHT_MAX               100
#define PWU_BACKLIGHT_VOLTAGE_MAX       19

#define PWM_DRIVER_MAGIC			    'T'
#define IOW_BACKLIGHT_OFF			    _IO(PWM_DRIVER_MAGIC, 2)
#define IOW_BACKLIGHT_ON			    _IO(PWM_DRIVER_MAGIC, 3)
#define IOW_BACKLIGHT_UPDATE		    _IOW(PWM_DRIVER_MAGIC, 9, unsigned)
#define IOR_BACKLIGHT_CURRENT		    _IOR(PWM_DRIVER_MAGIC, 11, unsigned)

#define PWM_ENABLE_MASK                 0x0000003F
#define PWM0_ENABLE_MASK                0x00000001
#define PWM0_ENABLE                     0x00000001
#define PWM0_DISABLE                    0x00000000
#define PWM1_ENABLE_MASK                0x00000002
#define PWM1_ENABLE                     0x00000002
#define PWM1_DISABLE                    0x00000000
#define PWM2_ENABLE_MASK                0x00000004
#define PWM2_ENABLE                     0x00000004
#define PWM2_DISABLE                    0x00000000
#define PWM3_ENABLE_MASK                0x00000008
#define PWM3_ENABLE                     0x00000008
#define PWM3_DISABLE                    0x00000000
#define PWM4_ENABLE_MASK                0x00000010
#define PWM4_ENABLE                     0x00000010
#define PWM4_DISABLE                    0x00000000
#define PWM5_ENABLE_MASK                0x00000020
#define PWM5_ENABLE                     0x00000020
#define PWM5_DISABLE                    0x00000000
#define PWM_SEQ_MODE_MASK               0x00000040
#define PWM_SEQ_MODE_ON                 0x00000040
#define PWM_SEQ_MODE_OFF                0x00000000

#define PWM_DELAY_DURATON_MASK          0x0000FFFF
#define PWM_DELAY_CLKSEL_MASK           0x00010000
#define PWM_DELAY_CLKSEL_52M            0x00000000
#define PWM_DELAY_CLKSEL_32K            0x00010000

#define PWM_CON_CLKDIV_SHIFT            0
#define PWM_CON_CLKDIV_MASK             0x00000007
#define PWM_CON_CLKDIV_1                0x00000000
#define PWM_CON_CLKDIV_2                0x00000001
#define PWM_CON_CLKDIV_4                0x00000010
#define PWM_CON_CLKDIV_8                0x00000011
#define PWM_CON_CLKDIV_16               0x00000100
#define PWM_CON_CLKDIV_32               0x00000101
#define PWM_CON_CLKDIV_64               0x00000110
#define PWM_CON_CLKDIV_128              0x00000111
#define PWM_CON_CLKSEL_SHIFT            3
#define PWM_CON_CLKSEL_MASK             0x00000008
#define PWM_CON_CLKSEL_52M              0x00000000
#define PWM_CON_CLKSEL_32K              0x00000008
#define PWM_CON_FIXED_CLKMODE_SHIFT     4
#define PWM_CON_FIXED_CLKMODE_MASK      0x00000010
#define PWM_CON_FIXED_CLKMODE_0         0x00000000
#define PWM_CON_FIXED_CLKMODE_1         0x00000010
#define PWM_CON_SRCSEL_SHIFT            5
#define PWM_CON_SRCSEL_MASK             0x00000020
#define PWM_CON_SRCSEL_FIFO             0x00000000
#define PWM_CON_SRCSEL_MEM              0x00000020
#define PWM_CON_MODE_SHIFT              6  
#define PWM_CON_MODE_MASK               0x00000040
#define PWM_CON_MODE_PERIODIC           0x00000000
#define PWM_CON_MODE_RANDOM             0x00000040
#define PWM_CON_IDLE_VALUE_SHIFT        7
#define PWM_CON_IDLE_VALUE_MASK         0x00000080
#define PWM_CON_IDLE_VALUE_0            0x00000000
#define PWM_CON_IDLE_VALUE_1            0x00000080
#define PWM_CON_GUARD_VALUE_SHIFT       8
#define PWM_CON_GUARD_VALUE_MASK        0x00000100
#define PWM_CON_GUARD_VALUE_0           0x00000000
#define PWM_CON_GUARD_VALUE_1           0x00000100
#define PWM_CON_STOP_BITPOS_SHIFT       9
#define PWM_CON_STOP_BITPOS_MASK        0x00007E00
#define PWM_CON_CLK_PWM_MODE_SHIFT      15
#define PWM_CON_CLK_PWM_MODE_MASK       0x00008000
#define PWM_CON_FIFO_MEM_PWM_MODE       0x00000000
#define PWM_CON_CLK_PWM_MODE            0x00008000

#define 	PWM_DATA_WIDTH_MASK         0x00003FFF

#define 	PWM_THRESH_MASK             0x00003FFF

#define PWM_INT_ENABLE_MASK             0x00000FFF

#define PWM0_INT_FINISH_EN_MASK         0x00000001
#define PWM0_INT_FINISH_DISABLE         0x00000000
#define PWM0_INT_FINISH_ENABLE          0x00000001
#define PWM0_INT_UNDERFLOW_EN_MASK      0x00000002
#define PWM0_INT_UNDERFLOW_DISABLE      0x00000000
#define PWM0_INT_UNDERFLOW_ENABLE       0x00000002

#define PWM1_INT_FINISH_EN_MASK         0x00000004
#define PWM1_INT_FINISH_DISABLE         0x00000000
#define PWM1_INT_FINISH_ENABLE          0x00000004
#define PWM1_INT_UNDERFLOW_EN_MASK      0x00000008
#define PWM1_INT_UNDERFLOW_DISABLE      0x00000000
#define PWM1_INT_UNDERFLOW_ENABLE       0x00000008

#define PWM2_INT_FINISH_EN_MASK         0x00000010
#define PWM2_INT_FINISH_DISABLE         0x00000000
#define PWM2_INT_FINISH_ENABLE          0x00000010
#define PWM2_INT_UNDERFLOW_EN_MASK      0x00000020
#define PWM2_INT_UNDERFLOW_DISABLE      0x00000000
#define PWM2_INT_UNDERFLOW_ENABLE       0x00000020

#define PWM3_INT_FINISH_EN_MASK         0x00000040
#define PWM3_INT_FINISH_DISABLE         0x00000000
#define PWM3_INT_FINISH_ENABLE          0x00000040
#define PWM3_INT_UNDERFLOW_EN_MASK      0x00000080
#define PWM3_INT_UNDERFLOW_DISABLE      0x00000000
#define PWM3_INT_UNDERFLOW_ENABLE       0x00000080

#define PWM4_INT_FINISH_EN_MASK         0x00000100
#define PWM4_INT_FINISH_DISABLE         0x00000000
#define PWM4_INT_FINISH_ENABLE          0x00000100
#define PWM4_INT_UNDERFLOW_EN_MASK      0x00000200
#define PWM4_INT_UNDERFLOW_DISABLE      0x00000000
#define PWM4_INT_UNDERFLOW_ENABLE       0x00000200

#define PWM5_INT_FINISH_EN_MASK         0x00000400
#define PWM5_INT_FINISH_DISABLE         0x00000000
#define PWM5_INT_FINISH_ENABLE          0x00000400
#define PWM5_INT_UNDERFLOW_EN_MASK      0x00000800
#define PWM5_INT_UNDERFLOW_DISABLE      0x00000000
#define PWM5_INT_UNDERFLOW_ENABLE       0x00000800

#define PWM_INT_STATUS_MASK             0x00000FFF
#define PWM0_INT_FINISH_EN_ST           0x00000001
#define PWM0_INT_UNDERFLOW_EN_ST        0x00000002
#define PWM1_INT_FINISH_EN_ST           0x00000004
#define PWM1_INT_UNDERFLOW_EN_ST        0x00000008
#define PWM2_INT_FINISH_EN_ST           0x00000010
#define PWM2_INT_UNDERFLOW_EN_ST        0x00000020
#define PWM3_INT_FINISH_EN_ST           0x00000040
#define PWM3_INT_UNDERFLOW_EN_ST        0x00000080
#define PWM4_INT_FINISH_EN_ST           0x00000100
#define PWM4_INT_UNDERFLOW_EN_ST        0x00000200
#define PWM5_INT_FINISH_EN_ST           0x00000400
#define PWM5_INT_UNDERFLOW_EN_ST        0x00000800

#define PWM_INT_ACK_MASK                0x00000FFF
#define PWM0_INT_FINISH_ACK             0x00000001
#define PWM0_INT_UNDERFLOW_ACK          0x00000002
#define PWM1_INT_FINISH_ACK             0x00000004
#define PWM1_INT_UNDERFLOW_ACK          0x00000008
#define PWM2_INT_FINISH_ACK             0x00000010
#define PWM2_INT_UNDERFLOW_ACK          0x00000020
#define PWM3_INT_FINISH_ACK             0x00000040
#define PWM3_INT_UNDERFLOW_ACK          0x00000080
#define PWM4_INT_FINISH_ACK             0x00000100
#define PWM4_INT_UNDERFLOW_ACK          0x00000200
#define PWM5_INT_FINISH_ACK             0x00000400
#define PWM5_INT_UNDERFLOW_ACK          0x00000800

#define PWM_FIFO_DATA_PATTERN1			0x55555555
#define PWM_FIFO_DATA_PATTERN2			0x00000000
#define PWM_FIFO_DATA_PATTERN3			0xFFFFFFFF

typedef enum 
{
	PWM0=0,
	PWM1,
	PWM2,
	PWM3,
	PWM4,
	PWM5,
    PWM_COUNT
}pwm_num_e;

typedef enum
{
   PWM_FIFO_MODE=0,
   PWM_MEMO_MODE,
   PWM_CLK_MODE,
   PWM_MODE_COUNT
} pwm_mode_e;


typedef enum 
{
	PWM_CLK_SEL_52M=0,
	PWM_CLK_SEL_32K
}pwm_clk_sel_e;

typedef enum 
{
	PWM_CLK_DIV_NONE=0,
	PWM_CLK_DIV_2=1,
	PWM_CLK_DIV_4=2,
	PWM_CLK_DIV_8=3,
	PWM_CLK_DIV_16=4,
	PWM_CLK_DIV_32=5,
	PWM_CLK_DIV_64=6,
	PWM_CLK_DIV_128=7
}pwm_clk_div_e;

#define   PWM_OUTPUT_LOW   0
#define   PWM_OUTPUT_HIGH  1

typedef struct {
   UINT32 data0;
   UINT32 data1;
   UINT16 repeat_count; /* 0 means endless repeat */
   UINT8  stop_bitpos;  /* 0~63*/
   UINT16 high_dur;     /* must>0. When set to N, the duration is N+1 clocks. */
   UINT16 low_dur;      /* must>0. When set to N, the duration is N+1 clocks. */
   UINT16 guard_dur;    /* must>0. When set to N, the duration is N+1 clocks. */
   UINT8  idle_output;  /* PWM_OUTPUT_LOW or PWM_OUTPUT_HIGH */  
                        //Boolean value actually.Myron.2008.0708
   UINT8  guard_output; /* PWM_OUTPUT_LOW or PWM_OUTPUT_HIGH */ 
                        //Boolean value actually.Myron.2008.0708
}pwm_fifo_para_s;

typedef struct {
   UINT32 *buf_addr;
   UINT16 buf_size;
   UINT16 repeat_count; /* 0 means endless repeat */
   INT8  stop_bitpos;   /* 0~31 in the last 32bits*/
   UINT16 high_dur;     /* must>0. When set to N, the duration is N+1 clocks. */
   UINT16 low_dur;      /* must>0. When set to N, the duration is N+1 clocks. */
   UINT16 guard_dur;    /* must>0. When set to N, the duration is N+1 clocks. */
   INT8  idle_output;   /* PWM_OUTPUT_LOW or PWM_OUTPUT_HIGH */
   INT8  guard_output;  /* PWM_OUTPUT_LOW or PWM_OUTPUT_HIGH */
}pwm_memo_para_s;

typedef struct {
   UINT16 data_width;
   UINT16 threshold;
   UINT16 repeat_count; /* 0 means endless repeat */
   UINT16 guard_dur;    /* must>0. When set to N, the duration is N+1 clocks. */
   UINT8  idle_output;  /* PWM_OUTPUT_LOW or PWM_OUTPUT_HIGH */
   UINT8  guard_output; /* PWM_OUTPUT_LOW or PWM_OUTPUT_HIGH */
}pwm_clk_para_s;


typedef enum
{
   PWM_SEQ_EN_PWM2=0x1,
   PWM_SEQ_EN_PWM23=0x03,
   PWM_SEQ_EN_PWM24=0x05,
   PWM_SEQ_EN_PWM234=0x07,
   PWM_SEQ_EN_PWM25=0x09,
   PWM_SEQ_EN_PWM235=0x0b,
   PWM_SEQ_EN_PWM245=0x0d,
   PWM_SEQ_EN_PWM2345=0x0f
} pwm_seq_en_cnt_e;

typedef enum
{
	PWM_INT_TYPE_FINISH,
	PWM_INT_TYPE_UNDERFLOW
}pwm_int_type_e;


void pwm_start(u32 pwm_num);
void pwm_stop(u32 pwm_num);
void pwm_seq_start(pwm_seq_en_cnt_e sequence);
void pwm_seq_stop(void);
void pwm_set_seq_delay(u32 pwm_num, u32 delay);
u32 pwm_configure(u32 pwm_num, u32 mode, void *para);
void pwm_enable_interrupt(u32 pwm_num, pwm_int_type_e int_type, bool is_enable);
void pwm_clk_init(u32 pwm_num, u32 clk_sel, u32 clk_div);
