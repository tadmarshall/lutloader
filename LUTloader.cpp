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

// Globals
//
HINSTANCE g_hInst;					// Instance handle
wstring s;							// Summary message

// Try for visual styles, if available
//
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int LoadLUTs(void) {
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
			SetDlgItemText(hWnd, IDC_SUMMARY_TEXT, s.c_str());
			break;
	}
	return 0;
}

// Other page dialog proc
//
INT_PTR CALLBACK OtherPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	switch (uMessage) {
		case WM_INITDIALOG:
			size_t monitorNumber = ((PROPSHEETPAGE *)lParam)->lParam;
			SetDlgItemText(hWnd, IDC_OTHER_TEXT, Monitor::GetMonitor(monitorNumber).SummaryString().c_str());
			break;
	}
	return 0;
}

int ShowLUTLoaderDialog(void) {

	// Loop through all display adapters
	//
	DISPLAY_DEVICE displayAdapter;
	ZeroMemory(&displayAdapter, sizeof(displayAdapter));
	displayAdapter.cb = sizeof(displayAdapter);
	int iAdapterNum = 0;
	while ( EnumDisplayDevices(NULL, iAdapterNum, &displayAdapter, 0) ) {
		if ( Adapter::IsAdapterActive(&displayAdapter) ) {
			Adapter adapter(&displayAdapter);
			Adapter::AddAdapter(adapter);
			//adapterList.push_back(adapter);

			// Loop through all monitors on this display adapter
			//
			DISPLAY_DEVICE displayMonitor;
			ZeroMemory(&displayMonitor, sizeof(displayMonitor));
			displayMonitor.cb = sizeof(displayMonitor);
			int iMonitorNum = 0;
			while ( EnumDisplayDevices(displayAdapter.DeviceName, iMonitorNum, &displayMonitor, 0) ) {
				if ( Monitor::IsMonitorActive(&displayMonitor) ) {
					Monitor monitor(iAdapterNum, &displayMonitor);
					Monitor::AddMonitor(monitor);
					//monitorList.push_back(monitor);
				}
				++iMonitorNum;
			}
		}
		++iAdapterNum;
	}

	// Build a display string describing the monitors we found
	//
	for (size_t i = 0; i < Monitor::GetMonitorListSize(); i++) {
		if ( !s.empty() ) {
			s += L"\n\n";						// Skip a line between monitors
		}
		s += Monitor::GetMonitor(i).SummaryString();
	}

	// Set up common controls
	//
	InitCommonControls();

	// Set up property sheet
	//
	PROPSHEETHEADER psh;
	PROPSHEETPAGE * pages = NULL;
	wchar_t (* headers)[128] = NULL;
	try {
		pages = new PROPSHEETPAGE[1 + Monitor::GetMonitorListSize()];
		headers = (wchar_t (*)[128])new wchar_t[Monitor::GetMonitorListSize() * 128];
		ZeroMemory(&psh, sizeof(psh));
		ZeroMemory(pages, sizeof(pages[0]) * Monitor::GetMonitorListSize());

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
			pages[i+1].pszTemplate = MAKEINTRESOURCE(IDD_OTHER_PAGE);
			pages[i+1].pszIcon = NULL;
			pages[i+1].pfnDlgProc = OtherPageProc;
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
		psh.nPages = 1 + (UINT)Monitor::GetMonitorListSize();
		psh.ppsp = pages;
		//psh.pfnCallback = PropSheetCallback;
		PropertySheet(&psh);

		delete [] pages;
		delete [] headers;
	}
	catch (bad_alloc &ba) {
		(void)ba;
		if (pages) {
			delete [] pages;
		}
		if (headers) {
			delete [] headers;
		}
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

	return retval;
}