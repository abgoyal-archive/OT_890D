

#include "../EplObd.h"

#ifndef _EPLOBDU_H_
#define _EPLOBDU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDU)) != 0)

#if EPL_OBD_USE_KERNEL != FALSE
#error "EPL OBDu module enabled, but OBD_USE_KERNEL == TRUE"
#endif

EPLDLLEXPORT tEplKernel PUBLIC EplObduWriteEntry(unsigned int uiIndex_p,
						 unsigned int uiSubIndex_p,
						 void *pSrcData_p,
						 tEplObdSize Size_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduReadEntry(unsigned int uiIndex_p,
						unsigned int uiSubIndex_p,
						void *pDstData_p,
						tEplObdSize * pSize_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduAccessOdPart(tEplObdPart ObdPart_p,
						   tEplObdDir Direction_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduDefineVar(tEplVarParam MEM * pVarParam_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT void *PUBLIC EplObduGetObjectDataPtr(unsigned int uiIndex_p,
						  unsigned int uiSubIndex_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduRegisterUserOd(tEplObdEntryPtr pUserOd_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT void PUBLIC EplObduInitVarEntry(tEplObdVarEntry MEM * pVarEntry_p,
					     BYTE bType_p,
					     tEplObdSize ObdSize_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplObdSize PUBLIC EplObduGetDataSize(unsigned int uiIndex_p,
						   unsigned int uiSubIndex_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT unsigned int PUBLIC EplObduGetNodeId(void);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduSetNodeId(unsigned int uiNodeId_p,
						tEplObdNodeIdType NodeIdType_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduGetAccessType(unsigned int uiIndex_p,
						    unsigned int uiSubIndex_p,
						    tEplObdAccess *
						    pAccessTyp_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduReadEntryToLe(unsigned int uiIndex_p,
						    unsigned int uiSubIndex_p,
						    void *pDstData_p,
						    tEplObdSize * pSize_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduWriteEntryFromLe(unsigned int uiIndex_p,
						       unsigned int
						       uiSubIndex_p,
						       void *pSrcData_p,
						       tEplObdSize Size_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduSearchVarEntry(EPL_MCO_DECL_INSTANCE_PTR_
						     unsigned int uiIndex_p,
						     unsigned int uiSubindex_p,
						     tEplObdVarEntry MEM **
						     ppVarEntry_p);

#elif EPL_OBD_USE_KERNEL != FALSE
#include "../kernel/EplObdk.h"

#define EplObduWriteEntry       EplObdWriteEntry

#define EplObduReadEntry        EplObdReadEntry

#define EplObduAccessOdPart     EplObdAccessOdPart

#define EplObduDefineVar        EplObdDefineVar

#define EplObduGetObjectDataPtr EplObdGetObjectDataPtr

#define EplObduRegisterUserOd   EplObdRegisterUserOd

#define EplObduInitVarEntry     EplObdInitVarEntry

#define EplObduGetDataSize      EplObdGetDataSize

#define EplObduGetNodeId        EplObdGetNodeId

#define EplObduSetNodeId        EplObdSetNodeId

#define EplObduGetAccessType    EplObdGetAccessType

#define EplObduReadEntryToLe    EplObdReadEntryToLe

#define EplObduWriteEntryFromLe EplObdWriteEntryFromLe

#define EplObduSearchVarEntry   EplObdSearchVarEntry

#define EplObduIsNumerical      EplObdIsNumerical

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDU)) != 0)

#endif // #ifndef _EPLOBDU_H_
