// Monitor.cpp -- Monitor class for handling display monitors returned by EnumDisplayDevices().
//

#include "stdafx.h"
#include "Adapter.h"
#include "Monitor.h"
#include <icm.h>
#include <strsafe.h>

// Constructor
//
Monitor::Monitor(int adapterNumber, PDISPLAY_DEVICE displayMonitor) {
	AdapterNumber = adapterNumber;
	StateFlags = displayMonitor->StateFlags;
	DeviceName = displayMonitor->DeviceName;
	DeviceString = displayMonitor->DeviceString;
	DeviceID = displayMonitor->DeviceID;
	DeviceKey = displayMonitor->DeviceKey;
	activeProfileIsUserProfile = false;

	wchar_t * activeProfileName = 0;
	wchar_t registryKey[512];

	// See if we are Vista or greater, so we know if per-user monitor profiles are supported
	//
	OSVERSIONINFOEX osInfo;
	SecureZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx((OSVERSIONINFO *)&osInfo);
	if (osInfo.dwMajorVersion >= 6) {

		// Vista or higher, show user default and system default profiles
		//
		wchar_t * userProfileName;
		BOOL perUser = FALSE;
		int len = lstrlenW(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
		wcscpy_s(
			registryKey,
			COUNTOF(registryKey),
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
		wcscat_s(registryKey, COUNTOF(registryKey), &displayMonitor->DeviceKey[len]);
		userProfileName = Monitor::GetProfileName(HKEY_CURRENT_USER, registryKey, &perUser);
		if (userProfileName) {
			userProfile = userProfileName;
		}
		activeProfileIsUserProfile = (perUser && !userProfile.empty());

		wchar_t * systemProfileName;
		len = lstrlenW(L"\\Registry\\Machine\\");
		wcscpy_s(registryKey, COUNTOF(registryKey), &displayMonitor->DeviceKey[len]);
		systemProfileName = Monitor::GetProfileName(HKEY_LOCAL_MACHINE, registryKey, 0);
		if (systemProfileName) {
			systemProfile = systemProfileName;
		}

		// Choose active profile name (if any), free the other one, if any
		//
		if (perUser && userProfileName) {
			activeProfileName = userProfileName;
			if (systemProfileName) {
				free(systemProfileName);
			}
		} else {
			if (systemProfileName) {
				activeProfileName = systemProfileName;
				if (userProfileName) {
					free(userProfileName);
				}
			}
		}

	} else {

		// XP or lower, show a single default profile
		//
		int len = lstrlenW(L"\\Registry\\Machine\\");
		wcscpy_s(registryKey, COUNTOF(registryKey), &displayMonitor->DeviceKey[len]);
		activeProfileName = Monitor::GetProfileName(HKEY_LOCAL_MACHINE, registryKey, 0);
		if (activeProfileName) {
			systemProfile = activeProfileName;
		}
	}

	// If we have an active profilename, maybe install it or just release it for now
	//
	if (activeProfileName) {
		free(activeProfileName);
	}
}

// Return a string for the Summary panel
//
wstring Monitor::SummaryString(void) const {
	wstring s;

	// See if we are Vista or greater, so we know if per-user monitor profiles are supported
	//
	OSVERSIONINFOEX osInfo;
	SecureZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx((OSVERSIONINFO *)&osInfo);
	bool bVistaOrHigher = (osInfo.dwMajorVersion >= 6);

	// Build a display string for this monitor
	//
	s += DeviceString;
	if( Adapter::GetAdapterStateFlags(AdapterNumber) & DISPLAY_DEVICE_PRIMARY_DEVICE ) {			// Note primary monitor
		s += L" (primary)";
	}
	if (bVistaOrHigher) {
		s += L"\r\n    User default profile = ";
		s += userProfile;
		s += activeProfileIsUserProfile ? L" (active)" : L" (inactive)";
		s += L"\r\n    System default profile = ";
		s += systemProfile;
		s += activeProfileIsUserProfile ? L" (inactive)" : L" (active)";
	} else {
		s += L"\r\n    Default profile = ";
		s += systemProfile;
	}

	// Display the start of the gamma ramp
	//
	HDC hDC = CreateDC(Adapter::GetAdapterDeviceName(AdapterNumber).c_str(), 0, 0, 0);
	if (hDC) {
		s += L"\r\n    Gamma ramp =";
		WORD deviceGammaRamp[3][256];
		memset(deviceGammaRamp, 0, sizeof(deviceGammaRamp));
		GetDeviceGammaRamp(hDC, deviceGammaRamp);

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				wchar_t buf[10];
				StringCbPrintf(buf, sizeof(buf), L" %04X", deviceGammaRamp[i][j]);
				s += buf;
			}
		}
		DeleteDC(hDC);
	}
	return s;
}

