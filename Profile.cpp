// Profile.cpp -- Profile class for handling display profiles.
//

#include "stdafx.h"
#include <math.h>
#include "LUT.h"
#include "Profile.h"
#include "Utility.h"
#include <strsafe.h>
//#include <banned.h>

extern wchar_t * ColorDirectory;
extern wchar_t * ColorDirectoryErrorString;

// Reverse engineered embedded WCS profile header
//
typedef struct tag_WCS_IN_ICC_HEADER {
	DWORD					wcshdrSignature;	// Always 'MS10'
	DWORD					wcshdrReserved;		// Always zero

	DWORD					wcshdrCDMoffset;	// Offset to Color Device Model
	DWORD					wcshdrCDMsize;		// Size of Color Device Model

	DWORD					wcshdrCAMoffset;	// Offset to Color Appearance Model
	DWORD					wcshdrCAMsize;		// Size of Color Appearance Model

	DWORD					wcshdrGMMoffset;	// Offset to Gamut Map Model
	DWORD					wcshdrGMMsize;		// Size of Gamut Map Model
} WCS_IN_ICC_HEADER;

// ICC encoding of date and time of profile creation
//
typedef struct tag_dateTimeNumber {
	unsigned short		year;
	unsigned short		month;
	unsigned short		day;
	unsigned short		hour;
	unsigned short		minute;
	unsigned short		second;
} dateTimeNumber;

// List of CMMs
//
const NAME_LOOKUP knownCMMs[] = {
	{ 'ADBE',				L"Adobe CMM" },
	{ 'ACMS',				L"Agfa CMM" },
	{ 'appl',				L"Apple CMM" },
	{ 'CCMS',				L"ColorGear CMM" },
	{ 'UCCM',				L"ColorGear CMM Lite" },
	{ 'UCMS',				L"ColorGear CMM C" },
	{ 'EFI ',				L"EFI CMM" },
	{ 'FF  ',				L"Fuji Film CMM" },
	{ 'HCMM',				L"Harlequin RIP CMM" },
	{ 'argl',				L"Argyll CMS CMM" },
	{ 'LgoS',				L"LogoSync CMM" },
	{ 'HDM ',				L"Heidelberg CMM" },
	{ 'lcms',				L"Little CMS CMM" },
	{ 'KCMS',				L"Kodak CMM" },
	{ 'MCML',				L"Konica Minolta CMM" },
	{ 'SIGN',				L"Mutoh CMM" },
	{ 'RGMS',				L"DeviceLink CMM" },
	{ 'SICC',				L"SampleICC CMM" },
	{ '32BT',				L"the imaging factory CMM" },
	{ 'WTG ',				L"Ware to Go CMM" },
	{ 'zc00',				L"Zoran CMM" }
};

// Windows Color System hybrid profiles -- WCS embedded in ICC profile
//
#ifndef CLASS_CAMP
#define CLASS_CAMP              'camp'
#endif
#ifndef CLASS_GMMP
#define CLASS_GMMP              'gmmp'
#endif
#ifndef CLASS_CDMP
#define CLASS_CDMP              'cdmp'
#endif

// List of profile classes
//
const NAME_LOOKUP profileClasses[] = {
	{ CLASS_MONITOR,		L"Display Device" },
	{ CLASS_PRINTER,		L"Output Device" },
	{ CLASS_SCANNER,		L"Input Device" },
	{ CLASS_LINK,			L"DeviceLink" },
	{ CLASS_ABSTRACT,		L"Abstract" },
	{ CLASS_COLORSPACE,		L"ColorSpace Conversion" },
	{ CLASS_NAMED,			L"Named Color" },
	{ CLASS_CAMP,			L"WCS Viewing Condition" },
	{ CLASS_GMMP,			L"WCS Gamut Mapping" },
	{ CLASS_CDMP,			L"WCS Device" }
};

// List of color spaces
//
const NAME_LOOKUP colorSpaces[] = {
	{ 'XYZ ',				L"XYZData" },
	{ 'Lab ',				L"labData" },
	{ 'Luv ',				L"luvData" },
	{ 'YCbr',				L"YCbCrData" },
	{ 'Yxy ',				L"YxyData" },
	{ 'RGB ',				L"rgbData" },
	{ 'GRAY',				L"grayData" },
	{ 'HSV ',				L"hsvData" },
	{ 'HLS ',				L"hlsData" },
	{ 'CMYK',				L"cmykData" },
	{ 'CMY ',				L"cmyData" },
	{ '2CLR',				L"2colourData" },
	{ '3CLR',				L"3colourData" },
	{ '4CLR',				L"4colourData" },
	{ '5CLR',				L"5colourData" },
	{ '6CLR',				L"6colourData" },
	{ '7CLR',				L"7colourData" },
	{ '8CLR',				L"8colourData" },
	{ '9CLR',				L"9colourData" },
	{ 'ACLR',				L"10colourData" },
	{ 'BCLR',				L"11colourData" },
	{ 'CCLR',				L"12colourData" },
	{ 'DCLR',				L"13colourData" },
	{ 'ECLR',				L"14colourData" },
	{ 'FCLR',				L"15colourData" }
};

// List of Primary Platforms
//
const NAME_LOOKUP knownPlatforms[] = {
	{ 'APPL',				L"Apple Computer, Inc." },
	{ 'MSFT',				L"Microsoft Corporation" },
	{ 'SGI ',				L"Silicon Graphics, Inc." },
	{ 'SUNW',				L"Sun Microsystems, Inc." },
	{ 'TGNT',				L"Taligent" }
};

// List of Rendering Intents
//
const NAME_LOOKUP renderingIntents[] = {
	{ INTENT_PERCEPTUAL,				L"Perceptual" },
	{ INTENT_RELATIVE_COLORIMETRIC,		L"Media-Relative Colorimetric" },
	{ INTENT_SATURATION,				L"Saturation" },
	{ INTENT_ABSOLUTE_COLORIMETRIC,		L"ICC-Absolute Colorimetric" }
};

// List of known tags
//
const NAME_LOOKUP knownTags[] = {
	{ 'A2B0',				L"Device to PCS perceptual transform" },
	{ 'A2B1',				L"Device to PCS colorimetric transform" },
	{ 'A2B2',				L"Device to PCS saturation transform" },
	{ 'B2A0',				L"PCS to device perceptual transform" },
	{ 'B2A1',				L"PCS to device colorimetric transform" },
	{ 'B2A2',				L"PCS to device saturation transform" },
	{ 'B2D0',				L"BToD0" },
	{ 'B2D1',				L"BToD1" },
	{ 'B2D2',				L"BToD2" },
	{ 'B2D3',				L"BToD3" },
	{ 'bfd ',				L"UcrBg" },
	{ 'bkpt',				L"Media black point" },
	{ 'bTRC',				L"Blue tone reproduction curve" },
	{ 'bXYZ',				L"Blue matrix column" },
	{ 'calt',				L"Calibration date and time" },
	{ 'chad',				L"Chromatic adaptation" },
	{ 'chrm',				L"Chromaticity type" },
	{ 'CIED',				L"GretagMacbeth private" },
	{ 'ciis',				L"Colorimetric intent image state" },
	{ 'clot',				L"Colorant table out" },
	{ 'clro',				L"Colorant order" },
	{ 'clrt',				L"Colorant table" },
	{ 'cprt',				L"Copyright" },
	{ 'crdi',				L"Crd information" },
	{ 'CxF ',				L"GretagMacbeth Color Exchange Format" },
	{ 'D2B0',				L"DToB0" },
	{ 'D2B1',				L"DToB1" },
	{ 'D2B2',				L"DToB2" },
	{ 'D2B3',				L"DToB3" },
	{ 'data',				L"Data" },
	{ 'DDPS',				L"GretagMacbeth Display Device Profile Settings" },
	{ 'desc',				L"Profile description" },
	{ 'DevD',				L"GretagMacbeth private" },
	{ 'devs',				L"Device settings" },
	{ 'dmdd',				L"Device model" },
	{ 'dmnd',				L"Device manufacturer" },
	{ 'dscm',				L"Profile description multi-lingual" },
	{ 'dtim',				L"Date time" },
	{ 'fpce',				L"Focal plane colorimetry estimates" },
	{ 'gamt',				L"Gamut" },
	{ 'gmps',				L"GretagMacbeth private" },
	{ 'gTRC',				L"Green tone reproduction curve" },
	{ 'gXYZ',				L"Green matrix column" },
	{ 'kTRC',				L"Gray tone reproduction curve" },
	{ 'lumi',				L"Luminance" },
	{ 'meas',				L"Alternative measurement type" },
	{ 'meta',				L"Metadata dictionary" },
	{ 'mmod',				L"Make and model" },
	{ 'MS00',				L"Windows Color System embedded profile" },
	{ 'ncl2',				L"Named color 2" },
	{ 'ncol',				L"Named color" },
	{ 'ndin',				L"Native display information" },
	{ 'Pmtr',				L"GretagMacbeth private" },
	{ 'pre0',				L"Preview 0" },
	{ 'pre1',				L"Preview 1" },
	{ 'pre2',				L"Preview 2" },
	{ 'ps2i',				L"PostScript 2 rendering intent" },
	{ 'ps2s',				L"PostScript 2 color space array" },
	{ 'psd0',				L"PostScript 2 color rendering dictionary 0" },
	{ 'psd1',				L"PostScript 2 color rendering dictionary 1" },
	{ 'psd2',				L"PostScript 2 color rendering dictionary 2" },
	{ 'psd3',				L"PostScript 2 color rendering dictionary 3" },
	{ 'pseq',				L"Profile sequence description" },
	{ 'psid',				L"Profile sequence ID" },
	{ 'psvm',				L"PostScript 2 color rendering dictionary VM Size" },
	{ 'ptcn',				L"Print condition" },
	{ 'resp',				L"Output response" },
	{ 'rig0',				L"Perceptual rendering intent gamut" },
	{ 'rig2',				L"Saturation rendering intent gamut" },
	{ 'rpoc',				L"Reflection print output colorimetry" },
	{ 'rTRC',				L"Red tone reproduction curve" },
	{ 'rXYZ',				L"Red matrix column" },
	{ 'scrd',				L"Screening description" },
	{ 'scrn',				L"Screening" },
	{ 'targ',				L"Characterization target data set name" },
	{ 'tech',				L"Technology" },
	{ 'vcgt',				L"Video card gamma" },
	{ 'view',				L"Viewing conditions" },
	{ 'vued',				L"Viewing conditions description" },
	{ 'wtpt',				L"Media white point" }
	//{ '',				L"" },
	//{ '',				L"" },
	//{ '',				L"" },
	//{ '',				L"" },
};

