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

#define STRICT							// Strict type checking in <windows.h>
#define WINVER 0x502					// Require Windows XP SP2 or above
#define _WIN32_WINNT 0x502				// Require Windows XP SP2 or above
//#define _WIN32_WINNT 0x600				// Require Vista or above
#define STRSAFE_USE_SECURE_CRT 1		// Try for more secure string functions
#define UNICODE 1						// Force Unicode

#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>

#include "resource.h"

#define COUNTOF(str) (sizeof(str) / sizeof(str[0]))

// Globals
//
HINSTANCE g_hInst;
TCHAR summaryMessage[10240];

// Try for visual styles, if available
//
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

TCHAR * GetProfileName(HKEY hKeyBase, TCHAR * registryKey, BOOL * perUser) {
	HKEY hKey;
	TCHAR * profileName = 0;
	int len = 0;
	if (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase, registryKey, 0, KEY_QUERY_VALUE, &hKey)) {
		DWORD dataSize;

		// Fetch the list of profiles
		//
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"ICMProfile", NULL, NULL, NULL, &dataSize)) {
			dataSize += sizeof(TCHAR);
			void * profileListBase = malloc(dataSize);
			if (profileListBase) {
				TCHAR * profileList = (TCHAR *)profileListBase;
				memset(profileList, 0, dataSize);
				RegQueryValueEx(hKey, L"ICMProfile", NULL, NULL, (LPBYTE)profileList, &dataSize);

				if (*profileList) {
					TCHAR * profilePtr = 0;

					// Find the last profile in the list
					//
					while (*profileList) {
						profilePtr = profileList;
						len = 1 + lstrlenW(profileList);
						profileList += len;
					}
					profileName = (TCHAR *)malloc(sizeof(TCHAR) * len);
					if (profileName) {
						wcscpy_s(profileName, len, profilePtr);
					}
				}
				free(profileListBase);
			}
		}

		// If caller wants to know if UsePerUserProfiles exists and is set to REG_DWORD:1, let him know
		//
		if (perUser) {
			DWORD data = 0;
			DWORD dataType;
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"UsePerUserProfiles", NULL, &dataType, NULL, &dataSize)) {
				if ( (REG_DWORD == dataType) && (sizeof(data) == dataSize) ) {
					RegQueryValueEx(hKey, L"UsePerUserProfiles", NULL, NULL, (LPBYTE)&data, &dataSize);
				}
			}
			*perUser = (1 == data);
		}
		RegCloseKey(hKey);
	}
	return profileName;
}


