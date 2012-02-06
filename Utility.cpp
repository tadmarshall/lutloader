// Utility.cpp -- Utility routines that are not tied to specific objects
//

#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <strsafe.h>

// Display data as a hex & ANSI dump
//
wstring HexDump(const LPBYTE data, size_t size, size_t rowWidth) {
	wstring s;

	for (size_t i = 0; i < size; i+=rowWidth) {
		size_t j, k, l;
		wchar_t buf[16];
		j = i & 0x0FFFF;					// Poor man's sequence numbers
		StringCchPrintf(buf, _countof(buf), L"0x%04x | ", j);
		s += buf;
		l = min(i+rowWidth, size);			// Calculate last position for this row
		for (k = i; k < l; k++) {			// Print (up to) ROW_WIDTH hex bytes
			StringCchPrintf(buf, _countof(buf), L"%02x ", data[k]);
			s += buf;
		}
		for (; k < (i+rowWidth); k++) {		// Handle partial last line
			s += L"   ";					// Print spaces to align ANSI part
		}
		for (k = i; k < l; k++) {			// Print (up to) rowWidth ANSI characters
			int wch = static_cast<int>(data[k]);
			if ( (wch < 32) || ( (wch > 126) && (wch < 161) ) ) {
				s += L".";					// Non-printable using Courier New font, print a period instead
			} else {
				s += static_cast<wchar_t>(wch);
			}
		}
		s += L"\r\n";							// New line after rowWidth bytes
	}
	return s;
}

// Return a string showing failure of the last system call
//
wstring ShowError(const wchar_t * functionName, const wchar_t * preMessageText, const wchar_t * postMessageText) {
	wstring s;
	wchar_t buf[1024];
	DWORD err = GetLastError();
	if (preMessageText) {
		s += preMessageText;
	}
	StringCbPrintf(buf, sizeof(buf), L"%s failed (0x%x): ", functionName, err);
	s += buf;
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, buf, _countof(buf), NULL);
	s += buf;
	if (postMessageText) {
		s += postMessageText;
	}
	return s;
}

// Return 'true' if the OS we are running on is major version 6 or higher
//
bool VistaOrHigher(void) {

	OSVERSIONINFOEX osInfo;
	SecureZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&osInfo));
	return (osInfo.dwMajorVersion >= 6);
}

// Lookup a name in a table -- should perhaps be rewritten to use some 'std' thing
//
const wchar_t * LookupName(
	const __in __ecount(tableSize) NAME_LOOKUP * nameTable,
	DWORD tableSize,
	DWORD identifier
) {
	for ( DWORD i = 0; i < tableSize; ++i ) {
		if ( nameTable[i].identifier == identifier ) {
			return nameTable[i].displayName;
		}
	}
	return L"";
}
