// PropertySheet.cpp -- Display a property sheet
//

#include "stdafx.h"
#include <commctrl.h>
#include <uxtheme.h>
#include "LUTview.h"
#include "Monitor.h"
#include "MonitorPage.h"
#include "MonitorSummaryItem.h"
#include "PropertySheet.h"
#include "Resize.h"
#include "resource.h"
#include "Utility.h"
#include <strsafe.h>

//#include <shellapi.h>

// Optional "features"
//
#define SHOW_EXTRA_COPY_OF_FIRST_MONITOR 1
#define SHOW_LUTVIEW_OF_FIRST_MONITOR 1
#define SHOW_PROFILE_IN_LUTVIEW_INSTEAD_OF_MONITOR 0

// Some constants
//
#define INITIAL_VERTICAL_OFFSET (7)
#define HORIZONTAL_PADDING_LEFT (7)
#define HORIZONTAL_PADDING_RIGHT (16 - 7)
#define EXTRA_VERTICAL_SPACING_BETWEEN_ITEMS (4)
#define FRAME_COLOR RGB(192,192,192)
#define STOCK_OBJECT_FOR_BACKGROUND WHITE_BRUSH
//#define LV_SIZE 220

// Symbols defined in other files
//
extern HINSTANCE g_hInst;

// Global externs defined in this file
//
extern WNDPROC oldEditWindowProc = 0;				// The original Edit control's window procedure
extern HWND hwnd_IDC_ORIGINAL_SIZE = 0;				// The window handle of the IDC_ORIGINAL_SIZE static control
extern HWND hwnd_IDC_RESIZED = 0;					// The window handle of the IDC_RESIZE static control
extern SIZE minimumWindowSize = {0};				// The minimum size of the PropertySheet window

// Global static symbols internal to this file
//
static WNDPROC oldPropSheetWindowProc = 0;			// The original PropertySheet's window procedure
static MonitorSummaryItem * testMonitorItem = 0;	// The single LUT viewer on the Summary page
static HWND hwndTestMonitorItem = 0;				// HWND for test MonitorSummaryItem we created
static bool summaryPageWasShown = false;			// Just a flag that the PropertySheet at least started us

#if SHOW_LUTVIEW_OF_FIRST_MONITOR
static LUTview * summaryLutView = 0;				// The single LUT viewer on the Summary page
static HWND hwndSummaryLUT = 0;						// HWND for LUTview we created
#endif

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

