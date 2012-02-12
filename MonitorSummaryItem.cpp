// MonitorSummaryItem.cpp -- Display summary information about a monitor on the Summary page
//

#include "stdafx.h"
#include "Adapter.h"
#include "LUT.h"
#include "LUTview.h"
#include "MonitorSummaryItem.h"
#include "Resize.h"
#include "resource.h"
#include "Utility.h"
#include <strsafe.h>
#include <banned.h>

// Optional "features"
//
#define SHOW_INACTIVE_PROFILE 0
#define DRAW_FRAME 0
#define DRAW_HIGHLIGHT 0
#define MOUSEOVER_EFFECTS 1
#define MOUSEOVER_DRAWS_FRAME 0
#define SHOW_PAINT_COUNT 0

// Some constants
//
#define HOT_TRACK_BACKGROUND_TOP		RGB(250,251,253)
#define HOT_TRACK_BACKGROUND_MIDDLE		RGB(243,247,253)
#define HOT_TRACK_BACKGROUND_BOTTOM		RGB(235,243,253)
#define HOT_TRACK_FRAME					RGB(184,214,251)
#define INFO_LINE_COUNT 2

// All of these are scaled at high DPI
//
#define INFLATERECT_AMOUNT				2
#define OFFSET_FROM_WINDOW_EDGE			4
#define HORIZONTAL_PADDING				18
#define VERTICAL_PADDING_PER_LINE		2
#define VERTICAL_PADDING_EXTRA			10
#define INFORMATION_LINE_INDENTATION	10

#if SHOW_INACTIVE_PROFILE
#define INFO_LINE_COUNT_VISTA 3
#endif

// Symbols defined in other files
//
extern HINSTANCE g_hInst;					// Instance handle
extern double dpiScale;						// Scaling factor for dots per inch (actual versus standard 96 DPI)
extern LUTview * summaryLutView;			// The single LUT viewer on the Summary page
extern HWND hwndSummaryLUT;					// HWND for LUTview we created

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
		hwndLoadLutButton(0),
		hwndRescanButton(0),
		lutViewShowsProfile(false),
		paintCount(0)
{
}

// Vector of MonitorSummaryItems
//
static vector <MonitorSummaryItem *> monitorSummaryItemList;

// Add a MonitorSummaryItem to the end of the list
//
MonitorSummaryItem * MonitorSummaryItem::Add(MonitorSummaryItem * newItem) {
	monitorSummaryItemList.push_back(newItem);
	return newItem;
}

// Clear the list of MonitorSummaryItems
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
	RECT rect;
	RECT * r;

	switch (uMessage) {
		case WM_NCCREATE:
			CREATESTRUCT * cs;
			cs = reinterpret_cast<CREATESTRUCT *>(lParam);
			thisView = reinterpret_cast<MonitorSummaryItem *>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, DLGWINDOWEXTRA, (__int3264)(LONG_PTR)thisView);
			return DefWindowProc(hWnd, uMessage, wParam, lParam);
			break;

		case WM_COMMAND:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			if (thisView && thisView->monitor) {
				if ( LOWORD(wParam) == IDC_LOAD_BUTTON ) {
					myMonitor = thisView->monitor;
					activeProfile = myMonitor->GetActiveProfile();
					pProfileLUT = activeProfile->GetLutPointer();
					if ( pProfileLUT ) {
						myMonitor->WriteLutToCard(pProfileLUT);
					} else {
						LUT * pLUT = new LUT;
						GetSignedLUT(pLUT);
						myMonitor->WriteLutToCard(pLUT);
						delete pLUT;
					}
					myMonitor->ReadLutFromCard();
					thisView->lutViewShowsProfile = false;
					thisView->Update();
					InvalidateRect(thisView->hwnd, NULL, FALSE);
				} else if ( LOWORD(wParam) == IDC_RESCAN_BUTTON ) {
#if 0
					// Try reading the LUT from the default "screen" DC
					//
					LUT myLUT;
					BOOL bRet = 0;
					HDC hDC = GetDC(0);
					if (hDC) {
						SecureZeroMemory(&myLUT, sizeof(LUT));
						bRet = GetDeviceGammaRamp(hDC, &myLUT);
						ReleaseDC(0, hDC);
					}
#endif
					// A rescan can potentially find a completely different set of profiles, changes
					// to user vs. system usage, etc., so we need to mostly act as if we were restarted as
					// far as what we show in the UI
					//
					myMonitor = thisView->monitor;
					myMonitor->Initialize();
					MonitorPage * page = myMonitor->GetMonitorPage();
					if (page) {
						page->Reset();
					}
					thisView->Update();
					InvalidateRect(thisView->hwnd, NULL, FALSE);
				}
			}
			break;

