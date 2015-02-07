

#include "../EplDll.h"

#ifndef _EPLIDENTU_H_
#define _EPLIDENTU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef tEplKernel(PUBLIC * tEplIdentuCbResponse) (unsigned int uiNodeId_p,
						   tEplIdentResponse *
						   pIdentResponse_p);

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplIdentuInit(void);

tEplKernel PUBLIC EplIdentuAddInstance(void);

tEplKernel PUBLIC EplIdentuDelInstance(void);

tEplKernel PUBLIC EplIdentuReset(void);

tEplKernel PUBLIC EplIdentuGetIdentResponse(unsigned int uiNodeId_p,
					    tEplIdentResponse **
					    ppIdentResponse_p);

tEplKernel PUBLIC EplIdentuRequestIdentResponse(unsigned int uiNodeId_p,
						tEplIdentuCbResponse
						pfnCbResponse_p);

#endif // #ifndef _EPLIDENTU_H_
