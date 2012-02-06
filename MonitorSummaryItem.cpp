// MonitorSummaryItem.cpp -- Display summary information about a monitor on the Summary page
//

#include "stdafx.h"
//#include <uxtheme.h>
//#include <vssym32.h>
#include "LUT.h"
#include "MonitorSummaryItem.h"
#include "Resize.h"
#include "resource.h"
#include "Utility.h"
#include <strsafe.h>

// Optional "features"
//
#define SHOW_INACTIVE_PROFILE 0
#define DRAW_FRAME 1
#define DRAW_HIGHLIGHT 1
#define MOUSEOVER_EFFECTS 1
#define MOUSEOVER_DRAWS_FRAME 0
#define SHOW_PAINT_COUNT 1

// Some constants
//
#define HOT_TRACK_BACKGROUND_TOP		RGB(250,251,253)
#define HOT_TRACK_BACKGROUND_MIDDLE		RGB(243,247,253)
#define HOT_TRACK_BACKGROUND_BOTTOM		RGB(235,243,253)
#define HOT_TRACK_FRAME RGB(184,214,251)

// All of these are a problem at high DPI ...
//
#define INFLATERECT_AMOUNT 2
#define OFFSET_FROM_WINDOW_EDGE (4)
#define VERTICAL_PADDING (2)
#define HORIZONTAL_PADDING (18)
#define INFO_LINE_COUNT (2)
#define VERTICAL_PADDING_EXTRA (8)
#define INFORMATION_LINE_INDENTATION (10)

//#define LOAD_BUTTON_WIDTH (90)
//#define LOAD_BUTTON_HEIGHT (26)

#if SHOW_INACTIVE_PROFILE
#define INFO_LINE_COUNT_VISTA (3)
#endif

// Symbols defined in other files
//
extern HINSTANCE g_hInst;

// Global externs defined in this file
//

// Global static symbols internal to this file
//
static wchar_t * MonitorSummaryItemClassName = L"Monitor Summary Item";

// Constructor
//
MonitorSummaryItem::MonitorSummaryItem(Monitor * hostMonitor) :
		monitor(hostMonitor),
		hwnd(0),
		paintCount(0)
{
}

// Vector of adapters
//
static vector <MonitorSummaryItem *> monitorSummaryItemList;

// Add an adapter to the end of the list
//
MonitorSummaryItem * MonitorSummaryItem::Add(MonitorSummaryItem * newItem) {
	monitorSummaryItemList.push_back(newItem);
	return newItem;
}

// Clear the list of adapters
//
void MonitorSummaryItem::ClearList(bool freeAllMemory) {
	size_t count = monitorSummaryItemList.size();
	for (size_t i = 0; i < count; ++i) {
		delete monitorSummaryItemList[i];
	}
	monitorSummaryItemList.clear();
	if ( freeAllMemory && (monitorSummaryItemList.capacity() > 0) ) {
		vector <MonitorSummaryItem *> dummy;
		monitorSummaryItemList.swap(dummy);
	}
}

// Our window procedure
//
LRESULT CALLBACK MonitorSummaryItem::MonitorSummaryItemProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {

	MonitorSummaryItem * thisView;
	Monitor * myMonitor;
	Profile * activeProfile;
	LUT * pProfileLUT;

	switch (uMessage) {
		case WM_NCCREATE:
			CREATESTRUCT * cs;
			cs = reinterpret_cast<CREATESTRUCT *>(lParam);
			thisView = reinterpret_cast<MonitorSummaryItem *>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, DLGWINDOWEXTRA, (__int3264)(LONG_PTR)thisView);
			return DefWindowProc(hWnd, uMessage, wParam, lParam);
			break;

		//case WM_LBUTTONDOWN:
		//	ShowWindow(hWnd, SW_HIDE);
		//	break;

		case WM_COMMAND:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			if ( LOWORD(wParam) == IDC_LOAD_BUTTON ) {
				myMonitor = thisView->monitor;
				activeProfile = myMonitor->GetActiveProfile();
				pProfileLUT = activeProfile->GetLutPointer();
				myMonitor->WriteLutToCard(pProfileLUT);
				myMonitor->ReadLutFromCard();
				thisView->Update();
			}
			break;

		//case WM_ERASEBKGND:
		//	return DefWindowProc(hWnd, uMessage, wParam, lParam);

		case WM_ERASEBKGND:
			return 1;

#if MOUSEOVER_EFFECTS
		// WM_SETCURSOR -- we change the cursor when the mouse is over our text
		//
		case WM_SETCURSOR:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			POINT cp;
			GetCursorPos(&cp);
			RECT rect;
			GetWindowRect(hWnd, &rect);
			cp.x -= rect.left;
			cp.y -= rect.top;
			RECT * r;
			r = &thisView->headingRect;
			if ( cp.x >= r->left && cp.x < r->right && cp.y >= r->top && cp.y < r->bottom) {
#if MOUSEOVER_DRAWS_FRAME
				RECT c = *r;
				InflateRect(&c, INFLATERECT_AMOUNT, INFLATERECT_AMOUNT);
				HDC hdc = GetDC(hWnd);
				HBRUSH hb = CreateSolidBrush(HOT_TRACK_BACKGROUND_TOP);	// hot-track color background
				//HBRUSH hb = CreateSolidBrush(HOT_TRACK_BACKGROUND_MIDDLE);	// hot-track color background
				HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hb);
				HPEN pen = CreatePen(PS_SOLID, 0, HOT_TRACK_FRAME);		// hot-track color frame
				HPEN oldPen = (HPEN)SelectObject(hdc, pen);
				RoundRect(hdc, c.left, c.top, c.right, c.bottom, 4, 4);
				SelectObject(hdc, oldBrush);
				DeleteObject(hb);
				SelectObject(hdc, oldPen);
				DeleteObject((HGDIOBJ)pen);
				ReleaseDC(hWnd, hdc);
#endif
				SetCursor(LoadCursor(NULL, IDC_HAND));
				SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, TRUE); 
				return TRUE;
			}
			break;
