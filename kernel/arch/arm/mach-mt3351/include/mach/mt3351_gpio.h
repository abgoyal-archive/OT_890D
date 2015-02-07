

#include <linux/fs.h>
#include <linux/ioctl.h>

/* For IOR_HWSTATUS ioctl */
typedef struct 
{
	u8   u8ChargeStatus;
	u16  u8InputStatus;
	u8   u8DockStatus;
} HARDWARE_STATUS;

#define GPIO_DEVNAME				"hwstatus"
#define GPIO_MAJOR					240

/* GO dock types */
#define GODOCK_NONE		    0
#define GODOCK_WINDSCREEN	1
#define GODOCK_CRIB		    2
#define GODOCK_DESK		    3
#define GODOCK_VIB		    4
#define GODOCK_MOTOR		5
#define GODOCK_RADIO		6


/* For HARDWARE_STATUS::u8InputStatus */
#define ONOFF_MASK                   0x001	/* On-off button      */
#define USB_DETECT_MASK              0x002	/* USB power detect   */
#define DOCK_LIGHTS_MASK             0x004	/* Lights detect */
#define DOCK_IGNITION_MASK           0x008	/* Ignition detect */
#define DOCK_EXTMIC_MASK             0x010	/* External microphone detect */
#define DOCK_HEADPHONE_MASK          0x020	/* Headphone detect */
#define DOCK_LINEIN_MASK             0x040	/* Line in detect PIN on K4 dock conn */
#define DOCK_RXD_DETECT_MASK         0x080	/* Serial detect via RXD break / interrupt signalling */
#define DOCK_ONOFF_MASK              0x100
#define DOCK_FM_TRANSMITTER_MASK     0x200
#define DOCK_MAINPOWER_ONOFF         0x400
#define DOCK_IPOD_DETECT             0x800
#define DOCK_TMC_DETECT              0x1000
#define LOW_DC_VCC_DETECT            0x2000

/* For HARDWARE_STATUS::u8DockStatus */
#define DOCK_NONE	                0
#define DOCK_WINDSCREEN	            1
#define DOCK_CRIB	                2
#define DOCK_DESK	                3
#define DOCK_VIB	                4
#define DOCK_MOTOR	                5
#define DOCK_RADIO	                6
#define DOCK_PERMANENT	            DOCK_CRIB

/* GPIO driver ioctls */
#define GPIO_DRIVER_MAGIC	            'U'
#define IOR_HWSTATUS		            _IOR(GPIO_DRIVER_MAGIC, 0,  HARDWARE_STATUS)
#define IOR_DISK_ACCESS		            _IOR(GPIO_DRIVER_MAGIC, 7,  u32)
#define IOR_BT_MODE		                _IOR(GPIO_DRIVER_MAGIC, 8,  u32)
#define IOR_GET_BT_ERROR	            _IOW(GPIO_DRIVER_MAGIC, 9,  u32)
#define IOR_SET_BT_ERROR	            _IOR(GPIO_DRIVER_MAGIC, 10, u32)
#define IOW_RESET_ONOFF_STATE	        _IOR(GPIO_DRIVER_MAGIC, 12, u32)
#define IOW_ENABLE_DOCK_UART	        _IOW(GPIO_DRIVER_MAGIC, 13, u32)
#define IOW_GSM_ON		                _IO(GPIO_DRIVER_MAGIC,  14)
#define OBSOLETE_IOW_SET_FM_FREQUENCY	_IOW(GPIO_DRIVER_MAGIC, 15, u32)
#define IOW_SET_MEMBUS_SPEED	        _IOW(GPIO_DRIVER_MAGIC, 16, u32)
#define IOW_CYCLE_DOCK_POWER	        _IO(GPIO_DRIVER_MAGIC,  17)
#define IOW_GSM_OFF		                _IO(GPIO_DRIVER_MAGIC,  18)
#define IOW_FACTORY_TEST_POINT          _IOW(GPIO_DRIVER_MAGIC, 19, u32)
#define IOW_POKE_RESET_BUTTON	        _IOW(GPIO_DRIVER_MAGIC, 20, u32)
#define IOW_RTCALARM_SUICIDE	        _IOW(GPIO_DRIVER_MAGIC, 21, u32)
#define IOW_SET_DVS_HACK	            _IOW(GPIO_DRIVER_MAGIC, 22, u32)
#define IOW_USB_VBUS_WAKEUP	            _IOW(GPIO_DRIVER_MAGIC, 23, u32)
#define IOW_KRAKOW_RDSTMC_HACK_VBUSOFF	_IOW(GPIO_DRIVER_MAGIC, 24, u32)
#define IOW_KRAKOW_RDSTMC_HACK_VBUSINP	_IOW(GPIO_DRIVER_MAGIC, 25, u32)

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


