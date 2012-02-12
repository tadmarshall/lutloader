// MonitorSummaryItem.h -- Display summary information about a monitor on the Summary page
//

#pragma once
#include "stdafx.h"
#include "Monitor.h"
#include "MonitorPage.h"

class MonitorSummaryItem {

public:
	MonitorSummaryItem(Monitor * hostMonitor);

	static MonitorSummaryItem * Add(MonitorSummaryItem * monitorSummaryItem);
	static void ClearList(bool freeAllMemory);

	HWND CreateMonitorSummaryItemWindow(
			HWND viewerParent,
			int x,
			int y,
			int width,
			int height );
	void Update(void);

	static int GetDesiredHeight(HDC hdc);
	static void RegisterWindowClass(void);
	static size_t GetListSize(void);

private:
	void DrawTextOnDC(HDC hdc);

	static LRESULT CALLBACK MonitorSummaryItemProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	Monitor *			monitor;
	HWND				hwnd;
	HWND				hwndLoadLutButton;
	HWND				hwndRescanButton;
	RECT				headingRect;
	RECT				activeProfileRect;
	RECT				lutStatusRect;
	RECT				primaryMonitorRect;
	bool				lutViewShowsProfile;
	wstring				activeProfileDisplayText;
	UINT				paintCount;
};