// Constructor
//
Profile::Profile(const wchar_t * profileName) :
		ProfileName(profileName),
		loaded(false),
		ProfileHeader(0),
		TagCount(0),
		TagTable(0),
		sortedTags(0),
		pVCGT(0),
		pLUT(0),
		vcgtIndex(-1),
		wcsProfileIndex(-1)
{
	ProfileSize.QuadPart = 0;
	SecureZeroMemory(&vcgtHeader, sizeof(vcgtHeader));
}

// Destructor
//
Profile::~Profile() {
	if (pLUT) {
		delete [] pLUT;
	}
	if (pVCGT) {
		delete [] pVCGT;
	}
	if (TagTable) {
		delete [] TagTable;
	}
	if (sortedTags) {
		delete [] sortedTags;
	}
	if (ProfileHeader) {
		delete [] ProfileHeader;
	}
}

// Vector of profiles
//
static vector <Profile *> profileList;

// Add a profile to the list if it isn't already on it -- if it is already on the list,
// delete the profile we were passed, and return a pointer to the one we found on the list.
//
Profile * Profile::Add(Profile * profile) {
	size_t index = profileList.size();
	for (size_t i = 0; i < index; ++i) {
		if ( profile->ProfileName == profileList[i]->ProfileName ) {
			delete profile;
			return profileList[i];
		}
	}
	profileList.push_back(profile);
	return profile;
}

// Clear the list of profiles
//
void Profile::ClearList(bool freeAllMemory) {
	size_t count = profileList.size();
	for (size_t i = 0; i < count; ++i) {
		delete profileList[i];
	}
	profileList.clear();
	if ( freeAllMemory && (profileList.capacity() > 0) ) {
		vector <Profile *> dummy;
		profileList.swap(dummy);
	}
}

// Get profile name
//
wstring Profile::GetName(void) const {
	return ProfileName;
}

// Get profile's LUT pointer (may be zero)
//
LUT * Profile::GetLutPointer(void) const {
	return pLUT;
}

// Get the profile's ErrorString
//
wstring Profile::GetErrorString(void) const {
	return ErrorString;
}

// See if this profile has an embedded WCS profile in it
//
bool Profile::HasEmbeddedWcsProfile(void) {
	return ( -1 != wcsProfileIndex );
}

// Return a list of all profiles associated with a given registry key, indicating the 'default' profile from the list
//
Profile * Profile::GetAllProfiles(HKEY hKeyBase, const wchar_t * registryKey, bool * perUser, ProfileList & pList) {
	HKEY hKey;
	int len = 0;
	Profile * profile = 0;
	pList.clear();

	if (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase, registryKey, 0, KEY_QUERY_VALUE, &hKey)) {
		DWORD dataSize;

		// Fetch the list of profiles
		//
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"ICMProfile", NULL, NULL, NULL, &dataSize)) {
			dataSize += sizeof(wchar_t);
			BYTE * profileListBase = reinterpret_cast<BYTE *>(malloc(dataSize));
			if (profileListBase) {
				memset(profileListBase, 0, dataSize);
				RegQueryValueEx(hKey, L"ICMProfile", NULL, NULL, profileListBase, &dataSize);
				wchar_t * profileListPtr = reinterpret_cast<wchar_t *>(profileListBase);

				// Find the last profile in the list
				//
				while (*profileListPtr) {
					profile = Add(new Profile(profileListPtr));
					pList.push_back(profile);
					len = 1 + StringLength(profileListPtr);
					profileListPtr += len;
				}
				free(profileListBase);
			}
		}

		// If caller wants to know if UsePerUserProfiles exists and is set to REG_DWORD:1, let him know
		//
		if (perUser) {
			DWORD data = 0;
			DWORD dataType;
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"UsePerUserProfiles", NULL, &dataType, NULL, &dataSize)) {
				if ( (REG_DWORD == dataType) && (sizeof(data) == dataSize) ) {
					RegQueryValueEx(hKey, L"UsePerUserProfiles", NULL, NULL, reinterpret_cast<BYTE *>(&data), &dataSize);
				}
			}
			*perUser = (1 == data);
		}
		RegCloseKey(hKey);
	}
	return profile;
}

// This routine serves to two roles: removing a profile's association with a monitor, on either
// the user or system list, and setting the default profile for a monitor (either list).  In both
// cases the profile name is removed from the list.  In the case of setting the default profile,
// the name is then reinserted at the end of the list.
//
bool Profile::EditRegistryProfileList(HKEY hKeyBase, const wchar_t * registryKey, bool moveToEnd) {
	wchar_t profileName[1024];
	HKEY hKey;
	DWORD dataSize;
	DWORD dataType;
	bool success = false;

	StringCbCopy(profileName, sizeof(profileName), ProfileName.c_str());
#if 0
	LONG lStatus = RegOpenKeyEx(hKeyBase, registryKey, 0, KEY_ALL_ACCESS, &hKey);
	if (ERROR_SUCCESS != lStatus) {
		wstring errorString = ShowError(L"RegOpenKeyEx", lStatus);
		errorString += L"\r\n\r\n";
	} else {
#else
	if (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase, registryKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey)) {
#endif
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"ICMProfile", NULL, NULL, NULL, &dataSize)) {
			dataSize += 2 * sizeof(wchar_t);
			BYTE * fromList = reinterpret_cast<BYTE *>(malloc(dataSize));
			BYTE * toList = reinterpret_cast<BYTE *>(malloc(dataSize));
			if ( fromList && toList ) {
				SecureZeroMemory(fromList, dataSize);
				SecureZeroMemory(toList, dataSize);
				RegQueryValueEx(hKey, L"ICMProfile", NULL, &dataType, fromList, &dataSize);
				wchar_t * fromPtr = reinterpret_cast<wchar_t *>(fromList);
				wchar_t * toPtr = reinterpret_cast<wchar_t *>(toList);
				wchar_t * fp;
				wchar_t * pn;

				// Copy profiles that do not match the given profile name
				//
				while (*fromPtr) {
					fp = fromPtr;
					pn = profileName;

					// Compare strings until we hit a difference or a NUL in either string
					//
					while ( *fp && *pn && (*fp == *pn) ) {
						++pn;
						++fp;
					}
					if ( *fp == *pn ) {

						// The strings matched, so just skip over the source string
						//
						fromPtr = fp + 1;
					} else {

						// The strings did not match, so copy this one
						//
						while (*fromPtr) {
							*toPtr++ = *fromPtr++;
						}
						++fromPtr;
						++toPtr;
					}
				}

				// If moveToEnd is false, we just removed a profile association.  If moveToEnd is true,
				// we reinsert it at the end of the list and it becomes the new default profile.
				//
				if (moveToEnd) {

					// We are done reading and skipping, now copy the profile name to the end
					//
					pn = profileName;
					while (*pn) {
						*toPtr++ = *pn++;
					}
				}
				*toPtr++ = 0;		// End string with two NULs
				*toPtr++ = 0;

				// Write the modified profile list to the registry
				//
				dataSize = static_cast<DWORD>(reinterpret_cast<BYTE *>(toPtr) - toList);
				success = (ERROR_SUCCESS == RegSetValueEx(hKey, L"ICMProfile", NULL, dataType, toList, dataSize));
			}
			if (fromList) {
				free(fromList);
			}
			if (toList) {
				free(toList);
			}
		}
		RegCloseKey(hKey);
	}
	return success;
}

// Try to make one of the many four "character" entries in an ICC profile display well.
// Offer four spaces as an option for zero, but return true to indicate the zero.
// Otherwise, return the characters with spaces substituted for any zeros in the entry.
//
bool ConvertFourBytesForDisplay(DWORD bytes, __out_bcount(len) wchar_t * output, size_t len) {
	if ( len < (5 * sizeof(wchar_t)) ) {
		return true;
	}
	if ( 0 == bytes ) {
		StringCbCopy(output, len, L"    ");
		return true;
	}
	BYTE * pb = reinterpret_cast<BYTE *>(&bytes);
	for ( size_t i = 0; i < 4; ++i ) {
		output[i] = ( (*pb >= 32) && (*pb < 127) ) ? static_cast<wchar_t>(*pb) : L' ';
		++pb;
	}
	output[4] = 0;
	return false;
}

// Routine for qsort() to call to help create a sorted index into the tag table
//
	int ComparePtrToDWORD(const void * elem1, const void * elem2) {
		DWORD item1;
		DWORD item2;
		const BYTE * ptr1 = *reinterpret_cast<const BYTE * const *>(elem1);
		const BYTE * ptr2 = *reinterpret_cast<const BYTE * const *>(elem2);
		BYTE * pb = reinterpret_cast<BYTE *>(&item1);
		for (size_t i = 0; i < 4; ++i) {
			if ( (ptr1[i] >= 'a') && (ptr1[i] <= 'z') ) {
				pb[i] = ptr1[i] + 'A' - 'a';
			} else {
				pb[i] = ptr1[i];
			}
		}
		pb = reinterpret_cast<BYTE *>(&item2);
		for (size_t i = 0; i < 4; ++i) {
			if ( (ptr2[i] >= 'a') && (ptr2[i] <= 'z') ) {
				pb[i] = ptr2[i] + 'A' - 'a';
			} else {
				pb[i] = ptr2[i];
			}
		}
		if (item1 < item2) {
			return -1;
		} else if (item1 == item2) {
			return 0;
		} else {
			return 1;
		}
	}

