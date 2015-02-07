

#include "../EplSdo.h"

#ifndef _EPLSDOUDPU_H_
#define _EPLSDOUDPU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDO_UDP)) != 0)

tEplKernel PUBLIC EplSdoUdpuInit(tEplSequLayerReceiveCb fpReceiveCb_p);

tEplKernel PUBLIC EplSdoUdpuAddInstance(tEplSequLayerReceiveCb fpReceiveCb_p);

tEplKernel PUBLIC EplSdoUdpuDelInstance(void);

tEplKernel PUBLIC EplSdoUdpuConfig(unsigned long ulIpAddr_p,
				   unsigned int uiPort_p);

tEplKernel PUBLIC EplSdoUdpuInitCon(tEplSdoConHdl * pSdoConHandle_p,
				    unsigned int uiTargetNodeId_p);

tEplKernel PUBLIC EplSdoUdpuSendData(tEplSdoConHdl SdoConHandle_p,
				     tEplFrame * pSrcData_p,
				     DWORD dwDataSize_p);

tEplKernel PUBLIC EplSdoUdpuDelCon(tEplSdoConHdl SdoConHandle_p);

#endif // end of #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDO_UDP)) != 0)

#endif // #ifndef _EPLSDOUDPU_H_
