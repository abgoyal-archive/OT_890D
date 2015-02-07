

#include <linux/fs.h>
#include <linux/ioctl.h>
#include <cust_gpio_usage.h>
/*----------------------------------------------------------------------------*/
//#define GPIO_INIT_DEBUG
//#define GPIO_INIT_PINMUX      /*the configuration should be always off because it should be replaced by mt6516_init_gpio()*/
/*----------------------------------------------------------------------------*/
#define GPIO_DEVNAME                "mtgpio"
#define GPIO_CLSNAME                "gpio"
/*----------------------------------------------------------------------------*/
//  Error Code No.
#define RSUCCESS        0
#define EEXCESSPINNO    1
#define EBADDIR         2
#define EBADINOUTVAL    3
#define EEXCESSCURRENT  4
#define EBADDEBUGVAL    5
#define ERRVAL          6
#define EBADOCFGFIELD   7
#define EBADICFGFIELD   8
#define EBADMUXFIELD    9
#define EBADPULLSELECT  10
#define EEXCESSCLKOUT   11
#define EBADCLKSRC      12
/*----------------------------------------------------------------------------*/
#define  MAX_GPIO_PIN           147
#define  MAX_GPIO_REG_BITS      16
#define  MAX_GPIO_MODE_PER_REG  8
typedef enum GPIO_PIN
{    
    GPIO_UNSUPPORTED = -1,    
        
    GPIO0,
    GPIO1,
    GPIO2,
    GPIO3,
    GPIO4,
    GPIO5,
    GPIO6,
    GPIO7,
    GPIO8,
    GPIO9,
    GPIO10,
    GPIO11,
    GPIO12,
    GPIO13,
    GPIO14,
    GPIO15,
    GPIO16,
    GPIO17,
    GPIO18,
    GPIO19,
    GPIO20,
    GPIO21,
    GPIO22,
    GPIO23,
    GPIO24,
    GPIO25,
    GPIO26,
    GPIO27,
    GPIO28,
    GPIO29,
    GPIO30,
    GPIO31,
    GPIO32,
    GPIO33,
    GPIO34,
    GPIO35,
    GPIO36,
    GPIO37,
    GPIO38,
    GPIO39,
    GPIO40,
    GPIO41,
    GPIO42,
    GPIO43,
    GPIO44,
    GPIO45,
    GPIO46,
    GPIO47,
    GPIO48,
    GPIO49,
    GPIO50,
    GPIO51,
    GPIO52,
    GPIO53,
    GPIO54,
    GPIO55,
    GPIO56,
    GPIO57,
    GPIO58,
    GPIO59,
    GPIO60,
    GPIO61,
    GPIO62,
    GPIO63,    
    GPIO64,
    GPIO65,
    GPIO66,
    GPIO67,
    GPIO68,
    GPIO69,
    GPIO70,
    GPIO71,
    GPIO72,
    GPIO73,
    GPIO74,
    GPIO75,
    GPIO76,
    GPIO77,
    GPIO78,
    GPIO79,
    GPIO80,
    GPIO81,
    GPIO82,
    GPIO83,
    GPIO84,
    GPIO85,
    GPIO86,
    GPIO87,
    GPIO88,
    GPIO89,
    GPIO90,
    GPIO91,
    GPIO92,
    GPIO93,
    GPIO94,
    GPIO95,
    GPIO96,
    GPIO97,
    GPIO98,
    GPIO99,
    GPIO100,
    GPIO101,
    GPIO102,
    GPIO103,
    GPIO104,
    GPIO105,
    GPIO106,
    GPIO107,
    GPIO108,
    GPIO109,
    GPIO110,
    GPIO111,
    GPIO112,
    GPIO113,
    GPIO114,
    GPIO115,
    GPIO116,
    GPIO117,
    GPIO118,
    GPIO119,
    GPIO120,
    GPIO121,
    GPIO122,
    GPIO123,
    GPIO124,
    GPIO125,
    GPIO126,
    GPIO127,
    GPIO128,
    GPIO129,
    GPIO130,
    GPIO131,
    GPIO132,
    GPIO133,
    GPIO134,
    GPIO135,
    GPIO136,
    GPIO137,
    GPIO138,
    GPIO139,
    GPIO140,
    GPIO141,
    GPIO142,
    GPIO143,
    GPIO144,
    GPIO145,
    GPIO146,
    
    GPIO_MAX
}GPIO_PIN;         

/* GPIO MODE CONTROL VALUE*/
typedef enum {
    GPIO_MODE_GPIO  = 0,
    GPIO_MODE_00    = 0,
    GPIO_MODE_01    = 1,
    GPIO_MODE_02    = 2,
    GPIO_MODE_03    = 3,
    GPIO_MODE_DEFAULT = GPIO_MODE_01,
} GPIO_MODE;

/* GPIO DIRECTION */
typedef enum {
    GPIO_DIR_IN     = 0,
    GPIO_DIR_OUT    = 1,
    GPIO_DIR_DEFAULT = GPIO_DIR_IN,
} GPIO_DIR;

/* GPIO PULL ENABLE*/
typedef enum {
    GPIO_PULL_DISABLE = 0,
    GPIO_PULL_ENABLE  = 1,
    GPIO_PULL_EN_DEFAULT = GPIO_PULL_ENABLE,
} GPIO_PULL_EN;

/* GPIO PULL-UP/PULL-DOWN*/
typedef enum {
    GPIO_PULL_DOWN  = 0,
    GPIO_PULL_UP    = 1,
    GPIO_PULL_DEFAULT = GPIO_PULL_DOWN
} GPIO_PULL;