// Load profile info from disk
//
wstring Profile::LoadFullProfile(bool forceReload) {

	wstring s;
	wchar_t buf[1024];
	wchar_t displayChars[5];
	BYTE * pb;
	const wchar_t * lookupString;
	bool foundNonZero;

	// Quit early if no profile
	//
	if (ProfileName.empty()) {
		return s;
	}

	// Don't load twice unless requested to do so
	//
	if (loaded && !forceReload) {
		return ErrorString;
	}

	// If we got this far, we'll set the 'loaded' flag ... its purpose is to
	// prevent duplicate work, not as a 'success' indication
	//
	loaded = true;

	// Also, if this is not the first time here (i.e. 'forceReload' is true),
	// then we need to clean up from prior passes ... releasing memory, for example
	//
	if (forceReload) {
		ErrorString.clear();
		ValidationFailures.clear();
		ProfileSize.QuadPart = 0;
		if (ProfileHeader) {
			delete [] ProfileHeader;
			ProfileHeader = 0;
		}
		TagCount = 0;
		if (TagTable) {
			delete [] TagTable;
			TagTable = 0;
		}
		if (sortedTags) {
			delete [] sortedTags;
			sortedTags = 0;
		}
		vcgtIndex = -1;
		if (pVCGT) {
			delete [] pVCGT;
			pVCGT = 0;
		}
		SecureZeroMemory(&vcgtHeader, sizeof(VCGT_HEADER));
		if (pLUT) {
			delete [] pLUT;
			pLUT = 0;
		}
		wcsProfileIndex = -1;
		WCS_ColorDeviceModel.clear();
		WCS_ColorAppearanceModel.clear();
		WCS_GamutMapModel.clear();
	}

	// Get color directory
	//
	wchar_t filepath[1024];
	if (ColorDirectory) {
		StringCbCopy(filepath, sizeof(filepath), ColorDirectory);
	} else {
		ErrorString = ColorDirectoryErrorString;
		return ErrorString;
	}

	// Open the profile file
	//
	StringCbCat(filepath, sizeof(filepath), L"\\");
	StringCbCat(filepath, sizeof(filepath), ProfileName.c_str());

	HANDLE hFile = CreateFileW(
			filepath,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING, 
			0,
			NULL
	);
	if (INVALID_HANDLE_VALUE == hFile) {
		wstring message = L"Cannot open profile file \"";
		message += filepath;
		message += L"\".\r\n\r\n";
		ErrorString += ShowError(L"CreateFile", 0, message.c_str());
		return ErrorString;
	}

	// Get file size
	//
	BOOL bRet = GetFileSizeEx(hFile, &ProfileSize);
	if (0 == bRet) {
		wstring message = L"Cannot determine the size of profile file \"";
		message += filepath;
		message += L"\".\r\n\r\n";
		ErrorString += ShowError(L"GetFileSizeEx", 0, message.c_str());
		CloseHandle(hFile);
		return ErrorString;
	}

	// Make sure that the file size is acceptable
	//
	if ( ProfileSize.QuadPart < (sizeof(PROFILEHEADER) + sizeof(TagCount)) ) {
		wstring message = L"File \"";
		message += filepath;
		message += L"\" is not a valid ICC profile.\r\nThe file size (";
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"%I64u bytes) is less than the size of the required profile header with tag count (132 bytes).\r\n\r\n",
				ProfileSize );
		message += buf;
		ErrorString = message;
		CloseHandle(hFile);
		return ErrorString;
	}

	// Read the profile header
	//
	ProfileHeader = new PROFILEHEADER;
	SecureZeroMemory(ProfileHeader, sizeof(PROFILEHEADER));
	DWORD cb = 0;
	bRet = ReadFile(hFile, ProfileHeader, sizeof(PROFILEHEADER), &cb, NULL);
	if ( 0 == bRet ) {
		wstring message = L"Cannot read header of profile file \"";
		message += filepath;
		message += L"\".\r\n\r\n";
		ErrorString += ShowError(L"ReadFile", 0, message.c_str());
		CloseHandle(hFile);
		return ErrorString;
	}
	if ( cb != sizeof(PROFILEHEADER) ) {
		wstring message = L"Cannot read header of profile file \"";
		message += filepath;
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"\".\r\nOnly %u bytes were read.  The required profile header is 128 bytes.\r\n\r\n",
				cb );
		message += buf;
		ErrorString = message;
		CloseHandle(hFile);
		return ErrorString;
	}

	// We have read 128 bytes of profile header, now validate it
	//
	DWORD profileClaimedSize = swap32(ProfileHeader->phSize);
	if ( ProfileSize.LowPart != profileClaimedSize ) {
		ValidationFailures += L"The profile's size on disk does not match the size stored in the file.\r\n";
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The disk size is %u bytes, but the profile's header says it is %u bytes.\r\n\r\n",
				ProfileSize.LowPart,
				profileClaimedSize );
		ValidationFailures += buf;
	}

	// The Color Management Module (CMM) should be one of the known ones, or zero
	//
	if (ProfileHeader->phCMMType) {
		lookupString = LookupName( knownCMMs, _countof(knownCMMs), swap32(ProfileHeader->phCMMType) );
		if ( 0 == *lookupString ) {
			ConvertFourBytesForDisplay(ProfileHeader->phCMMType, displayChars, sizeof(displayChars));
			StringCbPrintf(
					buf,
					sizeof(buf),
					L"The profile's preferred Color Management Module (CMM) is unrecognized.\r\n"
					L"The preferred CMM is reported as '%s', but should be either zero or a registered CMM.\r\n\r\n",
					displayChars );
			ValidationFailures += buf;
		}
	}

	// Report very high version numbers
	//
	pb = reinterpret_cast<BYTE *>(&ProfileHeader->phVersion);
	if ( *pb > 9 ) {
		ValidationFailures += L"The profile's version number is unrecognized.\r\nThe version is reported as ";
		StringCbPrintf(buf, sizeof(buf), L"%d.%d.%d .\r\n\r\n", pb[0], (pb[1]>>4), (pb[1]&0x0F));
		ValidationFailures += buf;
	}

	// The profile class should be one of the known ones
	//
	lookupString = LookupName( profileClasses, _countof(profileClasses), swap32(ProfileHeader->phClass) );
	if ( 0 == *lookupString ) {
		ConvertFourBytesForDisplay(ProfileHeader->phClass, displayChars, sizeof(displayChars));
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The profile's profile/device class is unrecognized.\r\n"
				L"The class is reported as '%s', but should be 'mntr' representing a Display Device profile.\r\n\r\n",
				displayChars );
		ValidationFailures += buf;
	}

	// The color space should be a known one
	//
	lookupString = LookupName( colorSpaces, _countof(colorSpaces), swap32(ProfileHeader->phDataColorSpace) );
	if ( 0 == *lookupString ) {
		ConvertFourBytesForDisplay(ProfileHeader->phDataColorSpace, displayChars, sizeof(displayChars));
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The profile's color space of data is unrecognized.\r\n"
				L"The color space is reported as '%s', but should be 'RGB ' for a Display Device profile.\r\n\r\n",
				displayChars );
		ValidationFailures += buf;
	}

	// The profile connection space should be a known one
	//
	lookupString = LookupName( colorSpaces, _countof(colorSpaces), swap32(ProfileHeader->phConnectionSpace) );
	if ( 0 == *lookupString ) {
		ConvertFourBytesForDisplay(ProfileHeader->phConnectionSpace, displayChars, sizeof(displayChars));
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The profile's connection space (PCS) is unrecognized.\r\n"
				L"The Profile Connection Space is reported as '%s', but should be 'XYZ ' for a Display Device profile.\r\n\r\n",
				displayChars );
		ValidationFailures += buf;
	}

	// The timestamp should pass a few tests
	//
	dateTimeNumber * dt = reinterpret_cast<dateTimeNumber *>(&ProfileHeader->phDateTime[0]);
	dateTimeNumber dateTime;
	dateTime.year   = swap16(dt->year);
	dateTime.month  = swap16(dt->month);
	dateTime.day    = swap16(dt->day);
	dateTime.hour   = swap16(dt->hour);
	dateTime.minute = swap16(dt->minute);
	dateTime.second = swap16(dt->second);
	if ( (dateTime.year < 1990) || (dateTime.year > 2111) ||
		 (dateTime.month > 12) ||
		 (dateTime.day > 31) ||
		 (dateTime.hour > 24) ||
		 (dateTime.minute > 60) ||
		 (dateTime.second > 60)
	) {
		StringCchPrintf(
			buf,
			_countof(buf),
			L"The profile's date and time of first creation are unreasonable.\r\n"
			L"The timestamp is reported as %04d-%02d-%02d %02d:%02d:%02dZ.\r\n\r\n", // RFC3389, NOTE: in 5.6
			dateTime.year,
			dateTime.month,
			dateTime.day,
			dateTime.hour,
			dateTime.minute,
			dateTime.second );
		ValidationFailures += buf;
	}

	// The signature has only one permissible value
	//
	if ( 'acsp' != swap32(ProfileHeader->phSignature) ) {
		ValidationFailures += L"The profile does not contain the required signature.\r\n";
		ConvertFourBytesForDisplay(ProfileHeader->phSignature, displayChars, sizeof(displayChars));
		StringCbPrintf(buf, sizeof(buf), L"The four characters '%s' should be 'acsp'.\r\n\r\n", displayChars);
		ValidationFailures += buf;
	}

	// The primary platform should be one of the known ones, or zero
	//
	if (ProfileHeader->phPlatform) {
		lookupString = LookupName( knownPlatforms, _countof(knownPlatforms), swap32(ProfileHeader->phPlatform) );
		if ( 0 == *lookupString ) {
			ConvertFourBytesForDisplay(ProfileHeader->phPlatform, displayChars, sizeof(displayChars));
			StringCbPrintf(
					buf,
					sizeof(buf),
					L"The profile's primary platform is unrecognized.\r\n"
					L"The primary platform is reported as '%s', but should be either zero or a registered primary platform.\r\n\r\n",
					displayChars );
			ValidationFailures += buf;
		}
	}

	// Only two flag bits are defined
	//
	DWORD flags = swap32(ProfileHeader->phProfileFlags);
	if ( 0 != (flags & ~0xFFFF0000) ) {
		if ( 0 != (flags & ~0xFFFF0003) ) {
			ValidationFailures += L"The profile's flags field has reserved bits set.  The low-order 16 bits are specified by the ICC.\r\n";
		} else {
			ValidationFailures += L"The profile's flags field indicates that this profile is intended to be embedded in an image.\r\n";
		}
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The value stored in the low-order 16 bits is 0x%04x.\r\n\r\n", (0x0000FFFF & flags) );
		ValidationFailures += buf;
	}

	// I'm going to give everyone a free pass on anything they put in the manufacturer and
	// model fields.  The ICC says that these too must be registered or zero, but I don't think
	// that's reasonable.
	//

	// Four bits are defined in the low-order 32 bits of the attributes field
	//
	flags = swap32(ProfileHeader->phAttributes[1]);
	if ( 0 != (flags & ~0x0000000F) ) {
		ValidationFailures += L"The profile's attributes field has reserved bits set.  The low-order 32 bits are specified by the ICC.\r\n";
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The value stored in the low-order 32 bits is 0x%08x.\r\n\r\n",
				flags );
		ValidationFailures += buf;
	}

	// There are four valid rendering intents, numbered 0 to 3
	//
	flags = swap32(ProfileHeader->phRenderingIntent);
	if ( 0 != (flags & ~0x00000003) ) {
		ValidationFailures += L"The profile's specified rendering intent is not one of the four valid values.\r\n";
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The value specified for rendering intent is 0x%08x.\r\n\r\n",
				flags );
		ValidationFailures += buf;
	}

	// The only legal PCS illuminant is D50, defined as X=0.9642, Y=1.0000, Z=0.8249 and encoded
	// (according to the version 4.2.0.0 ICC spec) as X=7B6Bh, Y=8000h, Z=6996h.  These numbers
	// are specified for the ICC's fixed-16 format and need to be doubled to match the s15fixed16
	// format used by the CIEXYZ fields.  I also allow some fudge factor since they only need to
	// be right to four digits (1 part in 10000, not 1 in 65536).
	//
	long X = swap32(ProfileHeader->phIlluminant.ciexyzX);
	long Y = swap32(ProfileHeader->phIlluminant.ciexyzY);
	long Z = swap32(ProfileHeader->phIlluminant.ciexyzZ);
	if ( (X < 0x0F6D3) || (X > 0x0F6D9) ||
		 (Y < 0x0FFFC) || (Y > 0x10003) ||
		 (Z < 0x0D329) || (Z > 0x0D32F) ) {
		ValidationFailures += L"The profile's PCS illuminant is not D50 (X=0.9642, Y=1.0000, Z=0.8249) as specified by the ICC.\r\n";
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"The value specified for PCS illuminant is X=%6.4f, Y=%6.4f, Z=%6.4f .\r\n\r\n",
				double(X) / double(65536),
				double(Y) / double(65536),
				double(Z) / double(65536) );
		ValidationFailures += buf;
	}

	// I'm giving another free pass on the creator field.  Why the ICC feels that every profile
	// creator must be registered with them is beyond me.
	//

	// Almost no one is using the profile ID field, which is supposed to be a checksum calculated
	// using RFC 1321 MD5 128bit after zeroing the flags (phProfileFlags), rendering intent
	// (phRenderingIntent) and this profile ID (not defined in Microsoft's icm.h, called icProfileID
	// in the ICC's icProfileHeader.h).  The profile ID was apparently introduced in profile version
	// 4.0.0, it's not in the 2.4.0 spec.
	//
	// Maybe come back to it later ... skip it for now.
	//

	// The rest of the profile header is supposed to be all zeros.
	//
	foundNonZero = false;
	pb = 16 + sizeof(ProfileHeader->phCreator) + reinterpret_cast<BYTE *>(&ProfileHeader->phCreator);
	BYTE * pbEnd = sizeof(PROFILEHEADER) + reinterpret_cast<BYTE *>(ProfileHeader);
	for ( ; pb < pbEnd; ++pb) {
		if (*pb) {
			foundNonZero = true;
			break;
		}
	}
	if (foundNonZero) {
		ValidationFailures += L"The profile's reserved bytes following the profile ID are not zero as specified by the ICC.\r\n";
		StringCbPrintf(buf, sizeof(buf), L"The value 0x%02x was found in the reserved area.\r\n\r\n", *pb );
		ValidationFailures += buf;
	}

	// Read the tag count
	//
	DWORD testTagCount = 0;
	bRet = ReadFile(hFile, &testTagCount, sizeof(testTagCount), &cb, NULL);
	if ( (0 == bRet) || (cb != sizeof(testTagCount) ) ) {
		wstring message = L"Cannot read tag count from profile file \"";
		message += filepath;
		message += L"\".\r\n\r\n";
		ErrorString += ShowError(L"ReadFile", 0, message.c_str());
		CloseHandle(hFile);
		ErrorString += ValidationFailures;
		return ErrorString;
	}

	// Do a little sanity checking on the proposed tag count.  Be generous.
	//
	testTagCount = swap32(testTagCount);
	if (testTagCount > 1024) {
		wstring message = L"The tag count in profile file \"";
		message += filepath;
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"\" is unreasonably large (%u).\r\nThis is probably not a valid ICC profile.\r\n\r\n",
				testTagCount );
		message += buf;
		ErrorString = message;
		CloseHandle(hFile);
		ErrorString += ValidationFailures;
		return ErrorString;
	}
	DWORD smallestPossibleSize = (testTagCount * sizeof(TAG_TABLE_ENTRY)) + sizeof(PROFILEHEADER) + sizeof(TagCount) + sizeof(DWORD);
	if ( smallestPossibleSize > ProfileSize.LowPart ) {
		wstring message = L"File \"";
		message += filepath;
		message += L"\" is not a valid ICC profile.\r\nThe tag count (";
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"%u) specifies a minimum file size (%u) that exceeds the actual file size (%u).\r\n\r\n",
				testTagCount,
				smallestPossibleSize,
				ProfileSize.LowPart );
		message += buf;
		ErrorString = message;
		CloseHandle(hFile);
		ErrorString += ValidationFailures;
		return ErrorString;
	}

	// The tag count seems reasonable, so read the tag table (directory)
	//
	TagCount = testTagCount;
	TagTable = new TAG_TABLE_ENTRY[TagCount];
	DWORD tagTableByteCount = TagCount * sizeof(TAG_TABLE_ENTRY);
	SecureZeroMemory(TagTable, tagTableByteCount);
	bRet = ReadFile(hFile, TagTable, tagTableByteCount, &cb, NULL);
	if ( (0 == bRet) || (cb != tagTableByteCount ) ) {
		wstring message = L"Cannot read tag table (directory) from profile file \"";
		message += filepath;
		message += L"\".\r\n\r\n";
		ErrorString += ShowError(L"ReadFile", 0, message.c_str());
		CloseHandle(hFile);
		ErrorString += ValidationFailures;
		return ErrorString;
	}

	// Store our version of the tag table in little-endian format
	//
	for (size_t i = 0; i < TagCount; ++i) {
		TagTable[i].Signature = swap32(TagTable[i].Signature);
		TagTable[i].Offset = swap32(TagTable[i].Offset);
		TagTable[i].Size = swap32(TagTable[i].Size);
	}

	// Generate a sort order for the tags
	//
	sortedTags = new DWORD *[TagCount];
	for (size_t i = 0; i < TagCount; ++i) {
		sortedTags[i] = &TagTable[i].Signature;
	}
	qsort(sortedTags, TagCount, sizeof(DWORD *), &ComparePtrToDWORD);

	// Sanity test every tag table entry
	//
	for (size_t i = 0; i < TagCount; ++i) {
		unsigned __int64 testSize = TagTable[i].Offset + TagTable[i].Size;
		if ( testSize > static_cast<unsigned __int64>(ProfileSize.QuadPart) ) {
			wstring message = L"File \"";
			message += filepath;
			message += L"\" is not a valid ICC profile.\r\nThe tag '";
			ConvertFourBytesForDisplay(swap32(TagTable[i].Signature), displayChars, sizeof(displayChars));
			StringCbPrintf(
					buf,
					sizeof(buf),
					L"%s' specifies an offset (%u) and size (%u) that exceed the actual file size (%u).\r\n\r\n",
					displayChars,
					TagTable[i].Offset,
					TagTable[i].Size,
					ProfileSize.LowPart );
			message += buf;
			ErrorString = message;
			CloseHandle(hFile);
			ErrorString += ValidationFailures;
			return ErrorString;
		}
		if ('vcgt' == TagTable[i].Signature) {
			vcgtIndex = static_cast<int>(i);
		}
		if ('MS00' == TagTable[i].Signature) {
			wcsProfileIndex = static_cast<int>(i);
		}
	}

	// If there is a 'vcgt' tag, read it
	//
	LARGE_INTEGER moveTo;
	if (-1 != vcgtIndex) {
		moveTo.LowPart = TagTable[vcgtIndex].Offset;
		moveTo.HighPart = 0;
		bRet = SetFilePointerEx(hFile, moveTo, NULL, FILE_BEGIN);
		if ( 0 == bRet ) {
			wstring message = L"Cannot read 'vcgt' tag from profile file \"";
			message += filepath;
			message += L"\".\r\n\r\n";
			ErrorString += ShowError(L"SetFilePointerEx", 0, message.c_str());
			CloseHandle(hFile);
			ErrorString += ValidationFailures;
			return ErrorString;
		}
		pVCGT = reinterpret_cast<VCGT_HEADER *>(new BYTE[TagTable[vcgtIndex].Size]);
		bRet = ReadFile(hFile, pVCGT, TagTable[vcgtIndex].Size, &cb, NULL);
		if ( 0 == bRet ) {
			wstring message = L"Cannot read 'vcgt' tag from profile file \"";
			message += filepath;
			message += L"\".\r\n\r\n";
			ErrorString += ShowError(L"ReadFile", 0, message.c_str());
			CloseHandle(hFile);
			ErrorString += ValidationFailures;
			return ErrorString;
		}
		vcgtHeader.vcgtSignature = swap32(pVCGT->vcgtSignature);
		vcgtHeader.vcgtReserved = swap32(pVCGT->vcgtReserved);
		vcgtHeader.vcgtType = static_cast<VCGT_TYPE>(swap32(pVCGT->vcgtType));
		if (VCGT_TYPE_TABLE == vcgtHeader.vcgtType) {
			vcgtHeader.vcgtContents.t.vcgtChannels = swap16(pVCGT->vcgtContents.t.vcgtChannels);
			vcgtHeader.vcgtContents.t.vcgtCount = swap16(pVCGT->vcgtContents.t.vcgtCount);
			vcgtHeader.vcgtContents.t.vcgtItemSize = swap16(pVCGT->vcgtContents.t.vcgtItemSize);

			// Sanity test the vcgt table
			//
			if (3 != vcgtHeader.vcgtContents.t.vcgtChannels) {
				wstring message = L"File \"";
				message += filepath;
				message += L"\" is not a valid ICC profile.\r\nThe 'vcgt' table header indicates a color table using ";
				StringCbPrintf(
						buf,
						sizeof(buf),
						L"%u channels.  Color displays use 3 channels.\r\n\r\n",
						vcgtHeader.vcgtContents.t.vcgtChannels );
				message += buf;
				ErrorString = message;
				CloseHandle(hFile);
				ErrorString += ValidationFailures;
				return ErrorString;
			}
			if (256 != vcgtHeader.vcgtContents.t.vcgtCount) {
				wstring message = L"File \"";
				message += filepath;
				message += L"\" is not a valid ICC profile.\r\nThe 'vcgt' table header indicates a color table using ";
				StringCbPrintf(
						buf,
						sizeof(buf),
						L"%u entries per channel.  The Windows API requires 256 entries per channel.\r\n\r\n",
						vcgtHeader.vcgtContents.t.vcgtCount );
				message += buf;
				ErrorString = message;
				CloseHandle(hFile);
				ErrorString += ValidationFailures;
				return ErrorString;
			}
			if ( (2 != vcgtHeader.vcgtContents.t.vcgtItemSize) && (1 != vcgtHeader.vcgtContents.t.vcgtItemSize) ) {
				wstring message = L"File \"";
				message += filepath;
				message += L"\" is not a valid ICC profile.\r\nThe 'vcgt' table header indicates a color table using ";
				StringCbPrintf(
						buf,
						sizeof(buf),
						L"%u bytes per entry.\r\nValid color tables must have either 1 or 2 bytes per entry.\r\n\r\n",
						vcgtHeader.vcgtContents.t.vcgtItemSize );
				message += buf;
				ErrorString = message;
				CloseHandle(hFile);
				ErrorString += ValidationFailures;
				return ErrorString;
			}
			DWORD testSize = vcgtHeader.vcgtContents.t.vcgtChannels *
							vcgtHeader.vcgtContents.t.vcgtCount *
							vcgtHeader.vcgtContents.t.vcgtItemSize + 12;
			if ( testSize > TagTable[vcgtIndex].Size ) {
				wstring message = L"File \"";
				message += filepath;
				message += L"\" is not a valid ICC profile.\r\nThe 'vcgt' data indicates a color table size (";
				StringCbPrintf(
						buf,
						sizeof(buf),
						L"%u) that exceeds the size of the 'vcgt' tag (%u).\r\n\r\n",
						testSize,
						TagTable[vcgtIndex].Size );
				message += buf;
				ErrorString = message;
				CloseHandle(hFile);
				ErrorString += ValidationFailures;
				return ErrorString;
			}

			// Ok, this is sort of questionable.  But ...
			//
			// Adobe Gamma ("Adobe Gamma.cpl") version 3.3.0.0 (at least, perhaps other versions) writes profiles
			// that contain badly formed 'vcgt' tags.  What they do is create a normal 2 byte-per-entry table
			// (using only the high byte, but that's OK), and then label it as a 1 byte-per-entry table.  Presumably,
			// "Adobe Gamma Loader.exe" (version 1.0.0.1) is "smart" enough to deal with this, but what about the
			// rest of us?
			//
			// So ... I'll try to detect this (bad) situation and "do the right thing".  If the 'vcgt' seems to be
			// mislabeled (double its suggested size, with every other entry zero), then I'll load it as the 2 byte-
			// per-entry table it really is.  Sigh ...
			//

			// Start out assuming that the header is correct ... tables that say they are
			// 1 byte-per-entry really are.
			//
			bool treatAsTwoByteTable = (2 == vcgtHeader.vcgtContents.t.vcgtItemSize);

			if ( !treatAsTwoByteTable ) {

				// Detect bad Adobe Gamma-style 'vcgt' table.  Is the size on disk twice what it needs to be?
				//
				DWORD testSize2 = vcgtHeader.vcgtContents.t.vcgtChannels * vcgtHeader.vcgtContents.t.vcgtCount * 2 + 12;
				if ( testSize2 <= TagTable[vcgtIndex].Size ) {

					// Yes, there is room in the 'vcgt' for a 2 byte-per-entry table.  Is every other entry zero?
					//
					foundNonZero = false;
					LUT * testLUT = reinterpret_cast<LUT *>(&pVCGT->vcgtContents.t.vcgtData[0]);
					for (size_t i = 0; i < 256; ++i) {
						if ( 0 != (testLUT->red[i] & 0xFF00) ) {
							foundNonZero = true;
							break;
						}
						if ( 0 != (testLUT->green[i] & 0xFF00) ) {
							foundNonZero = true;
							break;
						}
						if ( 0 != (testLUT->blue[i] & 0xFF00) ) {
							foundNonZero = true;
							break;
						}
					}
					if ( false == foundNonZero ) {

						// We have a winner!  This is really a 2 byte table mislabeled as a 1 byte table.
						//
						treatAsTwoByteTable = true;
						ValidationFailures += L"The 'vcgt' tag in this profile is badly formed.  The 'vcgt' table header\r\n";
						ValidationFailures += L"indicates that the color table uses 1 byte per entry, but in fact the table\r\n";
						ValidationFailures += L"is a 2 byte per entry table.  The table was loaded accounting for this error\r\n";
						ValidationFailures += L"and should behave correctly as loaded by this program.  This profile may cause\r\n";
						ValidationFailures += L"problems if its color table is loaded by other LUT loaders.\r\n\r\n";
					}
				}
			}

			// Create a byte-swapped copy of the profile's LUT
			//
			pLUT = new LUT;
			SecureZeroMemory(pLUT, sizeof(LUT));
			if (treatAsTwoByteTable) {

				// This is the normal case, matching LUT formats except for byte order
				//
				LUT * profileLUT = reinterpret_cast<LUT *>(&pVCGT->vcgtContents.t.vcgtData[0]);
				for (size_t i = 0; i < 256; ++i) {
					pLUT->red[i] = swap16(profileLUT->red[i]);
					pLUT->green[i] = swap16(profileLUT->green[i]);
					pLUT->blue[i] = swap16(profileLUT->blue[i]);
				}
			} else {

				// This is the odd case, where we create a 2-byte LUT entry from a 1-byte entry.  We could either
				//  just stuff the byte into the high-order byte (e.g. 0x56 => 0x5600) or we could distribute
				//  the values better by putting the byte into both halves (e.g. 0x56 => 0x5656).
				//
				LUT_1_BYTE * profileLUT = reinterpret_cast<LUT_1_BYTE *>(&pVCGT->vcgtContents.t.vcgtData[0]);
				for (size_t i = 0; i < 256; ++i) {

#define USE_BOTH_HALVES 1
#if USE_BOTH_HALVES
					pLUT->red[i] = (static_cast<WORD>(profileLUT->red[i]) << 8) + profileLUT->red[i];
					pLUT->green[i] = (static_cast<WORD>(profileLUT->green[i]) << 8) + profileLUT->green[i];
					pLUT->blue[i] = (static_cast<WORD>(profileLUT->blue[i]) << 8) + profileLUT->blue[i];
#else
					pLUT->red[i] = (static_cast<WORD>(profileLUT->red[i]) << 8);
					pLUT->green[i] = (static_cast<WORD>(profileLUT->green[i]) << 8);
					pLUT->blue[i] = (static_cast<WORD>(profileLUT->blue[i]) << 8);
#endif

				}
			}
		} else {

			// We have a formula type of 'vcgt', but we work mainly with tables, so store whatever
			// is there, then see if we can generate a table from it
			//
			vcgtHeader.vcgtContents.f.vcgtRedGamma = swap32(pVCGT->vcgtContents.f.vcgtRedGamma);
			vcgtHeader.vcgtContents.f.vcgtRedMin = swap32(pVCGT->vcgtContents.f.vcgtRedMin);
			vcgtHeader.vcgtContents.f.vcgtRedMax = swap32(pVCGT->vcgtContents.f.vcgtRedMax);

			vcgtHeader.vcgtContents.f.vcgtGreenGamma = swap32(pVCGT->vcgtContents.f.vcgtGreenGamma);
			vcgtHeader.vcgtContents.f.vcgtGreenMin = swap32(pVCGT->vcgtContents.f.vcgtGreenMin);
			vcgtHeader.vcgtContents.f.vcgtGreenMax = swap32(pVCGT->vcgtContents.f.vcgtGreenMax);

			vcgtHeader.vcgtContents.f.vcgtBlueGamma = swap32(pVCGT->vcgtContents.f.vcgtBlueGamma);
			vcgtHeader.vcgtContents.f.vcgtBlueMin = swap32(pVCGT->vcgtContents.f.vcgtBlueMin);
			vcgtHeader.vcgtContents.f.vcgtBlueMax = swap32(pVCGT->vcgtContents.f.vcgtBlueMax);

			// If the values look good, generate a LUT from the gamma formula
			//
#define MIN_GAMMA	0.2			// Arbitrary tests for validity ... gamma must be positive, but we'll hold
#define MAX_GAMMA	5.0			//  it to 0.2 <= gamma <= 5.0 .  The 'minimum' value should be less than the 
#define MAX_START	0.5			//  'maximum' value, and both must be between 0.0 and 1.0.  We further limit
#define MIN_FINISH	0.5			//  the range to 0.0 <= min <= 0.5 and 0.5 <= max <= 1.0 .

#define SYSTEM_GAMMA 2.2		// Some kind of fudge factor that I don't understand ...

			double redGamma = double(vcgtHeader.vcgtContents.f.vcgtRedGamma) / double(65536);
			double redMin = double(vcgtHeader.vcgtContents.f.vcgtRedMin) / double(65536);
			double redMax = double(vcgtHeader.vcgtContents.f.vcgtRedMax) / double(65536);

			double greenGamma = double(vcgtHeader.vcgtContents.f.vcgtGreenGamma) / double(65536);
			double greenMin = double(vcgtHeader.vcgtContents.f.vcgtGreenMin) / double(65536);
			double greenMax = double(vcgtHeader.vcgtContents.f.vcgtGreenMax) / double(65536);

			double blueGamma = double(vcgtHeader.vcgtContents.f.vcgtBlueGamma) / double(65536);
			double blueMin = double(vcgtHeader.vcgtContents.f.vcgtBlueMin) / double(65536);
			double blueMax = double(vcgtHeader.vcgtContents.f.vcgtBlueMax) / double(65536);

			if (	redGamma >= MIN_GAMMA	&& redGamma <= MAX_GAMMA	&&
					redMin >= 0.0			&& redMin <= MAX_START		&&
					redMax <= 1.0			&& redMax >= MIN_FINISH		&&
					redMin < redMax										&&

					greenGamma >= MIN_GAMMA	&& greenGamma <= MAX_GAMMA	&&
					greenMin >= 0.0			&& greenMin <= MAX_START	&&
					greenMax <= 1.0			&& greenMax >= MIN_FINISH	&&
					greenMin < greenMax									&&

					blueGamma >= MIN_GAMMA	&& blueGamma <= MAX_GAMMA	&&
					blueMin >= 0.0			&& blueMin <= MAX_START		&&
					blueMax <= 1.0			&& blueMax >= MIN_FINISH	&&
					blueMin < blueMax
			) {
				pLUT = new LUT;
				SecureZeroMemory(pLUT, sizeof(LUT));
				double redScale = redMax - redMin;
				double greenScale = greenMax - greenMin;
				double blueScale = blueMax - blueMin;
				redGamma *= SYSTEM_GAMMA;
				greenGamma *= SYSTEM_GAMMA;
				blueGamma *= SYSTEM_GAMMA;
				int result;
				for ( size_t i = 0; i < 256; ++i ) {
					result = static_cast<int>( double(65536) * (redMin + redScale * pow( (double(i)/double(255)), redGamma)) );
					if ( result < 0x0000 ) {
						pLUT->red[i] = 0x0000;
					} else if ( result > 0xFFFF ) {
						pLUT->red[i] = 0xFFFF;
					} else {
						pLUT->red[i] = static_cast<WORD>(result);
					}

					result = static_cast<int>( double(65536) * (greenMin + greenScale * pow( (double(i)/double(255)), greenGamma)) );
					if ( result < 0x0000 ) {
						pLUT->green[i] = 0;
					} else if ( result > 0xFFFF ) {
						pLUT->green[i] = 0xFFFF;
					} else {
						pLUT->green[i] = static_cast<WORD>(result);
					}

					result = static_cast<int>( double(65536) * (blueMin + blueScale * pow( (double(i)/double(255)), blueGamma)) );
					if ( result < 0x0000 ) {
						pLUT->blue[i] = 0;
					} else if ( result > 0xFFFF ) {
						pLUT->blue[i] = 0xFFFF;
					} else {
						pLUT->blue[i] = static_cast<WORD>(result);
					}
				}
			}
		}
	}

	if (-1 != wcsProfileIndex) {
		moveTo.LowPart = TagTable[wcsProfileIndex].Offset;
		moveTo.HighPart = 0;
		bRet = SetFilePointerEx(hFile, moveTo, NULL, FILE_BEGIN);
		if ( 0 == bRet ) {
			wstring message = L"Cannot read 'MS00' tag from profile file \"";
			message += filepath;
			message += L"\".\r\n\r\n";
			ErrorString += ShowError(L"SetFilePointerEx", 0, message.c_str());
			CloseHandle(hFile);
			ErrorString += ValidationFailures;
			return ErrorString;
		}
		BYTE * bigBuffer = new BYTE[TagTable[wcsProfileIndex].Size];
		bRet = ReadFile(hFile, bigBuffer, TagTable[wcsProfileIndex].Size, &cb, NULL);
		if (bRet) {
			WCS_IN_ICC_HEADER * wiPtr = reinterpret_cast<WCS_IN_ICC_HEADER *>(bigBuffer);
			DWORD siz;

			// A WCS profile embedded in an ICC profile is a set of three little-endian Unicode (UCS2)
			// XML "files" strung together, with a header that says how to find individual sections.
			// The three sections are:
			//   1) Color Device Model;
			//   2) Color Appearance Model;
			//   3) Gamut Map Model
			//
			wchar_t * cString;

			siz = swap32(wiPtr->wcshdrCDMsize) / 2;
			cString = new wchar_t[siz + 1];
			memcpy_s(cString, 2 * siz + sizeof(wchar_t), &bigBuffer[swap32(wiPtr->wcshdrCDMoffset)], 2 * siz + sizeof(wchar_t));
			cString[siz] = 0;
			WCS_ColorDeviceModel = cString;
			delete [] cString;

			siz = swap32(wiPtr->wcshdrCAMsize) / 2;
			cString = new wchar_t[siz + 1];
			memcpy_s(cString, 2 * siz + sizeof(wchar_t), &bigBuffer[swap32(wiPtr->wcshdrCAMoffset)], 2 * siz + sizeof(wchar_t));
			cString[siz] = 0;
			WCS_ColorAppearanceModel = cString;
			delete [] cString;

			siz = swap32(wiPtr->wcshdrGMMsize) / 2;
			cString = new wchar_t[siz + 1];
			memcpy_s(cString, 2 * siz + sizeof(wchar_t), &bigBuffer[swap32(wiPtr->wcshdrGMMoffset)], 2 * siz + sizeof(wchar_t));
			cString[siz] = 0;
			WCS_GamutMapModel = cString;
			delete [] cString;

		} else {
			s += ShowError(L"ReadFile");
		}
		delete [] bigBuffer;
	}
	CloseHandle(hFile);
	ErrorString = s;
	return s;
}

