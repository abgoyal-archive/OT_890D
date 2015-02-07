

#include "../EplObd.h"

#ifndef _EPLOBDK_H_
#define _EPLOBDK_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// global variables
//---------------------------------------------------------------------------

extern BYTE MEM abEplObdTrashObject_g[8];

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDK)) != 0)
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdInit(EPL_MCO_DECL_PTR_INSTANCE_PTR_
					  tEplObdInitParam MEM * pInitParam_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdAddInstance(EPL_MCO_DECL_PTR_INSTANCE_PTR_
						 tEplObdInitParam MEM *
						 pInitParam_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdDeleteInstance(EPL_MCO_DECL_INSTANCE_PTR);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdWriteEntry(EPL_MCO_DECL_INSTANCE_PTR_
						unsigned int uiIndex_p,
						unsigned int uiSubIndex_p,
						void *pSrcData_p,
						tEplObdSize Size_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdReadEntry(EPL_MCO_DECL_INSTANCE_PTR_
					       unsigned int uiIndex_p,
					       unsigned int uiSubIndex_p,
					       void *pDstData_p,
					       tEplObdSize * pSize_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC
EplObdSetStoreLoadObjCallback(EPL_MCO_DECL_INSTANCE_PTR_
			      tEplObdStoreLoadObjCallback fpCallback_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdAccessOdPart(EPL_MCO_DECL_INSTANCE_PTR_
						  tEplObdPart ObdPart_p,
						  tEplObdDir Direction_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdDefineVar(EPL_MCO_DECL_INSTANCE_PTR_
					       tEplVarParam MEM * pVarParam_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT void *PUBLIC EplObdGetObjectDataPtr(EPL_MCO_DECL_INSTANCE_PTR_
						 unsigned int uiIndex_p,
						 unsigned int uiSubIndex_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdRegisterUserOd(EPL_MCO_DECL_INSTANCE_PTR_
						    tEplObdEntryPtr pUserOd_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT void PUBLIC EplObdInitVarEntry(EPL_MCO_DECL_INSTANCE_PTR_
					    tEplObdVarEntry MEM * pVarEntry_p,
					    tEplObdType Type_p,
					    tEplObdSize ObdSize_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplObdSize PUBLIC EplObdGetDataSize(EPL_MCO_DECL_INSTANCE_PTR_
						  unsigned int uiIndex_p,
						  unsigned int uiSubIndex_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT unsigned int PUBLIC EplObdGetNodeId(EPL_MCO_DECL_INSTANCE_PTR);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdSetNodeId(EPL_MCO_DECL_INSTANCE_PTR_
					       unsigned int uiNodeId_p,
					       tEplObdNodeIdType NodeIdType_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdIsNumerical(EPL_MCO_DECL_INSTANCE_PTR_
						 unsigned int uiIndex_p,
						 unsigned int uiSubIndex_p,
						 BOOL * pfEntryNumerical);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdWriteEntryFromLe(EPL_MCO_DECL_INSTANCE_PTR_
						      unsigned int uiIndex_p,
						      unsigned int uiSubIndex_p,
						      void *pSrcData_p,
						      tEplObdSize Size_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdReadEntryToLe(EPL_MCO_DECL_INSTANCE_PTR_
						   unsigned int uiIndex_p,
						   unsigned int uiSubIndex_p,
						   void *pDstData_p,
						   tEplObdSize * pSize_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdGetAccessType(EPL_MCO_DECL_INSTANCE_PTR_
						   unsigned int uiIndex_p,
						   unsigned int uiSubIndex_p,
						   tEplObdAccess *
						   pAccessTyp_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdSearchVarEntry(EPL_MCO_DECL_INSTANCE_PTR_
						    unsigned int uiIndex_p,
						    unsigned int uiSubindex_p,
						    tEplObdVarEntry MEM **
						    ppVarEntry_p);

#endif // end of #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDK)) != 0)

#endif // #ifndef _EPLOBDK_H_
