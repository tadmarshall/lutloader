// Profile.h -- Profile class for handling display profiles.
//

#pragma once
#include "stdafx.h"
#include <icm.h>
#include "LUT.h"
#include "VideoCardGammaTag.h"

// Optional "features"
//
#define READ_EMBEDDED_WCS_PROFILE 0

// Forward references
//
class Profile;

typedef vector <Profile *> ProfileList;

// An entry in the tag table
//
typedef struct tag_EXTERNAL_TAG_TABLE_ENTRY {		// As stored in a profile file
	DWORD		Signature;
	DWORD		Offset;
	DWORD		Size;
} EXTERNAL_TAG_TABLE_ENTRY;

typedef struct tag_TAG_TABLE_ENTRY {				// As maintained in memory
	DWORD		Signature;
	DWORD		Offset;
	DWORD		Size;
	DWORD		Type;
} TAG_TABLE_ENTRY;

class Profile {

public:
	Profile(const wchar_t * profileName);
	~Profile();

	static Profile * Add(Profile * profile);
	static void ClearList(bool freeAllMemory);

	wstring GetName(void) const;
	wstring LoadFullProfile(bool forceReload);
	bool IsBadProfile(void) const;
	DWORD GetProfileClass(void) const;
	LUT * GetLutPointer(void) const;
	wstring DetailsString(void);
	LUT_COMPARISON CompareLUT(LUT * otherLUT, DWORD * maxError, DWORD * totalError);
	bool HasEmbeddedWcsProfile(void) const;
	bool EditRegistryProfileList(HKEY hKeyBase, const wchar_t * registryKey, bool moveToEnd);
	bool InsertIntoRegistryProfileList(HKEY hKeyBase, const wchar_t * registryKey);

	static Profile * GetAllProfiles(HKEY hKeyBase, const wchar_t * registryKey, bool * perUser, ProfileList & profileList);

private:
	bool ShowTagTypeDescription(TAG_TABLE_ENTRY * tagEntry, wstring & outputText);
	bool ShowShortTagContents(TAG_TABLE_ENTRY * tagEntry, wstring & outputText);
	bool ReadProfileBytes(DWORD offset, DWORD byteCount, BYTE * returnedBytePtr);
	bool ReadProfileBytesFromOpenFile(HANDLE hFile, DWORD offset, DWORD byteCount, BYTE * returnedBytePtr);

	wstring				ProfileName;					// Name of profile file without path
	bool				loaded;							// 'true' if already loaded from disk
	bool				failed;							// Should align with ErrorString, means bad profile
	wstring				ErrorString;					// If LoadFullProfile() fails, record error here
	wstring				ValidationFailures;				// Profile issues that don't prevent loading
	LARGE_INTEGER		ProfileSize;					// File size
	PROFILEHEADER *		ProfileHeader;					// Header (128 bytes)
	DWORD				TagCount;						// Count of tags in profile
	TAG_TABLE_ENTRY *	TagTable;						// Table of tags
	DWORD * *			sortedTags;						// Pointers into tags table in a sorted order
	int					vcgtIndex;						// Location of VCGT tag in TagTable, or -1
	VCGT_HEADER *		pVCGT;							// Video Card Gamma Tag structure as on disk
	VCGT_HEADER			vcgtHeader;						// A byte-swapped version for us to use
	LUT *				pLUT;							// A byte-swapped copy of the LUT from the vcgt
	int					wcsProfileIndex;				// Location of WCS tag in TagTable, or -1
#if READ_EMBEDDED_WCS_PROFILE
	wstring				WCS_ColorDeviceModel;			// XML copied from WCS profile
	wstring				WCS_ColorAppearanceModel;
	wstring				WCS_GamutMapModel;
#endif
};
