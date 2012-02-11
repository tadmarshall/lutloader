// MonitorPage.h -- Display a property sheet page for each monitor
//

#pragma once
#include "stdafx.h"
#include <commctrl.h>

// Forward references
//
class Monitor;
class TreeViewItem;

class MonitorPage {

public:
	MonitorPage(Monitor * hostMonitor);
	~MonitorPage();

	void SetEditControlText(wstring newText);
	Monitor * GetMonitor(void) const;
	void Reset(void);

	static INT_PTR CALLBACK MonitorPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	void BuildTreeView(HWND treeControlHwnd);
	TreeViewItem * AddTreeViewItem(TreeViewItem * treeViewItem);
	void ClearTreeViewItemList(bool freeAllMemory);

	Monitor *					monitor;
	HWND						savedHWND;
	vector <TreeViewItem *>		treeviewitemList;
	HTREEITEM					tvRoot;
	HTREEITEM					tvUserProfiles;
	HTREEITEM					tvSystemProfiles;
	bool						resetting;
};
