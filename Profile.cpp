// Profile.cpp -- Profile class for handling display profiles.
//

#include "stdafx.h"
#include "Profile.h"
#include "Utility.h"
#include <strsafe.h>

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

// Constructor
//
Profile::Profile() :
		ProfileHeader(0),
		TagCount(0),
		TagTable(0),
		pVCGT(0),
		vcgtIndex(-1),
		wcsProfileIndex(-1)
{
	ProfileSize.QuadPart = 0;
}

// Copy constructor
//
Profile::Profile(const Profile & from) :
		ProfileName(from.ProfileName),
		ErrorString(from.ErrorString),
		ProfileSize(from.ProfileSize),
		TagCount(from.TagCount),
		vcgtIndex(from.vcgtIndex),
		wcsProfileIndex(from.wcsProfileIndex),
		WCS_ColorDeviceModel(from.WCS_ColorDeviceModel),
		WCS_ColorAppearanceModel(from.WCS_ColorAppearanceModel),
		WCS_GamutMapModel(from.WCS_GamutMapModel)
{
	if (from.ProfileHeader) {
		ProfileHeader = new PROFILEHEADER;
		memcpy_s(ProfileHeader, sizeof(PROFILEHEADER), from.ProfileHeader, sizeof(PROFILEHEADER));
	} else {
		ProfileHeader = 0;
	}
	if (from.TagTable) {
		TagTable = new TAG_TABLE_ENTRY[TagCount];
		memcpy_s(TagTable, TagCount * sizeof(TAG_TABLE_ENTRY), from.TagTable, TagCount * sizeof(TAG_TABLE_ENTRY));
	} else {
		TagTable = 0;
	}
	if (from.pVCGT && TagTable) {
		pVCGT = reinterpret_cast<VCGT_HEADER *>(new BYTE[TagTable[vcgtIndex].Size]);
		memcpy_s(pVCGT, TagTable[vcgtIndex].Size, from.pVCGT, TagTable[vcgtIndex].Size);
	} else {
		pVCGT = 0;
	}
}

// Assignment operator
//
Profile & Profile::operator = (const Profile & from) {

	ProfileName = from.ProfileName;
	ErrorString = from.ErrorString;
	ProfileSize = from.ProfileSize;
	TagCount = from.TagCount;
	vcgtIndex = from.vcgtIndex;
	wcsProfileIndex = from.wcsProfileIndex;
	WCS_ColorDeviceModel = from.WCS_ColorDeviceModel;
	WCS_ColorAppearanceModel = from.WCS_ColorAppearanceModel;
	WCS_GamutMapModel = from.WCS_GamutMapModel;
	if (ProfileHeader) {
		delete [] ProfileHeader;
	}
	if (from.ProfileHeader) {
		ProfileHeader = new PROFILEHEADER;
		memcpy_s(ProfileHeader, sizeof(PROFILEHEADER), from.ProfileHeader, sizeof(PROFILEHEADER));
	} else {
		ProfileHeader = 0;
	}
	if (TagTable) {
		delete [] TagTable;
	}
	if (from.TagTable) {
		TagTable = new TAG_TABLE_ENTRY[TagCount];
		memcpy_s(TagTable, TagCount * sizeof(TAG_TABLE_ENTRY), from.TagTable, TagCount * sizeof(TAG_TABLE_ENTRY));
	} else {
		TagTable = 0;
	}
	if (pVCGT) {
		delete [] pVCGT;
	}
	if (from.pVCGT && TagTable) {
		pVCGT = reinterpret_cast<VCGT_HEADER *>(new BYTE[TagTable[vcgtIndex].Size]);
		memcpy_s(pVCGT, TagTable[vcgtIndex].Size, from.pVCGT, TagTable[vcgtIndex].Size);
	} else {
		pVCGT = 0;
	}
	return *this;
}

// Destructor
//
Profile::~Profile() {
	if (pVCGT) {
		delete [] pVCGT;
	}
	if (TagTable) {
		delete [] TagTable;
	}
	if (ProfileHeader) {
		delete [] ProfileHeader;
	}
}

// Set profile name
//
void Profile::SetName(const wchar_t * profileName) {
	ProfileName = profileName;
}

// Get profile name
//
wstring Profile::GetName(void) const {
	return ProfileName;
}

