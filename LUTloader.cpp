// LUTloader.cpp : Main program file to load Look-Up Tables into display adapters/monitors
// based on the configured monitor profiles.
//

#include "stdafx.h"
#include "Adapter.h"
#include "LUTview.h"
#include "Monitor.h"
#include "MonitorSummaryItem.h"
#include "PropertySheet.h"
#include "Resize.h"
#include "TreeViewItem.h"
#include "Utility.h"
#include "resource.h"
#include <strsafe.h>
#include <wingdi.h>

#pragma comment(lib, "mscms.lib")
#pragma comment(lib, "uxtheme.lib")

// Global externs defined in this file
//
extern HINSTANCE g_hInst = 0;						// Instance handle
extern wchar_t * ColorDirectory = 0;				// The string returned by GetColorDirectory()
extern wchar_t * ColorDirectoryErrorString = 0;		// The error message for when GetColorDirectory() fails

// Try for visual styles, if available
//
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Find the directory for profiles (usually "C:\Windows\system32\spool\drivers\color")
//
void FetchColorDirectory(void) {
	wchar_t filepath[1024];
	DWORD filepathSize = sizeof(filepath);
	SetLastError(0);
	BOOL bRet = GetColorDirectoryW(NULL, filepath, &filepathSize);
	if (bRet) {
		ColorDirectory = new wchar_t[filepathSize];
		StringCchCopy(ColorDirectory, filepathSize, filepath);
	} else {
		wstring s = ShowError(L"GetColorDirectoryW", L"Cannot get name of directory for profile files\r\n");
		size_t len = 1 + s.size();
		ColorDirectoryErrorString = new wchar_t[len];
		StringCchCopy(ColorDirectoryErrorString, len, s.c_str());
	}
}

// Build lists of adapters and monitors
//
void FetchMonitorInfo(void) {

	// Start with empty lists
	//
	Monitor::ClearList(false);
	Adapter::ClearList(false);

	// Loop through all display adapters
	//
	DISPLAY_DEVICE displayAdapter;
	SecureZeroMemory(&displayAdapter, sizeof(displayAdapter));
	displayAdapter.cb = sizeof(displayAdapter);
	int iAdapterNum = 0;
	while ( EnumDisplayDevices(NULL, iAdapterNum, &displayAdapter, 0) ) {
		if ( Adapter::IsActive(&displayAdapter) ) {
			Adapter * adapter = Adapter::Add(new Adapter(displayAdapter));

			// Loop through all monitors on this display adapter
			//
			DISPLAY_DEVICE displayMonitor;
			SecureZeroMemory(&displayMonitor, sizeof(displayMonitor));
			displayMonitor.cb = sizeof(displayMonitor);
			int iMonitorNum = 0;
			while ( EnumDisplayDevices(displayAdapter.DeviceName, iMonitorNum, &displayMonitor, 0) ) {
				if ( Monitor::IsActive(displayMonitor) ) {
					Monitor * monitor = Monitor::Add(new Monitor(adapter, displayMonitor));
					monitor->Initialize();
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

	// TODO -- for now, just show a message box
	//
	wchar_t szCaption[256];
	LoadString(g_hInst, IDS_CAPTION, szCaption, _countof(szCaption));
	MessageBox(NULL, L"Load LUTs only", szCaption, MB_ICONINFORMATION | MB_OK);
	return 0;
}

// Program entry point
//
int WINAPI WinMain(
		HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nShowCmd
) {
	UNREFERENCED_PARAMETER(hPrevInstance);

	g_hInst = hInstance;

#ifdef DEBUG_MEMORY_LEAKS
	//_crtBreakAlloc = 240;		// To debug memory leaks, set this to allocation number ("{nnn}")
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

	// Find the directory for profiles (usually "C:\Windows\system32\spool\drivers\color")
	//
	FetchColorDirectory();

	// Build lists of adapters and monitors
	//
	FetchMonitorInfo();

	// See if we are invoked with the /L switch
	//
	int retval;
	if (0 == strcmp(lpCmdLine, "/L")) {
		retval = LoadLUTs();
	} else {
		retval = ShowPropertySheet(nShowCmd);
	}

#ifdef DEBUG_MEMORY_LEAKS
	Monitor::ClearList(true);			// Forcibly free all vector memory to help see actual memory leaks
	Adapter::ClearList(true);
	Resize::ClearResizeList(true);
	Resize::ClearAnchorPresetList(true);
	Profile::ClearList(true);
	TreeViewItem::ClearList(true);
	LUTview::ClearList(true);
	MonitorSummaryItem::ClearList(true);

	if (ColorDirectory) {				// Two strings we may have 'new'-ed
		delete [] ColorDirectory;
		ColorDirectory = 0;
	}
	if (ColorDirectoryErrorString) {
		delete [] ColorDirectoryErrorString;
		ColorDirectoryErrorString = 0;
	}

	OutputDebugString(L">>>> LUTloader memory leak list -- start\n");
	_CrtDumpMemoryLeaks();
	OutputDebugString(L"<<<< LUTloader memory leak list -- end\n\n");
#endif // DEBUG_MEMORY_LEAKS

	return retval;
}
