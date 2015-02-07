

#include "EplInc.h"
#include "EplEvent.h"

#ifndef _EPLTIMER_H_
#define _EPLTIMER_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

// type for timer handle
typedef unsigned long tEplTimerHdl;

typedef struct {
	tEplEventSink m_EventSink;
	unsigned long m_ulArg;	// d.k.: converted to unsigned long because
	// it is never accessed as a pointer by the
	// timer module and the data the
	// pointer points to is not saved in any way.
	// It is just a value. The user is responsible
	// to store the data statically and convert
	// the pointer between address spaces.

} tEplTimerArg;

typedef struct {
	tEplTimerHdl m_TimerHdl;
	unsigned long m_ulArg;	// d.k.: converted to unsigned long because
	// it is never accessed as a pointer by the
	// timer module and the data the
	// pointer points to is not saved in any way.
	// It is just a value.

} tEplTimerEventArg;

typedef tEplKernel(PUBLIC * tEplTimerkCallback) (tEplTimerEventArg *
						 pEventArg_p);

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#endif // #ifndef _EPLTIMER_H_
