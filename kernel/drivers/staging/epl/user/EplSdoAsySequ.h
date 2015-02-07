

#include "../EplSdo.h"
#include "EplSdoUdpu.h"
#include "EplSdoAsndu.h"
#include "../EplEvent.h"
#include "EplTimeru.h"

#ifndef _EPLSDOASYSEQU_H_
#define _EPLSDOASYSEQU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsySeqInit(tEplSdoComReceiveCb fpSdoComCb_p,
				   tEplSdoComConCb fpSdoComConCb_p);

tEplKernel PUBLIC EplSdoAsySeqAddInstance(tEplSdoComReceiveCb fpSdoComCb_p,
					  tEplSdoComConCb fpSdoComConCb_p);

tEplKernel PUBLIC EplSdoAsySeqDelInstance(void);

tEplKernel PUBLIC EplSdoAsySeqInitCon(tEplSdoSeqConHdl * pSdoSeqConHdl_p,
				      unsigned int uiNodeId_p,
				      tEplSdoType SdoType);

tEplKernel PUBLIC EplSdoAsySeqSendData(tEplSdoSeqConHdl SdoSeqConHdl_p,
				       unsigned int uiDataSize_p,
				       tEplFrame * pData_p);

tEplKernel PUBLIC EplSdoAsySeqProcessEvent(tEplEvent * pEvent_p);

tEplKernel PUBLIC EplSdoAsySeqDelCon(tEplSdoSeqConHdl SdoSeqConHdl_p);

#endif // #ifndef _EPLSDOASYSEQU_H_
