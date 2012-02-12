// TreeViewItem.cpp -- Class for holding TreeView items
//

#include "stdafx.h"
#include "Monitor.h"
#include "MonitorPage.h"
#include "MonitorSummaryItem.h"
#include "resource.h"
#include "TreeViewItem.h"
#include "Utility.h"
#include <strsafe.h>
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

TREEVIEW_ITEM_TYPE TreeViewItem::GetItemType(void) const {
	return ItemType;
}

Profile * TreeViewItem::GetProfilePtr(void) const {
	return ProfilePtr;
}

HTREEITEM TreeViewItem::Get_HTREEITEM_ForProfile(HWND hwndTreeView, HTREEITEM subTree, Profile * profile) {
	TVITEMEX itemEx;
	SecureZeroMemory(&itemEx, sizeof(itemEx));
	HTREEITEM hChild = reinterpret_cast<HTREEITEM>(
			SendMessage(hwndTreeView, TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(subTree)) );
	itemEx.mask = TVIF_PARAM;
	while (hChild) {
		itemEx.hItem = hChild;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		TreeViewItem * tvItem = reinterpret_cast<TreeViewItem *>(itemEx.lParam);
		if ( tvItem && (tvItem->GetProfilePtr() == profile) ) {
			break;
		}
		hChild = reinterpret_cast<HTREEITEM>(
				SendMessage(hwndTreeView, TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(hChild)) );
	}
	return hChild;
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

		case TREEVIEW_ITEM_TYPE_OTHER_PROFILES:
			s += L"TREEVIEW_ITEM_TYPE_OTHER_PROFILES";
			break;

		case TREEVIEW_ITEM_TYPE_OTHER_PROFILE:
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
			ProfileListContextMenu(monitorPage, screenClickPoint);
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES:
			ProfileListContextMenu(monitorPage, screenClickPoint);
			break;

		case TREEVIEW_ITEM_TYPE_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_USER_PROFILE:
			ProfileContextMenu(monitorPage, screenClickPoint, true);
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE:
			ProfileContextMenu(monitorPage, screenClickPoint, false);
			break;

		case TREEVIEW_ITEM_TYPE_OTHER_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_OTHER_PROFILE:
			OtherProfileContextMenu(monitorPage, screenClickPoint);
			break;

	}
}

// Display a context menu for the User profiles or System profiles subtree
//
void TreeViewItem::ProfileListContextMenu(MonitorPage * monitorPage, POINT * screenClickPoint) {
	HMENU hPopup;
	MENUINFO menuInfo;
	MENUITEMINFO menuItemInfo;
	TVITEMEX itemEx;
	bool success = false;

	hPopup = CreatePopupMenu();

	SecureZeroMemory(&menuInfo, sizeof(menuInfo));
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_BACKGROUND;
	menuInfo.hbrBack = (HBRUSH)GetStockObject(WHITE_BRUSH);
	SetMenuInfo(hPopup, &menuInfo);

	SecureZeroMemory(&menuItemInfo, sizeof(menuItemInfo));
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	Monitor * monitor = monitorPage->GetMonitor();

	menuItemInfo.dwTypeData = L"User profiles active";
	menuItemInfo.wID = ID_USER_PROFILES_ACTIVE;
	if (monitor->GetActiveProfileIsUserProfile()) {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
		menuItemInfo.fState = MFS_DISABLED | MFS_CHECKED;
	} else {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
	}
	InsertMenuItem(hPopup, 0, TRUE, &menuItemInfo);

	menuItemInfo.dwTypeData = L"System profiles active";
	menuItemInfo.wID = ID_SYSTEM_PROFILES_ACTIVE;
	if (!monitor->GetActiveProfileIsUserProfile()) {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
		menuItemInfo.fState = MFS_DISABLED | MFS_CHECKED;
	} else {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
	}
	InsertMenuItem(hPopup, 1, TRUE, &menuItemInfo);

	HWND hwndTreeView = monitorPage->GetTreeViewHwnd();
	SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hTreeItem));
	int flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
	int id = TrackPopupMenuEx(hPopup, flags, screenClickPoint->x, screenClickPoint->y, monitorPage->GetHwnd(), NULL);

	bool changeSetting = false;
	bool setUser = false;
	Profile * profile = monitor->GetSystemProfile();
	HTREEITEM oldItem = monitorPage->GetUserHTREEITEM();
	HTREEITEM newItem = monitorPage->GetSystemHTREEITEM();
	if ( ID_USER_PROFILES_ACTIVE == id ) {
		changeSetting = true;
		setUser = true;
		profile = monitor->GetUserProfile();
		oldItem = monitorPage->GetSystemHTREEITEM();
		newItem = monitorPage->GetUserHTREEITEM();
	} else if ( ID_SYSTEM_PROFILES_ACTIVE == id ) {
		changeSetting = true;
	}

	if (changeSetting) {
		success = monitor->SetActiveProfileIsUserProfile(setUser);
		if (success) {
			LUT * pProfileLUT;
			if (profile) {
				pProfileLUT = profile->GetLutPointer();
			} else {
				pProfileLUT = 0;
			}
			if ( pProfileLUT ) {
				monitor->WriteLutToCard(pProfileLUT);
			} else {
				LUT MyLUT;
				GetSignedLUT(&MyLUT);
				monitor->WriteLutToCard(&MyLUT);
			}
			monitor->ReadLutFromCard();
			MonitorSummaryItem * monitorSummaryItem = monitor->GetMonitorSummaryItem();
			if (monitorSummaryItem) {
				monitorSummaryItem->Update();
			}

			SecureZeroMemory(&itemEx, sizeof(itemEx));
			itemEx.hItem = oldItem;
			itemEx.mask = TVIF_STATE;
			itemEx.stateMask = TVIS_BOLD;
			SendMessage(hwndTreeView, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));

			itemEx.hItem = newItem;
			itemEx.state = TVIS_BOLD;
			SendMessage(hwndTreeView, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		}
	}
	DestroyMenu(hPopup);
}

