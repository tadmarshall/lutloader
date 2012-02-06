// LUTview.cpp -- Display a LUT graphically
//

#include "stdafx.h"
#include "LUT.h"
#include "LUTview.h"
#include <strsafe.h>

// Optional "features"
//
#define SHOW_GRAY_BACKGROUND 1
#define SHOW_BLACK_BACKGROUND_INSTEAD_OF_WHITE 1

// Some constants
//
#define FRAME_COLOR RGB(192, 192, 192)

#if SHOW_GRAY_BACKGROUND
  #define STOCK_OBJECT_FOR_BACKGROUND			LTGRAY_BRUSH
#else
	#if SHOW_BLACK_BACKGROUND_INSTEAD_OF_WHITE
	  #define STOCK_OBJECT_FOR_BACKGROUND		BLACK_BRUSH
	#else
	  #define STOCK_OBJECT_FOR_BACKGROUND		WHITE_BRUSH
	#endif
#endif


// Symbols defined in other files
//
extern HINSTANCE g_hInst;

// Global externs defined in this file
//

// Global static symbols internal to this file
//
static wchar_t * LutViewClassName = LUTVIEW_CLASS_NAME;		// Global for register window class name of LUTview windows

// Constructor
//
LUTview::LUTview(const wchar_t * text) :
		displayText(text),
		hwnd(0),
		pLUT(0)
{
}

// Vector of adapters
//
static vector <LUTview *> lutViewList;

// Add an adapter to the end of the list
//
LUTview * LUTview::Add(LUTview * newView) {
	lutViewList.push_back(newView);
	return newView;
}

// Clear the list of viewers
//
void LUTview::ClearList(bool freeAllMemory) {
	size_t count = lutViewList.size();
	for (size_t i = 0; i < count; ++i) {
		delete lutViewList[i];
	}
	lutViewList.clear();
	if ( freeAllMemory && (lutViewList.capacity() > 0) ) {
		vector <LUTview *> dummy;
		lutViewList.swap(dummy);
	}
}

// Our window procedure
//
LRESULT CALLBACK LUTview::LUTviewWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	LUTview * thisView;

	switch (uMessage) {
		case WM_NCCREATE:
			CREATESTRUCT * cs;
			cs = reinterpret_cast<CREATESTRUCT *>(lParam);
			thisView = reinterpret_cast<LUTview *>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, 0, (__int3264)(LONG_PTR)thisView);
			break;

		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
			thisView = reinterpret_cast<LUTview *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, 0)));
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			thisView->DrawImageOnDC(ps.hdc);
			EndPaint(hWnd, &ps);
			break;
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

// Register our window class
//
void LUTview::RegisterWindowClass(void) {
	WNDCLASSEX wc;
	SecureZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
	wc.lpfnWndProc = LUTview::LUTviewWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(LUTview *);
	wc.hInstance = 0;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = LutViewClassName;
	wc.hIconSm = 0;
	RegisterClassEx(&wc);
}

// Create a window in our window class
//
HWND LUTview::CreateLUTviewWindow(
			HWND viewerParent,
			int x,
			int y,
			int width,
			int height
) {
	hwnd = CreateWindowEx(
			WS_EX_NOPARENTNOTIFY,
			LutViewClassName,
			displayText.c_str(),
			WS_CHILD | WS_GROUP | WS_VISIBLE,
			x,
			y,
			width,
			height,
			viewerParent,
			(HMENU)0,
			g_hInst,
			this );
	return hwnd;
}

void LUTview::SetText(wstring newText) {
	displayText = newText;
}

void LUTview::SetLUT(LUT * lutPointer) {
	pLUT = lutPointer;
}

