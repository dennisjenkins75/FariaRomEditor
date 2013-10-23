/* OverWorldGui.cpp */

#include "FariaRomEditor.h"

ZOOM_FACTORS zoom_factors[ZOOM_FACTOR_COUNT] =
{
	{ 1, 1 },	// Always have default zoom level first.
	{ 2, 1 },
	{ 3, 1 },
	{ 4, 1 },
	{ 1, 4 },
	{ 3, 8 },
	{ 1, 2 },
	{ 5, 8 },
};

static MAP_CONTAINER*	CreateMapDataStruct(HWND hWnd, MAP_INFO *pMapInfo)
{
	MAP_CONTAINER  *pMapData = (MAP_CONTAINER*)MyAlloc(sizeof(MAP_CONTAINER));

	pMapData->hWndContainer = hWnd;
	pMapData->hWndCanvas = NULL;	// Window will be created in WM_CREATE
	pMapData->hWndPalette = NULL;	// same
	pMapData->hWndTools = NULL;		// same
	pMapData->nCursorType = CM_TOOL_HAND_TOOL;

	pMapData->CanvasData.ptDisplayOffset.x = 0;
	pMapData->CanvasData.ptDisplayOffset.y = 0;
	pMapData->CanvasData.prev_highlight; // let default.
	pMapData->CanvasData.ptMouseDown.x = 0;
	pMapData->CanvasData.ptMouseDown.y = 0;
	pMapData->CanvasData.ptMouseUp.x = 0;
	pMapData->CanvasData.ptMouseUp.y = 0;
	pMapData->CanvasData.bLabelLocations = true;
	pMapData->CanvasData.nZoom = 0;		// 100%

	pMapData->PaletteData.prev_highlight = 0;
	pMapData->PaletteData.render_height = pMapInfo->tile_height * 2;
	pMapData->PaletteData.render_width = pMapInfo->tile_width * 2;
	pMapData->PaletteData.tile_spacing = 2;
	pMapData->PaletteData.x = 0;
	pMapData->PaletteData.current_tile = 0;

	pMapData->MapInfo = *pMapInfo;

	SetWindowText(hWnd, pMapInfo->szTitle);

	return pMapData;
}

static void DestroyMapDataStruct(MAP_CONTAINER *pMapData)
{
	_ASSERT(pMapData);
	MyFree(pMapData);
}

static void	SetCursorType(HWND hWnd, MAP_CONTAINER *pMapData, int nCursorType)
{
	pMapData->nCursorType = nCursorType;
	pMapData->CanvasData.bSelectionValid = false;

	// Can add other dependant behavior here.
}

static void	ActivateMapContainerMenu(HWND hWnd, MAP_CONTAINER *pMapData)
{
	if (!CheckMenuRadioItem (GetMenu(g_hMDIFrame), CM_TOOL_HAND_TOOL, CM_TOOL_PEN_TOOL, pMapData->nCursorType, MF_BYCOMMAND))
	{
		FatalError(GetLastError(), TEXT("CheckMenuRadioItem() failed"));
	}

	if (!CheckMenuRadioItem (GetMenu(g_hMDIFrame), CM_ZOOM_BASE, CM_ZOOM_BASE+ZOOM_FACTOR_COUNT-1, CM_ZOOM_BASE + pMapData->CanvasData.nZoom, MF_BYCOMMAND))
	{
		FatalError(GetLastError(), TEXT("CheckMenuRadioItem() failed"));
	}

	CheckMenuItem(GetMenu(g_hMDIFrame), CM_TOOL_LABELS, MF_BYCOMMAND | pMapData->CanvasData.bLabelLocations ? MF_CHECKED : MF_UNCHECKED);
}

void UpdateStatusBarCompression(MAP_CONTAINER *pMapData)
{
	int	nNeededBytes = 0;
	int nDiff = 0;
	TCHAR szTemp[256];

	if (pMapData->MapInfo.map_type == MAP_OVERWORLD)
	{
		nNeededBytes = FL_CompressOverworld(g_pRom, pMapData->MapInfo.pMap, true);
		nDiff = OVERWORLD_RLE_MAX_LEN - nNeededBytes;
		wnsprintf(szTemp, TCHAR_sizeof(szTemp), TEXT("RLE bytes free: %d"), nDiff);
		SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_COMPRESSION, (LPARAM)szTemp);
	}
	else
	{
		SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_COMPRESSION, (LPARAM)TEXT("No compression"));
	}
}

static const int sb_widths[SB_COLUMN_COUNT] =
{
	100,	// SB_COL_TITLE
	150,	// SB_COL_COORD
	150,	// SB_COL_COMPRESSION
	100,	// SB_COL_PALETTE_TILE
	100,	// SB_COL_ZOOM
};

