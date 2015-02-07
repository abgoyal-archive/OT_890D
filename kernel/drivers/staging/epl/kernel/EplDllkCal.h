

#ifndef _EPL_DLLKCAL_H_
#define _EPL_DLLKCAL_H_

#include "../EplDll.h"
#include "../EplEvent.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef struct {
	unsigned long m_ulCurTxFrameCountGen;
	unsigned long m_ulCurTxFrameCountNmt;
	unsigned long m_ulCurRxFrameCount;
	unsigned long m_ulMaxTxFrameCountGen;
	unsigned long m_ulMaxTxFrameCountNmt;
	unsigned long m_ulMaxRxFrameCount;

} tEplDllkCalStatistics;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLK)) != 0)

tEplKernel EplDllkCalAddInstance(void);

tEplKernel EplDllkCalDelInstance(void);

tEplKernel EplDllkCalAsyncGetTxCount(tEplDllAsyncReqPriority * pPriority_p,
				     unsigned int *puiCount_p);
tEplKernel EplDllkCalAsyncGetTxFrame(void *pFrame_p,
				     unsigned int *puiFrameSize_p,
				     tEplDllAsyncReqPriority Priority_p);
// only frames with registered AsndServiceIds are passed to CAL
tEplKernel EplDllkCalAsyncFrameReceived(tEplFrameInfo * pFrameInfo_p);

tEplKernel EplDllkCalAsyncSend(tEplFrameInfo * pFrameInfo_p,
			       tEplDllAsyncReqPriority Priority_p);

tEplKernel EplDllkCalAsyncClearBuffer(void);

tEplKernel EplDllkCalGetStatistics(tEplDllkCalStatistics ** ppStatistics);

tEplKernel EplDllkCalProcess(tEplEvent * pEvent_p);

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

tEplKernel EplDllkCalAsyncClearQueues(void);

tEplKernel EplDllkCalIssueRequest(tEplDllReqServiceId Service_p,
				  unsigned int uiNodeId_p, BYTE bSoaFlag1_p);

tEplKernel EplDllkCalAsyncGetSoaRequest(tEplDllReqServiceId * pReqServiceId_p,
					unsigned int *puiNodeId_p);

tEplKernel EplDllkCalAsyncSetPendingRequests(unsigned int uiNodeId_p,
					     tEplDllAsyncReqPriority
					     AsyncReqPrio_p,
					     unsigned int uiCount_p);

#endif //(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLK)) != 0)

#endif // #ifndef _EPL_DLLKCAL_H_
