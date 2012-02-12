// LUT.cpp -- Code to perform various functions with LUTs
//

#include "stdafx.h"
#include "LUT.h"
#include <banned.h>

// Function to test LUT for linearity
//
IS_LINEAR IsLinear(LUT * pLUT) {

	if (!pLUT) {
		return IL_ZERO_POINTER;
	}

	bool IsLinear8 = true;
	bool IsLinear16 = true;
	bool IsSignature = true;

	// Special-case our "signature"
	//
	size_t i = 0;
	for (; i < 4; ++i) {
		WORD linear8 = static_cast<WORD>(i << 8);
		WORD linear16 = static_cast<WORD>(linear8 + i);
		if ( pLUT->red[i] != linear8 ) {
			IsLinear8 = false;
		}
		if ( pLUT->red[i] != linear16 ) {
			IsLinear16 = false;
		}
		if ( 1 == i && 0x0102 != pLUT->red[i] ) {
			IsSignature = false;
		}
		if ( 2 == i && 0x0203 != pLUT->red[i] ) {
			IsSignature = false;
		}
		if ( 3 == i && 0x0304 != pLUT->red[i] ) {
			IsSignature = false;
		}
		if ( pLUT->green[i] != linear8 ) {
			IsLinear8 = false;
		}
		if ( pLUT->green[i] != linear16 ) {
			IsLinear16 = false;
		}
		if ( pLUT->blue[i] != linear8 ) {
			IsLinear8 = false;
		}
		if ( pLUT->blue[i] != linear16 ) {
			IsLinear16 = false;
		}
	}

	// Look at the rest of the LUT if we are OK so far
	//
	if ( IsLinear8 || IsLinear16 || IsSignature ) {
		for (; i < 256; ++i) {
			WORD linear8 = static_cast<WORD>(i << 8);
			WORD linear16 = static_cast<WORD>(linear8 + i);
			if ( pLUT->red[i] != linear8 ) {
				IsLinear8 = false;
			}
			if ( pLUT->red[i] != linear16 ) {
				IsLinear16 = false;
			}
			if ( pLUT->green[i] != linear8 ) {
				IsLinear8 = false;
			}
			if ( pLUT->green[i] != linear16 ) {
				IsLinear16 = false;
			}
			if ( pLUT->blue[i] != linear8 ) {
				IsLinear8 = false;
			}
			if ( pLUT->blue[i] != linear16 ) {
				IsLinear16 = false;
			}
		}
	} else {
		return IL_NOT_LINEAR;
	}

	// We finished looking at the LUT ... what did we find?
	//
	if (IsSignature) {
		return IL_SIGNATURE;
	}
	if (IsLinear8) {
		return IL_LINEAR_8;
	}
	if (IsLinear16) {
		return IL_LINEAR_16;
	}
	return IL_NOT_LINEAR;
}

// Write a "signed" linear LUT to a provided address (caller owns memory)
//
void GetSignedLUT(LUT * pLUT) {

	for ( size_t i = 0; i < 256; ++i ) {
		WORD linear16 = static_cast<WORD>( (i << 8) + i);
		pLUT->red[i] = linear16;
		pLUT->green[i] = linear16;
		pLUT->blue[i] = linear16;
	}

	// Add a "signature" to the linear LUT to help us detect other LUT loaders
	//
	pLUT->red[1] = 0x0102;
	pLUT->red[2] = 0x0203;
	pLUT->red[3] = 0x0304;
}
