// MonitorPage.cpp -- Display a property sheet page for each monitor
//

#include "stdafx.h"
#include <commctrl.h>
#include "Monitor.h"
#include "MonitorPage.h"
#include "PropertySheet.h"
#include "Resize.h"
#include "TreeViewItem.h"
#include "Utility.h"
#include "resource.h"
#include <strsafe.h>
//#include <banned.h>

// Optional "features"
//
#define TREEVIEW_USE_DOUBLE_BUFFERING_WHEN_AVAILABLE 1

// Constants
//
#define MINIMUM_TREEVIEW_WIDTH	120					// Minimum TreeView control width for splitter (DPI scaled)
#define MINIMUM_EDIT_WIDTH		120					// Minimum Edit control width for splitter (DPI scaled)

#define IMAGE_MONITOR			0					// Indexes to images in TreeView's ImageList
#define IMAGE_PROFILES			1
#define IMAGE_ICC_PROFILE		2

#define SHELL32_FOLDER_ICON		5					// The icon we use for folders from shell32.dll

// Global externs defined in this file
//
extern WNDPROC oldEditWindowProc = 0;				// The original Edit control's window procedure

// Symbols defined in other files
//
extern HINSTANCE g_hInst;							// Instance handle
extern double dpiScale;								// Scaling factor for dots per inch (actual versus standard 96 DPI)
extern wchar_t * ColorDirectory;

// Global static symbols internal to this file
//
static HIMAGELIST hImageList = 0;
static int imageWCSprofile = IMAGE_ICC_PROFILE;
static int overlayImage= 0;;

// Constructor
//
MonitorPage::MonitorPage(Monitor * hostMonitor) :
		monitor(hostMonitor),
		hwnd(0),
		hwndTreeView(0),
		hwndEdit(0),
		tvRoot(0),
		tvUserProfiles(0),
		tvSystemProfiles(0),
		tvOtherProfiles(0),
		requestedTreeViewWidth(0),
		requestedRootExpansion(true),
		requestedUserProfilesExpansion(false),
		requestedSystemProfilesExpansion(false),
		requestedOtherProfilesExpansion(false),
		resetting(false)
{
}

// Destructor
//
MonitorPage::~MonitorPage() {
	ClearTreeViewItemList(true);
}

void MonitorPage::SetEditControlText(wstring newText) {
	SetDlgItemText(hwnd, IDC_MONITOR_TEXT, newText.c_str());
}

HWND MonitorPage::GetHwnd(void) const {
	return hwnd;
}

HWND MonitorPage::GetTreeViewHwnd(void) const {
	return hwndTreeView;
}

HTREEITEM MonitorPage::GetUserHTREEITEM(void) const {
	return tvUserProfiles;
}

HTREEITEM MonitorPage::GetSystemHTREEITEM(void) const {
	return tvSystemProfiles;
}

HTREEITEM MonitorPage::GetOtherHTREEITEM(void) const {
	return tvOtherProfiles;
}

Monitor * MonitorPage::GetMonitor(void) const {
	return monitor;
}

void MonitorPage::RequestTreeViewWidth(int width) {
	requestedTreeViewWidth = width;
}

void MonitorPage::GetTreeViewNodeExpansionString(wchar_t * str, size_t strSize) {
	if (hwndTreeView) {
		TVITEMEX itemEx;
		bool userProfilesExpanded = false;

		SecureZeroMemory(&itemEx, sizeof(itemEx));
		itemEx.mask = TVIF_HANDLE | TVIF_STATE;
		itemEx.stateMask = TVIS_EXPANDED;
		itemEx.hItem = tvRoot;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		bool rootExpanded = (0 != (itemEx.state & TVIS_EXPANDED));

		if (tvUserProfiles) {
			itemEx.hItem = tvUserProfiles;
			SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
			userProfilesExpanded = (0 != (itemEx.state & TVIS_EXPANDED));
		}

		itemEx.hItem = tvSystemProfiles;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		bool systemProfilesExpanded = (0 != (itemEx.state & TVIS_EXPANDED));

		itemEx.hItem = tvOtherProfiles;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		bool otherProfilesExpanded = (0 != (itemEx.state & TVIS_EXPANDED));

		StringCbPrintf(
				str,
				strSize,
				L"%d,%d,%d,%d",
				rootExpanded,
				userProfilesExpanded,
				systemProfilesExpanded,
				otherProfilesExpanded );
	}
}

void MonitorPage::SetTreeViewNodeExpansionString(wchar_t * str) {
	int rootExpanded;
	int userProfilesExpanded;
	int systemProfilesExpanded;
	int otherProfilesExpanded;
	int nRead = swscanf_s(
			str,
			L"%d,%d,%d,%d",
			&rootExpanded,
			&userProfilesExpanded,
			&systemProfilesExpanded,
			&otherProfilesExpanded );
	if ( 4 == nRead ) {
		requestedRootExpansion = (0 != rootExpanded);
		requestedUserProfilesExpansion = (0 != userProfilesExpanded);
		requestedSystemProfilesExpansion = (0 != systemProfilesExpanded);
		requestedOtherProfilesExpansion = (0 != otherProfilesExpanded);
	}
}

// Add a TreeViewItem to the end of the list
//
TreeViewItem * MonitorPage::AddTreeViewItem(TreeViewItem * treeViewItem) {
	treeviewitemList.push_back(treeViewItem);
	return treeViewItem;
}

