

#include "EplInc.h"
#include "EplFrame.h"
#include "EplSdoAc.h"

#ifndef _EPLSDO_H_
#define _EPLSDO_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------
// global defines
#ifndef EPL_SDO_MAX_PAYLOAD
#define EPL_SDO_MAX_PAYLOAD     256
#endif

// handle between Protocol Abstraction Layer and asynchronous SDO Sequence Layer
#define EPL_SDO_UDP_HANDLE      0x8000
#define EPL_SDO_ASND_HANDLE     0x4000
#define EPL_SDO_ASY_HANDLE_MASK 0xC000
#define EPL_SDO_ASY_INVALID_HDL 0x3FFF

// handle between  SDO Sequence Layer and sdo command layer
#define EPL_SDO_ASY_HANDLE      0x8000
#define EPL_SDO_PDO_HANDLE      0x4000
#define EPL_SDO_SEQ_HANDLE_MASK 0xC000
#define EPL_SDO_SEQ_INVALID_HDL 0x3FFF

#define EPL_ASND_HEADER_SIZE        4
//#define EPL_SEQ_HEADER_SIZE         4
#define EPL_ETHERNET_HEADER_SIZE    14

#define EPL_SEQ_NUM_MASK            0xFC

// size for send buffer and history
#define EPL_MAX_SDO_FRAME_SIZE      EPL_C_IP_MIN_MTU
// size for receive frame
// -> needed because SND-Kit sends up to 1518 Byte
//    without Sdo-Command: Maximum Segment Size
#define EPL_MAX_SDO_REC_FRAME_SIZE  EPL_C_IP_MAX_MTU

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------
// handle between Protocol Abstraction Layer and asynchronuus SDO Sequence Layer
typedef unsigned int tEplSdoConHdl;

// callback function pointer for Protocol Abstraction Layer to call
// asynchronuus SDO Sequence Layer
typedef tEplKernel(PUBLIC * tEplSequLayerReceiveCb) (tEplSdoConHdl ConHdl_p,
						     tEplAsySdoSeq *
						     pSdoSeqData_p,
						     unsigned int uiDataSize_p);

// handle between asynchronuus SDO Sequence Layer and SDO Command layer
typedef unsigned int tEplSdoSeqConHdl;

// callback function pointer for asynchronuus SDO Sequence Layer to call
// SDO Command layer for received data
typedef tEplKernel(PUBLIC *
		   tEplSdoComReceiveCb) (tEplSdoSeqConHdl SdoSeqConHdl_p,
					 tEplAsySdoCom * pAsySdoCom_p,
					 unsigned int uiDataSize_p);

// status of connection
typedef enum {
	kAsySdoConStateConnected = 0x00,
	kAsySdoConStateInitError = 0x01,
	kAsySdoConStateConClosed = 0x02,
	kAsySdoConStateAckReceived = 0x03,
	kAsySdoConStateFrameSended = 0x04,
	kAsySdoConStateTimeout = 0x05
} tEplAsySdoConState;

// callback function pointer for asynchronuus SDO Sequence Layer to call
// SDO Command layer for connection status
typedef tEplKernel(PUBLIC * tEplSdoComConCb) (tEplSdoSeqConHdl SdoSeqConHdl_p,
					      tEplAsySdoConState
					      AsySdoConState_p);

// handle between  SDO Command layer and application
typedef unsigned int tEplSdoComConHdl;

// status of connection
typedef enum {
	kEplSdoComTransferNotActive = 0x00,
	kEplSdoComTransferRunning = 0x01,
	kEplSdoComTransferTxAborted = 0x02,
	kEplSdoComTransferRxAborted = 0x03,
	kEplSdoComTransferFinished = 0x04,
	kEplSdoComTransferLowerLayerAbort = 0x05
} tEplSdoComConState;

// SDO Services and Command-Ids from DS 1.0.0 p.152
typedef enum {
	kEplSdoServiceNIL = 0x00,
	kEplSdoServiceWriteByIndex = 0x01,
	kEplSdoServiceReadByIndex = 0x02
	    //--------------------------------
	    // the following services are optional and
	    // not supported now
} tEplSdoServiceType;

// describes if read or write access
typedef enum {
	kEplSdoAccessTypeRead = 0x00,
	kEplSdoAccessTypeWrite = 0x01
} tEplSdoAccessType;

typedef enum {
	kEplSdoTypeAuto = 0x00,
	kEplSdoTypeUdp = 0x01,
	kEplSdoTypeAsnd = 0x02,
	kEplSdoTypePdo = 0x03
} tEplSdoType;

typedef enum {
	kEplSdoTransAuto = 0x00,
	kEplSdoTransExpedited = 0x01,
	kEplSdoTransSegmented = 0x02
} tEplSdoTransType;

// structure to inform application about finish of SDO transfer
typedef struct {
	tEplSdoComConHdl m_SdoComConHdl;
	tEplSdoComConState m_SdoComConState;
	DWORD m_dwAbortCode;
	tEplSdoAccessType m_SdoAccessType;
	unsigned int m_uiNodeId;	// NodeId of the target
	unsigned int m_uiTargetIndex;	// index which was accessed
	unsigned int m_uiTargetSubIndex;	// subindex which was accessed
	unsigned int m_uiTransferredByte;	// number of bytes transferred
	void *m_pUserArg;	// user definable argument pointer

} tEplSdoComFinished;

// callback function pointer to inform application about connection
typedef tEplKernel(PUBLIC * tEplSdoFinishedCb) (tEplSdoComFinished *
						pSdoComFinished_p);

// structure to init SDO transfer to Read or Write by Index
typedef struct {
	tEplSdoComConHdl m_SdoComConHdl;
	unsigned int m_uiIndex;
	unsigned int m_uiSubindex;
	void *m_pData;
	unsigned int m_uiDataSize;
	unsigned int m_uiTimeout;	// not used in this version
	tEplSdoAccessType m_SdoAccessType;
	tEplSdoFinishedCb m_pfnSdoFinishedCb;
	void *m_pUserArg;	// user definable argument pointer

} tEplSdoComTransParamByIndex;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#endif // #ifndef _EPLSDO_H_
