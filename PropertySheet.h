// PropertySheet.h -- Display a property sheet
//

#pragma once
#include "stdafx.h"

extern WNDPROC oldEditWindowProc;
extern HWND hwnd_IDC_ORIGINAL_SIZE;
extern HWND hwnd_IDC_RESIZED;
extern SIZE minimumWindowSize;

extern int ShowPropertySheet(int nShowCmd);
extern INT_PTR CALLBACK EditSubclassProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
