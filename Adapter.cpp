// Adapter.cpp -- Adapter class for handling display adapters returned by EnumDisplayDevices()
//

#include "stdafx.h"
#include "Adapter.h"
//#include <banned.h>

// Constructor
//
Adapter::Adapter(const DISPLAY_DEVICEW & displayAdapter) :
		DeviceName(displayAdapter.DeviceName),
		DeviceString(displayAdapter.DeviceString),
		StateFlags(displayAdapter.StateFlags),
		DeviceID(displayAdapter.DeviceID),
		DeviceKey(displayAdapter.DeviceKey)
{
}

// Vector of adapters
//
static vector <Adapter *> * adapterList = 0;

// Add an adapter to the end of the list
//
Adapter * Adapter::Add(Adapter * adapter) {
	if ( 0 == adapterList ) {
		adapterList = new vector <Adapter *>;
	}
	adapterList->push_back(adapter);
	return adapter;
}

// Clear the list of adapters
//
void Adapter::ClearList(bool freeAllMemory) {
	if (adapterList) {
		size_t count = adapterList->size();
		for (size_t i = 0; i < count; ++i) {
			delete (*adapterList)[i];
		}
		if (freeAllMemory) {
			delete adapterList;
			adapterList = 0;
		} else {
			adapterList->clear();
		}
	}
}

// Return 'true' if this adapter is active and part of the desktop
//
bool Adapter::IsActive(const DISPLAY_DEVICEW * displayAdapter) {
	return (displayAdapter->StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP);
}

// Return the size of the adapter list
//
size_t Adapter::GetListSize(void) {
	if (adapterList) {
		return adapterList->size();
	} else {
		return 0;
	}
}

// Fetch an adapter's StateFlags
//
DWORD Adapter::GetStateFlags(void) {
	return StateFlags;
}

// Fetch an adapter's DeviceName
//
wstring Adapter::GetDeviceName(void) {
	return DeviceName;
}
