// Adapter.h -- Adapter class for handling display adapters returned by EnumDisplayDevices().
//

#pragma once
#include "stdafx.h"

class Adapter {

public:
	Adapter(const DISPLAY_DEVICEW & displayAdapter);

	static bool IsAdapterActive(const DISPLAY_DEVICEW * displayAdapter);
	static void ClearAdapterList(bool freeAllMemory);
	static void AddAdapter(const Adapter & adapter);
	static DWORD GetAdapterStateFlags(int adapterNumber);
	static wstring GetAdapterDeviceName(int adapterNumber);

private:
	wstring DeviceName;
	wstring DeviceString;
	DWORD	StateFlags;
	wstring DeviceID;
	wstring DeviceKey;
};
