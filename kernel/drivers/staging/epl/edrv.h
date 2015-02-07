

#ifndef _EDRV_H_
#define _EDRV_H_

#include "EplInc.h"
#include "EplFrame.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------
// --------------------------------------------------------------------------
#define MAX_ETH_DATA_SIZE       1500
#define MIN_ETH_DATA_SIZE         46

#define ETH_HDR_OFFSET 	 0	// Ethernet header at the top of the frame
#define ETH_HDR_SIZE	14	// size of Ethernet header
#define MIN_ETH_SIZE     (MIN_ETH_DATA_SIZE + ETH_HDR_SIZE)	// without CRC

#define ETH_CRC_SIZE	 4	// size of Ethernet CRC, i.e. FCS

//---------------------------------------------------------------------------
// types
//---------------------------------------------------------------------------

// position of a buffer in an ethernet-frame
typedef enum {
	kEdrvBufferFirstInFrame = 0x01,	// first data buffer in an ethernet frame
	kEdrvBufferMiddleInFrame = 0x02,	// a middle data buffer in an ethernet frame
	kEdrvBufferLastInFrame = 0x04	// last data buffer in an ethernet frame
} tEdrvBufferInFrame;

// format of a tx-buffer
typedef struct _tEdrvTxBuffer {
	tEplMsgType m_EplMsgType;	// IN: type of EPL message, set by calling function
	unsigned int m_uiTxMsgLen;	// IN: length of message to be send (set for each transmit call)
	// ----------------------
	unsigned int m_uiBufferNumber;	// OUT: number of the buffer, set by ethernetdriver
	BYTE *m_pbBuffer;	// OUT: pointer to the buffer, set by ethernetdriver
	tEplNetTime m_NetTime;	// OUT: Timestamp of end of transmission, set by ethernetdriver
	// ----------------------
	unsigned int m_uiMaxBufferLen;	// IN/OUT: maximum length of the buffer
} tEdrvTxBuffer;

// format of a rx-buffer
typedef struct _tEdrvRxBuffer {
	tEdrvBufferInFrame m_BufferInFrame;	// OUT position of received buffer in an ethernet-frame
	unsigned int m_uiRxMsgLen;	// OUT: length of received buffer (without CRC)
	BYTE *m_pbBuffer;	// OUT: pointer to the buffer, set by ethernetdriver
	tEplNetTime m_NetTime;	// OUT: Timestamp of end of receiption

} tEdrvRxBuffer;

//typedef void (*tEdrvRxHandler) (BYTE bBufferInFrame_p, tBufferDescr * pbBuffer_p);
//typedef void (*tEdrvRxHandler) (BYTE bBufferInFrame_p, BYTE * pbEthernetData_p, WORD wDataLen_p);
typedef void (*tEdrvRxHandler) (tEdrvRxBuffer * pRxBuffer_p);
typedef void (*tEdrvTxHandler) (tEdrvTxBuffer * pTxBuffer_p);

// format of init structure
typedef struct {
	BYTE m_abMyMacAddr[6];	// the own MAC address

//    BYTE            m_bNoOfRxBuffDescr;     // number of entries in rx bufferdescriptor table
//    tBufferDescr *  m_pRxBuffDescrTable;    // rx bufferdescriptor table
//    WORD            m_wRxBufferSize;        // size of the whole rx buffer

	tEdrvRxHandler m_pfnRxHandler;
	tEdrvTxHandler m_pfnTxHandler;

} tEdrvInitParam;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel EdrvInit(tEdrvInitParam * pEdrvInitParam_p);

tEplKernel EdrvShutdown(void);

tEplKernel EdrvDefineRxMacAddrEntry(BYTE * pbMacAddr_p);
tEplKernel EdrvUndefineRxMacAddrEntry(BYTE * pbMacAddr_p);

//tEplKernel EdrvDefineUnicastEntry     (BYTE * pbUCEntry_p);
//tEplKernel EdrvUndfineUnicastEntry    (BYTE * pbUCEntry_p);

tEplKernel EdrvAllocTxMsgBuffer(tEdrvTxBuffer * pBuffer_p);
tEplKernel EdrvReleaseTxMsgBuffer(tEdrvTxBuffer * pBuffer_p);

//tEplKernel EdrvWriteMsg               (tBufferDescr * pbBuffer_p);
tEplKernel EdrvSendTxMsg(tEdrvTxBuffer * pBuffer_p);
tEplKernel EdrvTxMsgReady(tEdrvTxBuffer * pBuffer_p);
tEplKernel EdrvTxMsgStart(tEdrvTxBuffer * pBuffer_p);

//tEplKernel EdrvReadMsg                (void);

// interrupt handler called by target specific interrupt handler
void EdrvInterruptHandler(void);

#endif // #ifndef _EDRV_H_
