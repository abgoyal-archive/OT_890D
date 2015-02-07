

#ifndef _EPL_EVENTU_H_
#define _EPL_EVENTU_H_

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
tEplKernel PUBLIC EplEventuInit(tEplProcessEventCb pfnApiProcessEventCb_p);

// add instance
tEplKernel PUBLIC EplEventuAddInstance(tEplProcessEventCb
				       pfnApiProcessEventCb_p);

// delete instance
tEplKernel PUBLIC EplEventuDelInstance(void);

// Task that dispatches events in userspace
tEplKernel PUBLIC EplEventuProcess(tEplEvent * pEvent_p);

// post events from userspace
tEplKernel PUBLIC EplEventuPost(tEplEvent * pEvent_p);

// post errorevents from userspace
tEplKernel PUBLIC EplEventuPostError(tEplEventSource EventSource_p,
				     tEplKernel EplError_p,
				     unsigned int uiArgSize_p, void *pArg_p);

#endif // #ifndef _EPL_EVENTU_H_
