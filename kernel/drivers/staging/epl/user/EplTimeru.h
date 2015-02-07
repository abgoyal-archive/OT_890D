

#include "../EplTimer.h"
#include "EplEventu.h"

#ifndef _EPLTIMERU_H_
#define _EPLTIMERU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplTimeruInit(void);

tEplKernel PUBLIC EplTimeruAddInstance(void);

tEplKernel PUBLIC EplTimeruDelInstance(void);

tEplKernel PUBLIC EplTimeruSetTimerMs(tEplTimerHdl * pTimerHdl_p,
				      unsigned long ulTime_p,
				      tEplTimerArg Argument_p);

tEplKernel PUBLIC EplTimeruModifyTimerMs(tEplTimerHdl * pTimerHdl_p,
					 unsigned long ulTime_p,
					 tEplTimerArg Argument_p);

tEplKernel PUBLIC EplTimeruDeleteTimer(tEplTimerHdl * pTimerHdl_p);

BOOL PUBLIC EplTimeruIsTimerActive(tEplTimerHdl TimerHdl_p);

#endif // #ifndef _EPLTIMERU_H_