// Display data as a hex & ANSI dump
//
wstring HexDump(LPBYTE data, size_t size, UINT rowWidth) {
	wstring s;

	for (unsigned int i = 0; i < size; i+=rowWidth) {
		unsigned int j, k, l;
		wchar_t buf[8];
		j = i & 0x0FFFF;					// Poor man's sequence numbers
		StringCchPrintf(buf, _countof(buf), L"%04x|", j);
		s += buf;
		l = min(i+rowWidth, size);			// Calculate last position for this row
		for (k = i; k < l; k++) {			// Print (up to) ROW_WIDTH hex bytes
			StringCchPrintf(buf, _countof(buf), L"%02x ", data[k]);
			s += buf;
		}
		for (; k < (i+rowWidth); k++) {		// Handle partial last line
			s += L"   ";					// Print spaces to align ANSI part
		}
		for (k = i; k < l; k++) {			// Print (up to) rowWidth ANSI characters
			int wch = static_cast<int>(data[k]);
			//if ( (wch < 32) || ( (wch > 126) && (wch < 161) ) ) {
			if ( (wch < 32) || (wch > 126) ) {
				s += L".";					// Non-printable using Courier New font, print a period instead
			} else {
				s += static_cast<wchar_t>(wch);
			}
		}
		s += L"\r\n";							// New line after rowWidth bytes
	}
	return s;
}

// Return a string showing failure of the last system call
//
wstring ShowError(wchar_t * functionName) {
	wstring s;
	wchar_t buf[1024];
	DWORD err = GetLastError();
	StringCbPrintf(buf, sizeof(buf), L"%s failed (0x%x): ", functionName, err);
	s += buf;
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, buf, COUNTOF(buf), NULL);
	s += buf;
	s += L"\r\n";
	return s;
}

#if 0 // It turns out that EnumICMProfiles is one of the problem APIs on multi-monitor.  It returns the wrong list.
wstring * ps;		// Helper for callback

int CALLBACK EnumICMProfilesProcCallback(
    LPTSTR lpszFilename,
    LPARAM lParam
) {
	UNREFERENCED_PARAMETER(lParam);

	(*ps) += lpszFilename;
	(*ps) += L"\r\n";
	return 1;
}
#endif