// Summary page dialog procedure
//
INT_PTR CALLBACK SummaryPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	RECT rect;
	HDC hdc;
	int desiredHeight;
	WINDOWINFO wi;
	int yOffset;

	switch (uMessage) {
		case WM_INITDIALOG:
		{
			// This is a flag to reduce seeing bogus error reporting on return from
			// PropertySheet().  It can sometimes fail without setting a negative
			// return code, and our check for GetLastError() is required.  But we can't
			// just check GetLastError(), because there may well be a last error even
			// if it all worked.  So, we won't check GetLastError() if we've at least
			// received the WM_INITDIALOG.
			//
			summaryPageWasShown = true;

			// Remember our size before any resizing as our minimum size
			//
			if ( 0 == minimumWindowSize.cx ) {
				GetWindowRect(GetParent(hWnd), &rect);
				minimumWindowSize.cx = rect.right - rect.left;
				minimumWindowSize.cy = rect.bottom - rect.top;
			}

			// Subclass the edit control
			//
			//oldEditWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(GetDlgItem(hWnd, IDC_SUMMARY_TEXT), GWLP_WNDPROC, (__int3264)(LONG_PTR)EditSubclassProc);

#if 0
			// See if we should show the "Use Windows display calibration" checkbox, and if so, set it
			//
			typedef BOOL (WINAPI * PFN_WcsGetCalibrationManagementState)(BOOL *);
			HINSTANCE hMSCMS = GetModuleHandle(L"mscms.dll");
			if (hMSCMS) {
				PFN_WcsGetCalibrationManagementState p_WcsGetCalibrationManagementState =
							(PFN_WcsGetCalibrationManagementState)GetProcAddress(hMSCMS, "WcsGetCalibrationManagementState");
				if (p_WcsGetCalibrationManagementState) {

					// Show the checkbox ...
					//
					ShowWindow(GetDlgItem(hWnd, IDC_USE_WINDOWS_DISPLAY_CALIBRATION), SW_SHOW);
					BOOL managementState = FALSE;
					p_WcsGetCalibrationManagementState(&managementState);
					SendDlgItemMessage(
							hWnd,
							IDC_USE_WINDOWS_DISPLAY_CALIBRATION,
							BM_SETCHECK,
							managementState ? BST_CHECKED : BST_UNCHECKED,
							0 );
				}
			}
#endif

			yOffset = INITIAL_VERTICAL_OFFSET;

#if SHOW_EXTRA_COPY_OF_FIRST_MONITOR

			// Create and show a monitor summary item
			//
			testMonitorItem = MonitorSummaryItem::Add(new MonitorSummaryItem(Monitor::Get(0)));

			SecureZeroMemory(&wi, sizeof(wi));
			wi.cbSize = sizeof(wi);
			GetWindowInfo(hWnd, &wi);

			hdc = GetDC(hWnd);
			desiredHeight = testMonitorItem->GetDesiredHeight(hdc);
			ReleaseDC(hWnd, hdc);

			hwndTestMonitorItem = testMonitorItem->CreateMonitorSummaryItemWindow(
					hWnd,
					HORIZONTAL_PADDING_LEFT,
					yOffset,
					wi.rcClient.right - wi.rcClient.left - (HORIZONTAL_PADDING_LEFT + HORIZONTAL_PADDING_RIGHT),
					desiredHeight );
			SetWindowLongPtr(hwndTestMonitorItem, GWLP_ID, 0x123);

			GetWindowRect(hwndTestMonitorItem, &rect);
			yOffset = rect.bottom - wi.rcClient.top + EXTRA_VERTICAL_SPACING_BETWEEN_ITEMS;
#endif

			// This page and its subwindows should grow with resizing
			//
			ANCHOR_PRESET anchorPreset;
			anchorPreset.hwnd = hWnd;
			anchorPreset.anchorLeft = true;
			anchorPreset.anchorTop = true;
			anchorPreset.anchorRight = true;
			anchorPreset.anchorBottom = true;
			Resize::AddAchorPreset(anchorPreset);

			// These two hidden windows are used to track resizing prior to
			// other pages being created.  We set IDC_RESIZED to grow in all
			// dimensions, and set IDC_ORIGINAL_SIZE to not grow at all.
			// By comparing their sizes, we can fix up the sizes and positions
			// of newly created controls on other pages.
			//
			hwnd_IDC_RESIZED = GetDlgItem(hWnd, IDC_RESIZED);
			anchorPreset.hwnd = hwnd_IDC_RESIZED;
			Resize::AddAchorPreset(anchorPreset);

			anchorPreset.anchorLeft = false;
			anchorPreset.anchorTop = false;
			anchorPreset.anchorRight = false;
			anchorPreset.anchorBottom = false;
			hwnd_IDC_ORIGINAL_SIZE = GetDlgItem(hWnd, IDC_ORIGINAL_SIZE);
			anchorPreset.hwnd = hwnd_IDC_ORIGINAL_SIZE;
			Resize::AddAchorPreset(anchorPreset);

#if 0
			anchorPreset.anchorLeft = true;
			anchorPreset.anchorBottom = true;
			anchorPreset.hwnd = GetDlgItem(hWnd, IDC_USE_WINDOWS_DISPLAY_CALIBRATION);
			Resize::AddAchorPreset(anchorPreset);

			anchorPreset.hwnd = hwndTestMonitorItem;
			Resize::AddAchorPreset(anchorPreset);

			anchorPreset.anchorBottom = false;
			anchorPreset.hwnd = GetDlgItem(hWnd, IDC_SUMMARY_TEXT);
			Resize::AddAchorPreset(anchorPreset);
#endif

			//HTHEME hTheme = OpenThemeData(hWnd, L"Button");

#if 1
			// Build the contents of the Summary page
			//
			//wstring summaryString;

			SecureZeroMemory(&wi, sizeof(wi));
			wi.cbSize = sizeof(wi);
			GetWindowInfo(hWnd, &wi);

			size_t count = Monitor::GetListSize();
			for (size_t i = 0; i < count; ++i) {

				MonitorSummaryItem * mi = 0;
				HWND hw = 0;

				// Create and show a monitor summary item
				//
				mi = MonitorSummaryItem::Add(new MonitorSummaryItem(Monitor::Get(i)));

				//int yOffset = INITIAL_VERTICAL_OFFSET;

				hdc = GetDC(hWnd);
				desiredHeight = mi->GetDesiredHeight(hdc);
				ReleaseDC(hWnd, hdc);

				hw = mi->CreateMonitorSummaryItemWindow(
						hWnd,
						HORIZONTAL_PADDING_LEFT,
						yOffset,
						wi.rcClient.right - wi.rcClient.left - (HORIZONTAL_PADDING_LEFT + HORIZONTAL_PADDING_RIGHT),
						desiredHeight );
				GetWindowRect(hw, &rect);
				yOffset = rect.bottom - wi.rcClient.top + EXTRA_VERTICAL_SPACING_BETWEEN_ITEMS;
			}
			//if ( 0 == count ) {
			//	wchar_t noMonitorsFound[256];
			//	LoadString(g_hInst, IDS_NO_MONITORS_FOUND, noMonitorsFound, _countof(noMonitorsFound));
			//	SetDlgItemText(hWnd, IDC_SUMMARY_TEXT, noMonitorsFound);
			//} else {
			//	DWORD tabSpacing = 8;
			//	SendDlgItemMessage(hWnd, IDC_MONITOR_TEXT, EM_SETTABSTOPS, 1, (LPARAM)&tabSpacing);
			//	SetDlgItemText(hWnd, IDC_SUMMARY_TEXT, summaryString.c_str());
			//}
#else
			// Build a display string describing the monitors we found
			//
			wstring summaryString;
			size_t count = Monitor::GetListSize();
			for (size_t i = 0; i < count; ++i) {
				if ( !summaryString.empty() ) {
					summaryString += L"\r\n\r\n";					// Skip a line between monitors
				}
				summaryString += Monitor::Get(i)->SummaryString();
			}
			if ( summaryString.empty() ) {
				wchar_t noMonitorsFound[256];
				LoadString(g_hInst, IDS_NO_MONITORS_FOUND, noMonitorsFound, _countof(noMonitorsFound));
				SetDlgItemText(hWnd, IDC_SUMMARY_TEXT, noMonitorsFound);
			} else {
				DWORD tabSpacing = 8;
				SendDlgItemMessage(hWnd, IDC_MONITOR_TEXT, EM_SETTABSTOPS, 1, (LPARAM)&tabSpacing);
				SetDlgItemText(hWnd, IDC_SUMMARY_TEXT, summaryString.c_str());
			}
#endif

#if SHOW_LUTVIEW_OF_FIRST_MONITOR
			// Create and show the LUT viewer
			//
			summaryLutView = LUTview::Add(new LUTview(L"Monitor"));

			SecureZeroMemory(&wi, sizeof(wi));
			wi.cbSize = sizeof(wi);
			GetWindowInfo(hWnd, &wi);

			hwndSummaryLUT = summaryLutView->CreateLUTviewWindow(
					hWnd,
					HORIZONTAL_PADDING_LEFT,
					yOffset,
					wi.rcClient.right - wi.rcClient.left - (HORIZONTAL_PADDING_LEFT + HORIZONTAL_PADDING_RIGHT),
					wi.rcClient.bottom - wi.rcClient.top - yOffset - EXTRA_VERTICAL_SPACING_BETWEEN_ITEMS );

			anchorPreset.hwnd = hwndSummaryLUT;
			anchorPreset.anchorLeft = true;
			anchorPreset.anchorTop = true;
			anchorPreset.anchorRight = true;
			anchorPreset.anchorBottom = true;
			Resize::AddAchorPreset(anchorPreset);

			// This is only for testing (as is having a LUT Viewer on the Summary page),
			// but just hard-code the viewer to display the first monitor
			//
  #if SHOW_PROFILE_IN_LUTVIEW_INSTEAD_OF_MONITOR
			summaryLutView->SetText(L"Active profile");
			Monitor::Get(0)->GetActiveProfile()->LoadFullProfile(false);
			summaryLutView->SetLUT(Monitor::Get(0)->GetActiveProfile()->GetLutPointer());
  #else
			summaryLutView->SetText(L"Real monitor");
			summaryLutView->SetLUT(Monitor::Get(0)->GetLutPointer());
  #endif
#endif

			// Tell the resizing system that its window list is out of date
			//
			Resize::SetNeedRebuild(true);

			break;
		}

		// These force the page and read-only edit controls to be white (instead of gray)
		//
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
			break;
	}
	return 0;
}

