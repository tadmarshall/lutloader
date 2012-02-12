// MonitorPage.h -- Display a property sheet page for each monitor
//

#pragma once
#include "stdafx.h"
#include <commctrl.h>		// For HTREEITEM

// Forward references
//
class Monitor;
class TreeViewItem;

class MonitorPage {

public:
	MonitorPage(Monitor * hostMonitor);
	~MonitorPage();

	HWND GetHwnd(void) const;
	HWND GetTreeViewHwnd(void) const;
	void RequestTreeViewWidth(int);
	void GetTreeViewNodeExpansionString(wchar_t * str, __in_bcount(str) size_t strSize);
	void SetTreeViewNodeExpansionString(wchar_t * str);
	void SetEditControlText(wstring newText);
	Monitor * GetMonitor(void) const;
	void Reset(void);

	static INT_PTR CALLBACK MonitorPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	void BuildTreeView(void);
	TreeViewItem * AddTreeViewItem(TreeViewItem * treeViewItem);
	void ClearTreeViewItemList(bool freeAllMemory);

	Monitor *					monitor;
	HWND						hwnd;
	HWND						hwndTreeView;
	HWND						hwndEdit;
	vector <TreeViewItem *>		treeviewitemList;
	HTREEITEM					tvRoot;
	HTREEITEM					tvUserProfiles;
	HTREEITEM					tvSystemProfiles;
	int							requestedTreeViewWidth;
	bool						requestedRootExpansion;
	bool						requestedUserProfilesExpansion;
	bool						requestedSystemProfilesExpansion;
	bool						resetting;
};