static void	InitStatusBar(MAP_CONTAINER *pMapData)
{
	int		tab_stops[SB_COLUMN_COUNT+1];
	int		i;

	tab_stops[0] = sb_widths[0];
	for (i = 1; i < SB_COLUMN_COUNT; i++)
	{
		tab_stops[i] = tab_stops[i-1] + sb_widths[i];
	}
	tab_stops[i] = -1;	// extend last tab stop to right edge.

	SendMessage (g_hWndStatusBar, SB_SETPARTS, SB_COLUMN_COUNT+1, (LPARAM)tab_stops);
	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_TITLE, (LPARAM)TEXT("Map Editor"));
	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_COORD, (LPARAM)TEXT("Coordinate"));
	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_COMPRESSION, (LPARAM)TEXT("Compression"));
	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_PALETTE_TILE, (LPARAM)TEXT("Palette"));
	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_ZOOM, (LPARAM)TEXT("Zoom"));

	UpdateStatusBarCompression(pMapData);
}

static void	ComputeLayout(const MAP_CONTAINER *pMapData, const RECT *rContainer, RECT *rPalette, RECT *rTools, RECT *rCanvas)
{
	int pal_height = pMapData->PaletteData.render_height + 
		pMapData->PaletteData.tile_spacing * 2 +
		GetSystemMetrics(SM_CYHSCROLL);

/*
	int tools_width = GetSystemMetrics(SM_CXVSCROLL) + 64;
	rPalette->left = rContainer->left;
	rPalette->top = rContainer->bottom - pal_height;
	rPalette->right = rContainer->right;
	rPalette->bottom = rContainer->bottom;

	rTools->left = rContainer->left;
	rTools->top = rContainer->top;
	rTools->right = rContainer->left + tools_width;
	rTools->bottom = rPalette->top - 2;

	rCanvas->left = rTools->right + 2;
	rCanvas->right = rContainer->right;
	rCanvas->top = rContainer->top;
	rCanvas->bottom = rTools->bottom;
*/

	int tools_width = pMapData->PaletteData.render_width * 2;

	rTools->left = rContainer->left;
	rTools->top = rContainer->bottom - pal_height + 2;
	rTools->right = rContainer->left + tools_width;
	rTools->bottom = rContainer->bottom;

	rPalette->left = rTools->right;
	rPalette->top = rTools->top;
	rPalette->right = rContainer->right;
	rPalette->bottom = rContainer->bottom;

	rCanvas->left = rContainer->left;
	rCanvas->right = rContainer->right;
	rCanvas->top = rContainer->top;
	rCanvas->bottom = rTools->top;
}

static void	DoSize(HWND hWnd, WPARAM wParam, LPARAM lParam, MAP_CONTAINER *pData)
{
	RECT	rPalette;
	RECT	rTools;
	RECT	rCanvas;
	RECT	rContainer;

	GetClientRect(pData->hWndContainer, &rContainer);
	ComputeLayout(pData, &rContainer, &rPalette, &rTools, &rCanvas);

	SetWindowPos(pData->hWndPalette, NULL, 
		rPalette.left, rPalette.top, 
		rPalette.right - rPalette.left, rPalette.bottom - rPalette.top, 
		SWP_NOZORDER);

	SetWindowPos(pData->hWndTools, NULL, 
		rTools.left, rTools.top, 
		rTools.right - rTools.left, rTools.bottom - rTools.top, 
		SWP_NOZORDER);

	SetWindowPos(pData->hWndCanvas, NULL, 
		rCanvas.left, rCanvas.top, 
		rCanvas.right - rCanvas.left, rCanvas.bottom - rCanvas.top, 
		SWP_NOZORDER);

	InvalidateRect(hWnd, NULL, false);
}


// Just like MSPAINT:
//  Status bar at very bottom, all the way across.
//  "Palette" window above status bar, all the way across.
//  "Tools" window on left, from top down to top of palette.
//  "Canvas" above palette and to the right of tools, all the way across and up.

