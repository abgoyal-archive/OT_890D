

#ifndef MT3351_UART_H
#define MT3351_UART_H

#include <mach/hardware.h>
#include <mach/mt3351_reg_base.h>

#define UART_FIFO_SIZE              (16)

/* system level, not related to hardware */
#define UST_DUMMY_READ              (1 << 31)

#define UART_VFIFO_PORT0            (0x80110000 + IO_OFFSET)
#define UART_VFIFO_PORT1            (0x80112000 + IO_OFFSET)
#define UART_VFIFO_PORT2            (0x80114000 + IO_OFFSET)
#define UART_VFIFO_PORT3            (0x80116000 + IO_OFFSET)
#define UART_VFIFO_PORT4            (0x80118000 + IO_OFFSET)
#define UART_VFIFO_PORT5            (0x8011a000 + IO_OFFSET)

/* IER */
#define UART_IER_ERBFI              (1 << 0) /* RX buffer conatins data int. */
#define UART_IER_ETBEI              (1 << 1) /* TX FIFO threshold trigger int. */
#define UART_IER_ELSI               (1 << 2) /* BE, FE, PE, or OE int. */
#define UART_IER_EDSSI              (1 << 3) /* CTS change (DCTS) int. */
#define UART_IER_XOFFI              (1 << 5)
#define UART_IER_RTSI               (1 << 6)
#define UART_IER_CTSI               (1 << 7)

#define UART_IER_ALL_INTS          (UART_IER_ERBFI|UART_IER_ETBEI|UART_IER_ELSI|\
                                    UART_IER_EDSSI|UART_IER_XOFFI|UART_IER_RTSI|\
                                    UART_IER_CTSI)
#define UART_IER_HW_NORMALINTS     (UART_IER_ERBFI|UART_IER_ELSI|UART_IER_EDSSI)
#define UART_IER_HW_ALLINTS        (UART_IER_ERBFI|UART_IER_ETBEI| \
                                    UART_IER_ELSI|UART_IER_EDSSI)

/* FCR */
#define UART_FCR_FIFOE              (1 << 0)
#define UART_FCR_CLRR               (1 << 1)
#define UART_FCR_CLRT               (1 << 2)
#define UART_FCR_DMA1               (1 << 3)
#define UART_FCR_RXFIFO_1B_TRI      (0 << 6)
#define UART_FCR_RXFIFO_6B_TRI      (1 << 6)
#define UART_FCR_RXFIFO_12B_TRI     (2 << 6)
#define UART_FCR_RXFIFO_RX_TRI      (3 << 6)
#define UART_FCR_TXFIFO_1B_TRI      (0 << 4)
#define UART_FCR_TXFIFO_4B_TRI      (1 << 4)
#define UART_FCR_TXFIFO_8B_TRI      (2 << 4)
#define UART_FCR_TXFIFO_14B_TRI     (3 << 4)

#define UART_FCR_FIFO_INIT          (UART_FCR_FIFOE|UART_FCR_CLRR|UART_FCR_CLRT)
#define UART_FCR_NORMAL             (UART_FCR_FIFO_INIT | \
                                     UART_FCR_TXFIFO_4B_TRI| \
                                     UART_FCR_RXFIFO_12B_TRI)
/* LCR */
#define UART_LCR_BREAK              (1 << 6)
#define UART_LCR_DLAB               (1 << 7)

#define UART_WLS_5                  (0 << 0)
#define UART_WLS_6                  (1 << 0)
#define UART_WLS_7                  (2 << 0)
#define UART_WLS_8                  (3 << 0)
#define UART_WLS_MASK               (3 << 0)

#define UART_1_STOP                 (0 << 2)
#define UART_2_STOP                 (1 << 2)
#define UART_1_5_STOP               (1 << 2)    /* Only when WLS=5 */
#define UART_STOP_MASK              (1 << 2)

