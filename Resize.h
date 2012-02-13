// Resize.h -- Handle moving and resizing windows when the user resizes the main window
//

#pragma once
#include "stdafx.h"

// Controls and dialogs to be moved or resized when the property sheet is resized
//
typedef struct tag_RESIZECHILD {		// Built by SetupForResizing()
	HWND			hwnd;				// Window handle
	RECT			rect;				// Window's RECT in client coordinates
	bool			anchorLeft;			// How the control is affected by resizing
	bool			anchorTop;
	bool			anchorRight;
	bool			anchorBottom;
} RESIZECHILD;

typedef struct tag_ANCHOR_PRESET {		// Achors are bottom+right unless preset
	HWND			hwnd;				// Window handle
	bool			anchorLeft;			// How the control is affected by resizing
	bool			anchorTop;
	bool			anchorRight;
	bool			anchorBottom;
} ANCHOR_PRESET;

class Resize {

public:
	Resize(HWND parent);
	HWND GetParentWindow(void);
	void AddChildWindow(const RESIZECHILD & child);

	static bool GetNeedRebuild(void);
	static void SetNeedRebuild(bool isRebuildNeeded);
	static void AddAnchorPreset(const ANCHOR_PRESET & anchorPreset);
	static void SetupForResizing(HWND parentBase);
	static void MainWindowHasResized(const WINDOWPOS & windowPos);
	static void ClearResizeList(bool freeAllMemory);
	static void ClearAnchorPresetList(bool freeAllMemory);

private:
	HWND parentWindow;
	vector <RESIZECHILD> childList;
};
