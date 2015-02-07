

#include "../EplInc.h"

#ifndef _EPLCFGMA_H_
#define _EPLCFGMA_H_

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFGMA)) != 0)

#include "EplObdu.h"
#include "EplSdoComu.h"

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------
//define max number of timeouts for configuration of 1 device
#define EPL_CFGMA_MAX_TIMEOUT   3

//callbackfunction, called if configuration is finished
typedef void (PUBLIC * tfpEplCfgMaCb) (unsigned int uiNodeId_p,
				       tEplKernel Errorstate_p);

//State for configuartion manager Statemachine
typedef enum {
	// general states
	kEplCfgMaIdle = 0x0000,	// Configurationsprocess
	// is idle
	kEplCfgMaWaitForSdocEvent = 0x0001,	// wait until the last
	// SDOC is finisched
	kEplCfgMaSkipMappingSub0 = 0x0002,	// write Sub0 of mapping
	// parameter with 0

	kEplCfgMaFinished = 0x0004	// configuartion is finished
} tEplCfgState;

typedef enum {
	kEplCfgMaDcfTypSystecSeg = 0x00,
	kEplCfgMaDcfTypConDcf = 0x01,
	kEplCfgMaDcfTypDcf = 0x02,	// not supported
	kEplCfgMaDcfTypXdc = 0x03	// not supported
} tEplCfgMaDcfTyp;

typedef enum {
	kEplCfgMaCommon = 0,	// all other index
	kEplCfgMaPdoComm = 1,	// communication index
	kEplCfgMaPdoMapp = 2,	// mapping index
	kEplCfgMaPdoCommAfterMapp = 3,	// write PDO Cob-Id after mapping subindex 0(set PDO valid)

} tEplCfgMaIndexType;

//bitcoded answer about the last sdo transfer saved in m_SdocState
// also used to singal start of the State Maschine
typedef enum {
	kEplCfgMaSdocBusy = 0x00,	// SDOC activ
	kEplCfgMaSdocReady = 0x01,	// SDOC finished
	kEplCfgMaSdocTimeout = 0x02,	// SDOC Timeout
	kEplCfgMaSdocAbortReceived = 0x04,	// SDOC Abort, see Abortcode
	kEplCfgMaSdocStart = 0x08	// start State Mschine
} tEplSdocState;

//internal structure (instancetable for modul configuration manager)
typedef struct {
	tEplCfgState m_CfgState;	// state of the configuration state maschine
	tEplSdoComConHdl m_SdoComConHdl;	// handle for sdo connection
	DWORD m_dwLastAbortCode;
	unsigned int m_uiLastIndex;	// last index of configuration, to compair with actual index
	BYTE *m_pbConcise;	// Ptr to concise DCF
	BYTE *m_pbActualIndex;	// Ptr to actual index in the DCF segment
	tfpEplCfgMaCb m_pfnCfgMaCb;	// Ptr to CfgMa Callback, is call if configuration finished
	tEplKernel m_EplKernelError;	// errorcode
	DWORD m_dwNumValueCopy;	// numeric values are copied in this variable
	unsigned int m_uiPdoNodeId;	// buffer for PDO node id
	BYTE m_bNrOfMappedObject;	// number of mapped objects
	unsigned int m_uiNodeId;	// Epl node addresse
	tEplSdocState m_SdocState;	// bitcoded state of the SDO transfer
	unsigned int m_uiLastSubIndex;	// last subindex of configuration
	BOOL m_fOneTranferOk;	// atleased one transfer was successful
	BYTE m_bEventFlag;	// for Eventsignaling to the State Maschine
	DWORD m_dwCntObjectInDcf;	// number of Objects in DCF
	tEplCfgMaIndexType m_SkipCfg;	// TRUE if a adsitional Configurationprocess
	// have to insert e.g. PDO-mapping
	WORD m_wTimeOutCnt;	// Timeout Counter, break configuration is
	// m_wTimeOutCnt == CFGMA_MAX_TIMEOUT

} tEplCfgMaNode;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Function:    EplCfgMaInit()
//
// Description: Function creates first instance of Configuration Manager
//
// Parameters:
//
// Returns:     tEplKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaInit();

