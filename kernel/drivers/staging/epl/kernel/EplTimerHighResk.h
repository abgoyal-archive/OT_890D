

#include "../EplTimer.h"

#ifndef _EPLTIMERHIGHRESK_H_
#define _EPLTIMERHIGHRESK_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplTimerHighReskInit(void);

tEplKernel PUBLIC EplTimerHighReskAddInstance(void);

tEplKernel PUBLIC EplTimerHighReskDelInstance(void);

tEplKernel PUBLIC EplTimerHighReskSetTimerNs(tEplTimerHdl * pTimerHdl_p,
					     unsigned long long ullTimeNs_p,
					     tEplTimerkCallback pfnCallback_p,
					     unsigned long ulArgument_p,
					     BOOL fContinuously_p);

tEplKernel PUBLIC EplTimerHighReskModifyTimerNs(tEplTimerHdl * pTimerHdl_p,
						unsigned long long ullTimeNs_p,
						tEplTimerkCallback
						pfnCallback_p,
						unsigned long ulArgument_p,
						BOOL fContinuously_p);

tEplKernel PUBLIC EplTimerHighReskDeleteTimer(tEplTimerHdl * pTimerHdl_p);

#endif // #ifndef _EPLTIMERHIGHRESK_H_