// Load profile info from disk
//
wstring Profile::Load(void) {

#pragma warning(push)
#pragma warning(disable:6011)	// warning C6011: Dereferencing NULL pointer 'TagTable': Lines: 78, 82, 86, ..., 173, 174, 175

	wstring s;

	// Quit early if no profile
	//
	if (ProfileName.empty()) {
		return s;
	}

	wchar_t filepath[1024];
	DWORD filepathSize = sizeof(filepath);
	BOOL bRet = GetColorDirectoryW(NULL, filepath, &filepathSize);
	if (bRet) {
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
		if (INVALID_HANDLE_VALUE != hFile) {
			bRet = GetFileSizeEx(hFile, &ProfileSize);
			if (bRet) {
				ProfileHeader = new PROFILEHEADER;
				DWORD cb;
				bRet = ReadFile(hFile, ProfileHeader, sizeof(PROFILEHEADER), &cb, NULL);
				if (bRet) {
					ProfileHeader->phSize = swap32(ProfileHeader->phSize);
					ProfileHeader->phClass = swap32(ProfileHeader->phClass);
					bRet = ReadFile(hFile, &TagCount, sizeof(TagCount), &cb, NULL);
					if (bRet) {
						TagCount = swap32(TagCount);
						TagTable = new TAG_TABLE_ENTRY[TagCount];

#pragma warning(push)
#pragma warning(disable:6029)	// warning C6029: Possible buffer overrun in call to 'ReadFile': use of unchecked value 'this'

						bRet = ReadFile(hFile, TagTable, TagCount * sizeof(TAG_TABLE_ENTRY), &cb, NULL);
#pragma warning(pop)

						if (bRet) {
							for (size_t i = 0; i < TagCount; i++) {
								TagTable[i].Offset = swap32(TagTable[i].Offset);
								TagTable[i].Size = swap32(TagTable[i].Size);
								if ('tgcv' == TagTable[i].Signature) {
									vcgtIndex = static_cast<int>(i);
								}
								if ('00SM' == TagTable[i].Signature) {
									wcsProfileIndex = static_cast<int>(i);
								}
							}
							LARGE_INTEGER moveTo;
							if (-1 != vcgtIndex) {
								moveTo.LowPart = TagTable[vcgtIndex].Offset;
								moveTo.HighPart = 0;
								bRet = SetFilePointerEx(hFile, moveTo, NULL, FILE_BEGIN);
								if (bRet) {
									pVCGT = reinterpret_cast<VCGT_HEADER *>(new BYTE[TagTable[vcgtIndex].Size]);
									bRet = ReadFile(hFile, pVCGT, TagTable[vcgtIndex].Size, &cb, NULL);
									if (bRet) {
										pVCGT->vcgtType = static_cast<VCGT_TYPE>(swap32(pVCGT->vcgtType));
										if (VCGT_TYPE_TABLE == pVCGT->vcgtType) {
											pVCGT->vcgtContents.t.vcgtChannels = swap16(pVCGT->vcgtContents.t.vcgtChannels);
											pVCGT->vcgtContents.t.vcgtCount = swap16(pVCGT->vcgtContents.t.vcgtCount);
											pVCGT->vcgtContents.t.vcgtItemSize = swap16(pVCGT->vcgtContents.t.vcgtItemSize);
										} else {
											pVCGT->vcgtContents.f.vcgtRedGamma = swap32(pVCGT->vcgtContents.f.vcgtRedGamma);
											pVCGT->vcgtContents.f.vcgtRedMin = swap32(pVCGT->vcgtContents.f.vcgtRedMin);
											pVCGT->vcgtContents.f.vcgtRedMax = swap32(pVCGT->vcgtContents.f.vcgtRedMax);

											pVCGT->vcgtContents.f.vcgtGreenGamma = swap32(pVCGT->vcgtContents.f.vcgtGreenGamma);
											pVCGT->vcgtContents.f.vcgtGreenMin = swap32(pVCGT->vcgtContents.f.vcgtGreenMin);
											pVCGT->vcgtContents.f.vcgtGreenMax = swap32(pVCGT->vcgtContents.f.vcgtGreenMax);

											pVCGT->vcgtContents.f.vcgtBlueGamma = swap32(pVCGT->vcgtContents.f.vcgtBlueGamma);
											pVCGT->vcgtContents.f.vcgtBlueMin = swap32(pVCGT->vcgtContents.f.vcgtBlueMin);
											pVCGT->vcgtContents.f.vcgtBlueMax = swap32(pVCGT->vcgtContents.f.vcgtBlueMax);
										}
									} else {
										s += ShowError(L"ReadFile");
									}
								} else {
									s += ShowError(L"SetFilePointerEx");
								}
							}

							if (-1 != wcsProfileIndex) {
								moveTo.LowPart = TagTable[wcsProfileIndex].Offset;
								moveTo.HighPart = 0;
								bRet = SetFilePointerEx(hFile, moveTo, NULL, FILE_BEGIN);
								if (bRet) {
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
								} else {
									s += ShowError(L"SetFilePointerEx");
								}
							}
						} else {
							s += ShowError(L"ReadFile");
						}
					} else {
						s += ShowError(L"ReadFile");
					}
				} else {
					s += ShowError(L"ReadFile");
				}
			} else {
				s += ShowError(L"GetFileSizeEx");
			}
			CloseHandle(hFile);
		} else {
			s += ShowError(L"CreateFile");
		}
	} else {
		s += ShowError(L"GetColorDirectory");
	}
	ErrorString = s;
	return s;
#pragma warning(pop)
}

