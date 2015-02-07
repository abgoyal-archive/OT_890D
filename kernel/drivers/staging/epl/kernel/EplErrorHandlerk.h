

#ifndef _EPL_ERRORHANDLERK_H_
#define _EPL_ERRORHANDLERK_H_

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
tEplKernel PUBLIC EplErrorHandlerkInit(void);

// add instance
tEplKernel PUBLIC EplErrorHandlerkAddInstance(void);

// delete instance
tEplKernel PUBLIC EplErrorHandlerkDelInstance(void);

// processes error events
tEplKernel PUBLIC EplErrorHandlerkProcess(tEplEvent * pEvent_p);

#endif // #ifndef _EPL_ERRORHANDLERK_H_
