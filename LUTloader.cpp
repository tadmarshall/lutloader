// LUTloader.cpp : Main program file to load Look-Up Tables into display adapters/monitors
// based on the configured monitor profiles.
//

#include "stdafx.h"
#include <commctrl.h>
#include "Adapter.h"
#include "Monitor.h"
#include "resource.h"
#include <strsafe.h>

#include <winuser.h>
#include <prsht.h>

#pragma comment(lib, "mscms.lib")

// Globals
//
HINSTANCE g_hInst;					// Instance handle

// Try for visual styles, if available
//
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Build lists of adapters and monitors
//
void FetchMonitorInfo(void) {

	// Start with empty lists
	//
	Monitor::ClearMonitorList(false);
	Adapter::ClearAdapterList(false);

	// Loop through all display adapters
	//
	DISPLAY_DEVICE displayAdapter;
	SecureZeroMemory(&displayAdapter, sizeof(displayAdapter));
	displayAdapter.cb = sizeof(displayAdapter);
	int iAdapterNum = 0;
	while ( EnumDisplayDevices(NULL, iAdapterNum, &displayAdapter, 0) ) {
		if ( Adapter::IsAdapterActive(&displayAdapter) ) {
			Adapter * adapter = new Adapter(&displayAdapter);
			Adapter::AddAdapter(*adapter);
			delete adapter;

			// Loop through all monitors on this display adapter
			//
			DISPLAY_DEVICE displayMonitor;
			SecureZeroMemory(&displayMonitor, sizeof(displayMonitor));
			displayMonitor.cb = sizeof(displayMonitor);
			int iMonitorNum = 0;
			while ( EnumDisplayDevices(displayAdapter.DeviceName, iMonitorNum, &displayMonitor, 0) ) {
				if ( Monitor::IsMonitorActive(&displayMonitor) ) {
					Monitor * monitor = new Monitor(iAdapterNum, &displayMonitor);
					Monitor::AddMonitor(*monitor);
					delete monitor;
				}
				++iMonitorNum;
			}
		}
		++iAdapterNum;
	}
}

// Load LUTs from active profiles for all monitors
//
int LoadLUTs(void) {

	// Build lists of adapters and monitors
	//
	FetchMonitorInfo();

	// TODO -- for now, just show a message box
	//
	wchar_t szCaption[256];
	LoadString(g_hInst, IDS_CAPTION, szCaption, _countof(szCaption));
	MessageBox(NULL, L"Load LUTs only", szCaption, MB_ICONINFORMATION | MB_OK);
	return 0;
}

	static WNDPROC oldPropSheetWindowProc = 0;
	static WINDOWINFO baseWindowInfo;
	static HWND parentWindow;
	static SIZE previousParentWindowSize;
	static RECT previousCancelButtonRect;
	static SIZE previousCancelButtonSize;
	static POINT propertySheetClientAreaOffset;