#define UART_NONE_PARITY            (0 << 3)
#define UART_ODD_PARITY             (0x1 << 3)
#define UART_EVEN_PARITY            (0x3 << 3)
#define UART_MARK_PARITY            (0x5 << 3)
#define UART_SPACE_PARITY           (0x7 << 3)
#define UART_PARITY_MASK            (0x7 << 3)

/* MCR */
#define UART_MCR_DTR    	        (1 << 0)
#define UART_MCR_RTS    	        (1 << 1)
#define UART_MCR_OUT1               (1 << 2)
#define UART_MCR_OUT2               (1 << 3)
#define UART_MCR_LOOP               (1 << 4)
#define UART_MCR_XOFF               (1 << 7)    /* read only */
#define UART_MCR_NORMAL	            (UART_MCR_DTR|UART_MCR_RTS)

/* LSR */
#define UART_LSR_DR                 (1 << 0)
#define UART_LSR_OE                 (1 << 1)
#define UART_LSR_PE                 (1 << 2)
#define UART_LSR_FE                 (1 << 3)
#define UART_LSR_BI                 (1 << 4)
#define UART_LSR_THRE               (1 << 5)
#define UART_LSR_TEMT               (1 << 6)
#define UART_LSR_FIFOERR            (1 << 7)

/* MSR */
#define UART_MSR_DCTS               (1 << 0)
#define UART_MSR_DDSR               (1 << 1)
#define UART_MSR_TERI               (1 << 2)
#define UART_MSR_DDCD               (1 << 3)
#define UART_MSR_CTS                (1 << 4)    
#define UART_MSR_DSR                (1 << 5)
#define UART_MSR_RI                 (1 << 6)
#define UART_MSR_DCD                (1 << 7)

/* EFR */
#define UART_EFR_EN                 (1 << 4)
#define UART_EFR_AUTO_RTS           (1 << 6)
#define UART_EFR_AUTO_CTS           (1 << 7)
#define UART_EFR_SW_CTRL_MASK       (0xf << 0)

#define UART_EFR_NO_SW_CTRL         (0)
#define UART_EFR_NO_FLOW_CTRL       (0)
#define UART_EFR_AUTO_RTSCTS        (UART_EFR_AUTO_RTS|UART_EFR_AUTO_CTS)
#define UART_EFR_XON1_XOFF1         (0xa) /* TX/RX XON1/XOFF1 flow control */
#define UART_EFR_XON2_XOFF2         (0x5) /* TX/RX XON2/XOFF2 flow control */
#define UART_EFR_XON12_XOFF12       (0xf) /* TX/RX XON1,2/XOFF1,2 flow control */

#define UART_EFR_XON1_XOFF1_MASK    (0xa)
#define UART_EFR_XON2_XOFF2_MASK    (0x5)

/* IIR (Read Only) */
#define UART_IIR_NO_INT_PENDING     (0x01)
#define UART_IIR_RLS                (0x06) /* Receiver Line Status */
#define UART_IIR_RDA                (0x04) /* Receive Data Available */
#define UART_IIR_CTI                (0x0C) /* Character Timeout Indicator */
#define UART_IIR_THRE               (0x02) /* Transmit Holding Register Empty */
#define UART_IIR_MS                 (0x00) /* Check Modem Status Register */
#define UART_IIR_SW_FLOW_CTRL       (0x10) /* Receive XOFF characters */
#define UART_IIR_HW_FLOW_CTRL       (0x20) /* CTS or RTS Rising Edge */
#define UART_IIR_FIFO_EN          	(0xc0)
#define UART_IIR_INT_MASK           (0x1f)

/* RateFix */
#define UART_RATE_FIX               (1 << 0)
#define UART_AUTORATE_FIX           (1 << 1)
#define UART_FREQ_SEL               (1 << 2)

