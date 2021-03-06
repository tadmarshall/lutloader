// Resize.cpp -- Handle moving and resizing windows when the user resizes the main window
//

#pragma once
#include "stdafx.h"
#include "Resize.h"
#include "Utility.h"
#include "resource.h"
//#include <banned.h>

// Constructor
//
Resize::Resize(HWND parent) :
		parentWindow(parent)
{
}

// Vector of Resize objects
//
static vector <Resize> * resizeList = 0;

// Vector of anchor presets
//
static vector <ANCHOR_PRESET> * anchorPresetList = 0;

// Top window in our tree (the PropertySheet's main window)
//
static HWND topWindow = 0;

// The size of the top window the last time its size changed
//
static SIZE previousTopWindowSize = {0};

// We need to rebuild the list of child windows on the next resize
//
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
void Resize::AddAnchorPreset(const ANCHOR_PRESET & anchorPreset) {
	if ( 0 == anchorPresetList ) {
		anchorPresetList = new vector <ANCHOR_PRESET>;
	}
	anchorPresetList->push_back(anchorPreset);
}

// Callback procedure for EnumChildWindows
//
BOOL CALLBACK EnumChildCallback(HWND hwnd, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);

	HWND parent = GetParent(hwnd);
	vector<Resize>::iterator it = resizeList->begin();
	vector<Resize>::iterator itEnd = resizeList->end();
	for (; it != itEnd; ++it) {
		if (it->GetParentWindow() == parent) {
			break;
		}
	}

	if (it == itEnd) {
		Resize * newList = new Resize(parent);
		resizeList->push_back(*newList);
		delete newList;
		it = resizeList->end() - 1;
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
	size_t count = 0;
	size_t i = 0;
	if (anchorPresetList) {
		count = anchorPresetList->size();
		for (; i < count; ++i) {
			if ((*anchorPresetList)[i].hwnd == hwnd) {
				c.anchorLeft = (*anchorPresetList)[i].anchorLeft;
				c.anchorTop = (*anchorPresetList)[i].anchorTop;
				c.anchorRight = (*anchorPresetList)[i].anchorRight;
				c.anchorBottom = (*anchorPresetList)[i].anchorBottom;
				break;
			}
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
	if (resizeList) {
		resizeList->clear();
	} else {
		resizeList = new vector <Resize>;
	}
	topWindow = parentBase;
	RECT rect;
	GetWindowRect(topWindow, &rect);
	previousTopWindowSize.cx = rect.right - rect.left;
	previousTopWindowSize.cy = rect.bottom - rect.top;
	EnumChildWindows(topWindow, EnumChildCallback, 0);
}

void Resize::MainWindowHasResized(const WINDOWPOS & windowPos) {

	RECT nr;

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
	if (resizeList) {
		vector<Resize>::iterator it = resizeList->begin();

		// Handle windows which are direct children of the top window
		//
		Resize * rs = &(*it);
		size_t count = rs->childList.size();
		SetLastError(0);
		HDWP hdwp = BeginDeferWindowPos(static_cast<int>(count));
		DWORD err = GetLastError();
		vector<RESIZECHILD>::iterator clEnd = it->childList.end();
		if (hdwp) {
			for (vector<RESIZECHILD>::iterator cl = it->childList.begin(); hdwp && !err && (cl != clEnd); ++cl) {
				RESIZECHILD * rc = &(*cl);
				nr.left = rc->rect.left + ( (rc->anchorRight && !rc->anchorLeft) ? parentSizeDelta.cx : 0 );
				nr.top  = rc->rect.top  + ( (rc->anchorBottom && !rc->anchorTop) ? parentSizeDelta.cy : 0 );
				nr.right  = rc->rect.right  + (rc->anchorRight  ? parentSizeDelta.cx : 0 );
				nr.bottom = rc->rect.bottom + (rc->anchorBottom ? parentSizeDelta.cy : 0 );
				if ( !EqualRect(&nr, &rc->rect) ) {
					hdwp = DeferWindowPos( hdwp, rc->hwnd, 0,
							rc->rect.left + ( (rc->anchorRight && !rc->anchorLeft) ? parentSizeDelta.cx : 0 ),
							rc->rect.top  + ( (rc->anchorBottom && !rc->anchorTop) ? parentSizeDelta.cy : 0 ),
							rc->rect.right - rc->rect.left + ( (rc->anchorLeft && rc->anchorRight) ? parentSizeDelta.cx : 0 ),
							rc->rect.bottom - rc->rect.top + ( (rc->anchorTop && rc->anchorBottom) ? parentSizeDelta.cy : 0 ),
							SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER );
					err = GetLastError();
					if (!err) {
						rc->rect = nr;
					}
				}
			}
		}
		if (hdwp) {
			EndDeferWindowPos(hdwp);
		}

		// Handle windows which are children of children of the top window
		//
		vector<Resize>::iterator itEnd = resizeList->end();
		for (++it; it != itEnd; ++it) {
			rs = &(*it);
			count = rs->childList.size();
			SetLastError(0);
			hdwp = BeginDeferWindowPos(static_cast<int>(count));
			err = GetLastError();
			if (hdwp) {
				clEnd = it->childList.end();
				for (vector<RESIZECHILD>::iterator cl = it->childList.begin(); hdwp && !err && (cl != clEnd); ++cl) {
					RESIZECHILD * rc = &(*cl);
					nr.left = rc->rect.left + ( (rc->anchorRight && !rc->anchorLeft) ? parentSizeDelta.cx : 0 );
					nr.top  = rc->rect.top  + ( (rc->anchorBottom && !rc->anchorTop) ? parentSizeDelta.cy : 0 );
					nr.right  = rc->rect.right  + ( rc->anchorRight  ? parentSizeDelta.cx : 0 );
					nr.bottom = rc->rect.bottom + ( rc->anchorBottom ? parentSizeDelta.cy : 0 );
					if ( !EqualRect(&nr, &rc->rect) ) {
						hdwp = DeferWindowPos( hdwp, rc->hwnd, 0,
								rc->rect.left + ( (rc->anchorRight && !rc->anchorLeft) ? parentSizeDelta.cx : 0 ),
								rc->rect.top  + ( (rc->anchorBottom && !rc->anchorTop) ? parentSizeDelta.cy : 0 ),
								rc->rect.right - rc->rect.left + ( (rc->anchorLeft && rc->anchorRight) ? parentSizeDelta.cx : 0 ),
								rc->rect.bottom - rc->rect.top + ( (rc->anchorTop && rc->anchorBottom) ? parentSizeDelta.cy : 0 ),
								SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER );
						err = GetLastError();
						if (!err) {
							rc->rect = nr;
						}
					}
				}
			}
			if (hdwp) {
				EndDeferWindowPos(hdwp);
			}
		}
	}
}

// Clear the list of Resize objects
//
void Resize::ClearResizeList(bool freeAllMemory) {
	if (resizeList) {
		if (freeAllMemory) {
			delete resizeList;
			resizeList = 0;
		} else {
			resizeList->clear();
		}
	}
}

// Clear the list of ANCHOR_PRESET objects
//
void Resize::ClearAnchorPresetList(bool freeAllMemory) {
	if (anchorPresetList) {
		if (freeAllMemory) {
			delete anchorPresetList;
			anchorPresetList = 0;
		} else {
			anchorPresetList->clear();
		}
	}
}