// Subclass procedure for the main PropertySheet
//
INT_PTR CALLBACK PropSheetSubclassProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	switch (uMessage) {

		// Notice changes in window size
		//
		case WM_WINDOWPOSCHANGED:
			WINDOWPOS * pWindowPos;
			pWindowPos = reinterpret_cast<WINDOWPOS *>(lParam);

			if ( 0 == (pWindowPos->flags & SWP_NOSIZE) && 0 != parentWindow ) {

				// Main window has resized, move or resize all controls
				//
				HWND dlgid;
				dlgid = GetDlgItem(hWnd, IDCANCEL);

				// Compute the PropertySheet's size delta, which we can then just add to our 'previous' Cancel button rect
				//
				SIZE parentSizeDelta;
				parentSizeDelta.cx = pWindowPos->cx - previousParentWindowSize.cx;
				parentSizeDelta.cy = pWindowPos->cy - previousParentWindowSize.cy;

				// Move the Cancel button
				//
				MoveWindow(
						dlgid,
						previousCancelButtonRect.left + parentSizeDelta.cx,
						previousCancelButtonRect.top + parentSizeDelta.cy,
						previousCancelButtonSize.cx,
						previousCancelButtonSize.cy,
						FALSE
				);

				// We can fix up redrawing better, I think ...
				//
				InvalidateRect(dlgid, NULL, TRUE);
				//RECT fixupRect;
				//fixupRect.left = previousCancelButtonRect.left - pWindowPos->x - propertySheetClientAreaOffset.x;
				//fixupRect.top = previousCancelButtonRect.top - pWindowPos->y - propertySheetClientAreaOffset.y;
				//fixupRect.right = previousCancelButtonRect.right - pWindowPos->x - propertySheetClientAreaOffset.x;
				//fixupRect.bottom = previousCancelButtonRect.bottom - pWindowPos->y - propertySheetClientAreaOffset.y;
				//InvalidateRect(hWnd, &fixupRect, TRUE);
				InvalidateRect(hWnd, NULL, TRUE);

				previousParentWindowSize.cx = pWindowPos->cx;
				previousParentWindowSize.cy = pWindowPos->cy;

				previousCancelButtonRect.left += parentSizeDelta.cx;
				previousCancelButtonRect.top += parentSizeDelta.cy;
				previousCancelButtonRect.right += parentSizeDelta.cx;
				previousCancelButtonRect.bottom += parentSizeDelta.cy;
			}
			return CallWindowProc(oldPropSheetWindowProc, hWnd, uMessage, wParam, lParam);
			break;

		default:
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
			LPDLGTEMPLATE lpTemplate;
			lpTemplate = reinterpret_cast<LPDLGTEMPLATE>(lParam);
			lpTemplate->style |= (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			break;

		case PSCB_INITIALIZED:

			// Subclass the property sheet
			//
			oldPropSheetWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)PropSheetSubclassProc);
			break;

	}
	return 0;
}

	static WNDPROC oldWindowProc = 0;

// Subclass procedure for edit control
//
INT_PTR CALLBACK EditSubclassProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	switch (uMessage) {

#if 0
		// This prevents us from auto-selecting all text when we get the focus
		//
		case WM_GETDLGCODE:
			INT_PTR temp;
			temp = CallWindowProcW(oldWindowProc, hWnd, uMessage, wParam, lParam);
			return temp & ~DLGC_HASSETSEL;
			break;
#endif

		// This enables control-A as a hotkey for Select All
		//
		case WM_CHAR:
			if (1 == wParam) {
				uMessage = EM_SETSEL;
				wParam = 0;
				lParam = -1;
				return CallWindowProc(oldWindowProc, hWnd, uMessage, wParam, lParam);
			}
			return CallWindowProc(oldWindowProc, hWnd, uMessage, wParam, lParam);
			break;

		default:
			break;
	}
	return CallWindowProc(oldWindowProc, hWnd, uMessage, wParam, lParam);
}

