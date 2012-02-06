// Adapter.h -- Adapter class for handling display adapters returned by EnumDisplayDevices().
//

#pragma once
#include "stdafx.h"

class Adapter {

public:
	Adapter(PDISPLAY_DEVICE displayAdapter);

	static bool IsAdapterActive(PDISPLAY_DEVICE displayAdapter);
	static void ClearAdapterList(bool freeAllMemory);
	static void AddAdapter(Adapter * const adapter);
	static DWORD GetAdapterStateFlags(int adapterNumber);
	static wstring GetAdapterDeviceName(int adapterNumber);

private:
	wstring DeviceName;
	DWORD	StateFlags;
	wstring DeviceString;
	wstring DeviceID;
	wstring DeviceKey;
};