// Return a string for the per-monitor panel
//
wstring Monitor::DetailsString(void) const {
	wstring s;
	s.reserve(10240);					// Reduce repeated reallocation
	//ps = &s;
	//HDC hDC = NULL;
	wchar_t buf[1024];
	wchar_t filepath[1024];
	DWORD filepathSize = sizeof(filepath);
	BOOL bRet = GetColorDirectoryW(NULL, filepath, &filepathSize);
	if (bRet) {
		StringCbCat(filepath, sizeof(filepath), L"\\");
		StringCbCat(filepath, sizeof(filepath), (activeProfileIsUserProfile ? userProfile : systemProfile).c_str());
		s += L"Active profile: '";
		s += filepath;
		s += L"'";

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
			LARGE_INTEGER fileSize;
			bRet = GetFileSizeEx(hFile, &fileSize);
			if (bRet) {
				StringCbPrintf(buf, sizeof(buf), L", %I64u bytes\r\n", fileSize.QuadPart);
				s += buf;

				PROFILEHEADER ph;
				DWORD cb;
				bRet = ReadFile(hFile, &ph, sizeof(ph), &cb, NULL);
				if (bRet) {
					StringCbPrintf(buf, sizeof(buf), L"Profile length = %d bytes\r\n", swap32(ph.phSize));
					s += buf;

					BYTE * pb = reinterpret_cast<BYTE *>(&ph.phVersion);
					StringCbPrintf(buf, sizeof(buf), L"Profile version = %d.%d.%d\r\n", pb[0], (pb[1]>>4), (pb[1]&0x0F));
					s += buf;

					StringCbPrintf(buf, sizeof(buf), L"Profile class is %s\r\n", (CLASS_MONITOR == swap32(ph.phClass)) ? L"Monitor" : L"not Monitor");
					s += buf;

					s += L"\r\nProfile header:\r\n";
					s += HexDump(reinterpret_cast<unsigned char *>(&ph), sizeof(ph), 16);
				} else {
					s += ShowError(L"ReadFile");
				}
			} else {
				s += ShowError(L"\nGetFileSizeEx");
			}
			CloseHandle(hFile);

		} else {
			s += ShowError(L"\nCreateFile");
		}

		// Testing OpenColorProfile()
		//
		PROFILE profileStruct;
		profileStruct.dwType = PROFILE_FILENAME;
		profileStruct.pProfileData = filepath;
		profileStruct.cbDataSize = static_cast<DWORD>( sizeof(wchar_t) * (wcslen(filepath) + 1) );
		HPROFILE hProfile = OpenColorProfile(&profileStruct, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING);
		if (hProfile) {
			DWORD tagCount;
			bRet = GetCountColorProfileElements(hProfile, &tagCount);
			if (bRet) {
				StringCbPrintf(buf, sizeof(buf), L"\r\nProfile contains %d tags:\r\n", tagCount);
				s += buf;

				TAGTYPE tagType;
				for (DWORD i = 1; i <= tagCount; i++) {
					GetColorProfileElementTag(hProfile, i, &tagType);
					BYTE * pb = reinterpret_cast<BYTE *>(&tagType);
					StringCbPrintf(buf, sizeof(buf), L"'%C%C%C%C'", pb[3], pb[2], pb[1], pb[0]);
					s += buf;
					if (i < tagCount) {
						s += L", ";
					}
				}
				s += L"\r\n";

				BYTE bigBuffer[10240];
				DWORD cb = sizeof(bigBuffer);
				BOOL multiReference = 0;
				bRet = GetColorProfileElement(hProfile, 'vcgt', 0, &cb, bigBuffer, &multiReference);
				if (bRet) {
					s += L"\r\nProfile includes Video Card Gamma Tag:\r\n";
					VCGT_HEADER * pVCGT = reinterpret_cast<VCGT_HEADER *>(&bigBuffer);
					BYTE * pb = reinterpret_cast<BYTE *>(&pVCGT->vcgtSignature);
					StringCbPrintf(buf, sizeof(buf), L"    Signature: '%C%C%C%C'", pb[0], pb[1], pb[2], pb[3]);
					s += buf;
					s += L"\r\n    Type: ";
					if (VCGT_TYPE_TABLE == swap32(pVCGT->vcgtType)) {
						s += L"Table";
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Channels: %d", swap16(pVCGT->vcgtContents.t.vcgtChannels));
						s += buf;
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Count: %d entries per channel", swap16(pVCGT->vcgtContents.t.vcgtCount));
						s += buf;
						WORD entrySize = swap16(pVCGT->vcgtContents.t.vcgtItemSize);
						wchar_t * str;
						if ( 1 == entrySize ) {
							str = L"\r\n    Size: %d byte per entry";
						} else {
							str = L"\r\n    Size: %d bytes per entry";
						}
						StringCbPrintf(buf, sizeof(buf), str, entrySize);
						s += buf;
					} else {
						s += L"Formula";

						StringCbPrintf(buf, sizeof(buf), L"\r\n    Red gamma: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtRedGamma)) / double(65536));
						s += buf;
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Red min: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtRedMin)) / double(65536));
						s += buf;
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Red max: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtRedMax)) / double(65536));
						s += buf;

						StringCbPrintf(buf, sizeof(buf), L"\r\n    Green gamma: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtGreenGamma)) / double(65536));
						s += buf;
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Green min: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtGreenMin)) / double(65536));
						s += buf;
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Green max: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtGreenMax)) / double(65536));
						s += buf;

						StringCbPrintf(buf, sizeof(buf), L"\r\n    Blue gamma: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtBlueGamma)) / double(65536));
						s += buf;
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Blue min: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtBlueMin)) / double(65536));
						s += buf;
						StringCbPrintf(buf, sizeof(buf), L"\r\n    Blue max: %6.3f", double(swap32(pVCGT->vcgtContents.f.vcgtBlueMax)) / double(65536));
						s += buf;
					}
					s += L"\r\n\r\nRaw dump of Video Card Gamma Tag:\r\n";
					s += HexDump(reinterpret_cast<unsigned char *>(&bigBuffer), cb, 16);
				} else {
					s += ShowError(L"GetColorProfileElement");
				}
			} else {
				s += ShowError(L"GetCountColorProfileElements");
			}
			CloseColorProfile(hProfile);
		} else {
			s += ShowError(L"OpenColorProfile");
		}