// Summary page dialog proc
//
INT_PTR CALLBACK SummaryPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	switch (uMessage) {
		case WM_INITDIALOG:
		{
			// Subclass the edit control
			//
			oldWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(GetDlgItem(hWnd, IDC_SUMMARY_TEXT), GWLP_WNDPROC, (__int3264)(LONG_PTR)EditSubclassProc);

			// Collect base information for dialog resizing
			//
			parentWindow = GetParent(hWnd);
			SecureZeroMemory(&baseWindowInfo, sizeof(baseWindowInfo));
			baseWindowInfo.cbSize = sizeof(baseWindowInfo);
			GetWindowInfo(parentWindow, &baseWindowInfo);

			previousParentWindowSize.cx = baseWindowInfo.rcWindow.right - baseWindowInfo.rcWindow.left;
			previousParentWindowSize.cy = baseWindowInfo.rcWindow.bottom - baseWindowInfo.rcWindow.top;

			propertySheetClientAreaOffset.x = baseWindowInfo.rcClient.left - baseWindowInfo.rcWindow.left;
			propertySheetClientAreaOffset.y = baseWindowInfo.rcClient.top - baseWindowInfo.rcWindow.top;

			GetWindowRect(GetDlgItem(parentWindow, IDCANCEL), &previousCancelButtonRect);

			previousCancelButtonSize.cx = previousCancelButtonRect.right - previousCancelButtonRect.left;
			previousCancelButtonSize.cy = previousCancelButtonRect.bottom - previousCancelButtonRect.top;

			previousCancelButtonRect.left -= baseWindowInfo.rcClient.left;
			previousCancelButtonRect.top -= baseWindowInfo.rcClient.top;
			previousCancelButtonRect.right -= baseWindowInfo.rcClient.left;
			previousCancelButtonRect.bottom -= baseWindowInfo.rcClient.top;

			// Build a display string describing the monitors we found
			//
			wstring summaryString;
			for (size_t i = 0; i < Monitor::GetMonitorListSize(); i++) {
				if ( !summaryString.empty() ) {
					summaryString += L"\r\n\r\n";					// Skip a line between monitors
				}
				summaryString += Monitor::GetMonitor(i).SummaryString();
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
			break;
		}

		// These force the page and read-only edit controls to be white (instead of gray)
		//
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
			break;

		//case WM_NOTIFY:
		//	NMHDR * lpnmhdr;
		//	lpnmhdr = reinterpret_cast<NMHDR *>(lParam);
		//	switch (lpnmhdr->code) {
		//		case PSN_APPLY:   //sent when OK or Apply button pressed
		//			break;

		//		case PSN_RESET:   //sent when Cancel button pressed
		//			break;

		//		case PSN_SETACTIVE:
		//			//this will be ignored if the property sheet is not a wizard
		//			//PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT);
		//			break;

		//		default:
		//			break;
		//	}
		//	break;
	}
	return 0;
}

