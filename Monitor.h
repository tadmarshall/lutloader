// Monitor.h -- Monitor class for handling display monitors returned by EnumDisplayDevices().
//

#pragma once
#include "stdafx.h"

class Monitor {

public:
	Monitor(int adapterNumber, PDISPLAY_DEVICE displayMonitor);
	wstring SummaryString(void) const;
	wstring DetailsString(void) const;

	static bool IsMonitorActive(PDISPLAY_DEVICE displayMonitor);
	static void ClearMonitorList(bool freeAllMemory);
	static void AddMonitor(Monitor * const monitor);
	static size_t GetMonitorListSize(void);
	static Monitor & GetMonitor(size_t monitorNumber);
	static wstring GetMonitorDeviceString(size_t monitorNumber);

private:
	static wchar_t * GetProfileName(HKEY hKeyBase, wchar_t * registryKey, BOOL * perUser);
	int		AdapterNumber;
	DWORD	StateFlags;
	wstring DeviceName;
	wstring DeviceString;
	wstring DeviceID;
	wstring DeviceKey;
	wstring userProfile;
	wstring systemProfile;
	bool	activeProfileIsUserProfile;
};

// VCGT table: 3 fixed size header values followed by (usually) red, green and blue arrays of LUT value items
//
typedef struct tag_VCGT_table {
	WORD					vcgtChannels;		// Number of channels, usually 3 (but could be 1)
	WORD					vcgtCount;			// Count of items in each channel, usually 256
	WORD					vcgtItemSize;		// Size of each item, usually 2 but can be 1
	BYTE					vcgtData[1];		// One or more arrays of items, each item vcgtItemSize bytes, each array vcgtCount long
} VCGT_TABLE;

// Signed fixed-point number, 32-bits:
//  1 bit  -- sign bit
// 15 bits -- integer part
// 16 bits -- fractional part
//
typedef long s15Fixed16Number;

// VCGT formula: gamma must be greater than 0.0, min and max must between 0.0 and 1.0
//
typedef struct tag_VCGT_formula {
	s15Fixed16Number		vcgtRedGamma;		// Red gamma, min and max
	s15Fixed16Number		vcgtRedMin;
	s15Fixed16Number		vcgtRedMax;

	s15Fixed16Number		vcgtGreenGamma;		// Green gamma, min and max
	s15Fixed16Number		vcgtGreenMin;
	s15Fixed16Number		vcgtGreenMax;

	s15Fixed16Number		vcgtBlueGamma;		// Blue gamma, min and max
	s15Fixed16Number		vcgtBlueMin;
	s15Fixed16Number		vcgtBlueMax;
} VCGT_FORMULA;

// VCGT type enumeration, used in vcgtType to select VCGT_TABLE or VCGT_TYPE_FORMULA
//
enum {
	VCGT_TYPE_TABLE = 0,
	VCGT_TYPE_FORMULA = 1
};

// VCGT data can be in the form of a table or a formula
//
typedef union tag_VCGT_table_or_formula {
	VCGT_TABLE				t;					// VCGT table
	VCGT_FORMULA			f;					// VCGT formula
} VCGT_TABLE_OR_FORMULA;

// VCGT tag format
//
typedef struct tag_VCGT_header {
	DWORD					vcgtSignature;		// Always 'vcgt'
	DWORD					vcgtReserved;		// Always zero
	DWORD					vcgtType;			// VCGT tag type: 0 == table, 1 == formula
	VCGT_TABLE_OR_FORMULA	vcgtContents;		// Either a table of a formula, based on vcgtType
} VCGT_HEADER;

// Example of VCGT_HEADER use:
//
//		VCGT_HEADER *		pVCGT;
//
//		if (VCGT_TYPE_TABLE == pVCGT->vcgtType) {
//			DWORD channelCount = pVCGT->vcgtContents.t.vcgtChannels;
//		} else {
//			s15Fixed16Number redGamma = pVCGT->vcgtContents.f.vcgtRedGamma;
//		}
