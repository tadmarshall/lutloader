// VideoCardGammaTag.h -- definitions for working with 'vcgt' tags in profile files
//

#pragma once

// Note: 'vcgt' sections, like everything in ICC profiles, are stored big-endian, so numeric values
// must be byte-swapped for use on little-endian Intel machines like Windows systems.  Non-numeric
// values (e.g. character data, such as the 'vcgt' signature) are stored in their natural byte order
// (i.e. first character followed by second character, etc.) and do not required byte-swapping.
//

// VCGT table: fixed header values followed by red, green and blue arrays of LUT value items
//
typedef struct tag_VCGT_TABLE {
	WORD					vcgtChannels;		// Number of channels, usually 3 (but could be 1)
	WORD					vcgtCount;			// Count of items in each channel, usually 256
	WORD					vcgtItemSize;		// Size of each item in bytes, usually 2 but can be 1
	BYTE					vcgtData[1];		// 'vcgtChannels' arrays of items, each item is
												//  'vcgtItemSize' bytes, each array 'vcgtCount' long
} VCGT_TABLE;

// Signed fixed-point number, 32-bits:
//  1 bit  -- sign bit
// 15 bits -- integer part
// 16 bits -- fractional part
//
// Examples:
// 0x00010000 == 1.0 (integer part 1, fractional part 0)
// 0x00018000 == 1.5 (integer part 1, fractional part represents 32768 divided by 65536)
//
typedef long s15Fixed16Number;

// VCGT formula: gamma must be greater than 0.0, min and max must between 0.0 and 1.0
//
typedef struct tag_VCGT_FORMULA {
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
typedef enum tag_VCGT_TYPE {
	VCGT_TYPE_TABLE = 0,
	VCGT_TYPE_FORMULA = 1
} VCGT_TYPE;									// Stored in DWORD in VCGT_HEADER

// VCGT data can be in the form of a table or a formula
//
typedef union tag_VCGT_table_or_formula {
	VCGT_TABLE				t;					// VCGT table
	VCGT_FORMULA			f;					// VCGT formula
} VCGT_TABLE_OR_FORMULA;

// VCGT tag format
//
typedef struct tag_VCGT_header {
	DWORD					vcgtSignature;		// Always 'vcgt', stored as 0x76, 0x63, 0x67, 0x74 ('v', 'c', 'g', 't')
	DWORD					vcgtReserved;		// Always zero
	VCGT_TYPE				vcgtType;			// (DWORD) VCGT tag type: 0 == table, 1 == formula
	VCGT_TABLE_OR_FORMULA	vcgtContents;		// Either a table of a formula, based on vcgtType
} VCGT_HEADER;

// Example of VCGT_HEADER use:
//
//		VCGT_HEADER *		pVCGT;				// Assume that data are still in big-endian form
//
//		if ( VCGT_TYPE_TABLE == swap32(pVCGT->vcgtType) ) {
//			WORD channelCount = swap16(pVCGT->vcgtContents.t.vcgtChannels);
//		} else {
//			s15Fixed16Number redGamma = swap32(pVCGT->vcgtContents.f.vcgtRedGamma);
//		}
