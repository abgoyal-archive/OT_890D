

#include "../EplTimer.h"
#include "../user/EplEventu.h"

#ifndef _EPLTIMERK_H_
#define _EPLTIMERK_H_

#if EPL_TIMER_USE_USER != FALSE
#include "../user/EplTimeru.h"
#endif

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#if EPL_TIMER_USE_USER != FALSE
#define EplTimerkInit           EplTimeruInit
#define EplTimerkAddInstance    EplTimeruAddInstance
#define EplTimerkDelInstance    EplTimeruDelInstance
#define EplTimerkSetTimerMs     EplTimeruSetTimerMs
#define EplTimerkModifyTimerMs  EplTimeruModifyTimerMs
#define EplTimerkDeleteTimer    EplTimeruDeleteTimer
#endif

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
#if EPL_TIMER_USE_USER == FALSE
tEplKernel PUBLIC EplTimerkInit(void);

tEplKernel PUBLIC EplTimerkAddInstance(void);

tEplKernel PUBLIC EplTimerkDelInstance(void);

tEplKernel PUBLIC EplTimerkSetTimerMs(tEplTimerHdl * pTimerHdl_p,
				      unsigned long ulTime_p,
				      tEplTimerArg Argument_p);

tEplKernel PUBLIC EplTimerkModifyTimerMs(tEplTimerHdl * pTimerHdl_p,
					 unsigned long ulTime_p,
					 tEplTimerArg Argument_p);

tEplKernel PUBLIC EplTimerkDeleteTimer(tEplTimerHdl * pTimerHdl_p);
#endif
#endif // #ifndef _EPLTIMERK_H_
