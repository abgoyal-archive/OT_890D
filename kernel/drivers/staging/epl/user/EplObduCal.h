

#include "../EplObd.h"

#ifndef _EPLOBDUCAL_H_
#define _EPLOBDUCAL_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalWriteEntry(unsigned int uiIndex_p,
						    unsigned int uiSubIndex_p,
						    void *pSrcData_p,
						    tEplObdSize Size_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalReadEntry(unsigned int uiIndex_p,
						   unsigned int uiSubIndex_p,
						   void *pDstData_p,
						   tEplObdSize * pSize_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalAccessOdPart(tEplObdPart ObdPart_p,
						      tEplObdDir Direction_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalDefineVar(tEplVarParam MEM *
						   pVarParam_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT void *PUBLIC EplObduCalGetObjectDataPtr(unsigned int uiIndex_p,
						     unsigned int uiSubIndex_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalRegisterUserOd(tEplObdEntryPtr
							pUserOd_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT void PUBLIC EplObduCalInitVarEntry(tEplObdVarEntry MEM *
						pVarEntry_p, BYTE bType_p,
						tEplObdSize ObdSize_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplObdSize PUBLIC EplObduCalGetDataSize(unsigned int uiIndex_p,
						      unsigned int
						      uiSubIndex_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT unsigned int PUBLIC EplObduCalGetNodeId(void);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalSetNodeId(unsigned int uiNodeId_p,
						   tEplObdNodeIdType
						   NodeIdType_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalGetAccessType(unsigned int uiIndex_p,
						       unsigned int
						       uiSubIndex_p,
						       tEplObdAccess *
						       pAccessTyp_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalReadEntryToLe(unsigned int uiIndex_p,
						       unsigned int
						       uiSubIndex_p,
						       void *pDstData_p,
						       tEplObdSize * pSize_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduCalWriteEntryFromLe(unsigned int
							  uiIndex_p,
							  unsigned int
							  uiSubIndex_p,
							  void *pSrcData_p,
							  tEplObdSize Size_p);
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC
EplObduCalSearchVarEntry(EPL_MCO_DECL_INSTANCE_PTR_ unsigned int uiIndex_p,
			 unsigned int uiSubindex_p,
			 tEplObdVarEntry MEM ** ppVarEntry_p);

#endif // #ifndef _EPLOBDUCAL_H_
