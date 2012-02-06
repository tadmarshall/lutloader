// Resize.cpp -- Handle moving and resizing windows when the user resizes the main window
//

#pragma once
#include "stdafx.h"
#include "Resize.h"
#include "Utility.h"
#include "resource.h"

// Constructor
//
Resize::Resize(HWND parent) :
		parentWindow(parent)
{
}

// Static (private) globals
//
static vector <Resize> resizeList;
static vector <ANCHOR_PRESET> achorPresetList;
static HWND topWindow;
static SIZE previousTopWindowSize;
static bool needRebuild = false;

// Get the parent window for a Resize object
//
HWND Resize::GetParentWindow(void) {
	return parentWindow;
}

// Add an child window of the resize list
//
void Resize::AddChildWindow(const RESIZECHILD & child) {
	childList.push_back(child);
}

// Client asks whether list of windows needs to be rebuilt
//
bool Resize::GetNeedRebuild(void) {
	return needRebuild;
}

// Client specifies whether list of windows needs to be rebuilt
//
void Resize::SetNeedRebuild(bool isRebuildNeeded) {
	needRebuild = isRebuildNeeded;
}

// Add an anchor preset
//
void Resize::AddAchorPreset(const ANCHOR_PRESET & anchorPreset) {
	achorPresetList.push_back(anchorPreset);
}

// Callback proc for EnumChildWindows
//
BOOL CALLBACK EnumChildCallback(HWND hwnd, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);

	HWND parent = GetParent(hwnd);
	vector<Resize>::iterator it = resizeList.begin();
	for (; it != resizeList.end(); ++it) {
		if (it->GetParentWindow() == parent) {
			break;
		}
	}

	if (it == resizeList.end()) {
		Resize * newList = new Resize(parent);
		resizeList.push_back(*newList);
		delete newList;
		it = resizeList.end() - 1;
	}
	WINDOWINFO wiParent;
	SecureZeroMemory(&wiParent, sizeof(wiParent));
	wiParent.cbSize = sizeof(wiParent);
	GetWindowInfo(parent, &wiParent);

	// Set up the child window in our list
	//
	RESIZECHILD c;
	c.hwnd = hwnd;
	GetWindowRect(hwnd, &c.rect);	
	c.rect.left -= wiParent.rcClient.left;
	c.rect.top -= wiParent.rcClient.top;
	c.rect.right -= wiParent.rcClient.left;
	c.rect.bottom -= wiParent.rcClient.top;

	// If the window is in the preset list, use the entry
	//
	size_t count = achorPresetList.size();
	size_t i;
	for (i = 0; i < count; ++i) {
		if (achorPresetList[i].hwnd == hwnd) {
			c.anchorLeft = achorPresetList[i].anchorLeft;
			c.anchorTop = achorPresetList[i].anchorTop;
			c.anchorRight = achorPresetList[i].anchorRight;
			c.anchorBottom = achorPresetList[i].anchorBottom;
			break;
		}
	}

	// If no preset, default to bottom+right anchoring
	//
	if (i == count) {
		c.anchorLeft = false;
		c.anchorTop = false;
		c.anchorRight = true;
		c.anchorBottom = true;
	}
	it->AddChildWindow(c);
	return 1;
}

// Rebuild the window list
//
void Resize::SetupForResizing(HWND parentBase) {
	resizeList.clear();
	topWindow = parentBase;
	RECT rect;
	GetWindowRect(topWindow, &rect);
	previousTopWindowSize.cx = rect.right - rect.left;
	previousTopWindowSize.cy = rect.bottom - rect.top;
	EnumChildWindows(topWindow, EnumChildCallback, 0);
}