//---------------------------------------------------------------------------
// Function:    EplCfgMaAddInstance()
//
// Description: Function creates additional instance of Configuration Manager
//
// Parameters:
//
// Returns:     tEplKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaAddInstance();

//---------------------------------------------------------------------------
// Function:    EplCfgMaDelInstance()
//
// Description: Function delete instance of Configuration Manager
//
// Parameters:
//
// Returns:     tEplKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaDelInstance();

//---------------------------------------------------------------------------
// Function:    EplCfgMaStartConfig()
//
// Description: Function starts the configuration process
//              initialization the statemachine for CfgMa- process
//
// Parameters:  uiNodeId_p              = NodeId of the node to configure
//              pbConcise_p             = pointer to DCF
//              fpCfgMaCb_p             = pointer to callback function (should not be NULL)
//              SizeOfConcise_p         = size of DCF in BYTE -> for future use
//              DcfType_p               = type of the DCF
//
// Returns:     tCopKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaStartConfig(unsigned int uiNodeId_p,
				      BYTE * pbConcise_p,
				      tfpEplCfgMaCb fpCfgMaCb_p,
				      tEplObdSize SizeOfConcise_p,
				      tEplCfgMaDcfTyp DcfType_p);

//---------------------------------------------------------------------------
// Function:    CfgMaStartConfigurationNode()
//
// Description: Function started the configuration process
//              with the DCF from according OD-entry Subindex == bNodeId_p
//
// Parameters:  uiNodeId_p              = NodeId of the node to configure
//              fpCfgMaCb_p             = pointer to callback function (should not be NULL)
//              DcfType_p               = type of the DCF
//
// Returns:     tCopKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaStartConfigNode(unsigned int uiNodeId_p,
					  tfpEplCfgMaCb fpCfgMaCb_p,
					  tEplCfgMaDcfTyp DcfType_p);

//---------------------------------------------------------------------------
// Function:    EplCfgMaStartConfigNodeDcf()
//
// Description: Function starts the configuration process
//              and links the configuration data to the OD
//
// Parameters:  uiNodeId_p              = NodeId of the node to configure
//              pbConcise_p             = pointer to DCF
//              fpCfgMaCb_p             = pointer to callback function (should not be NULL)
//              SizeOfConcise_p         = size of DCF in BYTE -> for future use
//              DcfType_p               = type of the DCF
//
// Returns:     tCopKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaStartConfigNodeDcf(unsigned int uiNodeId_p,
					     BYTE * pbConcise_p,
					     tfpEplCfgMaCb fpCfgMaCb_p,
					     tEplObdSize SizeOfConcise_p,
					     tEplCfgMaDcfTyp DcfType_p);

//---------------------------------------------------------------------------
// Function:    EplCfgMaLinkDcf()
//
// Description: Function links the configuration data to the OD
//
// Parameters:  uiNodeId_p              = NodeId of the node to configure
//              pbConcise_p             = pointer to DCF
//              SizeOfConcise_p        = size of DCF in BYTE -> for future use
//              DcfType_p               = type of the DCF
//
// Returns:     tCopKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaLinkDcf(unsigned int uiNodeId_p,
				  BYTE * pbConcise_p,
				  tEplObdSize SizeOfConcise_p,
				  tEplCfgMaDcfTyp DcfType_p);

//---------------------------------------------------------------------------
// Function:    EplCfgMaCheckDcf()
//
// Description: Function check if there is allready a configuration file linked
//              to the OD (type is given by DcfType_p)
//
// Parameters:  uiNodeId_p              = NodeId
//              DcfType_p               = type of the DCF
//
// Returns:     tCopKernel              = error code
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplCfgMaCheckDcf(unsigned int uiNodeId_p,
				   tEplCfgMaDcfTyp DcfType_p);

#endif // #if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFGMA)) != 0)

#endif // _EPLCFGMA_H_

// EOF
