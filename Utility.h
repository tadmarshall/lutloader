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
	FC_INFORMATION = 2
} FONT_CLASS;

wstring HexDump(const LPBYTE data, size_t size, size_t rowWidth);
wstring ShowError(const wchar_t * functionName, const wchar_t * preMessageText = 0, const wchar_t * postMessageText = 0);
bool VistaOrHigher(void);
const wchar_t * LookupName(
	const __in __ecount(tableSize) NAME_LOOKUP * nameTable,
	DWORD tableSize,
	DWORD identifier );
HFONT GetFont(HDC hdc, FONT_CLASS fontClass);
