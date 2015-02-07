

#include "user/EplTimeru.h"

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
/*          C L A S S  <Epl Userspace-Timermodule NULL-Implementation>     */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
//
// Description: Epl Userspace-Timermodule NULL-Implementation
//
//
/***************************************************************************/

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplTimeruInit
//
// Description: function init first instance
//
//
//
// Parameters:
//
//
// Returns:     tEplKernel  = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplTimeruInit()
{
	tEplKernel Ret;

	Ret = EplTimeruAddInstance();

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplTimeruAddInstance
//
// Description: function init additional instance
//
//
//
// Parameters:
//
//
// Returns:     tEplKernel  = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplTimeruAddInstance()
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplTimeruDelInstance
//
// Description: function delte instance
//              -> under Win32 nothing to do
//              -> no instnace table needed
//
//
//
// Parameters:
//
//
// Returns:     tEplKernel  = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplTimeruDelInstance()
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplTimeruSetTimerMs
//
// Description: function create a timer and return a handle to the pointer
//
//
//
// Parameters:  pTimerHdl_p = pointer to a buffer to fill in the handle
//              ulTime_p    = time for timer in ms
//              Argument_p  = argument for timer
//
//
// Returns:     tEplKernel  = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplTimeruSetTimerMs(tEplTimerHdl * pTimerHdl_p,
				      unsigned long ulTime_p,
				      tEplTimerArg Argument_p)
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	// check handle
	if (pTimerHdl_p == NULL) {
		Ret = kEplTimerInvalidHandle;
		goto Exit;
	}

      Exit:
	return Ret;
}

 //---------------------------------------------------------------------------
//
// Function:    EplTimeruModifyTimerMs
//
// Description: function change a timer and return a handle to the pointer
//
//
//
// Parameters:  pTimerHdl_p = pointer to a buffer to fill in the handle
//              ulTime_p    = time for timer in ms
//              Argument_p  = argument for timer
//
//
// Returns:     tEplKernel  = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplTimeruModifyTimerMs(tEplTimerHdl * pTimerHdl_p,
					 unsigned long ulTime_p,
					 tEplTimerArg Argument_p)
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	// check parameter
	if (pTimerHdl_p == NULL) {
		Ret = kEplTimerInvalidHandle;
		goto Exit;
	}

      Exit:
	return Ret;
}

 //---------------------------------------------------------------------------
//
// Function:    EplTimeruDeleteTimer
//
// Description: function delte a timer
//
//
//
// Parameters:  pTimerHdl_p = pointer to a buffer to fill in the handle
//
//
// Returns:     tEplKernel  = errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplTimeruDeleteTimer(tEplTimerHdl * pTimerHdl_p)
{
	tEplKernel Ret;

	Ret = kEplSuccessful;

	// check parameter
	if (pTimerHdl_p == NULL) {
		Ret = kEplTimerInvalidHandle;
		goto Exit;
	}
	// set handle invalide
	*pTimerHdl_p = 0;

      Exit:
	return Ret;

}

//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

// EOF
