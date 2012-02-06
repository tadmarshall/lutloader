// LUT.h -- The color lookup table format used by GetDeviceGammaTable() and the 'vcgt' profile tag
//

#pragma once
#include "stdafx.h"

// The standard LUT -- 3 channels, 256 entries, 2 bytes per entry, red then green then blue
//
typedef struct tag_LUT {
	WORD	red[256];
	WORD	green[256];
	WORD	blue[256];
} LUT;

// Unusual, but a 'vcgt' sometimes uses only 1 byte per color entry
//
typedef struct tag_LUT_1_BYTE {
	BYTE	red[256];
	BYTE	green[256];
	BYTE	blue[256];
} LUT_1_BYTE;

// Results of comparing a profile's LUT with another (from the adapter, or another profile)
//
typedef enum tag_LUT_COMPARISON {
	LC_EQUAL = 0,								// LUTs match word for word
	LC_VARIATION_ON_LINEAR = 1,					// One uses 0x0100, the other uses 0x0101 style
	LC_TRUNCATION_IN_LOW_BYTE = 2,				// Low byte zeroed
	LC_ROUNDING_IN_LOW_BYTE = 3,				// Windows 7 rounds to nearest high byte
	LC_PROFILE_HAS_NO_LUT_OTHER_LINEAR = 4,		// Match, given the circumstances
	LC_PROFILE_HAS_NO_LUT_OTHER_NONLINEAR = 5,	// Mismatch, other should be linear
	LC_UNEQUAL = 6,								// LUTs do not meet any matching criteria
	LC_ERROR_NO_LUT_PROVIDED = 7				// Caller provided a zero pointer
} LUT_COMPARISON;