#if MOUSEOVER_EFFECTS
		// WM_SETCURSOR -- we change the cursor when the mouse is over our text
		//
		case WM_SETCURSOR:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			if (thisView && thisView->monitor) {
				POINT cp;
				GetCursorPos(&cp);
				GetWindowRect(hWnd, &rect);
				cp.x -= rect.left;
				cp.y -= rect.top;
				r = &thisView->headingRect;
				if ( cp.x >= r->left && cp.x < r->right && cp.y >= r->top && cp.y < r->bottom ) {
#if MOUSEOVER_DRAWS_FRAME
					HFONT hFont;
					HGDIOBJ oldFont;
					wchar_t buf[1024];
					COLORREF highlightColor = RGB(0x00, 0x33, 0x99);	// Vista UI guidelines shade of blue
					RECT c = *r;
					int inflaterectAmount = static_cast<int>(INFLATERECT_AMOUNT * dpiScale);
					InflateRect(&c, inflaterectAmount, inflaterectAmount);
					wstring s = thisView->monitor->GetDeviceString();
					StringCbCopy(buf, sizeof(buf), s.c_str());
					HDC hdc = GetDC(hWnd);
					HBRUSH hb = CreateSolidBrush(HOT_TRACK_BACKGROUND_TOP);	// hot-track color background
					HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hb);
					HPEN pen = CreatePen(PS_SOLID, 0, HOT_TRACK_FRAME);		// hot-track color frame
					HPEN oldPen = (HPEN)SelectObject(hdc, pen);
					RoundRect(hdc, c.left, c.top, c.right, c.bottom, 4, 4);
					hFont = GetFont(hdc, FC_HEADING);
					oldFont = SelectObject(hdc, hFont);
					SetTextColor(hdc, highlightColor);
					int oldMode = SetBkMode(hdc, TRANSPARENT);
					DrawText(hdc, buf, -1, &thisView->headingRect, 0);
					SetBkMode(hdc, oldMode);
					SelectObject(hdc, oldFont);
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

				r = &thisView->activeProfileRect;
				if ( cp.x >= r->left && cp.x < r->right && cp.y >= r->top && cp.y < r->bottom ) {
#if MOUSEOVER_DRAWS_FRAME
					HFONT hFont;
					HGDIOBJ oldFont;
					wchar_t buf[1024];
					COLORREF normalColor    = RGB(0x00, 0x00, 0x00);	// Vista UI guidelines black
					RECT c = *r;
					int inflaterectAmount = static_cast<int>(INFLATERECT_AMOUNT * dpiScale);
					InflateRect(&c, inflaterectAmount, inflaterectAmount);
					StringCbCopy(buf, sizeof(buf), thisView->activeProfileDisplayText.c_str());
					HDC hdc = GetDC(hWnd);
					HBRUSH hb = CreateSolidBrush(HOT_TRACK_BACKGROUND_TOP);	// hot-track color background
					HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hb);
					HPEN pen = CreatePen(PS_SOLID, 0, HOT_TRACK_FRAME);		// hot-track color frame
					HPEN oldPen = (HPEN)SelectObject(hdc, pen);
					RoundRect(hdc, c.left, c.top, c.right, c.bottom, 4, 4);
					hFont = GetFont(hdc, FC_INFORMATION);
					oldFont = SelectObject(hdc, hFont);
					SetTextColor(hdc, normalColor);
					int oldMode = SetBkMode(hdc, TRANSPARENT);
					DrawText(hdc, buf, -1, &thisView->activeProfileRect, 0);
					SetBkMode(hdc, oldMode);
					SelectObject(hdc, oldFont);
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
			}
			break;
