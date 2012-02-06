// Monitor.cpp -- Monitor class for handling display monitors returned by EnumDisplayDevices().
//

#include "stdafx.h"
#include "Adapter.h"
#include "Monitor.h"
#include <strsafe.h>

// Constructor
//
Monitor::Monitor(int adapterNumber, const DISPLAY_DEVICEW & displayMonitor) :
		DeviceName(displayMonitor.DeviceName),
		DeviceString(displayMonitor.DeviceString),
		StateFlags(displayMonitor.StateFlags),
		DeviceID(displayMonitor.DeviceID),
		DeviceKey(displayMonitor.DeviceKey),
		activeProfileIsUserProfile(false),
		AdapterNumber(adapterNumber)
{
	wchar_t registryKey[512];
	int len;

	// See if we are Vista or greater, so we know if per-user monitor profiles are supported
	//
	OSVERSIONINFOEX osInfo;
	SecureZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx((OSVERSIONINFO *)&osInfo);
	if (osInfo.dwMajorVersion >= 6) {

		// Vista or higher, set up user profile, if any
		//
		wchar_t * userProfileName;
		len = lstrlenW(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
		wcscpy_s(
			registryKey,
			_countof(registryKey),
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
		wcscat_s(registryKey, _countof(registryKey), &displayMonitor.DeviceKey[len]);
		userProfileName = Profile::FindProfileName(HKEY_CURRENT_USER, registryKey, &activeProfileIsUserProfile);
		if (userProfileName) {
			UserProfile.SetName(userProfileName);
			free(userProfileName);
			if (activeProfileIsUserProfile) {
				UserProfile.Load();
			}
		}
	}

	// Set up SystemProfile for any supported OS (this will be the only profile for XP)
	//
	wchar_t * systemProfileName;
	len = lstrlenW(L"\\Registry\\Machine\\");
	wcscpy_s(registryKey, _countof(registryKey), &displayMonitor.DeviceKey[len]);
	systemProfileName = Profile::FindProfileName(HKEY_LOCAL_MACHINE, registryKey, 0);
	if (systemProfileName) {
		SystemProfile.SetName(systemProfileName);
		free(systemProfileName);
		if (!activeProfileIsUserProfile) {
			SystemProfile.Load();
		}
	}
}

// Copy constructor
//
Monitor::Monitor(const Monitor & from) :
		DeviceName(from.DeviceName),
		DeviceString(from.DeviceString),
		StateFlags(from.StateFlags),
		DeviceID(from.DeviceID),
		DeviceKey(from.DeviceKey),
		UserProfile(from.UserProfile),
		SystemProfile(from.SystemProfile),
		activeProfileIsUserProfile(from.activeProfileIsUserProfile),
		AdapterNumber(from.AdapterNumber)
{
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
	GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&osInfo));
	bool bVistaOrHigher = (osInfo.dwMajorVersion >= 6);

	// Build a display string for this monitor
	//
	s += DeviceString;
	if( Adapter::GetAdapterStateFlags(AdapterNumber) & DISPLAY_DEVICE_PRIMARY_DEVICE ) {			// Note primary monitor
		s += L" (primary)";
	}
	if (bVistaOrHigher) {
		s += L"\r\n    User default profile: ";
		s += UserProfile.GetName();
		if (activeProfileIsUserProfile) {
			s += L" (active)";
		}
		s += L"\r\n    System default profile: ";
		s += SystemProfile.GetName();
		if (!activeProfileIsUserProfile) {
			s += L" (active)";
		}
	} else {
		s += L"\r\n    Active profile: ";
		s += SystemProfile.GetName();
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

// Return a string for the per-monitor panel (Profile.cpp does all the work)
//
wstring Monitor::DetailsString(void) const {
	return activeProfileIsUserProfile ? UserProfile.DetailsString() : SystemProfile.DetailsString();
}

// Return 'true' if this monitor is active and part of the desktop
//
bool Monitor::IsMonitorActive(const DISPLAY_DEVICEW & displayMonitor) {
	return (displayMonitor.StateFlags & DISPLAY_DEVICE_ACTIVE) && (displayMonitor.StateFlags & DISPLAY_DEVICE_ATTACHED);
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
void Monitor::AddMonitor(const Monitor & monitor) {
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
