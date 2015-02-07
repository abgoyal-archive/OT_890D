

#ifndef _EPL_ERRORDEF_H_
#define _EPL_ERRORDEF_H_

//---------------------------------------------------------------------------
// return codes
//---------------------------------------------------------------------------

typedef enum {
	// area for generic errors 0x0000 - 0x000F
	kEplSuccessful = 0x0000,	// no error/successful run
	kEplIllegalInstance = 0x0001,	// the called Instanz does not exist
	kEplInvalidInstanceParam = 0x0002,	//
	kEplNoFreeInstance = 0x0003,	// XxxAddInstance was called but no free instance is available
	kEplWrongSignature = 0x0004,	// wrong signature while writing to object 0x1010 or 0x1011
	kEplInvalidOperation = 0x0005,	// operation not allowed in this situation
	kEplInvalidNodeId = 0x0007,	// invalid NodeId was specified
	kEplNoResource = 0x0008,	// resource could not be created (Windows, PxROS, ...)
	kEplShutdown = 0x0009,	// stack is shutting down
	kEplReject = 0x000A,	// reject the subsequent command

	// area for EDRV module 0x0010 - 0x001F
//    kEplEdrvNoFrame             = 0x0010,       // no CAN message was received
//    kEplEdrvMsgHigh             = 0x0011,       // CAN message with high priority was received
//    kEplEdrvMsgLow              = 0x0012,       // CAN message with low priority was received
	kEplEdrvInitError = 0x0013,	// initialisation error
	kEplEdrvNoFreeBufEntry = 0x0014,	// no free entry in internal buffer table for Tx frames
	kEplEdrvBufNotExisting = 0x0015,	// specified Tx buffer does not exist
//    kEplEdrvNoFreeChannel       = 0x0014,       // CAN controller has not a free channel
//    kEplEdrvTxBuffHighOverrun   = 0x0015,       // buffer for high priority CAN transmit messages has overrun
//    kEplEdrvTxBuffLowOverrun    = 0x0016,       // buffer for low priority CAN transmit messages has overrun
//    kEplEdrvIllegalBdi          = 0x0017,       // unsupported baudrate within baudrate table
//    kEplEdrvBusy                = 0x0018,       // remote frame can not be updated because no bus contact or CAN
	// transmission is activ
//    kEplEdrvInvalidDriverType   = 0x0019,       // (PC: Windows or Linux) invalid driver type
//    kEplEdrvDriverNotFound      = 0x001A,       // (PC: Windows or Linux) driver (DLL) could not be found
//    kEplEdrvInvalidBaseAddress  = 0x001B,       // (PC: Windows or Linux) driver could not found the CAN controller
//    kEplEdrvInvalidParam        = 0x001C,       // invalid param in function call

	// area for COB module 0x0020 - 0x002F
	kEplDllIllegalHdl = 0x0022,	// illegal handle for a TxFrame was passed
	kEplDllCbAsyncRegistered = 0x0023,	// handler for non-EPL frames was already registered before
//    kEplDllAsyncRxBufferFull    = 0x0024,       // receive buffer for asynchronous frames is full
	kEplDllAsyncTxBufferEmpty = 0x0025,	// transmit buffer for asynchronous frames is empty
	kEplDllAsyncTxBufferFull = 0x0026,	// transmit buffer for asynchronous frames is full
	kEplDllNoNodeInfo = 0x0027,	// MN: too less space in the internal node info structure
	kEplDllInvalidParam = 0x0028,	// invalid parameters passed to function
	kEplDllTxBufNotReady = 0x002E,	// TxBuffer (e.g. for PReq) is not ready yet
	kEplDllTxFrameInvalid = 0x002F,	// TxFrame (e.g. for PReq) is invalid or does not exist
	// area for OBD module 0x0030 - 0x003F
	kEplObdIllegalPart = 0x0030,	// unknown OD part
	kEplObdIndexNotExist = 0x0031,	// object index does not exist in OD
	kEplObdSubindexNotExist = 0x0032,	// subindex does not exist in object index
	kEplObdReadViolation = 0x0033,	// read access to a write-only object
	kEplObdWriteViolation = 0x0034,	// write access to a read-only object
	kEplObdAccessViolation = 0x0035,	// access not allowed
	kEplObdUnknownObjectType = 0x0036,	// object type not defined/known
	kEplObdVarEntryNotExist = 0x0037,	// object does not contain VarEntry structure
	kEplObdValueTooLow = 0x0038,	// value to write to an object is too low
	kEplObdValueTooHigh = 0x0039,	// value to write to an object is too high
	kEplObdValueLengthError = 0x003A,	// value to write is to long or to short
//    kEplObdIllegalFloat         = 0x003B,       // illegal float variable
//    kEplObdWrongOdBuilderKey    = 0x003F,       // OD was generated with demo version of tool ODBuilder

	// area for NMT module 0x0040 - 0x004F
	kEplNmtUnknownCommand = 0x0040,	// unknown NMT command
	kEplNmtInvalidFramePointer = 0x0041,	// pointer to the frame is not valid
	kEplNmtInvalidEvent = 0x0042,	// invalid event send to NMT-modul
	kEplNmtInvalidState = 0x0043,	// unknown state in NMT-State-Maschine
	kEplNmtInvalidParam = 0x0044,	// invalid parameters specified

	// area for SDO/UDP module 0x0050 - 0x005F
	kEplSdoUdpMissCb = 0x0050,	// missing callback-function pointer during inti of
	// module
	kEplSdoUdpNoSocket = 0x0051,	// error during init of socket
	kEplSdoUdpSocketError = 0x0052,	// error during usage of socket
	kEplSdoUdpThreadError = 0x0053,	// error during start of listen thread
	kEplSdoUdpNoFreeHandle = 0x0054,	// no free connection handle for Udp
	kEplSdoUdpSendError = 0x0055,	// Error during send of frame
	kEplSdoUdpInvalidHdl = 0x0056,	// the connection handle is invalid

	// area for SDO Sequence layer module 0x0060 - 0x006F
	kEplSdoSeqMissCb = 0x0060,	// no callback-function assign
	kEplSdoSeqNoFreeHandle = 0x0061,	// no free handle for connection
	kEplSdoSeqInvalidHdl = 0x0062,	// invalid handle in SDO sequence layer
	kEplSdoSeqUnsupportedProt = 0x0063,	// unsupported Protocol selected
	kEplSdoSeqNoFreeHistory = 0x0064,	// no free entry in history
	kEplSdoSeqFrameSizeError = 0x0065,	// the size of the frames is not correct
	kEplSdoSeqRequestAckNeeded = 0x0066,	// indeicates that the history buffer is full
	// and a ack request is needed
	kEplSdoSeqInvalidFrame = 0x0067,	// frame not valid
	kEplSdoSeqConnectionBusy = 0x0068,	// connection is busy -> retry later
	kEplSdoSeqInvalidEvent = 0x0069,	// invalid event received

	// area for SDO Command Layer Module 0x0070 - 0x007F
	kEplSdoComUnsupportedProt = 0x0070,	// unsupported Protocol selected
	kEplSdoComNoFreeHandle = 0x0071,	// no free handle for connection
	kEplSdoComInvalidServiceType = 0x0072,	// invalid SDO service type specified
	kEplSdoComInvalidHandle = 0x0073,	// handle invalid
	kEplSdoComInvalidSendType = 0x0074,	// the stated to of frame to send is
	// not possible
	kEplSdoComNotResponsible = 0x0075,	// internal error: command layer handle is
	// not responsible for this event from sequence layer
	kEplSdoComHandleExists = 0x0076,	// handle to same node already exists
	kEplSdoComHandleBusy = 0x0077,	// transfer via this handle is already running
	kEplSdoComInvalidParam = 0x0078,	// invalid parameters passed to function

	// area for EPL Event-Modul 0x0080 - 0x008F
	kEplEventUnknownSink = 0x0080,	// unknown sink for event
	kEplEventPostError = 0x0081,	// error during post of event

	// area for EPL Timer Modul 0x0090 - 0x009F
	kEplTimerInvalidHandle = 0x0090,	// invalid handle for timer
	kEplTimerNoTimerCreated = 0x0091,	// no timer was created caused by
	// an error

	// area for EPL SDO/Asnd Module 0x00A0 - 0x0AF
	kEplSdoAsndInvalidNodeId = 0x00A0,	//0 node id is invalid
	kEplSdoAsndNoFreeHandle = 0x00A1,	// no free handle for connection
	kEplSdoAsndInvalidHandle = 0x00A2,	// handle for connection is invalid

	// area for PDO module 0x00B0 - 0x00BF
	kEplPdoNotExist = 0x00B0,	// selected PDO does not exist
	kEplPdoLengthExceeded = 0x00B1,	// length of PDO mapping exceedes 64 bis
	kEplPdoGranularityMismatch = 0x00B2,	// configured PDO granularity is not equal to supported granularity
	kEplPdoInitError = 0x00B3,	// error during initialisation of PDO module
	kEplPdoErrorPdoEncode = 0x00B4,	// error during encoding a PDO
	kEplPdoErrorPdoDecode = 0x00B5,	// error during decoding a PDO
	kEplPdoErrorSend = 0x00B6,	// error during sending a PDO
	kEplPdoErrorSyncWin = 0x00B7,	// the SYNC window runs out during sending SYNC-PDOs
	kEplPdoErrorMapp = 0x00B8,	// invalid PDO mapping
	kEplPdoVarNotFound = 0x00B9,	// variable was not found in function PdoSignalVar()
	kEplPdoErrorEmcyPdoLen = 0x00BA,	// the length of a received PDO is unequal to the expected value
	kEplPdoWriteConstObject = 0x00BB,	// constant object can not be written
	// (only TxType, Inhibit-, Event Time for CANopen Kit)

	// area for LSS slave module
	// area for emergency consumer module 0x0090 - 0x009F
	// area for dynamic OD 0x00A0 - 0x00AF
	// area for hertbeat consumer module 0x00B0 - 0x00BF
	// Configuration manager module 0x00C0 - 0x00CF
	kEplCfgMaConfigError = 0x00C0,	// error in configuration manager
	kEplCfgMaSdocTimeOutError = 0x00C1,	// error in configuration manager, Sdo timeout
	kEplCfgMaInvalidDcf = 0x00C2,	// configration file not valid
	kEplCfgMaUnsupportedDcf = 0x00C3,	// unsupported Dcf format
	kEplCfgMaConfigWithErrors = 0x00C4,	// configuration finished with errors
	kEplCfgMaNoFreeConfig = 0x00C5,	// no free configuration entry
	kEplCfgMaNoConfigData = 0x00C6,	// no configuration data present
	kEplCfgMaUnsuppDatatypeDcf = 0x00C7,	// unsupported datatype found in dcf
	// -> this entry was not configured

	// area for LSS master module 0x00D0 - 0x00DF
	// area for CCM modules 0x00E0 - 0xEF
	// area for SRDO module 0x0100 - 0x011F

	kEplApiTaskDeferred = 0x0140,	// EPL performs task in background and informs the application (or vice-versa), when it is finished
	kEplApiInvalidParam = 0x0142,	// passed invalid parameters to a function (e.g. invalid node id)

	// area untill 0x07FF is reserved
	// area for user application from 0x0800 to 0x7FFF

} tEplKernel;

#endif
//EOF

// Die letzte Zeile mu� unbedingt eine leere Zeile sein, weil manche Compiler
// damit ein Problem haben, wenn das nicht so ist (z.B. GNU oder Borland C++ Builder).
