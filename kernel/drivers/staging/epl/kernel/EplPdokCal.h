

#ifndef _EPL_PDOKCAL_H_
#define _EPL_PDOKCAL_H_

#include "../EplInc.h"
//#include "EplPdo.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel EplPdokCalAddInstance(void);

tEplKernel EplPdokCalDelInstance(void);

// sets flag for validity of TPDOs in shared memory
tEplKernel EplPdokCalSetTpdosValid(BOOL fValid_p);

// gets flag for validity of TPDOs from shared memory
tEplKernel EplPdokCalAreTpdosValid(BOOL * pfValid_p);

#endif // #ifndef _EPL_PDOKCAL_H_
