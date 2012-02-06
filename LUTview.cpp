// LUTview.cpp -- Display a LUT graphically
//

#include "stdafx.h"
#include "LUT.h"
#include "LUTview.h"
#include "resource.h"
#include "Utility.h"
#include <strsafe.h>

// Optional "features"
//
#define SHOW_PAINT_COUNT 1

// Some constants
//
#define GRAPH_CAPTION_INDENT_X			4
#define GRAPH_CAPTION_INDENT_Y			4

// Colors for the different display styles
//
#define RED_COLOR_NORMAL						RGB(255,  0,  0)
#define GREEN_COLOR_NORMAL						RGB(  0,255,  0)
#define BLUE_COLOR_NORMAL						RGB(  0,  0,255)
#define WHITE_COLOR								RGB(255,255,255)
#define BLACK_COLOR								RGB(  0,  0,  0)
#define LIGHT_GRAY_COLOR						RGB(192,192,192)

#if 0 // currently unused
#define RED_COLOR_ALTERED						RGB(232, 64, 37)
#define GREEN_COLOR_ALTERED						RGB(138,236,  0)
#define BLUE_COLOR_ALTERED						RGB(  0, 91,255)
#define BACKGROUND_COLOR_I1MATCH				RGB( 81, 81, 81)
#define FRAME_COLOR_I1MATCH						RGB(  0,  0,  0)
#define TEXT_COLOR_I1MATCH						RGB(255,255,255)
#define GRID_COLOR_I1MATCH						RGB(136,136,136)
#endif

// Symbols defined in other files
//
extern HINSTANCE g_hInst;
extern double dpiScale;						// Scaling factor for dots per inch (actual versus standard 96 DPI)

// Global externs defined in this file
//

// Global static symbols internal to this file
//
static wchar_t * LutViewClassName = LUTVIEW_CLASS_NAME;		// Global for register window class name of LUTview windows

typedef struct tag_STYLE_SET {
	COLORREF	background;
	COLORREF	frame;
	COLORREF	grid;
	COLORREF	gridDashBkColor;
	COLORREF	text;
	COLORREF	red;
	COLORREF	green;
	COLORREF	blue;
	DWORD		gridlineCount;
} STYLE_SET;

static STYLE_SET styleTable[3] = {
	{		// LGDS_WHITE style
		WHITE_COLOR,			// background
		RGB(  0,  0,  0),		// frame
		LIGHT_GRAY_COLOR,		// grid
		RGB(240,240,240),		// grid dashes
		BLACK_COLOR,			// text
		RED_COLOR_NORMAL,		// red
		GREEN_COLOR_NORMAL,		// green
		BLUE_COLOR_NORMAL,		// blue
		14						// how many grid lines to draw
	},
	{		// LGDS_BLACK style
		BLACK_COLOR,			// background
		RGB(128,128,128),		// frame
		RGB( 96, 96, 96),		// grid
		RGB( 64, 64, 64),		// grid dashes
		WHITE_COLOR,			// text
		RED_COLOR_NORMAL,		// red
		GREEN_COLOR_NORMAL,		// green
		BLUE_COLOR_NORMAL,		// blue
		6						// how many grid lines to draw
	},
	{		// LGDS_GRADIENT style
		WHITE_COLOR,			// background
		RGB(128,128,128),		// frame
		RGB(160,160,160),		// grid
		RGB( 96, 96, 96),		// grid dashes
		BLACK_COLOR,			// text
		RED_COLOR_NORMAL,		// red
		GREEN_COLOR_NORMAL,		// green
		BLUE_COLOR_NORMAL,		// blue
		0						// how many grid lines to draw
	}
};

// Constructor
//
LUTview::LUTview(wstring text, LUT_GRAPH_DISPLAY_STYLE displayStyle, bool userControl) :
		displayText(text),
		hwnd(0),
		pLUT(0),
		graphDisplayStyle(displayStyle),
		userCanChangeDisplayMode(userControl),
		paintCount(0)
{
}