// Display a context menu for a profile on the User profiles or System profiles subtree
//
void TreeViewItem::ProfileContextMenu(MonitorPage * monitorPage, POINT * screenClickPoint, bool isUser) {
	int flags;
	HMENU hPopup;
	MENUINFO menuInfo;
	MENUITEMINFO menuItemInfo;
	TVINSERTSTRUCT tvInsertStruct;
	int id;
	LUT * pProfileLUT;
	size_t count;
	bool foundIt;
	wchar_t buf[1024];
	bool success = false;

	hPopup = CreatePopupMenu();

	SecureZeroMemory(&menuInfo, sizeof(menuInfo));
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_BACKGROUND;
	menuInfo.hbrBack = (HBRUSH)GetStockObject(WHITE_BRUSH);
	SetMenuInfo(hPopup, &menuInfo);

	SecureZeroMemory(&menuItemInfo, sizeof(menuItemInfo));
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	Monitor * monitor = monitorPage->GetMonitor();
	Profile * userProfile = monitor->GetUserProfile();
	Profile * systemProfile = monitor->GetSystemProfile();
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
	} else if ( ProfilePtr->IsBadProfile() ) {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
		menuItemInfo.fState = MFS_DISABLED;
	} else {
		menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
	}
	InsertMenuItem(hPopup, 0, TRUE, &menuItemInfo);

	menuItemInfo.wID = ID_REMOVE_ASSOCIATION;
	menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
	menuItemInfo.dwTypeData = L"Remove association";
	InsertMenuItem(hPopup, 1, TRUE, &menuItemInfo);

	if (VistaOrHigher()) {
		foundIt = false;
		ProfileList profileList = monitor->GetProfileList(!isUser);
		count = profileList.size();
		for (size_t i = 0; i < count; ++i) {
			if ( ProfilePtr == profileList[i] ) {
				foundIt = true;
				break;
			}
		}
		if (isUser) {
			menuItemInfo.dwTypeData = L"Add association to system profiles";
			menuItemInfo.wID = ID_ADD_SYSTEM_ASSOCIATION;
		} else {
			menuItemInfo.dwTypeData = L"Add association to user profiles";
			menuItemInfo.wID = ID_ADD_USER_ASSOCIATION;
		}
		if ( foundIt || ProfilePtr->IsBadProfile() ) {
			menuItemInfo.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
			menuItemInfo.fState = MFS_DISABLED;
		} else {
			menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
		}
		InsertMenuItem(hPopup, 2, TRUE, &menuItemInfo);
	}

	HWND hwndTreeView = monitorPage->GetTreeViewHwnd();
	SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hTreeItem));
	flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
	id = TrackPopupMenuEx(hPopup, flags, screenClickPoint->x, screenClickPoint->y, monitorPage->GetHwnd(), NULL);

	bool addAssociation = false;
	Profile * oldDefaultProfile = 0;
	bool settingNewDefault = false;
	TREEVIEW_ITEM_TYPE newType = ItemType;
	SecureZeroMemory(&tvInsertStruct, sizeof(tvInsertStruct));
	if ( ID_SET_DEFAULT_PROFILE == id ) {
		if (isUser) {
			success = monitor->SetDefaultUserProfile(ProfilePtr);
		} else {
			success = monitor->SetDefaultSystemProfile(ProfilePtr);
		}
		if ( isUser == monitor->GetActiveProfileIsUserProfile() ) {
			pProfileLUT = ProfilePtr->GetLutPointer();
			if ( pProfileLUT ) {
				monitor->WriteLutToCard(pProfileLUT);
			} else {
				LUT MyLUT;
				GetSignedLUT(&MyLUT);
				monitor->WriteLutToCard(&MyLUT);
			}
			monitor->ReadLutFromCard();
			MonitorSummaryItem * monitorSummaryItem = monitor->GetMonitorSummaryItem();
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
					SendMessage( hwndTreeView, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(hTreeItem) ) );
			HTREEITEM hChild = reinterpret_cast<HTREEITEM>(
					SendMessage( hwndTreeView, TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(hParent) ) );
			TVITEMEX itemEx;
			SecureZeroMemory(&itemEx, sizeof(itemEx));
			itemEx.mask = TVIF_HANDLE | TVIF_STATE;
			while (hChild) {
				if ( hChild != hTreeItem ) {
					itemEx.hItem = hChild;
					itemEx.stateMask = TVIS_BOLD;
					SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
					if ( 0 != (itemEx.state & TVIS_BOLD) ) {
						itemEx.state = 0;
						itemEx.stateMask = TVIS_BOLD;
						SendMessage(hwndTreeView, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
					}
				}
				hChild = reinterpret_cast<HTREEITEM>(
						SendMessage( hwndTreeView, TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(hChild) ) );
			}
			itemEx.hItem = hTreeItem;
			itemEx.state = TVIS_BOLD;
			itemEx.stateMask = TVIS_BOLD;
			SendMessage(hwndTreeView, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		}
	} else if ( ID_REMOVE_ASSOCIATION == id ) {
		if (isUser) {
			settingNewDefault = (monitor->GetUserProfile() == ProfilePtr);
			success = monitor->RemoveUserProfileAssociation(ProfilePtr);
		} else {
			settingNewDefault = (monitor->GetSystemProfile() == ProfilePtr);
			success = monitor->RemoveSystemProfileAssociation(ProfilePtr);
		}

		// If we don't have Administrator privileges and we hit the system profile,
		// then we didn't actually remove this association
		//
		if (success) {

			// If we just removed the default profile, maybe update the screen and the Summary panel
			//
			if (settingNewDefault) {
				if ( isUser == monitor->GetActiveProfileIsUserProfile() ) {
					Profile * profile = monitor->GetActiveProfile();
					if (profile) {
						pProfileLUT = profile->GetLutPointer();
					} else {
						pProfileLUT = 0;
					}
					if (pProfileLUT) {
						monitor->WriteLutToCard(pProfileLUT);
					} else {
						LUT MyLUT;
						GetSignedLUT(&MyLUT);
						monitor->WriteLutToCard(&MyLUT);
					}
					monitor->ReadLutFromCard();
					MonitorSummaryItem * monitorSummaryItem = monitor->GetMonitorSummaryItem();
					if (monitorSummaryItem) {
						monitorSummaryItem->Update();
					}
				}
			}

			// Get the old treeview item's state before we delete it
			//
			SecureZeroMemory(&tvInsertStruct, sizeof(tvInsertStruct));
			tvInsertStruct.itemex.mask = TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.hItem = hTreeItem;
			tvInsertStruct.itemex.stateMask = TVIS_OVERLAYMASK;
			SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct.itemex));

			// Delete this node from the TreeView
			//
			SendMessage(hwndTreeView, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(hTreeItem));

			// Maybe add the profile to the "other" list
			//
			foundIt = false;
			if (VistaOrHigher()) {
				ProfileList profileList = monitor->GetProfileList(!isUser);
				count = profileList.size();
				for (size_t i = 0; i < count; ++i) {
					if ( ProfilePtr == profileList[i] ) {
						foundIt = true;
						break;
					}
				}
			}
			if (!foundIt) {
				tvInsertStruct.hParent = monitorPage->GetOtherHTREEITEM();
				tvInsertStruct.hInsertAfter = TVI_LAST;
				tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				StringCbCopy(buf, sizeof(buf), ProfilePtr->GetName().c_str());
				tvInsertStruct.itemex.pszText = buf;
				ItemType = TREEVIEW_ITEM_TYPE_OTHER_PROFILE;
				hTreeItem = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
				SendMessage(hwndTreeView, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvInsertStruct.hParent));
			}

			// If we just got a new default profile on the list that we just removed one from, then make it bold
			//
			if (settingNewDefault) {
				if (isUser) {
					tvInsertStruct.itemex.hItem = TreeViewItem::Get_HTREEITEM_ForProfile(
							hwndTreeView,
							monitorPage->GetUserHTREEITEM(),
							monitor->GetUserProfile() );
				} else {
					tvInsertStruct.itemex.hItem = TreeViewItem::Get_HTREEITEM_ForProfile(
							hwndTreeView,
							monitorPage->GetSystemHTREEITEM(),
							monitor->GetSystemProfile() );
				}
				if (tvInsertStruct.itemex.hItem) {
					tvInsertStruct.itemex.mask = TVIF_STATE;
					tvInsertStruct.itemex.state = TVIS_BOLD;
					tvInsertStruct.itemex.stateMask = TVIS_BOLD;
					SendMessage(hwndTreeView, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct.itemex));
					SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(tvInsertStruct.itemex.hItem));
				}
			}
		}
	} else if ( ID_ADD_USER_ASSOCIATION == id ) {
		oldDefaultProfile = monitor->GetUserProfile();
		success = monitor->AddUserProfileAssociation(ProfilePtr);
		tvInsertStruct.hParent = monitorPage->GetUserHTREEITEM();
		addAssociation = true;
		newType = TREEVIEW_ITEM_TYPE_USER_PROFILE;
	} else if ( ID_ADD_SYSTEM_ASSOCIATION == id ) {
		oldDefaultProfile = monitor->GetSystemProfile();
		success = monitor->AddSystemProfileAssociation(ProfilePtr);
		tvInsertStruct.hParent = monitorPage->GetSystemHTREEITEM();
		addAssociation = true;
		newType = TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE;
	}
	if ( addAssociation && success ) {

		// If we just added the first profile to the list, it became the new default profile
		//
		if ( 0 == oldDefaultProfile ) {
			settingNewDefault = true;
			if ( isUser != monitor->GetActiveProfileIsUserProfile() ) {
				pProfileLUT = ProfilePtr->GetLutPointer();
				if ( pProfileLUT ) {
					monitor->WriteLutToCard(pProfileLUT);
				} else {
					LUT MyLUT;
					GetSignedLUT(&MyLUT);
					monitor->WriteLutToCard(&MyLUT);
				}
				monitor->ReadLutFromCard();
				MonitorSummaryItem * monitorSummaryItem = monitor->GetMonitorSummaryItem();
				if (monitorSummaryItem) {
					monitorSummaryItem->Update();
				}
			}
		}

		// Get the old treeview item's state
		//
		tvInsertStruct.itemex.mask = TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvInsertStruct.itemex.hItem = hTreeItem;
		tvInsertStruct.itemex.stateMask = TVIS_OVERLAYMASK;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct.itemex));

		// Add the profile to the selected profiles list
		//
		tvInsertStruct.hInsertAfter = TVI_LAST;
		tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		StringCbCopy(buf, sizeof(buf), ProfilePtr->GetName().c_str());
		tvInsertStruct.itemex.pszText = buf;
		TreeViewItem * tiObject = monitorPage->AddTreeViewItem(new TreeViewItem(newType));
		tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
		if (settingNewDefault) {
			tvInsertStruct.itemex.state |= TVIS_BOLD;
			tvInsertStruct.itemex.stateMask |= TVIS_BOLD;
		}
		HTREEITEM tvItem = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
		tiObject->SetHTREEITEM(tvItem);
		tiObject->SetProfile(ProfilePtr);
		SendMessage(hwndTreeView, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvInsertStruct.hParent));
		if (settingNewDefault) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvInsertStruct.hParent));
		}
	}
	DestroyMenu(hPopup);
}

