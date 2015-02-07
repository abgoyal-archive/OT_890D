

#include "../EplSdo.h"
#include "../EplDll.h"

#ifndef _EPLSDOASNDU_H_
#define _EPLSDOASNDU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDO_ASND)) != 0)

tEplKernel PUBLIC EplSdoAsnduInit(tEplSequLayerReceiveCb fpReceiveCb_p);

tEplKernel PUBLIC EplSdoAsnduAddInstance(tEplSequLayerReceiveCb fpReceiveCb_p);

tEplKernel PUBLIC EplSdoAsnduDelInstance(void);

tEplKernel PUBLIC EplSdoAsnduInitCon(tEplSdoConHdl * pSdoConHandle_p,
				     unsigned int uiTargetNodeId_p);

tEplKernel PUBLIC EplSdoAsnduSendData(tEplSdoConHdl SdoConHandle_p,
				      tEplFrame * pSrcData_p,
				      DWORD dwDataSize_p);

tEplKernel PUBLIC EplSdoAsnduDelCon(tEplSdoConHdl SdoConHandle_p);

#endif // end of #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDO_ASND)) != 0)

#endif // #ifndef _EPLSDOASNDU_H_