// Destructor
//
LUTview::~LUTview() {
	if (pLUT) {
		delete [] pLUT;
	}
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
			break;

		case WM_PAINT:
			thisView = reinterpret_cast<LUTview *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, 0)));
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			if (thisView->pLUT) {
				++thisView->paintCount;
				thisView->DrawImageOnDC(ps.hdc);
			} else {
				thisView->DrawNoLutTextOnDC(ps.hdc);
			}
			EndPaint(hWnd, &ps);
			return 0;
			break;

		case WM_CONTEXTMENU:
			thisView = reinterpret_cast<LUTview *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, 0)));
			if (thisView->userCanChangeDisplayMode && thisView->pLUT) {
				POINT pt;
				pt.x = static_cast<signed short>(LOWORD(lParam));
				pt.y = static_cast<signed short>(HIWORD(lParam));
				RECT rect;
				GetWindowRect(hWnd, &rect);
				rect.right = rect.left + thisView->innerRect.right;
				rect.bottom = rect.top + thisView->innerRect.bottom;
				rect.left += thisView->innerRect.left;
				rect.top += thisView->innerRect.top;
				if (PtInRect(&rect, pt)) {
					int flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION;
					HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_LUTVIEW_POPUP_MENU));
					HMENU hPopup = GetSubMenu(hMenu, 0);
					CheckMenuRadioItem(
							hPopup,
							ID_WHITEBACKGROUND,
							ID_GRADIENTBACKGROUND,
							ID_WHITEBACKGROUND + thisView->graphDisplayStyle,
							MF_BYCOMMAND );
					int id = TrackPopupMenu(hPopup, flags, pt.x, pt.y, 0, hWnd, NULL);
					if ( id >= ID_WHITEBACKGROUND && id <= ID_GRADIENTBACKGROUND ) {
						thisView->graphDisplayStyle = static_cast<LUT_GRAPH_DISPLAY_STYLE>(id - ID_WHITEBACKGROUND);
						InvalidateRect(hWnd, NULL, TRUE);
					}
					DestroyMenu(hMenu);
				}
			}
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
			WS_CHILD | WS_GROUP | WS_VISIBLE | WS_CLIPSIBLINGS,
			//WS_CHILD | WS_GROUP | WS_VISIBLE,
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
	if (pLUT) {
		delete [] pLUT;
	}
	if (lutPointer) {
		pLUT = new LUT;
		memcpy_s(pLUT, sizeof(LUT), lutPointer, sizeof(LUT));
	} else {
		pLUT = 0;
	}
}