static void	DoCreate(HWND hWnd, LPARAM lParam)
{
	MAP_CONTAINER	*pMapData = NULL;
	MAP_INFO	*pMapInfo = NULL;
	RECT		rPalette;
	RECT		rTools;
	RECT		rCanvas;
	RECT		rContainer;
	DWORD		dwStyle;
	DWORD		dwExStyle;

	// Worst pointer manipulation ever.
	// Read the "Remarks" section of the Platform SDK for "MDICREATESTRUCT"
	pMapInfo = (MAP_INFO*)(((MDICREATESTRUCT *)(((CREATESTRUCT *)lParam)->lpCreateParams))->lParam);
	pMapData = CreateMapDataStruct(hWnd, pMapInfo);
	SetWindowLongPtr(hWnd, OFFSET_MAP_CONTAINER, (LONG_PTR)pMapData);

	GetClientRect(pMapData->hWndContainer, &rContainer);
	ComputeLayout(pMapData, &rContainer, &rPalette, &rTools, &rCanvas);

	// WM_CREATE handler for the child windows will try to modify the status bar.
	// so we must create it before the other windows.
	InitStatusBar(pMapData);
	SetCursorType(hWnd, pMapData, CM_TOOL_HAND_TOOL);

	dwStyle = WS_VISIBLE | WS_CHILD | WS_HSCROLL;
	dwExStyle = 0;
	if (NULL == (pMapData->hWndPalette = CreateWindowEx(
		dwExStyle,						// dwExStyle
		szClassMapEditorPalette,		// lpClassName
		TEXT("Palette"),				// lpWindowName
		dwStyle,						// dwStyle
		rPalette.left,					// X
		rPalette.top,					// Y
		rPalette.right-rPalette.left,	// nWidth
		rPalette.bottom-rPalette.top,	// nHeight
		hWnd,							// hWndParent
		(HMENU)NULL,					// hMenu
		g_hInstance,					// hInstance
		(void*)pMapData)))				// lParam
	{
		FatalError(GetLastError(), TEXT("CreateMDIWindow('%s') failed."), szClassMapEditorPalette);
	}

	dwStyle = WS_VISIBLE | WS_CHILD;
	dwExStyle = 0;
	if (NULL == (pMapData->hWndTools = CreateWindowEx(
		dwExStyle,						// dwExStyle
		szClassMapEditorTools,			// lpClassName
		TEXT("Tools"),					// lpWindowName
		dwStyle,						// dwStyle
		rTools.left,					// X
		rTools.top,						// Y
		rTools.right-rTools.left,		// nWidth
		rTools.bottom-rTools.top,		// nHeight
		hWnd,							// hWndParent
		(HMENU)NULL,					// hMenu
		g_hInstance,					// hInstance
		(void*)pMapData)))				// lParam
	{
		FatalError(GetLastError(), TEXT("CreateMDIWindow('%s') failed."), szClassMapEditorTools);
	}

	dwStyle = WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL;
	dwExStyle = 0;
	if (NULL == (pMapData->hWndCanvas = CreateWindowEx(
		dwExStyle,						// dwExStyle
		szClassMapEditorCanvas,			// lpClassName
		TEXT("Canvas"),					// lpWindowName
		dwStyle,						// dwStyle
		rCanvas.left,					// X
		rCanvas.top,					// Y
		rCanvas.right-rCanvas.left,		// nWidth
		rCanvas.bottom-rCanvas.top,		// nHeight
		hWnd,							// hWndParent
		(HMENU)NULL,					// hMenu
		g_hInstance,					// hInstance
		(void*)pMapData)))				// lParam
	{
		FatalError(GetLastError(), TEXT("CreateMDIWindow('%s') failed."), szClassMapEditorCanvas);
	}
}

// With rare exception, all messages should call "DefMDIChildProc()" instead of 
// just doing a "return 0" or "return 1".
long FAR PASCAL MapEditorContainer_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MAP_CONTAINER		*pMapData = (MAP_CONTAINER*)GetWindowLongPtr (hWnd, OFFSET_MAP_CONTAINER);

	switch (message)
	{
		case WM_CREATE:
			DoCreate(hWnd, lParam);
			// NOTE: "pMapData" is NULL right now!
			break;
			
		case WM_DESTROY:
			DestroyMapDataStruct(pMapData);
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)NULL);
			break;

		case WM_SIZE:
			DoSize(hWnd, wParam, lParam, pMapData);
			break;
			
		case WM_MDIACTIVATE:
			// Set our menu if we are gaining focus.
			if (lParam == (LPARAM) hWnd)
			{
				SendMessage (GetParent(hWnd), WM_MDISETMENU, (WPARAM) g_hMenuMapEditor, (LPARAM) g_hMenuMapEditor_Window);
				InitStatusBar(pMapData);
				ActivateMapContainerMenu(hWnd, pMapData);
			}

			// Do other things, like enable, disable, check, uncheck menu items.

			// If we are losing focus, restore original menu.
			if (lParam != (LPARAM) hWnd)
			{
				SendMessage (GetParent(hWnd), WM_MDISETMENU, (WPARAM) g_hMenuFrame, (LPARAM) g_hMenuFrame_Window);
				ResetStatusBar();
			}

			DrawMenuBar (g_hMDIFrame);
			return 0 ;

		// Pass these to the canvas for processing.
		case WM_USER_MAP_EDITOR_UPDATE_TILE:
		case WM_MOUSEWHEEL:				// zoom in/out
			SendMessage(pMapData->hWndCanvas, message, wParam, lParam);
			return 0;	// Must return "0" to indicate that we've procecessed WM_MOUSEWHEEL

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case CM_TOOL_PEN_TOOL:
				case CM_TOOL_SELECT_TOOL:
				case CM_TOOL_HAND_TOOL:
					SetCursorType(hWnd, pMapData, LOWORD(wParam));
					ActivateMapContainerMenu(hWnd, pMapData);
					break;

				case CM_TOOL_LABELS:
					pMapData->CanvasData.bLabelLocations ^= true;
					ActivateMapContainerMenu(hWnd, pMapData);
					InvalidateRect(pMapData->hWndCanvas, NULL, false);
					break;

				default:
					if ((LOWORD(wParam) >= CM_ZOOM_BASE) && (LOWORD(wParam) <= (CM_ZOOM_BASE + ZOOM_FACTOR_COUNT)))
					{
						SendMessage(pMapData->hWndCanvas, message, wParam, lParam);
						return 0;
					}
					break;
			}
			break;
	}

	return DefMDIChildProc (hWnd, message, wParam, lParam) ;
}


