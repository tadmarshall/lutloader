// LUTview.h -- Display a LUT graphically
//

#pragma once
#include "stdafx.h"
#include "LUT.h"

#define LUTVIEW_CLASS_NAME (L"LUT Viewer")

class LUTview {

public:
	LUTview(const wchar_t * text);

	static LUTview * Add(LUTview * lutView);
	static void ClearList(bool freeAllMemory);

	HWND CreateLUTviewWindow(
			HWND viewerParent,
			int x,
			int y,
			int width,
			int height
	);
	void SetText(wstring newText);
	void SetLUT(LUT * lutPointer);

	static size_t GetListSize(void);
	static void RegisterWindowClass(void);

private:
	void DrawImageOnDC(HDC hdc);
	void DrawTextOnDC(HDC hdc);

	static LRESULT CALLBACK LUTviewWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	wstring				displayText;
	HWND				hwnd;
	LUT *				pLUT;
};