// Clear the list of TreeViewItems
//
void MonitorPage::ClearTreeViewItemList(bool freeAllMemory) {
	size_t count = treeviewitemList.size();
	for (size_t i = 0; i < count; ++i) {
		delete treeviewitemList[i];
	}
	treeviewitemList.clear();
	if ( freeAllMemory && (treeviewitemList.capacity() > 0) ) {
		vector <TreeViewItem *> dummy;
		treeviewitemList.swap(dummy);
	}
}

// A Reset() call tells us that our UI should be reset to starting condition,
// assuming that we have had our WM_INITDIALOG call and so have something to reset
//
void MonitorPage::Reset(void) {
	if (hwnd) {
		HTREEITEM hTvItem;
		TVITEMEX itemEx;
		TREEVIEW_ITEM_TYPE oldItemType = TREEVIEW_ITEM_TYPE_NONE;
		Profile * oldProfilePtr = 0;
		bool userProfilesExpanded = false;

		HTREEITEM hSelection = reinterpret_cast<HTREEITEM>(
				SendMessage( hwndTreeView, TVM_GETNEXTITEM, TVGN_CARET, 0 ) );

		SecureZeroMemory(&itemEx, sizeof(itemEx));
		itemEx.mask = TVIF_HANDLE | TVIF_PARAM;
		itemEx.hItem = hSelection;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));

		TreeViewItem * oldSelection = reinterpret_cast<TreeViewItem *>(itemEx.lParam);
		if (oldSelection) {
			oldItemType = oldSelection->GetItemType();
			oldProfilePtr = oldSelection->GetProfilePtr();
		}

		itemEx.mask = TVIF_HANDLE | TVIF_STATE;
		itemEx.stateMask = TVIS_EXPANDED;
		itemEx.hItem = tvRoot;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		bool rootExpanded = (0 != (itemEx.state & TVIS_EXPANDED));

		if (tvUserProfiles) {
			itemEx.hItem = tvUserProfiles;
			SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
			userProfilesExpanded = (0 != (itemEx.state & TVIS_EXPANDED));
		}

		itemEx.hItem = tvSystemProfiles;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		bool systemProfilesExpanded = (0 != (itemEx.state & TVIS_EXPANDED));

		itemEx.hItem = tvOtherProfiles;
		SendMessage(hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
		bool otherProfilesExpanded = (0 != (itemEx.state & TVIS_EXPANDED));

		resetting = true;
		SendMessage(hwndTreeView, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
		ClearTreeViewItemList(false);

		BuildTreeView();

		if (rootExpanded) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvRoot));
		}
		if (userProfilesExpanded) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvUserProfiles));
		}
		if (systemProfilesExpanded) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvSystemProfiles));
		}
		if (otherProfilesExpanded) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvOtherProfiles));
		}

		switch (oldItemType) {
			case TREEVIEW_ITEM_TYPE_MONITOR:
				SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(tvRoot));
				break;

			case TREEVIEW_ITEM_TYPE_USER_PROFILES:
				SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(tvUserProfiles));
				break;

			case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES:
				SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(tvSystemProfiles));
				break;

			case TREEVIEW_ITEM_TYPE_PROFILES:
				SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(tvSystemProfiles));
				break;

			case TREEVIEW_ITEM_TYPE_OTHER_PROFILES:
				SendMessage(hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(tvOtherProfiles));
				break;

			case TREEVIEW_ITEM_TYPE_USER_PROFILE:
				hTvItem = TreeViewItem::Get_HTREEITEM_ForProfile( hwndTreeView, tvUserProfiles, oldProfilePtr );
				SendMessage( hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hTvItem ? hTvItem : tvUserProfiles) );
				break;

			case TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE:
				hTvItem = TreeViewItem::Get_HTREEITEM_ForProfile( hwndTreeView, tvSystemProfiles, oldProfilePtr );
				SendMessage( hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hTvItem ? hTvItem : tvSystemProfiles) );
				break;

			case TREEVIEW_ITEM_TYPE_OTHER_PROFILE:
				hTvItem = TreeViewItem::Get_HTREEITEM_ForProfile( hwndTreeView, tvOtherProfiles, oldProfilePtr );
				SendMessage( hwndTreeView, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hTvItem ? hTvItem : tvOtherProfiles) );
				break;

		}
		resetting = false;
	}
}

