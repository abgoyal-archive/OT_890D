

#ifndef _EPL_DLLUCAL_H_
#define _EPL_DLLUCAL_H_

#include "../EplDll.h"
#include "../EplEvent.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef tEplKernel(PUBLIC * tEplDlluCbAsnd) (tEplFrameInfo * pFrameInfo_p);

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel EplDlluCalAddInstance(void);

tEplKernel EplDlluCalDelInstance(void);

tEplKernel EplDlluCalRegAsndService(tEplDllAsndServiceId ServiceId_p,
				    tEplDlluCbAsnd pfnDlluCbAsnd_p,
				    tEplDllAsndFilter Filter_p);

tEplKernel EplDlluCalAsyncSend(tEplFrameInfo * pFrameInfo,
			       tEplDllAsyncReqPriority Priority_p);

tEplKernel EplDlluCalProcess(tEplEvent * pEvent_p);

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

tEplKernel EplDlluCalAddNode(tEplDllNodeInfo * pNodeInfo_p);

tEplKernel EplDlluCalDeleteNode(unsigned int uiNodeId_p);

tEplKernel EplDlluCalSoftDeleteNode(unsigned int uiNodeId_p);

tEplKernel EplDlluCalIssueRequest(tEplDllReqServiceId Service_p,
				  unsigned int uiNodeId_p, BYTE bSoaFlag1_p);

#endif

#endif // #ifndef _EPL_DLLUCAL_H_