void handleMonitor(
		PDISPLAY_DEVICE displayAdapter,
		PDISPLAY_DEVICE displayMonitor,
		TCHAR * message,
		int messageSize
) {
	TCHAR * activeProfileName = 0;

#if 1
	TCHAR szDebugString[256];
	StringCbCopy(szDebugString, sizeof(szDebugString), L"LUTLoader.exe -- handleMonitor for '");
	StringCbCat(szDebugString, sizeof(szDebugString), displayMonitor->DeviceName);
	StringCbCat(szDebugString, sizeof(szDebugString), L"' (");
	StringCbCat(szDebugString, sizeof(szDebugString), displayMonitor->DeviceString);
	StringCbCat(szDebugString, sizeof(szDebugString), L")\n");
	OutputDebugString(szDebugString);
#endif

	// Show monitor name, skip a line between monitors
	//
	if (message[0]) {
		wcscat_s(message, messageSize, L"\n\n");
	}
	wcscat_s(message, messageSize, displayMonitor->DeviceString);

	// Note primary monitor
	//
	if( displayAdapter->StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
		wcscat_s(message, messageSize, L" (primary)");
	}

	// See if we are Vista or greater, so we know if per-user monitor profiles are supported
	//
	OSVERSIONINFOEX osInfo;
	ZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx((OSVERSIONINFO *)&osInfo);
	if (osInfo.dwMajorVersion >= 6) {

		// Vista or higher, show user default and system default profiles
		//
		wcscat_s(message, messageSize, L"\n    User default profile = ");
		TCHAR registryKey[512];
		TCHAR * userProfileName;
		BOOL perUser = FALSE;
		int len = lstrlenW(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
		wcscpy_s(
			registryKey,
			COUNTOF(registryKey),
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display");
		wcscat_s(registryKey, COUNTOF(registryKey), &displayMonitor->DeviceKey[len]);
		userProfileName = GetProfileName(HKEY_CURRENT_USER, registryKey, &perUser);
		if (userProfileName) {
			wcscat_s(message, messageSize, userProfileName);
		}
		if (perUser && userProfileName) {
			wcscat_s(message, messageSize, L" (active)");
		} else {
			wcscat_s(message, messageSize, L" (inactive)");
		}

		wcscat_s(message, messageSize, L"\n    System default profile = ");
		TCHAR * systemProfileName;
		len = lstrlenW(L"\\Registry\\Machine\\");
		wcscpy_s(registryKey, COUNTOF(registryKey), &displayMonitor->DeviceKey[len]);
		systemProfileName = GetProfileName(HKEY_LOCAL_MACHINE, registryKey, 0);
		if (systemProfileName) {
			wcscat_s(message, messageSize, systemProfileName);
		}

		// Choose active profile name (if any), free the other one, if any
		//
		if (perUser && userProfileName) {
			wcscat_s(message, messageSize, L" (inactive)");
			activeProfileName = userProfileName;
			if (systemProfileName) {
				free(systemProfileName);
			}
		} else {
			wcscat_s(message, messageSize, L" (active)");
			if (systemProfileName) {
				activeProfileName = systemProfileName;
				if (userProfileName) {
					free(userProfileName);
				}
			}
		}

	} else {

		// XP or lower, show a single default profile
		//
		wcscat_s(message, messageSize, L"\n    Profile = ");
		TCHAR registryKey[512];
		int len = lstrlenW(L"\\Registry\\Machine\\");
		wcscpy_s(registryKey, COUNTOF(registryKey), &displayMonitor->DeviceKey[len]);
		activeProfileName = GetProfileName(HKEY_LOCAL_MACHINE, registryKey, 0);
		if (activeProfileName) {
			wcscat_s(message, messageSize, activeProfileName);
		}
	}

	// Display the start of the gamma ramp
	//
	HDC hDC = CreateDC(displayAdapter->DeviceName, 0, 0, 0);
	if (hDC) {
		wcscat_s(message, messageSize, L"\n    Gamma ramp =");
		WORD deviceGammaRamp[3][256];
		memset(deviceGammaRamp, 0, sizeof(deviceGammaRamp));
		GetDeviceGammaRamp(hDC, deviceGammaRamp);

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				TCHAR buf[10];
				StringCbPrintf(buf, sizeof(buf),  L" %04X", deviceGammaRamp[i][j]);
				wcscat_s(message, messageSize, buf);
			}
		}

#if 0
		// For testing, optionally call SetDeviceGammaRamp() on what we just got as the gamma ramp
		//
		TCHAR innerMessage[512];
		innerMessage[0] = 0;
		wcscat_s(innerMessage, COUNTOF(innerMessage), L"Call SetDeviceGammaRamp() on ");
		wcscat_s(innerMessage, COUNTOF(innerMessage), displayMonitor->DeviceString);
		wcscat_s(innerMessage, COUNTOF(innerMessage), L" on ");
		wcscat_s(innerMessage, COUNTOF(innerMessage), displayAdapter->DeviceName);
		wcscat_s(innerMessage, COUNTOF(innerMessage), L"?");
		if (IDYES == MessageBox(NULL,
			innerMessage, L"LUT Loader -- Call SetDeviceGammaRamp()?", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2)
		) {
			//// For testing, clobber the start of the gamma table
			////
			//for (int i = 0; i < 3; i++) {
			//	for (int j = 0; j < 4; j++) {
			//		deviceGammaRamp[i][j] = 0;
			//	}
			//}
			//wcscat_s(message, messageSize, L"\n    Clobbered gamma ramp =");
			//for (int i = 0; i < 3; i++) {
			//	for (int j = 0; j < 4; j++) {
			//		TCHAR buf[10];
			//		wsprintf(buf, L" %04X", deviceGammaRamp[i][j]);
			//		wcscat_s(message, messageSize, buf);
			//	}
			//}
			SetDeviceGammaRamp(hDC, deviceGammaRamp);
		}
#endif // if 0

		// If we have an active profilename, maybe install it or just release it for now
		//
		if (activeProfileName) {
			free(activeProfileName);
		}

		DeleteDC(hDC);
	}

}

void handleAdapter(
		PDISPLAY_DEVICE displayAdapter,
		TCHAR * message,
		int messageSize
) {
	DISPLAY_DEVICE displayMonitor;
	int iMonitorNum = 0;

#if 1
	TCHAR szDebugString[256];
	StringCbCopy(szDebugString, sizeof(szDebugString), L"LUTLoader.exe -- handleAdapter for '");
	StringCbCat(szDebugString, sizeof(szDebugString), displayAdapter->DeviceName);
	StringCbCat(szDebugString, sizeof(szDebugString), L"' (");
	StringCbCat(szDebugString, sizeof(szDebugString), displayAdapter->DeviceString);
	StringCbCat(szDebugString, sizeof(szDebugString), L")\n");
	OutputDebugString(szDebugString);
#endif

	ZeroMemory(&displayMonitor, sizeof(displayMonitor));
	displayMonitor.cb = sizeof(displayMonitor);

	// Loop through all monitors on this display adapter
	//
	while (EnumDisplayDevices(displayAdapter->DeviceName, iMonitorNum, &displayMonitor, 0)) {

		// If this monitor is active and part of the desktop, handle it
		//
		if ( (displayMonitor.StateFlags & DISPLAY_DEVICE_ACTIVE) &&
			 (displayMonitor.StateFlags & DISPLAY_DEVICE_ATTACHED)
		) {
			handleMonitor(displayAdapter, &displayMonitor, message, messageSize);
		}
		++iMonitorNum;
	}
}

