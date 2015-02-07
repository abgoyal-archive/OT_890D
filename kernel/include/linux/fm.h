

#ifndef __FM_H__
#define __FM_H__

//#define FMDEBUG

#include <linux/ioctl.h>

#define FM_NAME             "fm"
#define FM_DEVICE_NAME      "/dev/fm"

// errno
#define FM_SUCCESS      0
#define FM_FAILED       1
#define FM_EPARM        2
#define FM_BADSTATUS    3
#define FM_TUNE_FAILED  4
#define FM_SEEK_FAILED  5
#define FM_BUSY         6
#define FM_SCAN_FAILED  7

// band

#define FM_BAND_UNKNOWN 0
#define FM_BAND_UE      1 // US/Europe band  87.5MHz ~ 108MHz (DEFAULT)
#define FM_BAND_JAPAN   2 // Japan band      76MHz   ~ 90MHz
#define FM_BAND_JAPANW  3 // Japan wideband  76MHZ   ~ 108MHz
#define FM_BAND_DEFAULT FM_BAND_UE

// space
#define FM_SPACE_UNKNOWN    0
#define FM_SPACE_100K       1
#define FM_SPACE_200K       2
#define FM_SPACE_DEFAULT    FM_SPACE_100K,

// auto HiLo
#define FM_AUTO_HILO_OFF    0
#define FM_AUTO_HILO_ON     1

// seek direction
#define FM_SEEK_UP          0
#define FM_SEEK_DOWN        1

// seek threshold

#define FM_SEEKTH_LEVEL_DEFAULT 4

#define FM_IOC_MAGIC        0xf5 // FIXME: any conflict?

struct fm_tune_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint16_t freq; // IN/OUT parameter
};

struct fm_seek_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint8_t seekdir;
    uint8_t seekth;
    uint16_t freq; // IN/OUT parameter
};

struct fm_scan_parm {
    uint8_t  err;
    uint8_t  band;
    uint8_t  space;
    uint8_t  hilo;
    uint16_t freq; // OUT parameter
    uint16_t ScanTBL[16]; //need no less than the chip
    uint16_t ScanTBLSize; //IN/OUT parameter
};

//For RDS feature
typedef struct
{
   uint8_t TP;
   uint8_t TA;
   uint8_t Music;
   uint8_t Stereo;
   uint8_t Artificial_Head;
   uint8_t Compressed;
   uint8_t Dynamic_PTY;
   uint8_t Text_AB;
   uint32_t flag_status;
}RDSFlag_Struct;

typedef struct
{
   uint16_t Month;
   uint16_t Day;
   uint16_t Year;
   uint16_t Hour;
   uint16_t Minute;
   uint8_t Local_Time_offset_signbit;
   uint8_t Local_Time_offset_half_hour;
}CT_Struct;

typedef struct
{
   int16_t AF_Num;
   int16_t AF[2][25];  //100KHz
   uint8_t Addr_Cnt;
   uint8_t isMethod_A;
   uint8_t isAFNum_Get;
}AF_Info;

typedef struct
{
   uint8_t PS[3][8];
   uint8_t Addr_Cnt;
}PS_Info;

typedef struct
{
   uint8_t TextData[4][64];
   uint8_t GetLength;
   uint8_t isRTDisplay;
   uint8_t TextLength;
   uint8_t isTypeA;
   uint8_t BufCnt;
   uint16_t Addr_Cnt;
}RT_Info;

typedef struct
{
   CT_Struct CT;
   RDSFlag_Struct RDSFlag;
   uint16_t PI;
   uint8_t Switch_TP;
   uint8_t PTY;
   AF_Info AF_Data;
   AF_Info AFON_Data;
   uint8_t Radio_Page_Code;
   uint16_t Program_Item_Number_Code;
   uint8_t Extend_Country_Code;
   uint16_t Language_Code;
   PS_Info PS_Data;
   uint8_t PS_ON[8];   
   RT_Info RT_Data;
   //uint16_t Block_Backup[32][4];
   //uint16_t Group_Cnt[32];
   //uint8_t EINT_Flag;
   //uint8_t RDS_Flag;
   uint16_t event_status; //will use RDSFlag_Struct RDSFlag->flag_status to check which event, is that ok? 
} RDSData_Struct;


//Need care the following definition.
//valid Rds Flag for notify
typedef enum {
   RDS_FLAG_IS_TP              = 0x0001, // Program is a traffic program
   RDS_FLAG_IS_TA              = 0x0002, // Program currently broadcasts a traffic ann.
   RDS_FLAG_IS_MUSIC           = 0x0004, // Program currently broadcasts music
   RDS_FLAG_IS_STEREO          = 0x0008, // Program is transmitted in stereo
   RDS_FLAG_IS_ARTIFICIAL_HEAD = 0x0010, // Program is an artificial head recording
   RDS_FLAG_IS_COMPRESSED      = 0x0020, // Program content is compressed
   RDS_FLAG_IS_DYNAMIC_PTY     = 0x0040, // Program type can change 
   RDS_FLAG_TEXT_AB            = 0x0080  // If this flag changes state, a new radio text 					 string begins
} RdsFlag;

