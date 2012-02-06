// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#define STRICT

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

#ifdef _DEBUG
#define DEBUG_MEMORY_LEAKS
#endif

#ifdef DEBUG_MEMORY_LEAKS
	#define _CRTDBG_MAP_ALLOC
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
