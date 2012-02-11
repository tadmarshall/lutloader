// Monitor.h -- Monitor class for handling display monitors returned by EnumDisplayDevices()
//

#pragma once
#include "stdafx.h"
#include "Profile.h"

// Forward references
//
class Adapter;
class MonitorPage;
class MonitorSummaryItem;

class Monitor {

public:
	Monitor(Adapter * hostAdapter, const DISPLAY_DEVICEW & displayMonitor);
	~Monitor();

	void Initialize(void);

	static Monitor * Add(Monitor * monitor);
	static void ClearList(bool freeAllMemory);

	void SetMonitorPage(MonitorPage * pagePtr);
	MonitorPage * GetMonitorPage(void);
	void SetMonitorSummaryItem(MonitorSummaryItem * itemPtr);
	MonitorSummaryItem * GetMonitorSummaryItem(void);
	wstring SummaryString(void) const;
	bool GetActiveProfileIsUserProfile(void) const;
	Profile * GetActiveProfile(void) const;
	Profile * GetUserProfile(void) const;
	Profile * GetSystemProfile(void) const;
	ProfileList & GetProfileList(bool userProfiles);
	wstring GetDeviceString(void) const;
	Adapter * GetAdapter(void) const;
	LUT * GetLutPointer(void) const;
	bool ReadLutFromCard(void);
	bool WriteLutToCard(LUT * lutToWriteToAdapter);

	static bool IsActive(const DISPLAY_DEVICEW & displayMonitor);
	static size_t GetListSize(void);
	static Monitor * Get(size_t index);

private:
	wstring					DeviceName;
	wstring					DeviceString;
	DWORD					StateFlags;
	wstring					DeviceID;
	wstring					DeviceKey;
	Adapter *				adapter;
	MonitorPage *			monitorPage;
	MonitorSummaryItem *	monitorSummaryItem;
	LUT *					pLUT;
	Profile *				UserProfile;
	ProfileList				UserProfileList;
	Profile *				SystemProfile;
	ProfileList				SystemProfileList;
	bool					activeProfileIsUserProfile;
};