// GPIO DIRECTION
#define GPIO_DIR_IN     0
#define GPIO_DIR_OUT    1

// GPIO OUTPUT
#define GPIO_OUT_ZERO   0
#define GPIO_OUT_ONE    1

// GPIO DRIVER CONTROL
#define GPIO_DRIVING_CURRENT_2MA 0x01
#define GPIO_DRIVING_CURRENT_4MA 0x02
#define GPIO_DRIVING_CURRENT_8MA 0x04
#define GPIO_DRIVING_CURRENT_SR  0x08

// GPIO DEBUG OUTPUT
#define GPIO_DEBUG_DISABLE      0
#define GPIO_DEBUG_ENABLE       1

#define GPIO_PU_DISABLE         0
#define GPIO_PU_ENABLE          1
#define GPIO_PD_DISABLE         0
#define GPIO_PD_ENABLE          1

/* OCFG FIELDS */

#define 	GPIO6_OCTL      0   // 0 : GPIO, 1 : usb drv vbus
#define 	GPIO8_OCTL      1   // 0 : GPIO, 1:uart1 rts
#define 	GPIO10_OCTL     2   // 0 : GPIO, 1:uart2 rts
#define 	GPIO12_OCTL     3   // 0 : GPIO, 1:uart4 tx, 2: PWM4
#define 	GPIO13_OCTL     5   // 0 : GPIO, 1:IrDA PDN, 2 : PWM5
#define 	GPIO44_OCTL     7   // 0 : GPIO, 1:PWM4 2 : I2S
#define 	GPIO45_OCTL     9   // 0 : GPIO, 1:PWM5, 2 :  I2S WS			
#define 	GPIO46_OCTL     11  // 0 : GPIO, 1:UART4 TX, 2 : Keypad Row 7
#define 	GPIO62_OCTL     13  // 0 : GPIO, 1:IrDA TX, 2 : Keypad Row 7
#define 	GPIO64_OCTL     15  // 0 : GPIO, 1:IrDA TX
#define 	GPIO66_OCTL     16  // 0 : GPIO, 1:IrDA TX, 2 : Keypad Row 7
#define 	GPIO68_OCTL     18  // 0 : GPIO, 1:IrDA TX, 2 : Keypad Row 7							
#define 	GPIO104_OCTL    20  // 0 : GPIO, 1:EMI BANK 3		
#define 	GPIO105_OCTL    21  // 0 : GPIO, 1:EMI BANK 2
#define 	GPIO106_OCTL    22  // 0 : GPIO, 1:EMI BANK 1				
#define 	GPIO22_OCTL     23  // 0 : GPIO, 1:External Clock

/* ICFG FIELDS */

#define EINT0_SRC       0
#define EINT1_SRC       1
#define EINT2_SRC       2
#define EINT3_SRC       3
#define EINT4_SRC       4
#define EINT5_SRC       6
#define EINT6_SRC       8
#define EINT7_SRC       9
#define EINT8_SRC       10
#define EINT9_SRC       11
#define EINT10_SRC      12
#define EINT11_SRC      13
#define EINT12_SRC      14
#define EINT13_SRC      15
#define EINT14_SRC      16
#define EINT15_SRC      17
#define IRDARXD_SRC     18
#define UCTS1_SRC       20
#define UCTS2_SRC       21
#define URXD4_SRC       22
#define PWRKEY_SRC      23
#define KPCOL7_SRC      24
#define DSPEINT_SRC     26

/* PINMUX FIELDS */

#define JTAG_CTL    0
#define SPI_CTL     2
#define UT0_CTL     4
#define UT1_CTL     5
#define UT2_CTL     6
#define UT3_CTL     7
#define UT4_CTL     8
#define DAI_CTL     9
#define PWM_CTL     10
#define MC0_CTL     11
#define MC1_CTL     13
#define CAM0_CTL    14
#define CAM1_CTL    16
#define NLD_CTLH    18
#define NLD_CTLL    21
#define EP_CTL      23
#define DPI_CTL     27

// ---------------------------------------------------------------------------
//  OCFG Value Enum Definitions
// ---------------------------------------------------------------------------

