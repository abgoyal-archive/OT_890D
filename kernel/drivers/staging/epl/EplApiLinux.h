

#ifndef _EPL_API_LINUX_H_
#define _EPL_API_LINUX_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#define EPLLIN_DEV_NAME     "epl"	// used for "/dev" and "/proc" entry

//---------------------------------------------------------------------------
//  Commands for <ioctl>
//---------------------------------------------------------------------------

#define EPLLIN_CMD_INITIALIZE               0	// ulArg_p ~ tEplApiInitParam*
#define EPLLIN_CMD_PI_IN                    1	// ulArg_p ~ tEplApiProcessImage*
#define EPLLIN_CMD_PI_OUT                   2	// ulArg_p ~ tEplApiProcessImage*
#define EPLLIN_CMD_WRITE_OBJECT             3	// ulArg_p ~ tEplLinSdoObject*
#define EPLLIN_CMD_READ_OBJECT              4	// ulArg_p ~ tEplLinSdoObject*
#define EPLLIN_CMD_WRITE_LOCAL_OBJECT       5	// ulArg_p ~ tEplLinLocalObject*
#define EPLLIN_CMD_READ_LOCAL_OBJECT        6	// ulArg_p ~ tEplLinLocalObject*
#define EPLLIN_CMD_FREE_SDO_CHANNEL         7	// ulArg_p ~ tEplSdoComConHdl
#define EPLLIN_CMD_NMT_COMMAND              8	// ulArg_p ~ tEplNmtEvent
#define EPLLIN_CMD_GET_EVENT                9	// ulArg_p ~ tEplLinEvent*
#define EPLLIN_CMD_MN_TRIGGER_STATE_CHANGE 10	// ulArg_p ~ tEplLinNodeCmdObject*
#define EPLLIN_CMD_PI_SETUP                11	// ulArg_p ~ 0
#define EPLLIN_CMD_SHUTDOWN                12	// ulArg_p ~ 0

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef struct {
	unsigned int m_uiEventArgSize;
	tEplApiEventArg *m_pEventArg;
	tEplApiEventType *m_pEventType;
	tEplKernel m_RetCbEvent;

} tEplLinEvent;

typedef struct {
	tEplSdoComConHdl m_SdoComConHdl;
	BOOL m_fValidSdoComConHdl;
	unsigned int m_uiNodeId;
	unsigned int m_uiIndex;
	unsigned int m_uiSubindex;
	void *m_le_pData;
	unsigned int m_uiSize;
	tEplSdoType m_SdoType;
	void *m_pUserArg;

} tEplLinSdoObject;

typedef struct {
	unsigned int m_uiIndex;
	unsigned int m_uiSubindex;
	void *m_pData;
	unsigned int m_uiSize;

} tEplLinLocalObject;

typedef struct {
	unsigned int m_uiNodeId;
	tEplNmtNodeCommand m_NodeCommand;

} tEplLinNodeCmdObject;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#endif // #ifndef _EPL_API_LINUX_H_
