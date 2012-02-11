// LUT.h -- The color lookup table format used by GetDeviceGammaTable() and the 'vcgt' profile tag
//
// Also, a header file for non-class functions in LUT.cpp
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
	LC_TRUNCATION_OR_ROUNDING = 4,				// Could be either truncation or rounding
	LC_PROFILE_HAS_NO_LUT_OTHER_LINEAR = 5,		// Match, given the circumstances
	LC_PROFILE_HAS_NO_LUT_OTHER_NONLINEAR = 6,	// Mismatch, other should be linear
	LC_UNEQUAL = 7,								// LUTs do not meet any matching criteria
	LC_ERROR_NO_LUT_PROVIDED = 8				// Caller provided a zero pointer
} LUT_COMPARISON;

// Notes on the "signature":

// The problem we are trying to solve is this:  when running on a system with multiple
//  monitors (XP, Vista or Windows 7), there is more than one way to reprogram the LUT.
//
// A multi-monitor-aware program (that is doing it right) will call CreateDC() on a
//  "device string" such as "\\.\DISPLAY1\Monitor1" and get a handle to a Device Context
//  (an HDC) for a single monitor.  When that program then calls SetDeviceGammaRamp(hdc,ramp),
//  it will touch the desired monitor and no other.  But, other programs are less "aware",
//  and simply call GetDC(NULL), which gives them an HDC for "the screen".  This means the
//  entire multi-monitor desktop.
//
// When they then call SetDeviceGammaRamp(hdc,ramp), they hit every monitor with the same
//  color lookup table (LUT).  Since the single LUT is unlikely to be correct for every
//  monitor, this clobbers multi-monitor color management.
//
// What makes this challenging is that Windows maintains different "virtual LUTs" for these
//  various ways of accessing monitors on a multi-monitor system.
//
// On a single monitor system, there is no problem.  A call to SetDeviceGammaRamp() will
//  affect the single monitor, and both the GetDC(NULL) and CreateDC(deviceString,0,0,0) calls
//  will provide HDCs that will do the same thing when SetDeviceGammaRamp(hdc,ramp) is called.
//
// On a system with multiple monitors, GetDC(NULL) is tracked separately from the individual
//  monitors.  Programs that call GetDeviceGammaRamp(hdc,ramp) with a specific monitor
//  will not see changes made by a program that called SetDeviceGammaRamp(hdc, ramp) with
//  an HDC for "the screen".  These multi-monitor-aware programs need to check both the screen
//  and the specific device, and if they are different (which they usually will be on multi
//  -monitor systems) they don't know which one represents what the user is actually seeing.
//
// Hence, the "signature trick".  When told to "reset everything", we will do, in order:
//   1)  Hit the "screen" with a "signed" linear LUT.  It differs from a linear16 ramp
//       in only three low-order WORDs of the red ramp, and only by a single unit, and will
//       be visually all-but identical to a proper linear ramp if anyone sees it;
//   2)  Hit the individual monitors with their correct LUTs.
//
// The "feature" here is that if we are later asked to check if the LUTs are correct on the
//  monitors, we can see if some other program has clobbered "the screen".  If we know
//  that we wrote a signature to "the screen" earlier (or we saw one when we started up),
//  and if that signature is no longer there, then we can guess with some reliability
//  that another program has hammered the LUT on all monitors.  We will see the same LUT
//  we wrote if we query individual monitors, but seeing a clobbered signature for "the
//  screen" indicates that we are not the only program calling SetDeviceGammaRamp().
//
// The reason to be "close" to linear is to minimize issues if anyone actually sees this
//  LUT.  Hopefully, they won't, since we only write it when we are "resetting everything"
//  and so we will immediately change this LUT to the one that the profile indicates.
//
// The reason to be "different from linear" is to recognize programs that reset everything
//  to linear as having "changed something".  We do nothing without a request from the user,
//  so the signature will not be there unless the /L or /S switches were used on a non-GUI
//  execution, or a "Load all LUTs" button was clicked from the GUI.  Hopefully, this
//  approach will provide increased visibility into LUT changes outside our control.
//

// Result of testing for "linear" LUT
//
typedef enum tag_IS_LINEAR {
	IL_LINEAR_8 = 0,					// All: 0x0000, 0x0100, 0x0200, 0x0300, 0x0400 ...
	IL_LINEAR_16 = 1,					// All: 0x0000, 0x0101, 0x0202, 0x0303, 0x0404 ...
	IL_SIGNATURE = 2,					// Red: 0x0000, 0x0102, 0x0203, 0x0304, 0x0404, then IL_LINEAR_16
	IL_ZERO_POINTER = 3,				// No pointer provided (zero)
	IL_NOT_LINEAR = 4					// Anything else (i.e. a LUT that is not linear)
} IS_LINEAR;

// Function to test LUT for linearity
//
IS_LINEAR IsLinear(LUT * pLUT);

// Write a "signed" linear LUT to a provided address (caller owns memory)
//
void GetSignedLUT(LUT * pLUT);
