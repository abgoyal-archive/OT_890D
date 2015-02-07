

#ifndef _EPL_PDOK_H_
#define _EPL_PDOK_H_

#include "../EplPdo.h"
#include "../EplEvent.h"
#include "../EplDll.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

// process events from queue (PDOs/frames and SoA for synchronization)
tEplKernel EplPdokProcess(tEplEvent * pEvent_p);

// copies RPDO to event queue for processing
// is called by DLL in NMT_CS_READY_TO_OPERATE and NMT_CS_OPERATIONAL
// PDO needs not to be valid
tEplKernel EplPdokCbPdoReceived(tEplFrameInfo * pFrameInfo_p);

// posts pointer and size of TPDO to event queue
// is called by DLL in NMT_CS_PRE_OPERATIONAL_2,
//     NMT_CS_READY_TO_OPERATE and NMT_CS_OPERATIONAL
tEplKernel EplPdokCbPdoTransmitted(tEplFrameInfo * pFrameInfo_p);

// posts SoA event to queue
tEplKernel EplPdokCbSoa(tEplFrameInfo * pFrameInfo_p);

tEplKernel EplPdokAddInstance(void);

tEplKernel EplPdokDelInstance(void);

#endif // #ifndef _EPL_PDOK_H_
