// TreeViewItem.cpp -- Class for holding TreeView items
//

#include "stdafx.h"
#include "Monitor.h"
#include "MonitorPage.h"
#include "MonitorSummaryItem.h"
#include "resource.h"
#include "TreeViewItem.h"
//#include "Utility.h"
//#include <banned.h>

// Symbols defined in other files
//
extern HINSTANCE g_hInst;

// Constructor
//
TreeViewItem::TreeViewItem(TREEVIEW_ITEM_TYPE itemType) :
		ItemType(itemType),
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

int TreeViewItem::GetItemType(void) const {
	return ItemType;
}

Profile * TreeViewItem::GetProfilePtr(void) const {
	return ProfilePtr;
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

		case TREEVIEW_ITEM_TYPE_USER_PROFILE:
			s += ProfilePtr->DetailsString();
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE:
			s += ProfilePtr->DetailsString();
			break;
	}
	monitorPage->SetEditControlText(s);
}

void TreeViewItem::Handle_WM_CONTEXTMENU(MonitorPage * monitorPage, POINT * screenClickPoint) {

	switch (ItemType) {
		case TREEVIEW_ITEM_TYPE_NONE:
			break;

		case TREEVIEW_ITEM_TYPE_MONITOR:
			break;

		case TREEVIEW_ITEM_TYPE_USER_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_USER_PROFILE:
			ProfileContextMenu(monitorPage, screenClickPoint, true);
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE:
			ProfileContextMenu(monitorPage, screenClickPoint, false);
			break;
	}
}

void TreeViewItem::ProfileContextMenu(MonitorPage * monitorPage, POINT * screenClickPoint, bool isUser) {
	int flags;
	HMENU hPopup;
	MENUINFO menuInfo;
	MENUITEMINFO menuItemInfo;
	int id;
	LUT * pProfileLUT;
	Monitor * monitor;
	MonitorSummaryItem * monitorSummaryItem;
	Profile * profile;
	Profile * userProfile;
	Profile * systemProfile;

	hPopup = CreatePopupMenu();

	SecureZeroMemory(&menuInfo, sizeof(menuInfo));
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_BACKGROUND;
	menuInfo.hbrBack = (HBRUSH)GetStockObject(WHITE_BRUSH);
	SetMenuInfo(hPopup, &menuInfo);

	SecureZeroMemory(&menuItemInfo, sizeof(menuItemInfo));
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	monitor = monitorPage->GetMonitor();
	userProfile = monitor->GetUserProfile();
	systemProfile = monitor->GetSystemProfile();
	profile = isUser ? userProfile : systemProfile;
	if ( isUser == monitor->GetActiveProfileIsUserProfile() ) {
		menuItemInfo.dwTypeData = L"Set as default profile";
	} else {
		menuItemInfo.dwTypeData = isUser ? L"Set as default user profile (inactive)" : L"Set as default system profile (inactive)";
	}
	ProfilePtr->LoadFullProfile(false);
	menuItemInfo.wID = ID_SET_DEFAULT_PROFILE;
	if ( (isUser && (ProfilePtr == userProfile)) || (!isUser && (ProfilePtr == systemProfile)) ) {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
		menuItemInfo.fState = MFS_DISABLED | MFS_CHECKED;
	} else if ( !ProfilePtr->GetErrorString().empty() ) {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
		menuItemInfo.fState = MFS_DISABLED;
	} else {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
	}
	InsertMenuItem(hPopup, 0, TRUE, &menuItemInfo);

	menuItemInfo.wID = ID_REMOVE_ASSOCIATION;
	if ( (isUser && (ProfilePtr == userProfile)) || (!isUser && (ProfilePtr == systemProfile)) ) {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
		menuItemInfo.fState = MFS_DISABLED;
	} else {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
	}
	menuItemInfo.dwTypeData = L"Remove association";
	InsertMenuItem(hPopup, 1, TRUE, &menuItemInfo);

	HWND hwndTreeView = monitorPage->GetTreeViewHwnd();
	SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hTreeItem));
	flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
	id = TrackPopupMenuEx(hPopup, flags, screenClickPoint->x, screenClickPoint->y, monitorPage->GetHwnd(), NULL);

	if ( ID_SET_DEFAULT_PROFILE == id ) {
		bool success = monitor->SetDefaultProfile(ProfilePtr, isUser);
		if ( isUser == monitor->GetActiveProfileIsUserProfile() ) {
			pProfileLUT = ProfilePtr->GetLutPointer();
			if ( pProfileLUT ) {
				monitor->WriteLutToCard(pProfileLUT);
			} else {
				LUT * pLUT = new LUT;
				GetSignedLUT(pLUT);
				monitor->WriteLutToCard(pLUT);
				delete pLUT;
			}
			monitor->ReadLutFromCard();
			monitorSummaryItem = monitor->GetMonitorSummaryItem();
			if (monitorSummaryItem) {
				monitorSummaryItem->Update();
			}
		}

		// If we don't have Administrator privileges and we hit the system profile,
		// then we didn't actually change the setting (we just loaded the LUT)
		//
		if (success) {

			// Unbold the currently bold item, make ourselves bold
			//
			HTREEITEM hParent = reinterpret_cast<HTREEITEM>(
					SendMessage(
							hwndTreeView,
							TVM_GETNEXTITEM,
							TVGN_PARENT,
							reinterpret_cast<LPARAM>(hTreeItem) ) );
			HTREEITEM hChild = reinterpret_cast<HTREEITEM>(
					SendMessage(
							hwndTreeView,
							TVM_GETNEXTITEM,
							TVGN_CHILD,
							reinterpret_cast<LPARAM>(hParent) ) );
			TVITEMEX itemEx;
			SecureZeroMemory(&itemEx, sizeof(itemEx));
			itemEx.mask = TVIF_HANDLE | TVIF_STATE;
			while (hChild) {
				if ( hChild != hTreeItem ) {
					itemEx.hItem = hChild;
					itemEx.state = TVIS_BOLD;
					itemEx.stateMask = TVIS_BOLD;
					SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
					if ( 0 != (itemEx.state & TVIS_BOLD) ) {
						itemEx.state = 0;
						itemEx.stateMask = TVIS_BOLD;
						SendMessage(hwndTreeView, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
					}
				}
				hChild = reinterpret_cast<HTREEITEM>(
						SendMessage(
								hwndTreeView,
								TVM_GETNEXTITEM,
								TVGN_NEXT,
								reinterpret_cast<LPARAM>(hChild) ) );
			}
			itemEx.hItem = hTreeItem;
			itemEx.state = TVIS_BOLD;
			itemEx.stateMask = TVIS_BOLD;
			SendMessage(hwndTreeView, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		}
	} else 	if ( ID_REMOVE_ASSOCIATION == id ) {
		bool success = monitor->RemoveProfileAssociation(ProfilePtr, isUser);

		// If we don't have Administrator privileges and we hit the system profile,
		// then we didn't actually remove this association
		//
		if (success) {

			// Delete this node from the TreeView
			//
			SendMessage(hwndTreeView, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(hTreeItem));
		}
	}
	DestroyMenu(hPopup);
}
