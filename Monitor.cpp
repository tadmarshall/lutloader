// Monitor.cpp -- Monitor class for handling display monitors returned by EnumDisplayDevices()
//

#include "stdafx.h"
#include <winreg.h>
#include "Adapter.h"
#include "Monitor.h"
#include "MonitorPage.h"
#include "MonitorSummaryItem.h"
#include "Utility.h"
#include <strsafe.h>
//#include <banned.h>

// Optional "features"
//
#define SHOW_GAMMA_RAMP_IN_SUMMARY_STRING 1

// Some constants
//
#define COUNT_OF_GAMMA_VALUES_TO_SHOW 10

// Constructor
//
Monitor::Monitor(Adapter * hostAdapter, const DISPLAY_DEVICEW & displayMonitor) :
		DeviceName(displayMonitor.DeviceName),
		DeviceString(displayMonitor.DeviceString),
		StateFlags(displayMonitor.StateFlags),
		DeviceID(displayMonitor.DeviceID),
		DeviceKey(displayMonitor.DeviceKey),
		adapter(hostAdapter),
		monitorPage(0),
		monitorSummaryItem(0),
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

// Read the LUT from the adapter
//
bool Monitor::ReadLutFromCard(void) {
	if (pLUT) {
		delete [] pLUT;
		pLUT = 0;
	}
	BOOL bRet = 0;
	HDC hDC = CreateDC(adapter->GetDeviceName().c_str(), 0, 0, 0);
	if (hDC) {
		pLUT = new LUT;
		SecureZeroMemory(pLUT, sizeof(LUT));
		bRet = GetDeviceGammaRamp(hDC, pLUT);
		DeleteDC(hDC);
	}
	return ( 0 != bRet );
}

// Write a LUT (from any source) to the adapter
//
bool Monitor::WriteLutToCard(LUT * lutToWriteToAdapter) const {
	if (lutToWriteToAdapter) {
		BOOL bRet = 0;
		HDC hDC = CreateDC(adapter->GetDeviceName().c_str(), 0, 0, 0);
		if (hDC) {

			// Try doing it twice with slightly different ramps ...
			//
			++lutToWriteToAdapter->red[0];
			bRet = SetDeviceGammaRamp(hDC, lutToWriteToAdapter);
			--lutToWriteToAdapter->red[0];
			bRet = SetDeviceGammaRamp(hDC, lutToWriteToAdapter);
			DeleteDC(hDC);
		}
		return ( 0 != bRet );
	} else {
		return false;
	}
}