// Other page dialog proc
//
INT_PTR CALLBACK MonitorPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);

	switch (uMessage) {
		case WM_INITDIALOG:
		{
			// Subclass the edit control
			//
			oldWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(GetDlgItem(hWnd, IDC_MONITOR_TEXT), GWLP_WNDPROC, (__int3264)(LONG_PTR)EditSubclassProc);

			//wchar_t cString[1024];
			//GetWindowText(hWnd, cString, _countof(cString));
			//GetWindowText(GetParent(hWnd), cString, _countof(cString));

			//WINDOWINFO wi;
			//SecureZeroMemory(&wi, sizeof(wi));
			//wi.cbSize = sizeof(wi);
			//GetWindowInfo(GetParent(hWnd), &wi);

			// Build a display string describing this monitor and its profile
			//
			wstring s;
			size_t monitorNumber = ((PROPSHEETPAGE *)lParam)->lParam;
			s += Monitor::GetMonitor(monitorNumber).SummaryString();
			s += L"\r\n\r\n";
			s += Monitor::GetMonitor(monitorNumber).DetailsString();
			DWORD tabSpacing = 8;
			SendDlgItemMessage(hWnd, IDC_MONITOR_TEXT, EM_SETTABSTOPS, 1, (LPARAM)&tabSpacing);
			SetDlgItemText(hWnd, IDC_MONITOR_TEXT, s.c_str());
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

// Show a property sheet dialog
//
int ShowLUTLoaderDialog(void) {

	// Build lists of adapters and monitors
	//
	FetchMonitorInfo();

	// Initialize common controls
	//
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icce);

	// Set up property sheet
	//
	PROPSHEETHEADER psh;
	PROPSHEETPAGE * pages = NULL;
	wchar_t (* headers)[128] = NULL;
	try {
		size_t pageCount = 1 + Monitor::GetMonitorListSize();
		pages = new PROPSHEETPAGE[pageCount];
		headers = (wchar_t (*)[128])new wchar_t[Monitor::GetMonitorListSize() * 128];
		SecureZeroMemory(&psh, sizeof(psh));
		SecureZeroMemory(pages, sizeof(pages[0]) * pageCount);

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
		size_t listSize = Monitor::GetMonitorListSize();
		for (size_t i = 0; i < listSize; i++) {
		//for (size_t i = 0; i < Monitor::GetMonitorListSize(); i++) {
			pages[i+1].dwSize = sizeof(pages[0]);
			pages[i+1].hInstance = g_hInst;
			pages[i+1].pszTemplate = MAKEINTRESOURCE(IDD_MONITOR_PAGE);
			pages[i+1].pszIcon = NULL;
			pages[i+1].pfnDlgProc = MonitorPageProc;
			pages[i+1].lParam = i;
			StringCbCopy(headers[i], sizeof(headers[0]), Monitor::GetMonitorDeviceString(i).c_str());
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
		psh.pszIcon = MAKEINTRESOURCE(IDI_BACKCOLOR);
		wchar_t szCaption[256];
		LoadString(g_hInst, IDS_CAPTION, szCaption, _countof(szCaption));
		psh.pszCaption = szCaption;
		psh.nPages = (UINT)pageCount;
		psh.ppsp = pages;
		psh.pfnCallback = PropSheetCallback;
		INT_PTR retVal = PropertySheet(&psh);
		if ( retVal < 0 ) {
			wchar_t errorMessage[256];
			wchar_t errorMessageCaption[256];
			LoadString(g_hInst, IDS_PROPSHEET_FAILURE, errorMessage, _countof(errorMessage));
			LoadString(g_hInst, IDS_ERROR, errorMessageCaption, _countof(errorMessageCaption));
			MessageBox(NULL, errorMessage, errorMessageCaption, MB_ICONINFORMATION | MB_OK);
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

int WINAPI WinMain(
		HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nShowCmd
) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nShowCmd);

	g_hInst = hInstance;

#ifdef DEBUG_MEMORY_LEAKS
	//_crtBreakAlloc = 147;		// To debug memory leaks, set this to allocation number ("{nnn}")
	//_crtBreakAlloc = 168;		// To debug memory leaks, set this to allocation number ("{nnn}")
#endif

	// Turn on DEP, if available
	//
	typedef BOOL (WINAPI * PFN_SetProcessDEPPolicy)(DWORD);
	HINSTANCE hKernel;
	hKernel = GetModuleHandle(L"kernel32.dll");
	if (hKernel) {
		PFN_SetProcessDEPPolicy p_SetProcessDEPPolicy = (PFN_SetProcessDEPPolicy)GetProcAddress(hKernel, "SetProcessDEPPolicy");
		if (p_SetProcessDEPPolicy) {
			p_SetProcessDEPPolicy( /* PROCESS_DEP_ENABLE */ 0x00000001 );
		}
	}

	// Do not load DLLs from the current directory
	//
	typedef BOOL (WINAPI * PFN_SetDllDirectoryW)(LPCWSTR);
	if (hKernel) {
		PFN_SetDllDirectoryW p_SetDllDirectoryW = (PFN_SetDllDirectoryW)GetProcAddress(hKernel, "SetDllDirectoryW");
		if (p_SetDllDirectoryW) {
			p_SetDllDirectoryW(L"");
		}
	}

	// See if we are invoked with the /L switch
	//
	int retval;
	if (0 == strcmp(lpCmdLine, "/L")) {
		retval = LoadLUTs();
	} else {
		retval = ShowLUTLoaderDialog();
	}

#ifdef DEBUG_MEMORY_LEAKS
	Monitor::ClearMonitorList(true);		// Forcibly free all vector memory to help see actual memory leaks
	Adapter::ClearAdapterList(true);
	OutputDebugString(L">>>> LUTloader memory leak list -- start\n");
	_CrtDumpMemoryLeaks();
	OutputDebugString(L"<<<< LUTloader memory leak list -- end\n\n");
#endif // DEBUG_MEMORY_LEAKS

	return retval;
}
