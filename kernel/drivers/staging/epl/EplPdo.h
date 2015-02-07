

#ifndef _EPL_PDO_H_
#define _EPL_PDO_H_

#include "EplInc.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

// invalid PDO-NodeId
#define EPL_PDO_INVALID_NODE_ID     0xFF
// NodeId for PReq RPDO
#define EPL_PDO_PREQ_NODE_ID        0x00
// NodeId for PRes TPDO
#define EPL_PDO_PRES_NODE_ID        0x00

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef struct {
	void *m_pVar;
	WORD m_wOffset;		// in Bits
	WORD m_wSize;		// in Bits
	BOOL m_fNumeric;	// numeric value -> use AMI functions

} tEplPdoMapping;

typedef struct {
	unsigned int m_uiSizeOfStruct;
	unsigned int m_uiPdoId;
	unsigned int m_uiNodeId;
	// 0xFF=invalid, RPDO: 0x00=PReq, localNodeId=PRes, remoteNodeId=PRes
	//               TPDO: 0x00=PRes, MN: CnNodeId=PReq

	BOOL m_fTxRx;
	BYTE m_bMappingVersion;
	unsigned int m_uiMaxMappingEntries;	// maximum number of mapping entries, i.e. size of m_aPdoMapping
	tEplPdoMapping m_aPdoMapping[1];

} tEplPdoParam;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#endif // #ifndef _EPL_PDO_H_