#if 0 // It turns out that this is one of the problem APIs on multi-monitor.  It may return the wrong profile.
		// Testing GetICMProfile
		//
		hDC = CreateDC(Adapter::GetAdapterDeviceName(AdapterNumber).c_str(), 0, 0, 0);
		if (hDC) {
			wchar_t profileFromDC[1024];
			DWORD cb = COUNTOF(profileFromDC);
			bRet = GetICMProfile(hDC, &cb, profileFromDC);
			if (bRet) {
				StringCbPrintf(buf, sizeof(buf), L"\r\nGetICMProfile reports: %s\r\n", profileFromDC);
				s += buf;
			} else {
				s += ShowError(L"GetICMProfile");
			}
			DeleteDC(hDC);
		} else {
			s += ShowError(L"CreateDC");
		}
#endif

#if 0 // It turns out that this is one of the problem APIs on multi-monitor.  It returns the wrong list.
		// Testing EnumICMProfiles
		//
		hDC = CreateDC(Adapter::GetAdapterDeviceName(AdapterNumber).c_str(), 0, 0, 0);
		if (hDC) {
			s += L"\r\nCalling EnumICMProfiles:\r\n";

			EnumICMProfiles(hDC, EnumICMProfilesProcCallback, 0);
			DeleteDC(hDC);
		} else {
			s += ShowError(L"CreateDC");
		}
#endif
	} else {
		s += ShowError(L"GetColorDirectory");
	}
	return s;
}

// Return 'true' if this monitor is active and part of the desktop
//
bool Monitor::IsMonitorActive(PDISPLAY_DEVICE displayMonitor) {
	return (displayMonitor->StateFlags & DISPLAY_DEVICE_ACTIVE) && (displayMonitor->StateFlags & DISPLAY_DEVICE_ATTACHED);
}

// Vector of monitors
//
static vector <Monitor> monitorList;

// Clear the list of monitors
//
void Monitor::ClearMonitorList(bool freeAllMemory) {
	monitorList.clear();
	if ( freeAllMemory && (monitorList.capacity() > 0) ) {
		vector <Monitor> dummy;
		monitorList.swap(dummy);
	}
}

// Add a monitor to the end of the list
//
void Monitor::AddMonitor(Monitor * const monitor) {
	monitorList.push_back(*monitor);
}

// Return the size of the monitor list
//
size_t Monitor::GetMonitorListSize(void) {
	return monitorList.size();
}

// Fetch a reference to a monitor by index number
//
Monitor & Monitor::GetMonitor(size_t monitorNumber) {
	return monitorList[monitorNumber];
}

// Fetch a monitor's DeviceString by index number
//
wstring Monitor::GetMonitorDeviceString(size_t monitorNumber) {
	return monitorList[monitorNumber].DeviceString;
}

// Return a profile filename (no path) from the registry
//
wchar_t * Monitor::GetProfileName(HKEY hKeyBase, wchar_t * registryKey, BOOL * perUser) {
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
