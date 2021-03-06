// Utility.h -- Utility routines that are not tied to specific objects
//

#pragma once
#include "stdafx.h"

typedef struct tagNAME_LOOKUP
{
	DWORD identifier;
	const wchar_t * displayName;
} NAME_LOOKUP;

typedef enum tag_FONT_CLASS {
	FC_HEADING = 0,
	FC_FILENAME = 1,
	FC_INFORMATION = 2,
	FC_DIALOG = 3
} FONT_CLASS;

wstring HexDump(const LPBYTE data, size_t size, size_t rowWidth);
wstring ShowError(
		const wchar_t * functionName,
		const LONG errorReturn = 0,
		const wchar_t * preMessageText = 0,
		const wchar_t * postMessageText = 0 );
bool VistaOrHigher(void);
const wchar_t * LookupName(
	const __in __ecount(tableSize) NAME_LOOKUP * nameTable,
	DWORD tableSize,
	DWORD identifier );
HFONT GetFont(HDC hdc, FONT_CLASS fontClass, bool newCopy = false);
bool AnsiToUnicode(char * AnsiText, wchar_t * & RefUnicodeText, DWORD codePage = CP_ACP);
bool ByteSwapUnicode(wchar_t * InputUnicodeText, wchar_t * & RefOutputUnicodeText, size_t InputLengthInCharacters = -1);
