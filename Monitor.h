// Monitor.h -- Monitor class for handling display monitors returned by EnumDisplayDevices().
//

#pragma once
#include "stdafx.h"
#include "Adapter.h"

class Monitor {

public:
	Monitor(int adapterNumber, PDISPLAY_DEVICE displayMonitor);
	wstring SummaryString(void) const;

	static bool IsMonitorActive(PDISPLAY_DEVICE displayMonitor);
	static void AddMonitor(Monitor monitor);
	static size_t GetMonitorListSize(void);
	static Monitor & GetMonitor(size_t monitorNumber);
	static wstring GetMonitorDeviceString(size_t monitorNumber);

private:
	static wchar_t * GetProfileName(HKEY hKeyBase, wchar_t * registryKey, BOOL * perUser);
	int		AdapterNumber;
	DWORD	StateFlags;
	wstring DeviceName;
	wstring DeviceString;
	wstring DeviceID;
	wstring DeviceKey;
	wstring userProfile;
	wstring systemProfile;
	bool	activeProfileIsUserProfile;
};