// Initialize
//
void Monitor::Initialize(void) {

	ReadLutFromCard();

	// Find all associated profiles
	//
	wchar_t registryKey[1024];
	int len;
	if ( VistaOrHigher() ) {

		// Vista or higher, set up user profile, if any
		//
		len = StringLength(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
		StringCbCopy( registryKey, sizeof(registryKey),
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
		StringCbCat(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
		UserProfile = Profile::GetAllProfiles(HKEY_CURRENT_USER, registryKey, &activeProfileIsUserProfile, UserProfileList);
	}

	// Set up SystemProfile for any supported OS (this will be the only profile for XP)
	//
	len = StringLength(L"\\Registry\\Machine\\");
	StringCbCopy(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	SystemProfile = Profile::GetAllProfiles(HKEY_LOCAL_MACHINE, registryKey, 0, SystemProfileList);
}

void Monitor::AddProfileToInternalUserList(Profile * profile) {
	UserProfileList.insert(UserProfileList.begin(), profile);
	if ( 1 == UserProfileList.size() ) {
		UserProfile = profile;
	}
}

void Monitor::AddProfileToInternalSystemList(Profile * profile) {
	SystemProfileList.insert(SystemProfileList.begin(), profile);
	if ( 1 == SystemProfileList.size() ) {
		SystemProfile = profile;
	}
}

void Monitor::RemoveProfileFromInternalUserList(Profile * profile) {
	bool removingDefault = (profile == UserProfile);
	ProfileList::iterator itEnd = UserProfileList.end();
	for (ProfileList::iterator it = UserProfileList.begin(); it != itEnd; ++it) {
		if ( *it == profile ) {
			UserProfileList.erase(it);
			break;
		}
	}
	if ( removingDefault ) {
		size_t count = UserProfileList.size();
		UserProfile = (0 == count) ? 0 : UserProfileList[count - 1];
	}
}

void Monitor::RemoveProfileFromInternalSystemList(Profile * profile) {
	bool removingDefault = (profile == SystemProfile);
	ProfileList::iterator itEnd = SystemProfileList.end();
	for (ProfileList::iterator it = SystemProfileList.begin(); it != itEnd; ++it) {
		if ( *it == profile ) {
			SystemProfileList.erase(it);
			break;
		}
	}
	if ( removingDefault ) {
		size_t count = SystemProfileList.size();
		SystemProfile = (0 == count) ? 0 : SystemProfileList[count - 1];
	}
}

bool Monitor::SetDefaultUserProfile(Profile * profile) {
	wchar_t registryKey[1024];
	int len = StringLength(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
	StringCbCopy( registryKey, sizeof(registryKey),
		L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
	StringCbCat(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	bool success = profile->EditRegistryProfileList(HKEY_CURRENT_USER, registryKey, true);
	if (success) {
		ProfileList::iterator itEnd = UserProfileList.end();
		for (ProfileList::iterator it = UserProfileList.begin(); it != itEnd; ++it) {
			if ( *it == profile ) {
				UserProfileList.erase(it);
				break;
			}
		}
		UserProfileList.push_back(profile);
		UserProfile = profile;
	}
	return success;
}

bool Monitor::SetDefaultSystemProfile(Profile * profile) {
	wchar_t registryKey[1024];
	int len = StringLength(L"\\Registry\\Machine\\");
	StringCbCopy(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	bool success = profile->EditRegistryProfileList(HKEY_LOCAL_MACHINE, registryKey, true);
	if (success) {
		ProfileList::iterator itEnd = SystemProfileList.end();
		for (ProfileList::iterator it = SystemProfileList.begin(); it != itEnd; ++it) {
			if ( *it == profile ) {
				SystemProfileList.erase(it);
				break;
			}
		}
		SystemProfileList.push_back(profile);
		SystemProfile = profile;
	}
	return success;
}

bool Monitor::AddUserProfileAssociation(Profile * profile) {
	wchar_t registryKey[1024];
	int len = StringLength(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
	StringCbCopy( registryKey, sizeof(registryKey),
		L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
	StringCbCat(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	bool success = profile->InsertIntoRegistryProfileList(HKEY_CURRENT_USER, registryKey);
	if (success) {
		AddProfileToInternalUserList(profile);
	}
	return success;
}

bool Monitor::AddSystemProfileAssociation(Profile * profile) {
	wchar_t registryKey[1024];
	int len = StringLength(L"\\Registry\\Machine\\");
	StringCbCopy(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	bool success = profile->InsertIntoRegistryProfileList(HKEY_LOCAL_MACHINE, registryKey);
	if (success) {
		AddProfileToInternalSystemList(profile);
	}
	return success;
}

bool Monitor::RemoveUserProfileAssociation(Profile * profile) {
	wchar_t registryKey[1024];
	int len = StringLength(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
	StringCbCopy( registryKey, sizeof(registryKey),
		L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
	StringCbCat(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	bool success = profile->EditRegistryProfileList(HKEY_CURRENT_USER, registryKey, false);
	if (success) {
		RemoveProfileFromInternalUserList(profile);
	}
	return success;
}

bool Monitor::RemoveSystemProfileAssociation(Profile * profile) {
	wchar_t registryKey[1024];
	int len = StringLength(L"\\Registry\\Machine\\");
	StringCbCopy(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	bool success = profile->EditRegistryProfileList(HKEY_LOCAL_MACHINE, registryKey, false);
	if (success) {
		RemoveProfileFromInternalSystemList(profile);
	}
	return success;
}

bool Monitor::SetActiveProfileIsUserProfile(bool userActive) {
	HKEY hKey;
	wchar_t registryKey[1024];
	int len = StringLength(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
	StringCbCopy( registryKey, sizeof(registryKey),
		L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
	StringCbCat(registryKey, sizeof(registryKey), &DeviceKey.c_str()[len]);
	bool success = false;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, registryKey, 0, KEY_SET_VALUE, &hKey)) {
		DWORD data = userActive ? 1 : 0;
		success = (ERROR_SUCCESS == RegSetValueEx(hKey, L"UsePerUserProfiles", NULL, REG_DWORD, reinterpret_cast<BYTE *>(&data), sizeof(data)));
		RegCloseKey(hKey);
	}
	if (success) {
		activeProfileIsUserProfile = userActive;
	}
	return success;
}

// Vector of monitors
//
static vector <Monitor *> * monitorList = 0;

// Add a monitor to the end of the list
//
Monitor * Monitor::Add(Monitor * monitor) {
	if ( 0 == monitorList ) {
		monitorList = new vector <Monitor *>;
	}
	monitorList->push_back(monitor);
	return monitor;
}

// Clear the list of monitors
//
void Monitor::ClearList(bool freeAllMemory) {
	if (monitorList) {
		size_t count = monitorList->size();
		for (size_t i = 0; i < count; ++i) {
			delete (*monitorList)[i];
		}
		if (freeAllMemory) {
			delete monitorList;
			monitorList = 0;
		} else {
			monitorList->clear();
		}
	}
}

// Return 'true' if this monitor is active and part of the desktop
//
bool Monitor::IsActive(const DISPLAY_DEVICEW & displayMonitor) {
	return (displayMonitor.StateFlags & DISPLAY_DEVICE_ACTIVE) && (displayMonitor.StateFlags & DISPLAY_DEVICE_ATTACHED);
}

void Monitor::SetMonitorPage(MonitorPage * pagePtr) {
	monitorPage = pagePtr;
}

MonitorPage * Monitor::GetMonitorPage(void) const {
	return monitorPage;
}

void Monitor::SetMonitorSummaryItem(MonitorSummaryItem * itemPtr) {
	monitorSummaryItem = itemPtr;
}

MonitorSummaryItem * Monitor::GetMonitorSummaryItem(void) const {
	return monitorSummaryItem;
}

bool Monitor::GetActiveProfileIsUserProfile(void) const {
	return activeProfileIsUserProfile;
}

Profile * Monitor::GetActiveProfile(void) const {
	return activeProfileIsUserProfile ? UserProfile : SystemProfile;
}

Profile * Monitor::GetUserProfile(void) const {
	return UserProfile;
}

Profile * Monitor::GetSystemProfile(void) const {
	return SystemProfile;
}

ProfileList & Monitor::GetProfileList(bool userProfiles) {
	return userProfiles ? UserProfileList : SystemProfileList;
}

wstring Monitor::GetDeviceString(void) const {
	return DeviceString;
}

Adapter * Monitor::GetAdapter(void) const {
	return adapter;
}

LUT * Monitor::GetLutPointer(void) const {
	return pLUT;
}

size_t Monitor::GetListSize(void) {
	if (monitorList) {
		return monitorList->size();
	} else {
		return 0;
	}
}

// Fetch a reference to a monitor by index number
//
Monitor * Monitor::Get(size_t index) {
	return (*monitorList)[index];
}

// Return a summary string
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
	s += L"\r\n";

	// See if the loaded LUT is correct
	//
	Profile * activeProfile = GetActiveProfile();
	if (activeProfile) {
		s += L"\r\n";
		activeProfile->LoadFullProfile(false);
		DWORD maxError = 0;
		DWORD totalError = 0;
		LUT_COMPARISON result = activeProfile->CompareLUT(pLUT, &maxError, &totalError);
		switch (result) {

			// LUTs match word for word
			//
			case LC_EQUAL:
				s += L"The current LUT matches the active profile\r\n";
				break;

			// One uses 0x0100, the other uses 0x0101 style
			//
			case LC_VARIATION_ON_LINEAR:
				s += L"The current LUT and the active profile are linear\r\n";
				break;

			// Low byte zeroed
			//
			case LC_TRUNCATION_IN_LOW_BYTE:
				s += L"The current LUT is similar to the active profile (truncation)\r\n";
				break;

			// Windows 7 rounds to nearest high byte
			//
			case LC_ROUNDING_IN_LOW_BYTE:
				s += L"The current LUT is similar to the active profile (rounding)\r\n";
				break;

			// Match, given the circumstances
			//
			case LC_PROFILE_HAS_NO_LUT_OTHER_LINEAR:
				s += L"The current LUT is linear and the active profile has no LUT\r\n";
				break;

			// Mismatch, other should be linear
			//
			case LC_PROFILE_HAS_NO_LUT_OTHER_NONLINEAR:
				if ( activeProfile->HasEmbeddedWcsProfile() ) {
					s += L"The active profile is a Windows Color System profile, and has no LUT\r\n";
				} else {
					s += L"The current LUT is not linear and the active profile has no LUT\r\n";
				}
				break;

			// LUTs do not meet any matching criteria
			//
			case LC_UNEQUAL:
				s += L"The current LUT does not match the active profile\r\n";
				break;
		}
	}

#if SHOW_GAMMA_RAMP_IN_SUMMARY_STRING

	// Display the start of the gamma ramp
	//
	s += L"\r\nStart of gamma ramp (first 10 entries):\r\n\tRed \t= ";
	for (int i = 0; i < COUNT_OF_GAMMA_VALUES_TO_SHOW; ++i) {
		wchar_t buf[10];
		StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->red[i]);
		s += buf;
	}
	s += L" \r\n\tGreen\t= ";
	for (int i = 0; i < COUNT_OF_GAMMA_VALUES_TO_SHOW; ++i) {
		wchar_t buf[10];
		StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->green[i]);
		s += buf;
	}
	s += L"\r\n\tBlue\t= ";
	for (int i = 0; i < COUNT_OF_GAMMA_VALUES_TO_SHOW; ++i) {
		wchar_t buf[10];
		StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->blue[i]);
		s += buf;
	}
	s += L"\r\n";

#endif

	return s;
}