#define UART_RATE_FIX_13M           (1 << 0) /* means UARTclk = APBclk / 4 */
#define UART_AUTORATE_FIX_13M       (1 << 1) 
#define UART_FREQ_SEL_13M           (1 << 2)
#define UART_RATE_FIX_ALL_13M       (UART_RATE_FIX_13M|UART_AUTORATE_FIX_13M| \
                                     UART_FREQ_SEL_13M)

#define UART_RATE_FIX_26M           (0 << 0) /* means UARTclk = APBclk / 2 */
#define UART_AUTORATE_FIX_26M       (0 << 1) 
#define UART_FREQ_SEL_26M           (0 << 2)

/* Autobaud sample */
#define UART_AUTOBADUSAM_13M         7
#define UART_AUTOBADUSAM_26M        15
#define UART_AUTOBADUSAM_52M        31
//#define UART_AUTOBADUSAM_52M        29  /* CHECKME! 28 or 29 ? */
#define UART_AUTOBAUDSAM_58_5M      31  /* CHECKME! 31 or 32 ? */

/* VFIFO enable */
#define UART_VFIFO_ON               (1 << 0)

/* Debugging */
typedef struct {
	u32 RBR:8;
	u32 dummy:24;
} UART_RBR_REG;

typedef struct {
	u32 ERBFI:1;
	u32 ETBEI:1;
	u32 ELSI:1;
	u32 EDSSI:1;
	u32 dummy1:1;
	u32 XOFFI:1;
	u32 RTSI:1;
	u32 CTSI:1;
	u32 dummy2:24;
} UART_IER_REG;

typedef struct {
	u32 FIFOE:1;
	u32 CLRR:1;
	u32 CLRT:1;
	u32 DMA1:1;
	u32 TFTL:2;
	u32 RFTL:2;
	u32 dummy2:24;
} UART_FCR_REG;

typedef struct {
	u32 WLS:2;
	u32 STB:1;
	u32 PEN:1;
	u32 EPS:1;
	u32 SP:1;
	u32 SB:1;
	u32 DLAB:1;
	u32 dummy:24;
} UART_LCR_REG;

typedef struct {
	u32 DTR:1;
	u32 RTS:1;
	u32 dummy1:2;
	u32 LOOP:1;
	u32 dummy2:2;
	u32 XOFF:1;
	u32 dummy3:24;
} UART_MCR_REG;

typedef struct {
	u32 DR:1;
	u32 OE:1;
	u32 PE:1;
	u32 FE:1;
	u32 BI:1;
	u32 THRE:1;
	u32 TEMT:1;
	u32 FIFOERR:1;
	u32 dummy:24;
} UART_LSR_REG;

typedef struct {
	u32 DCTS:1;
	u32 DDSR:1; /* CHECKME! */
	u32 TERI:1; /* CHECKME! */
	u32 DDCD:1; /* CHECKME! */
	u32 CTS:1;
	u32 DSR:1;  /* CHECKME! */
	u32 RI:1;   /* CHECKME! */
	u32 DCD:1;  /* CHECKME! */
	u32 dummy:24;
} UART_MSR_REG;

typedef struct {
	u32 SCR:8;
	u32 dummy:24;
} UART_SCR_REG;

typedef struct {
	u32 AUTO_EN:1;
	u32 dummy:31;
} UART_AUTOBAUD_EN_REG;

typedef struct {
	u32 SPEED:2;
	u32 dummy:30;
} UART_HIGHSPEED_REG;

typedef struct {
	u32 SAM_COUNT:8;
	u32 dummy:24;
} UART_SAM_COUNT_REG;

typedef struct {
	u32 SAM_POINT:8;
	u32 dummy:24;
} UART_SAM_POINT_REG;

typedef struct {
	u32 BAUD_RATE:4;
	u32 BAUD_STAT:4;
	u32 dummy:24;
} UART_AUTOBAUD_REG_REG;

typedef struct {
	u32 RATE_FIX:1;
	u32 AUTOBAUD_RATE_FIX:1;
	u32 FREQ_SEL:1;
	u32 RESTRICT:1;
	u32 dummy:28;
} UART_RATEFIX_AD_REG;

