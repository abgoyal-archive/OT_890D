

#ifndef _EPL_DLLK_H_
#define _EPL_DLLK_H_

#include "../EplDll.h"
#include "../EplEvent.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef tEplKernel(*tEplDllkCbAsync) (tEplFrameInfo * pFrameInfo_p);

typedef struct {
	BYTE m_be_abSrcMac[6];

} tEplDllkInitParam;

// forward declaration
struct _tEdrvTxBuffer;

struct _tEplDllkNodeInfo {
	struct _tEplDllkNodeInfo *m_pNextNodeInfo;
	struct _tEdrvTxBuffer *m_pPreqTxBuffer;
	unsigned int m_uiNodeId;
	DWORD m_dwPresTimeout;
	unsigned long m_ulDllErrorEvents;
	tEplNmtState m_NmtState;
	WORD m_wPresPayloadLimit;
	BYTE m_be_abMacAddr[6];
	BYTE m_bSoaFlag1;
	BOOL m_fSoftDelete;	// delete node after error and ignore error

};

typedef struct _tEplDllkNodeInfo tEplDllkNodeInfo;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLK)) != 0)

tEplKernel EplDllkAddInstance(tEplDllkInitParam * pInitParam_p);

tEplKernel EplDllkDelInstance(void);

// called before NMT_GS_COMMUNICATING will be entered to configure fixed parameters
tEplKernel EplDllkConfig(tEplDllConfigParam * pDllConfigParam_p);

// set identity of local node (may be at any time, e.g. in case of hostname change)
tEplKernel EplDllkSetIdentity(tEplDllIdentParam * pDllIdentParam_p);

// process internal events and do work that cannot be done in interrupt-context
tEplKernel EplDllkProcess(tEplEvent * pEvent_p);

// registers handler for non-EPL frames
tEplKernel EplDllkRegAsyncHandler(tEplDllkCbAsync pfnDllkCbAsync_p);

// deregisters handler for non-EPL frames
tEplKernel EplDllkDeregAsyncHandler(tEplDllkCbAsync pfnDllkCbAsync_p);

// register C_DLL_MULTICAST_ASND in ethernet driver if any AsndServiceId is registered
tEplKernel EplDllkSetAsndServiceIdFilter(tEplDllAsndServiceId ServiceId_p,
					 tEplDllAsndFilter Filter_p);

// creates the buffer for a Tx frame and registers it to the ethernet driver
tEplKernel EplDllkCreateTxFrame(unsigned int *puiHandle_p,
				tEplFrame ** ppFrame_p,
				unsigned int *puiFrameSize_p,
				tEplMsgType MsgType_p,
				tEplDllAsndServiceId ServiceId_p);

tEplKernel EplDllkDeleteTxFrame(unsigned int uiHandle_p);

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

tEplKernel EplDllkAddNode(tEplDllNodeInfo * pNodeInfo_p);

tEplKernel EplDllkDeleteNode(unsigned int uiNodeId_p);

tEplKernel EplDllkSoftDeleteNode(unsigned int uiNodeId_p);

tEplKernel EplDllkSetFlag1OfNode(unsigned int uiNodeId_p, BYTE bSoaFlag1_p);

tEplKernel EplDllkGetFirstNodeInfo(tEplDllkNodeInfo ** ppNodeInfo_p);

#endif //(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLK)) != 0)

#endif // #ifndef _EPL_DLLK_H_