#endif

		case WM_LBUTTONDOWN:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			if (thisView && thisView->monitor) {
				int xPos;
				int yPos;				
				xPos = static_cast<signed short>(LOWORD(lParam));
				yPos = static_cast<signed short>(HIWORD(lParam));
				GetWindowRect(hWnd, &rect);
				r = &thisView->headingRect;
				if ( xPos >= r->left && xPos < r->right && yPos >= r->top && yPos < r->bottom ) {
					thisView->lutViewShowsProfile = false;
					thisView->Update();
					break;
				}
				r = &thisView->activeProfileRect;
				if ( xPos >= r->left && xPos < r->right && yPos >= r->top && yPos < r->bottom ) {
					thisView->lutViewShowsProfile = true;
					thisView->Update();
					break;
				}
			}
			break;

#if 0
		case WM_NCCALCSIZE:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			if (wParam) {
				NCCALCSIZE_PARAMS * ncp = reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam);
				DWORD dw;
				dw = 0;
				++dw;
			} else {
				RECT * r2 = reinterpret_cast<RECT *>(lParam);
				DWORD dw;
				dw = 0;
				++dw;
			}
			break;
#endif

		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
			thisView = reinterpret_cast<MonitorSummaryItem *>(static_cast<LONG_PTR>(GetWindowLongPtr(hWnd, DLGWINDOWEXTRA)));
			PAINTSTRUCT ps;
			HDC hdc;
			hdc = BeginPaint(hWnd, &ps);
			if (thisView) {
				++thisView->paintCount;
				thisView->DrawTextOnDC(hdc);
			}
			EndPaint(hWnd, &ps);
			return 0;
			break;

		case WM_CTLCOLORBTN:
			return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
			break;

	}
	return DefDlgProc(hWnd, uMessage, wParam, lParam);
}

// Register our window class
//
void MonitorSummaryItem::RegisterWindowClass(void) {
	WNDCLASSEX wc;
	SecureZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = MonitorSummaryItem::MonitorSummaryItemProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA + sizeof(MonitorSummaryItem *);
	wc.hInstance = g_hInst;
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
			WS_CHILD | WS_GROUP | WS_VISIBLE | WS_CLIPCHILDREN,
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

	ANCHOR_PRESET anchorPreset;
	anchorPreset.hwnd = hwnd;
	anchorPreset.anchorLeft = true;
	anchorPreset.anchorTop = true;
	anchorPreset.anchorRight = true;
	anchorPreset.anchorBottom = false;
	Resize::AddAchorPreset(anchorPreset);

	// If there are no monitors, our role in life is to say so, so we don't need buttons
	//
	if (monitor) {

		Profile * activeProfile = monitor->GetActiveProfile();
		if (activeProfile) {
			activeProfile->LoadFullProfile(false);
		}

		RECT buttonRect;
		buttonRect.left = baseButtonRect.left - summaryPageRect.left - x;
		buttonRect.right = buttonRect.left + baseButtonRect.right - baseButtonRect.left;
		buttonRect.top = width - buttonRect.right;
		buttonRect.bottom = buttonRect.top + baseButtonRect.bottom - baseButtonRect.top;

		hwndLoadLutButton = CreateWindowEx(
				WS_EX_NOPARENTNOTIFY,
				L"Button",
				(activeProfile && activeProfile->GetLutPointer()) ? L"Load LUT" : L"Set linear",
				WS_CHILD | WS_GROUP | WS_VISIBLE | WS_TABSTOP,
				buttonRect.left,
				buttonRect.top,
				buttonRect.right - buttonRect.left,
				buttonRect.bottom - buttonRect.top,
				hwnd,
				(HMENU)0,
				g_hInst,
				this );
		SetWindowLongPtr(hwndLoadLutButton, GWLP_ID, IDC_LOAD_BUTTON);

		HDC hdc = GetDC(hwndLoadLutButton);
		HFONT hFont = GetFont(hdc, FC_DIALOG, true);
		ReleaseDC(hwndLoadLutButton, hdc);
		SendMessage(hwndLoadLutButton, WM_SETFONT, (WPARAM)hFont, TRUE);

		buttonRect.top += buttonRect.top + baseButtonRect.bottom - baseButtonRect.top;
		buttonRect.bottom = buttonRect.top + baseButtonRect.bottom - baseButtonRect.top;

		hwndRescanButton = CreateWindowEx(
				WS_EX_NOPARENTNOTIFY,
				L"Button",
				L"Rescan",
				WS_CHILD | WS_GROUP | WS_VISIBLE | WS_TABSTOP,
				buttonRect.left,
				buttonRect.top,
				buttonRect.right - buttonRect.left,
				buttonRect.bottom - buttonRect.top,
				hwnd,
				(HMENU)0,
				g_hInst,
				this );
		SetWindowLongPtr(hwndRescanButton, GWLP_ID, IDC_RESCAN_BUTTON);
		SendMessage(hwndRescanButton, WM_SETFONT, (WPARAM)hFont, TRUE);

		anchorPreset.hwnd = hwndLoadLutButton;
		anchorPreset.anchorLeft = false;
		Resize::AddAchorPreset(anchorPreset);

		anchorPreset.hwnd = hwndRescanButton;
		Resize::AddAchorPreset(anchorPreset);
	}
	return hwnd;
}

