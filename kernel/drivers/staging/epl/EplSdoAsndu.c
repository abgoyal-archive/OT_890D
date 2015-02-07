

#include "user/EplSdoAsndu.h"
#include "user/EplDlluCal.h"

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDO_ASND)) != 0)

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

#ifndef EPL_SDO_MAX_CONNECTION_ASND
#define EPL_SDO_MAX_CONNECTION_ASND 5
#endif

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

// instance table
typedef struct {
	unsigned int m_auiSdoAsndConnection[EPL_SDO_MAX_CONNECTION_ASND];
	tEplSequLayerReceiveCb m_fpSdoAsySeqCb;

} tEplSdoAsndInstance;

//---------------------------------------------------------------------------
// modul globale vars
//---------------------------------------------------------------------------

static tEplSdoAsndInstance SdoAsndInstance_g;

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplSdoAsnduCb(tEplFrameInfo * pFrameInfo_p);

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          C L A S S  <EPL SDO-Asnd Protocolabstraction layer>            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
//
// Description: EPL SDO-Asnd Protocolabstraction layer
//
//
/***************************************************************************/

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplSdoAsnduInit
//
// Description: init first instance of the module
//
//
//
// Parameters:  pReceiveCb_p    =   functionpointer to Sdo-Sequence layer
//                                  callback-function
//
//
// Returns:     tEplKernel  = Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsnduInit(tEplSequLayerReceiveCb fpReceiveCb_p)
{
	tEplKernel Ret;

	Ret = EplSdoAsnduAddInstance(fpReceiveCb_p);

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplSdoAsnduAddInstance
//
// Description: init additional instance of the module
//
//
//
// Parameters:  pReceiveCb_p    =   functionpointer to Sdo-Sequence layer
//                                  callback-function
//
//
// Returns:     tEplKernel  = Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsnduAddInstance(tEplSequLayerReceiveCb fpReceiveCb_p)
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	// init control structure
	EPL_MEMSET(&SdoAsndInstance_g, 0x00, sizeof(SdoAsndInstance_g));

	// save pointer to callback-function
	if (fpReceiveCb_p != NULL) {
		SdoAsndInstance_g.m_fpSdoAsySeqCb = fpReceiveCb_p;
	} else {
		Ret = kEplSdoUdpMissCb;
	}

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)
	Ret = EplDlluCalRegAsndService(kEplDllAsndSdo,
				       EplSdoAsnduCb, kEplDllAsndFilterLocal);
#endif

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplSdoAsnduDelInstance
//
// Description: del instance of the module
//              del socket and del Listen-Thread
//
//
//
// Parameters:
//
//
// Returns:     tEplKernel  = Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsnduDelInstance()
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)
	// deregister callback function from DLL
	Ret = EplDlluCalRegAsndService(kEplDllAsndSdo,
				       NULL, kEplDllAsndFilterNone);
#endif

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplSdoAsnduInitCon
//
// Description: init a new connect
//
//
//
// Parameters:  pSdoConHandle_p = pointer for the new connection handle
//              uiTargetNodeId_p = NodeId of the target node
//
//
// Returns:     tEplKernel  = Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsnduInitCon(tEplSdoConHdl * pSdoConHandle_p,
				     unsigned int uiTargetNodeId_p)
{
	tEplKernel Ret;
	unsigned int uiCount;
	unsigned int uiFreeCon;
	unsigned int *puiConnection;

	Ret = kEplSuccessful;

	if ((uiTargetNodeId_p == EPL_C_ADR_INVALID)
	    || (uiTargetNodeId_p >= EPL_C_ADR_BROADCAST)) {
		Ret = kEplSdoAsndInvalidNodeId;
		goto Exit;
	}
	// get free entry in control structure
	uiCount = 0;
	uiFreeCon = EPL_SDO_MAX_CONNECTION_ASND;
	puiConnection = &SdoAsndInstance_g.m_auiSdoAsndConnection[0];
	while (uiCount < EPL_SDO_MAX_CONNECTION_ASND) {
		if (*puiConnection == uiTargetNodeId_p) {	// existing connection to target node found
			// save handle for higher layer
			*pSdoConHandle_p = (uiCount | EPL_SDO_ASND_HANDLE);

			goto Exit;
		} else if (*puiConnection == 0) {	// free entry-> save target nodeId
			uiFreeCon = uiCount;
		}
		uiCount++;
		puiConnection++;
	}

	if (uiFreeCon == EPL_SDO_MAX_CONNECTION_ASND) {
		// no free connection
		Ret = kEplSdoAsndNoFreeHandle;
	} else {
		puiConnection =
		    &SdoAsndInstance_g.m_auiSdoAsndConnection[uiFreeCon];
		*puiConnection = uiTargetNodeId_p;
		// save handle for higher layer
		*pSdoConHandle_p = (uiFreeCon | EPL_SDO_ASND_HANDLE);

		goto Exit;
	}

      Exit:
	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplSdoAsnduSendData
//
// Description: send data using exisiting connection
//
//
//
// Parameters:  SdoConHandle_p  = connection handle
//              pSrcData_p      = pointer to data
//              dwDataSize_p    = number of databyte
//                                  -> without asnd-header!!!
//
// Returns:     tEplKernel  = Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsnduSendData(tEplSdoConHdl SdoConHandle_p,
				      tEplFrame * pSrcData_p,
				      DWORD dwDataSize_p)
{
	tEplKernel Ret;
	unsigned int uiArray;
	tEplFrameInfo FrameInfo;

	Ret = kEplSuccessful;

	uiArray = (SdoConHandle_p & ~EPL_SDO_ASY_HANDLE_MASK);

	if (uiArray > EPL_SDO_MAX_CONNECTION_ASND) {
		Ret = kEplSdoAsndInvalidHandle;
		goto Exit;
	}
	// fillout Asnd header
	// own node id not needed -> filled by DLL

	// set message type
	AmiSetByteToLe(&pSrcData_p->m_le_bMessageType, (BYTE) kEplMsgTypeAsnd);	// ASnd == 0x06
	// target node id
	AmiSetByteToLe(&pSrcData_p->m_le_bDstNodeId,
		       (BYTE) SdoAsndInstance_g.
		       m_auiSdoAsndConnection[uiArray]);
	// set source-nodeid (filled by DLL 0)
	AmiSetByteToLe(&pSrcData_p->m_le_bSrcNodeId, 0x00);

	// calc size
	dwDataSize_p += EPL_ASND_HEADER_SIZE;

	// send function of DLL
	FrameInfo.m_uiFrameSize = dwDataSize_p;
	FrameInfo.m_pFrame = pSrcData_p;
	EPL_MEMSET(&FrameInfo.m_NetTime, 0x00, sizeof(tEplNetTime));
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLU)) != 0)
	Ret = EplDlluCalAsyncSend(&FrameInfo, kEplDllAsyncReqPrioGeneric);