typedef struct {
	u32 AUTOBAUD_SAM:8;
	u32 dummy:24;
} UART_AUTOBAUD_SAM_REG;

typedef struct {
	u32 GUARD_CNT:4;
	u32 GUARD_EN:1;
	u32 dummy:27;
} UART_GUARD_REG;

typedef struct {
	u32 ESC_DATA:8;
	u32 dummy:24;
} UART_ESC_DATA_REG;

typedef struct {
	u32 ESC_EN:1;
	u32 dummy:31;
} UART_ESC_EN_REG;

typedef struct {
	u32 SLEEP_EN:1;
	u32 dummy:31;
} UART_SLEEP_EN_REG;

typedef struct {
	u32 VFIFO_EN:1;
	u32 dummy:31;
} UART_VFIFO_EN_REG;

typedef struct {
	u32 RXTRIG:4;
	u32 dummy:28;
} UART_RXTRIG_REG;

typedef struct {
	u32 THR:8;
	u32 dummy:24;
} UART_THR_REG;

typedef struct {
	u32 NINT:1;
	u32 ID:5;
	u32 FIFOE:2;
	u32 dummy:24;
} UART_IIR_REG;

typedef struct {
	u32 DLL:8;
	u32 dummy:24;
} UART_DLL_REG;

typedef struct {
	u32 DLH:8;
	u32 dummy:24;
} UART_DLH_REG;

typedef struct {
	u32 SW_FLOW_CTRL:4;
	u32 EFR_EN:1;
	u32 dummy1:1;
	u32 AUTO_RTS:1;
	u32 AUTO_CTS:1;
	u32 dummy2:24;
} UART_EFR_REG;

typedef struct {
	u32 XON1:8;
	u32 dummy:24;
} UART_XON1_REG;

typedef struct {
	u32 XON2:8;
	u32 dummy:24;
} UART_XON2_REG;

typedef struct {
	u32 XOFF1:8;
	u32 dummy:24;
} UART_XOFF1_REG;

typedef struct {
	u32 XOFF2:8;
	u32 dummy:24;
} UART_XOFF2_REG;

typedef struct {
    u32 LEN:16;
    u32 dummy:16;
} VFIFO_DMA_COUNT_REG;

typedef struct {
    u32 SIZE:2;
    u32 SINC:1;
    u32 DINC:1;
    u32 DREQ:1;
    u32 B2W:1;
    u32 dummy1:2;
    u32 BRUST:3;
    u32 dummy2:4;
    u32 ITEN:1;
    u32 WPSD:1;
    u32 WPEN:1;
    u32 DIR:1;
    u32 dummy3:1;
    u32 MAS:5;
    u32 dummy4:7;
} VFIFO_DMA_CTRL_REG;

typedef struct {
    u32 dummy1:15;
    u32 STR:1;
    u32 dummy2:16;
} VFIFO_DMA_START_REG;

typedef struct {
    u32 dummy1:15;
    u32 INT:1;
    u32 dummy2:16;
} VFIFO_DMA_INTSTA_REG;

typedef struct {
    u32 dummy1:15;
    u32 ACK:1;
    u32 dummy2:16;
} VFIFO_DMA_ACKINT_REG;

typedef struct {
    u32 ADDR;
} VFIFO_DMA_PGMADDR_REG;

typedef struct {
    u32 WRPTR;
} VFIFO_DMA_WRPTR_REG;

typedef struct {
    u32 RDPTR;
} VFIFO_DMA_RDPTR_REG;

typedef struct {
    u32 FFCNT:16;
    u32 dummy:16;
} VFIFO_DMA_FFCNT_REG;

typedef struct {
    u32 FULL:1;
    u32 EMPTY:1;
    u32 ALT:1;
    u32 dummy:29;
} VFIFO_DMA_FFSTA_REG;

