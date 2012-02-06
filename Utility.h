// Utility.h -- Utility routines that are not tied to specific objects
//

#pragma once
#include "stdafx.h"

wstring HexDump(const LPBYTE data, size_t size, size_t rowWidth);
wstring ShowError(const wchar_t * functionName);

