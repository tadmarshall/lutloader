// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#define STRICT

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

// Tests in Windows header files are a bit funky.  SetDllDirectory() requires Windows XP with service pack 2,
// but the test in the header file "winbase.h" is "#if _WIN32_WINNT >= 0x0502", and "0x0502" is _WIN32_WINNT_WS03.
// So, for header file purposes, pretend that we require Windows Server 2003 instead of Windows XP SP2.
//
#ifndef WINVER
#define WINVER _WIN32_WINNT_WS03			// Require Windows XP SP2 or above
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03			// Require Windows XP SP2 or above
//#define NTDDI_VERSION NTDDI_WINXPSP2		// Require Windows XP SP2 or above
#endif

#ifndef _WIN32_WINNT		
#define _WIN32_WINNT _WIN32_WINNT_WS03		// Require Windows XP SP2 or above
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410				// Allow use of features specific to Windows 98 or later
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600					// Allow use of features specific to IE 6.0 or later
#endif

#define DEBUG_MEMORY_LEAKS
#ifdef DEBUG_MEMORY_LEAKS
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif // DEBUG_MEMORY_LEAKS

#include <windows.h>
#include <string>
#include <vector>
using namespace std;

#define COUNTOF(str) (sizeof(str) / sizeof(str[0]))
