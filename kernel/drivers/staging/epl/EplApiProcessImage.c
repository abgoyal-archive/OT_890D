

#include "Epl.h"
//#include "kernel/EplPdokCal.h"

#if (TARGET_SYSTEM == _LINUX_) && defined(__KERNEL__)
#include <asm/uaccess.h>
#endif

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// modul globale vars
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          C L A S S  EplApi                                              */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
//
// Description:
//
//
/***************************************************************************/

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

#if ((EPL_API_PROCESS_IMAGE_SIZE_IN > 0) || (EPL_API_PROCESS_IMAGE_SIZE_OUT > 0))
typedef struct {
#if EPL_API_PROCESS_IMAGE_SIZE_IN > 0
	BYTE m_abProcessImageInput[EPL_API_PROCESS_IMAGE_SIZE_IN];
#endif
#if EPL_API_PROCESS_IMAGE_SIZE_OUT > 0
	BYTE m_abProcessImageOutput[EPL_API_PROCESS_IMAGE_SIZE_OUT];
#endif

} tEplApiProcessImageInstance;

//---------------------------------------------------------------------------
// local vars
//---------------------------------------------------------------------------

static tEplApiProcessImageInstance EplApiProcessImageInstance_g;
#endif

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplApiProcessImageSetup()
//
// Description: sets up static process image
//
// Parameters:  (none)
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplApiProcessImageSetup(void)
{
	tEplKernel Ret = kEplSuccessful;
#if ((EPL_API_PROCESS_IMAGE_SIZE_IN > 0) || (EPL_API_PROCESS_IMAGE_SIZE_OUT > 0))
	unsigned int uiVarEntries;
	tEplObdSize ObdSize;
#endif

#if EPL_API_PROCESS_IMAGE_SIZE_IN > 0
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_IN;
	ObdSize = 1;
	Ret = EplApiLinkObject(0x2000,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageInput, &uiVarEntries, &ObdSize,
			       1);

	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_IN;
	ObdSize = 1;
	Ret = EplApiLinkObject(0x2001,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageInput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 2;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_IN / ObdSize;
	Ret = EplApiLinkObject(0x2010,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageInput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 2;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_IN / ObdSize;
	Ret = EplApiLinkObject(0x2011,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageInput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 4;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_IN / ObdSize;
	Ret = EplApiLinkObject(0x2020,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageInput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 4;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_IN / ObdSize;
	Ret = EplApiLinkObject(0x2021,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageInput, &uiVarEntries, &ObdSize,
			       1);
#endif

#if EPL_API_PROCESS_IMAGE_SIZE_OUT > 0
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_OUT;
	ObdSize = 1;
	Ret = EplApiLinkObject(0x2030,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageOutput, &uiVarEntries, &ObdSize,
			       1);

	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_OUT;
	ObdSize = 1;
	Ret = EplApiLinkObject(0x2031,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageOutput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 2;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_OUT / ObdSize;
	Ret = EplApiLinkObject(0x2040,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageOutput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 2;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_OUT / ObdSize;
	Ret = EplApiLinkObject(0x2041,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageOutput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 4;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_OUT / ObdSize;
	Ret = EplApiLinkObject(0x2050,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageOutput, &uiVarEntries, &ObdSize,
			       1);

	ObdSize = 4;
	uiVarEntries = EPL_API_PROCESS_IMAGE_SIZE_OUT / ObdSize;
	Ret = EplApiLinkObject(0x2051,
			       EplApiProcessImageInstance_g.
			       m_abProcessImageOutput, &uiVarEntries, &ObdSize,
			       1);
#endif

	return Ret;
}

//----------------------------------------------------------------------------
// Function:    EplApiProcessImageExchangeIn()
//
// Description: replaces passed input process image with the one of EPL stack
//
// Parameters:  pPI_p                   = input process image
//
// Returns:     tEplKernel              = error code
//
// State:
//----------------------------------------------------------------------------

tEplKernel PUBLIC EplApiProcessImageExchangeIn(tEplApiProcessImage * pPI_p)
{
	tEplKernel Ret = kEplSuccessful;

#if EPL_API_PROCESS_IMAGE_SIZE_IN > 0
#if (TARGET_SYSTEM == _LINUX_) && defined(__KERNEL__)
	copy_to_user(pPI_p->m_pImage,
		     EplApiProcessImageInstance_g.m_abProcessImageInput,
		     min(pPI_p->m_uiSize,
			 sizeof(EplApiProcessImageInstance_g.
				m_abProcessImageInput)));
#else
	EPL_MEMCPY(pPI_p->m_pImage,
		   EplApiProcessImageInstance_g.m_abProcessImageInput,
		   min(pPI_p->m_uiSize,
		       sizeof(EplApiProcessImageInstance_g.
			      m_abProcessImageInput)));
#endif
#endif

	return Ret;
}

//----------------------------------------------------------------------------
// Function:    EplApiProcessImageExchangeOut()
//
// Description: copies passed output process image to EPL stack.
//
// Parameters:  pPI_p                   = output process image
//
// Returns:     tEplKernel              = error code
//
// State:
//----------------------------------------------------------------------------

tEplKernel PUBLIC EplApiProcessImageExchangeOut(tEplApiProcessImage * pPI_p)
{
	tEplKernel Ret = kEplSuccessful;

#if EPL_API_PROCESS_IMAGE_SIZE_OUT > 0
#if (TARGET_SYSTEM == _LINUX_) && defined(__KERNEL__)
	copy_from_user(EplApiProcessImageInstance_g.m_abProcessImageOutput,
		       pPI_p->m_pImage,
		       min(pPI_p->m_uiSize,
			   sizeof(EplApiProcessImageInstance_g.
				  m_abProcessImageOutput)));
#else
	EPL_MEMCPY(EplApiProcessImageInstance_g.m_abProcessImageOutput,
		   pPI_p->m_pImage,
		   min(pPI_p->m_uiSize,
		       sizeof(EplApiProcessImageInstance_g.
			      m_abProcessImageOutput)));
#endif
#endif

	return Ret;
}

//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

// EOF
