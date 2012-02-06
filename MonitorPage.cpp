// MonitorPage.cpp -- Display a property sheet page for each monitor
//

#include "stdafx.h"
#include <commctrl.h>
//#include <uxtheme.h>
//#include <vssym32.h>
#include "Monitor.h"
#include "MonitorPage.h"
#include "PropertySheet.h"
#include "Resize.h"
#include "TreeViewItem.h"
#include "Utility.h"
#include "resource.h"
#include <strsafe.h>

// Global externs defined in this file
//
extern WNDPROC oldEditWindowProc = 0;				// The original Edit control's window procedure

// Symbols defined in other files
//
extern double dpiScale;								// Scaling factor for dots per inch (actual versus standard 96 DPI)

// Constructor
//
MonitorPage::MonitorPage(Monitor * hostMonitor) :
		monitor(hostMonitor),
		tvRoot(0),
		tvUserProfiles(0),
		tvSystemProfiles(0)
{
}

void MonitorPage::SetEditControlText(wstring newText) {
	SetDlgItemText(savedHWND, IDC_MONITOR_TEXT, newText.c_str());
}

Monitor * MonitorPage::GetMonitor(void) const {
	return monitor;
}

// Build the TreeView
//
void MonitorPage::BuildTreeView(HWND treeControlHwnd) {

	ProfileList profileList;
	HTREEITEM tvItem;
	size_t count;
	wchar_t buf[1024];
	TreeViewItem * tiObject = 0;

	TVINSERTSTRUCT tvInsertStruct;
	SecureZeroMemory(&tvInsertStruct, sizeof(tvInsertStruct));
	tvInsertStruct.hInsertAfter = TVI_ROOT;
	tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tvInsertStruct.itemex.state = TVIS_BOLD;
	tvInsertStruct.itemex.stateMask = TVIS_BOLD;
	StringCbCopy(buf, sizeof(buf), monitor->GetDeviceString().c_str());
	tvInsertStruct.itemex.pszText = buf;
	tiObject = TreeViewItem::Add(new TreeViewItem(TREEVIEW_ITEM_TYPE_MONITOR, tvInsertStruct.itemex.pszText)); 
	tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
	tvRoot = reinterpret_cast<HTREEITEM>( SendMessage(treeControlHwnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
	tiObject->SetHTREEITEM(tvRoot);
	tiObject->SetMonitor(monitor);

	if (VistaOrHigher()) {

		bool hiliteUser = monitor->GetActiveProfileIsUserProfile();
		tvInsertStruct.hParent = tvRoot;
		tvInsertStruct.hInsertAfter = TVI_LAST;
		if (hiliteUser) {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
		}
		tvInsertStruct.itemex.pszText = L"User profiles";
		tiObject = TreeViewItem::Add(new TreeViewItem(TREEVIEW_ITEM_TYPE_USER_PROFILES, tvInsertStruct.itemex.pszText)); 
		tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
		tvUserProfiles = reinterpret_cast<HTREEITEM>( SendMessage(treeControlHwnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
		tiObject->SetHTREEITEM(tvUserProfiles);
		tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM;

		profileList = monitor->GetProfileList(true);
		count = profileList.size();
		tvInsertStruct.hParent = tvUserProfiles;
		for (size_t i = 0; i < count; ++i) {
			if ( profileList[i] == monitor->GetUserProfile() ) {
				tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
			}
			StringCbCopy(buf, sizeof(buf), profileList[i]->GetName().c_str());
			tvInsertStruct.itemex.pszText = buf;
			tiObject = TreeViewItem::Add(new TreeViewItem(TREEVIEW_ITEM_TYPE_PROFILE, tvInsertStruct.itemex.pszText)); 
			tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
			tvItem = reinterpret_cast<HTREEITEM>( SendMessage(treeControlHwnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
			tiObject->SetHTREEITEM(tvItem);
			tiObject->SetProfile(profileList[i]);
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM;
		}
		SendMessage(treeControlHwnd, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvUserProfiles));

		tvInsertStruct.hParent = tvRoot;
		if (!hiliteUser) {
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
		}
		tvInsertStruct.itemex.pszText = L"System profiles";
		tiObject = TreeViewItem::Add(new TreeViewItem(TREEVIEW_ITEM_TYPE_SYSTEM_PROFILES, tvInsertStruct.itemex.pszText)); 
		tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
		tvSystemProfiles = reinterpret_cast<HTREEITEM>( SendMessage(treeControlHwnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
		tiObject->SetHTREEITEM(tvSystemProfiles);
		tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM;

		profileList = monitor->GetProfileList(false);
		count = profileList.size();
		tvInsertStruct.hParent = tvSystemProfiles;
		for (size_t i = 0; i < count; ++i) {
			if ( profileList[i] == monitor->GetSystemProfile() ) {
				tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
			}
			StringCbCopy(buf, sizeof(buf), profileList[i]->GetName().c_str());
			tvInsertStruct.itemex.pszText = buf;
			tiObject = TreeViewItem::Add(new TreeViewItem(TREEVIEW_ITEM_TYPE_PROFILE, tvInsertStruct.itemex.pszText)); 
			tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
			tvItem = reinterpret_cast<HTREEITEM>( SendMessage(treeControlHwnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
			tiObject->SetHTREEITEM(tvItem);
			tiObject->SetProfile(profileList[i]);
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM;
		}
		SendMessage(treeControlHwnd, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvSystemProfiles));

	} else {
		tvInsertStruct.hParent = tvRoot;
		tvInsertStruct.itemex.pszText = L"Profiles";
		tiObject = TreeViewItem::Add(new TreeViewItem(TREEVIEW_ITEM_TYPE_PROFILES, tvInsertStruct.itemex.pszText)); 
		tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
		tvSystemProfiles = reinterpret_cast<HTREEITEM>( SendMessage(treeControlHwnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
		tiObject->SetHTREEITEM(tvSystemProfiles);

		profileList = monitor->GetProfileList(false);
		count = profileList.size();
		tvInsertStruct.hParent = tvSystemProfiles;
		for (size_t i = 0; i < count; ++i) {
			if ( profileList[i] == monitor->GetSystemProfile() ) {
				tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
			}
			StringCbCopy(buf, sizeof(buf), profileList[i]->GetName().c_str());
			tvInsertStruct.itemex.pszText = buf;
			tiObject = TreeViewItem::Add(new TreeViewItem(TREEVIEW_ITEM_TYPE_PROFILE, tvInsertStruct.itemex.pszText)); 
			tvInsertStruct.itemex.lParam = reinterpret_cast<LPARAM>(tiObject);
			tvItem = reinterpret_cast<HTREEITEM>( SendMessage(treeControlHwnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)) );
			tiObject->SetHTREEITEM(tvItem);
			tiObject->SetProfile(profileList[i]);
			tvInsertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM;
		}
		SendMessage(treeControlHwnd, TVM_SORTCHILDREN, FALSE, reinterpret_cast<LPARAM>(tvSystemProfiles));
	}
	SendMessage(treeControlHwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(tvRoot));
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

#if 0
		case WM_ERASEBKGND:
			RECT rect;
			GetClientRect(hWnd, &rect);
			HDC hdc;
			hdc = GetDC(hWnd);
			FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));
			//FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNHIGHLIGHT + 1));
			ReleaseDC(hWnd, hdc);
			return 1;
#endif

	}
	return CallWindowProc(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
}

// Monitor page dialog proc
//
INT_PTR CALLBACK MonitorPage::MonitorPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);

	MonitorPage * thisPage;
	PROPSHEETPAGE * psp;
	HWND treeControlHwnd;
	HWND editControlHwnd;
	WINDOWINFO wiParent;
	RECT tvRect;
	RECT edRect;
	RECT rect;
	POINT cp;
	int xPosNew;

	switch (uMessage) {
		case WM_INITDIALOG:
		{

			// Remember our size before any resizing as our minimum size
			//
			if ( 0 == minimumWindowSize.cx ) {
				GetWindowRect(GetParent(hWnd), &rect);
				minimumWindowSize.cx = rect.right - rect.left;
				minimumWindowSize.cy = rect.bottom - rect.top;
			}

			// Stuff our 'this' pointer into the dialog's window words
			//
			psp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
			thisPage = reinterpret_cast<MonitorPage *>(psp->lParam);
			SetWindowLongPtr(hWnd, DWLP_USER, (__int3264)(LONG_PTR)thisPage);

			// Save some things in our object
			//
			thisPage->savedHWND = hWnd;

			// Subclass the edit control
			//
			editControlHwnd = GetDlgItem(hWnd, IDC_MONITOR_TEXT);
			oldEditWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(editControlHwnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)EditSubclassProc);

			// This page and its subwindows should grow with resizing
			//
			ANCHOR_PRESET anchorPreset;
			anchorPreset.hwnd = hWnd;
			anchorPreset.anchorLeft = true;
			anchorPreset.anchorTop = true;
			anchorPreset.anchorRight = true;
			anchorPreset.anchorBottom = true;
			Resize::AddAchorPreset(anchorPreset);

			anchorPreset.hwnd = editControlHwnd;
			Resize::AddAchorPreset(anchorPreset);

			treeControlHwnd = GetDlgItem(hWnd, IDC_TREE1);
			anchorPreset.hwnd = treeControlHwnd;
			anchorPreset.anchorRight = false;
			Resize::AddAchorPreset(anchorPreset);

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
				SecureZeroMemory(&wiParent, sizeof(wiParent));
				wiParent.cbSize = sizeof(wiParent);
				GetWindowInfo(hWnd, &wiParent);

				GetWindowRect(editControlHwnd, &rect);
				rect.left -= wiParent.rcClient.left;
				rect.top -= wiParent.rcClient.top;
				rect.right -= wiParent.rcClient.left;
				rect.bottom -= wiParent.rcClient.top;
				rect.right += sizeDelta.cx;
				rect.bottom += sizeDelta.cy;
				MoveWindow(editControlHwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);

				GetWindowRect(treeControlHwnd, &rect);
				rect.left -= wiParent.rcClient.left;
				rect.top -= wiParent.rcClient.top;
				rect.right -= wiParent.rcClient.left;
				rect.bottom -= wiParent.rcClient.top;
				rect.bottom += sizeDelta.cy;
				MoveWindow(treeControlHwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
			}

			// Tell the resizing system that its window list is out of date
			//
			Resize::SetNeedRebuild(true);

			// Try setting the fonts in the TreeView and Edit controls, ignoring the dialog template
			//
			HDC hdc = GetDC(hWnd);
			HFONT hFont = GetFont(hdc, FC_INFORMATION, true);
			ReleaseDC(hWnd, hdc);
			SendMessage(treeControlHwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(editControlHwnd, WM_SETFONT, (WPARAM)hFont, TRUE);

			//SetWindowTheme(treeControlHwnd, L"Explorer", 0);

			// Build the TreeView
			//
			thisPage->BuildTreeView(treeControlHwnd);

			//SendDlgItemMessage(hWnd, IDC_MONITOR_TEXT, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), FALSE);
			DWORD tabSpacing = 12;
			//SendDlgItemMessage(hWnd, IDC_MONITOR_TEXT, EM_SETTABSTOPS, 0, (LPARAM)&tabSpacing);
			SendDlgItemMessage(hWnd, IDC_MONITOR_TEXT, EM_SETTABSTOPS, 1, (LPARAM)&tabSpacing);
			break;
		}

		// These force the page and read-only edit controls to be white (instead of gray)
		//
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
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
					treeControlHwnd = GetDlgItem(hWnd, IDC_TREE1);

					// Call the associated TreeViewItem object to handle the selection
					//
					TreeViewItem * tiObject;
					tiObject = reinterpret_cast<TreeViewItem *>(pNMTREEVIEWW->itemNew.lParam);
					tiObject->Handle_TVN_SELCHANGEDW(thisPage, pNMTREEVIEWW);
					break;

#if 0
				// InfoTip
				//
				case TVN_GETINFOTIPW:
					NMTVGETINFOTIPW * pNMTVGETINFOTIPW;
					pNMTVGETINFOTIPW = reinterpret_cast<NMTVGETINFOTIPW *>(lParam);
					StringCbCopy(pNMTVGETINFOTIPW->pszText, pNMTVGETINFOTIPW->cchTextMax, L"Tad's wonderful InfoTip");
					static int count_TVN_GETINFOTIPW;
					++count_TVN_GETINFOTIPW;
					break;
#endif

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
						SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER );
				InvalidateRect(treeControlHwnd, NULL, TRUE);
				hdwp = DeferWindowPos( hdwp, editControlHwnd, 0,
						xRight,
						edRect.top - wiParent.rcClient.top,
						edRect.right - xRight - wiParent.rcClient.left,
						edRect.bottom - edRect.top,
						SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER );
				InvalidateRect(editControlHwnd, NULL, TRUE);
				InvalidateRect(hWnd, NULL, TRUE);
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
