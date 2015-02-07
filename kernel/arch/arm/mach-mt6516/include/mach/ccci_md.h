

#ifndef __CCCI_MD_H__
#define __CCCI_MD_H__

#include <mach/ccci.h>


#define CCCI_MODEM_IMAGE   "modem.img"
#define CCCI_MODEM_REGION  (0L)

#define CCCI_SYSFS_MD_INIT "modem"
#define CCCI_SYSFS_MD_BOOT_ATTR "boot"
#define CCCI_PCM_SMEM_SIZE (4 * 1024)
#define MD_BOOT_CMD_CHAR '0'
#define NORMAL_BOOT_ID 0
#define META_BOOT_ID 1
#define MD_RUNTIME_ADDR (CCIF_BASE + 0x0140)
#define UART_MAX_PORT_NUM 8
#define SLEEP_CON 0xF0001204
#define CCCI_CURRENT_VERSION 0x00000923
#define MD_EX_LOG_SIZE 512
#define MD_IMG_DUMP_SIZE (0x1000)
#define CCCI_TTY_BUFF_NR 10
#define NR_CCCI_RESET_USER 10
#define NR_CCCI_RESET_USER_NAME 8

enum { MD_BOOT_STAGE_0 = 0, MD_BOOT_STAGE_1 = 1, MD_BOOT_STAGE_2 = 2 };

enum { MD_INIT_START_BOOT = 0x00000000, MD_INIT_CHK_ID = 0x5555FFFF,
       MD_EX = 0x00000004, MD_EX_CHK_ID = 0x45584350,
       MD_EX_REC_OK = 0x00000006, MD_EX_REC_OK_CHK_ID = 0x45524543 };

enum { ER_MB_START_CMD = -1, ER_MB_CHK_ID = -2, 
       ER_MB_BOOT_READY = -3, ER_MB_UNKNOW_STAGE = -4 };

enum { MD_EX_TYPE_INVALID = 0, MD_EX_TYPE_UNDEF = 1, MD_EX_TYPE_SWI = 2,
       MD_EX_TYPE_PREF_ABT = 3, MD_EX_TYPE_DATA_ABT = 4, MD_EX_TYPE_ASSERT = 5,
       MD_EX_TYPE_FATALERR_TASK = 6, MD_EX_TYPE_FATALERR_BUF = 7,
       MD_EX_TYPE_LOCKUP = 8, MD_EX_TYPE_ASSERT_DUMP = 9 };

#ifdef AP_MD_EINT_SHARE_DATA
enum { CCCI_EXCH_CORE_AWAKEN = 0, CCCI_EXCH_CORE_SLEEP = 1, CCCI_EXCH_CORE_SLUMBER = 2 };
#endif

/* CCCI system message */
enum { CCCI_SYS_MSG_RESET_MD = 0x20100406 };

/* MODEM MAUI SW ASSERT LOG */
struct modem_assert_log
{
    char ex_type;
    char ex_nvram;
    short ex_serial;
    char data1[212];
    char filename[24];
    int linenumber;
    char data2[268];
};

/* MODEM MAUI SW FATAL ERROR LOG */
struct modem_fatalerr_log
{
    char ex_type;
    char ex_nvram;
    short ex_serial;
    char data1[212];
    int err_code1;
    int err_code2;
    char data2[288];
};

struct cores_sleep_info
{
    unsigned char AP_Sleep;
    unsigned char padding1[3];
    unsigned int RTC_AP_WakeUp;
    unsigned int AP_SettleTime; /* clock settle duration */
    unsigned char MD_Sleep;
    unsigned char padding2[3];
    unsigned int RTC_MD_WakeUp;
    unsigned int RTC_MD_Settle_OK;  /* clock settle done time */
};

struct core_eint_config
{
    unsigned char eint_no; 
    unsigned char Sensitivity;
    unsigned char ACT_Polarity;
    unsigned char Dbounce_En;
    unsigned int Dbounce_ms;
};

struct ccci_cores_exch_data
{
    struct cores_sleep_info sleep_info;
    unsigned int report_os_tick;    /* report OS Tick Periodic in second unit */
                                    /* ( 0 = disable ) */
    unsigned int nr_eint_config;
    unsigned int eint_config_offset;    /* offset from SysShareMemBase for struct coreeint_config */
};

struct ccci_reset_sta
{
    int is_allocate;
    int is_reset;
    char name[NR_CCCI_RESET_USER_NAME];
};

typedef int (*ccci_cores_sleep_info_base_req)(void *);
typedef int (*ccci_core_eint_config_setup)(int, void *);

#define CCCI_SYS_SMEM_SIZE sizeof(struct ccci_cores_exch_data)

#endif  /* !__CCCI_MD_H__ */