typedef enum
{
    GPIO22_OCTL_GPIO = 0x0,     //Normal GPIO [22] output
    GPIO22_OCTL_ECLK = 0x1,     //External clock source output
} OCFG_GPIO22_OCTL;

typedef enum
{
    GPIO106_OCTL_GPIO    = 0x0,     //Normal GPIO [106] output
    GPIO106_OCTL_EMI1    = 0x1,     //EMI bank1 chip select
} OCFG_GPIO106_OCTL;

typedef enum
{
    GPIO105_OCTL_GPIO   = 0x0,     //Normal GPIO [105] output
    GPIO105_OCTL_EMI2   = 0x1,     //EMI bank2 chip select
} OCFG_GPIO105_OCTL;

typedef enum
{
    GPIO104_OCTL_GPIO   = 0x0,     //Normal GPIO [104] output
    GPIO104_OCTL_EMI3   = 0x1,     //EMI bank3 chip select
} OCFG_GPIO104_OCTL;

typedef enum
{
    GPIO68_OCTL_GPIO    = 0x0,     //Normal GPIO [68] output
    GPIO68_OCTL_IRDATX  = 0x1,     //IrDA TX
    GPIO68_OCTL_KEYPAD  = 0x2,     //Keypad Row 7
    GPIO68_OCTL_RESVED  = 0x3,     //Reserved 
} OCFG_GPIO68_OCTL;


typedef enum
{
    GPIO67_OCTL_GPIO    = 0x0,     //Normal GPIO [67] output
    GPIO67_OCTL_IRDATX  = 0x1,     //IrDA TX
    GPIO67_OCTL_KEYPAD  = 0x2,     //Keypad Row 7
    GPIO67_OCTL_RESVED  = 0x3,     //Reserved 
} OCFG_GPIO67_OCTL;

typedef enum
{
    GPIO64_OCTL_GPIO    = 0x0,     //Normal GPIO [64] output
    GPIO64_OCTL_IRDATX  = 0x1,     //IrDA TX 
} OCFG_GPIO64_OCTL;

typedef enum
{
    GPIO62_OCTL_GPIO    = 0x0,     //Normal GPIO [62] output
    GPIO62_OCTL_IRDATX  = 0x1,     //IrDA TX
    GPIO62_OCTL_KEYPAD  = 0x2,     //Keypad Row 7
    GPIO62_OCTL_RESVED  = 0x3,     //Reserved 
} OCFG_GPIO62_OCTL;

typedef enum
{
    GPIO46_OCTL_GPIO     = 0x0,     //Normal GPIO [46] output
    GPIO46_OCTL_UART4TX  = 0x1,     //UART 4 TX
    GPIO46_OCTL_I2SCLK   = 0x2,     //I2S CLK
    GPIO46_OCTL_KEYPAD   = 0x3,     //Keypad Row 7 
} OCFG_GPIO46_OCTL;

typedef enum
{
    GPIO45_OCTL_GPIO     = 0x0,     //Normal GPIO [45] output
    GPIO45_OCTL_PWM5     = 0x1,     //PWM channel 5
    GPIO45_OCTL_I2SWS    = 0x2,     //I2S WS
    GPIO45_OCTL_RESVED   = 0x3,     //Reserved 
} OCFG_GPIO45_OCTL;

typedef enum
{
    GPIO44_OCTL_GPIO     = 0x0,     //Normal GPIO [44] output
    GPIO44_OCTL_PWM4     = 0x1,     //PWM channel 4
    GPIO44_OCTL_I2SDATA  = 0x2,     //I2S Data
    GPIO44_OCTL_RESVED   = 0x3,     //Reserved 
} OCFG_GPIO44_OCTL;

typedef enum
{
    GPIO13_OCTL_GPIO     = 0x0,     //Normal GPIO [13] output
    GPIO13_OCTL_IRDAPND  = 0x1,     //IrDA PDN
    GPIO13_OCTL_PWM5     = 0x2,     //PWM channel 5
    GPIO13_OCTL_RESVED   = 0x3,     //Reserved 
} OCFG_GPIO13_OCTL;

typedef enum
{
    GPIO12_OCTL_GPIO     = 0x0,     //Normal GPIO [12] output
    GPIO12_OCTL_UART4TX  = 0x1,     //UART 4 TX
    GPIO12_OCTL_PWM4     = 0x2,     //PWM channel 4
    GPIO12_OCTL_RESVED   = 0x3,     //Reserved 
} OCFG_GPIO12_OCTL;