// Subclass procedure for the main PropertySheet
//
INT_PTR CALLBACK PropSheetSubclassProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	switch (uMessage) {

		// Notice changes in window size
		//
		case WM_WINDOWPOSCHANGING:
			WINDOWPOS * pWindowPos;
			pWindowPos = reinterpret_cast<WINDOWPOS *>(lParam);
			if ( 0 == (pWindowPos->flags & SWP_NOSIZE) && Resize::GetNeedRebuild() ) {
				Resize::SetupForResizing(hWnd);
				Resize::SetNeedRebuild(false);
			}
			return CallWindowProc(oldPropSheetWindowProc, hWnd, uMessage, wParam, lParam);
			break;

		// Notice changes in window size
		//
		case WM_WINDOWPOSCHANGED:
			pWindowPos = reinterpret_cast<WINDOWPOS *>(lParam);
			if ( 0 == (pWindowPos->flags & SWP_NOSIZE) ) {

				// Don't react if we haven't hit any of our WM_INITDIALOG calls yet
				//  (shown by minimumWindowSize not set)
				//
				// Don't react to being minimized (new cx and cy are icon-sized)
				//
				if ( (0 != minimumWindowSize.cx) && (pWindowPos->cx >= minimumWindowSize.cx) ) {
					Resize::MainWindowHasResized(*pWindowPos);
				}
			}
			return CallWindowProc(oldPropSheetWindowProc, hWnd, uMessage, wParam, lParam);
			break;

		// Disallow resizing to smaller than the original size
		//
		case WM_GETMINMAXINFO:
			if ( 0 != minimumWindowSize.cx ) {
				MINMAXINFO * pMINMAXINFO;
				pMINMAXINFO = reinterpret_cast<MINMAXINFO *>(lParam);
				pMINMAXINFO->ptMinTrackSize.x = minimumWindowSize.cx;
				pMINMAXINFO->ptMinTrackSize.y = minimumWindowSize.cy;
				return 0;
			}
			break;
	}
	return CallWindowProc(oldPropSheetWindowProc, hWnd, uMessage, wParam, lParam);
}

