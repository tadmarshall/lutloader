// Profile.h -- Profile class for handling display profiles.
//

#pragma once
#include "stdafx.h"
#include <icm.h>
#include "VideoCardGammaTag.h"

// An entry in the tag table
//
typedef struct tag_TAG_TABLE_ENTRY {
	DWORD		Signature;
	DWORD		Offset;
	DWORD		Size;
} TAG_TABLE_ENTRY;

class Profile {

public:
	Profile();
	Profile(const Profile & from);
	Profile & operator = (const Profile &);
	~Profile();
	void SetName(const wchar_t * profileName);
	wstring GetName(void) const;
	wstring Load(void);
	wstring DetailsString(void) const;

	static wchar_t * FindProfileName(HKEY hKeyBase, const wchar_t * registryKey, bool * perUser);

private:
	wstring				ProfileName;					// Name of profile file without path
	wstring				ErrorString;					// If Load() fails, record error here
	LARGE_INTEGER		ProfileSize;					// File size
	PROFILEHEADER *		ProfileHeader;					// Header (128 bytes)
	DWORD				TagCount;						// Count of tags in profile
	TAG_TABLE_ENTRY *	TagTable;						// Table of tags
	int					vcgtIndex;						// Location of VCGT tag in TagTable, or -1
	VCGT_HEADER *		pVCGT;							// Video Card Gamma Tag
	int					wcsProfileIndex;				// Location of WCS tag in TagTable, or -1
	wstring				WCS_ColorDeviceModel;			// XML copied from WCS profile
	wstring				WCS_ColorAppearanceModel;
	wstring				WCS_GamutMapModel;
};
