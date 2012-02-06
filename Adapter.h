// Adapter.h -- Adapter class for handling display adapters returned by EnumDisplayDevices()
//

#pragma once
#include "stdafx.h"

class Adapter {

public:
	Adapter(const DISPLAY_DEVICEW & displayAdapter);

	static Adapter * Add(Adapter * adapter);
	static void ClearList(bool freeAllMemory);

	DWORD GetStateFlags(void);
	wstring GetDeviceName(void);

	static bool IsActive(const DISPLAY_DEVICEW * displayAdapter);
	static size_t GetListSize(void);

private:
	wstring				DeviceName;
	wstring				DeviceString;
	DWORD				StateFlags;
	wstring				DeviceID;
	wstring				DeviceKey;
};