// PropertySheet callback routine
//
int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM lParam) {
	UNREFERENCED_PARAMETER(hWnd);

	switch(uMsg) {

		case PSCB_PRECREATE:

			// Change the PropertySheet's window styles
			//
			DLGTEMPLATE * lpTemplate;
			lpTemplate = reinterpret_cast<DLGTEMPLATE *>(lParam);
			lpTemplate->style |= (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			break;

		case PSCB_INITIALIZED:

			// Fooling around ... try running with themes (visual styles) disabled
			//
			//SetWindowTheme(hWnd, L" ", L" ");

			// Add the SysTabControl32 control to the anchor list
			//
			HWND tabControl;
			tabControl = FindWindowEx(hWnd, NULL, L"SysTabControl32", NULL);
			if (tabControl) {
				ANCHOR_PRESET anchorPreset;
				anchorPreset.hwnd = tabControl;
				anchorPreset.anchorLeft = true;
				anchorPreset.anchorTop = true;
				anchorPreset.anchorRight = true;
				anchorPreset.anchorBottom = true;
				Resize::AddAchorPreset(anchorPreset);
			}

			// More fun and games ... try changing the font on the tab control
			//
			HDC hdc = GetDC(hWnd);
			HFONT hFont = GetFont(hdc, FC_INFORMATION);
			ReleaseDC(hWnd, hdc);
			SendMessage(tabControl, WM_SETFONT, (WPARAM)hFont, TRUE);

			// Subclass the property sheet
			//
			oldPropSheetWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)PropSheetSubclassProc);
			break;
	}
	return 0;
}

