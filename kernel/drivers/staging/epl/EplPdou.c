

#include "EplInc.h"
//#include "user/EplPdouCal.h"
#include "user/EplObdu.h"
#include "user/EplPdou.h"
#include "EplSdoAc.h"

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_PDOU)) != 0)

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDU)) == 0) && (EPL_OBD_USE_KERNEL == FALSE)
#error "EPL PDOu module needs EPL module OBDU or OBDK!"
#endif

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

#define EPL_PDOU_OBD_IDX_RX_COMM_PARAM  0x1400
#define EPL_PDOU_OBD_IDX_RX_MAPP_PARAM  0x1600
#define EPL_PDOU_OBD_IDX_TX_COMM_PARAM  0x1800
#define EPL_PDOU_OBD_IDX_TX_MAPP_PARAM  0x1A00
#define EPL_PDOU_OBD_IDX_MAPP_PARAM     0x0200
#define EPL_PDOU_OBD_IDX_MASK           0xFF00
#define EPL_PDOU_PDO_ID_MASK            0x00FF

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// modul globale vars
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          C L A S S  EplPdou                                             */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
//
// Description:
//
//
/***************************************************************************/

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local vars
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

static tEplKernel EplPdouCheckPdoValidity(tEplObdCbParam MEM * pParam_p,
					  unsigned int uiIndex_p);

static void EplPdouDecodeObjectMapping(QWORD qwObjectMapping_p,
				       unsigned int *puiIndex_p,
				       unsigned int *puiSubIndex_p,
				       unsigned int *puiBitOffset_p,
				       unsigned int *puiBitSize_p);

static tEplKernel EplPdouCheckObjectMapping(QWORD qwObjectMapping_p,
					    tEplObdAccess AccessType_p,
					    DWORD * pdwAbortCode_p,
					    unsigned int *puiPdoSize_p);

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplPdouAddInstance()
//
// Description: add and initialize new instance of EPL stack
//
// Parameters:  none
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplPdouAddInstance(void)
{

	return kEplSuccessful;
}

//---------------------------------------------------------------------------
//
// Function:    EplPdouDelInstance()
//
// Description: deletes an instance of EPL stack
//
// Parameters:  none
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplPdouDelInstance(void)
{

	return kEplSuccessful;
}

