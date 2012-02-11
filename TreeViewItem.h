// TreeViewItem.h -- Class for holding TreeView items
//

#pragma once
#include "stdafx.h"
#include <commctrl.h>

// Forward references
//
class Monitor;
class MonitorPage;
class Profile;

// TreeViewItem type enumeration, used to figure out what to do with something when it's selected
//
typedef enum tag_TREEVIEW_ITEM_TYPE {
	TREEVIEW_ITEM_TYPE_NONE = 0,				// Uninitialized or do-nothing entry
	TREEVIEW_ITEM_TYPE_MONITOR = 1,				// The monitor (subject of everything in this TreeView instance)
	TREEVIEW_ITEM_TYPE_USER_PROFILES = 2,		// "User profiles" node, for Vista and later
	TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES = 3,		// "System profiles" node, for Vista and later
	TREEVIEW_ITEM_TYPE_PROFILES = 4,			// "Profiles" node, for Windows XP
	TREEVIEW_ITEM_TYPE_PROFILE = 5,				// A named profile file
} TREEVIEW_ITEM_TYPE;

class TreeViewItem {

public:
	TreeViewItem(TREEVIEW_ITEM_TYPE itemType, const wchar_t * tviText);

	void SetHTREEITEM(HTREEITEM hItem);
	void SetMonitor(Monitor * monitor);
	void SetProfile(Profile * profile);
	void Handle_TVN_SELCHANGEDW(MonitorPage * monitorPage, NMTREEVIEWW * pNMTREEVIEWW);

private:
	wstring				ItemText;
	int					ItemType;
	Monitor *			MonitorPtr;
	Profile *			ProfilePtr;
	HTREEITEM			hTreeItem;
};