// Return a string for the per-monitor panel
//
wstring Profile::DetailsString(void) {

	wstring s;
	wchar_t displayChars[5];

	// Quit early if no profile
	//
	if (ProfileName.empty()) {
		s = L"No profile\r\n";
		return s;
	}

	// If we already failed to load the profile, report the error we had.
	// If we haven't yet read the profile, try to do it now.
	// If it fails, report the error.
	//
	if ( !ErrorString.empty() ) {
		return ErrorString;
	}
	if ( 0 == ProfileSize.QuadPart ) {
		LoadFullProfile(false);
		if ( !ErrorString.empty() ) {
			return ErrorString;
		}
	}

	s.reserve(10240);					// Reduce repeated reallocation
	wchar_t buf[1024];
	BYTE * pb = 0;
	const wchar_t * lookupString = 0;

	// Get color directory
	//
	wchar_t filepath[1024];
	if (ColorDirectory) {
		StringCbCopy(filepath, sizeof(filepath), ColorDirectory);
	} else {
		ErrorString = ColorDirectoryErrorString;
		return ErrorString;
	}
	StringCbCat(filepath, sizeof(filepath), L"\\");
	StringCbCat(filepath, sizeof(filepath), ProfileName.c_str());
	s += filepath;
	StringCbPrintf(buf, sizeof(buf), L"\r\nFile size is %I64u bytes\r\n\r\n", ProfileSize);
	s += buf;

	// Display validation failures, if any
	//
	s += ValidationFailures;

	// Display the profile header fields
	//
	StringCbPrintf(buf, sizeof(buf), L"Profile header fields:\r\n  Size:  %d bytes\r\n", swap32(ProfileHeader->phSize));
	s += buf;
	s += L"  Preferred CMM:  ";
	if ( ConvertFourBytesForDisplay(ProfileHeader->phCMMType, displayChars, sizeof(displayChars)) ) {
		s += L"No preferred CMM\r\n";
	} else {
		s += displayChars;
		lookupString = LookupName( knownCMMs, _countof(knownCMMs), swap32(ProfileHeader->phCMMType) );
		if ( *lookupString ) {
			StringCbPrintf(buf, sizeof(buf), L" (%s)\r\n", lookupString);
			s += buf;
		} else {
			s += L"\r\n";
		}
	}
	s += L"  Version:  ";
	if ( 0x00002004 == ProfileHeader->phVersion ) {		// Latest ICC documentation likes "4.2.0.0" instead of "4.2.0"
		s += L"4.2.0.0\r\n";
	} else {
		pb = reinterpret_cast<BYTE *>(&ProfileHeader->phVersion);
		StringCbPrintf(buf, sizeof(buf), L"%d.%d.%d\r\n", pb[0], (pb[1]>>4), (pb[1]&0x0F));
		s += buf;
	}
	ConvertFourBytesForDisplay(ProfileHeader->phClass, displayChars, sizeof(displayChars));
	StringCbPrintf(buf, sizeof(buf), L"  Profile/Device class:  %s", displayChars);
	s += buf;
	lookupString = LookupName( profileClasses, _countof(profileClasses), swap32(ProfileHeader->phClass) );
	if ( *lookupString ) {
		StringCbPrintf(buf, sizeof(buf), L" (%s)\r\n", lookupString);
		s += buf;
	} else {
		s += L"\r\n";
	}
	ConvertFourBytesForDisplay(ProfileHeader->phDataColorSpace, displayChars, sizeof(displayChars));
	StringCbPrintf(buf, sizeof(buf), L"  Color space of data:  %s", displayChars);
	s += buf;
	lookupString = LookupName( colorSpaces, _countof(colorSpaces), swap32(ProfileHeader->phDataColorSpace) );
	if ( *lookupString ) {
		StringCbPrintf(buf, sizeof(buf), L" (%s)\r\n", lookupString);
		s += buf;
	} else {
		s += L"\r\n";
	}
	ConvertFourBytesForDisplay(ProfileHeader->phConnectionSpace, displayChars, sizeof(displayChars));
	StringCbPrintf(buf, sizeof(buf), L"  Profile connection space:  %s", displayChars);
	s += buf;
	lookupString = LookupName( colorSpaces, _countof(colorSpaces), swap32(ProfileHeader->phConnectionSpace) );
	if ( *lookupString ) {
		StringCbPrintf(buf, sizeof(buf), L" (%s)\r\n", lookupString);
		s += buf;
	} else {
		s += L"\r\n";
	}
	dateTimeNumber * dt = reinterpret_cast<dateTimeNumber *>(&ProfileHeader->phDateTime[0]);
	StringCchPrintf(
		buf,
		_countof(buf),
		L"  Date and time this profile was first created:  %04d-%02d-%02d %02d:%02d:%02dZ\r\n", // RFC3389, NOTE: in 5.6
		swap16(dt->year),
		swap16(dt->month),
		swap16(dt->day),
		swap16(dt->hour),
		swap16(dt->minute),
		swap16(dt->second) );
	s += buf;
	ConvertFourBytesForDisplay(ProfileHeader->phSignature, displayChars, sizeof(displayChars));
	StringCbPrintf(buf, sizeof(buf), L"  Signature (must be 'acsp'):  %s\r\n", displayChars);
	s += buf;
	s += L"  Primary Platform signature:  ";
	if ( ConvertFourBytesForDisplay(ProfileHeader->phPlatform, displayChars, sizeof(displayChars)) ) {
		s += L"No primary platform\r\n";
	} else {
		s += displayChars;
		lookupString = LookupName( knownPlatforms, _countof(knownPlatforms), swap32(ProfileHeader->phPlatform) );
		if ( *lookupString ) {
			StringCbPrintf(buf, sizeof(buf), L" (%s)\r\n", lookupString);
			s += buf;
		} else {
			s += L"\r\n";
		}
	}
	DWORD flags = swap32(ProfileHeader->phProfileFlags);
	s += L"  Profile flags:\r\n    Embedded Profile:  ";
	s += (0 != (flags & 0x00000001)) ? L"True" : L"False";
	s += L"\r\n    Use only with embedded color data:  ";
	s += (0 != (flags & 0x00000002)) ? L"True" : L"False";
	s += L"\r\n";
	if ( 0 != (flags & ~0xFFFF0003) ) {
		StringCbPrintf(buf, sizeof(buf), L"  => Undefined flags set in low-order 16 bits:  0x%08x\r\n", flags);
		s += buf;
	}
	s += L"  Device manufacturer:  ";
	ConvertFourBytesForDisplay(ProfileHeader->phManufacturer, displayChars, sizeof(displayChars));
	s += displayChars;
	s += L"\r\n  Device model:  ";
	ConvertFourBytesForDisplay(ProfileHeader->phModel, displayChars, sizeof(displayChars));
	s += displayChars;
	flags = swap32(ProfileHeader->phAttributes[1]);
	s += L"\r\n  Device attributes:\r\n    Reflective or transparency:  ";
	s += (0 != (flags & 0x00000001)) ? L"Transparency" : L"Reflective";
	s += L"\r\n    Glossy or matte:  ";
	s += (0 != (flags & 0x00000002)) ? L"Matte" : L"Glossy";
	s += L"\r\n    Media polarity:  ";
	s += (0 != (flags & 0x00000004)) ? L"Negative" : L"Positive";
	s += L"\r\n    Color or B&W media:  ";
	s += (0 != (flags & 0x00000008)) ? L"Black & white" : L"Color";
	s += L"\r\n";
	if ( 0 != (flags & ~0x0000000F) ) {
		StringCbPrintf(
				buf,
				sizeof(buf),
				L"  => Undefined flags set:  0x%08x\r\n",
				flags );
		s += buf;
	}
	s += L"  Rendering Intent:  ";
	lookupString = LookupName( renderingIntents, _countof(renderingIntents), swap32(ProfileHeader->phRenderingIntent) );
	if ( *lookupString ) {
		StringCbPrintf(buf, sizeof(buf), L"%s\r\n", lookupString);
		s += buf;
	} else {
		StringCbPrintf(buf, sizeof(buf), L"Unknown value: 0x%08x\r\n", swap32(ProfileHeader->phRenderingIntent));
		s += buf;
	}
	StringCbPrintf(
			buf,
			sizeof(buf),
			L"  Illuminant:  X=%6.4f, Y=%6.4f, Z=%6.4f\r\n",
			double(swap32(ProfileHeader->phIlluminant.ciexyzX)) / double(65536),
			double(swap32(ProfileHeader->phIlluminant.ciexyzY)) / double(65536),
			double(swap32(ProfileHeader->phIlluminant.ciexyzZ)) / double(65536) );
	s += buf;
	s += L"  Profile Creator:  ";
	ConvertFourBytesForDisplay(ProfileHeader->phCreator, displayChars, sizeof(displayChars));
	s += displayChars;
	DWORD * pProfileID = reinterpret_cast<DWORD *>(&ProfileHeader->phCreator);
	StringCbPrintf(
			buf,
			sizeof(buf),
			L"\r\n  Profile ID:  %08x %08x %08x %08x\r\n",
			swap32(pProfileID[1]),
			swap32(pProfileID[2]),
			swap32(pProfileID[3]),
			swap32(pProfileID[4]) );
	s += buf;

#if 0
	StringCbPrintf(buf, sizeof(buf), L"\r\nProfile header (%d bytes):\r\n", sizeof(PROFILEHEADER));
	s += buf;
	s += HexDump(reinterpret_cast<unsigned char *>(ProfileHeader), sizeof(PROFILEHEADER), 16);
#endif

	StringCbPrintf(buf, sizeof(buf), L"\r\nProfile contains %d tags:\r\n", TagCount);
	s += buf;
	for (size_t i = 0; i < TagCount; ++i) {
		DWORD tagSignature = *sortedTags[i];
		ConvertFourBytesForDisplay(swap32(tagSignature), displayChars, sizeof(displayChars));
		lookupString = LookupName( knownTags, _countof(knownTags), tagSignature );
		StringCbPrintf(buf, sizeof(buf), L"  %s:  %s\r\n", displayChars, lookupString);
		s += buf;
	}

	if (-1 == vcgtIndex) {
		s += L"\r\nProfile does not include a Video Card Gamma Tag\r\n";
	} else {
		s += L"\r\nProfile includes a Video Card Gamma Tag:\r\n  Type: ";
		if (VCGT_TYPE_TABLE == vcgtHeader.vcgtType) {
			s += L"Table";
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Channels: %d", vcgtHeader.vcgtContents.t.vcgtChannels);
			s += buf;
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Count: %d entries per channel", vcgtHeader.vcgtContents.t.vcgtCount);
			s += buf;
			wchar_t * str;
			if ( 1 == vcgtHeader.vcgtContents.t.vcgtItemSize ) {
				str = L"\r\n  Size: %d byte per entry";
			} else {
				str = L"\r\n  Size: %d bytes per entry";
			}
			StringCbPrintf(buf, sizeof(buf), str, vcgtHeader.vcgtContents.t.vcgtItemSize);
			s += buf;
		} else {
			s += L"Formula";

			StringCbPrintf(buf, sizeof(buf), L"\r\n  Red gamma: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtRedGamma) / double(65536));
			s += buf;
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Red min: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtRedMin) / double(65536));
			s += buf;
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Red max: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtRedMax) / double(65536));
			s += buf;

			StringCbPrintf(buf, sizeof(buf), L"\r\n  Green gamma: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtGreenGamma) / double(65536));
			s += buf;
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Green min: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtGreenMin) / double(65536));
			s += buf;
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Green max: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtGreenMax) / double(65536));
			s += buf;

			StringCbPrintf(buf, sizeof(buf), L"\r\n  Blue gamma: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtBlueGamma) / double(65536));
			s += buf;
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Blue min: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtBlueMin) / double(65536));
			s += buf;
			StringCbPrintf(buf, sizeof(buf), L"\r\n  Blue max: %6.4f", double(vcgtHeader.vcgtContents.f.vcgtBlueMax) / double(65536));
			s += buf;
		}
