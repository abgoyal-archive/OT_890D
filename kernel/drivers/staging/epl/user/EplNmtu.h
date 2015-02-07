

#include "../EplNmt.h"
#include "EplEventu.h"

#ifndef _EPLNMTU_H_
#define _EPLNMTU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------
// nmt commands
typedef enum {
	// requestable ASnd ServiceIds    0x01..0x1F
	kEplNmtCmdIdentResponse = 0x01,
	kEplNmtCmdStatusResponse = 0x02,
	// plain NMT state commands       0x20..0x3F
	kEplNmtCmdStartNode = 0x21,
	kEplNmtCmdStopNode = 0x22,
	kEplNmtCmdEnterPreOperational2 = 0x23,
	kEplNmtCmdEnableReadyToOperate = 0x24,
	kEplNmtCmdResetNode = 0x28,
	kEplNmtCmdResetCommunication = 0x29,
	kEplNmtCmdResetConfiguration = 0x2A,
	kEplNmtCmdSwReset = 0x2B,
	// extended NMT state commands    0x40..0x5F
	kEplNmtCmdStartNodeEx = 0x41,
	kEplNmtCmdStopNodeEx = 0x42,
	kEplNmtCmdEnterPreOperational2Ex = 0x43,
	kEplNmtCmdEnableReadyToOperateEx = 0x44,
	kEplNmtCmdResetNodeEx = 0x48,
	kEplNmtCmdResetCommunicationEx = 0x49,
	kEplNmtCmdResetConfigurationEx = 0x4A,
	kEplNmtCmdSwResetEx = 0x4B,
	// NMT managing commands          0x60..0x7F
	kEplNmtCmdNetHostNameSet = 0x62,
	kEplNmtCmdFlushArpEntry = 0x63,
	// NMT info services              0x80..0xBF
	kEplNmtCmdPublishConfiguredCN = 0x80,
	kEplNmtCmdPublishActiveCN = 0x90,
	kEplNmtCmdPublishPreOperational1 = 0x91,
	kEplNmtCmdPublishPreOperational2 = 0x92,
	kEplNmtCmdPublishReadyToOperate = 0x93,
	kEplNmtCmdPublishOperational = 0x94,
	kEplNmtCmdPublishStopped = 0x95,
	kEplNmtCmdPublishEmergencyNew = 0xA0,
	kEplNmtCmdPublishTime = 0xB0,

	kEplNmtCmdInvalidService = 0xFF
} tEplNmtCommand;

typedef tEplKernel(PUBLIC *
		   tEplNmtuStateChangeCallback) (tEplEventNmtStateChange
						 NmtStateChange_p);

typedef tEplKernel(PUBLIC *
		   tEplNmtuCheckEventCallback) (tEplNmtEvent NmtEvent_p);

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMTU)) != 0)

EPLDLLEXPORT tEplKernel PUBLIC EplNmtuInit(void);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtuAddInstance(void);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtuDelInstance(void);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtuNmtEvent(tEplNmtEvent NmtEvent_p);

EPLDLLEXPORT tEplNmtState PUBLIC EplNmtuGetNmtState(void);

EPLDLLEXPORT tEplKernel PUBLIC EplNmtuProcessEvent(tEplEvent * pEplEvent_p);

EPLDLLEXPORT tEplKernel PUBLIC
EplNmtuRegisterStateChangeCb(tEplNmtuStateChangeCallback
			     pfnEplNmtStateChangeCb_p);

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMTU)) != 0)

#endif // #ifndef _EPLNMTU_H_
