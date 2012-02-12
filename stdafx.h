// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

//#define _BIND_TO_CURRENT_CRT_VERSION 0		// Do NOT force every user to have the CRT version I have

#define STRICT								// Use strict type checking in <windows.h>

#ifndef WINVER
#define WINVER _WIN32_WINNT_WINXP			// Require Windows XP or above
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WINXP			// Require Windows XP or above
#endif

#ifndef _WIN32_WINNT		
#define _WIN32_WINNT _WIN32_WINNT_WINXP		// Require Windows XP or above
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT_WINXP	// Require Windows XP or above
#endif

#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_IE_IE60			// Allow use of features specific to IE 6.0 or later
#endif

//#define __STDC_WANT_SECURE_LIB__ 1			// Use memset_s instead of memset for RtlZeroMemory, for example
//#define STRSAFE_USE_SECURE_CRT 1			// strsafe.h calls secure versions of CRT functions

#ifdef _DEBUG
#define DEBUG_MEMORY_LEAKS					// Look for memory leaks on program exit
#endif

#ifdef DEBUG_MEMORY_LEAKS
	#define _CRTDBG_MAP_ALLOC				// Track memory allocations by source line
	#include <stdlib.h>
	#include <crtdbg.h>
#endif // DEBUG_MEMORY_LEAKS

#include <windows.h>
#include <string>
#include <vector>
using namespace std;

// Return a byte-reversed WORD
//
__inline WORD swap16(const WORD n) {
	return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

// Return a byte-reversed DWORD
//
__inline DWORD swap32(const DWORD n) {
	return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24);
}

// A basic Unicode string length routine, so I can get rid of all the lstrlenW and wcslen calls
//
__inline int StringLength(const wchar_t * str) {
	const wchar_t * p = str;
	while (*p) {
		++p;
	}
	return static_cast<int>(p - str);
}
