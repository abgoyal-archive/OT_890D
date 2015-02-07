

#ifndef _EPL_VETH_H_
#define _EPL_VETH_H_

#include "EplDllk.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)

tEplKernel PUBLIC VEthAddInstance(tEplDllkInitParam * pInitParam_p);

tEplKernel PUBLIC VEthDelInstance(void);

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)

#endif // #ifndef _EPL_VETH_H_
