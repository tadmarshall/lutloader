// Adapter.cpp -- Adapter class for handling display adapters returned by EnumDisplayDevices().
//

#include "stdafx.h"
#include "Adapter.h"

// Constructor
//
Adapter::Adapter(PDISPLAY_DEVICE displayAdapter) {
	StateFlags = displayAdapter->StateFlags;
	DeviceName = displayAdapter->DeviceName;
	DeviceString = displayAdapter->DeviceString;
	DeviceID = displayAdapter->DeviceID;
	DeviceKey = displayAdapter->DeviceKey;
}

// Return 'true' if this adapter is active and part of the desktop
//
bool Adapter::IsAdapterActive(PDISPLAY_DEVICE displayAdapter) {
	return (displayAdapter->StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP);
}

// Vector of adapters
//
static vector <Adapter> adapterList;

// Clear the list of adapters
//
void Adapter::ClearAdapterList(bool freeAllMemory) {
	adapterList.clear();
	if ( freeAllMemory && (adapterList.capacity() > 0) ) {
		vector <Adapter> dummy;
		adapterList.swap(dummy);
	}
}

// Add an adapter to the end of the list
//
void Adapter::AddAdapter(Adapter * const adapter) {
	adapterList.push_back(*adapter);
}

// Fetch an adapter's StateFlags by index number
//
DWORD Adapter::GetAdapterStateFlags(int adapterNumber) {
	return adapterList[adapterNumber].StateFlags;
}

// Fetch an adapter's DeviceName by index number
//
wstring Adapter::GetAdapterDeviceName(int adapterNumber) {
	return adapterList[adapterNumber].DeviceName;
}
