// TreeViewItem.cpp -- Class for holding TreeView items
//

#include "stdafx.h"
#include "Monitor.h"
#include "MonitorPage.h"
#include "MonitorSummaryItem.h"
#include "resource.h"
#include "TreeViewItem.h"
//#include "Utility.h"

// Symbols defined in other files
//
extern HINSTANCE g_hInst;

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
	int flags;
	HMENU hMenu = 0;
	HMENU hPopup = 0;
	MENUINFO menuInfo;
	MENUITEMINFO menuItemInfo;
	int id;
	wstring s;
	LUT * pProfileLUT;
	Monitor * monitor;
	MonitorSummaryItem * monitorSummaryItem;

	switch (ItemType) {
		case TREEVIEW_ITEM_TYPE_NONE:
			return;
			break;

		case TREEVIEW_ITEM_TYPE_MONITOR:
			hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_LUTVIEW_POPUP_MENU));
			hPopup = GetSubMenu(hMenu, 0);
			CheckMenuRadioItem(
					hPopup,
					ID_WHITEBACKGROUND,
					ID_GRADIENTBACKGROUND,
					ID_WHITEBACKGROUND + 0,
					MF_BYCOMMAND );

			SecureZeroMemory(&menuInfo, sizeof(menuInfo));
			menuInfo.cbSize = sizeof(menuInfo);
			menuInfo.fMask = MIM_BACKGROUND;
			menuInfo.hbrBack = (HBRUSH)GetStockObject(WHITE_BRUSH);
			//menuInfo.hbrBack = CreateSolidBrush(RGB(235,243,253));
			SetMenuInfo(hPopup, &menuInfo);

			//EnableMenuItem(hPopup, 1, MF_BYPOSITION | MF_DISABLED);

			//SetMenuDefaultItem(hPopup, 1, TRUE);

			flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
			id = TrackPopupMenuEx(hPopup, flags, screenClickPoint->x, screenClickPoint->y, monitorPage->GetHWND(), NULL);
			if ( id >= ID_WHITEBACKGROUND && id <= ID_GRADIENTBACKGROUND ) {
				switch (id) {
					case ID_WHITEBACKGROUND:
						s = L"User chose white background";
						break;
					case ID_BLACKBACKGROUND:
						s = L"User chose black background";
						break;
					case ID_GRADIENTBACKGROUND:
						s = L"User chose gradient background";
						break;
				}
				monitorPage->SetEditControlText(s);
			}
			DestroyMenu(hMenu);
			break;

		case TREEVIEW_ITEM_TYPE_USER_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_PROFILES:
			break;

		case TREEVIEW_ITEM_TYPE_USER_PROFILE:
			hPopup = CreatePopupMenu();

			SecureZeroMemory(&menuInfo, sizeof(menuInfo));
			menuInfo.cbSize = sizeof(menuInfo);
			menuInfo.fMask = MIM_BACKGROUND;
			menuInfo.hbrBack = (HBRUSH)GetStockObject(WHITE_BRUSH);
			SetMenuInfo(hPopup, &menuInfo);

			SecureZeroMemory(&menuItemInfo, sizeof(menuItemInfo));
			menuItemInfo.cbSize = sizeof(menuItemInfo);
			menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
			monitor = monitorPage->GetMonitor();
			if (monitor->GetActiveProfileIsUserProfile()) {
				menuItemInfo.dwTypeData = L"Set as default profile";
			} else {
				menuItemInfo.dwTypeData = L"Set as default user profile (inactive)";
			}
			menuItemInfo.wID = 17;
			InsertMenuItem(hPopup, 0, TRUE, &menuItemInfo);

			//menuItemInfo.dwTypeData = L"Remove association";
			//InsertMenuItem(hPopup, 1, TRUE, &menuItemInfo);

			flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
			id = TrackPopupMenuEx(hPopup, flags, screenClickPoint->x, screenClickPoint->y, monitorPage->GetHWND(), NULL);

			if ( 17 == id ) {
				wstring errorString;
				ProfilePtr->LoadFullProfile(false);
				monitor->SetDefaultProfile(ProfilePtr, true, errorString);
				if (monitor->GetActiveProfileIsUserProfile()) {
					pProfileLUT = ProfilePtr->GetLutPointer();
					if ( pProfileLUT ) {
						monitor->WriteLutToCard(pProfileLUT);
						monitor->ReadLutFromCard();
					}
					monitorSummaryItem = monitor->GetMonitorSummaryItem();
					if (monitorSummaryItem) {
						monitorSummaryItem->Update();
					}
				}
			}

			DestroyMenu(hPopup);
			break;

		case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE:
			hPopup = CreatePopupMenu();

			SecureZeroMemory(&menuInfo, sizeof(menuInfo));
			menuInfo.cbSize = sizeof(menuInfo);
			menuInfo.fMask = MIM_BACKGROUND;
			menuInfo.hbrBack = (HBRUSH)GetStockObject(WHITE_BRUSH);
			SetMenuInfo(hPopup, &menuInfo);

			SecureZeroMemory(&menuItemInfo, sizeof(menuItemInfo));
			menuItemInfo.cbSize = sizeof(menuItemInfo);
			menuItemInfo.fMask = MIIM_STRING | MIIM_ID;
			monitor = monitorPage->GetMonitor();
			if (monitor->GetActiveProfileIsUserProfile()) {
				menuItemInfo.dwTypeData = L"Set as default system profile (inactive)";
			} else {
				menuItemInfo.dwTypeData = L"Set as default profile";
			}
			menuItemInfo.wID = 17;
			InsertMenuItem(hPopup, 0, TRUE, &menuItemInfo);

			//menuItemInfo.dwTypeData = L"Remove association";
			//InsertMenuItem(hPopup, 1, TRUE, &menuItemInfo);

			flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
			id = TrackPopupMenuEx(hPopup, flags, screenClickPoint->x, screenClickPoint->y, monitorPage->GetHWND(), NULL);

			if ( 17 == id ) {
				wstring errorString;
				ProfilePtr->LoadFullProfile(false);
				monitor->SetDefaultProfile(ProfilePtr, false, errorString);
				if ( false == monitor->GetActiveProfileIsUserProfile() ) {
					pProfileLUT = ProfilePtr->GetLutPointer();
					if ( pProfileLUT ) {
						monitor->WriteLutToCard(pProfileLUT);
						monitor->ReadLutFromCard();
					}
					monitorSummaryItem = monitor->GetMonitorSummaryItem();
					if (monitorSummaryItem) {
						monitorSummaryItem->Update();
					}
				}
			}

			DestroyMenu(hPopup);
			break;
	}
}
