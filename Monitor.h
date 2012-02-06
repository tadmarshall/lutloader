// Monitor.h -- Monitor class for handling display monitors returned by EnumDisplayDevices().
//

#pragma once
#include "stdafx.h"
#include "Profile.h"

class Monitor {

public:
	Monitor(int adapterNumber, const DISPLAY_DEVICEW * displayMonitor);
	Monitor(const Monitor & from);
	wstring SummaryString(void) const;
	wstring DetailsString(void) const;

	static bool IsMonitorActive(const DISPLAY_DEVICEW * displayMonitor);
	static void ClearMonitorList(bool freeAllMemory);
	static void AddMonitor(const Monitor & monitor);
	static size_t GetMonitorListSize(void);
	static Monitor & GetMonitor(size_t monitorNumber);
	static wstring GetMonitorDeviceString(size_t monitorNumber);

private:
	wstring		DeviceName;
	wstring		DeviceString;
	DWORD		StateFlags;
	wstring		DeviceID;
	wstring		DeviceKey;
	Profile		UserProfile;
	Profile		SystemProfile;
	bool		activeProfileIsUserProfile;
	int			AdapterNumber;
};
