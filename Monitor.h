// Monitor.h -- Monitor class for handling display monitors returned by EnumDisplayDevices()
//

#pragma once
#include "stdafx.h"
#include "Adapter.h"
#include "Profile.h"

class Monitor {

public:
	Monitor(Adapter * hostAdapter, const DISPLAY_DEVICEW & displayMonitor);
	~Monitor();

	void Initialize(void);

	static Monitor * Add(Monitor * monitor);
	static void ClearList(bool freeAllMemory);

	wstring SummaryString(void) const;
	wstring DetailsString(void) const;
	bool GetActiveProfileIsUserProfile(void) const;
	Profile * GetActiveProfile(void) const;
	Profile * GetUserProfile(void) const;
	Profile * GetSystemProfile(void) const;
	ProfileList & GetProfileList(bool userProfiles);
	wstring GetDeviceString(void) const;
	Adapter * GetAdapter(void);
	LUT * GetLutPointer(void) const;
	bool ReadLutFromCard(void);
	bool WriteLutToCard(LUT * lutToWriteToWriteToAdapter);

	static bool IsActive(const DISPLAY_DEVICEW & displayMonitor);
	static size_t GetListSize(void);
	static Monitor * Get(size_t index);

private:
	wstring				DeviceName;
	wstring				DeviceString;
	DWORD				StateFlags;
	wstring				DeviceID;
	wstring				DeviceKey;
	Adapter *			adapter;
	LUT *				pLUT;
	Profile *			UserProfile;
	ProfileList			UserProfileList;
	Profile *			SystemProfile;
	ProfileList			SystemProfileList;
	bool				activeProfileIsUserProfile;
};
