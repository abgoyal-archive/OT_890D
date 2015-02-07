


#include "ieee754dp.h"

int ieee754dp_finite(ieee754dp x)
{
	return DPBEXP(x) != DP_EMAX + 1 + DP_EBIAS;
}

ieee754dp ieee754dp_copysign(ieee754dp x, ieee754dp y)
{
	CLEARCX;
	DPSIGN(x) = DPSIGN(y);
	return x;
}


ieee754dp ieee754dp_neg(ieee754dp x)
{
	COMPXDP;

	EXPLODEXDP;
	CLEARCX;
	FLUSHXDP;

	/*
	 * Invert the sign ALWAYS to prevent an endless recursion on
	 * pow() in libc.
	 */
	/* quick fix up */
	DPSIGN(x) ^= 1;

	if (xc == IEEE754_CLASS_SNAN) {
		ieee754dp y = ieee754dp_indef();
		SETCX(IEEE754_INVALID_OPERATION);
		DPSIGN(y) = DPSIGN(x);
		return ieee754dp_nanxcpt(y, "neg");
	}

	if (ieee754dp_isnan(x))	/* but not infinity */
		return ieee754dp_nanxcpt(x, "neg", x);
	return x;
}


ieee754dp ieee754dp_abs(ieee754dp x)
{
	COMPXDP;

	EXPLODEXDP;
	CLEARCX;
	FLUSHXDP;

	if (xc == IEEE754_CLASS_SNAN) {
		SETCX(IEEE754_INVALID_OPERATION);
		return ieee754dp_nanxcpt(ieee754dp_indef(), "neg");
	}

	if (ieee754dp_isnan(x))	/* but not infinity */
		return ieee754dp_nanxcpt(x, "abs", x);

	/* quick fix up */
	DPSIGN(x) = 0;
	return x;
}
