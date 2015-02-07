

 
#ifndef _MTK_DVT_TEST_ADC_HW_H
#define _MTK_DVT_TEST_ADC_HW_H


#define AUXADC_CON0                     (AUXADC_BASE + 0x000)
#define AUXADC_CON1                     (AUXADC_BASE + 0x004)
#define AUXADC_CON2                     (AUXADC_BASE + 0x008)
#define AUXADC_CON3                     (AUXADC_BASE + 0x00C)

#define AUXADC_DAT0                     (AUXADC_BASE + 0x010)
#define AUXADC_DAT1                     (AUXADC_BASE + 0x014)
#define AUXADC_DAT2                     (AUXADC_BASE + 0x018)
#define AUXADC_DAT3                     (AUXADC_BASE + 0x01C)
#define AUXADC_DAT4                     (AUXADC_BASE + 0x020)
#define AUXADC_DAT5                     (AUXADC_BASE + 0x024)
#define AUXADC_DAT6                     (AUXADC_BASE + 0x028)
#define AUXADC_DAT7                     (AUXADC_BASE + 0x02C)
#define AUXADC_DAT8                     (AUXADC_BASE + 0x030)
#define AUXADC_DAT9                     (AUXADC_BASE + 0x034)

#define AUXADC_DET_CTL                  (AUXADC_BASE + 0x048)
#define AUXADC_DET_SEL                  (AUXADC_BASE + 0x04C)
#define AUXADC_DET_PERIOD               (AUXADC_BASE + 0x050)
#define AUXADC_DET_DEBT                 (AUXADC_BASE + 0x054)

#define AUXADC_QBIT                     (AUXADC_BASE + 0x058)

#define AUXADC_TDMA0_CNT                (AUXADC_BASE + 0x05C)
#define AUXADC_TDMA1_CNT                (AUXADC_BASE + 0x060)


//-----------------------------------------------------------------------------

/*AUXADC_SYNC on AUXADC_CON0*/
#define AUXADC_SYNC_CHAN(_line)         (0x0001<<_line)   /*Time event 1*/

/*AUXADC_IMM on AUXADC_CON1*/
#define AUXADC_IMM_CHAN(_line)          (0x0001<<_line)

/*AUXADC_SYN on AUXADC_CON2*/
#define AUXADC_SYN_BIT                  (0x0001)         /*Time event 0*/

/*AUXADC_CON3*/
#define AUXADC_CON3_STATUS_mask         (0x0001)
    #define AUXADC_STATUS_BUSY          (0x01)
    #define AUXADC_STATUS_IDLE          (0x00) 
//#define AUXADC_CON_CALI_MASK          (0x007c)
//#define AUXADC_CON_TESTDACMON         (0x0080)
#define AUXADC_CON3_AUTOCLR0_mask       (0x0100)
#define AUXADC_CON3_AUTOCLR0_offset     (8)
#define AUXADC_CON3_AUTOCLR1_mask       (0x0200)
#define AUXADC_CON3_AUTOCLR1_offset     (9)
#define AUXADC_CON3_PUWAIT_EN_mask      (0x0800)
#define AUXADC_CON3_PUWAIT_EN_offset    (11)
#define AUXADC_CON3_AUTOSET_mask        (0x8000)
#define AUXADC_CON3_AUTOSET_offset      (15)

/*AUXADC_DET_CTL*/
#define AUXADC_VOLT_INV_mask            (0x03FF)
#define AUXADC_VOLT_THRESHOLD_mask      (0x8000)
#define AUXADC_VOLT_THRESHOLD_offset    (15)

/*AUXADC_QBIT*/
#define AUXADC_QBIT_RATE_CNT2           (0x0010)
#define AUXADC_QBIT_RATE_CNT1           (0x0008)
#define AUXADC_QBIT_RATE_CNT0           (0x0000)

/*AUXADC_TDMA1_CNT*/
#define AUXADC_TDMA1_FORCETIMER_mask    (0x0200)
#define AUXADC_TDMA1_FORCETIMER_offset   (9)
#define AUXADC_TDMA1_ENABLECOUTER_mask  (0x0100)
#define AUXADC_TDMA1_ENABLECOUTER_offset (8)
#define AUXADC_TDMA1_TIMERCOUNT_mask    (0x00FF)

/*AUXADC_TDMA0_CNT*/
#define AUXADC_TDMA0_FORCETIMER_mask    (0x0200)
#define AUXADC_TDMA0_FORCETIMER_offset   (9)
#define AUXADC_TDMA0_ENABLECOUTER_mask  (0x0100)
#define AUXADC_TDMA0_ENABLECOUTER_offset (8)
#define AUXADC_TDMA0_TIMERCOUNT_mask    (0x00FF)


#endif   /*_MTK_DVT_TEST_ADC_HW_H*/