#endif

      Exit:
	return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplSdoAsnduDelCon
//
// Description: delete connection from intern structure
//
//
//
// Parameters:  SdoConHandle_p  = connection handle
//
// Returns:     tEplKernel  = Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsnduDelCon(tEplSdoConHdl SdoConHandle_p)
{
	tEplKernel Ret;
	unsigned int uiArray;

	Ret = kEplSuccessful;

	uiArray = (SdoConHandle_p & ~EPL_SDO_ASY_HANDLE_MASK);
	// check parameter
	if (uiArray > EPL_SDO_MAX_CONNECTION_ASND) {
		Ret = kEplSdoAsndInvalidHandle;
		goto Exit;
	}
	// set target nodeId to 0
	SdoAsndInstance_g.m_auiSdoAsndConnection[uiArray] = 0;

      Exit:
	return Ret;
}

//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplSdoAsnduCb
//
// Description: callback function for SDO ASnd frames
//
//
//
// Parameters:      pFrameInfo_p = Frame with SDO payload
//
//
// Returns:         tEplKernel = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoAsnduCb(tEplFrameInfo * pFrameInfo_p)
{
	tEplKernel Ret = kEplSuccessful;
	unsigned int uiCount;
	unsigned int *puiConnection;
	unsigned int uiNodeId;
	unsigned int uiFreeEntry = 0xFFFF;
	tEplSdoConHdl SdoConHdl;
	tEplFrame *pFrame;

	pFrame = pFrameInfo_p->m_pFrame;

	uiNodeId = AmiGetByteFromLe(&pFrame->m_le_bSrcNodeId);

	// search corresponding entry in control structure
	uiCount = 0;
	puiConnection = &SdoAsndInstance_g.m_auiSdoAsndConnection[0];
	while (uiCount < EPL_SDO_MAX_CONNECTION_ASND) {
		if (uiNodeId == *puiConnection) {
			break;
		} else if ((*puiConnection == 0)
			   && (uiFreeEntry == 0xFFFF)) {	// free entry
			uiFreeEntry = uiCount;
		}
		uiCount++;
		puiConnection++;
	}

	if (uiCount == EPL_SDO_MAX_CONNECTION_ASND) {
		if (uiFreeEntry != 0xFFFF) {
			puiConnection =
			    &SdoAsndInstance_g.
			    m_auiSdoAsndConnection[uiFreeEntry];
			*puiConnection = uiNodeId;
			uiCount = uiFreeEntry;
		} else {
			EPL_DBGLVL_SDO_TRACE0
			    ("EplSdoAsnduCb(): no free handle\n");
			goto Exit;
		}
	}
//    if (uiNodeId == *puiConnection)
	{			// entry found or created
		SdoConHdl = (uiCount | EPL_SDO_ASND_HANDLE);

		SdoAsndInstance_g.m_fpSdoAsySeqCb(SdoConHdl,
						  &pFrame->m_Data.m_Asnd.
						  m_Payload.m_SdoSequenceFrame,
						  (pFrameInfo_p->m_uiFrameSize -
						   18));
	}

      Exit:
	return Ret;

}

#endif // end of #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDO_ASND)) != 0)
// EOF