#if 0
		StringCbPrintf(buf, sizeof(buf), L"\r\n\r\nDump of Video Card Gamma Tag (%d bytes):\r\n", TagTable[vcgtIndex].Size);
		s += buf;
		s += HexDump(reinterpret_cast<unsigned char *>(pVCGT), TagTable[vcgtIndex].Size, 16);
#endif
	}

	if (-1 != wcsProfileIndex) {
		s += L"\r\nProfile includes an embedded WCS profile ('MS00'):\r\n";

		// A WCS profile embedded in an ICC profile is a set of three little-endian Unicode (UCS2)
		// XML "files" strung together, with a header that says how to find individual sections.
		// The three sections are:
		//   1) Color Device Model;
		//   2) Color Appearance Model;
		//   3) Gamut Map Model
		//
		if (!WCS_ColorDeviceModel.empty()) {
			s += L"\r\n  Color Device Model:\r\n";
			s += WCS_ColorDeviceModel;
		}
		if (!WCS_ColorAppearanceModel.empty()) {
			s += L"\r\n  Color Appearance Model:\r\n";
			s += WCS_ColorAppearanceModel;
		}
		if (!WCS_GamutMapModel.empty()) {
			s += L"\r\n  Gamut Map Model:\r\n";
			s += WCS_GamutMapModel;
		}
	}
	return s;
}

