// PropertySheet.cpp -- Display a property sheet
//

#include "stdafx.h"
#include <commctrl.h>
#include "LUTview.h"
#include "Monitor.h"
#include "MonitorPage.h"
#include "MonitorSummaryItem.h"
#include "PropertySheet.h"
#include "Resize.h"
#include "resource.h"
#include "Utility.h"
#include <strsafe.h>
#include <banned.h>

// Optional "features"
//
#define SHOW_EXTRA_COPY_OF_FIRST_MONITOR 0

// Some constants
//
#define FINAL_VERTICAL_OFFSET	4					// Scaled by DPI factor

// Symbols defined in other files
//
extern HINSTANCE g_hInst;

// Global externs defined in this file
//
extern HWND hwnd_IDC_ORIGINAL_SIZE = 0;				// The window handle of the IDC_ORIGINAL_SIZE static control
extern HWND hwnd_IDC_RESIZED = 0;					// The window handle of the IDC_RESIZE static control
extern SIZE minimumWindowSize = {0};				// The minimum size of the PropertySheet window
extern double dpiScale = 1.0;						// Scaling factor for dots per inch (actual versus standard 96 DPI)
extern LUTview * summaryLutView = 0;				// The single LUT viewer on the Summary page
extern HWND hwndSummaryLUT = 0;						// HWND for LUTview we created

// Global static symbols internal to this file
//
static WNDPROC oldPropSheetWindowProc = 0;			// The original PropertySheet's window procedure
static bool summaryPageWasShown = false;			// Just a flag that the PropertySheet at least started us

#if SHOW_EXTRA_COPY_OF_FIRST_MONITOR
static MonitorSummaryItem * testMonitorItem = 0;	// The test MonitorSummaryItem we created
static HWND hwndTestMonitorItem = 0;				// HWND for test MonitorSummaryItem we created
#endif