// Create an ImageList for the TreeView
//
void CreateImageList(void) {
	HBITMAP hBitmap;
	HICON hIcon;
	HKEY hKey;
	wchar_t registryString[2048];
	wchar_t expandedString[2048];
	DWORD dataSize;
	wchar_t * ptr;
	wchar_t * comma;
	int index;
	int nRead;
	HMODULE hModule;
	int iconSize;

	// Adjust icon size for high DPI
	//
	iconSize = static_cast<int>(0.49 + (16 * dpiScale));
	hImageList = ImageList_Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 5, 1);
	if (hImageList) {

		// Index 0 -- IMAGE_MONITOR -- Monitor
		//
		hIcon = reinterpret_cast<HICON>(LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ICON_SETUPAPI_35), IMAGE_ICON, iconSize, iconSize, LR_CREATEDIBSECTION));
		ImageList_ReplaceIcon(hImageList, -1, hIcon);
		DeleteObject(hIcon);

		// Index 1 -- IMAGE_PROFILES -- any subtree of profiles (User profiles, System profiles, Profiles)
		//
		StringCbCopy(registryString, sizeof(registryString), L"%SystemRoot%\\System32\\shell32.dll");
		SecureZeroMemory(expandedString, sizeof(expandedString));
		ExpandEnvironmentStrings(registryString, expandedString, _countof(expandedString));
		hModule = LoadLibraryEx(expandedString, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		if (hModule) {
			hIcon = reinterpret_cast<HICON>(LoadImage(hModule, MAKEINTRESOURCE(SHELL32_FOLDER_ICON), IMAGE_ICON, iconSize, iconSize, LR_CREATEDIBSECTION));
			ImageList_ReplaceIcon(hImageList, -1, hIcon);
			DeleteObject(hIcon);
			FreeLibrary(hModule);
		}

		// Index 2 -- IMAGE_ICC_PROFILE -- use the DefaultIcon for .icc and .icm files
		//
		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"icmfile\\DefaultIcon", 0, KEY_QUERY_VALUE, &hKey)) {
			dataSize = sizeof(registryString);
			SecureZeroMemory(registryString, dataSize);
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, NULL, NULL, NULL, reinterpret_cast<BYTE *>(registryString), &dataSize)) {
				SecureZeroMemory(expandedString, sizeof(expandedString));
				ExpandEnvironmentStrings(registryString, expandedString, _countof(expandedString));
				ptr = expandedString;
				comma = 0;
				while ( *ptr ) {
					if ( L',' == *ptr ) {
						comma = ptr;
					}
					++ptr;
				}
				if (comma) {
					*comma = 0;
					++comma;
					nRead = swscanf_s(comma, L"%d", &index);
					if ( 1 == nRead ) {
						if ( index < 0 ) {
							index = -index;
						} else if ( 0 == index ) {
							index = 100;		// Hard code XP index, sigh, it has 0 in its registry entry ...
						}
						hModule = LoadLibraryEx(expandedString, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
						if (hModule) {
							hIcon = reinterpret_cast<HICON>(LoadImage(hModule, MAKEINTRESOURCE(index), IMAGE_ICON, iconSize, iconSize, LR_CREATEDIBSECTION));
							ImageList_ReplaceIcon(hImageList, -1, hIcon);
							DeleteObject(hIcon);
							FreeLibrary(hModule);
						}
					}
				}
			}
			RegCloseKey(hKey);
		}

		// Index 3, if used -- imageWCSindex -- try to use the DefaultIcon for .cdmp files, otherwise just use the ICC image
		//
		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"cdmpfile\\DefaultIcon", 0, KEY_QUERY_VALUE, &hKey)) {
			dataSize = sizeof(registryString);
			SecureZeroMemory(registryString, dataSize);
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, NULL, NULL, NULL, reinterpret_cast<BYTE *>(registryString), &dataSize)) {
				SecureZeroMemory(expandedString, sizeof(expandedString));
				ExpandEnvironmentStrings(registryString, expandedString, _countof(expandedString));
				ptr = expandedString;
				comma = 0;
				while ( *ptr ) {
					if ( L',' == *ptr ) {
						comma = ptr;
					}
					++ptr;
				}
				if (comma) {
					*comma = 0;
					++comma;
					nRead = swscanf_s(comma, L"%d", &index);
					if ( 1 == nRead ) {
						if ( index < 0 ) {
							index = -index;
						}
						hModule = LoadLibraryEx(expandedString, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
						if (hModule) {
							hIcon = reinterpret_cast<HICON>(LoadImage(hModule, MAKEINTRESOURCE(index), IMAGE_ICON, iconSize, iconSize, LR_CREATEDIBSECTION));
							imageWCSprofile = ImageList_ReplaceIcon(hImageList, -1, hIcon);
							DeleteObject(hIcon);
							FreeLibrary(hModule);
						}
					}
				}
			}
			RegCloseKey(hKey);
		}

		// Image 3 or 4 (depending on if XP) -- overlay for profile indicating that it has a VCGT
		//
		int resourceID;									// Select overlay image based on DPI setting
		if (iconSize <= 16) {
			resourceID = IDB_VCGT_OVERLAY_16X16;
		} else if (iconSize <= 20) {
			resourceID = IDB_VCGT_OVERLAY_20X20;
		} else if (iconSize <= 24) {
			resourceID = IDB_VCGT_OVERLAY_24X24;
		} else {
			resourceID = IDB_VCGT_OVERLAY_32X32;
		}
		HBITMAP hBitmap2;		// We need to use two icon handles, otherwise ImageList_SetOverlayImage() draws a black image (the mask)
		hBitmap  = reinterpret_cast<HBITMAP>(LoadImage(g_hInst, MAKEINTRESOURCE(resourceID), IMAGE_BITMAP, iconSize, iconSize, LR_CREATEDIBSECTION));
		hBitmap2 = reinterpret_cast<HBITMAP>(LoadImage(g_hInst, MAKEINTRESOURCE(resourceID), IMAGE_BITMAP, iconSize, iconSize, LR_CREATEDIBSECTION));
		overlayImage = ImageList_Add(hImageList, hBitmap, hBitmap2);
		DeleteObject(hBitmap);
		DeleteObject(hBitmap2);
		ImageList_SetOverlayImage(hImageList, overlayImage, 1);
	}
}