#endif

		case WM_PAINT:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			++thisView->paintCount;
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			thisView->DrawTextOnDC(ps.hdc);
			EndPaint(hWnd, &ps);
			return DefWindowProc(hWnd, uMessage, wParam, lParam);
			break;

	}
	return DefDlgProc(hWnd, uMessage, wParam, lParam);
	//return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

// Register our window class
//
void MonitorSummaryItem::RegisterWindowClass(void) {
	WNDCLASSEX wc;
	SecureZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = CS_SAVEBITS | CS_DBLCLKS;
	wc.lpfnWndProc = MonitorSummaryItem::MonitorSummaryItemProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA + sizeof(MonitorSummaryItem *);
	wc.hInstance = 0;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = MonitorSummaryItemClassName;
	wc.hIconSm = 0;
	RegisterClassEx(&wc);
}

// Create a window in our window class
//
HWND MonitorSummaryItem::CreateMonitorSummaryItemWindow(
			HWND viewerParent,
			int x,
			int y,
			int width,
			int height
) {
	hwnd = CreateWindowEx(
			WS_EX_CONTROLPARENT,
			MonitorSummaryItemClassName,
			L"",
			WS_CHILD | WS_GROUP | WS_VISIBLE | DS_3DLOOK | DS_FIXEDSYS | DS_SETFONT | DS_CONTROL,
			x,
			y,
			width,
			height,
			viewerParent,
			(HMENU)0,
			g_hInst,
			this );

	RECT summaryPageRect;
	RECT baseButtonRect;
	GetWindowRect(viewerParent, &summaryPageRect);
	GetWindowRect(GetDlgItem(viewerParent, IDC_LOAD_BUTTON), &baseButtonRect);

	RECT buttonRect;
	buttonRect.left = baseButtonRect.left - summaryPageRect.left - x;
	buttonRect.right = buttonRect.left + baseButtonRect.right - baseButtonRect.left;
	buttonRect.top = width - buttonRect.right;
	buttonRect.bottom = buttonRect.top + baseButtonRect.bottom - baseButtonRect.top;

	HWND hwndButton = CreateWindowEx(
			WS_EX_NOPARENTNOTIFY,
			L"Button",
			L"Load LUT",
			WS_CHILD | WS_GROUP | WS_VISIBLE | WS_TABSTOP,
			buttonRect.left,
			buttonRect.top,
			buttonRect.right - buttonRect.left,
			buttonRect.bottom - buttonRect.top,
			hwnd,
			(HMENU)0,
			g_hInst,
			this );
	SetWindowLongPtr(hwndButton, GWLP_ID, IDC_LOAD_BUTTON);

	HDC hdc = GetDC(hwndButton);
	HFONT hFont = GetFont(hdc, FC_INFORMATION);
	ReleaseDC(hwndButton, hdc);
	SendMessage(hwndButton, WM_SETFONT, (WPARAM)hFont, TRUE);

	ANCHOR_PRESET anchorPreset;
	anchorPreset.hwnd = hwnd;
	anchorPreset.anchorLeft = true;
	anchorPreset.anchorTop = true;
	anchorPreset.anchorRight = true;
	anchorPreset.anchorBottom = false;
	Resize::AddAchorPreset(anchorPreset);

	anchorPreset.hwnd = hwndButton;
	anchorPreset.anchorLeft = false;
	Resize::AddAchorPreset(anchorPreset);

	return hwnd;
}

void MonitorSummaryItem::Update(void) {
	InvalidateRect(hwnd, NULL, TRUE);
}

