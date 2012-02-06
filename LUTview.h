// LUTview.h -- Display a LUT graphically
//

#pragma once
#include "stdafx.h"
#include "LUT.h"

#define LUTVIEW_CLASS_NAME (L"LUT Viewer")

typedef enum tag_LUT_GRAPH_DISPLAY_STYLE {
	LGDS_WHITE = 0,
	LGDS_BLACK = 1,
	LGDS_GRADIENT = 2
} LUT_GRAPH_DISPLAY_STYLE;

class LUTview {

public:
	LUTview(wstring text, LUT_GRAPH_DISPLAY_STYLE displayStyle, bool userControl);
	~LUTview();

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
	void DrawNoLutTextOnDC(HDC hdc);

	static LRESULT CALLBACK LUTviewWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	wstring						displayText;
	LUT_GRAPH_DISPLAY_STYLE		graphDisplayStyle;
	bool						userCanChangeDisplayMode;
	HWND						hwnd;
	LUT *						pLUT;
	RECT						outerRect;
	RECT						innerRect;
	UINT						paintCount;
};
