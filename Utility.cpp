// Utility.cpp -- Utility routines that are not tied to specific objects
//

#pragma once
#include "stdafx.h"
#include "Utility.h"
#include <strsafe.h>
//#include <banned.h>

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
		for (k = i; k < l; ++k) {			// Print (up to) ROW_WIDTH hex bytes
			StringCchPrintf(buf, _countof(buf), L"%02x ", data[k]);
			s += buf;
		}
		for (; k < (i+rowWidth); ++k) {		// Handle partial last line
			s += L"   ";					// Print spaces to align ANSI part
		}
		for (k = i; k < l; ++k) {			// Print (up to) rowWidth ANSI characters
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
wstring ShowError(const wchar_t * functionName, const LONG errorReturn, const wchar_t * preMessageText, const wchar_t * postMessageText) {
	wstring s;
	wchar_t buf[1024];
	DWORD err = errorReturn ? errorReturn : GetLastError();
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

	static unsigned int osMajorVersion;

	//osMajorVersion = 6;

	if ( 0 == osMajorVersion ) {
		OSVERSIONINFOEX osInfo;
		SecureZeroMemory(&osInfo, sizeof(osInfo));
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&osInfo));
		osMajorVersion = osInfo.dwMajorVersion;
	}
	return (osMajorVersion >= 6);
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

// Get a font based on a font class
//
HFONT GetFont(HDC hdc, FONT_CLASS fontClass, bool newCopy) {

	if (newCopy) {
		int fontPitch;
		int fontWeight;
		wchar_t * faceName;

		switch (fontClass) {

			case FC_HEADING:
				fontPitch = 12;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"Arial";
				break;

			case FC_FILENAME:
				fontPitch = 11;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"Arial";
				break;

			case FC_DIALOG:
				fontPitch = VistaOrHigher() ? 9 : 8;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"MS Shell Dlg 2";
				break;

			default:
			case FC_INFORMATION:
				fontPitch = 9;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"Arial";
				break;
		}
		int nHeight = -MulDiv(fontPitch, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		return CreateFont(
			nHeight,					// height
			0,							// width
			0,							// escapement
			0,							// orientation
			fontWeight,					// weight
			FALSE,						// italic
			FALSE,						// underline
			FALSE,						// strikeout
			ANSI_CHARSET,				// character set
			OUT_OUTLINE_PRECIS,			// output precision (want outline font)
			CLIP_DEFAULT_PRECIS,		// clipping precision
			CLEARTYPE_QUALITY,			// quality
			FF_SWISS,					// font family
			faceName );					// face name
	} else {
		static HFONT fontHeading;
		static HFONT fontFilename;
		static HFONT fontInformation;
		static HFONT fontDialog;

		int fontPitch;
		int fontWeight;
		wchar_t * faceName;

		switch (fontClass) {

			case FC_HEADING:
				if (fontHeading) {
					return fontHeading;
				}
				fontPitch = 12;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"Arial";
				break;

			case FC_FILENAME:
				if (fontFilename) {
					return fontFilename;
				}
				fontPitch = 11;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"Arial";
				break;

			case FC_DIALOG:
				if (fontDialog) {
					return fontDialog;
				}
				fontPitch = VistaOrHigher() ? 9 : 8;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"MS Shell Dlg 2";
				break;

			default:
			case FC_INFORMATION:
				if (fontInformation) {
					return fontInformation;
				}
				fontPitch = 9;
				fontWeight = FW_NORMAL;
				faceName = VistaOrHigher() ? L"Segoe UI" : L"Arial";
				break;
		}
		int nHeight = -MulDiv(fontPitch, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		HFONT hFont = CreateFont(
			nHeight,					// height
			0,							// width
			0,							// escapement
			0,							// orientation
			fontWeight,					// weight
			FALSE,						// italic
			FALSE,						// underline
			FALSE,						// strikeout
			ANSI_CHARSET,				// character set
			OUT_OUTLINE_PRECIS,			// output precision (want outline font)
			CLIP_DEFAULT_PRECIS,		// clipping precision
			CLEARTYPE_QUALITY,			// quality
			FF_SWISS,					// font family
			faceName );					// face name

		switch (fontClass) {

			case FC_HEADING:
				fontHeading = hFont;
				break;

			case FC_FILENAME:
				fontFilename = hFont;
				break;

			case FC_DIALOG:
				fontDialog = hFont;
				break;

			default:
			case FC_INFORMATION:
				fontInformation = hFont;
				break;

		}
		return hFont;
	}
}

// Convert an ANSI string to Unicode.  We allocate the Unicode buffer with malloc(), caller
// must free it with free();
//
bool AnsiToUnicode(char * AnsiText, wchar_t * & RefUnicodeText, DWORD codePage) {

	bool success = false;

	// Call MultiByteToWideChar() to get the required size of the output buffer
	//
	int wideStringChars = MultiByteToWideChar(
			codePage,							// Code page
			0,									// Flags
			AnsiText,							// Input string
			-1,									// Count, -1 for NUL-terminated
			NULL,								// No output buffer
			0									// Zero output buffer size means "compute required size"
	);
	if (wideStringChars) {

		// Call MultiByteToWideChar() a second time to translate the string to Unicode
		//
		RefUnicodeText = reinterpret_cast<wchar_t *>(malloc(sizeof(wchar_t) * wideStringChars));
		if (RefUnicodeText) {
			int iRetVal = MultiByteToWideChar(
					codePage,						// Code page
					0,								// Flags
					AnsiText,						// Input string
					-1,								// Count, -1 for NUL-terminated
					RefUnicodeText,					// Unicode output buffer
					wideStringChars					// Buffer size in wide characters
			);
			if (iRetVal) {
				success = true;
			} else {
				free(RefUnicodeText);
			}
		}
	}
	return success;
}

// Convert a Unicode string from big-endian to little-endian or the other way around.  Pass in
// a length in characters to deal with non-NUL-terminated input strings, or use the default -1
// "length" to make us determine the length of the NUL-terminated input string.  The caller must
// free the buffer we allocate on his behalf.  We use malloc(), he should use free().
//
bool ByteSwapUnicode(wchar_t * InputUnicodeText, wchar_t * & RefOutputUnicodeText, size_t InputLengthInCharacters) {
	bool success = false;
	wchar_t * inputPtr;
	size_t characterCount;
	
	// Use the provided input length unless it's the default value of -1
	//
	if ( -1 == InputLengthInCharacters ) {
		inputPtr = InputUnicodeText;
		while (*inputPtr) {
			++inputPtr;
		}
		characterCount = inputPtr - InputUnicodeText;
	} else {
		characterCount = InputLengthInCharacters;
	}

	RefOutputUnicodeText = reinterpret_cast<wchar_t *>(malloc(sizeof(wchar_t) * (1 + characterCount)));
	if (RefOutputUnicodeText) {
		inputPtr = InputUnicodeText;
		wchar_t * outputPtr = RefOutputUnicodeText;
		for (size_t i = 0; i < characterCount; ++i) {
			*outputPtr = swap16(*inputPtr);
			++inputPtr;
			++outputPtr;
		}
		*outputPtr = 0;
		success = true;
	}
	return success;
}
