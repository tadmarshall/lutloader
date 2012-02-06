// Monitor.cpp -- Monitor class for handling display monitors returned by EnumDisplayDevices()
//

#include "stdafx.h"
#include "Adapter.h"
#include "Monitor.h"
#include "Utility.h"
#include <strsafe.h>

// Constructor
//
Monitor::Monitor(Adapter * hostAdapter, const DISPLAY_DEVICEW & displayMonitor) :
		DeviceName(displayMonitor.DeviceName),
		DeviceString(displayMonitor.DeviceString),
		StateFlags(displayMonitor.StateFlags),
		DeviceID(displayMonitor.DeviceID),
		DeviceKey(displayMonitor.DeviceKey),
		adapter(hostAdapter),
		pLUT(0),
		UserProfile(0),
		SystemProfile(0),
		activeProfileIsUserProfile(false)
{
}

// Destructor
//
Monitor::~Monitor() {
	if (pLUT) {
		delete [] pLUT;
	}
}

// Initialize
//
void Monitor::Initialize(void) {

	// Make a copy of the current LUT
	//
	HDC hDC = CreateDC(adapter->GetDeviceName().c_str(), 0, 0, 0);
	if (hDC) {
		pLUT = new LUT;
		SecureZeroMemory(pLUT, sizeof(LUT));
		GetDeviceGammaRamp(hDC, pLUT);
		DeleteDC(hDC);
	}

	// Find all associated profiles
	//
	wchar_t registryKey[512];
	int len;
	if (VistaOrHigher()) {

		// Vista or higher, set up user profile, if any
		//
		len = lstrlenW(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
		wcscpy_s(
			registryKey,
			_countof(registryKey),
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
		wcscat_s(registryKey, _countof(registryKey), &DeviceKey.c_str()[len]);
		UserProfile = Profile::GetAllProfiles(HKEY_CURRENT_USER, registryKey, &activeProfileIsUserProfile, UserProfileList);
	}

	// Set up SystemProfile for any supported OS (this will be the only profile for XP)
	//
	len = lstrlenW(L"\\Registry\\Machine\\");
	wcscpy_s(registryKey, _countof(registryKey), &DeviceKey.c_str()[len]);
	SystemProfile = Profile::GetAllProfiles(HKEY_LOCAL_MACHINE, registryKey, 0, SystemProfileList);
}

// Vector of monitors
//
static vector <Monitor *> monitorList;

// Add a monitor to the end of the list
//
Monitor * Monitor::Add(Monitor * monitor) {
	monitorList.push_back(monitor);
	return monitor;
}

// Clear the list of monitors
//
void Monitor::ClearList(bool freeAllMemory) {
	size_t count = monitorList.size();
	for (size_t i = 0; i < count; ++i) {
		delete monitorList[i];
	}
	monitorList.clear();
	if ( freeAllMemory && (monitorList.capacity() > 0) ) {
		vector <Monitor *> dummy;
		monitorList.swap(dummy);
	}
}

// Return 'true' if this monitor is active and part of the desktop
//
bool Monitor::IsActive(const DISPLAY_DEVICEW & displayMonitor) {
	return (displayMonitor.StateFlags & DISPLAY_DEVICE_ACTIVE) && (displayMonitor.StateFlags & DISPLAY_DEVICE_ATTACHED);
}

// Return a string for the Summary panel
//
wstring Monitor::SummaryString(void) const {
	wstring s;

	// Build a display string for this monitor
	//
	s += DeviceString;
	if( adapter->GetStateFlags() & DISPLAY_DEVICE_PRIMARY_DEVICE ) {			// Note primary monitor
		s += L" (primary)";
	}
	if ( VistaOrHigher() ) {
		s += L"\r\n    User default profile: ";
		if (UserProfile) {
			s += UserProfile->GetName();
		} else {
			s += L"<No user profiles>";
		}
		if (activeProfileIsUserProfile) {
			s += L" (active)";
		}
		s += L"\r\n    System default profile: ";
		if (SystemProfile) {
			s += SystemProfile->GetName();
		} else {
			s += L"<No system profiles>";
		}
		if (!activeProfileIsUserProfile) {
			s += L" (active)";
		}
	} else {
		s += L"\r\n    Active profile: ";
		if (SystemProfile) {
			s += SystemProfile->GetName();
		} else {
			s += L"<No profiles>";
		}
	}
	s += L"\r\n\r\n";

	// See if the loaded LUT is correct
	//
	Profile * active = GetActiveProfile();
	active->Load(false);
	switch (active->CompareLUT(pLUT)) {

		// LUTs match word for word
		//
		case LC_EQUAL:
			s += L"The currently loaded LUT matches the active profile exactly\r\n";
			break;

		// One uses 0x0100, the other uses 0x0101 style
		//
		case LC_VARIATION_ON_LINEAR:
			s += L"The currently loaded LUT and the active profile are both linear but with style differences\r\n";
			break;

		// Low byte zeroed
		//
		case LC_TRUNCATION_IN_LOW_BYTE:
			s += L"The currently loaded LUT is similar to the active profile but has had the low byte zeroed\r\n";
			break;

		// Windows 7 rounds to nearest high byte
		//
		case LC_ROUNDING_IN_LOW_BYTE:
			s += L"The currently loaded LUT is similar to the active profile but has been rounded to the nearest multiple of 256\r\n";
			break;

		// Match, given the circumstances
		//
		case LC_PROFILE_HAS_NO_LUT_OTHER_LINEAR:
			s += L"The currently loaded LUT is linear and the active profile has no LUT\r\n";
			break;

		// Mismatch, other should be linear
		//
		case LC_PROFILE_HAS_NO_LUT_OTHER_NONLINEAR:
			s += L"The currently loaded LUT is  not linear but it should be because the active profile has no LUT\r\n";
			break;

		// LUTs do not meet any matching criteria
		//
		case LC_UNEQUAL:
			s += L"The currently loaded LUT does not match the active profile\r\n";
			break;

	}

#if 0
	// Display the start of the gamma ramp
	//
	s += L"    Start of gamma ramp:\r\n  Red   =";
	for (int i = 0; i < 4; i++) {
		wchar_t buf[10];
		StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->red[i]);
		s += buf;
	}
	s += L"\r\n  Green =";
	for (int i = 0; i < 4; i++) {
		wchar_t buf[10];
		StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->green[i]);
		s += buf;
	}
	s += L"\r\n  Blue  =";
	for (int i = 0; i < 4; i++) {
		wchar_t buf[10];
		StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->blue[i]);
		s += buf;
	}
#endif
	return s;
}

// Return a string for the per-monitor panel (Profile.cpp does all the work)
//
wstring Monitor::DetailsString(void) const {
	return activeProfileIsUserProfile ? UserProfile->DetailsString() : SystemProfile->DetailsString();
}

bool Monitor::GetActiveProfileIsUserProfile(void) const {
	return activeProfileIsUserProfile;
}

Profile * Monitor::GetActiveProfile(void) const {
	return activeProfileIsUserProfile ? UserProfile : SystemProfile;
}

Profile * Monitor::GetDefaultUserProfile(void) const {
	return UserProfile;
}

Profile * Monitor::GetDefaultSystemProfile(void) const {
	return SystemProfile;
}

ProfileList & Monitor::GetProfileList(bool userProfiles) {
	return userProfiles ? UserProfileList : SystemProfileList;
}

wstring Monitor::GetDeviceString(void) const {
	return DeviceString;
}

LUT * Monitor::GetLUT(void) const {
	return pLUT;
}

size_t Monitor::GetListSize(void) {
	return monitorList.size();
}

// Fetch a reference to a monitor by index number
//
Monitor * Monitor::Get(size_t index) {
	return monitorList[index];
}
