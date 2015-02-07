

//#include "global.h"
//#include "EplAmi.h"
#include "EplInc.h"

#if (!defined(EPL_AMI_INLINED)) || defined(INLINE_ENABLED)

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef struct {
	WORD m_wWord;

} twStruct;

typedef struct {
	DWORD m_dwDword;

} tdwStruct;

typedef struct {
	QWORD m_qwQword;

} tqwStruct;

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    AmiSetXXXToBe()
//
// Description: writes the specified value to the absolute address in
//              big endian
//
// Parameters:  pAddr_p                 = absolute address
//              xXXXVal_p               = value
//
// Returns:     (none)
//
// State:
//
//---------------------------------------------------------------------------

//------------< write BYTE in big endian >--------------------------

//------------< write WORD in big endian >--------------------------

INLINE_FUNCTION void PUBLIC AmiSetWordToBe(void FAR * pAddr_p, WORD wWordVal_p)
{
	twStruct FAR *pwStruct;
	twStruct wValue;

	wValue.m_wWord = (WORD) ((wWordVal_p & 0x00FF) << 8);	//LSB to MSB
	wValue.m_wWord |= (WORD) ((wWordVal_p & 0xFF00) >> 8);	//MSB to LSB

	pwStruct = (twStruct FAR *) pAddr_p;
	pwStruct->m_wWord = wValue.m_wWord;

}

//------------< write DWORD in big endian >-------------------------