int MonitorSummaryItem::GetDesiredHeight(HDC hdc) {
	wchar_t buf[] = L"Wy";
	HFONT hFont = GetFont(hdc, FC_HEADING);
	HGDIOBJ oldFont = SelectObject(hdc, hFont);
	SIZE sizeHeading;
	GetTextExtentPoint32(hdc, buf, 2, &sizeHeading);
	hFont = GetFont(hdc, FC_INFORMATION);
	DeleteObject(SelectObject(hdc, hFont));
	SIZE sizeInformation;
	GetTextExtentPoint32(hdc, buf, 2, &sizeInformation);
	DeleteObject(SelectObject(hdc, oldFont));

#if SHOW_INACTIVE_PROFILE
	int lineCount = VistaOrHigher() ? INFO_LINE_COUNT_VISTA : INFO_LINE_COUNT;
#else
	int lineCount = INFO_LINE_COUNT;
#endif
	int textHeight =
			sizeHeading.cy +
			( lineCount * (sizeInformation.cy + VERTICAL_PADDING) ) +
			VERTICAL_PADDING_EXTRA;
	return textHeight;
}

void MonitorSummaryItem::DrawTextOnDC(HDC hdc) {
	SIZE sizeHeading;
	SIZE sizeActiveProfile;
	SIZE sizeLutStatus;
	SIZE sz2;
	HFONT hFont;
	HGDIOBJ oldFont;
	wstring s;
	wchar_t buf[1024];

	COLORREF normalColor    = RGB(0x00, 0x00, 0x00);	// Vista UI guidelines black
	COLORREF highlightColor = RGB(0x00, 0x33, 0x99);	// Vista UI guidelines shade of blue
	COLORREF errorColor     = RGB(0x99, 0x00, 0x00);	// Dim red

#if SHOW_INACTIVE_PROFILE
	RECT inactiveProfileRect;
	COLORREF disabledColor  = RGB(0x32, 0x32, 0x32);	// Vista UI guidelines dark gray
#endif

	GetClientRect(hwnd, &headingRect);

	//FillRect(hdc, &headingRect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
	FillRect(hdc, &headingRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
#if DRAW_FRAME
	FrameRect(hdc, &headingRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
#endif

	headingRect.left += OFFSET_FROM_WINDOW_EDGE;

	// added for testing highlight frame
	headingRect.top += OFFSET_FROM_WINDOW_EDGE;

	s += monitor->GetDeviceString();
	StringCbCopy(buf, sizeof(buf), s.c_str());

	hFont = GetFont(hdc, FC_HEADING);
	oldFont = SelectObject(hdc, hFont);
	GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &sizeHeading);
	headingRect.right = headingRect.left + sizeHeading.cx;
	headingRect.bottom = headingRect.top + sizeHeading.cy;

#if DRAW_HIGHLIGHT
	RECT c = headingRect;
	InflateRect(&c, INFLATERECT_AMOUNT, INFLATERECT_AMOUNT);
	COLORREF hotTrackBackground = HOT_TRACK_BACKGROUND_TOP;
	//COLORREF hotTrackBackground = HOT_TRACK_BACKGROUND_MIDDLE;
	HBRUSH hb = CreateSolidBrush(hotTrackBackground);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hb);
	HPEN pen = CreatePen(PS_SOLID, 0, HOT_TRACK_FRAME);
	HPEN oldPen = (HPEN)SelectObject(hdc, pen);
	RoundRect(hdc, c.left, c.top, c.right, c.bottom, 5, 5);
	SelectObject(hdc, oldBrush);
	DeleteObject(hb);
	SelectObject(hdc, oldPen);
	DeleteObject((HGDIOBJ)pen);
	if ( 0 != GetWindowLongPtr(hwnd, GWLP_ID) ) {
		SetTextColor(hdc, normalColor);
	} else {
		SetTextColor(hdc, highlightColor);
	}
	COLORREF oldColor = SetBkColor(hdc, hotTrackBackground);
	DrawText(hdc, buf, -1, &headingRect, 0);
	SetBkColor(hdc, oldColor);
#else
	SetTextColor(hdc, highlightColor);
	DrawText(hdc, buf, -1, &headingRect, 0);
#endif

	hFont = GetFont(hdc, FC_INFORMATION);
	DeleteObject(SelectObject(hdc, hFont));
	SetTextColor(hdc, normalColor);

	if ( monitor->GetAdapter()->GetStateFlags() & DISPLAY_DEVICE_PRIMARY_DEVICE ) {
		s = L"Primary monitor";
		StringCbCopy(buf, sizeof(buf), s.c_str());
		primaryMonitorRect = headingRect;
		primaryMonitorRect.left += sizeHeading.cx + HORIZONTAL_PADDING;
		GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &sz2);
		primaryMonitorRect.top += sizeHeading.cy - sz2.cy;
		primaryMonitorRect.right = primaryMonitorRect.left + sz2.cx;
		primaryMonitorRect.bottom = primaryMonitorRect.top + sz2.cy;
		DrawText(hdc, buf, -1, &primaryMonitorRect, 0);
	}

	activeProfileRect = headingRect;
	activeProfileRect.left += INFORMATION_LINE_INDENTATION;
	activeProfileRect.top += sizeHeading.cy + VERTICAL_PADDING;
	s = L"Active ";
	if (VistaOrHigher()) {
		s += monitor->GetActiveProfileIsUserProfile() ? L"(user) " : L"(system) ";
	}
	s += L"profile:  ";
	s += monitor->GetActiveProfile()->GetName();
	StringCbCopy(buf, sizeof(buf), s.c_str());
	GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &sizeActiveProfile);
	lutStatusRect = activeProfileRect;
	lutStatusRect.top += sizeActiveProfile.cy + VERTICAL_PADDING;
	activeProfileRect.right = activeProfileRect.left + sizeActiveProfile.cx;
	activeProfileRect.bottom = activeProfileRect.top + sizeActiveProfile.cy;
	DrawText(hdc, buf, -1, &activeProfileRect, 0);

	// See if the loaded LUT is correct
	//
	Profile * active = monitor->GetActiveProfile();
	active->LoadFullProfile(false);

	DWORD maxError = 0;
	DWORD totalError = 0;

	LUT_COMPARISON result = active->CompareLUT(monitor->GetLutPointer(), &maxError, &totalError);
	s.clear();
	COLORREF color = normalColor;
	switch (result) {

		// LUTs match word for word
		//
		case LC_EQUAL:
			s += L"The current LUT matches the active profile";
			color = highlightColor;
			break;

		// One uses 0x0100, the other uses 0x0101 style
		//
		case LC_VARIATION_ON_LINEAR:
			s += L"The current LUT and the active profile are linear";
			color = highlightColor;
			break;

		// Low byte zeroed
		//
		case LC_TRUNCATION_IN_LOW_BYTE:
			s += L"The current LUT is similar to the active profile (truncation)";
			color = normalColor;
			break;

		// Windows 7 rounds to nearest high byte
		//
		case LC_ROUNDING_IN_LOW_BYTE:
			s += L"The current LUT is similar to the active profile (rounding)";
			color = normalColor;
			break;

		// Match, given the circumstances
		//
		case LC_PROFILE_HAS_NO_LUT_OTHER_LINEAR:
			s += L"The current LUT is linear and the active profile has no LUT";
			color = normalColor;
			break;

		// Mismatch, other should be linear
		//
		case LC_PROFILE_HAS_NO_LUT_OTHER_NONLINEAR:
			if ( active->HasEmbeddedWcsProfile() ) {
				s += L"The active profile is a Windows Color System profile, and has no LUT";
				color = normalColor;
			} else {
				s += L"The current LUT is not linear and the active profile has no LUT";
				color = errorColor;
			}
			break;

		// LUTs do not meet any matching criteria
		//
		case LC_UNEQUAL:
			s += L"The current LUT does not match the active profile";
			color = errorColor;
			break;
	}
	StringCbCopy(buf, sizeof(buf), s.c_str());

