// LUTloader.cpp : Main program file to load Look-Up Tables into display adapters/monitors
// based on the configured monitor profiles.
//

#include "stdafx.h"
#include <commctrl.h>
#include "Adapter.h"
#include "Monitor.h"
#include "Utility.h"
#include "Resize.h"
#include "resource.h"
#include <strsafe.h>

#include <commctrl.h>
//#include <winuser.h>
//#include <prsht.h>

#pragma comment(lib, "mscms.lib")

// Globals
//
HINSTANCE g_hInst = 0;					// Instance handle

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
static SIZE minimumWindowSize = {0};

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
				Resize::MainWindowHasResized(*pWindowPos);
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

			// Subclass the property sheet
			//
			oldPropSheetWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)PropSheetSubclassProc);
			break;
	}
	return 0;
}

static WNDPROC oldEditWindowProc = 0;

// Subclass procedure for edit control
//
INT_PTR CALLBACK EditSubclassProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	switch (uMessage) {

#if 0
		// This prevents us from auto-selecting all text when we get the focus
		//
		case WM_GETDLGCODE:
			INT_PTR temp;
			temp = CallWindowProcW(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
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
				return CallWindowProc(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
			}
			return CallWindowProc(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
			break;

		default:
			break;
	}
	return CallWindowProc(oldEditWindowProc, hWnd, uMessage, wParam, lParam);
}

static HWND hwnd_IDC_ORIGINAL_SIZE = 0;
static HWND hwnd_IDC_RESIZED = 0;

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
			oldEditWindowProc = (WNDPROC)(INT_PTR)SetWindowLongPtr(GetDlgItem(hWnd, IDC_SUMMARY_TEXT), GWLP_WNDPROC, (__int3264)(LONG_PTR)EditSubclassProc);

			// This page and its subwindows should grow with resizing
			//
			ANCHOR_PRESET anchorPreset;
			anchorPreset.hwnd = hWnd;
			anchorPreset.anchorLeft = true;
			anchorPreset.anchorTop = true;
			anchorPreset.anchorRight = true;
			anchorPreset.anchorBottom = true;
			Resize::AddAchorPreset(anchorPreset);

			anchorPreset.hwnd = GetDlgItem(hWnd, IDC_SUMMARY_TEXT);
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

			// Tell the resizing system that its window list is out of date
			//
			Resize::SetNeedRebuild(true);

			RECT rect;
			GetWindowRect(GetParent(hWnd), &rect);
			minimumWindowSize.cx = rect.right - rect.left;
			minimumWindowSize.cy = rect.bottom - rect.top;

			// Build a display string describing the monitors we found
			//
			wstring summaryString;
			for (size_t i = 0; i < Monitor::GetMonitorListSize(); ++i) {
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
			HWND editControlHwnd = GetDlgItem(hWnd, IDC_MONITOR_TEXT);
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

			// Fix up the size of the edit control in case the Summary tab was resized
			// before this tab was created.
			//
			RECT originalSize;
			RECT newSize;
			GetClientRect(hwnd_IDC_ORIGINAL_SIZE, &originalSize);
			GetClientRect(hwnd_IDC_RESIZED, &newSize);
			SIZE sizeDelta;
			sizeDelta.cx = newSize.right - originalSize.right;
			sizeDelta.cy = newSize.bottom - originalSize.bottom;
			RECT editSize;
			GetWindowRect(editControlHwnd, &editSize);
			WINDOWINFO wiParent;
			SecureZeroMemory(&wiParent, sizeof(wiParent));
			wiParent.cbSize = sizeof(wiParent);
			GetWindowInfo(hWnd, &wiParent);
			editSize.left -= wiParent.rcClient.left;
			editSize.top -= wiParent.rcClient.top;
			editSize.right -= wiParent.rcClient.left;
			editSize.bottom -= wiParent.rcClient.top;
			//ScreenToClient(editControlHwnd, reinterpret_cast<POINT *>(&editSize.left));
			//ScreenToClient(editControlHwnd, reinterpret_cast<POINT *>(&editSize.right));
			editSize.right += sizeDelta.cx;
			editSize.bottom += sizeDelta.cy;
			MoveWindow(editControlHwnd, editSize.left, editSize.top, editSize.right, editSize.bottom, FALSE);

			// Tell the resizing system that its window list is out of date
			//
			Resize::SetNeedRebuild(true);

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
		size_t listSize = Monitor::GetMonitorListSize();
		for (size_t i = 0; i < listSize; ++i) {
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
		DWORD err = GetLastError();
		if ( err || (retVal < 0) ) {
			wstring s = ShowError(L"PropertySheet");
			wchar_t errorMessageCaption[256];
			LoadString(g_hInst, IDS_ERROR, errorMessageCaption, _countof(errorMessageCaption));
			MessageBox(NULL, s.c_str(), errorMessageCaption, MB_ICONINFORMATION | MB_OK);
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
	Resize::ClearResizeList(true);
	Resize::ClearAnchorPresetList(true);

	OutputDebugString(L">>>> LUTloader memory leak list -- start\n");
	_CrtDumpMemoryLeaks();
	OutputDebugString(L"<<<< LUTloader memory leak list -- end\n\n");
#endif // DEBUG_MEMORY_LEAKS

	return retval;
}
