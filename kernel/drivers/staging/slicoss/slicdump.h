
#ifndef _SLIC_DUMP_H_
#define _SLIC_DUMP_H_

#define DEBUG_SUCCESS   0

#define UTILITY_RESET       0x0
#define UTILITY_ISP_ADDR    0x4     /* Interrupt status Pointer */
#define UTILITY_ISR_ADDR    0x8     /* Interrupt status Register */
#define UTILITY_ICR_ADDR    0xc     /* Interrupt Control Register */
#define UTILITY_CPR_ADDR    0x10    /* Command Pointer Register */
#define UTILITY_DPR_ADDR    0x14    /* Data Pointer Register */
#define UTILITY_DMP_TRQ     0x18    /* Dump queue onto ALU for analyser */
#define UTILITY_UPP_ADDR    0x1c    /* Bits 63-32 of cmd/data pointer */

#define SLIC_ISR_CC         0x10000000  /* Command complete - synchronous */
#define SLIC_ISR_ERR        0x01000000  /* Command Error - synchronous */
#define SLIC_ISR_CMD_MASK   0x11000000  /* Command status mask */
#define SLIC_ISR_TPH        0x00080000  /* Transmit processor halted - async */
#define SLIC_ISR_RPH        0x00040000  /* Receive processor halted - async */

#define SLIC_ICR_OFF        0           /* Interrupts disabled */
#define SLIC_ICR_ON         1           /* Interrupts enabled */
#define SLIC_ICR_MASK       2           /* Interrupts masked */

#define WRITE_DREG(reg, value, flush)                           \
{                                                               \
    writel((value), (reg));                                     \
    if ((flush)) {                                               \
	mb();                                                   \
    }                                                           \
}


#define COMMAND_BYTE(command, alt_proc, proc) ((command) | (alt_proc) | (proc))

#define CMD_HALT        0x0     /* Send a halt to the INIC */
#define CMD_RUN         0x8     /* Start the halted INIC */
#define CMD_STEP        0x10    /* Single step the inic */
#define CMD_BREAK       0x18    /* Set a breakpoint - 8 byte command */
#define CMD_RESET_BREAK 0x20    /* Reset a breakpoint - 8 byte cmd */
#define CMD_DUMP        0x28    /* Dump INIC memory - 8 byte command */
#define CMD_LOAD        0x30    /* Load INIC memory - 8 byte command */
#define CMD_MAP         0x38    /* Map out a ROM instruction - 8 BC */
#define CMD_CAM_OPS     0x38    /* perform ops on specific CAM */
#define CMD_XMT         0x40    /* Transmit frame */
#define CMD_RCV         0x48    /* Receive frame */

#define ALT_PROC_TRANSMIT   0x0
#define ALT_PROC_RECEIVE    0x4

#define PROC_INVALID        0x0
#define PROC_NONE           0x0  /* Gigabit use */
#define PROC_TRANSMIT       0x1
#define PROC_RECEIVE        0x2
#define PROC_UTILITY        0x3


struct BREAK {
    unsigned char     command;    /* Command word defined above */
    unsigned char     resvd;
    ushort    count;      /* Number of executions before break */
    u32   addr;       /* Address of break point */
};

struct dump_cmd {
    unsigned char     cmd;        /* Command word defined above */
    unsigned char     desc;       /* Descriptor values - defined below */
    ushort    count;      /* number of 4 byte words to be transferred */
    u32   addr;       /* start address of dump or load */
};

struct RCV_OR_XMT_FRAME {
    unsigned char     command;    /* Command word defined above */
    unsigned char     MacId;      /* Mac ID of interface - transmit only */
    ushort    count;      /* Length of frame in bytes */
    u32   pad;        /* not used */
};

#define DESC_RFILE          0x0     /* Register file */
#define DESC_SRAM           0x1     /* SRAM */
#define DESC_DRAM           0x2     /* DRAM */
#define DESC_QUEUE          0x3     /* queues */
#define DESC_REG            0x4     /* General registers (pc, status, etc) */
#define DESC_SENSE          0x5     /* Sense register */

/* Descriptor field definitions for CMD_DUMP_CAM */
#define DUMP_CAM_A              0
#define DUMP_CAM_B              1               /* unused at present */
#define DUMP_CAM_C              2
#define DUMP_CAM_D              3
#define SEARCH_CAM_A            4
#define SEARCH_CAM_C            5

struct MAP {
    unsigned char   command;    /* Command word defined above */
    unsigned char   not_used[3];
    ushort  map_to;     /* Instruction address in WCS */
    ushort  map_out;    /* Instruction address in ROM */
};

#define SLIC_MAX_QUEUE       32 /* Total # of queues on the INIC (0-31)*/
#define SLIC_4MAX_REG       512 /* Total # of 4-port file-registers    */
#define SLIC_1MAX_REG       384 /* Total # of file-registers           */
#define SLIC_GBMAX_REG     1024 /* Total # of Gbit file-registers      */
#define SLIC_NUM_REG         32 /* non-file-registers = NUM_REG in tm-simba.h */
#define SLIC_GB_CAMA_SZE     32
#define SLIC_GB_CAMB_SZE     16
#define SLIC_GB_CAMAB_SZE    32
#define SLIC_GB_CAMC_SZE     16
#define SLIC_GB_CAMD_SZE     16
#define SLIC_GB_CAMCD_SZE    32

struct CORE_Q {
    u32   queueOff;           /* Offset of queue */
    u32   queuesize;          /* size of queue */
};

#define DRIVER_NAME_SIZE    32

struct sliccore_hdr {
    unsigned char driver_version[DRIVER_NAME_SIZE]; /* Driver version string */
    u32   RcvRegOff;          /* Offset of receive registers */
    u32   RcvRegsize;         /* size of receive registers */
    u32   XmtRegOff;          /* Offset of transmit registers */
    u32   XmtRegsize;         /* size of transmit registers */
    u32   FileRegOff;         /* Offset of register file */
    u32   FileRegsize;        /* size of register file */
    u32   SramOff;            /* Offset of Sram */
    u32   Sramsize;           /* size of Sram */
    u32   DramOff;            /* Offset of Dram */
    u32   Dramsize;           /* size of Dram */
    CORE_Q    queues[SLIC_MAX_QUEUE]; /* size and offsets of queues */
    u32   CamAMOff;           /* Offset of CAM A contents */
    u32   CamASize;           /* Size of Cam A */
    u32   CamBMOff;           /* Offset of CAM B contents */
    u32   CamBSize;           /* Size of Cam B */
    u32   CamCMOff;           /* Offset of CAM C contents */
    u32   CamCSize;           /* Size of Cam C */
    u32   CamDMOff;           /* Offset of CAM D contents */
    u32   CamDSize;           /* Size of Cam D */
};

#define BUFMAX      0x20000 /* 128k - size of input/output buffer */
#define BUFMAXP2    5       /* 2**5 (32) 4K pages */

#define IOCTL_SIMBA_BREAK           _IOW('s', 0, unsigned long)
/* #define IOCTL_SIMBA_INIT            _IOW('s', 1, unsigned long) */
#define IOCTL_SIMBA_KILL_TGT_PROC   _IOW('s', 2, unsigned long)


#define THREADRECEIVE   1   /* bit 0 of StoppedThreads */
#define THREADTRANSMIT  2   /* bit 1 of StoppedThreads */
#define THREADBOTH      3   /* bit 0 and 1.. */

#endif  /*  _SLIC_DUMP_H  */