// Show a property sheet
//
int ShowPropertySheet(int nShowCmd) {
	UNREFERENCED_PARAMETER(nShowCmd);

	// Initialize common controls
	//
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icce);

	// Register the window classes we use in the property sheet
	//
	MonitorSummaryItem::RegisterWindowClass();
	LUTview::RegisterWindowClass();

	// Set up property sheet
	//
	PROPSHEETHEADER psh;
	PROPSHEETPAGE * pages = NULL;
	wchar_t (* headers)[128] = NULL;
	try {
		size_t listSize = Monitor::GetListSize();
		size_t pageCount = 1 + listSize;
		pages = new PROPSHEETPAGE[pageCount];
		headers = (wchar_t (*)[128])new wchar_t[listSize * 128];
		SecureZeroMemory(&psh, sizeof(psh));
		SecureZeroMemory(pages, sizeof(PROPSHEETPAGE) * pageCount);

		// Set up the Summary page
		//
		pages[0].dwSize = sizeof(pages[0]);
		pages[0].hInstance = g_hInst;
		pages[0].pszTemplate = MAKEINTRESOURCE(IDD_SUMMARY_PAGE);
		pages[0].pszIcon = NULL;
		pages[0].pfnDlgProc = SummaryPageProc;
		pages[0].lParam = 0;

		// Set up a page for each monitor
		//
		for (size_t i = 0; i < listSize; ++i) {
			pages[i+1].dwSize = sizeof(pages[0]);
			pages[i+1].hInstance = g_hInst;
			pages[i+1].pszTemplate = MAKEINTRESOURCE(IDD_MONITOR_PAGE);
			pages[i+1].pszIcon = NULL;
			pages[i+1].pfnDlgProc = MonitorPage::MonitorPageProc;
			pages[i+1].lParam = reinterpret_cast<LPARAM>(new MonitorPage(Monitor::Get(i)));
			StringCbCopy(headers[i], sizeof(headers[0]), Monitor::Get(i)->GetDeviceString().c_str());
			pages[i+1].pszTitle = headers[i];
			pages[i+1].dwFlags = PSP_USETITLE;
		}

		// Show the PropertySheet window
		//
		psh.dwSize = sizeof(psh);
		psh.dwFlags =
			PSH_PROPSHEETPAGE	|
			PSH_NOAPPLYNOW		|
			PSH_NOCONTEXTHELP	|
			PSH_USEICONID		|
			PSH_USECALLBACK
			;
		psh.hwndParent = NULL;
		psh.hInstance = g_hInst;
		psh.pszIcon = MAKEINTRESOURCE(IDI_ICON_SETUPAPI_35);
#define HARDCODE_PROGRAM_CAPTION 1			// Display 32-bit vs. 64-bit, Debug vs. Release in program caption
#if HARDCODE_PROGRAM_CAPTION
  #ifdef _WIN64
    #ifdef _DEBUG
		psh.pszCaption = L"LUT Loader 64-bit Debug";
    #else
		psh.pszCaption = L"LUT Loader 64-bit Release";
    #endif
  #else
    #ifdef _DEBUG
		psh.pszCaption = L"LUT Loader 32-bit Debug";
    #else
		psh.pszCaption = L"LUT Loader 32-bit Release";
    #endif
  #endif
#else
		wchar_t szCaption[256];
		LoadString(g_hInst, IDS_CAPTION, szCaption, _countof(szCaption));
		psh.pszCaption = szCaption;
#endif
		psh.nPages = (UINT)pageCount;
		psh.ppsp = pages;
		psh.nStartPage = 0;								// 0 == Summary page, 1 == 1st Monitor page for testing
		psh.pfnCallback = PropSheetCallback;
		summaryPageWasShown = false;
		SetLastError(0);
		INT_PTR retVal = PropertySheet(&psh);
		DWORD err;
		if (summaryPageWasShown) {
			err = 0;
		} else {
			err = GetLastError();
		}
		if ( err || (retVal < 0) ) {
			wstring s = ShowError(L"PropertySheet");
			wchar_t errorMessageCaption[256];
			LoadString(g_hInst, IDS_ERROR, errorMessageCaption, _countof(errorMessageCaption));
			MessageBox(NULL, s.c_str(), errorMessageCaption, MB_ICONINFORMATION | MB_OK);
		}

		// Delete the MonitorPage objects we created
		//
		for (size_t i = 0; i < listSize; ++i) {
			delete reinterpret_cast<MonitorPage *>(pages[i+1].lParam);
		}
		delete [] pages;
		delete [] headers;
	}
	catch (bad_alloc &ba) {
		if (pages) {
			delete [] pages;
		}
		if (headers) {
			delete [] headers;
		}
		wchar_t exceptionMessage[256];
		wchar_t errorMessage[256];
		wchar_t errorMessageCaption[256];
		int bigBufferSize;
		wchar_t * bigBuffer = L"\0";

		// Call MultiByteToWideChar() to get the required size of the output buffer
		//
		bigBufferSize = MultiByteToWideChar(
				CP_ACP,								// Code page
				0,									// Flags
				ba.what(),							// Input string
				-1,									// Count, -1 for NUL-terminated
				NULL,								// No output buffer
				0									// Zero output buffer size means "compute required size"
		);

		if (bigBufferSize) {

			// Call MultiByteToWideChar() a second time to translate the string to Unicode
			//
			bigBuffer = new wchar_t[bigBufferSize];
			int iRetVal = MultiByteToWideChar(
					CP_ACP,							// Code page
					0,								// Flags
					ba.what(),						// Input string
					-1,								// Count, -1 for NUL-terminated
					bigBuffer,						// Unicode output buffer
					bigBufferSize					// Buffer size in wide characters
			);
			if ( 0 == iRetVal ) {
				bigBuffer[0] = L'\0';
			}
		}
		LoadString(g_hInst, IDS_EXCEPTION, exceptionMessage, _countof(exceptionMessage));
		StringCbPrintf(errorMessage, sizeof(errorMessage), exceptionMessage, bigBuffer);
		LoadString(g_hInst, IDS_ERROR, errorMessageCaption, _countof(errorMessageCaption));
		MessageBox(NULL, errorMessage, errorMessageCaption, MB_ICONINFORMATION | MB_OK);
		delete [] bigBuffer;
		return -1;
	}
	return 0;
}
