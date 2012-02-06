// LUTloader.cpp : Main program file to load Look-Up Tables into display adapters/monitors
// based on the configured monitor profiles.
//
// $Log: /LUT Loader.root/LUT Loader/LUTloader.cpp $
// 
// 4     6/27/10 2:20p Tad
// Convert from MessageBox() to a single-page PropertySheet.  Parse the
// command line for an exact match to "/L" and run a stub (do-nothing for
// now) routine.  Eventually, this will load the LUTs and not show any UI.
// 
// 3     6/26/10 8:23p Tad
// Modified code to display both a per-user profile and the system default
// profile and which is active when running on Vista and higher.
// 
// 2     6/23/10 7:12a Tad
// Fixed bugs with extracting profile name, especially when none is set.
// Added some code to view the head of the gamma ramp.
// 
// 1     6/20/10 6:44p Tad
// First checkin.  It shows the correct monitors and profile names on XP,
// is basically a C program, doesn't do any LUT loading and only shows the
// machine-level settings on Vista or Windows 7.  Works correctly (for
// what it does) on my Windows XP machine with the driver (etc.) that
// reports both monitors on both display adapters.
// 

#include "stdafx.h"
#include <commctrl.h>
#include "Adapter.h"
#include "Monitor.h"
#include "resource.h"
#include <strsafe.h>

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
			Adapter::AddAdapter( adapter );
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
					Monitor::AddMonitor( monitor );
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
	LoadString(g_hInst, IDS_CAPTION, szCaption, COUNTOF(szCaption));
	MessageBox(NULL, L"Load LUTs only", szCaption, MB_ICONINFORMATION | MB_OK);
	return 0;
}

// PropertySheet callback routine
//
//int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM lParam) {
//	return 0;
//}

// Summary page dialog proc
//
INT_PTR CALLBACK SummaryPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	switch (uMessage) {
		case WM_INITDIALOG:

		{
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
				LoadString(g_hInst, IDS_NO_MONITORS_FOUND, noMonitorsFound, COUNTOF(noMonitorsFound));
				SetDlgItemText(hWnd, IDC_SUMMARY_TEXT, noMonitorsFound);
			} else {
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
	}
	return 0;
}

// Other page dialog proc
//
INT_PTR CALLBACK MonitorPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	switch (uMessage) {
		case WM_INITDIALOG:
		{
			wstring s;

			size_t monitorNumber = ((PROPSHEETPAGE *)lParam)->lParam;
			s += Monitor::GetMonitor(monitorNumber).SummaryString();
			s += L"\r\n\r\n";
			s += Monitor::GetMonitor(monitorNumber).DetailsString();
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
		for (size_t i = 0; i < Monitor::GetMonitorListSize(); i++) {
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
			PSH_NOCONTEXTHELP
			;
		psh.hwndParent = NULL;
		psh.hInstance = g_hInst;
		psh.hIcon = NULL;
		wchar_t szCaption[256];
		LoadString(g_hInst, IDS_CAPTION, szCaption, COUNTOF(szCaption));
		psh.pszCaption = szCaption;
		psh.nPages = (UINT)pageCount;
		psh.ppsp = pages;
		//psh.pfnCallback = PropSheetCallback;
		INT_PTR retVal = PropertySheet(&psh);
		if ( retVal < 0 ) {
			wchar_t errorMessage[256];
			wchar_t errorMessageCaption[256];
			LoadString(g_hInst, IDS_PROPSHEET_FAILURE, errorMessage, COUNTOF(errorMessage));
			LoadString(g_hInst, IDS_ERROR, errorMessageCaption, COUNTOF(errorMessageCaption));
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
		LoadString(g_hInst, IDS_EXCEPTION, exceptionMessage, COUNTOF(exceptionMessage));
		StringCbPrintf(errorMessage, sizeof(errorMessage), exceptionMessage, bigBuffer);
		LoadString(g_hInst, IDS_ERROR, errorMessageCaption, COUNTOF(errorMessageCaption));
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

	//// hack -- just testing macros/functions ...
	////
	//DWORD littleEndian = 0x01020304;
	//DWORD netOrder = htonl(littleEndian);
	//DWORD hostOrder = ntohl(littleEndian);
	//(void)netOrder;
	//(void)hostOrder;

	//_crtBreakAlloc = 147;		// To debug memory leaks, set this to allocation number ("{nnn}")
	//_crtBreakAlloc = 168;		// To debug memory leaks, set this to allocation number ("{nnn}")

	// Turn on DEP, if available
	//
	typedef BOOL (WINAPI * PFN_SetProcessDEPPolicy)(DWORD);
	HINSTANCE hKernel = GetModuleHandle(L"kernel32.dll");
	if (hKernel) {
		PFN_SetProcessDEPPolicy p_SetProcessDEPPolicy = (PFN_SetProcessDEPPolicy)GetProcAddress(hKernel, "SetProcessDEPPolicy");
		if (p_SetProcessDEPPolicy) {
			p_SetProcessDEPPolicy( /* PROCESS_DEP_ENABLE */ 0x00000001 );
		}
	}

	// Do not load DLLs from the current directory
	//
	SetDllDirectory(L"");

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