/* GPIO INVERSION */
typedef enum {
    GPIO_DATA_UNINV = 0,
    GPIO_DATA_INV   = 1,
    GPIO_DATA_INV_DEFAULT = GPIO_DATA_UNINV
} GPIO_INVERSION;

/* GPIO OUTPUT */
typedef enum {
    GPIO_OUT_ZERO = 0,
    GPIO_OUT_ONE  = 1,
    GPIO_OUT_DEFAULT = GPIO_OUT_ZERO,
    GPIO_DATA_OUT_DEFAULT = GPIO_OUT_ZERO,  /*compatible with DCT*/
} GPIO_OUT;

/* CLOCK OUT*/
typedef enum 
{
    CLK_OUT_UNSUPPORTED = -1,
        
    CLK_OUT0,
    CLK_OUT1,
    CLK_OUT2,
    CLK_OUT3,
    CLK_OUT4,
    CLK_OUT5,
    CLK_OUT6,
    CLK_OUT7,
    
    CLK_MAX     
} GPIO_CLKOUT;

typedef enum CLK_SRC
{
    CLK_SRC_UNSUPPORTED = -1,

    CLK_SRC_APMCUSYS,   /*ahmclk*/
    CLK_SRC_DSPLIO,
    CLK_SRC_F13M,
    CLK_SRC_F65M,
    CLK_SRC_F48M,
    CLK_SRC_F32K,
    CLK_SRC_F26M,
    CLK_SRC_MDMCUSYS,   /*mhmclk*/

    CLK_SRC_MAX
} GPIO_CLKSRC;
/*----------------------------------------------------------------------------*/
typedef struct {    
    u16 val;        /*one GPIO register use 16 bits(2 bytes)*/
    u16 unused[7];  /*The other 14 bytes are unused*/
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
    VAL_REGS    dir[10];            /*0x0000 ~ 0x009F: 160 bytes*/
    u16         rsv00A0[48];        /*0x00A0 ~ 0x00FF: 96  bytes*/
    VAL_REGS    pullen[10];         /*0x0100 ~ 0x019F: 160 bytes*/
    u16         rsv01A0[48];        /*0x01A0 ~ 0x01FF: 96  bytes*/
    VAL_REGS    pullsel[10];        /*0x0200 ~ 0x029F: 160 bytes*/
    u16         rsv02A0[48];        /*0x02A0 ~ 0x02FF: 96  bytes*/
    VAL_REGS    dinv[10];           /*0x0300 ~ 0x039F: 160 bytes*/
    u16         rsv03A0[48];        /*0x03A0 ~ 0x03FF: 96  bytes*/
    VAL_REGS    dout[10];           /*0x0400 ~ 0x049F: 160 bytes*/
    u16         rsv04A0[48];        /*0x04A0 ~ 0x04FF: 96  bytes*/
    VAL_REGS    din[10];            /*0x0500 ~ 0x059F: 160 bytes*/
    u16         rsv05A0[48];        /*0x05A0 ~ 0x05FF: 96  bytes*/
    VAL_REGS    mode[19];           /*0x0600 ~ 0x072F: 304 bytes*/
    u16         rsv2730[232];       /*0x0730 ~ 0x08FF: 464 bytes*/
    VAL_REGS    clkout[8];          /*0x0900 ~ 0x097F: 128 bytes*/
} GPIO_REGS;
/*----------------------------------------------------------------------------*/
typedef enum {
    GPIO_CONF_DIR       =   0x0001,
    GPIO_CONF_PULLEN    =   0x0002,
    GPIO_CONF_PULLSEL   =   0x0004,
    GPIO_CONF_DINV      =   0x0008,
    GPIO_CONF_DOUT      =   0x0010,
    GPIO_CONF_DIN       =   0x0020,
    GPIO_CONF_MODE      =   0x0040,
    GPIO_CONF_CLKOUT    =   0x0080,

    GPIO_CONF_ALL       =   0x00FF,
} GPIO_CONF;
/*direction*/
s32 mt_set_gpio_dir(u32 u4Pin, u32 u4Dir);
s32 mt_get_gpio_dir(u32 u4Pin);

/*pull enable*/
s32 mt_set_gpio_pull_enable(u32 u4Pin, u8 bPullEn);
s32 mt_get_gpio_pull_enable(u32 u4Pin);
/*pull select*/
s32 mt_set_gpio_pull_select(u32 u4Pin, u8 uPullSel);    
s32 mt_get_gpio_pull_select(u32 u4Pin);

/*data inversion*/
s32 mt_set_gpio_inversion(u32 u4Pin, u8 bInvEn);
s32 mt_get_gpio_inversion(u32 u4Pin);

/*input/output*/
s32 mt_set_gpio_out(u32 u4Pin, u32 u4PinOut);
s32 mt_get_gpio_out(u32 u4Pin);
s32 mt_get_gpio_in(u32 u4Pin);

/*mode control*/
s32 mt_set_gpio_mode(u32 u4Pin, u32 u4Mode);
s32 mt_get_gpio_mode(u32 u4Pin);

/*clock output setting*/
s32 mt_set_clock_output(u32 u4ClkOut, u32 u4Src);
s32 mt_get_clock_output(u32 u4ClkOut);

void mt_gpio_set_default(void);
void mt_gpio_dump(GPIO_REGS *regs);
void mt_gpio_load(GPIO_REGS *regs);
void mt_gpio_checkpoint_save(void);
void mt_gpio_checkpoint_compare(void);
void mt_gpio_unlock_init(int all);
void mt_gpio_unlock_dump(void);
void mt_gpio_unlock_pin(int pin, u32 type);