void	CreateMapEditorWindow(MAP_INFO *pMapInfo)
{
	_ASSERT(g_pRom);
	_ASSERT(pMapInfo);

	DWORD dwStyle = 0;
	HWND hWnd;

	if (NULL == (hWnd = CreateMDIWindow(
		szClassMapEditorContainer,				// lpClassName
		TEXT("Map Editor"),			// lpWindowName
		dwStyle,					// dwStyle
		CW_USEDEFAULT,				// X
		CW_USEDEFAULT,				// Y
		CW_USEDEFAULT,				// nWidth
		CW_USEDEFAULT,				// nHeight
		g_hMDIClient,				// hWndParent
		g_hInstance,				// hInstance
		(LPARAM)pMapInfo)))			// lParam
	{
		FatalError(GetLastError(), TEXT("CreateMDIWindow('%s') failed."), szClassMapEditorContainer);
	}
}



void	RegisterMapEditorClass(void)
{
    WNDCLASSEX wc; 

	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc); 
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MapEditorContainer_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = MAP_CONTAINER_EXTRA_BYTES;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = g_hIconLand;
    wc.hCursor       = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_SCROLLBAR + 1); 
    wc.lpszMenuName  = (LPCTSTR)NULL;
    wc.lpszClassName = szClassMapEditorContainer;
	wc.hIconSm		 = NULL;

    if (!RegisterClassEx (&wc)) 
	{
		FatalError(GetLastError(), TEXT("RegisterClassEx('%s') failed"), wc.lpszClassName);
	}

	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc); 
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MapEditorCanvas_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(MAP_CONTAINER*);
    wc.hInstance     = g_hInstance;
    wc.hIcon         = (HICON)NULL;
    wc.hCursor       = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1); 
    wc.lpszMenuName  = (LPCTSTR)NULL;
    wc.lpszClassName = szClassMapEditorCanvas;
	wc.hIconSm		 = NULL;
 
    if (!RegisterClassEx (&wc)) 
	{
		FatalError(GetLastError(), TEXT("RegisterClassEx('%s') failed"), wc.lpszClassName);
	}

	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc); 
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MapEditorPalette_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(MAP_CONTAINER*);
    wc.hInstance     = g_hInstance;
    wc.hIcon         = (HICON)NULL;
    wc.hCursor       = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1); 
    wc.lpszMenuName  = (LPCTSTR)NULL;
    wc.lpszClassName = szClassMapEditorPalette;
	wc.hIconSm		 = NULL;
 
    if (!RegisterClassEx (&wc)) 
	{
		FatalError(GetLastError(), TEXT("RegisterClassEx('%s') failed"), wc.lpszClassName);
	}

	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc); 
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MapEditorTools_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(MAP_CONTAINER*);
    wc.hInstance     = g_hInstance;
    wc.hIcon         = (HICON)NULL;
    wc.hCursor       = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1); 
    wc.lpszMenuName  = (LPCTSTR)NULL;
    wc.lpszClassName = szClassMapEditorTools;
	wc.hIconSm		 = NULL;
 
    if (!RegisterClassEx (&wc)) 
	{
		FatalError(GetLastError(), TEXT("RegisterClassEx('%s') failed"), wc.lpszClassName);
	}
}