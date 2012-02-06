// MonitorPage.h -- Display a property sheet page for each monitor
//

#pragma once
#include "stdafx.h"
#include <commctrl.h>
#include "Monitor.h"

class MonitorPage {

public:
	MonitorPage(Monitor * hostMonitor);

	void SetEditControlText(wstring newText);
	Monitor * GetMonitor(void) const;
	void BuildTreeView(HWND treeControlHwnd);

	static INT_PTR CALLBACK MonitorPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	Monitor *			monitor;
	HWND				savedHWND;
	HTREEITEM			tvRoot;
	HTREEITEM			tvUserProfiles;
	HTREEITEM			tvSystemProfiles;
};

#define MINIMUM_TREEVIEW_WIDTH (120)		// Minimum TreeView control width for splitter
#define MINIMUM_EDIT_WIDTH (120)			// Minimum Edit control width for splitter