// Return a profile filename (no path) from the registry
//
wchar_t * Profile::FindProfileName(HKEY hKeyBase, const wchar_t * registryKey, bool * perUser) {
	HKEY hKey;
	wchar_t * profileName = 0;
	int len = 0;
	if (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase, registryKey, 0, KEY_QUERY_VALUE, &hKey)) {
		DWORD dataSize;

		// Fetch the list of profiles
		//
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"ICMProfile", NULL, NULL, NULL, &dataSize)) {
			dataSize += sizeof(wchar_t);
			void * profileListBase = malloc(dataSize);
			if (profileListBase) {
				wchar_t * profileList = (wchar_t *)profileListBase;
				memset(profileList, 0, dataSize);
				RegQueryValueEx(hKey, L"ICMProfile", NULL, NULL, (LPBYTE)profileList, &dataSize);

				if (*profileList) {
					wchar_t * profilePtr = 0;

					// Find the last profile in the list
					//
					while (*profileList) {
						profilePtr = profileList;
						len = 1 + lstrlenW(profileList);
						profileList += len;
					}
					profileName = (wchar_t *)malloc(sizeof(wchar_t) * len);
					if (profileName) {
						wcscpy_s(profileName, len, profilePtr);
					}
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
					RegQueryValueEx(hKey, L"UsePerUserProfiles", NULL, NULL, (LPBYTE)&data, &dataSize);
				}
			}
			*perUser = (1 == data);
		}
		RegCloseKey(hKey);
	}
	return profileName;
}

