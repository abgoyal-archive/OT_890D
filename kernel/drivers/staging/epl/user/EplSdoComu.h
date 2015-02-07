

#include "../EplSdo.h"
#include "../EplObd.h"
#include "../EplSdoAc.h"
#include "EplObdu.h"
#include "EplSdoAsySequ.h"

#ifndef _EPLSDOCOMU_H_
#define _EPLSDOCOMU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoComInit(void);

tEplKernel PUBLIC EplSdoComAddInstance(void);

tEplKernel PUBLIC EplSdoComDelInstance(void);

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDOC)) != 0)

tEplKernel PUBLIC EplSdoComDefineCon(tEplSdoComConHdl * pSdoComConHdl_p,
				     unsigned int uiTargetNodeId_p,
				     tEplSdoType ProtType_p);

tEplKernel PUBLIC EplSdoComInitTransferByIndex(tEplSdoComTransParamByIndex *
					       pSdoComTransParam_p);

tEplKernel PUBLIC EplSdoComUndefineCon(tEplSdoComConHdl SdoComConHdl_p);

tEplKernel PUBLIC EplSdoComGetState(tEplSdoComConHdl SdoComConHdl_p,
				    tEplSdoComFinished * pSdoComFinished_p);

tEplKernel PUBLIC EplSdoComSdoAbort(tEplSdoComConHdl SdoComConHdl_p,
				    DWORD dwAbortCode_p);

#endif

// for future extention

#endif // #ifndef _EPLSDOCOMU_H_