// Display a context menu for a profile on the Other profiles subtree
//
void TreeViewItem::OtherProfileContextMenu(MonitorPage * monitorPage, POINT * screenClickPoint) {
	MENUINFO menuInfo;
	MENUITEMINFO menuItemInfo;
	TVINSERTSTRUCT tvInsertStruct;
	wchar_t buf[1024];
	bool success;

	HMENU hPopup = CreatePopupMenu();

	SecureZeroMemory(&menuInfo, sizeof(menuInfo));
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_BACKGROUND;
	menuInfo.hbrBack = (HBRUSH)GetStockObject(WHITE_BRUSH);
	SetMenuInfo(hPopup, &menuInfo);

	SecureZeroMemory(&menuItemInfo, sizeof(menuItemInfo));
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
	UINT menuPosition = 0;
	if (VistaOrHigher()) {
		menuItemInfo.dwTypeData = L"Add association to user profiles";
		menuItemInfo.wID = ID_ADD_USER_ASSOCIATION;
		InsertMenuItem(hPopup, menuPosition, TRUE, &menuItemInfo);
		++menuPosition;
	}
	if (VistaOrHigher()) {
		menuItemInfo.dwTypeData = L"Add association to system profiles";
	} else {
		menuItemInfo.dwTypeData = L"Add association";
	}
	menuItemInfo.wID = ID_ADD_SYSTEM_ASSOCIATION;
	InsertMenuItem(hPopup, menuPosition, TRUE, &menuItemInfo);

	HWND hwndTreeView = monitorPage->GetTreeViewHwnd();
	SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hTreeItem));

	int flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
	int id = TrackPopupMenuEx(hPopup, flags, screenClickPoint->x, screenClickPoint->y, monitorPage->GetHwnd(), NULL);

	Monitor * monitor = monitorPage->GetMonitor();
	Profile * oldDefaultProfile = 0;
	bool isUser = false;
	bool settingNewDefault = false;
	LUT * pProfileLUT = 0;
	TREEVIEW_ITEM_TYPE newType = ItemType;
	SecureZeroMemory(&tvInsertStruct, sizeof(tvInsertStruct));
	if (ID_ADD_USER_ASSOCIATION == id) {
		isUser = true;
		oldDefaultProfile = monitor->GetUserProfile();
		success = monitor->AddUserProfileAssociation(ProfilePtr);
		tvInsertStruct.hParent = monitorPage->GetUserHTREEITEM();
		newType = TREEVIEW_ITEM_TYPE_USER_PROFILE;
	} else if (ID_ADD_SYSTEM_ASSOCIATION == id) {
		isUser = false;
		oldDefaultProfile = monitor->GetSystemProfile();
		success = monitor->AddSystemProfileAssociation(ProfilePtr);
		tvInsertStruct.hParent = monitorPage->GetSystemHTREEITEM();
		newType = TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE;
	} else {
		success = false;
	}
	if (success) {

		// If we just added the first profile to the list, it became the new default profile
		//
		if ( 0 == oldDefaultProfile ) {
			settingNewDefault = true;
			if ( isUser == monitor->GetActiveProfileIsUserProfile() ) {
				pProfileLUT = ProfilePtr->GetLutPointer();
				if ( pProfileLUT ) {
					monitor->WriteLutToCard(pProfileLUT);
				} else {
					LUT MyLUT;
					GetSignedLUT(&MyLUT);
					monitor->WriteLutToCard(&MyLUT);
				}
				monitor->ReadLutFromCard();
				MonitorSummaryItem * monitorSummaryItem = monitor->GetMonitorSummaryItem();
				if (monitorSummaryItem) {
					monitorSummaryItem->Update();
				}
			}
		}

		// Get the old treeview item's state before we delete it
		//
		tvInsertStruct.itemex.mask = TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvInsertStruct.itemex.hItem = hTreeItem;
		tvInsertStruct.itemex.stateMask = TVIS_OVERLAYMASK;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct.itemex));

		// Delete it
		//
		SendMessage(hwndTreeView, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(hTreeItem));

		// Add the profile to the selected profiles list
		//
		tvInsertStruct.hInsertAfter = TVI_LAST;
		tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		StringCbCopy(buf, sizeof(buf), ProfilePtr->GetName().c_str());
		tvInsertStruct.itemex.pszText = buf;
		ItemType = newType;
		if (settingNewDefault) {
			tvInsertStruct.itemex.state |= TVIS_BOLD;
			tvInsertStruct.itemex.stateMask |= TVIS_BOLD;
		}
		hTreeItem = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
		SendMessage(hwndTreeView, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvInsertStruct.hParent));
		if (settingNewDefault) {
			HTREEITEM parent = isUser ? monitorPage->GetUserHTREEITEM() : monitorPage->GetSystemHTREEITEM();
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(parent));
		}
	}
	DestroyMenu(hPopup);
}