void LUTview::DrawImageOnDC(HDC hdc) {
	//DrawTextOnDC(hdc);

	// Compute the largest square that fits within our rect, centered
	//
	RECT outerRect;
	RECT innerRect;
	GetClientRect(hwnd, &outerRect);
	if (outerRect.right < outerRect.bottom) {

		// Too tall, center ourselves vertically
		//
		innerRect.left = 0;
		innerRect.top = (outerRect.bottom - outerRect.right) / 2;
		innerRect.right = outerRect.right;
		innerRect.bottom = innerRect.top + outerRect.right;
	} else if (outerRect.right > outerRect.bottom) {

		// Too wide, center ourselves horizontally
		//
		innerRect.left = (outerRect.right - outerRect.bottom) / 2;
		innerRect.top = 0;
		innerRect.right = innerRect.left + outerRect.bottom;
		innerRect.bottom = outerRect.bottom;
	} else {
		innerRect = outerRect;
	}

	// Draw the background
	//
	//FillRect(hdc, &innerRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	//FillRect(hdc, &innerRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	FillRect(hdc, &innerRect, (HBRUSH)GetStockObject(STOCK_OBJECT_FOR_BACKGROUND));

	// Draw a frame
	//
	POINT pt[256];
	pt[0].x = innerRect.left;
	pt[0].y = innerRect.top;

	pt[1].x = innerRect.left;
	pt[1].y = innerRect.bottom;

	pt[2].x = innerRect.right;
	pt[2].y = innerRect.bottom;

	pt[3].x = innerRect.right;
	pt[3].y = innerRect.top;

	pt[4].x = innerRect.left;
	pt[4].y = innerRect.top;

	HGDIOBJ framePen = CreatePen(PS_SOLID, 0, FRAME_COLOR);
	HGDIOBJ oldPen = SelectObject(hdc, framePen);
	Polyline(hdc, pt, 5);

	// Compute scaling for graph
	//
	double xFactor = ( (innerRect.right - innerRect.left) / double(255) );
	double yFactor = ( (innerRect.bottom - innerRect.top) / double(65535) );

	// Draw red
	//
	HGDIOBJ redPen = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
	SelectObject(hdc, redPen);
	for (size_t i = 0; i < 256; ++i) {
		pt[i].x = innerRect.left + (int)(xFactor * double(i));
		pt[i].y = innerRect.bottom - (int)(yFactor * double(pLUT->red[i]));
	}
	Polyline(hdc, pt, 256);

	// Draw green
	//
	HGDIOBJ greenPen = CreatePen(PS_SOLID, 0, RGB(0, 255, 0));
	SelectObject(hdc, greenPen);
	for (size_t i = 0; i < 256; ++i) {
		pt[i].x = innerRect.left + (int)(xFactor * double(i));
		pt[i].y = innerRect.bottom - (int)(yFactor * double(pLUT->green[i]));
	}
	Polyline(hdc, pt, 256);

	// Draw blue
	//
	HGDIOBJ bluePen = CreatePen(PS_SOLID, 0, RGB(0, 0, 255));
	SelectObject(hdc, bluePen);
	for (size_t i = 0; i < 256; ++i) {
		pt[i].x = innerRect.left + (int)(xFactor * double(i));
		pt[i].y = innerRect.bottom - (int)(yFactor * double(pLUT->blue[i]));
	}
	Polyline(hdc, pt, 256);

	// Restore original pen, delete the ones we created
	//
	SelectObject(hdc, oldPen);
	DeleteObject(framePen);
	DeleteObject(redPen);
	DeleteObject(greenPen);
	DeleteObject(bluePen);
}

void LUTview::DrawTextOnDC(HDC hdc) {
	RECT rect;
	GetClientRect(hwnd, &rect);
	wstring s;
	wchar_t buf[1024];

	if (pLUT) {
		FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));
		//FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNSHADOW + 1));

		// Display the start of the gamma ramp
		//
		s += displayText;
		s += L"\r\n\r\nStart of gamma ramp:\r\n  Red   =";
		for (int i = 0; i < 4; i++) {
			StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->red[i]);
			s += buf;
		}
		s += L"\r\n  Green =";
		for (int i = 0; i < 4; i++) {
			StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->green[i]);
			s += buf;
		}
		s += L"\r\n  Blue  =";
		for (int i = 0; i < 4; i++) {
			StringCbPrintf(buf, sizeof(buf), L" %04X", pLUT->blue[i]);
			s += buf;
		}
		StringCbCopy(buf, sizeof(buf), s.c_str());
		rect.left += 5;
		DrawText(hdc, buf, -1, &rect, 0);
	} else {
		FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNSHADOW + 1));
		s += displayText;
		s += L"\r\n\r\nLUT not set";
		StringCbCopy(buf, sizeof(buf), s.c_str());
		rect.left += 5;
		DrawText(hdc, buf, -1, &rect, 0);
	}
}