// Compare the LUT for this profile with another LUT, and return the result
//
LUT_COMPARISON Profile::CompareLUT(LUT * otherLUT, DWORD * maxError, DWORD * totalError) {

	if (!otherLUT) {
		return LC_ERROR_NO_LUT_PROVIDED;
	}

	bool thisLutIsLinear = true;
	bool otherLutIsLinear = true;
	bool lutsMatchExactly = true;
	bool roundingIsPossible = true;
	bool truncationIsPossible = true;
	DWORD max = 0;
	DWORD total = 0;
	DWORD diff;
	DWORD diff8;
	DWORD diff16;
	size_t i;

	// If this profile has no LUT, we still need to check the other one
	// for linearity
	//
	if ( 0 == pLUT ) {
		i = 0;
		// Special-case our "signature" ... consider it linear
		//
		for (; i < 4; ++i) {
			WORD linear8 = static_cast<WORD>(i << 8);			// Two versions of linearity
			WORD linear16 = static_cast<WORD>(linear8 + i);
			if ( (otherLUT->red[i] != linear8) && (otherLUT->red[i] != linear16) ) {
				if ((1 == i && 0x0102 == otherLUT->red[i])	||
					(2 == i && 0x0203 == otherLUT->red[i])	||
					(3 == i && 0x0304 == otherLUT->red[i])
				) {
					;
				} else {
					otherLutIsLinear = false;
					diff8 = (otherLUT->red[i] > linear8) ? (otherLUT->red[i] - linear8) : (linear8 - otherLUT->red[i]);
					diff16 = (otherLUT->red[i] > linear16) ? (otherLUT->red[i] - linear16) : (linear16 - otherLUT->red[i]);
					diff = (diff8 < diff16) ? diff8 : diff16;
					max = (diff > max) ? diff : max;
					total += diff;
				}
			}
			if ( (otherLUT->green[i] != linear8) && (otherLUT->green[i] != linear16) ) {
				otherLutIsLinear = false;
				diff8 = (otherLUT->green[i] > linear8) ? (otherLUT->green[i] - linear8) : (linear8 - otherLUT->green[i]);
				diff16 = (otherLUT->green[i] > linear16) ? (otherLUT->green[i] - linear16) : (linear16 - otherLUT->green[i]);
				diff = (diff8 < diff16) ? diff8 : diff16;
				max = (diff > max) ? diff : max;
				total += diff;
			}
			if ( (otherLUT->blue[i] != linear8) && (otherLUT->blue[i] != linear16) ) {
				otherLutIsLinear = false;
				diff8 = (otherLUT->blue[i] > linear8) ? (otherLUT->blue[i] - linear8) : (linear8 - otherLUT->blue[i]);
				diff16 = (otherLUT->blue[i] > linear16) ? (otherLUT->blue[i] - linear16) : (linear16 - otherLUT->blue[i]);
				diff = (diff8 < diff16) ? diff8 : diff16;
				max = (diff > max) ? diff : max;
				total += diff;
			}
		}
		for (; i < 256; ++i) {
			WORD linear8 = static_cast<WORD>(i << 8);			// Two versions of linearity
			WORD linear16 = static_cast<WORD>(linear8 + i);
			if ( (otherLUT->red[i] != linear8) && (otherLUT->red[i] != linear16) ) {
				otherLutIsLinear = false;
				diff8 = (otherLUT->red[i] > linear8) ? (otherLUT->red[i] - linear8) : (linear8 - otherLUT->red[i]);
				diff16 = (otherLUT->red[i] > linear16) ? (otherLUT->red[i] - linear16) : (linear16 - otherLUT->red[i]);
				diff = (diff8 < diff16) ? diff8 : diff16;
				max = (diff > max) ? diff : max;
				total += diff;
			}
			if ( (otherLUT->green[i] != linear8) && (otherLUT->green[i] != linear16) ) {
				otherLutIsLinear = false;
				diff8 = (otherLUT->green[i] > linear8) ? (otherLUT->green[i] - linear8) : (linear8 - otherLUT->green[i]);
				diff16 = (otherLUT->green[i] > linear16) ? (otherLUT->green[i] - linear16) : (linear16 - otherLUT->green[i]);
				diff = (diff8 < diff16) ? diff8 : diff16;
				max = (diff > max) ? diff : max;
				total += diff;
			}
			if ( (otherLUT->blue[i] != linear8) && (otherLUT->blue[i] != linear16) ) {
				otherLutIsLinear = false;
				diff8 = (otherLUT->blue[i] > linear8) ? (otherLUT->blue[i] - linear8) : (linear8 - otherLUT->blue[i]);
				diff16 = (otherLUT->blue[i] > linear16) ? (otherLUT->blue[i] - linear16) : (linear16 - otherLUT->blue[i]);
				diff = (diff8 < diff16) ? diff8 : diff16;
				max = (diff > max) ? diff : max;
				total += diff;
			}
		}
		if (maxError) {
			*maxError = max;
		}
		if (totalError) {
			*totalError = total;
		}
		return otherLutIsLinear ? LC_PROFILE_HAS_NO_LUT_OTHER_LINEAR : LC_PROFILE_HAS_NO_LUT_OTHER_NONLINEAR;
	}

	// We have two LUTs to compare, scan them in parallel
	//
	for (i = 0; i < 256; ++i) {
		WORD linear8 = static_cast<WORD>(i << 8);			// Two versions of linearity
		WORD linear16 = static_cast<WORD>(linear8 + i);

		// Check for exact equality
		//
		if (pLUT->red[i] != otherLUT->red[i]) {
			lutsMatchExactly = false;
			diff = (otherLUT->red[i] > pLUT->red[i]) ? (otherLUT->red[i] - pLUT->red[i]) : (pLUT->red[i] - otherLUT->red[i]);
			max = (diff > max) ? diff : max;
			total += diff;
		}
		if (pLUT->green[i] != otherLUT->green[i]) {
			lutsMatchExactly = false;
			diff = (otherLUT->green[i] > pLUT->green[i]) ? (otherLUT->green[i] - pLUT->green[i]) : (pLUT->green[i] - otherLUT->green[i]);
			max = (diff > max) ? diff : max;
			total += diff;
		}
		if (pLUT->blue[i] != otherLUT->blue[i]) {
			lutsMatchExactly = false;
			diff = (otherLUT->blue[i] > pLUT->blue[i]) ? (otherLUT->blue[i] - pLUT->blue[i]) : (pLUT->blue[i] - otherLUT->blue[i]);
			max = (diff > max) ? diff : max;
			total += diff;
		}

		// Check for linearity (either version), checking for our signature
		//
		if ( (pLUT->red[i] != linear8) && (pLUT->red[i] != linear16) ) {
			if ((1 == i && 0x0102 == pLUT->red[i])	||
				(2 == i && 0x0203 == pLUT->red[i])	||
				(3 == i && 0x0304 == pLUT->red[i])
			) {
				;
			} else {
				thisLutIsLinear = false;
			}
		}
		if ( (pLUT->green[i] != linear8) && (pLUT->green[i] != linear16) ) {
			thisLutIsLinear = false;
		}
		if ( (pLUT->blue[i] != linear8) && (pLUT->blue[i] != linear16) ) {
			thisLutIsLinear = false;
		}
		if ( (otherLUT->red[i] != linear8) && (otherLUT->red[i] != linear16) ) {
			if ((1 == i && 0x0102 == otherLUT->red[i])	||
				(2 == i && 0x0203 == otherLUT->red[i])	||
				(3 == i && 0x0304 == otherLUT->red[i])
			) {
				;
			} else {
				otherLutIsLinear = false;
			}
		}
		if ( (otherLUT->green[i] != linear8) && (otherLUT->green[i] != linear16) ) {
			otherLutIsLinear = false;
		}
		if ( (otherLUT->blue[i] != linear8) && (otherLUT->blue[i] != linear16) ) {
			otherLutIsLinear = false;
		}

		// See if differences can be accounted for by trucation or rounding
		//
		if ( 0 != (otherLUT->red[i] & 0xFF) ) {
			roundingIsPossible = false;
			truncationIsPossible = false;
		} else {
			if ( (otherLUT->red[i] & 0xFF00) != (pLUT->red[i] & 0xFF00) ) {
				truncationIsPossible = false;
			}
			if ( (otherLUT->red[i] & 0xFF00) != ( (pLUT->red[i] + 0x80) & 0xFF00) ) {
				roundingIsPossible = false;
			}
		}
		if ( 0 != (otherLUT->green[i] & 0xFF) ) {
			roundingIsPossible = false;
			truncationIsPossible = false;
		} else {
			if ( (otherLUT->green[i] & 0xFF00) != (pLUT->green[i] & 0xFF00) ) {
				truncationIsPossible = false;
			}
			if ( (otherLUT->green[i] & 0xFF00) != ( (pLUT->green[i] + 0x80) & 0xFF00) ) {
				roundingIsPossible = false;
			}
		}
		if ( 0 != (otherLUT->blue[i] & 0xFF) ) {
			roundingIsPossible = false;
			truncationIsPossible = false;
		} else {
			if ( (otherLUT->blue[i] & 0xFF00) != (pLUT->blue[i] & 0xFF00) ) {
				truncationIsPossible = false;
			}
			if ( (otherLUT->blue[i] & 0xFF00) != ( (pLUT->blue[i] + 0x80) & 0xFF00) ) {
				roundingIsPossible = false;
			}
		}
	}

	// Maybe return error counts
	//
	if (maxError) {
		*maxError = max;
	}
	if (totalError) {
		*totalError = total;
	}

	// We looked at every element ... what did we find?
	//
	if (lutsMatchExactly) {
		return LC_EQUAL;
	}
	if (thisLutIsLinear && otherLutIsLinear) {
		return LC_VARIATION_ON_LINEAR;
	}
	if (roundingIsPossible && truncationIsPossible) {
		return LC_TRUNCATION_OR_ROUNDING;
	}
	if (truncationIsPossible) {
		return LC_TRUNCATION_IN_LOW_BYTE;
	}
	if (roundingIsPossible) {
		return LC_ROUNDING_IN_LOW_BYTE;
	}
	return LC_UNEQUAL;
}
