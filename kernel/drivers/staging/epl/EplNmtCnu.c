

#include "EplInc.h"
#include "user/EplNmtCnu.h"
#include "user/EplDlluCal.h"

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_CN)) != 0)

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

typedef struct {
	unsigned int m_uiNodeId;
	tEplNmtuCheckEventCallback m_pfnCheckEventCb;

} tEplNmtCnuInstance;

//---------------------------------------------------------------------------
// modul globale vars
//---------------------------------------------------------------------------

static tEplNmtCnuInstance EplNmtCnuInstance_g;

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

static tEplNmtCommand EplNmtCnuGetNmtCommand(tEplFrameInfo * pFrameInfo_p);

static BOOL EplNmtCnuNodeIdList(BYTE * pbNmtCommandDate_p);

static tEplKernel PUBLIC EplNmtCnuCommandCb(tEplFrameInfo * pFrameInfo_p);

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuInit
//
// Description: init the first instance of the module
//
//
//
// Parameters:      uiNodeId_p = NodeId of the local node
//
//
// Returns:         tEplKernel = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuInit(unsigned int uiNodeId_p)
{
	tEplKernel Ret;

	Ret = EplNmtCnuAddInstance(uiNodeId_p);

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuAddInstance
//
// Description: init the add new instance of the module
//
//
//
// Parameters:      uiNodeId_p = NodeId of the local node
//
//
// Returns:         tEplKernel = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuAddInstance(unsigned int uiNodeId_p)
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	// reset instance structure
	EPL_MEMSET(&EplNmtCnuInstance_g, 0, sizeof(EplNmtCnuInstance_g));

	// save nodeid
	EplNmtCnuInstance_g.m_uiNodeId = uiNodeId_p;

	// register callback-function for NMT-commands
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)
	Ret = EplDlluCalRegAsndService(kEplDllAsndNmtCommand,
				       EplNmtCnuCommandCb,
				       kEplDllAsndFilterLocal);
#endif

	return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuDelInstance
//
// Description: delte instance of the module
//
//
//
// Parameters:
//
//
// Returns:         tEplKernel = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuDelInstance()
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)
	// deregister callback function from DLL
	Ret = EplDlluCalRegAsndService(kEplDllAsndNmtCommand,
				       NULL, kEplDllAsndFilterNone);
#endif

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuSendNmtRequest
//
// Description: Send an NMT-Request to the MN
//
//
//
// Parameters:      uiNodeId_p = NodeId of the local node
//                  NmtCommand_p = requested NMT-Command
//
//
// Returns:         tEplKernel = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplNmtCnuSendNmtRequest(unsigned int uiNodeId_p,
						       tEplNmtCommand
						       NmtCommand_p)
{
	tEplKernel Ret;
	tEplFrameInfo NmtRequestFrameInfo;
	tEplFrame NmtRequestFrame;

	Ret = kEplSuccessful;

	// build frame
	EPL_MEMSET(&NmtRequestFrame.m_be_abDstMac[0], 0x00, sizeof(NmtRequestFrame.m_be_abDstMac));	// set by DLL
	EPL_MEMSET(&NmtRequestFrame.m_be_abSrcMac[0], 0x00, sizeof(NmtRequestFrame.m_be_abSrcMac));	// set by DLL
	AmiSetWordToBe(&NmtRequestFrame.m_be_wEtherType,
		       EPL_C_DLL_ETHERTYPE_EPL);
	AmiSetByteToLe(&NmtRequestFrame.m_le_bDstNodeId, (BYTE) EPL_C_ADR_MN_DEF_NODE_ID);	// node id of the MN
	AmiSetByteToLe(&NmtRequestFrame.m_le_bMessageType,
		       (BYTE) kEplMsgTypeAsnd);
	AmiSetByteToLe(&NmtRequestFrame.m_Data.m_Asnd.m_le_bServiceId,
		       (BYTE) kEplDllAsndNmtRequest);
	AmiSetByteToLe(&NmtRequestFrame.m_Data.m_Asnd.m_Payload.
		       m_NmtRequestService.m_le_bNmtCommandId,
		       (BYTE) NmtCommand_p);
	AmiSetByteToLe(&NmtRequestFrame.m_Data.m_Asnd.m_Payload.m_NmtRequestService.m_le_bTargetNodeId, (BYTE) uiNodeId_p);	// target for the nmt command
	EPL_MEMSET(&NmtRequestFrame.m_Data.m_Asnd.m_Payload.m_NmtRequestService.
		   m_le_abNmtCommandData[0], 0x00,
		   sizeof(NmtRequestFrame.m_Data.m_Asnd.m_Payload.
			  m_NmtRequestService.m_le_abNmtCommandData));

	// build info-structure
	NmtRequestFrameInfo.m_NetTime.m_dwNanoSec = 0;
	NmtRequestFrameInfo.m_NetTime.m_dwSec = 0;
	NmtRequestFrameInfo.m_pFrame = &NmtRequestFrame;
	NmtRequestFrameInfo.m_uiFrameSize = EPL_C_DLL_MINSIZE_NMTREQ;	// sizeof(NmtRequestFrame);

	// send NMT-Request
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)
	Ret = EplDlluCalAsyncSend(&NmtRequestFrameInfo,	// pointer to frameinfo
				  kEplDllAsyncReqPrioNmt);	// priority