//---------------------------------------------------------------------------
//
// Function:    EplPdouCbObdAccess
//
// Description: callback function for OD accesses
//
// Parameters:  pParam_p                = OBD parameter
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplPdouCbObdAccess(tEplObdCbParam MEM * pParam_p)
{
	tEplKernel Ret = kEplSuccessful;
	unsigned int uiPdoId;
	unsigned int uiIndexType;
	tEplObdSize ObdSize;
	BYTE bObjectCount;
	QWORD qwObjectMapping;
	tEplObdAccess AccessType;
	BYTE bMappSubindex;
	unsigned int uiCurPdoSize;
	WORD wMaxPdoSize;
	unsigned int uiSubIndex;

	// fetch PDO ID
	uiPdoId = pParam_p->m_uiIndex & EPL_PDOU_PDO_ID_MASK;

	// fetch object index type
	uiIndexType = pParam_p->m_uiIndex & EPL_PDOU_OBD_IDX_MASK;

	if (pParam_p->m_ObdEvent != kEplObdEvPreWrite) {	// read accesses, post write events etc. are OK
		pParam_p->m_dwAbortCode = 0;
		goto Exit;
	}
	// check index type
	switch (uiIndexType) {
	case EPL_PDOU_OBD_IDX_RX_COMM_PARAM:
		// RPDO communication parameter accessed
	case EPL_PDOU_OBD_IDX_TX_COMM_PARAM:
		{		// TPDO communication parameter accessed
			Ret = EplPdouCheckPdoValidity(pParam_p,
						      (EPL_PDOU_OBD_IDX_MAPP_PARAM
						       | pParam_p->m_uiIndex));
			if (Ret != kEplSuccessful) {	// PDO is valid or does not exist
				goto Exit;
			}

			goto Exit;
		}

	case EPL_PDOU_OBD_IDX_RX_MAPP_PARAM:
		{		// RPDO mapping parameter accessed

			AccessType = kEplObdAccWrite;
			break;
		}

	case EPL_PDOU_OBD_IDX_TX_MAPP_PARAM:
		{		// TPDO mapping parameter accessed

			AccessType = kEplObdAccRead;
			break;
		}

	default:
		{		// this callback function is only for
			// PDO mapping and communication parameters
			pParam_p->m_dwAbortCode = EPL_SDOAC_GENERAL_ERROR;
			goto Exit;
		}
	}

	// RPDO and TPDO mapping parameter accessed

	if (pParam_p->m_uiSubIndex == 0) {	// object mapping count accessed

		// PDO is enabled or disabled
		bObjectCount = *((BYTE *) pParam_p->m_pArg);

		if (bObjectCount == 0) {	// PDO shall be disabled

			// that is always possible
			goto Exit;
		}
		// PDO shall be enabled
		// it should have been disabled for this operation
		Ret = EplPdouCheckPdoValidity(pParam_p, pParam_p->m_uiIndex);
		if (Ret != kEplSuccessful) {	// PDO is valid or does not exist
			goto Exit;
		}

		if (AccessType == kEplObdAccWrite) {
			uiSubIndex = 0x04;	// PReqActPayloadLimit_U16
		} else {
			uiSubIndex = 0x05;	// PResActPayloadLimit_U16
		}

		// fetch maximum PDO size from Object 1F98h: NMT_CycleTiming_REC
		ObdSize = sizeof(wMaxPdoSize);
		Ret =
		    EplObduReadEntry(0x1F98, uiSubIndex, &wMaxPdoSize,
				     &ObdSize);
		if (Ret != kEplSuccessful) {	// other fatal error occured
			pParam_p->m_dwAbortCode = EPL_SDOAC_GENERAL_ERROR;
			goto Exit;
		}
		// check all objectmappings
		for (bMappSubindex = 1; bMappSubindex <= bObjectCount;
		     bMappSubindex++) {
			// read object mapping from OD
			ObdSize = sizeof(qwObjectMapping);	// QWORD
			Ret = EplObduReadEntry(pParam_p->m_uiIndex,
					       bMappSubindex, &qwObjectMapping,
					       &ObdSize);
			if (Ret != kEplSuccessful) {	// other fatal error occured
				pParam_p->m_dwAbortCode =
				    EPL_SDOAC_GENERAL_ERROR;
				goto Exit;
			}
			// check object mapping
			Ret = EplPdouCheckObjectMapping(qwObjectMapping,
							AccessType,
							&pParam_p->
							m_dwAbortCode,
							&uiCurPdoSize);
			if (Ret != kEplSuccessful) {	// illegal object mapping
				goto Exit;
			}

			if (uiCurPdoSize > wMaxPdoSize) {	// mapping exceeds object size
				pParam_p->m_dwAbortCode =
				    EPL_SDOAC_GENERAL_ERROR;
				Ret = kEplPdoVarNotFound;
			}

		}

	} else {		// ObjectMapping
		Ret = EplPdouCheckPdoValidity(pParam_p, pParam_p->m_uiIndex);
		if (Ret != kEplSuccessful) {	// PDO is valid or does not exist
			goto Exit;
		}
		// check existence of object and validity of object length

		qwObjectMapping = *((QWORD *) pParam_p->m_pArg);

		Ret = EplPdouCheckObjectMapping(qwObjectMapping,
						AccessType,
						&pParam_p->m_dwAbortCode,
						&uiCurPdoSize);

	}

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
// Function:    EplPdouCheckPdoValidity
//
// Description: check if PDO is valid
//
// Parameters:  pParam_p                = OBD parameter
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplPdouCheckPdoValidity(tEplObdCbParam MEM * pParam_p,
					  unsigned int uiIndex_p)
{
	tEplKernel Ret = kEplSuccessful;
	tEplObdSize ObdSize;
	BYTE bObjectCount;

	ObdSize = 1;
	// read number of mapped objects from OD; this indicates if the PDO is valid
	Ret = EplObduReadEntry(uiIndex_p, 0x00, &bObjectCount, &ObdSize);
	if (Ret != kEplSuccessful) {	// other fatal error occured
		pParam_p->m_dwAbortCode =
		    EPL_SDOAC_GEN_INTERNAL_INCOMPATIBILITY;
		goto Exit;
	}
	// entry read successfully
	if (bObjectCount != 0) {	// PDO in OD is still valid
		pParam_p->m_dwAbortCode = EPL_SDOAC_GEN_PARAM_INCOMPATIBILITY;
		Ret = kEplPdoNotExist;
		goto Exit;
	}

      Exit:
	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplPdouDecodeObjectMapping
//
// Description: decodes the given object mapping entry into index, subindex,
//              bit offset and bit size.
//
// Parameters:  qwObjectMapping_p       = object mapping entry
//              puiIndex_p              = [OUT] pointer to object index
//              puiSubIndex_p           = [OUT] pointer to subindex
//              puiBitOffset_p          = [OUT] pointer to bit offset
//              puiBitSize_p            = [OUT] pointer to bit size
//
// Returns:     (void)
//
// State:
//
//---------------------------------------------------------------------------

static void EplPdouDecodeObjectMapping(QWORD qwObjectMapping_p,
				       unsigned int *puiIndex_p,
				       unsigned int *puiSubIndex_p,
				       unsigned int *puiBitOffset_p,
				       unsigned int *puiBitSize_p)
{
	*puiIndex_p = (unsigned int)
	    (qwObjectMapping_p & 0x000000000000FFFFLL);

	*puiSubIndex_p = (unsigned int)
	    ((qwObjectMapping_p & 0x0000000000FF0000LL) >> 16);

	*puiBitOffset_p = (unsigned int)
	    ((qwObjectMapping_p & 0x0000FFFF00000000LL) >> 32);

	*puiBitSize_p = (unsigned int)
	    ((qwObjectMapping_p & 0xFFFF000000000000LL) >> 48);

}

//---------------------------------------------------------------------------
//
// Function:    EplPdouCheckObjectMapping
//
// Description: checks the given object mapping entry.
//
// Parameters:  qwObjectMapping_p       = object mapping entry
//              AccessType_p            = access type to mapped object:
//                                        write = RPDO and read = TPDO
//              puiPdoSize_p            = [OUT] pointer to covered PDO size
//                                        (offset + size) in byte;
//                                        0 if mapping failed
//              pdwAbortCode_p          = [OUT] pointer to SDO abort code;
//                                        0 if mapping is possible
//
// Returns:     tEplKernel              = error code
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplPdouCheckObjectMapping(QWORD qwObjectMapping_p,
					    tEplObdAccess AccessType_p,
					    DWORD * pdwAbortCode_p,
					    unsigned int *puiPdoSize_p)
{
	tEplKernel Ret = kEplSuccessful;
	tEplObdSize ObdSize;
	unsigned int uiIndex;
	unsigned int uiSubIndex;
	unsigned int uiBitOffset;
	unsigned int uiBitSize;
	tEplObdAccess AccessType;
	BOOL fNumerical;

	if (qwObjectMapping_p == 0) {	// discard zero value
		*puiPdoSize_p = 0;
		goto Exit;
	}
	// decode object mapping
	EplPdouDecodeObjectMapping(qwObjectMapping_p,
				   &uiIndex,
				   &uiSubIndex, &uiBitOffset, &uiBitSize);

	if ((uiBitOffset & 0x7) != 0x0) {	// bit mapping is not supported
		*pdwAbortCode_p = EPL_SDOAC_GENERAL_ERROR;
		Ret = kEplPdoGranularityMismatch;
		goto Exit;
	}

	if ((uiBitSize & 0x7) != 0x0) {	// bit mapping is not supported
		*pdwAbortCode_p = EPL_SDOAC_GENERAL_ERROR;
		Ret = kEplPdoGranularityMismatch;
		goto Exit;
	}
	// check access type
	Ret = EplObduGetAccessType(uiIndex, uiSubIndex, &AccessType);
	if (Ret != kEplSuccessful) {	// entry doesn't exist
		*pdwAbortCode_p = EPL_SDOAC_OBJECT_NOT_EXIST;
		goto Exit;
	}

	if ((AccessType & kEplObdAccPdo) == 0) {	// object is not mappable
		*pdwAbortCode_p = EPL_SDOAC_OBJECT_NOT_MAPPABLE;
		Ret = kEplPdoVarNotFound;
		goto Exit;
	}

	if ((AccessType & AccessType_p) == 0) {	// object is not writeable (RPDO) or readable (TPDO) respectively
		*pdwAbortCode_p = EPL_SDOAC_OBJECT_NOT_MAPPABLE;
		Ret = kEplPdoVarNotFound;
		goto Exit;
	}

	ObdSize = EplObduGetDataSize(uiIndex, uiSubIndex);
	if (ObdSize < (uiBitSize >> 3)) {	// object does not exist or has smaller size
		*pdwAbortCode_p = EPL_SDOAC_GENERAL_ERROR;
		Ret = kEplPdoVarNotFound;
	}

	Ret = EplObduIsNumerical(uiIndex, uiSubIndex, &fNumerical);
	if (Ret != kEplSuccessful) {	// entry doesn't exist
		*pdwAbortCode_p = EPL_SDOAC_OBJECT_NOT_EXIST;
		goto Exit;
	}

	if ((fNumerical != FALSE)
	    && ((uiBitSize >> 3) != ObdSize)) {
		// object is numerical,
		// therefor size has to fit, but it does not.
		*pdwAbortCode_p = EPL_SDOAC_GENERAL_ERROR;
		Ret = kEplPdoVarNotFound;
		goto Exit;
	}
	// calucaled needed PDO size
	*puiPdoSize_p = (uiBitOffset >> 3) + (uiBitSize >> 3);

      Exit:
	return Ret;
}

#endif // #if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_PDOU)) != 0)

// EOF
