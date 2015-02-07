

#include "EplNmtu.h"
#include "../EplDll.h"
#include "../EplFrame.h"

#ifndef _EPLNMTCNU_H_
#define _EPLNMTCNU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_CN)) != 0)

EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuInit(unsigned int uiNodeId_p);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuAddInstance(unsigned int uiNodeId_p);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuDelInstance(void);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuSendNmtRequest(unsigned int uiNodeId_p,
						       tEplNmtCommand
						       NmtCommand_p);

EPLDLLEXPORT tEplKernel PUBLIC
EplNmtCnuRegisterCheckEventCb(tEplNmtuCheckEventCallback
			      pfnEplNmtCheckEventCb_p);

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_CN)) != 0)

#endif // #ifndef _EPLNMTCNU_H_