void Resize::MainWindowHasResized(const WINDOWPOS & windowPos) {

	// Compute delta for sizing and moving
	//
	SIZE parentSizeDelta;
	parentSizeDelta.cx = windowPos.cx - previousTopWindowSize.cx;
	parentSizeDelta.cy = windowPos.cy - previousTopWindowSize.cy;
	previousTopWindowSize.cx = windowPos.cx;
	previousTopWindowSize.cy = windowPos.cy;

	// If SetupForResizing() has never been called, we aren't ready yet
	//
	if (!topWindow) {
		return;
	}
	vector<Resize>::iterator it = resizeList.begin();

	// Handle windows which are direct children of the top window
	//
	Resize * rs = &(*it);
	size_t count = rs->childList.size();
	SetLastError(0);
	HDWP hdwp = BeginDeferWindowPos(static_cast<int>(count));
	DWORD err = GetLastError();
	for (vector<RESIZECHILD>::iterator cl = it->childList.begin(); hdwp && !err && (cl != it->childList.end()); ++cl) {
		RESIZECHILD * rc = &(*cl);
		hdwp = DeferWindowPos( hdwp, rc->hwnd, 0,
				rc->rect.left + ( (rc->anchorRight && !rc->anchorLeft) ? parentSizeDelta.cx : 0 ),
				rc->rect.top  + ( (rc->anchorBottom && !rc->anchorTop) ? parentSizeDelta.cy : 0 ),
				rc->rect.right - rc->rect.left + ( (rc->anchorLeft && rc->anchorRight) ? parentSizeDelta.cx : 0 ),
				rc->rect.bottom - rc->rect.top + ( (rc->anchorTop && rc->anchorBottom) ? parentSizeDelta.cy : 0 ),
				SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER );
		err = GetLastError();
		if (!err) {
			rc->rect.left += ( (rc->anchorRight && !rc->anchorLeft) ? parentSizeDelta.cx : 0 );
			rc->rect.top  += ( (rc->anchorBottom && !rc->anchorTop) ? parentSizeDelta.cy : 0 );
			rc->rect.right  += rc->anchorRight  ? parentSizeDelta.cx : 0 ;
			rc->rect.bottom += rc->anchorBottom ? parentSizeDelta.cy : 0 ;
		}
		InvalidateRect(rc->hwnd, NULL, TRUE);
	}
	if (hdwp) {
		BOOL retVal = EndDeferWindowPos(hdwp);
		err = GetLastError();
		if ( err || (0 == retVal) ) {
			extern HINSTANCE g_hInst;
			wstring s = ShowError(L"EndDeferWindowPos");
			wchar_t errorMessageCaption[256];
			LoadString(g_hInst, IDS_ERROR, errorMessageCaption, _countof(errorMessageCaption));
			MessageBox(NULL, s.c_str(), errorMessageCaption, MB_ICONINFORMATION | MB_OK);
			InvalidateRect(topWindow, NULL, TRUE);
			return;
		}
	}

	// Handle windows which are children of children of the top window
	//
	for (++it; it != resizeList.end(); ++it) {
		rs = &(*it);
		count = rs->childList.size();
		SetLastError(0);
		hdwp = BeginDeferWindowPos(static_cast<int>(count));
		err = GetLastError();
		for (vector<RESIZECHILD>::iterator cl = it->childList.begin(); hdwp && !err && (cl != it->childList.end()); ++cl) {
			RESIZECHILD * rc = &(*cl);
			hdwp = DeferWindowPos( hdwp, rc->hwnd, 0,
					rc->rect.left,
					rc->rect.top,
					rc->rect.right - rc->rect.left + ( (rc->anchorLeft && rc->anchorRight) ? parentSizeDelta.cx : 0 ),
					rc->rect.bottom - rc->rect.top + ( (rc->anchorTop && rc->anchorBottom) ? parentSizeDelta.cy : 0 ),
					SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER );
			err = GetLastError();
			if (!err) {
				//rc->rect.left += ( (rc->anchorRight && !rc->anchorLeft) ? parentSizeDelta.cx : 0 );
				//rc->rect.top  += ( (rc->anchorBottom && !rc->anchorTop) ? parentSizeDelta.cy : 0 );
				rc->rect.right  += (rc->anchorLeft && rc->anchorRight) ? parentSizeDelta.cx : 0 ;
				rc->rect.bottom += (rc->anchorTop && rc->anchorBottom) ? parentSizeDelta.cy : 0 ;
			}
			InvalidateRect(rc->hwnd, NULL, TRUE);
		}
		if (hdwp) {
			BOOL retVal = EndDeferWindowPos(hdwp);
			err = GetLastError();
			if ( err || (0 == retVal) ) {
				extern HINSTANCE g_hInst;
				wstring s = ShowError(L"EndDeferWindowPos");
				wchar_t errorMessageCaption[256];
				LoadString(g_hInst, IDS_ERROR, errorMessageCaption, _countof(errorMessageCaption));
				MessageBox(NULL, s.c_str(), errorMessageCaption, MB_ICONINFORMATION | MB_OK);
			}
		}
	}
	InvalidateRect(topWindow, NULL, TRUE);
}

// Clear the list of Resize objects
//
void Resize::ClearResizeList(bool freeAllMemory) {
	resizeList.clear();
	if ( freeAllMemory && (resizeList.capacity() > 0) ) {
		vector <Resize> dummy;
		resizeList.swap(dummy);
	}
}

// Clear the list of ANCHOR_PRESET objects
//
void Resize::ClearAnchorPresetList(bool freeAllMemory) {
	achorPresetList.clear();
	if ( freeAllMemory && (achorPresetList.capacity() > 0) ) {
		vector <ANCHOR_PRESET> dummy;
		achorPresetList.swap(dummy);
	}
}