INLINE_FUNCTION void PUBLIC AmiSetDwordToBe(void FAR * pAddr_p,
					    DWORD dwDwordVal_p)
{
	tdwStruct FAR *pdwStruct;
	tdwStruct dwValue;

	dwValue.m_dwDword = ((dwDwordVal_p & 0x000000FF) << 24);	//LSB to MSB
	dwValue.m_dwDword |= ((dwDwordVal_p & 0x0000FF00) << 8);
	dwValue.m_dwDword |= ((dwDwordVal_p & 0x00FF0000) >> 8);
	dwValue.m_dwDword |= ((dwDwordVal_p & 0xFF000000) >> 24);	//MSB to LSB

	pdwStruct = (tdwStruct FAR *) pAddr_p;
	pdwStruct->m_dwDword = dwValue.m_dwDword;

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetXXXToLe()
//
// Description: writes the specified value to the absolute address in
//              little endian
//
// Parameters:  pAddr_p                 = absolute address
//              xXXXVal_p               = value
//
// Returns:     (none)
//
// State:
//
//---------------------------------------------------------------------------

//------------< write BYTE in little endian >--------------------------

//------------< write WORD in little endian >--------------------------

INLINE_FUNCTION void PUBLIC AmiSetWordToLe(void FAR * pAddr_p, WORD wWordVal_p)
{
	twStruct FAR *pwStruct;

	pwStruct = (twStruct FAR *) pAddr_p;
	pwStruct->m_wWord = wWordVal_p;

}

//------------< write DWORD in little endian >-------------------------

INLINE_FUNCTION void PUBLIC AmiSetDwordToLe(void FAR * pAddr_p,
					    DWORD dwDwordVal_p)
{
	tdwStruct FAR *pdwStruct;

	pdwStruct = (tdwStruct FAR *) pAddr_p;
	pdwStruct->m_dwDword = dwDwordVal_p;

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetXXXFromBe()
//
// Description: reads the specified value from the absolute address in
//              big endian
//
// Parameters:  pAddr_p                 = absolute address
//
// Returns:     XXX                     = value
//
// State:
//
//---------------------------------------------------------------------------

//------------< read BYTE in big endian >---------------------------

//------------< read WORD in big endian >---------------------------

INLINE_FUNCTION WORD PUBLIC AmiGetWordFromBe(void FAR * pAddr_p)
{
	twStruct FAR *pwStruct;
	twStruct wValue;

	pwStruct = (twStruct FAR *) pAddr_p;

	wValue.m_wWord = (WORD) ((pwStruct->m_wWord & 0x00FF) << 8);	//LSB to MSB
	wValue.m_wWord |= (WORD) ((pwStruct->m_wWord & 0xFF00) >> 8);	//MSB to LSB

	return (wValue.m_wWord);

}

//------------< read DWORD in big endian >--------------------------

INLINE_FUNCTION DWORD PUBLIC AmiGetDwordFromBe(void FAR * pAddr_p)
{
	tdwStruct FAR *pdwStruct;
	tdwStruct dwValue;

	pdwStruct = (tdwStruct FAR *) pAddr_p;

	dwValue.m_dwDword = ((pdwStruct->m_dwDword & 0x000000FF) << 24);	//LSB to MSB
	dwValue.m_dwDword |= ((pdwStruct->m_dwDword & 0x0000FF00) << 8);
	dwValue.m_dwDword |= ((pdwStruct->m_dwDword & 0x00FF0000) >> 8);
	dwValue.m_dwDword |= ((pdwStruct->m_dwDword & 0xFF000000) >> 24);	//MSB to LSB

	return (dwValue.m_dwDword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetXXXFromLe()
//
// Description: reads the specified value from the absolute address in
//              little endian
//
// Parameters:  pAddr_p                 = absolute address
//
// Returns:     XXX                     = value
//
// State:
//
//---------------------------------------------------------------------------

//------------< read BYTE in little endian >---------------------------

//------------< read WORD in little endian >---------------------------

INLINE_FUNCTION WORD PUBLIC AmiGetWordFromLe(void FAR * pAddr_p)
{
	twStruct FAR *pwStruct;

	pwStruct = (twStruct FAR *) pAddr_p;
	return (pwStruct->m_wWord);

}

//------------< read DWORD in little endian >--------------------------

INLINE_FUNCTION DWORD PUBLIC AmiGetDwordFromLe(void FAR * pAddr_p)
{
	tdwStruct FAR *pdwStruct;

	pdwStruct = (tdwStruct FAR *) pAddr_p;
	return (pdwStruct->m_dwDword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetDword24ToBe()
//
// Description: sets a 24 bit value to a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              dwDwordVal_p    = value to set
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetDword24ToBe(void FAR * pAddr_p,
					      DWORD dwDwordVal_p)
{

	((BYTE FAR *) pAddr_p)[0] = ((BYTE FAR *) & dwDwordVal_p)[2];
	((BYTE FAR *) pAddr_p)[1] = ((BYTE FAR *) & dwDwordVal_p)[1];
	((BYTE FAR *) pAddr_p)[2] = ((BYTE FAR *) & dwDwordVal_p)[0];

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetDword24ToLe()
//
// Description: sets a 24 bit value to a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              dwDwordVal_p    = value to set
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetDword24ToLe(void FAR * pAddr_p,
					      DWORD dwDwordVal_p)
{

	((BYTE FAR *) pAddr_p)[0] = ((BYTE FAR *) & dwDwordVal_p)[0];
	((BYTE FAR *) pAddr_p)[1] = ((BYTE FAR *) & dwDwordVal_p)[1];
	((BYTE FAR *) pAddr_p)[2] = ((BYTE FAR *) & dwDwordVal_p)[2];

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetDword24FromBe()
//
// Description: reads a 24 bit value from a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      DWORD           = read value
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION DWORD PUBLIC AmiGetDword24FromBe(void FAR * pAddr_p)
{

	tdwStruct dwStruct;

	dwStruct.m_dwDword = AmiGetDwordFromBe(pAddr_p);
	dwStruct.m_dwDword >>= 8;

	return (dwStruct.m_dwDword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetDword24FromLe()
//
// Description: reads a 24 bit value from a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      DWORD           = read value
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION DWORD PUBLIC AmiGetDword24FromLe(void FAR * pAddr_p)
{

	tdwStruct dwStruct;

	dwStruct.m_dwDword = AmiGetDwordFromLe(pAddr_p);
	dwStruct.m_dwDword &= 0x00FFFFFF;

	return (dwStruct.m_dwDword);

}

//#ifdef USE_VAR64

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword64ToBe()
//
// Description: sets a 64 bit value to a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword64ToBe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	((BYTE FAR *) pAddr_p)[0] = ((BYTE FAR *) & qwQwordVal_p)[7];
	((BYTE FAR *) pAddr_p)[1] = ((BYTE FAR *) & qwQwordVal_p)[6];
	((BYTE FAR *) pAddr_p)[2] = ((BYTE FAR *) & qwQwordVal_p)[5];
	((BYTE FAR *) pAddr_p)[3] = ((BYTE FAR *) & qwQwordVal_p)[4];
	((BYTE FAR *) pAddr_p)[4] = ((BYTE FAR *) & qwQwordVal_p)[3];
	((BYTE FAR *) pAddr_p)[5] = ((BYTE FAR *) & qwQwordVal_p)[2];
	((BYTE FAR *) pAddr_p)[6] = ((BYTE FAR *) & qwQwordVal_p)[1];
	((BYTE FAR *) pAddr_p)[7] = ((BYTE FAR *) & qwQwordVal_p)[0];

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword64ToLe()
//
// Description: sets a 64 bit value to a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword64ToLe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	QWORD FAR *pqwDst;

	pqwDst = (QWORD FAR *) pAddr_p;
	*pqwDst = qwQwordVal_p;

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword64FromBe()
//
// Description: reads a 64 bit value from a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword64FromBe(void FAR * pAddr_p)
{

	tqwStruct qwStruct;

	((BYTE FAR *) & qwStruct.m_qwQword)[0] = ((BYTE FAR *) pAddr_p)[7];
	((BYTE FAR *) & qwStruct.m_qwQword)[1] = ((BYTE FAR *) pAddr_p)[6];
	((BYTE FAR *) & qwStruct.m_qwQword)[2] = ((BYTE FAR *) pAddr_p)[5];
	((BYTE FAR *) & qwStruct.m_qwQword)[3] = ((BYTE FAR *) pAddr_p)[4];
	((BYTE FAR *) & qwStruct.m_qwQword)[4] = ((BYTE FAR *) pAddr_p)[3];
	((BYTE FAR *) & qwStruct.m_qwQword)[5] = ((BYTE FAR *) pAddr_p)[2];
	((BYTE FAR *) & qwStruct.m_qwQword)[6] = ((BYTE FAR *) pAddr_p)[1];
	((BYTE FAR *) & qwStruct.m_qwQword)[7] = ((BYTE FAR *) pAddr_p)[0];

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword64FromLe()
//
// Description: reads a 64 bit value from a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword64FromLe(void FAR * pAddr_p)
{

	tqwStruct FAR *pqwStruct;
	tqwStruct qwStruct;

	pqwStruct = (tqwStruct FAR *) pAddr_p;
	qwStruct.m_qwQword = pqwStruct->m_qwQword;

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword40ToBe()
//
// Description: sets a 40 bit value to a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword40ToBe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	((BYTE FAR *) pAddr_p)[0] = ((BYTE FAR *) & qwQwordVal_p)[4];
	((BYTE FAR *) pAddr_p)[1] = ((BYTE FAR *) & qwQwordVal_p)[3];
	((BYTE FAR *) pAddr_p)[2] = ((BYTE FAR *) & qwQwordVal_p)[2];
	((BYTE FAR *) pAddr_p)[3] = ((BYTE FAR *) & qwQwordVal_p)[1];
	((BYTE FAR *) pAddr_p)[4] = ((BYTE FAR *) & qwQwordVal_p)[0];

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword40ToLe()
//
// Description: sets a 40 bit value to a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword40ToLe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	((DWORD FAR *) pAddr_p)[0] = ((DWORD FAR *) & qwQwordVal_p)[0];
	((BYTE FAR *) pAddr_p)[4] = ((BYTE FAR *) & qwQwordVal_p)[4];

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword40FromBe()
//
// Description: reads a 40 bit value from a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      QWORD
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword40FromBe(void FAR * pAddr_p)
{

	tqwStruct qwStruct;

	qwStruct.m_qwQword = AmiGetQword64FromBe(pAddr_p);
	qwStruct.m_qwQword >>= 24;

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword40FromLe()
//
// Description: reads a 40 bit value from a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      QWORD
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword40FromLe(void FAR * pAddr_p)
{

	tqwStruct qwStruct;

	qwStruct.m_qwQword = AmiGetQword64FromLe(pAddr_p);
	qwStruct.m_qwQword &= 0x000000FFFFFFFFFFLL;

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword48ToBe()
//
// Description: sets a 48 bit value to a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword48ToBe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	((BYTE FAR *) pAddr_p)[0] = ((BYTE FAR *) & qwQwordVal_p)[5];
	((BYTE FAR *) pAddr_p)[1] = ((BYTE FAR *) & qwQwordVal_p)[4];
	((BYTE FAR *) pAddr_p)[2] = ((BYTE FAR *) & qwQwordVal_p)[3];
	((BYTE FAR *) pAddr_p)[3] = ((BYTE FAR *) & qwQwordVal_p)[2];
	((BYTE FAR *) pAddr_p)[4] = ((BYTE FAR *) & qwQwordVal_p)[1];
	((BYTE FAR *) pAddr_p)[5] = ((BYTE FAR *) & qwQwordVal_p)[0];

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword48ToLe()
//
// Description: sets a 48 bit value to a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword48ToLe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	((DWORD FAR *) pAddr_p)[0] = ((DWORD FAR *) & qwQwordVal_p)[0];
	((WORD FAR *) pAddr_p)[2] = ((WORD FAR *) & qwQwordVal_p)[2];

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword48FromBe()
//
// Description: reads a 48 bit value from a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      QWORD
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword48FromBe(void FAR * pAddr_p)
{

	tqwStruct qwStruct;

	qwStruct.m_qwQword = AmiGetQword64FromBe(pAddr_p);
	qwStruct.m_qwQword >>= 16;

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword48FromLe()
//
// Description: reads a 48 bit value from a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      QWORD
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword48FromLe(void FAR * pAddr_p)
{

	tqwStruct qwStruct;

	qwStruct.m_qwQword = AmiGetQword64FromLe(pAddr_p);
	qwStruct.m_qwQword &= 0x0000FFFFFFFFFFFFLL;

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword56ToBe()
//
// Description: sets a 56 bit value to a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword56ToBe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	((BYTE FAR *) pAddr_p)[0] = ((BYTE FAR *) & qwQwordVal_p)[6];
	((BYTE FAR *) pAddr_p)[1] = ((BYTE FAR *) & qwQwordVal_p)[5];
	((BYTE FAR *) pAddr_p)[2] = ((BYTE FAR *) & qwQwordVal_p)[4];
	((BYTE FAR *) pAddr_p)[3] = ((BYTE FAR *) & qwQwordVal_p)[3];
	((BYTE FAR *) pAddr_p)[4] = ((BYTE FAR *) & qwQwordVal_p)[2];
	((BYTE FAR *) pAddr_p)[5] = ((BYTE FAR *) & qwQwordVal_p)[1];
	((BYTE FAR *) pAddr_p)[6] = ((BYTE FAR *) & qwQwordVal_p)[0];

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetQword56ToLe()
//
// Description: sets a 56 bit value to a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              qwQwordVal_p    = quadruple word value
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetQword56ToLe(void FAR * pAddr_p,
					      QWORD qwQwordVal_p)
{

	((DWORD FAR *) pAddr_p)[0] = ((DWORD FAR *) & qwQwordVal_p)[0];
	((WORD FAR *) pAddr_p)[2] = ((WORD FAR *) & qwQwordVal_p)[2];
	((BYTE FAR *) pAddr_p)[6] = ((BYTE FAR *) & qwQwordVal_p)[6];

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword56FromBe()
//
// Description: reads a 56 bit value from a buffer in big endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      QWORD
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword56FromBe(void FAR * pAddr_p)
{

	tqwStruct qwStruct;

	qwStruct.m_qwQword = AmiGetQword64FromBe(pAddr_p);
	qwStruct.m_qwQword >>= 8;

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetQword56FromLe()
//
// Description: reads a 56 bit value from a buffer in little endian
//
// Parameters:  pAddr_p         = pointer to source buffer
//
// Return:      QWORD
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION QWORD PUBLIC AmiGetQword56FromLe(void FAR * pAddr_p)
{

	tqwStruct qwStruct;

	qwStruct.m_qwQword = AmiGetQword64FromLe(pAddr_p);
	qwStruct.m_qwQword &= 0x00FFFFFFFFFFFFFFLL;

	return (qwStruct.m_qwQword);

}

//---------------------------------------------------------------------------
//
// Function:    AmiSetTimeOfDay()
//
// Description: sets a TIME_OF_DAY (CANopen) value to a buffer
//
// Parameters:  pAddr_p         = pointer to destination buffer
//              pTimeOfDay_p    = pointer to struct TIME_OF_DAY
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiSetTimeOfDay(void FAR * pAddr_p,
					    tTimeOfDay FAR * pTimeOfDay_p)
{

	AmiSetDwordToLe(((BYTE FAR *) pAddr_p),
			pTimeOfDay_p->m_dwMs & 0x0FFFFFFF);
	AmiSetWordToLe(((BYTE FAR *) pAddr_p) + 4, pTimeOfDay_p->m_wDays);

}

//---------------------------------------------------------------------------
//
// Function:    AmiGetTimeOfDay()
//
// Description: reads a TIME_OF_DAY (CANopen) value from a buffer
//
// Parameters:  pAddr_p         = pointer to source buffer
//              pTimeOfDay_p    = pointer to struct TIME_OF_DAY
//
// Return:      void
//
// State:       not tested
//
//---------------------------------------------------------------------------

INLINE_FUNCTION void PUBLIC AmiGetTimeOfDay(void FAR * pAddr_p,
					    tTimeOfDay FAR * pTimeOfDay_p)
{

	pTimeOfDay_p->m_dwMs =
	    AmiGetDwordFromLe(((BYTE FAR *) pAddr_p)) & 0x0FFFFFFF;
	pTimeOfDay_p->m_wDays = AmiGetWordFromLe(((BYTE FAR *) pAddr_p) + 4);

}

#endif

// EOF

// Die letzte Zeile muﬂ unbedingt eine leere Zeile sein, weil manche Compiler
// damit ein Problem haben, wenn das nicht so ist (z.B. GNU oder Borland C++ Builder).
