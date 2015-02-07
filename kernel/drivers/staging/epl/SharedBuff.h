

#ifndef _SHAREDBUFF_H_
#define _SHAREDBUFF_H_

//---------------------------------------------------------------------------
//  Type definitions
//---------------------------------------------------------------------------

typedef enum {
	kShbOk = 0,
	kShbNoReadableData = 1,
	kShbDataTruncated = 2,
	kShbBufferFull = 3,
	kShbDataOutsideBufferArea = 4,
	kShbBufferAlreadyCompleted = 5,
	kShbMemUsedByOtherProcs = 6,
	kShbOpenMismatch = 7,
	kShbInvalidBufferType = 8,
	kShbInvalidArg = 9,
	kShbBufferInvalid = 10,
	kShbOutOfMem = 11,
	kShbAlreadyReseting = 12,
	kShbAlreadySignaling = 13,
	kShbExceedDataSizeLimit = 14,

} tShbError;

// 2006/08/24 d.k.: Priority for threads (new data, job signaling)
typedef enum {
	kShbPriorityLow = 0,
	kShbPriorityNormal = 1,
	kshbPriorityHigh = 2
} tShbPriority;

typedef struct {
	unsigned int m_uiFullBlockSize;	// real size of allocated block (incl. alignment fill bytes)
	unsigned long m_ulAvailableSize;	// still available size for data
	unsigned long m_ulWrIndex;	// current write index
	unsigned int m_fBufferCompleted;	// TRUE if allocated block is complete filled with data

} tShbCirChunk;

typedef void *tShbInstance;

typedef void (*tShbCirSigHndlrNewData) (tShbInstance pShbInstance_p,
					unsigned long ulDataBlockSize_p);
typedef void (*tShbCirSigHndlrReset) (tShbInstance pShbInstance_p,
				      unsigned int fTimeOut_p);

//---------------------------------------------------------------------------
//  Prototypes
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif


	tShbError ShbInit(void);
	tShbError ShbExit(void);

// Circular Shared Buffer
	tShbError ShbCirAllocBuffer(unsigned long ulBufferSize_p,
				    const char *pszBufferID_p,
				    tShbInstance * ppShbInstance_p,
				    unsigned int *pfShbNewCreated_p);
	tShbError ShbCirReleaseBuffer(tShbInstance pShbInstance_p);

#if !defined(INLINE_ENABLED)

	tShbError ShbCirResetBuffer(tShbInstance pShbInstance_p,
				    unsigned long ulTimeOut_p,
				    tShbCirSigHndlrReset
				    pfnSignalHandlerReset_p);
	tShbError ShbCirWriteDataBlock(tShbInstance pShbInstance_p,
				       const void *pSrcDataBlock_p,
				       unsigned long ulDataBlockSize_p);
	tShbError ShbCirAllocDataBlock(tShbInstance pShbInstance_p,
				       tShbCirChunk * pShbCirChunk_p,
				       unsigned long ulDataBufferSize_p);
	tShbError ShbCirWriteDataChunk(tShbInstance pShbInstance_p,
				       tShbCirChunk * pShbCirChunk_p,
				       const void *pSrcDataChunk_p,
				       unsigned long ulDataChunkSize_p,
				       unsigned int *pfBufferCompleted_p);
	tShbError ShbCirReadDataBlock(tShbInstance pShbInstance_p,
				      void *pDstDataBlock_p,
				      unsigned long ulRdBuffSize_p,
				      unsigned long *pulDataBlockSize_p);
	tShbError ShbCirGetReadDataSize(tShbInstance pShbInstance_p,
					unsigned long *pulDataBlockSize_p);
	tShbError ShbCirGetReadBlockCount(tShbInstance pShbInstance_p,
					  unsigned long *pulDataBlockCount_p);
	tShbError ShbCirSetSignalHandlerNewData(tShbInstance pShbInstance_p,
						tShbCirSigHndlrNewData
						pfnShbSignalHandlerNewData_p,
						tShbPriority ShbPriority_p);

#endif

// Linear Shared Buffer
	tShbError ShbLinAllocBuffer(unsigned long ulBufferSize_p,
				    const char *pszBufferID_p,
				    tShbInstance * ppShbInstance_p,
				    unsigned int *pfShbNewCreated_p);
	tShbError ShbLinReleaseBuffer(tShbInstance pShbInstance_p);

#if !defined(INLINE_ENABLED)

	tShbError ShbLinWriteDataBlock(tShbInstance pShbInstance_p,
				       unsigned long ulDstBufferOffs_p,
				       const void *pSrcDataBlock_p,
				       unsigned long ulDataBlockSize_p);
	tShbError ShbLinReadDataBlock(tShbInstance pShbInstance_p,
				      void *pDstDataBlock_p,
				      unsigned long ulSrcBufferOffs_p,
				      unsigned long ulDataBlockSize_p);

#endif

#ifndef NDEBUG
	tShbError ShbCirTraceBuffer(tShbInstance pShbInstance_p);
	tShbError ShbLinTraceBuffer(tShbInstance pShbInstance_p);
	tShbError ShbTraceDump(const unsigned char *pabStartAddr_p,
			       unsigned long ulDataSize_p,
			       unsigned long ulAddrOffset_p,
			       const char *pszInfoText_p);
#else
#define ShbCirTraceBuffer(p0)
#define ShbLinTraceBuffer(p0)
#define ShbTraceDump(p0, p1, p2, p3)
#endif

#undef  INLINE_ENABLED		// disable actual inlining of functions
#undef  INLINE_FUNCTION
#define INLINE_FUNCTION		// define INLINE_FUNCTION to nothing

#ifdef __cplusplus
}
#endif
#endif				// #ifndef _SHAREDBUFF_H_
