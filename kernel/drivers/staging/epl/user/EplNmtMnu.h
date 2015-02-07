

#include "EplNmtu.h"

#ifndef _EPLNMTMNU_H_
#define _EPLNMTMNU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef tEplKernel(PUBLIC * tEplNmtMnuCbNodeEvent) (unsigned int uiNodeId_p,
						    tEplNmtNodeEvent
						    NodeEvent_p,
						    tEplNmtState NmtState_p,
						    WORD wErrorCode_p,
						    BOOL fMandatory_p);

typedef tEplKernel(PUBLIC *
		   tEplNmtMnuCbBootEvent) (tEplNmtBootEvent BootEvent_p,
					   tEplNmtState NmtState_p,
					   WORD wErrorCode_p);

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

tEplKernel EplNmtMnuInit(tEplNmtMnuCbNodeEvent pfnCbNodeEvent_p,
			 tEplNmtMnuCbBootEvent pfnCbBootEvent_p);

tEplKernel EplNmtMnuAddInstance(tEplNmtMnuCbNodeEvent pfnCbNodeEvent_p,
				tEplNmtMnuCbBootEvent pfnCbBootEvent_p);

tEplKernel EplNmtMnuDelInstance(void);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtMnuProcessEvent(tEplEvent * pEvent_p);

tEplKernel EplNmtMnuSendNmtCommand(unsigned int uiNodeId_p,
				   tEplNmtCommand NmtCommand_p);

tEplKernel EplNmtMnuTriggerStateChange(unsigned int uiNodeId_p,
				       tEplNmtNodeCommand NodeCommand_p);

tEplKernel PUBLIC EplNmtMnuCbNmtStateChange(tEplEventNmtStateChange
					    NmtStateChange_p);

tEplKernel PUBLIC EplNmtMnuCbCheckEvent(tEplNmtEvent NmtEvent_p);

tEplKernel PUBLIC EplNmtMnuGetDiagnosticInfo(unsigned int
					     *puiMandatorySlaveCount_p,
					     unsigned int
					     *puiSignalSlaveCount_p,
					     WORD * pwFlags_p);

#endif

#endif // #ifndef _EPLNMTMNU_H_
