

#ifndef _EPLNMTK_H_
#define _EPLNMTK_H_

#include "../EplNmt.h"
#include "EplEventk.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMTK)) != 0)
EPLDLLEXPORT tEplKernel PUBLIC EplNmtkInit(EPL_MCO_DECL_PTR_INSTANCE_PTR);

EPLDLLEXPORT tEplKernel PUBLIC
EplNmtkAddInstance(EPL_MCO_DECL_PTR_INSTANCE_PTR);

EPLDLLEXPORT tEplKernel PUBLIC
EplNmtkDelInstance(EPL_MCO_DECL_PTR_INSTANCE_PTR);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtkProcess(EPL_MCO_DECL_PTR_INSTANCE_PTR_
					      tEplEvent * pEvent_p);

EPLDLLEXPORT tEplNmtState PUBLIC
EplNmtkGetNmtState(EPL_MCO_DECL_PTR_INSTANCE_PTR);

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMTK)) != 0)

#endif // #ifndef _EPLNMTK_H_
