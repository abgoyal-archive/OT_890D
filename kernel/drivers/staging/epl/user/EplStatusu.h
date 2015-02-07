

#include "../EplDll.h"

#ifndef _EPLSTATUSU_H_
#define _EPLSTATUSU_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef tEplKernel(PUBLIC * tEplStatusuCbResponse) (unsigned int uiNodeId_p,
						    tEplStatusResponse *
						    pStatusResponse_p);

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplStatusuInit(void);

tEplKernel PUBLIC EplStatusuAddInstance(void);

tEplKernel PUBLIC EplStatusuDelInstance(void);

tEplKernel PUBLIC EplStatusuReset(void);

tEplKernel PUBLIC EplStatusuRequestStatusResponse(unsigned int uiNodeId_p,
						  tEplStatusuCbResponse
						  pfnCbResponse_p);

#endif // #ifndef _EPLSTATUSU_H_