int LoadLUTs(void) {
	TCHAR szCaption[256];
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
			SetDlgItemText(hWnd, IDC_SUMMARY_TEXT, summaryMessage);
			break;
	}
	return 0;
}

#if 0
// Other page dialog proc
//
INT_PTR CALLBACK OtherPageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {
	return 0;
}
#endif // if 0

int ShowLUTLoaderDialog(void) {

	// Create message text
	//
	DISPLAY_DEVICE displayAdapter;
	int iAdapterNum = 0;
	TCHAR szCaption[256];

	ZeroMemory(&displayAdapter, sizeof(displayAdapter));
	displayAdapter.cb = sizeof(displayAdapter);
	summaryMessage[0] = NULL;

	// Loop through all display adapters
	//
	while (EnumDisplayDevices(NULL, iAdapterNum, &displayAdapter, 0)) {

		// If this is a non-virtual adapter that is part of the desktop, handle it
		//
		if ( (0 == (displayAdapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) &&
			 (displayAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
		) {
			handleAdapter(&displayAdapter, summaryMessage, COUNTOF(summaryMessage));
		}
		++iAdapterNum;
	}

	// Set up common controls
	//
	InitCommonControls();

	// Set up property sheet
	//
	PROPSHEETHEADER psh;
	PROPSHEETPAGE pages[1];
	ZeroMemory(&psh, sizeof(psh));
	ZeroMemory(&pages, sizeof(pages));

	// Set up the Summary page
	//
	pages[0].dwSize = sizeof(pages[0]);
	pages[0].hInstance = g_hInst;
	pages[0].pszTemplate = MAKEINTRESOURCE(IDD_SUMMARY_PAGE);
	pages[0].pszIcon = NULL;
	pages[0].pfnDlgProc = SummaryPageProc;
	pages[0].lParam = 0;

#if 0
	// Set up the Other page
	//
	pages[1].dwSize = sizeof(pages[0]);
	pages[1].hInstance = g_hInst;
	pages[1].pszTemplate = MAKEINTRESOURCE(IDD_OTHER_PAGE);
	pages[1].pszIcon = NULL;
	pages[1].pfnDlgProc = OtherPageProc;
	pages[1].lParam = 0;
#endif // if 0

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
	LoadString(g_hInst, IDS_CAPTION, szCaption, COUNTOF(szCaption));
	psh.pszCaption = szCaption;
	//psh.nPages = sizeof(pages) / sizeof(pages[0]);
	psh.nPages = COUNTOF(pages);
	psh.ppsp = pages;
	//psh.pfnCallback = PropSheetCallback;
	PropertySheet(&psh);

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

#if 1
	// Turn on DEP, if available
	//
	typedef BOOL (WINAPI * PFN_SetProcessDEPPolicy)(DWORD);
	HINSTANCE hKernel = ::GetModuleHandle(L"kernel32.dll");
	if (hKernel) {
		PFN_SetProcessDEPPolicy p_SetProcessDEPPolicy = (PFN_SetProcessDEPPolicy)GetProcAddress(hKernel, "SetProcessDEPPolicy");
		if (p_SetProcessDEPPolicy) {
			p_SetProcessDEPPolicy( /* PROCESS_DEP_ENABLE */ 0x00000001 );
		}
	}
#endif

#if 1
	// Do not load DLLs from the current directory
	//
	SetDllDirectory(L"");
#endif

#if 0
	int nButtonPressed                  = 0;
	TASKDIALOGCONFIG config             = {0};
	const TASKDIALOG_BUTTON buttons[]   = { 
											{ IDOK, L"Change password" }
										  };
	config.cbSize                       = sizeof(config);
	config.hInstance                    = g_hInst;
	config.dwCommonButtons              = TDCBF_CANCEL_BUTTON;
	config.pszMainIcon                  = TD_WARNING_ICON;
	config.pszMainInstruction           = L"Change Password";
	config.pszContent                   = L"Remember your changed password.";
	config.pButtons                     = buttons;
	config.cButtons                     = ARRAYSIZE(buttons);

	TaskDialogIndirect(&config, &nButtonPressed, NULL, NULL);
	switch (nButtonPressed)
	{
		case IDOK:
			break; // the user pressed button 0 (change password).
		case IDCANCEL:
			break; // user cancelled the dialog
		default:
			break; // should never happen
	}
#endif

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