void LUTview::DrawImageOnDC(HDC hdc) {
	SIZE headingSize;
	RECT headingRect;
	int clipped;
	wstring s;
	wchar_t buf[1024];
	HFONT hFont;
	HGDIOBJ oldFont;
	POINT pt[256];
	HGDIOBJ oldPen = 0;
	HGDIOBJ gridPen = 0;
	HGDIOBJ gridPen2 = 0;

	// This is just to suppress "warning C6385: Invalid data: accessing
	// 'struct tag_COLOR_SET * styleTable', the readable size is '84' bytes, but '140' bytes might be read"
	//
	if (graphDisplayStyle > LGDS_GRADIENT) {	// It's an 'enum', so it really should be in range ...
		return;
	}

	// Compute the largest square that fits within our rect, centered
	//
	//RECT outerRect;
	//RECT innerRect;
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

	// Fetch the heading text and compute its size and rectangle.  We wait until the
	// end to draw it, but we want to avoid clipping it with gridlines
	//
	StringCbCopy(buf, sizeof(buf), displayText.c_str());

#if SHOW_PAINT_COUNT
	wchar_t buf2[128];
	//StringCbPrintf(buf2, sizeof(buf2), L"\nPaint count == %u", paintCount);
	StringCbPrintf(buf2, sizeof(buf2), L" -- paint count == %u", paintCount);
	StringCbCat(buf, sizeof(buf), buf2);
#endif

	hFont = GetFont(hdc, FC_INFORMATION);
	oldFont = SelectObject(hdc, hFont);
	GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &headingSize);

	int graphCaptionIndentX = static_cast<int>(GRAPH_CAPTION_INDENT_X * dpiScale);
	int graphCaptionIndentY = static_cast<int>(GRAPH_CAPTION_INDENT_Y * dpiScale);

	headingRect.left = innerRect.left + graphCaptionIndentX;
	headingRect.top = innerRect.top + graphCaptionIndentY;
	headingRect.right = headingRect.left + headingSize.cx;
	headingRect.bottom = headingRect.top + headingSize.cy;

	// This will cause flicker, so fix it ...
	//
	FillRect(hdc, &outerRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	// Draw the background
	//
	if ( LGDS_GRADIENT == graphDisplayStyle ) {
		TRIVERTEX t[4];
		t[0].x = innerRect.left + 1;		// Upper left corner
		t[0].y = innerRect.top + 1;
		t[0].Red = 128 << 8;
		t[0].Green = 128 << 8;
		t[0].Blue = 128 << 8;
		t[0].Alpha = 255 << 8;

		t[1].x = innerRect.right - 1;		// Upper right corner
		t[1].y = innerRect.top + 1;
		t[1].Red = 255 << 8;
		t[1].Green = 255 << 8;
		t[1].Blue = 255 << 8;
		t[1].Alpha = 255 << 8;

		t[2].x = innerRect.right - 1;		// Lower right corner
		t[2].y = innerRect.bottom - 1;
		t[2].Red = 128 << 8;
		t[2].Green = 128 << 8;
		t[2].Blue = 128 << 8;
		t[2].Alpha = 255 << 8;

		t[3].x = innerRect.left + 1;		// Lower left corner
		t[3].y = innerRect.bottom - 1;
		t[3].Red = 0 << 8;
		t[3].Green = 0 << 8;
		t[3].Blue = 0 << 8;
		t[3].Alpha = 255 << 8;

		GRADIENT_TRIANGLE g[2];
		g[0].Vertex1 = 0;
		g[0].Vertex2 = 2;
		g[0].Vertex3 = 3;

		g[1].Vertex1 = 0;
		g[1].Vertex2 = 2;
		g[1].Vertex3 = 1;

		GradientFill(hdc, t, _countof(t), g, _countof(g), GRADIENT_FILL_TRIANGLE);
	} else {
		HBRUSH hBrushBackground = CreateSolidBrush(styleTable[graphDisplayStyle].background);
		FillRect(hdc, &innerRect, hBrushBackground);
		DeleteObject(hBrushBackground);
	}

	// Maybe draw crosshatch lines
	//
	if ( styleTable[graphDisplayStyle].gridlineCount ) {
		int gridLeft = innerRect.left;
		int gridTop = innerRect.top;
		int gridRight = innerRect.right;
		int gridBottom = innerRect.bottom;
		int gridVerticalMidpoint2 = (gridTop + gridBottom) / 2;
		int gridHorizontalMidpoint2 = (gridLeft + gridRight) / 2;
		int gridVerticalMidpoint1 = (gridTop + gridVerticalMidpoint2) / 2;
		int gridHorizontalMidpoint1 = (gridLeft + gridHorizontalMidpoint2) / 2;
		int gridVerticalMidpoint3 = (gridVerticalMidpoint2 + gridBottom) / 2;
		int gridHorizontalMidpoint3 = (gridHorizontalMidpoint2 + gridRight) / 2;
		DWORD ptIndex[8];

		int edgeRight = headingRect.right + graphCaptionIndentX;
		int edgeBottom = headingRect.bottom + graphCaptionIndentY;

		clipped = gridTop;
		if ( gridHorizontalMidpoint1 < edgeRight ) {
			clipped = edgeBottom;
		}
		pt[0].x = gridHorizontalMidpoint1;
		pt[0].y = clipped;
		pt[1].x = gridHorizontalMidpoint1;
		pt[1].y = gridBottom;
		ptIndex[0] = 2;

		clipped = gridTop;
		if ( gridHorizontalMidpoint2 < edgeRight ) {
			clipped = edgeBottom;
		}
		pt[2].x = gridHorizontalMidpoint2;
		pt[2].y = clipped;
		pt[3].x = gridHorizontalMidpoint2;
		pt[3].y = gridBottom;
		ptIndex[1] = 2;

		clipped = gridTop;
		if ( gridHorizontalMidpoint3 < edgeRight ) {
			clipped = edgeBottom;
		}
		pt[4].x = gridHorizontalMidpoint3;
		pt[4].y = clipped;
		pt[5].x = gridHorizontalMidpoint3;
		pt[5].y = gridBottom;
		ptIndex[2] = 2;

		pt[6].x = gridLeft;
		pt[6].y = gridVerticalMidpoint1;
		pt[7].x = gridRight;
		pt[7].y = gridVerticalMidpoint1;
		ptIndex[3] = 2;

		pt[8].x = gridLeft;
		pt[8].y = gridVerticalMidpoint2;
		pt[9].x = gridRight;
		pt[9].y = gridVerticalMidpoint2;
		ptIndex[4] = 2;

		pt[10].x = gridLeft;
		pt[10].y = gridVerticalMidpoint3;
		pt[11].x = gridRight;
		pt[11].y = gridVerticalMidpoint3;
		ptIndex[5] = 2;

		gridPen = CreatePen(PS_SOLID, 0, styleTable[graphDisplayStyle].grid);
		oldPen = SelectObject(hdc, gridPen);
		DWORD lineCount = styleTable[graphDisplayStyle].gridlineCount;
		if (lineCount > 6) {
			lineCount = 6;
		}
		PolyPolyline(hdc, pt, ptIndex, lineCount);

		if (styleTable[graphDisplayStyle].gridlineCount > 6) {

			int gridVerticalFraction1   = (gridTop  + gridVerticalMidpoint1) / 2;
			int gridHorizontalFraction1 = (gridLeft + gridHorizontalMidpoint1) / 2;

			int gridVerticalFraction2   = (gridVerticalMidpoint1   + gridVerticalMidpoint2) / 2;
			int gridHorizontalFraction2 = (gridHorizontalMidpoint1 + gridHorizontalMidpoint2) / 2;

			int gridVerticalFraction3   = (gridVerticalMidpoint2   + gridVerticalMidpoint3) / 2;
			int gridHorizontalFraction3 = (gridHorizontalMidpoint2 + gridHorizontalMidpoint3) / 2;

			int gridVerticalFraction4   = (gridVerticalMidpoint3   + gridBottom) / 2;
			int gridHorizontalFraction4 = (gridHorizontalMidpoint3 + gridRight) / 2;

			clipped = gridTop;
			if ( gridHorizontalFraction1 < edgeRight ) {
				clipped = edgeBottom;
			}
			pt[0].x = gridHorizontalFraction1;
			pt[0].y = clipped;
			pt[1].x = gridHorizontalFraction1;
			pt[1].y = gridBottom;
			ptIndex[0] = 2;

			clipped = gridTop;
			if ( gridHorizontalFraction2 < edgeRight ) {
				clipped = edgeBottom;
			}
			pt[2].x = gridHorizontalFraction2;
			pt[2].y = clipped;
			pt[3].x = gridHorizontalFraction2;
			pt[3].y = gridBottom;
			ptIndex[1] = 2;

			clipped = gridTop;
			if ( gridHorizontalFraction3 < edgeRight ) {
				clipped = edgeBottom;
			}
			pt[4].x = gridHorizontalFraction3;
			pt[4].y = clipped;
			pt[5].x = gridHorizontalFraction3;
			pt[5].y = gridBottom;
			ptIndex[2] = 2;

			clipped = gridTop;
			if ( gridHorizontalFraction4 < edgeRight ) {
				clipped = edgeBottom;
			}
			pt[6].x = gridHorizontalFraction4;
			pt[6].y = clipped;
			pt[7].x = gridHorizontalFraction4;
			pt[7].y = gridBottom;
			ptIndex[3] = 2;

			pt[8].x = gridLeft;
			pt[8].y = gridVerticalFraction1;
			pt[9].x = gridRight;
			pt[9].y = gridVerticalFraction1;
			ptIndex[4] = 2;

			pt[10].x = gridLeft;
			pt[10].y = gridVerticalFraction2;
			pt[11].x = gridRight;
			pt[11].y = gridVerticalFraction2;
			ptIndex[5] = 2;

			pt[12].x = gridLeft;
			pt[12].y = gridVerticalFraction3;
			pt[13].x = gridRight;
			pt[13].y = gridVerticalFraction3;
			ptIndex[6] = 2;

			pt[14].x = gridLeft;
			pt[14].y = gridVerticalFraction4;
			pt[15].x = gridRight;
			pt[15].y = gridVerticalFraction4;
			ptIndex[7] = 2;

			gridPen2 = CreatePen(PS_DOT, 0, styleTable[graphDisplayStyle].grid);
			SelectObject(hdc, gridPen2);
			COLORREF oldBkColor = SetBkColor(hdc, styleTable[graphDisplayStyle].gridDashBkColor);
			PolyPolyline(hdc, pt, ptIndex, 8);
			SetBkColor(hdc, oldBkColor);
		}
	}

	// Compute scaling for graph
	//
	double xFactor = ( (innerRect.right - innerRect.left - 2) / double(255) );
	double yFactor = ( (innerRect.bottom - innerRect.top - 2) / double(65535) );

	// Try merging colors (works best on a black background, useless on white)
	//
	if ( LGDS_BLACK == graphDisplayStyle ) {
		SetROP2(hdc, R2_MERGEPEN);
	}

	// Draw red
	//
	HGDIOBJ redPen = CreatePen(PS_SOLID, 0, styleTable[graphDisplayStyle].red);
	if (oldPen) {
		SelectObject(hdc, redPen);
	} else {
		oldPen = SelectObject(hdc, redPen);
	}
	for (size_t i = 0; i < 256; ++i) {
		pt[i].x = innerRect.left + (int)(xFactor * double(i) + 0.49) + 1;
		if ( pt[i].x < (innerRect.left + 1) ) {
			pt[i].x = (innerRect.left + 1);
		} else if ( pt[i].x > (innerRect.right - 1) ) {
			pt[i].x = (innerRect.right - 1);
		}
		pt[i].y = innerRect.bottom - (int)(yFactor * double(pLUT->red[i]) + 0.49) - 2;
		if ( pt[i].y < (innerRect.top) ) {
			pt[i].y = (innerRect.top);
		} else if ( pt[i].y > (innerRect.bottom - 1) ) {
			pt[i].y = (innerRect.bottom - 1);
		}
	}
	Polyline(hdc, pt, 256);

	// Draw green
	//
	HGDIOBJ greenPen = CreatePen(PS_SOLID, 0, styleTable[graphDisplayStyle].green);
	SelectObject(hdc, greenPen);
	for (size_t i = 0; i < 256; ++i) {
		pt[i].x = innerRect.left + (int)(xFactor * double(i) + 0.49) + 1;
		if ( pt[i].x < (innerRect.left + 1) ) {
			pt[i].x = (innerRect.left + 1);
		} else if ( pt[i].x > (innerRect.right - 1) ) {
			pt[i].x = (innerRect.right - 1);
		}
		pt[i].y = innerRect.bottom - (int)(yFactor * double(pLUT->green[i]) + 0.49) - 2;
		if ( pt[i].y < (innerRect.top) ) {
			pt[i].y = (innerRect.top);
		} else if ( pt[i].y > (innerRect.bottom - 1) ) {
			pt[i].y = (innerRect.bottom - 1);
		}
	}
	Polyline(hdc, pt, 256);

	// Draw blue
	//
	HGDIOBJ bluePen = CreatePen(PS_SOLID, 0, styleTable[graphDisplayStyle].blue);
	SelectObject(hdc, bluePen);
	for (size_t i = 0; i < 256; ++i) {
		pt[i].x = innerRect.left + (int)(xFactor * double(i) + 0.49) + 1;
		if ( pt[i].x < (innerRect.left + 1) ) {
			pt[i].x = (innerRect.left + 1);
		} else if ( pt[i].x > (innerRect.right - 1) ) {
			pt[i].x = (innerRect.right - 1);
		}
		pt[i].y = innerRect.bottom - (int)(yFactor * double(pLUT->blue[i]) + 0.49) - 2;
		if ( pt[i].y < (innerRect.top) ) {
			pt[i].y = (innerRect.top);
		} else if ( pt[i].y > (innerRect.bottom - 1) ) {
			pt[i].y = (innerRect.bottom - 1);
		}
	}
	Polyline(hdc, pt, 256);

	// We need to restore overwrite mode to draw the solid frame
	//
	SetROP2(hdc, R2_COPYPEN);

	// Draw a frame
	//
	pt[0].x = innerRect.left;
	pt[0].y = innerRect.top;

	pt[1].x = innerRect.left;
	pt[1].y = innerRect.bottom - 1;

	pt[2].x = innerRect.right - 1;
	pt[2].y = innerRect.bottom - 1;

	pt[3].x = innerRect.right - 1;
	pt[3].y = innerRect.top;

	pt[4].x = innerRect.left;
	pt[4].y = innerRect.top;

	HGDIOBJ framePen = CreatePen(PS_SOLID, 0, styleTable[graphDisplayStyle].frame);
	SelectObject(hdc, framePen);
	Polyline(hdc, pt, 5);

	// Draw the heading text, if any
	//
	if (buf[0]) {
		int oldMode = SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, styleTable[graphDisplayStyle].text);
		DrawText(hdc, buf, -1, &headingRect, 0);
		SetBkMode(hdc, oldMode);
	}

	// Restore original pen, delete the ones we created
	//
	SelectObject(hdc, oldPen);
	if (gridPen) {
		DeleteObject(gridPen);
	}
	if (gridPen2) {
		DeleteObject(gridPen2);
	}
	DeleteObject(redPen);
	DeleteObject(greenPen);
	DeleteObject(bluePen);
	DeleteObject(framePen);
}