#endif

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuRegisterStateChangeCb
//
// Description: register Callback-function go get informed about a
//              NMT-Change-State-Event
//
//
//
// Parameters:  pfnEplNmtStateChangeCb_p = functionpointer
//
//
// Returns:     tEplKernel  = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC
EplNmtCnuRegisterCheckEventCb(tEplNmtuCheckEventCallback
			      pfnEplNmtCheckEventCb_p)
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	// save callback-function in modul global var
	EplNmtCnuInstance_g.m_pfnCheckEventCb = pfnEplNmtCheckEventCb_p;

	return Ret;

}

//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuCommandCb
//
// Description: callback funktion for NMT-Commands
//
//
//
// Parameters:      pFrameInfo_p = Frame with the NMT-Commando
//
//
// Returns:         tEplKernel = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
static tEplKernel PUBLIC EplNmtCnuCommandCb(tEplFrameInfo * pFrameInfo_p)
{
	tEplKernel Ret = kEplSuccessful;
	tEplNmtCommand NmtCommand;
	BOOL fNodeIdInList;
	tEplNmtEvent NmtEvent = kEplNmtEventNoEvent;

	if (pFrameInfo_p == NULL) {
		Ret = kEplNmtInvalidFramePointer;
		goto Exit;
	}

	NmtCommand = EplNmtCnuGetNmtCommand(pFrameInfo_p);

	// check NMT-Command
	switch (NmtCommand) {

		//------------------------------------------------------------------------
		// plain NMT state commands
	case kEplNmtCmdStartNode:
		{		// send NMT-Event to state maschine kEplNmtEventStartNode
			NmtEvent = kEplNmtEventStartNode;
			break;
		}

	case kEplNmtCmdStopNode:
		{		// send NMT-Event to state maschine kEplNmtEventStopNode
			NmtEvent = kEplNmtEventStopNode;
			break;
		}

	case kEplNmtCmdEnterPreOperational2:
		{		// send NMT-Event to state maschine kEplNmtEventEnterPreOperational2
			NmtEvent = kEplNmtEventEnterPreOperational2;
			break;
		}

	case kEplNmtCmdEnableReadyToOperate:
		{		// send NMT-Event to state maschine kEplNmtEventEnableReadyToOperate
			NmtEvent = kEplNmtEventEnableReadyToOperate;
			break;
		}

	case kEplNmtCmdResetNode:
		{		// send NMT-Event to state maschine kEplNmtEventResetNode
			NmtEvent = kEplNmtEventResetNode;
			break;
		}

	case kEplNmtCmdResetCommunication:
		{		// send NMT-Event to state maschine kEplNmtEventResetCom
			NmtEvent = kEplNmtEventResetCom;
			break;
		}

	case kEplNmtCmdResetConfiguration:
		{		// send NMT-Event to state maschine kEplNmtEventResetConfig
			NmtEvent = kEplNmtEventResetConfig;
			break;
		}

	case kEplNmtCmdSwReset:
		{		// send NMT-Event to state maschine kEplNmtEventSwReset
			NmtEvent = kEplNmtEventSwReset;
			break;
		}

		//------------------------------------------------------------------------
		// extended NMT state commands

	case kEplNmtCmdStartNodeEx:
		{
			// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&
						(pFrameInfo_p->m_pFrame->m_Data.
						 m_Asnd.m_Payload.
						 m_NmtCommandService.
						 m_le_abNmtCommandData[0]));
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventStartNode;
			}
			break;
		}

	case kEplNmtCmdStopNodeEx:
		{		// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&pFrameInfo_p->m_pFrame->m_Data.
						m_Asnd.m_Payload.
						m_NmtCommandService.
						m_le_abNmtCommandData[0]);
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventStopNode;
			}
			break;
		}

	case kEplNmtCmdEnterPreOperational2Ex:
		{		// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&pFrameInfo_p->m_pFrame->m_Data.
						m_Asnd.m_Payload.
						m_NmtCommandService.
						m_le_abNmtCommandData[0]);
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventEnterPreOperational2;
			}
			break;
		}

	case kEplNmtCmdEnableReadyToOperateEx:
		{		// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&pFrameInfo_p->m_pFrame->m_Data.
						m_Asnd.m_Payload.
						m_NmtCommandService.
						m_le_abNmtCommandData[0]);
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventEnableReadyToOperate;
			}
			break;
		}

	case kEplNmtCmdResetNodeEx:
		{		// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&pFrameInfo_p->m_pFrame->m_Data.
						m_Asnd.m_Payload.
						m_NmtCommandService.
						m_le_abNmtCommandData[0]);
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventResetNode;
			}
			break;
		}

	case kEplNmtCmdResetCommunicationEx:
		{		// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&pFrameInfo_p->m_pFrame->m_Data.
						m_Asnd.m_Payload.
						m_NmtCommandService.
						m_le_abNmtCommandData[0]);
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventResetCom;
			}
			break;
		}

	case kEplNmtCmdResetConfigurationEx:
		{		// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&pFrameInfo_p->m_pFrame->m_Data.
						m_Asnd.m_Payload.
						m_NmtCommandService.
						m_le_abNmtCommandData[0]);
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventResetConfig;
			}
			break;
		}

	case kEplNmtCmdSwResetEx:
		{		// check if own nodeid is in EPL node list
			fNodeIdInList =
			    EplNmtCnuNodeIdList(&pFrameInfo_p->m_pFrame->m_Data.
						m_Asnd.m_Payload.
						m_NmtCommandService.
						m_le_abNmtCommandData[0]);
			if (fNodeIdInList != FALSE) {	// own nodeid in list
				// send event to process command
				NmtEvent = kEplNmtEventSwReset;
			}
			break;
		}

		//------------------------------------------------------------------------
		// NMT managing commands

		// TODO: add functions to process managing command (optional)

	case kEplNmtCmdNetHostNameSet:
		{
			break;
		}

	case kEplNmtCmdFlushArpEntry:
		{
			break;
		}

		//------------------------------------------------------------------------
		// NMT info services

		// TODO: forward event with infos to the application (optional)

	case kEplNmtCmdPublishConfiguredCN:
		{
			break;
		}

	case kEplNmtCmdPublishActiveCN:
		{
			break;
		}

	case kEplNmtCmdPublishPreOperational1:
		{
			break;
		}

	case kEplNmtCmdPublishPreOperational2:
		{
			break;
		}

	case kEplNmtCmdPublishReadyToOperate:
		{
			break;
		}

	case kEplNmtCmdPublishOperational:
		{
			break;
		}

	case kEplNmtCmdPublishStopped:
		{
			break;
		}

	case kEplNmtCmdPublishEmergencyNew:
		{
			break;
		}

	case kEplNmtCmdPublishTime:
		{
			break;
		}

		//-----------------------------------------------------------------------
		// error from MN
		// -> requested command not supported by MN
	case kEplNmtCmdInvalidService:
		{

			// TODO: errorevent to application
			break;
		}

		//------------------------------------------------------------------------
		// default
	default:
		{
			Ret = kEplNmtUnknownCommand;
			goto Exit;
		}

	}			// end of switch(NmtCommand)

	if (NmtEvent != kEplNmtEventNoEvent) {
		if (EplNmtCnuInstance_g.m_pfnCheckEventCb != NULL) {
			Ret = EplNmtCnuInstance_g.m_pfnCheckEventCb(NmtEvent);
			if (Ret == kEplReject) {
				Ret = kEplSuccessful;
				goto Exit;
			} else if (Ret != kEplSuccessful) {
				goto Exit;
			}
		}
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMTU)) != 0)
		Ret = EplNmtuNmtEvent(NmtEvent);
