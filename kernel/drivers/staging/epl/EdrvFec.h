

#ifndef _EDRVFEC_H_
#define _EDRVFEC_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------
// do this in config header
#define TARGET_HARDWARE TGTHW_SPLC_CF54

// base addresses
#if ((TARGET_HARDWARE & TGT_CPU_MASK_) == TGT_CPU_5282)

#elif ((TARGET_HARDWARE & TGT_CPU_MASK_) == TGT_CPU_5485)

#else

#error 'ERROR: Target was never implemented!'

#endif

//---------------------------------------------------------------------------
// types
//---------------------------------------------------------------------------

// Rx and Tx buffer descriptor format
typedef struct {
	WORD m_wStatus;		// control / status  ---  used by edrv, do not change in application
	WORD m_wLength;		// transfer length
	BYTE *m_pbData;		// buffer address
} tBufferDescr;

#if ((TARGET_HARDWARE & TGT_CPU_MASK_) == TGT_CPU_5282)

#elif ((TARGET_HARDWARE & TGT_CPU_MASK_) == TGT_CPU_5485)

#endif

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#endif // #ifndef _EDRV_FEC_H_