void LUTview::DrawNoLutTextOnDC(HDC hdc) {
	SIZE headingSize;
	RECT rect;
	GetClientRect(hwnd, &rect);
	wstring s;
	wchar_t buf[1024];
	HFONT hFont;
	HGDIOBJ oldFont;

	// Compute the largest square that fits within our rect, centered
	//
	//RECT outerRect;
	//RECT innerRect;
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

	// This will cause flicker, so fix it ...
	//
	FillRect(hdc, &outerRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	// Draw a frame
	//
	POINT pt[256];
	pt[0].x = innerRect.left;
	pt[0].y = innerRect.top;

	pt[1].x = innerRect.left;
	pt[1].y = innerRect.bottom - 1;

	pt[2].x = innerRect.right - 1;
	pt[2].y = innerRect.bottom - 1;

	pt[3].x = innerRect.right - 1;
	pt[3].y = innerRect.top;

	pt[4].x = innerRect.left;
	pt[4].y = innerRect.top;

	HGDIOBJ framePen = CreatePen(PS_SOLID, 0, LIGHT_GRAY_COLOR);
	//HGDIOBJ framePen = CreatePen(PS_SOLID, 0, styleTable[LGDS_WHITE].frame);
	HGDIOBJ oldPen = SelectObject(hdc, framePen);
	Polyline(hdc, pt, 5);

	COLORREF highlightColor = RGB(0x00, 0x33, 0x99);	// Vista UI guidelines shade of blue
	COLORREF errorColor     = RGB(0x99, 0x00, 0x00);	// Dim red

	StringCbCopy(buf, sizeof(buf), displayText.c_str());
	hFont = GetFont(hdc, FC_HEADING);
	oldFont = SelectObject(hdc, hFont);
	GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &headingSize);
	rect.bottom = innerRect.top + (innerRect.bottom - innerRect.top) / 2;
	rect.top = rect.bottom - headingSize.cy;
	rect.left = innerRect.left + (innerRect.right - innerRect.left - headingSize.cx) / 2;
	rect.right = rect.left + headingSize.cx;
	SetTextColor(hdc, highlightColor);
	DrawText(hdc, buf, -1, &rect, 0);

	StringCbCopy(buf, sizeof(buf), L"No data to display");
	hFont = GetFont(hdc, FC_INFORMATION);
	SelectObject(hdc, hFont);
	GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &headingSize);
	rect.top = rect.bottom;
	rect.bottom = rect.top + headingSize.cy;
	rect.left = innerRect.left + (innerRect.right - innerRect.left - headingSize.cx) / 2;
	rect.right = rect.left + headingSize.cx;
	SetTextColor(hdc, errorColor);
	DrawText(hdc, buf, -1, &rect, 0);

	SelectObject(hdc, oldPen);
	DeleteObject((HGDIOBJ)framePen);
	SelectObject(hdc, oldFont);
}
