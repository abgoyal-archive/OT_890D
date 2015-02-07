

#ifndef _EPL_DLLU_H_
#define _EPL_DLLU_H_

#include "../EplDll.h"

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

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)

tEplKernel EplDlluAddInstance(void);

tEplKernel EplDlluDelInstance(void);

tEplKernel EplDlluRegAsndService(tEplDllAsndServiceId ServiceId_p,
				 tEplDlluCbAsnd pfnDlluCbAsnd_p,
				 tEplDllAsndFilter Filter_p);

tEplKernel EplDlluAsyncSend(tEplFrameInfo * pFrameInfo_p,
			    tEplDllAsyncReqPriority Priority_p);

// processes asynch frames
tEplKernel EplDlluProcess(tEplFrameInfo * pFrameInfo_p);

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)

#endif // #ifndef _EPL_DLLU_H_