typedef enum {
   RDS_EVENT_FLAGS          = 0x0001, // One of the RDS flags has changed state
   RDS_EVENT_PI_CODE        = 0x0002, // The program identification code has changed
   RDS_EVENT_PTY_CODE       = 0x0004, // The program type code has changed
   RDS_EVENT_PROGRAMNAME    = 0x0008, // The program name has changed
   RDS_EVENT_UTCDATETIME    = 0x0010, // A new UTC date/time is available
   RDS_EVENT_LOCDATETIME    = 0x0020, // A new local date/time is available
   RDS_EVENT_LAST_RADIOTEXT = 0x0040, // A radio text string was completed
   RDS_EVENT_AF             = 0x0080, // Current Channel RF signal strength too weak, need do AF switch  
   RDS_EVENT_AF_LIST        = 0x0100, // An alternative frequency list is ready
   RDS_EVENT_AFON_LIST      = 0x0200, // An alternative frequency list is ready
   RDS_EVENT_TAON           = 0x0400,  // Other Network traffic announcement start
   RDS_EVENT_TAON_OFF       = 0x0800, // Other Network traffic announcement finished.
   RDS_EVENT_RDS            = 0x2000, // RDS Interrupt had arrived durint timer period  
   RDS_EVENT_NO_RDS         = 0x4000, // RDS Interrupt not arrived durint timer period  
   RDS_EVENT_RDS_TIMER      = 0x8000 // Timer for RDS Bler Check. ---- BLER  block error rate
} RdsEvent;

struct fm_rds_tx_parm {
    uint8_t err;
    uint16_t pi;
    uint16_t ps[12]; // 4 ps
    uint16_t other_rds[87];  // 0~29 other groups
    uint8_t other_rds_cnt; // # of other group
};

#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IO(FM_IOC_MAGIC,   1)
#define FM_IOCTL_TUNE          _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_SETVOL        _IOWR(FM_IOC_MAGIC, 4, uint32_t)
#define FM_IOCTL_GETVOL        _IOWR(FM_IOC_MAGIC, 5, uint32_t*)
#define FM_IOCTL_MUTE          _IOWR(FM_IOC_MAGIC, 6, uint32_t)
#define FM_IOCTL_GETRSSI       _IOWR(FM_IOC_MAGIC, 7, uint32_t*)
#define FM_IOCTL_SCAN          _IOWR(FM_IOC_MAGIC, 8, struct fm_scan_parm*)
#define FM_IOCTL_STOP_SCAN     _IO(FM_IOC_MAGIC,   9)
#define FM_IOCTL_POWERUP_TX    _IOWR(FM_IOC_MAGIC, 20, struct fm_tune_parm*)
#define FM_IOCTL_TUNE_TX       _IOWR(FM_IOC_MAGIC, 21, struct fm_tune_parm*)
#define FM_IOCTL_RDS_TX        _IOWR(FM_IOC_MAGIC, 22, struct fm_rds_tx_parm*)

//IOCTL and struct for test
#define FM_IOCTL_GETCHIPID     _IOWR(FM_IOC_MAGIC, 10, uint16_t*)
#define FM_IOCTL_EM_TEST       _IOWR(FM_IOC_MAGIC, 11, struct fm_em_parm*)
#define FM_IOCTL_RW_REG        _IOWR(FM_IOC_MAGIC, 12, struct fm_ctl_parm*)
#define FM_IOCTL_GETMONOSTERO  _IOWR(FM_IOC_MAGIC, 13, uint16_t*)
#define FM_IOCTL_GETCURPAMD    _IOWR(FM_IOC_MAGIC, 14, uint16_t*)
#define FM_IOCTL_GETGOODBCNT   _IOWR(FM_IOC_MAGIC, 15, uint16_t*)
#define FM_IOCTL_GETBADBNT     _IOWR(FM_IOC_MAGIC, 16, uint16_t*)
#define FM_IOCTL_GETBLERRATIO  _IOWR(FM_IOC_MAGIC, 17, uint16_t*)


//IOCTL for RDS 
#define FM_IOCTL_RDS_ONOFF     _IOWR(FM_IOC_MAGIC, 18, uint16_t*)
#define FM_IOCTL_RDS_SUPPORT   _IOWR(FM_IOC_MAGIC, 19, uint16_t*)

#define FM_IOCTL_RDS_SIM_DATA  _IOWR(FM_IOC_MAGIC, 23, uint32_t*)
#define FM_IOCTL_IS_FM_POWERED_UP  _IOWR(FM_IOC_MAGIC, 24, uint32_t*)

#ifdef FMDEBUG
#define FM_IOCTL_DUMP_REG   _IO(FM_IOC_MAGIC, 0xFF)
#endif

enum group_idx {
    mono=0,
    stereo,
    RSSI_threshold,
    HCC_Enable,
    PAMD_threshold,
    Softmute_Enable,
    De_emphasis,
    HL_Side,
    Demod_BW,
    Dynamic_Limiter,
    Softmute_Rate,
    AFC_Enable,
    Softmute_Level,
    Analog_Volume,
    GROUP_TOTAL_NUMS
};
	
enum item_idx {
    Sblend_OFF=0,
    Sblend_ON,  
    ITEM_TOTAL_NUMS
};

struct fm_ctl_parm {
    uint8_t err;
    uint8_t addr;
    uint16_t val;
    uint16_t rw_flag;//0:write, 1:read
};

struct fm_em_parm {
	uint16_t group_idx;
	uint16_t item_idx;
	uint32_t item_value;	
};

#endif // __FM_H__