void MonitorSummaryItem::Update(void) {
	Profile * activeProfile = monitor->GetActiveProfile();
	if (activeProfile) {
		activeProfile->LoadFullProfile(false);
	}
	if (lutViewShowsProfile) {
		if (activeProfile) {
			summaryLutView->SetText(activeProfile->GetName());
			summaryLutView->SetLUT(activeProfile->GetLutPointer());
		} else {
			summaryLutView->SetText(L"No profile");
			summaryLutView->SetLUT(0);
		}
	} else {
		summaryLutView->SetText(monitor->GetDeviceString());
		summaryLutView->SetLUT(monitor->GetLutPointer());
	}
	summaryLutView->SetUpdateBitmap();
	SendMessage(
			hwndLoadLutButton,
			WM_SETTEXT,
			0,
			reinterpret_cast<LPARAM>( (activeProfile && activeProfile->GetLutPointer()) ? L"Load LUT" : L"Set linear" )
	);
	InvalidateRect(hwndSummaryLUT, summaryLutView->GetGraphRect(), FALSE);
}

int MonitorSummaryItem::GetDesiredHeight(HDC hdc) {
	wchar_t buf[] = L"Wy";
	HFONT hFont = GetFont(hdc, FC_HEADING);
	HGDIOBJ oldFont = SelectObject(hdc, hFont);
	SIZE sizeHeading;
	GetTextExtentPoint32(hdc, buf, 2, &sizeHeading);
	hFont = GetFont(hdc, FC_INFORMATION);
	SelectObject(hdc, hFont);
	SIZE sizeInformation;
	GetTextExtentPoint32(hdc, buf, 2, &sizeInformation);
	SelectObject(hdc, oldFont);

#if SHOW_INACTIVE_PROFILE
	int lineCount = VistaOrHigher() ? INFO_LINE_COUNT_VISTA : INFO_LINE_COUNT;
#else
	int lineCount = INFO_LINE_COUNT;
#endif
	int verticalPaddingPerLine = static_cast<int>(VERTICAL_PADDING_PER_LINE * dpiScale);
	int verticalPaddingExtra = static_cast<int>(VERTICAL_PADDING_EXTRA * dpiScale);
	int textHeight =
			sizeHeading.cy +
			( lineCount * (sizeInformation.cy + verticalPaddingPerLine) ) +
			verticalPaddingExtra;
	return textHeight;
}