typedef enum
{
    GPIO10_OCTL_GPIO     = 0x0,     //Normal GPIO [10] output
    GPIO10_OCTL_UART2RTS = 0x1,     //UART 2 RTS 
} OCFG_GPIO10_OCTL;

typedef enum
{
    GPIO8_OCTL_GPIO      = 0x0,     //Normal GPIO [8] output
    GPIO8_OCTL_UART1RTS  = 0x1,     //UART 1 RTS 
} OCFG_GPIO8_OCTL;

typedef enum
{
    GPIO6_OCTL_GPIO      = 0x0,     //Normal GPIO [6] output
    GPIO6_OCTL_USBVBUS   = 0x1,     //USB DRV VBUS 
} OCFG_GPIO6_OCTL;

// ---------------------------------------------------------------------------
//  OCFG Value Enum Definitions
// ---------------------------------------------------------------------------

typedef enum
{
    DSPEINT_GPIO6_INPUT         = 0x0,     //GPIO [6] input
    DSPEINT_GPIO13_INPUT        = 0x1,     //GPIO [13] input    
} ICFG_DSPEINT_SRC;

typedef enum
{
    KPCOL7_GPIO44_INPUT        = 0x0,     //GPIO [44] input
    KPCOL7_GPIO61_INPUT        = 0x1,     //GPIO [61] input
    KPCOL7_GPIO65_INPUT        = 0x2,     //GPIO [65] input
    KPCOL7_GPIO67_INPUT        = 0x3,     //GPIO [67] input
} ICFG_KPCOL7_SRC;

typedef enum
{
    PWRKEY_DEBOUNCE_INPUT      = 0x0,     //Debounced power key from PMU
    PWRKEY_GPIO13_INPUT        = 0x1,     //GPIO [13] input    
} ICFG_PWRKEY_SRC;

typedef enum
{
    URXD4_ZERO_INPUT           = 0x0,     //UART4 RXD is always 0
    URXD4_GPIO11_INPUT         = 0x1,     //GPIO [11] input    
} ICFG_URXD4_SRC;

typedef enum
{
    UCTS2_ZERO_INPUT           = 0x0,     //UART2 CTS is always 0
    UCTS2_GPIO9_INPUT          = 0x1,     //GPIO [9] input    
} ICFG_UCTS2_SRC;

typedef enum
{
    UCTS1_ZERO_INPUT           = 0x0,     //UART1 CTS is always 0
    UCTS1_GPIO7_INPUT          = 0x1,     //GPIO [7] input    
} ICFG_UCTS1_SRC;

typedef enum
{
    IRDARXD_GPIO61_INPUT       = 0x0,     //GPIO [61] input
    IRDARXD_GPIO63_INPUT       = 0x1,     //GPIO [63] input
    IRDARXD_GPIO65_INPUT       = 0x2,     //GPIO [65] input
    IRDARXD_GPIO67_INPUT       = 0x3,     //GPIO [67] input
} ICFG_IRDARXD_SRC;

typedef enum
{
    EINT15_GPIO91_INPUT        = 0x0,     //GPIO [91] input
    EINT15_GPIO110_INPUT       = 0x1,     //GPIO [110] input    
} ICFG_EINT15_SRC;

typedef enum
{
    EINT14_GPIO90_INPUT        = 0x0,     //GPIO [90] input
    EINT14_GPIO109_INPUT       = 0x1,     //GPIO [109] input    
} ICFG_EINT14_SRC;

typedef enum
{
    EINT13_GPIO89_INPUT        = 0x0,     //GPIO [89] input
    EINT13_GPIO108_INPUT       = 0x1,     //GPIO [108] input    
} ICFG_EINT13_SRC;

typedef enum
{
    EINT12_GPIO88_INPUT        = 0x0,     //GPIO [88] input
    EINT12_GPIO107_INPUT       = 0x1,     //GPIO [107] input    
} ICFG_EINT12_SRC;

typedef enum
{
    EINT11_GPIO87_INPUT        = 0x0,     //GPIO [87] input
    EINT11_GPIO106_INPUT       = 0x1,     //GPIO [106] input    
} ICFG_EINT11_SRC;

typedef enum
{
    EINT10_GPIO86_INPUT        = 0x0,     //GPIO [86] input
    EINT10_GPIO105_INPUT       = 0x1,     //GPIO [105] input    
} ICFG_EINT10_SRC;

typedef enum
{
    EINT9_GPIO85_INPUT        = 0x0,     //GPIO [85] input
    EINT9_GPIO104_INPUT       = 0x1,     //GPIO [104] input    
} ICFG_EINT9_SRC;