#endif
	}

      Exit:
	return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuGetNmtCommand()
//
// Description: returns the NMT-Command from the frame
//
//
//
// Parameters:      pFrameInfo_p = pointer to the Frame
//                                 with the NMT-Command
//
//
// Returns:         tEplNmtCommand = NMT-Command
//
//
// State:
//
//---------------------------------------------------------------------------
static tEplNmtCommand EplNmtCnuGetNmtCommand(tEplFrameInfo * pFrameInfo_p)
{
	tEplNmtCommand NmtCommand;
	tEplNmtCommandService *pNmtCommandService;

	pNmtCommandService =
	    &pFrameInfo_p->m_pFrame->m_Data.m_Asnd.m_Payload.
	    m_NmtCommandService;

	NmtCommand =
	    (tEplNmtCommand) AmiGetByteFromLe(&pNmtCommandService->
					      m_le_bNmtCommandId);

	return NmtCommand;
}

//---------------------------------------------------------------------------
//
// Function:    EplNmtCnuNodeIdList()
//
// Description: check if the own nodeid is set in EPL Node List
//
//
//
// Parameters:      pbNmtCommandDate_p = pointer to the data of the NMT Command
//
//
// Returns:         BOOL = TRUE if nodeid is set in EPL Node List
//                         FALSE if nodeid not set in EPL Node List
//
//
// State:
//
//---------------------------------------------------------------------------
static BOOL EplNmtCnuNodeIdList(BYTE * pbNmtCommandDate_p)
{
	BOOL fNodeIdInList;
	unsigned int uiByteOffset;
	BYTE bBitOffset;
	BYTE bNodeListByte;

	// get byte-offset of the own nodeid in NodeIdList
	// devide though 8
	uiByteOffset = (unsigned int)(EplNmtCnuInstance_g.m_uiNodeId >> 3);
	// get bitoffset
	bBitOffset = (BYTE) EplNmtCnuInstance_g.m_uiNodeId % 8;

	bNodeListByte = AmiGetByteFromLe(&pbNmtCommandDate_p[uiByteOffset]);
	if ((bNodeListByte & bBitOffset) == 0) {
		fNodeIdInList = FALSE;
	} else {
		fNodeIdInList = TRUE;
	}

	return fNodeIdInList;
}

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_CN)) != 0)

// EOF