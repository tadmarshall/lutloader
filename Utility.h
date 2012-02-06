// Utility.h -- Utility routines that are not tied to specific objects
//

#pragma once
#include "stdafx.h"

// Used to convert DWORDs to display strings
//
typedef struct tagNAME_LOOKUP
{
	DWORD identifier;					// Identifier (e.g. PIDSI_TITLE (0x00000002) or VT_LPSTR (30))
	const wchar_t * displayName;		// Display as (e.g. "Title" or "VT_LPSTR")
} NAME_LOOKUP;

wstring HexDump(const LPBYTE data, size_t size, size_t rowWidth);
wstring ShowError(const wchar_t * functionName, const wchar_t * preMessageText = 0, const wchar_t * postMessageText = 0);
bool VistaOrHigher(void);
const wchar_t * LookupName(
	const __in __ecount(tableSize) NAME_LOOKUP * nameTable,
	DWORD tableSize,
	DWORD identifier );