// Summary page dialog procedure
//
INT_PTR CALLBACK SummaryPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	switch (uMessage) {
		case WM_INITDIALOG:
		{
			RECT rect;
			HDC hdc;
			int desiredHeight;
			WINDOWINFO wi;
			int yOffset;
			int finalVerticalOffset;

			// This is a flag to reduce seeing bogus error reporting on return from PropertySheet().
			// PropertySheet can sometimes fail without setting a negative return code, and our
			// check for GetLastError() is required.  But we can't just check GetLastError(),
			// because there may well be a last error even if it all worked.  So, we won't check
			// GetLastError() if we've at least received the WM_INITDIALOG.
			//
			summaryPageWasShown = true;

			// Remember our size before any resizing as our minimum size
			//
			if ( 0 == minimumWindowSize.cx ) {
				GetWindowRect(GetParent(hWnd), &rect);
				minimumWindowSize.cx = rect.right - rect.left;
				minimumWindowSize.cy = rect.bottom - rect.top;
			}

			// Get the DPI scaling factor and the desired height of monitor summary items
			//
			hdc = GetDC(hWnd);
			dpiScale = static_cast<double>(GetDeviceCaps(hdc, LOGPIXELSY)) / 96.0;
			desiredHeight = MonitorSummaryItem::GetDesiredHeight(hdc);
			ReleaseDC(hWnd, hdc);

			yOffset = 0;
			finalVerticalOffset = static_cast<int>(FINAL_VERTICAL_OFFSET * dpiScale);

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
			// Get count of monitors
			//
			size_t monitorCount = Monitor::GetListSize();

#if SHOW_EXTRA_COPY_OF_FIRST_MONITOR
			if ( monitorCount > 0 ) {
				// Create and show a monitor summary item
				//
				testMonitorItem = MonitorSummaryItem::Add(new MonitorSummaryItem(Monitor::Get(0)));

				SecureZeroMemory(&wi, sizeof(wi));
				wi.cbSize = sizeof(wi);
				GetWindowInfo(hWnd, &wi);

				hwndTestMonitorItem = testMonitorItem->CreateMonitorSummaryItemWindow(
						hWnd,
						0,
						yOffset,
						wi.rcClient.right - wi.rcClient.left,
						desiredHeight );
				SetWindowLongPtr(hwndTestMonitorItem, GWLP_ID, 0x123);

				GetWindowRect(hwndTestMonitorItem, &rect);
				yOffset = rect.bottom - wi.rcClient.top;
			}
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
#endif

			// Build the contents of the Summary page
			//
			SecureZeroMemory(&wi, sizeof(wi));
			wi.cbSize = sizeof(wi);
			GetWindowInfo(hWnd, &wi);

			MonitorSummaryItem * mi;
			HWND hw;
			if ( monitorCount > 0 ) {
				for (size_t i = 0; i < monitorCount; ++i) {

					// Create and show a monitor summary item
					//
					Monitor * monitor = Monitor::Get(i);
					mi = MonitorSummaryItem::Add(new MonitorSummaryItem(monitor));
					monitor->SetMonitorSummaryItem(mi);
					hw = mi->CreateMonitorSummaryItemWindow(
							hWnd,
							0,
							yOffset,
							wi.rcClient.right - wi.rcClient.left,
							desiredHeight );
					GetWindowRect(hw, &rect);
					yOffset = rect.bottom - wi.rcClient.top;
				}

				// Create and show the LUT viewer
				//
				LUT_GRAPH_DISPLAY_STYLE myStyle;
				myStyle = LGDS_WHITE;
				summaryLutView = LUTview::Add( new LUTview( Monitor::Get(0)->GetDeviceString(), myStyle, true ) );
				SecureZeroMemory(&wi, sizeof(wi));
				wi.cbSize = sizeof(wi);
				GetWindowInfo(hWnd, &wi);
				hwndSummaryLUT = summaryLutView->CreateLUTviewWindow(
						hWnd,
						0,
						yOffset,
						wi.rcClient.right - wi.rcClient.left,
						wi.rcClient.bottom - wi.rcClient.top - yOffset - finalVerticalOffset);
				anchorPreset.hwnd = hwndSummaryLUT;
				anchorPreset.anchorLeft = true;
				anchorPreset.anchorTop = true;
				anchorPreset.anchorRight = true;
				anchorPreset.anchorBottom = true;
				Resize::AddAchorPreset(anchorPreset);

				// Tell the LUT viewer to display the LUT for the first monitor
				//
				summaryLutView->SetLUT(Monitor::Get(0)->GetLutPointer());
			} else {

				// If there are no monitors, use a MonitorSummaryItem to display this fact
				//
				mi = MonitorSummaryItem::Add(new MonitorSummaryItem(0));
				hw = mi->CreateMonitorSummaryItemWindow(
						hWnd,
						0,
						yOffset,
						wi.rcClient.right - wi.rcClient.left,
						desiredHeight );
				GetWindowRect(hw, &rect);
				yOffset = rect.bottom - wi.rcClient.top;
			}

			// Tell the resizing system that its window list is out of date
			//
			Resize::SetNeedRebuild(true);

			// Fix up buttons at the bottom of the dialog box
			//
			HWND parent = GetParent(hWnd);
			LONG_PTR style;

			// Disable and hide the dummy "Load LUT" button
			//
			HWND loadLutButton = GetDlgItem(hWnd, IDC_LOAD_BUTTON);
			EnableWindow(loadLutButton, FALSE);
			ShowWindow(loadLutButton, SW_HIDE);

			// Disable and hide the OK button
			//
			HWND okButton = GetDlgItem(parent, IDOK);
			style = GetWindowLongPtr(okButton, GWL_STYLE);
			style &= ~WS_TABSTOP;
			style |= WS_GROUP;
			SetWindowLongPtr(okButton, GWL_STYLE, static_cast<__int3264>(style));
			EnableWindow(okButton, FALSE);
			ShowWindow(okButton, SW_HIDE);

			// Change Cancel to Close, set its font
			//
			HWND cancelButton = GetDlgItem(parent, IDCANCEL);
			SetWindowText(cancelButton, L"Close");
			hdc = GetDC(cancelButton);
			HFONT hFont = GetFont(hdc, FC_DIALOG, true);
			ReleaseDC(cancelButton, hdc);
			SendMessage(cancelButton, WM_SETFONT, (WPARAM)hFont, TRUE);
			style = GetWindowLongPtr(cancelButton, GWL_STYLE);
			style |= WS_GROUP;
			SetWindowLongPtr(cancelButton, GWL_STYLE, static_cast<__int3264>(style));

			// Turn on WS_CLIPCHILDREN in our parent
			//
			style = GetWindowLongPtr(parent, GWL_STYLE);
			style |= WS_CLIPCHILDREN;
			SetWindowLongPtr(parent, GWL_STYLE, static_cast<__int3264>(style));

			// Try to get the PropertySheet to put focus on Close
			//
			PostMessage(parent, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(cancelButton), TRUE);

			return FALSE;
			break;
		}

		// Force the page background to be white
		//
		case WM_CTLCOLORDLG:
			return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
			break;

		// WM_NOTIFY is sent by our PropertySheet parent
		//
		//case WM_NOTIFY:
		//	NMHDR * pNMHDR;
		//	pNMHDR = reinterpret_cast<NMHDR *>(lParam);
		//	PSHNOTIFY * pPSHNOTIFY;

		//	switch (pNMHDR->code) {

				// Apply or OK pressed
				//
				//case PSN_APPLY:
				//	pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
				//	MessageBox(NULL, L"PSN_APPLY", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
				//	break;

				// Dialog is closing
				//
				//case PSN_RESET:
				//	pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
				//	MessageBox(NULL, L"PSN_RESET", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
				//	break;

				// Page activated (switched to)
				//
				//case PSN_SETACTIVE:
				//	pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
				//	MessageBox(NULL, L"PSN_SETACTIVE", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
				//	break;

				// Leaving page
				//
				//case PSN_KILLACTIVE:
				//	pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
				//	MessageBox(NULL, L"PSN_KILLACTIVE", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
				//	break;

				// User pressed Cancel
				//
				//case PSN_QUERYCANCEL:
				//	pPSHNOTIFY = reinterpret_cast<PSHNOTIFY *>(lParam);
				//	MessageBox(NULL, L"PSN_QUERYCANCEL", L"PropertySheet notification", MB_ICONINFORMATION | MB_OK);
				//	break;

			//}
			//break;

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

		//case WM_PAINT:
		//	DWORD dw;
		//	dw = 0;
		//	++dw;
		//	break;

		//case WM_ERASEBKGND:
		//	DWORD dw2;
		//	dw2 = 2;
		//	++dw2;
		//	break;

		//case WM_NCPAINT:
		//	DWORD dw3;
		//	dw3 = 3;
		//	++dw3;
		//	break;

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

			// Change the font on the tab control
			//
			HDC hdc = GetDC(hWnd);
			HFONT hFont = GetFont(hdc, FC_DIALOG, true);
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
		pages[0].pszTemplate = VistaOrHigher() ?
				MAKEINTRESOURCE(IDD_SUMMARY_PAGE_VISTA) :
				MAKEINTRESOURCE(IDD_SUMMARY_PAGE_XP);
		pages[0].pszIcon = NULL;
		pages[0].pfnDlgProc = SummaryPageProc;
		pages[0].lParam = 0;

		// Set up a page for each monitor
		//
		for (size_t i = 0; i < listSize; ++i) {

			Monitor * monitor = Monitor::Get(i);					// Get a pointer to the monitor
			MonitorPage * monitorPage = new MonitorPage(monitor);	// Create a MonitorPage object for the monitor
			monitor->SetMonitorPage(monitorPage);					// Tell the monitor how to talk to his display page

			pages[i+1].dwSize = sizeof(pages[0]);
			pages[i+1].hInstance = g_hInst;
			pages[i+1].pszTemplate = VistaOrHigher() ?
					MAKEINTRESOURCE(IDD_MONITOR_PAGE_VISTA) :
					MAKEINTRESOURCE(IDD_MONITOR_PAGE_XP);
			pages[i+1].pszIcon = NULL;
			pages[i+1].pfnDlgProc = MonitorPage::MonitorPageProc;
			pages[i+1].lParam = reinterpret_cast<LPARAM>(monitorPage);
			StringCbCopy(headers[i], sizeof(headers[0]), monitor->GetDeviceString().c_str());
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
		psh.nStartPage = 0;
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
