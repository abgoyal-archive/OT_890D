

#ifndef _EPL_PDOU_H_
#define _EPL_PDOU_H_

#include "../EplPdo.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel EplPdouAddInstance(void);

tEplKernel EplPdouDelInstance(void);

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_PDOU)) != 0)
tEplKernel PUBLIC EplPdouCbObdAccess(tEplObdCbParam MEM * pParam_p);
#else
#define EplPdouCbObdAccess		NULL
#endif

// returns error if bPdoId_p is already valid

#endif // #ifndef _EPL_PDOU_H_
