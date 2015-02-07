

#include "../EplLed.h"
#include "../EplNmt.h"
#include "EplEventu.h"

#ifndef _EPLLEDU_H_
#define _EPLLEDU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef tEplKernel(PUBLIC * tEplLeduStateChangeCallback) (tEplLedType LedType_p,
							  BOOL fOn_p);

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_LEDU)) != 0)

tEplKernel PUBLIC EplLeduInit(tEplLeduStateChangeCallback pfnCbStateChange_p);

tEplKernel PUBLIC EplLeduAddInstance(tEplLeduStateChangeCallback
				     pfnCbStateChange_p);

tEplKernel PUBLIC EplLeduDelInstance(void);

tEplKernel PUBLIC EplLeduCbNmtStateChange(tEplEventNmtStateChange
					  NmtStateChange_p);

tEplKernel PUBLIC EplLeduProcessEvent(tEplEvent * pEplEvent_p);

#endif // #if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_LEDU)) != 0)

#endif // #ifndef _EPLLEDU_H_
