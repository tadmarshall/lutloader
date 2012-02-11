// TreeViewItem.cpp -- Class for holding TreeView items
//

#include "stdafx.h"
#include "Monitor.h"
#include "MonitorPage.h"
#include "TreeViewItem.h"

// Constructor
//
TreeViewItem::TreeViewItem(TREEVIEW_ITEM_TYPE itemType, const wchar_t * tviText) :
		ItemType(itemType),
		ItemText(tviText),
		MonitorPtr(0),
		ProfilePtr(0),
		hTreeItem(0)
{
}

void TreeViewItem::SetHTREEITEM(HTREEITEM hItem) {
	hTreeItem = hItem;
}

void TreeViewItem::SetMonitor(Monitor * monitor) {
	MonitorPtr = monitor;
}

void TreeViewItem::SetProfile(Profile * profile) {
	ProfilePtr = profile;
}

void TreeViewItem::Handle_TVN_SELCHANGEDW(MonitorPage * monitorPage, NMTREEVIEWW * pNMTREEVIEWW) {
	UNREFERENCED_PARAMETER(pNMTREEVIEWW);

	wstring s;
	switch (ItemType) {
		case TREEVIEW_ITEM_TYPE_NONE:
			s += L"TREEVIEW_ITEM_TYPE_NONE";
			break;

		case TREEVIEW_ITEM_TYPE_MONITOR:
			s += MonitorPtr->SummaryString();
			break;

		case TREEVIEW_ITEM_TYPE_USER_PROFILES:
			s += L"TREEVIEW_ITEM_TYPE_USER_PROFILES";
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES:
			s += L"TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES";
			break;

		case TREEVIEW_ITEM_TYPE_PROFILES:
			s += L"TREEVIEW_ITEM_TYPE_PROFILES";
			break;

		case TREEVIEW_ITEM_TYPE_PROFILE:
			s += ProfilePtr->DetailsString();
			break;
	}
	monitorPage->SetEditControlText(s);
}
