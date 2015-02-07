

#ifndef _EPL_DLLCAL_H_
#define _EPL_DLLCAL_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#ifndef EPL_DLLCAL_BUFFER_ID_TX_NMT
#define EPL_DLLCAL_BUFFER_ID_TX_NMT     "EplSblDllCalTxNmt"
#endif

#ifndef EPL_DLLCAL_BUFFER_SIZE_TX_NMT
#define EPL_DLLCAL_BUFFER_SIZE_TX_NMT   32767
#endif

#ifndef EPL_DLLCAL_BUFFER_ID_TX_GEN
#define EPL_DLLCAL_BUFFER_ID_TX_GEN     "EplSblDllCalTxGen"
#endif

#ifndef EPL_DLLCAL_BUFFER_SIZE_TX_GEN
#define EPL_DLLCAL_BUFFER_SIZE_TX_GEN   32767
#endif

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef struct {
	tEplDllAsndServiceId m_ServiceId;
	tEplDllAsndFilter m_Filter;

} tEplDllCalAsndServiceIdFilter;

typedef struct {
	tEplDllReqServiceId m_Service;
	unsigned int m_uiNodeId;
	BYTE m_bSoaFlag1;

} tEplDllCalIssueRequest;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#endif // #ifndef _EPL_DLLKCAL_H_