// Return a string for the per-monitor panel
//
wstring Profile::DetailsString(void) const {

	wstring s;

	// Quit early if no profile
	//
	if (ProfileName.empty()) {
		s = L"No profile\r\n";
		return s;
	}

	// We should have loaded the profile earlier, so if we didn't, then there was an error
	//
	if ( 0 == ProfileSize.QuadPart ) {
		s = ErrorString;
		return s;
	}

	s.reserve(10240);					// Reduce repeated reallocation
	wchar_t buf[1024];
	wchar_t filepath[1024];
	DWORD filepathSize = sizeof(filepath);
	BOOL bRet = GetColorDirectoryW(NULL, filepath, &filepathSize);
	if (bRet) {
		StringCbCat(filepath, sizeof(filepath), L"\\");
		StringCbCat(filepath, sizeof(filepath), ProfileName.c_str());
		s += L"Active profile: '";
		s += filepath;
		StringCbPrintf(buf, sizeof(buf), L"', %I64u bytes\r\n", ProfileSize);
		s += buf;
		StringCbPrintf(buf, sizeof(buf), L"Profile length = %d bytes\r\n", ProfileHeader->phSize);
		s += buf;
		BYTE * pb = reinterpret_cast<BYTE *>(&ProfileHeader->phVersion);
		StringCbPrintf(buf, sizeof(buf), L"Profile version = %d.%d.%d\r\n", pb[0], (pb[1]>>4), (pb[1]&0x0F));
		s += buf;
		StringCbPrintf(buf, sizeof(buf), L"Profile class is %s\r\n", (CLASS_MONITOR == ProfileHeader->phClass) ? L"Monitor" : L"not Monitor");
		s += buf;
		StringCbPrintf(buf, sizeof(buf), L"\r\nProfile header (%d bytes):\r\n", sizeof(PROFILEHEADER));
		s += buf;
		s += HexDump(reinterpret_cast<unsigned char *>(ProfileHeader), sizeof(PROFILEHEADER), 16);
		StringCbPrintf(buf, sizeof(buf), L"\r\nProfile contains %d tags:\r\n", TagCount);
		s += buf;
		for (size_t i = 0; i < TagCount; i++) {
			pb = reinterpret_cast<BYTE *>(&TagTable[i].Signature);
			StringCbPrintf(buf, sizeof(buf), L"'%C%C%C%C'", pb[0], pb[1], pb[2], pb[3]);
			s += buf;
			if (i < TagCount - 1) {
				s += L", ";
			}
		}
		if (-1 == vcgtIndex) {
			s += L"\r\n\r\nProfile does not include a Video Card Gamma Tag\r\n";
		} else {
			s += L"\r\n\r\nProfile includes a Video Card Gamma Tag:\r\n";
			pb = reinterpret_cast<BYTE *>(&pVCGT->vcgtSignature);
			StringCbPrintf(buf, sizeof(buf), L"    Signature: '%C%C%C%C'", pb[0], pb[1], pb[2], pb[3]);
			s += buf;
			s += L"\r\n    Type: ";
			if (VCGT_TYPE_TABLE == pVCGT->vcgtType) {
				s += L"Table";
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Channels: %d", pVCGT->vcgtContents.t.vcgtChannels);
				s += buf;
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Count: %d entries per channel", pVCGT->vcgtContents.t.vcgtCount);
				s += buf;
				wchar_t * str;
				if ( 1 == pVCGT->vcgtContents.t.vcgtItemSize ) {
					str = L"\r\n    Size: %d byte per entry";
				} else {
					str = L"\r\n    Size: %d bytes per entry";
				}
				StringCbPrintf(buf, sizeof(buf), str, pVCGT->vcgtContents.t.vcgtItemSize);
				s += buf;
			} else {
				s += L"Formula";

				StringCbPrintf(buf, sizeof(buf), L"\r\n    Red gamma: %6.3f", double(pVCGT->vcgtContents.f.vcgtRedGamma) / double(65536));
				s += buf;
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Red min: %6.3f", double(pVCGT->vcgtContents.f.vcgtRedMin) / double(65536));
				s += buf;
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Red max: %6.3f", double(pVCGT->vcgtContents.f.vcgtRedMax) / double(65536));
				s += buf;

				StringCbPrintf(buf, sizeof(buf), L"\r\n    Green gamma: %6.3f", double(pVCGT->vcgtContents.f.vcgtGreenGamma) / double(65536));
				s += buf;
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Green min: %6.3f", double(pVCGT->vcgtContents.f.vcgtGreenMin) / double(65536));
				s += buf;
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Green max: %6.3f", double(pVCGT->vcgtContents.f.vcgtGreenMax) / double(65536));
				s += buf;

				StringCbPrintf(buf, sizeof(buf), L"\r\n    Blue gamma: %6.3f", double(pVCGT->vcgtContents.f.vcgtBlueGamma) / double(65536));
				s += buf;
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Blue min: %6.3f", double(pVCGT->vcgtContents.f.vcgtBlueMin) / double(65536));
				s += buf;
				StringCbPrintf(buf, sizeof(buf), L"\r\n    Blue max: %6.3f", double(pVCGT->vcgtContents.f.vcgtBlueMax) / double(65536));
				s += buf;
			}
			StringCbPrintf(buf, sizeof(buf), L"\r\n\r\nDump of Video Card Gamma Tag (%d bytes):\r\n", TagTable[vcgtIndex].Size);
			s += buf;
			s += HexDump(reinterpret_cast<unsigned char *>(pVCGT), TagTable[vcgtIndex].Size, 16);
		}

		if (-1 != wcsProfileIndex) {
			s += L"\r\nProfile includes an embedded WCS profile ('MS00'):\r\n";

			//WCS_IN_ICC_HEADER * wiPtr = reinterpret_cast<WCS_IN_ICC_HEADER *>(bigBuffer);
			//pb = reinterpret_cast<BYTE *>(&wiPtr->wcshdrSignature);
			//StringCbPrintf(buf, sizeof(buf), L"    Signature: '%C%C%C%C'\r\n", pb[0], pb[1], pb[2], pb[3]);
			//s += buf;
			//StringCbPrintf(buf, sizeof(buf), L"\r\nDump of embedded WCS profile header (%d bytes):\r\n", sizeof(WCS_IN_ICC_HEADER));
			//s += buf;
			//s += HexDump(reinterpret_cast<unsigned char *>(wiPtr), sizeof(WCS_IN_ICC_HEADER), 16);

			// A WCS profile embedded in an ICC profile is a set of three little-endian Unicode (UCS2)
			// XML "files" strung together, with a header that says how to find individual sections.
			// The three sections are:
			//   1) Color Device Model;
			//   2) Color Appearance Model;
			//   3) Gamut Map Model
			//
			if (!WCS_ColorDeviceModel.empty()) {
				s += L"\r\n\t\tColor Device Model:\r\n";
				s += WCS_ColorDeviceModel;
			}
			if (!WCS_ColorAppearanceModel.empty()) {
				s += L"\r\n\t\tColor Appearance Model:\r\n";
				s += WCS_ColorAppearanceModel;
			}
			if (!WCS_GamutMapModel.empty()) {
				s += L"\r\n\t\tGamut Map Model:\r\n";
				s += WCS_GamutMapModel;
			}
		}
	} else {
		s += ShowError(L"GetColorDirectory");
	}
	return s;
}