#if SHOW_PAINT_COUNT
	wchar_t buf2[128];
	StringCbPrintf(buf2, sizeof(buf2), L" -- paint count == %u", paintCount);
	StringCbCat(buf, sizeof(buf), buf2);
#endif

	GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &sizeLutStatus);
	lutStatusRect.right = lutStatusRect.left + sizeLutStatus.cx;
	lutStatusRect.bottom = lutStatusRect.top + sizeLutStatus.cy;
	SetTextColor(hdc, color);
	DrawText(hdc, buf, -1, &lutStatusRect, 0);

#if SHOW_INACTIVE_PROFILE
	if (VistaOrHigher()) {
		inactiveProfileRect = lutStatusRect;
		inactiveProfileRect.top += sz.cy + VERTICAL_PADDING;
		s = L"Inactive ";
		bool userIsActive = monitor->GetActiveProfileIsUserProfile();
		s += userIsActive ? L"(system) " : L"(user) ";
		s += L"profile:  ";
		Profile * inactive = userIsActive ? monitor->GetSystemProfile() : monitor->GetUserProfile();
		s += inactive->GetName();
		StringCbCopy(buf, sizeof(buf), s.c_str());
		SetTextColor(hdc, disabledColor);
		//GetTextExtentPoint32(hdc, buf, static_cast<int>(wcslen(buf)), &sz);
		DrawText(hdc, buf, -1, &inactiveProfileRect, 0);
	}
#endif
	DeleteObject(SelectObject(hdc, oldFont));
}