typedef struct {
    u32 ALTLEN:6;
    u32 dummy:26;
} VFIFO_DMA_ALTLEN_REG;

typedef struct {
    u32 FFSIZE:16;
    u32 dummy:16;
} VFIFO_DMA_FFIZE_REG;


struct mt3351_uart_regs {
    UART_EFR_REG *EFR;  /* Only when LCR = 0xbf */
    UART_IER_REG *IER;  
    UART_IIR_REG *IIR;  /* Read only */
    UART_FCR_REG *FCR;  /* Write only */
    UART_LCR_REG *LCR;
    UART_MCR_REG *MCR;
    UART_LSR_REG *LSR;
    UART_MSR_REG *MSR;
    UART_SCR_REG *SCR;

    UART_DLL_REG *DLL;  /* Only when LCR.DLAB = 1 */
    UART_DLH_REG *DLH;  /* Only when LCR.DLAB = 1 */

    UART_VFIFO_EN_REG *VFIFO_EN;
    UART_RXTRIG_REG *RXTRIG;

    UART_HIGHSPEED_REG *HIGHSPEED;
    UART_SAM_COUNT_REG *SAM_CNT;
    UART_SAM_POINT_REG *SAM_POT;
    UART_RATEFIX_AD_REG *RATEFIX_AD;

    UART_AUTOBAUD_EN_REG *AUTOBAUD_EN;
    UART_AUTOBAUD_REG_REG *AUTOBAUD;
    UART_AUTOBAUD_SAM_REG *AUTOBAUD_SAM;

    UART_RBR_REG *RBR;  /* Read only */
    UART_THR_REG *THR;  /* Write only */
    
    UART_GUARD_REG *GUARD;
    UART_ESC_DATA_REG *ESC_DATA;
    UART_ESC_EN_REG *ESC_EN;
    UART_SLEEP_EN_REG *SLEEP_EN;

    UART_XON1_REG *XON1;    /* Only when LCR = 0xbf */
    UART_XON2_REG *XON2;    /* Only when LCR = 0xbf */
    UART_XOFF1_REG *XOFF1;  /* Only when LCR = 0xbf */
    UART_XOFF2_REG *XOFF2;  /* Only when LCR = 0xbf */
};

struct mt3351_dma_vfifo_reg {
    VFIFO_DMA_COUNT_REG *DMA_CNT;
    VFIFO_DMA_CTRL_REG  *DMA_CTRL;
    VFIFO_DMA_START_REG *DMA_START;
    VFIFO_DMA_INTSTA_REG *DMA_INTSTA;
    VFIFO_DMA_ACKINT_REG *DMA_ACKINT;
    VFIFO_DMA_PGMADDR_REG *DMA_PGMADDR;
    VFIFO_DMA_WRPTR_REG *DMA_WRPTR;
    VFIFO_DMA_RDPTR_REG *DMA_RDPTR;
    VFIFO_DMA_FFCNT_REG *DMA_FFCNT;
    VFIFO_DMA_FFSTA_REG *DMA_FFSTA;
    VFIFO_DMA_ALTLEN_REG *DMA_ALTLEN;
    VFIFO_DMA_FFIZE_REG *DMA_FFSIZE;
};