// Build the TreeView
//
void MonitorPage::BuildTreeView(void) {

	ProfileList profileList;
	HTREEITEM tvItem;
	size_t count;
	wchar_t buf[1024];
	TreeViewItem * tiObject = 0;

	if ( !hImageList ) {
		CreateImageList();
	}
	SendMessage(hwndTreeView, TVM_SETIMAGELIST, TVSIL_NORMAL, reinterpret_cast<LPARAM>(hImageList));

#if TREEVIEW_USE_DOUBLE_BUFFERING_WHEN_AVAILABLE
	// For Vista and later, turn on double-buffering (offscreen bitmap drawing)
	//
	if (VistaOrHigher()) {
#ifndef TVS_EX_DOUBLEBUFFER
#define TVS_EX_DOUBLEBUFFER         0x0004
#endif
		SendMessage(hwndTreeView, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	}
#endif

	TVINSERTSTRUCT tvInsertStruct;
	SecureZeroMemory(&tvInsertStruct, sizeof(tvInsertStruct));
	tvInsertStruct.hInsertAfter = TVI_ROOT;
	tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvInsertStruct.itemex.iImage = IMAGE_MONITOR;
	tvInsertStruct.itemex.iSelectedImage = IMAGE_MONITOR;
	StringCbCopy(buf, sizeof(buf), monitor->GetDeviceString().c_str());
	tvInsertStruct.itemex.pszText = buf;
	tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_MONITOR));
	tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
	tvRoot = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
	tiObject->SetHTREEITEM(tvRoot);
	tiObject->SetMonitor(monitor);

	if (VistaOrHigher()) {

		// Vista or higher gets two profile subtrees, User and System
		//
		bool hiliteUser = monitor->GetActiveProfileIsUserProfile();
		tvInsertStruct.hParent = tvRoot;
		tvInsertStruct.hInsertAfter = TVI_LAST;
		if (hiliteUser) {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.state = TVIS_BOLD;
			tvInsertStruct.itemex.stateMask = TVIS_BOLD;
		} else {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.state = 0;
			tvInsertStruct.itemex.stateMask = 0;
		}
		tvInsertStruct.itemex.iImage = IMAGE_PROFILES;
		tvInsertStruct.itemex.iSelectedImage = IMAGE_PROFILES;
		tvInsertStruct.itemex.pszText = L"User profiles";
		tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_USER_PROFILES));
		tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
		tvUserProfiles = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
		tiObject->SetHTREEITEM(tvUserProfiles);
		tvInsertStruct.hParent = tvUserProfiles;

		profileList = monitor->GetProfileList(true);
		count = profileList.size();
		for (size_t i = 0; i < count; ++i) {
			profileList[i]->LoadFullProfile(false);
 			if (profileList[i]->HasEmbeddedWcsProfile()) {
				tvInsertStruct.itemex.iImage = imageWCSprofile;
			} else {
				tvInsertStruct.itemex.iImage = IMAGE_ICC_PROFILE;
			}
			if (profileList[i]->GetLutPointer()) {
				tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tvInsertStruct.itemex.state = INDEXTOOVERLAYMASK(1);
				tvInsertStruct.itemex.stateMask = TVIS_OVERLAYMASK;
			} else {
				tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tvInsertStruct.itemex.state = 0;
				tvInsertStruct.itemex.stateMask = 0;
			}
			tvInsertStruct.itemex.iSelectedImage = tvInsertStruct.itemex.iImage;
			if ( profileList[i] == monitor->GetUserProfile() ) {
				tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tvInsertStruct.itemex.state |= TVIS_BOLD;
				tvInsertStruct.itemex.stateMask |= TVIS_BOLD;
			}
			StringCbCopy(buf, sizeof(buf), profileList[i]->GetName().c_str());
			tvInsertStruct.itemex.pszText = buf;
			tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_USER_PROFILE));
			tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
			tvItem = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
			tiObject->SetHTREEITEM(tvItem);
			tiObject->SetProfile(profileList[i]);
		}
		SendMessage(hwndTreeView, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvUserProfiles));

		if (!hiliteUser) {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.state = TVIS_BOLD;
			tvInsertStruct.itemex.stateMask = TVIS_BOLD;
		} else {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.state = 0;
			tvInsertStruct.itemex.stateMask = 0;
		}
		tvInsertStruct.itemex.pszText = L"System profiles";
		tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES));

		// Code merges with Windows XP code below (Vista "System profiles" is the same as XP "Profiles")
		//

	} else {

		// Windows XP gets a single profile subtree, just called "Profiles"
		//
		tvInsertStruct.itemex.pszText = L"Profiles";
		tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_PROFILES));
	}

	// Common code for Vista and Windows 7 "System profiles" and XP "Profiles"
	//
	tvInsertStruct.hParent = tvRoot;
	tvInsertStruct.itemex.iImage = IMAGE_PROFILES;
	tvInsertStruct.itemex.iSelectedImage = IMAGE_PROFILES;
	tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
	tvSystemProfiles = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
	tiObject->SetHTREEITEM(tvSystemProfiles);
	tvInsertStruct.hParent = tvSystemProfiles;

	profileList = monitor->GetProfileList(false);
	count = profileList.size();
	for (size_t i = 0; i < count; ++i) {
		profileList[i]->LoadFullProfile(false);
		if (profileList[i]->HasEmbeddedWcsProfile()) {
			tvInsertStruct.itemex.iImage = imageWCSprofile;
		} else {
			tvInsertStruct.itemex.iImage = IMAGE_ICC_PROFILE;
		}
		if (profileList[i]->GetLutPointer()) {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.state = INDEXTOOVERLAYMASK(1);
			tvInsertStruct.itemex.stateMask = TVIS_OVERLAYMASK;
		} else {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.state = 0;
			tvInsertStruct.itemex.stateMask = 0;
		}
		tvInsertStruct.itemex.iSelectedImage = tvInsertStruct.itemex.iImage;
		if ( profileList[i] == monitor->GetSystemProfile() ) {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvInsertStruct.itemex.state |= TVIS_BOLD;
			tvInsertStruct.itemex.stateMask |= TVIS_BOLD;
		}
		StringCbCopy(buf, sizeof(buf), profileList[i]->GetName().c_str());
		tvInsertStruct.itemex.pszText = buf;
		tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_SYSTEM_PROFILE));
		tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
		tvItem = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
		tiObject->SetHTREEITEM(tvItem);
		tiObject->SetProfile(profileList[i]);
	}
	SendMessage(hwndTreeView, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvSystemProfiles));

	// Add other display profiles -- everything in the color directory that is a display profile
	// and isn't already on one of the lists
	//
	tvInsertStruct.hParent = tvRoot;
	tvInsertStruct.hInsertAfter = TVI_LAST;
	tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvInsertStruct.itemex.state = 0;
	tvInsertStruct.itemex.stateMask = 0;
	tvInsertStruct.itemex.iImage = IMAGE_PROFILES;
	tvInsertStruct.itemex.iSelectedImage = IMAGE_PROFILES;
	tvInsertStruct.itemex.pszText = L"Other installed display profiles";
	tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_OTHER_PROFILES));
	tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
	tvOtherProfiles = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
	tiObject->SetHTREEITEM(tvOtherProfiles);
	tvInsertStruct.hParent = tvOtherProfiles;

	if (ColorDirectory) {
		wchar_t filepath[1024];
		StringCbCopy(filepath, sizeof(filepath), ColorDirectory);
		StringCbCat(filepath, sizeof(filepath), L"\\*");
		WIN32_FIND_DATA findData;
		SecureZeroMemory(&findData, sizeof(findData));
		bool keepGoing = true;
		Profile * profile = 0;
		HANDLE findHandle = FindFirstFile(filepath, &findData);
		if (INVALID_HANDLE_VALUE != findHandle) {
			while (keepGoing) {
#define SKIP_BITS (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_VIRTUAL)
				if ( 0 == (SKIP_BITS & findData.dwFileAttributes) ) {
					profile = Profile::Add(new Profile(findData.cFileName));

					// Filter out profiles that are on another list already
					//
					bool foundIt = false;
					if (VistaOrHigher()) {
						profileList = monitor->GetProfileList(true);
						count = profileList.size();
						for (size_t i = 0; i < count; ++i) {
							if (profile == profileList[i]) {
								foundIt = true;
								break;
							}
						}
					}
					if ( false == foundIt ) {
						profileList = monitor->GetProfileList(false);
						count = profileList.size();
						for (size_t i = 0; i < count; ++i) {
							if (profile == profileList[i]) {
								foundIt = true;
								break;
							}
						}
					}

					// It's not already on a list for this monitor, so if its a valid display profile,
					// add it to the "other" list
					//
					if ( false == foundIt ) {
						profile->LoadFullProfile(false);
						if ( (false == profile->IsBadProfile()) && (CLASS_MONITOR == profile->GetProfileClass()) ) {
							if (profile->HasEmbeddedWcsProfile()) {
								tvInsertStruct.itemex.iImage = imageWCSprofile;
							} else {
								tvInsertStruct.itemex.iImage = IMAGE_ICC_PROFILE;
							}
							if (profile->GetLutPointer()) {
								tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
								tvInsertStruct.itemex.state = INDEXTOOVERLAYMASK(1);
								tvInsertStruct.itemex.stateMask = TVIS_OVERLAYMASK;
							} else {
								tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
								tvInsertStruct.itemex.state = 0;
								tvInsertStruct.itemex.stateMask = 0;
							}
							tvInsertStruct.itemex.iSelectedImage = tvInsertStruct.itemex.iImage;
							StringCbCopy(buf, sizeof(buf), profile->GetName().c_str());
							tvInsertStruct.itemex.pszText = buf;
							tiObject = AddTreeViewItem(new TreeViewItem(TREEVIEW_ITEM_TYPE_OTHER_PROFILE));
							tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
							tvItem = reinterpret_cast<HTREEITEM>( SendMessage(hwndTreeView, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
							tiObject->SetHTREEITEM(tvItem);
							tiObject->SetProfile(profile);
						}
					}
				}
				keepGoing = (0 != FindNextFile(findHandle, &findData));
			}
			FindClose(findHandle);
		}
		SendMessage(hwndTreeView, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvOtherProfiles));
	}

	if ( !resetting ) {
		if (requestedRootExpansion) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvRoot));
		}
		if (requestedUserProfilesExpansion) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvUserProfiles));
		}
		if (requestedSystemProfilesExpansion) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvSystemProfiles));
		}
		if (requestedOtherProfilesExpansion) {
			SendMessage(hwndTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvOtherProfiles));
		}
	}
}