typedef enum
{
    EINT8_GPIO84_INPUT        = 0x0,     //GPIO [84] input
    EINT8_GPIO103_INPUT       = 0x1,     //GPIO [103] input    
} ICFG_EINT8_SRC;

typedef enum
{
    EINT7_GPIO13_INPUT        = 0x0,     //GPIO [13] input
    EINT7_GPIO19_INPUT        = 0x1,     //GPIO [19] input    
} ICFG_EINT7_SRC;

typedef enum
{
    EINT6_GPIO12_INPUT        = 0x0,     //GPIO [12] input
    EINT6_GPIO46_INPUT        = 0x1,     //GPIO [46] input    
} ICFG_EINT6_SRC;

typedef enum
{
    EINT5_GPIO5_INPUT         = 0x0,     //GPIO [5] input
    EINT5_GPIO11_INPUT        = 0x1,     //GPIO [11] input
    EINT5_GPIO45_INPUT        = 0x2,     //GPIO [45] input
} ICFG_EINT5_SRC;

typedef enum
{
    EINT4_GPIO4_INPUT         = 0x0,     //GPIO [4] input
    EINT4_GPIO10_INPUT        = 0x1,     //GPIO [10] input
    EINT4_GPIO44_INPUT        = 0x2,     //GPIO [44] input
} ICFG_EINT4_SRC;

typedef enum
{
    EINT3_GPIO3_INPUT        = 0x0,     //GPIO [3] input
    EINT3_GPIO9_INPUT        = 0x1,     //GPIO [9] input    
} ICFG_EINT3_SRC;

typedef enum
{
    EINT2_GPIO2_INPUT        = 0x0,     //GPIO [2] input
    EINT2_GPIO8_INPUT        = 0x1,     //GPIO [8] input    
} ICFG_EINT2_SRC;

typedef enum
{
    EINT1_GPIO1_INPUT        = 0x0,     //GPIO [1] input
    EINT1_GPIO7_INPUT        = 0x1,     //GPIO [7] input    
} ICFG_EINT1_SRC;

typedef enum
{
    EINT0_GPIO0_INPUT        = 0x0,     //GPIO [0] input
    EINT0_GPIO6_INPUT        = 0x1,     //GPIO [6] input    
} ICFG_EINT0_SRC;

// ---------------------------------------------------------------------------
//  PinMux Value Enum Definitions
// ---------------------------------------------------------------------------

typedef enum
{
    PIN_MUX_JTAG_CTRL_GPIO = 0x0,
    PIN_MUX_JTAG_CTRL_JTAG = 0x1,
    PIN_MUX_JTAG_CTRL_SPI1 = 0x2,
    PIN_MUX_JTAG_CTRL_I2S  = 0x3,
} PIN_MUX_JTAG_CTRL;

typedef enum
{
    PIN_MUX_DAI_CTRL_GPIO = 0x0,
    PIN_MUX_DAI_CTRL_DAI  = 0x1,
} PIN_MUX_DAI_CTRL;

typedef enum
{
    PIN_MUX_PWM_CTRL_GPIO = 0x0,
    PIN_MUX_PWM_CTRL_PWM  = 0x1,
} PIN_MUX_PWM_CTRL;

typedef enum
{
    PIN_MUX_CAM0_CTRL_GPIO   = 0x0,
    PIN_MUX_CAM0_CTRL_CAMERA = 0x1,
    PIN_MUX_CAM0_CTRL_KEYPAD = 0x2,
    PIN_MUX_CAM0_CTRL_DSPICE = 0x3,
} PIN_MUX_CAM0_CTRL;

typedef enum
{
    PIN_MUX_CAM1_CTRL_GPIO   = 0x0,
    PIN_MUX_CAM1_CTRL_CAMERA = 0x1,
    PIN_MUX_CAM1_CTRL_KEYPAD = 0x2,
} PIN_MUX_CAM1_CTRL;