#define INI_REGS(BASE) \
{   .RBR = (UART_RBR_REG *)((BASE) + 0x0), \
    .THR = (UART_THR_REG *)((BASE) + 0x0), \
    .IER = (UART_IER_REG *)((BASE) + 0x4), \
    .IIR = (UART_IIR_REG *)((BASE) + 0x8), \
    .FCR = (UART_FCR_REG *)((BASE) + 0x8), \
    .LCR = (UART_LCR_REG *)((BASE) + 0xc), \
    .MCR = (UART_MCR_REG *)((BASE) + 0x10), \
    .LSR = (UART_LSR_REG *)((BASE) + 0x14), \
    .MSR = (UART_MSR_REG *)((BASE) + 0x18), \
    .SCR = (UART_SCR_REG *)((BASE) + 0x1c), \
    .DLL = (UART_DLL_REG *)((BASE) + 0x0), \
    .DLH = (UART_DLH_REG *)((BASE) + 0x4), \
    .EFR = (UART_EFR_REG *)((BASE) + 0x8), \
    .XON1 = (UART_XON1_REG *)((BASE) + 0x10), \
    .XON2 = (UART_XON2_REG *)((BASE) + 0x14), \
    .XOFF1 = (UART_XOFF1_REG *)((BASE) + 0x18), \
    .XOFF2 = (UART_XOFF2_REG *)((BASE) + 0x1c), \
    .AUTOBAUD_EN = (UART_AUTOBAUD_EN_REG *)((BASE) + 0x20), \
    .HIGHSPEED = (UART_HIGHSPEED_REG *)((BASE) + 0x24), \
    .SAM_CNT = (UART_SAM_COUNT_REG *)((BASE) + 0x28), \
    .SAM_POT = (UART_SAM_POINT_REG *)((BASE) + 0x2c), \
    .AUTOBAUD = (UART_AUTOBAUD_REG_REG *)((BASE) + 0x30), \
    .RATEFIX_AD = (UART_RATEFIX_AD_REG *)((BASE) + 0x34), \
    .AUTOBAUD_SAM = (UART_AUTOBAUD_SAM_REG *)((BASE) + 0x38), \
    .GUARD = (UART_GUARD_REG *)((BASE) + 0x3c), \
    .ESC_DATA = (UART_ESC_DATA_REG *)((BASE) + 0x40), \
    .ESC_EN = (UART_ESC_EN_REG *)((BASE) + 0x44), \
    .SLEEP_EN = (UART_SLEEP_EN_REG *)((BASE) + 0x48), \
    .VFIFO_EN = (UART_VFIFO_EN_REG *)((BASE) + 0x4c), \
    .RXTRIG = (UART_RXTRIG_REG *)((BASE) + 0x50), \
}

#define INI_DMA_VFIFO_REGS(BASE,N) \
{   .DMA_CNT = (VFIFO_DMA_COUNT_REG*)((BASE)+ 0x100 * (N+1) + 0x10), \
    .DMA_CTRL = (VFIFO_DMA_CTRL_REG*)((BASE)+ 0x100 * (N+1) + 0x14), \
    .DMA_START= (VFIFO_DMA_START_REG*)((BASE)+ 0x100 * (N+1) + 0x18), \
    .DMA_INTSTA = (VFIFO_DMA_INTSTA_REG*)((BASE)+ 0x100 * (N+1) + 0x1c), \
    .DMA_ACKINT = (VFIFO_DMA_ACKINT_REG*)((BASE)+ 0x100 * (N+1) + 0x20), \
    .DMA_PGMADDR = (VFIFO_DMA_PGMADDR_REG*)((BASE)+ 0x100 * (N+1) + 0x2c), \
    .DMA_WRPTR = (VFIFO_DMA_WRPTR_REG*)((BASE)+ 0x100 * (N+1) + 0x30), \
    .DMA_RDPTR = (VFIFO_DMA_RDPTR_REG*)((BASE)+ 0x100 * (N+1) + 0x34), \
    .DMA_FFCNT = (VFIFO_DMA_FFCNT_REG*)((BASE)+ 0x100 * (N+1) + 0x38), \
    .DMA_FFSTA = (VFIFO_DMA_FFSTA_REG*)((BASE)+ 0x100 * (N+1) + 0x3c), \
    .DMA_ALTLEN = (VFIFO_DMA_ALTLEN_REG*)((BASE)+ 0x100 * (N+1) + 0x40), \
    .DMA_FFSIZE = (VFIFO_DMA_FFIZE_REG*)((BASE)+ 0x100 * (N+1) + 0x44), \
}

#endif /* MT3351_UART_H */