void MonitorSummaryItem::DrawTextOnDC(HDC hdc) {
	RECT clientRect;
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

	GetClientRect(hwnd, &clientRect);
	headingRect = clientRect;
	int offsetFromWindowEdge = static_cast<int>(OFFSET_FROM_WINDOW_EDGE * dpiScale);
	headingRect.left += offsetFromWindowEdge;
	headingRect.top  += offsetFromWindowEdge;

	if (monitor) {
		s += monitor->GetDeviceString();
	} else {
		s += L"No non-virtual monitors found";
	}
	StringCbCopy(buf, sizeof(buf), s.c_str());

#if 0
	hFont = (HFONT)SendMessage(GetDlgItem(GetParent(GetParent(hwnd)), IDCANCEL), WM_GETFONT, 0, 0);
	oldFont = SelectObject(hdc, hFont);
	//GetTextFace(hdc, _countof(buf), buf);
	LOGFONT lf;
	SecureZeroMemory(&lf, sizeof(lf));
	GetObject(hFont, sizeof(lf), &lf);
	StringCbCopy(buf, sizeof(buf), lf.lfFaceName);
	hFont = GetFont(hdc, FC_HEADING);
	SelectObject(hdc, hFont);
#else
	hFont = GetFont(hdc, FC_HEADING);
	oldFont = SelectObject(hdc, hFont);
#endif
	GetTextExtentPoint32(hdc, buf, StringLength(buf), &sizeHeading);
	headingRect.right = headingRect.left + sizeHeading.cx;
	headingRect.bottom = headingRect.top + sizeHeading.cy;

#if DRAW_HIGHLIGHT
	if (monitor) {
		if (RectVisible(hdc, &headingRect)) {
			RECT c = headingRect;
			int inflaterectAmount = static_cast<int>(INFLATERECT_AMOUNT * dpiScale);
			InflateRect(&c, inflaterectAmount, inflaterectAmount);
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
			SetTextColor(hdc, highlightColor);
			COLORREF oldColor = SetBkColor(hdc, hotTrackBackground);
			DrawText(hdc, buf, -1, &headingRect, 0);
			SetBkColor(hdc, oldColor);
			ExcludeClipRect(hdc, headingRect.left, headingRect.top, headingRect.right, headingRect.bottom);
		}
	}
#else
	if (RectVisible(hdc, &headingRect)) {
		SetTextColor(hdc, highlightColor);
		DrawText(hdc, buf, -1, &headingRect, 0);
		ExcludeClipRect(hdc, headingRect.left, headingRect.top, headingRect.right, headingRect.bottom);
	}
#endif

	SetTextColor(hdc, normalColor);
	if ( 0 == monitor ) {
		SelectObject(hdc, oldFont);
		FillRect(hdc, &clientRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
#if DRAW_FRAME
		FrameRect(hdc, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
#endif
		return;
	}

	hFont = GetFont(hdc, FC_INFORMATION);
	SelectObject(hdc, hFont);

	if ( monitor->GetAdapter()->GetStateFlags() & DISPLAY_DEVICE_PRIMARY_DEVICE ) {
		s = L"Primary monitor";
		StringCbCopy(buf, sizeof(buf), s.c_str());
		primaryMonitorRect = headingRect;
		int horizontalPadding = static_cast<int>(HORIZONTAL_PADDING * dpiScale);
		primaryMonitorRect.left += sizeHeading.cx + horizontalPadding;
		GetTextExtentPoint32(hdc, buf, StringLength(buf), &sz2);
		primaryMonitorRect.top += sizeHeading.cy - sz2.cy;
		primaryMonitorRect.right = primaryMonitorRect.left + sz2.cx;
		primaryMonitorRect.bottom = primaryMonitorRect.top + sz2.cy;
		if (RectVisible(hdc, &primaryMonitorRect)) {
			DrawText(hdc, buf, -1, &primaryMonitorRect, 0);
			ExcludeClipRect(hdc, primaryMonitorRect.left, primaryMonitorRect.top, primaryMonitorRect.right, primaryMonitorRect.bottom);
		}
	}

	activeProfileRect = headingRect;
	int informationLineIndentation = static_cast<int>(INFORMATION_LINE_INDENTATION * dpiScale);
	activeProfileRect.left += informationLineIndentation;
	int verticalPaddingPerLine = static_cast<int>(VERTICAL_PADDING_PER_LINE * dpiScale);
	activeProfileRect.top += sizeHeading.cy + verticalPaddingPerLine;
	s = L"Active ";
	if (VistaOrHigher()) {
		s += monitor->GetActiveProfileIsUserProfile() ? L"(user) " : L"(system) ";
	}
	s += L"profile:  ";
	Profile * activeProfile = monitor->GetActiveProfile();
	if (activeProfile) {
		s += activeProfile->GetName();
		activeProfile->LoadFullProfile(false);
	} else {
		s += L"No profile";
	}
	activeProfileDisplayText = s;
	StringCbCopy(buf, sizeof(buf), s.c_str());
	GetTextExtentPoint32(hdc, buf, StringLength(buf), &sizeActiveProfile);
	lutStatusRect = activeProfileRect;
	lutStatusRect.top += sizeActiveProfile.cy + verticalPaddingPerLine;
	activeProfileRect.right = activeProfileRect.left + sizeActiveProfile.cx;
	activeProfileRect.bottom = activeProfileRect.top + sizeActiveProfile.cy;
	if (RectVisible(hdc, &activeProfileRect)) {
		DrawText(hdc, buf, -1, &activeProfileRect, 0);
		ExcludeClipRect(hdc, activeProfileRect.left, activeProfileRect.top, activeProfileRect.right, activeProfileRect.bottom);
	}

	// See if the loaded LUT is correct
	//
	s.clear();
	COLORREF color;
	if (activeProfile) {
		DWORD maxError = 0;
		DWORD totalError = 0;
		LUT_COMPARISON result = activeProfile->CompareLUT(monitor->GetLutPointer(), &maxError, &totalError);
		color = normalColor;
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
				if ( activeProfile->HasEmbeddedWcsProfile() ) {
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
	} else {
		s += L"No profile has been set for this monitor";
		color = errorColor;
	}
	StringCbCopy(buf, sizeof(buf), s.c_str());

#if SHOW_PAINT_COUNT
	wchar_t buf2[128];
	StringCbPrintf(buf2, sizeof(buf2), L" -- paint count == %u", paintCount);
	StringCbCat(buf, sizeof(buf), buf2);
#endif

	GetTextExtentPoint32(hdc, buf, StringLength(buf), &sizeLutStatus);
	lutStatusRect.right = lutStatusRect.left + sizeLutStatus.cx;
	lutStatusRect.bottom = lutStatusRect.top + sizeLutStatus.cy;
	SetTextColor(hdc, color);
	if (RectVisible(hdc, &lutStatusRect)) {
		DrawText(hdc, buf, -1, &lutStatusRect, 0);
		ExcludeClipRect(hdc, lutStatusRect.left, lutStatusRect.top, lutStatusRect.right, lutStatusRect.bottom);
	}

#if SHOW_INACTIVE_PROFILE
	if (VistaOrHigher()) {
		inactiveProfileRect = lutStatusRect;
		inactiveProfileRect.top += sizeLutStatus.cy + verticalPaddingPerLine;
		s = L"Inactive ";
		bool userIsActive = monitor->GetActiveProfileIsUserProfile();
		s += userIsActive ? L"(system) " : L"(user) ";
		s += L"profile:  ";
		Profile * inactive = userIsActive ? monitor->GetSystemProfile() : monitor->GetUserProfile();
		s += inactive->GetName();
		StringCbCopy(buf, sizeof(buf), s.c_str());
		SetTextColor(hdc, disabledColor);
		GetTextExtentPoint32(hdc, buf, StringLength(buf), &sz2);
		inactiveProfileRect.right = inactiveProfileRect.left + sz2.cx;
		inactiveProfileRect.bottom = inactiveProfileRect.top + sz2.cy;
		if (RectVisible(hdc, &inactiveProfileRect)) {
			DrawText(hdc, buf, -1, &inactiveProfileRect, 0);
			ExcludeClipRect(hdc, inactiveProfileRect.left, inactiveProfileRect.top, inactiveProfileRect.right, inactiveProfileRect.bottom);
		}
	}
#endif
	FillRect(hdc, &clientRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

#if DRAW_FRAME
	FrameRect(hdc, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
#endif

	SelectObject(hdc, oldFont);
}
