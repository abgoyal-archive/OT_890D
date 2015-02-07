

#ifndef _EPL_EVENTK_H_
#define _EPL_EVENTK_H_

#include "../EplEvent.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

// init function
tEplKernel PUBLIC EplEventkInit(tEplSyncCb fpSyncCb);

// add instance
tEplKernel PUBLIC EplEventkAddInstance(tEplSyncCb fpSyncCb);

// delete instance
tEplKernel PUBLIC EplEventkDelInstance(void);

// Kernelthread that dispatches events in kernelspace
tEplKernel PUBLIC EplEventkProcess(tEplEvent * pEvent_p);

// post events from kernelspace
tEplKernel PUBLIC EplEventkPost(tEplEvent * pEvent_p);

// post errorevents from kernelspace
tEplKernel PUBLIC EplEventkPostError(tEplEventSource EventSource_p,
				     tEplKernel EplError_p,
				     unsigned int uiArgSize_p, void *pArg_p);

#endif // #ifndef _EPL_EVENTK_H_