// Subclass procedure for edit control
//
INT_PTR CALLBACK EditSubclassProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	switch (uMessage) {

		// This prevents us from auto-selecting all text when we get the focus
		//
		case WM_GETDLGCODE:
			INT_PTR temp;
			temp = CallWindowProcW(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
			return temp & ~DLGC_HASSETSEL;
			break;

		// This enables control-A as a hotkey for Select All
		//
		case WM_CHAR:
			if (1 == wParam) {
				uMessage = EM_SETSEL;
				wParam = 0;
				lParam = -1;
			}
			return CallWindowProc(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
			break;

	}
	return CallWindowProc(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
}

// Monitor page dialog proc
//
INT_PTR CALLBACK MonitorPage::MonitorPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);

	MonitorPage * thisPage;
	PROPSHEETPAGE * psp;
	WINDOWINFO wiParent;
	RECT tvRect;
	RECT edRect;
	RECT rect;
	POINT cp;
	int xPosNew;
	HTREEITEM hSelection;

	switch (uMessage) {
		case WM_INITDIALOG:
		{

			// Stuff our 'this' pointer into the dialog's window words
			//
			psp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
			thisPage = reinterpret_cast<MonitorPage *>(psp->lParam);
			SetWindowLongPtr(hWnd, DWLP_USER, (__int3264)(LONG_PTR)thisPage);

			// Save some things in our object
			//
			thisPage->hwnd = hWnd;
			thisPage->hwndTreeView = GetDlgItem(hWnd, IDC_TREE1);
			thisPage->hwndEdit = GetDlgItem(hWnd, IDC_MONITOR_TEXT);

			// Subclass the edit control
			//
			oldEditWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(thisPage->hwndEdit, GWLP_WNDPROC, (__int3264)(LONG_PTR)EditSubclassProc);

			// This page and its subwindows should grow with resizing
			//
			ANCHOR_PRESET anchorPreset;
			anchorPreset.hwnd = hWnd;
			anchorPreset.anchorLeft = true;
			anchorPreset.anchorTop = true;
			anchorPreset.anchorRight = true;
			anchorPreset.anchorBottom = true;
			Resize::AddAchorPreset(anchorPreset);

			anchorPreset.hwnd = thisPage->hwndEdit;
			Resize::AddAchorPreset(anchorPreset);

			anchorPreset.hwnd = thisPage->hwndTreeView;
			anchorPreset.anchorRight = false;
			Resize::AddAchorPreset(anchorPreset);

			// Get the page's WINDOWINFO
			//
			SecureZeroMemory(&wiParent, sizeof(wiParent));
			wiParent.cbSize = sizeof(wiParent);
			GetWindowInfo(hWnd, &wiParent);

			// If we have a TreeView width to restore, set it up before accounting for resizing
			//
			if (thisPage->requestedTreeViewWidth) {
				GetWindowRect(thisPage->hwndTreeView, &rect);
				int widthDelta = thisPage->requestedTreeViewWidth - (rect.right - rect.left);
				if ( 0 != widthDelta ) {

					rect.left -= wiParent.rcClient.left;
					rect.top -= wiParent.rcClient.top;
					rect.right -= wiParent.rcClient.left;
					rect.bottom -= wiParent.rcClient.top;
					rect.right += widthDelta;
					MoveWindow(thisPage->hwndTreeView, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);

					GetWindowRect(thisPage->hwndEdit, &rect);
					rect.left -= wiParent.rcClient.left;
					rect.top -= wiParent.rcClient.top;
					rect.right -= wiParent.rcClient.left;
					rect.bottom -= wiParent.rcClient.top;
					rect.left += widthDelta;
					MoveWindow(thisPage->hwndEdit, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
				}
			}

			// Fix up the size of the controls in case the Summary tab was resized
			// before this tab was created.
			//
			if ( 0 != hwnd_IDC_ORIGINAL_SIZE && 0 != hwnd_IDC_RESIZED ) {
				RECT originalSize;
				RECT newSize;
				SecureZeroMemory(&originalSize, sizeof(originalSize));
				GetClientRect(hwnd_IDC_ORIGINAL_SIZE, &originalSize);
				SecureZeroMemory(&newSize, sizeof(newSize));
				GetClientRect(hwnd_IDC_RESIZED, &newSize);
				SIZE sizeDelta;
				sizeDelta.cx = newSize.right - originalSize.right;
				sizeDelta.cy = newSize.bottom - originalSize.bottom;
				if ( (0 != sizeDelta.cx) || (0 != sizeDelta.cy) ) {
					GetWindowRect(thisPage->hwndEdit, &rect);
					rect.left -= wiParent.rcClient.left;
					rect.top -= wiParent.rcClient.top;
					rect.right -= wiParent.rcClient.left;
					rect.bottom -= wiParent.rcClient.top;
					rect.right += sizeDelta.cx;
					rect.bottom += sizeDelta.cy;
					MoveWindow(thisPage->hwndEdit, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);

					GetWindowRect(thisPage->hwndTreeView, &rect);
					rect.left -= wiParent.rcClient.left;
					rect.top -= wiParent.rcClient.top;
					rect.right -= wiParent.rcClient.left;
					rect.bottom -= wiParent.rcClient.top;
					rect.bottom += sizeDelta.cy;
					MoveWindow(thisPage->hwndTreeView, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
				}
			}

			// Tell the resizing system that its window list is out of date
			//
			Resize::SetNeedRebuild(true);

			// Try setting the fonts in the TreeView and Edit controls, overriding the dialog template
			//
			HDC hdc = GetDC(hWnd);
			HFONT hFont = GetFont(hdc, FC_DIALOG, true);
			ReleaseDC(hWnd, hdc);
			SendMessage(thisPage->hwndTreeView, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(thisPage->hwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

			// Build the TreeView
			//
			thisPage->BuildTreeView();

			DWORD tabSpacing = 12;
			SendMessage(thisPage->hwndEdit, EM_SETTABSTOPS, 1, reinterpret_cast<LPARAM>(&tabSpacing));
			break;
		}

		// Force the page to be white (instead of COLOR_BTNFACE)
		//
		case WM_CTLCOLORDLG:
			return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
			break;

		// Force the read-only edit control to be white (instead of COLOR_BTNFACE)
		//
		case WM_CTLCOLORSTATIC:
			return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
			break;

		// Handle context menu requests from TreeView items
		//
		case WM_CONTEXTMENU:
			thisPage = reinterpret_cast<MonitorPage *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DWLP_USER)));
			if (0 == thisPage) {
				break;
			}
			if ( thisPage->hwndTreeView == reinterpret_cast<HWND>(wParam) ) {
				TVITEMEX itemEx;
				SecureZeroMemory(&itemEx, sizeof(itemEx));
				itemEx.mask = TVIF_PARAM;
				POINT pt;
				pt.x = static_cast<signed short>(LOWORD(lParam));
				pt.y = static_cast<signed short>(HIWORD(lParam));
				if ( -1 == pt.x && -1 == pt.y ) {

					// The WM_CONTEXTMENU is from a Shift+F10 keypress on the selected item:
					// find the selected item and where to pop up the menu
					//
					itemEx.hItem = reinterpret_cast<HTREEITEM>(SendMessage(thisPage->hwndTreeView, TVM_GETNEXTITEM, TVGN_CARET, 0));
					RECT itemRect;
					*reinterpret_cast<HTREEITEM *>(&itemRect) = itemEx.hItem;
					SendMessage(thisPage->hwndTreeView, TVM_GETITEMRECT, TRUE, reinterpret_cast<LPARAM>(&itemRect));
					//pt.x = itemRect.left + (itemRect.right - itemRect.left) / 2;
					//pt.y = itemRect.top + (itemRect.bottom - itemRect.top) / 2;
					pt.x = itemRect.right - 8;
					pt.y = itemRect.bottom - 8;
					ClientToScreen(thisPage->hwndTreeView, &pt);
				} else {

					// The WM_CONTEXTMENU is from a mouse right-click: see where the user clicked
					//
					TVHITTESTINFO hitTest;
					hitTest.pt = pt;
					ScreenToClient(thisPage->hwndTreeView, &hitTest.pt);
					hitTest.flags = 0;
					hitTest.hItem = 0;
					SendMessage(thisPage->hwndTreeView, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTest));
					if ( 0 != (hitTest.flags & TVHT_ONITEM) ) {
						itemEx.hItem = hitTest.hItem;
					}
				}
				if ( 0 != itemEx.hItem ) {
					SendMessage(thisPage->hwndTreeView, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&itemEx));
					TreeViewItem * tvItem = reinterpret_cast<TreeViewItem *>(itemEx.lParam);
					if ( 0 != tvItem ) {
						tvItem->Handle_WM_CONTEXTMENU(thisPage, &pt);
					}
				}
			}
			break;

		// WM_NOTIFY is sent by our PropertySheet parent and by the TreeView control
		//
		case WM_NOTIFY:
			NMHDR * pNMHDR;
			pNMHDR = reinterpret_cast<NMHDR *>(lParam);

			thisPage = reinterpret_cast<MonitorPage *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DWLP_USER)));
			if (0 == thisPage) {

				// The TreeView likes to send us NM_CUSTOMDRAW notification before the PropertySheet
				// has called us with WM_INITDIALOG.  This means that we don't have our object pointer
				// stored yet, so we're really not interested in these notifications.  Just return.
				//
				break;
			}

			switch (pNMHDR->code) {

				case PSN_APPLY:
					//PSHNOTIFY * pPSHNOTIFY;
					//pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
					//MessageBox(NULL, L"PSN_APPLY", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
					break;

				// Dialog is closing
				//
				case PSN_RESET:
					//PSHNOTIFY * pPSHNOTIFY;
					//pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
					//MessageBox(NULL, L"PSN_RESET", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
					break;

				// Page activated (switched to)
				//
				case PSN_SETACTIVE:
					//PSHNOTIFY * pPSHNOTIFY;
					//pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
					//MessageBox(NULL, L"PSN_SETACTIVE", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);

					// If showing the text for the monitor, refresh it in case the Load LUT button has been pressed
					//
					if (thisPage->hwndTreeView) {
						hSelection = reinterpret_cast<HTREEITEM>(
								SendMessage( thisPage->hwndTreeView, TVM_GETNEXTITEM, TVGN_CARET, 0 ) );
						if (hSelection && (hSelection == thisPage->tvRoot) ) {
							thisPage->SetEditControlText(thisPage->monitor->SummaryString());
						}
					}
					break;

				// Leaving page
				//
				case PSN_KILLACTIVE:
					//PSHNOTIFY * pPSHNOTIFY;
					//pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
					//MessageBox(NULL, L"PSN_KILLACTIVE", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
					break;

				// User pressed Cancel
				//
				case PSN_QUERYCANCEL:
					//PSHNOTIFY * pPSHNOTIFY;
					//pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
					//MessageBox(NULL, L"PSN_QUERYCANCEL", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
					break;

				// TreeView selection changed
				//
				case TVN_SELCHANGEDW:
					NMTREEVIEWW * pNMTREEVIEWW;
					pNMTREEVIEWW = reinterpret_cast<NMTREEVIEWW *>(lParam);

					// Call the associated TreeViewItem object to handle the selection
					//
					TreeViewItem * tiObject;
					tiObject = reinterpret_cast<TreeViewItem *>(pNMTREEVIEWW->itemNew.lParam);
					tiObject->Handle_TVN_SELCHANGEDW(thisPage, pNMTREEVIEWW);
					break;

				// Convert a right-click in the TreeView into a WM_CONTEXTMENU message
				//
				case NM_RCLICK:
					if ( pNMHDR->hwndFrom == thisPage->hwndTreeView ) {
						SendMessage(hWnd, WM_CONTEXTMENU, reinterpret_cast<WPARAM>(thisPage->hwndTreeView), GetMessagePos());
						SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, TRUE);
						return TRUE;										// Suppress default handling of NM_RCLICK
					}
					break;

			}
			break;

		// We handle four window messages to implement a splitter control (which is really just a blank
		//  area in the dialog that we handle specially).
		//
		// WM_SETCURSOR -- we change the cursor when the mouse is over our splitter
		//
		case WM_SETCURSOR:
			GetWindowRect(GetDlgItem(hWnd, IDC_TREE1), &tvRect);
			GetWindowRect(GetDlgItem(hWnd, IDC_MONITOR_TEXT), &edRect);
			GetCursorPos(&cp);
			if (cp.x >= tvRect.right && cp.x < edRect.left && cp.y >= tvRect.top && cp.y < tvRect.bottom) {
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));
				SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, TRUE); 
				return TRUE;
			}
			break;

		// We set capture on WM_LBUTTONDOWN and record our position
		//
		case WM_LBUTTONDOWN:
			static int xLeft;
			static int xRight;
			static int xLeftLimit;
			static int xRightLimit;
			static int xOffset;
			static bool splitterActive;
			int xPos;
			int yPos;
			xPos = static_cast<signed short>(LOWORD(lParam));
			yPos = static_cast<signed short>(HIWORD(lParam));
			GetWindowRect(GetDlgItem(hWnd, IDC_TREE1), &tvRect);
			GetWindowRect(GetDlgItem(hWnd, IDC_MONITOR_TEXT), &edRect);
			SecureZeroMemory(&wiParent, sizeof(wiParent));
			wiParent.cbSize = sizeof(wiParent);
			GetWindowInfo(hWnd, &wiParent);
			if (xPos >= (tvRect.right - wiParent.rcClient.left) && xPos < (edRect.left - wiParent.rcClient.left) &&
					yPos >= (tvRect.top - wiParent.rcClient.top) && yPos < (tvRect.bottom - wiParent.rcClient.top)) {
				splitterActive = true;
				SetCapture(hWnd);

				int minimumTreeviewWidth = static_cast<int>(MINIMUM_TREEVIEW_WIDTH * dpiScale);
				int minimumEditWidth = static_cast<int>(MINIMUM_EDIT_WIDTH * dpiScale);
				xLeft = (tvRect.right - wiParent.rcClient.left);
				xLeftLimit = (tvRect.left - wiParent.rcClient.left) + minimumTreeviewWidth;
				xRight = (edRect.left - wiParent.rcClient.left);
				xRightLimit = (edRect.right - wiParent.rcClient.left) - minimumEditWidth;
				xOffset = xPos - xLeft;
			}
			break;

		// We do the resizing/moving of windows on WM_MOUSEMOVE
		//
		case WM_MOUSEMOVE:
			if (splitterActive) {

				// During sizing, we will get "negative" client coordinates if the mouse is dragged
				//  to the left of the client window, so we need 0xFFFF to become 0xFFFFFFFF, not
				//  0x0000FFFF ... hence the cast to signed short.
				//
				xPosNew = static_cast<signed short>(LOWORD(lParam));
				HWND treeControlHwnd;
				HWND editControlHwnd;

				treeControlHwnd = GetDlgItem(hWnd, IDC_TREE1);
				GetWindowRect(treeControlHwnd, &tvRect);
				editControlHwnd = GetDlgItem(hWnd, IDC_MONITOR_TEXT);
				GetWindowRect(editControlHwnd, &edRect);
				SecureZeroMemory(&wiParent, sizeof(wiParent));
				wiParent.cbSize = sizeof(wiParent);
				GetWindowInfo(hWnd, &wiParent);

				xLeft = xPosNew - xOffset;
				if (xLeft < xLeftLimit) {
					xLeft = xLeftLimit;
				}
				if (xLeft > xRightLimit) {
					xLeft = xRightLimit;
				}
				xRight = xLeft + (edRect.left - tvRect.right);

				// Resize the windows
				//
				SetLastError(0);
				HDWP hdwp = BeginDeferWindowPos(2);
				hdwp = DeferWindowPos( hdwp, treeControlHwnd, 0,
						tvRect.left - wiParent.rcClient.left,
						tvRect.top - wiParent.rcClient.top,
						xLeft - (tvRect.left - wiParent.rcClient.left),
						tvRect.bottom - tvRect.top,
						SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER );
						//SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER );
				//InvalidateRect(treeControlHwnd, NULL, TRUE);
				hdwp = DeferWindowPos( hdwp, editControlHwnd, 0,
						xRight,
						edRect.top - wiParent.rcClient.top,
						edRect.right - xRight - wiParent.rcClient.left,
						edRect.bottom - edRect.top,
						SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER );
						//SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER );
				//InvalidateRect(editControlHwnd, NULL, TRUE);
				//InvalidateRect(hWnd, NULL, TRUE);
				if (hdwp) {
					BOOL retVal = EndDeferWindowPos(hdwp);
					DWORD err = GetLastError();
					if ( err || (0 == retVal) ) {
						ReleaseCapture();
						splitterActive = false;
						Resize::SetNeedRebuild(true);
						PostMessage(GetParent(hWnd), WM_SIZE, 0, 0);
						wstring s = ShowError(L"EndDeferWindowPos");
						wchar_t errorMessageCaption[256];
						extern HINSTANCE g_hInst;
						LoadString(g_hInst, IDS_ERROR, errorMessageCaption, _countof(errorMessageCaption));
						MessageBox(NULL, s.c_str(), errorMessageCaption, MB_ICONINFORMATION | MB_OK);
					}
				}
			}
			break;

		// On WM_LBUTTONUP we release the capture and signal our subclass procedure for the PropertySheet
		//  that its resizing database is now out of date (because we moved windows ouselves) and so it
		//  needs to rebuild the database.  It does not actually have to redraw anything at this point.
		//
		case WM_LBUTTONUP:
			if (splitterActive) {
				ReleaseCapture();
				splitterActive = false;
				Resize::SetNeedRebuild(true);
				PostMessage(GetParent(hWnd), WM_SIZE, 0, 0);
			}
			break;

	}
	return 0;
}
