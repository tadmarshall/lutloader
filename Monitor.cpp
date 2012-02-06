// Monitor.cpp -- Monitor class for handling display monitors returned by EnumDisplayDevices().
//

#include "stdafx.h"
#include "Monitor.h"
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
	wchar_t * activeProfileName = 0;
	wchar_t registryKey[512];

	// See if we are Vista or greater, so we know if per-user monitor profiles are supported
	//
	OSVERSIONINFOEX osInfo;
	ZeroMemory(&osInfo, sizeof(osInfo));
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
	//static wstring GetMonitorSummaryString(size_t monitorNumber);
wstring Monitor::SummaryString(void) const {
	wstring s;

	// See if we are Vista or greater, so we know if per-user monitor profiles are supported
	//
	OSVERSIONINFOEX osInfo;
	ZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx((OSVERSIONINFO *)&osInfo);
	bool bVistaOrHigher = (osInfo.dwMajorVersion >= 6);

	// Build a display string for this monitor
	//
	s += DeviceString;
	//if( Adapter::GetAdapter(AdapterNumber).StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE ) {			// Note primary monitor
	if( Adapter::GetAdapterStateFlags(AdapterNumber) & DISPLAY_DEVICE_PRIMARY_DEVICE ) {			// Note primary monitor
		s += L" (primary)";
	}
	if (bVistaOrHigher) {
		s += L"\n    User default profile = ";
		s += userProfile;
		s += activeProfileIsUserProfile ? L" (active)" : L" (inactive)";
		s += L"\n    System default profile = ";
		s += systemProfile;
		s += activeProfileIsUserProfile ? L" (inactive)" : L" (active)";
	} else {
		s += L"\n    Default profile = ";
		s += systemProfile;
	}

	// Display the start of the gamma ramp
	//
	HDC hDC = CreateDC(Adapter::GetAdapterDeviceName(AdapterNumber).c_str(), 0, 0, 0);
	if (hDC) {
		s += L"\n    Gamma ramp =";
		WORD deviceGammaRamp[3][256];
		memset(deviceGammaRamp, 0, sizeof(deviceGammaRamp));
		GetDeviceGammaRamp(hDC, deviceGammaRamp);

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				wchar_t buf[10];
				StringCbPrintf(buf, sizeof(buf),  L" %04X", deviceGammaRamp[i][j]);
				s += buf;
			}
		}
		DeleteDC(hDC);
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

// Add a monitor to the end of the list
//
void Monitor::AddMonitor(Monitor monitor) {
	monitorList.push_back(monitor);
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