typedef enum
{
    PIN_MUX_EP_CTRL_2X_DDR16      = 0x0,
    PIN_MUX_EP_CTRL_DDR16         = 0x1,
    PIN_MUX_EP_CTRL_DDR32         = 0x2,
    PIN_MUX_EP_CTRL_2X_SDR16      = 0x3,
    PIN_MUX_EP_CTRL_SDR16         = 0x4,
    PIN_MUX_EP_CTRL_SDR32         = 0x5,
    PIN_MUX_EP_CTRL_PSRAM16_DEMUX = 0x6,
    PIN_MUX_EP_CTRL_PSRAM16_ADMUX = 0x7,
    PIN_MUX_EP_CTRL_DDR16_PSRAM16 = 0x8,
    PIN_MUX_EP_CTRL_SDR16_PSRAM16 = 0x9,
    PIN_MUX_EP_CTRL_DDR32_PSRAM16 = 0xA,
    PIN_MUX_EP_CTRL_SDR32_PSRAM16 = 0xB,
} PIN_MUX_EP_CTRL;

typedef enum
{
    PIN_MUX_DPI_CTRL_DPI666 = 0x0,
    PIN_MUX_DPI_CTRL_PHOST  = 0x1,
    PIN_MUX_DPI_CTRL_SHOST  = 0x2,
} PIN_MUX_DPI_CTRL;

#define PIN_MUX_SPI_CTRL_GPIO           0x0
#define PIN_MUX_SPI_CTRL_SPI2           0x1
#define PIN_MUX_SPI_CTRL_SPI1           0x2
#define PIN_MUX_SPI_CTRL_SPI0           0x3

#define PIN_MUX_UT0_CTRL_GPIO           0x0
#define PIN_MUX_UT0_CTRL_UART           0x1
#define PIN_MUX_UT1_CTRL_GPIO           0x0
#define PIN_MUX_UT1_CTRL_UART           0x1
#define PIN_MUX_UT2_CTRL_GPIO           0x0
#define PIN_MUX_UT2_CTRL_UART           0x1
#define PIN_MUX_UT3_CTRL_GPIO           0x0
#define PIN_MUX_UT3_CTRL_UART           0x1
#define PIN_MUX_UT4_CTRL_GPIO           0x0
#define PIN_MUX_UT4_CTRL_UART           0x1

#define PIN_MUX_MC0_CTRL_GPIO           0x0
#define PIN_MUX_MC0_CTRL_MC4            0x1
#define PIN_MUX_MC0_CTRL_MC8_ON_BOARD   0x2
#define PIN_MUX_MC0_CTRL_MC8            0x3

#define PIN_MUX_MC1_CTRL_GPIO           0x0
#define PIN_MUX_MC1_CTRL_MC             0x1

#define PIN_MUX_NLD_CTRLH_GPIO          0x0
#define PIN_MUX_NLD_CTRLH_NAND16        0x1
#define PIN_MUX_NLD_CTRLH_PHOSTAUX      0x2
#define PIN_MUX_NLD_CTRLH_DPI888        0x3
#define PIN_MUX_NLD_CTRLH_KEYPAD        0x4
#define PIN_MUX_NLD_CTRLH_SHOSTAUX      0x5

#define PIN_MUX_NLD_CTRLL_GPIO          0x0
#define PIN_MUX_NLD_CTRLL_NAND          0x1
#define PIN_MUX_NLD_CTRLL_KEYPAD        0x2
#define PIN_MUX_NLD_CTRLL_MC            0x3

s32 mt_set_gpio_dir(u32 u4Pin, u32 u4Dir);
s32 mt_get_gpio_dir(u32 u4Pin);

s32 mt_set_gpio_pullup(u32 u4Pin, u8 bPullUpEn);
s32 mt_get_gpio_pullup(u32 u4Pin);
s32 mt_set_gpio_pulldown(u32 u4Pin, u8 bPullDownEn);
s32 mt_get_gpio_pulldown(u32 u4Pin);

s32 mt_set_gpio_out(u32 u4Pin, u32 u4PinOut);
s32 mt_get_gpio_out(u32 u4Pin);
s32 mt_get_gpio_in(u32 u4Pin);

s32 mt_set_gpio_driving_current(u32 u4Pin, u32 u4PinCurrent);
s32 mt_get_gpio_driving_current(u32 u4Pin);

s32 mt_set_gpio_OCFG(u32 u4Value, u32 u4Field);
s32 mt_get_gpio_OCFG(u32 u4Field);

s32 mt_set_gpio_ICFG(u32 u4Value, u32 u4Field);
s32 mt_get_gpio_ICFG(u32 u4Field);

s32 mt_set_gpio_debug(u32 u4Pin, u32 u4PinDBG);
s32 mt_get_gpio_debug(u32 u4Pin);

s32 mt_set_gpio_TM(u8 bTM);

s32 mt_set_gpio_PinMux(u32 u4PinMux, u32 u4Field);
s32 mt_get_gpio_PinMux(u32 u4Field);

