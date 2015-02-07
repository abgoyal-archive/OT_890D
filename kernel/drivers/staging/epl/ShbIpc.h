

#ifndef _SHBIPC_H_
#define _SHBIPC_H_

//---------------------------------------------------------------------------
//  Type definitions
//---------------------------------------------------------------------------

typedef int (*tSigHndlrNewData) (tShbInstance pShbInstance_p);
typedef void (*tSigHndlrJobReady) (tShbInstance pShbInstance_p,
				   unsigned int fTimeOut_p);

#if (TARGET_SYSTEM == _WIN32_)
#if defined(INLINE_FUNCTION_DEF)
#undef  INLINE_FUNCTION
#define INLINE_FUNCTION     INLINE_FUNCTION_DEF
#define SHBIPC_INLINE_ENABLED      TRUE
#define SHBIPC_INLINED
#include "ShbIpc-Win32.c"
#endif

#elif (TARGET_SYSTEM == _LINUX_)
#if defined(INLINE_FUNCTION_DEF)
#undef  INLINE_FUNCTION
#define INLINE_FUNCTION     INLINE_FUNCTION_DEF
#define SHBIPC_INLINE_ENABLED      TRUE
#define SHBIPC_INLINED
#include "ShbIpc-LinuxKernel.c"
#endif
#endif

//---------------------------------------------------------------------------
//  Prototypes
//---------------------------------------------------------------------------

tShbError ShbIpcInit(void);
tShbError ShbIpcExit(void);

tShbError ShbIpcAllocBuffer(unsigned long ulBufferSize_p,
			    const char *pszBufferID_p,
			    tShbInstance * ppShbInstance_p,
			    unsigned int *pfShbNewCreated_p);
tShbError ShbIpcReleaseBuffer(tShbInstance pShbInstance_p);

#if !defined(SHBIPC_INLINE_ENABLED)

tShbError ShbIpcEnterAtomicSection(tShbInstance pShbInstance_p);
tShbError ShbIpcLeaveAtomicSection(tShbInstance pShbInstance_p);

tShbError ShbIpcStartSignalingNewData(tShbInstance pShbInstance_p,
				      tSigHndlrNewData
				      pfnSignalHandlerNewData_p,
				      tShbPriority ShbPriority_p);
tShbError ShbIpcStopSignalingNewData(tShbInstance pShbInstance_p);
tShbError ShbIpcSignalNewData(tShbInstance pShbInstance_p);

tShbError ShbIpcStartSignalingJobReady(tShbInstance pShbInstance_p,
				       unsigned long ulTimeOut_p,
				       tSigHndlrJobReady
				       pfnSignalHandlerJobReady_p);
tShbError ShbIpcSignalJobReady(tShbInstance pShbInstance_p);

void *ShbIpcGetShMemPtr(tShbInstance pShbInstance_p);
#endif

#undef  SHBIPC_INLINE_ENABLED	// disable actual inlining of functions
#undef  INLINE_FUNCTION
#define INLINE_FUNCTION		// define INLINE_FUNCTION to nothing

#endif // #ifndef _SHBIPC_H_
