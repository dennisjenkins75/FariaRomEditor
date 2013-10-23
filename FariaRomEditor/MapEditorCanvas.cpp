/* MapEditorCanvas.cpp */

#include "FariaRomEditor.h"

static void	RenderMapTiles(HWND hWnd, HDC hDC, MAP_CONTAINER *pMapData, RECT *rTiles);
static void	DoSetCursor(HWND hWnd, MAP_CONTAINER *pMapData);
static void LabelLocation(HDC hDC, MAP_CONTAINER *pMapData, int col, int row, int x, int y);

static inline int Zoom(MAP_CONTAINER *pMapData, int value)
{
	int n = zoom_factors[pMapData->CanvasData.nZoom].nNumerator;
	int d = zoom_factors[pMapData->CanvasData.nZoom].nDenominator;

	return (value * n) / d;
}

static inline BYTE GetMapTile(MAP_CONTAINER *pMapData, int col, int row)
{
	register int w = pMapData->MapInfo.map_width;
	register int h = pMapData->MapInfo.map_height;

	if ((col >= 0) && (col < w) && (row >= 0) && (row < h))
	{
		return pMapData->MapInfo.pMap[ row * w + col];
	}

	return 0;
}

static inline void SetMapTile(MAP_CONTAINER *pMapData, int col, int row, BYTE tile)
{
	register int w = pMapData->MapInfo.map_width;
	register int h = pMapData->MapInfo.map_height;

	if ((col >= 0) && (col < w) && (row >= 0) && (row < h))
	{
		pMapData->MapInfo.pMap[ row * w + col] = tile;
	}
}

// Given pixel offsets into client window, returns map tile col, row
// for the tile at that pixel location.  Returns "false" if tile is "out of bounds".
static inline bool ScreenToTile(MAP_CONTAINER *pMapData, int x, int y, int &col, int &row)
{
	col = (x + pMapData->CanvasData.ptDisplayOffset.x) / Zoom(pMapData, pMapData->MapInfo.tile_width);
	row = (y + pMapData->CanvasData.ptDisplayOffset.y) / Zoom(pMapData, pMapData->MapInfo.tile_height);

	return ((col >= 0) && (col < pMapData->MapInfo.map_width) && (row >= 0) && (row < pMapData->MapInfo.map_height));
}

static inline bool ScreenToTile(MAP_CONTAINER *pMapData, long x, long y, long &col, long &row)
{
	col = (x + pMapData->CanvasData.ptDisplayOffset.x) / Zoom(pMapData, pMapData->MapInfo.tile_width);
	row = (y + pMapData->CanvasData.ptDisplayOffset.y) / Zoom(pMapData, pMapData->MapInfo.tile_height);

	return ((col >= 0) && (col < pMapData->MapInfo.map_width) && (row >= 0) && (row < pMapData->MapInfo.map_height));
}

static inline void TileToScreen(MAP_CONTAINER *pMapData, int &x, int &y, int col, int row)
{
	x = col * Zoom(pMapData, pMapData->MapInfo.tile_width) - pMapData->CanvasData.ptDisplayOffset.x;
	y = row * Zoom(pMapData, pMapData->MapInfo.tile_height) - pMapData->CanvasData.ptDisplayOffset.y;
}

// "She looks like the princess, but she is not."
static inline void TileToScreen(MAP_CONTAINER *pMapData, long &x, long &y, long col, long row)
{
	x = col * Zoom(pMapData, pMapData->MapInfo.tile_width) - pMapData->CanvasData.ptDisplayOffset.x;
	y = row * Zoom(pMapData, pMapData->MapInfo.tile_height) - pMapData->CanvasData.ptDisplayOffset.y;
}

static void	UpdateStatusBarZoomDisplay(MAP_CONTAINER *pMapData)
{
	TCHAR szTemp[64];
	wnsprintf(szTemp, TCHAR_sizeof(szTemp), TEXT("Zoom: %d%%"), Zoom(pMapData, 100));
	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_ZOOM, (LPARAM)szTemp);
}

static void	SetScrollBarsFromCanvasCoords(HWND hWnd, MAP_CONTAINER *pMapData)
{
	SCROLLINFO sbi;

	memset(&sbi, 0, sizeof(sbi));
	sbi.cbSize = sizeof(sbi);
	sbi.fMask = SIF_POS;
	sbi.nPos = pMapData->CanvasData.ptDisplayOffset.x + Zoom(pMapData, pMapData->MapInfo.tile_width * EXTRA_COL_TILES);
	SetScrollInfo(hWnd, SB_HORZ, &sbi, true);

	sbi.nPos = pMapData->CanvasData.ptDisplayOffset.y + Zoom(pMapData, pMapData->MapInfo.tile_height * EXTRA_ROW_TILES);
	SetScrollInfo(hWnd, SB_VERT, &sbi, true);
}

static void	ClampCanvasCoords(HWND hWnd, MAP_CONTAINER *pMapData)
{
	RECT r;

	GetClientRect(hWnd, &r);

	int x_min = - Zoom(pMapData, pMapData->MapInfo.tile_width * EXTRA_COL_TILES);
	int y_min = - Zoom(pMapData, pMapData->MapInfo.tile_height * EXTRA_ROW_TILES);
	int x_max = Zoom(pMapData, pMapData->MapInfo.tile_width * (pMapData->MapInfo.map_width + EXTRA_COL_TILES)) - r.right;
	int y_max = Zoom(pMapData, pMapData->MapInfo.tile_width * (pMapData->MapInfo.map_height + EXTRA_ROW_TILES)) - r.bottom;

	// If the map is smaller than the window, then "x_max < x_min", which is just plain WRONG.
	
	if (x_max < x_min)
	{
		x_max = x_min;
	} 

	if (y_max < y_min)
	{
		y_max = y_min;
	}

	if (pMapData->CanvasData.ptDisplayOffset.x < x_min)
	{
		pMapData->CanvasData.ptDisplayOffset.x = x_min;
	}
	else if (pMapData->CanvasData.ptDisplayOffset.x > x_max)
	{
		pMapData->CanvasData.ptDisplayOffset.x = x_max;
	}
	
	if (pMapData->CanvasData.ptDisplayOffset.y < y_min)
	{
		pMapData->CanvasData.ptDisplayOffset.y = y_min;
	}
	else if (pMapData->CanvasData.ptDisplayOffset.y > y_max)
	{
		pMapData->CanvasData.ptDisplayOffset.y = y_max;
	}
}

// Determine tile that is "dead-center" in the window.
static void	GetCenterPoint (MAP_CONTAINER *pMapData, int &col, int &row)
{
	RECT rect;

	GetClientRect(pMapData->hWndCanvas, &rect);
	ScreenToTile(pMapData, rect.right / 2, rect.bottom / 2, col, row);
}

// Scroll window and scroll bars such that specified tile is as close to the center
// of the screen as possible.
static void	SetCenterPoint (MAP_CONTAINER *pMapData, int col, int row)
{
	int w = Zoom(pMapData, pMapData->MapInfo.tile_height);
	int h = Zoom(pMapData, pMapData->MapInfo.tile_width);
	RECT rect;

	GetClientRect(pMapData->hWndCanvas, &rect);

	pMapData->CanvasData.ptDisplayOffset.x = col * w - rect.right/2;
	pMapData->CanvasData.ptDisplayOffset.y = col * h - rect.bottom/2;

	SetScrollBarsFromCanvasCoords(pMapData->hWndCanvas, pMapData);

	InvalidateRect(pMapData->hWndCanvas, NULL, false);
}


// rTiles should contain tile indicies, not pixels.
static void	HighlightTile(HWND hWnd, MAP_CONTAINER *pMapData, RECT *rTiles, COLORREF rgb)
{
	RECT rScreen;
	HDC hDC;
	HBRUSH hBrush;

	_ASSERT(pMapData);
	_ASSERT(rTiles);

	// Compute bounding rectangle that includes all tiles in 'rTiles'.
	TileToScreen(pMapData, rScreen.left, rScreen.top, rTiles->left, rTiles->top);
	TileToScreen(pMapData, rScreen.right, rScreen.bottom, rTiles->right + 1, rTiles->bottom+1);
	rScreen.right;
	rScreen.bottom;

	// Draw it on screen.
	hDC = GetDC (hWnd) ;

	// Restore previous map tile(s).
	RenderMapTiles(hWnd, hDC, pMapData, &(pMapData->CanvasData.prev_highlight));

	hBrush = CreateSolidBrush (rgb);
	SelectObject (hDC, hBrush);

	//Rectangle (hdc, rScreen.left, rScreen.top, rScreen.right, rScreen.bottom);
	FrameRect(hDC, &rScreen, hBrush);

	ReleaseDC (hWnd, hDC) ;
	DeleteObject (hBrush) ;

	pMapData->CanvasData.prev_highlight = *rTiles;
}

static void	EditTileWithPenTool(HWND hWnd, MAP_CONTAINER *pMapData, int mouse_x, int mouse_y)
{
	int col = 0;
	int row = 0;
	BYTE old_tile = 0;
	RECT rTiles;
	MAP_UPDATE_STRUCT mus;

	if (ScreenToTile(pMapData, mouse_x, mouse_y, col, row))
	{
		rTiles.left = rTiles.right = col;
		rTiles.top = rTiles.bottom = row;

		old_tile = GetMapTile(pMapData, col, row);

		if (old_tile == pMapData->PaletteData.current_tile)
		{
			return;
		}

		SetMapTile(pMapData, col, row, pMapData->PaletteData.current_tile);
		HighlightTile(hWnd, pMapData, &rTiles, g_rgbHighlightColor);

		// Tell frame to let all other map editors know that we updated a tile.
		mus.rUpdateRect = rTiles;
		mus.pMapData = pMapData;
		SendMessage(g_hMDIFrame, WM_USER_MAP_EDITOR_UPDATE_TILE, 0, (LPARAM)&mus);

		pMapData->CanvasData.bIdleDirty = true;
		PostMessage(hWnd, WM_RECALC_COMPRESSION, 0, 0);
	}
}

static void	DoFillTiles(HWND hWnd, MAP_CONTAINER *pMapData)
{
	int row, col;
	RECT rSel = pMapData->CanvasData.rSelection;
	BYTE old_tile;
	MAP_UPDATE_STRUCT mus;

	NormalizeRect(&rSel);

	for (row = rSel.top; row <= rSel.bottom; row++)
	{
		for (col = rSel.left; col <= rSel.right; col++)
		{
			old_tile = GetMapTile(pMapData, col, row);

			if (old_tile == pMapData->PaletteData.current_tile)
			{
				continue;
			}

			SetMapTile(pMapData, col, row, pMapData->PaletteData.current_tile);
		}
	}

	// Tell frame to let all other map editors know that we updated a tile.
	mus.rUpdateRect = rSel;
	mus.pMapData = pMapData;
	SendMessage(g_hMDIFrame, WM_USER_MAP_EDITOR_UPDATE_TILE, 0, (LPARAM)&mus);

	pMapData->CanvasData.bIdleDirty = true;
	PostMessage(hWnd, WM_RECALC_COMPRESSION, 0, 0);
}

static void	DrawSelectionRectangle(HWND hWnd, HDC hDC, MAP_CONTAINER *pMapData)
{
	RECT rScreen;
	RECT rTiles;
	HBRUSH hBrush;
	HBRUSH hOldBrush;

	if (!pMapData->CanvasData.bSelectionValid)
	{
		return;
	}

	rTiles = pMapData->CanvasData.rSelection;
	NormalizeRect(&rTiles);

	TileToScreen(pMapData, rScreen.left, rScreen.top, rTiles.left, rTiles.top);
	TileToScreen(pMapData, rScreen.right, rScreen.bottom, rTiles.right+1, rTiles.bottom+1);

	rScreen.left--;
	rScreen.top--;
	
	hBrush = CreateHatchBrush (HS_BDIAGONAL, RGB(255,255,255));
	hOldBrush = (HBRUSH)SelectObject (hDC, hBrush);

	// Rectangle (hDC, rScreen.left, rScreen.top, rScreen.right, rScreen.bottom);
	FrameRect(hDC, &rScreen, hBrush);

	SelectObject (hDC, hOldBrush);
	DeleteObject (hBrush) ;

	pMapData->CanvasData.prev_highlight = rTiles;
}

static bool PasteTilesFromClipboard(HWND hWnd, MAP_CONTAINER *pMapData, RECT *rTiles, bool bPattern)
{
	HANDLE hObject = NULL;
	MAP_CLIPBOARD_FORMAT *cbf = NULL;
	bool bResult = false;
	int x, y, src_x, src_y, dest_x, dest_y, dest_x_max, dest_y_max;
	BYTE new_tile = 0;
	BYTE old_tile = 0;
	int src_w, src_h;
	MAP_UPDATE_STRUCT mus;

	if (!OpenClipboard(hWnd))
	{
		return false;	// no need to call CloseClipboard()
	}

	if (NULL == (hObject = GetClipboardData(g_uMapClipboardFormat)))
	{
		goto exit;
	}

	if (NULL == (cbf = (MAP_CLIPBOARD_FORMAT *)GlobalLock(hObject)))
	{
		goto exit;
	}

	// Sanity checks.
	if (cbf->map_type != pMapData->MapInfo.map_type)
	{
		goto exit;
	}

	src_w = cbf->rOrigin.right - cbf->rOrigin.left + 1;
	src_h = cbf->rOrigin.bottom - cbf->rOrigin.top + 1;

	if (bPattern)
	{
		dest_x_max = rTiles->right + 1;
		dest_y_max = rTiles->bottom + 1;
	}
	else
	{
		dest_x_max = rTiles->left + src_w;
		dest_y_max = rTiles->top + src_h;

		dest_x_max = min(dest_x_max, rTiles->right + 1);
		dest_y_max = min(dest_y_max, rTiles->bottom + 1);
	}

	for (y = 0, dest_y = rTiles->top; dest_y < dest_y_max; y++, dest_y++)
	{
		src_y = y % src_h;

		for (x = 0, dest_x = rTiles->left; dest_x < dest_x_max; x++, dest_x++)
		{
			src_x = x % src_w;

			old_tile = GetMapTile(pMapData, dest_x, dest_y);
			new_tile = cbf->ucTiles[src_y * src_w + src_x];

			if (new_tile != old_tile)
			{
				SetMapTile(pMapData, dest_x, dest_y, new_tile);
			}
		}
	}

	// Tell frame to let all other map editors know that we updated a tile.
	mus.rUpdateRect = *rTiles;
	mus.pMapData = pMapData;
	SendMessage(g_hMDIFrame, WM_USER_MAP_EDITOR_UPDATE_TILE, 0, (LPARAM)&mus);

	pMapData->CanvasData.bIdleDirty = true;
	PostMessage(hWnd, WM_RECALC_COMPRESSION, 0, 0);

	bResult = true;

exit:
	if (hObject)
	{
		GlobalUnlock(hObject);
	}
	CloseClipboard();
	return bResult;
}

static bool CopyTilesToClipboard(HWND hWnd, MAP_CONTAINER *pMapData, RECT *rTiles)
{
	HGLOBAL hglbCopy = NULL;
	MAP_CLIPBOARD_FORMAT *cbf = NULL;
	DWORD dwBytes = 0;
	int width = 0;
	int height = 0;
	int x = 0;
	int y = 0;
	BYTE *pDest = NULL;
	RECT rTemp = *rTiles;

	NormalizeRect(&rTemp);


	height = rTemp.bottom - rTemp.top + 1;
	width = rTemp.right - rTemp.left + 1;
	dwBytes = sizeof(MAP_CLIPBOARD_FORMAT) + (height * width - 1) * sizeof(BYTE);

    if (NULL == (hglbCopy = GlobalAlloc(GMEM_MOVEABLE, dwBytes)))
	{
		return false;
	}

    cbf = (MAP_CLIPBOARD_FORMAT*) GlobalLock(hglbCopy); 

	cbf->rOrigin = rTemp;
	cbf->map_type = pMapData->MapInfo.map_type;
	pDest = cbf->ucTiles;

	for (y = rTemp.top; y <= rTemp.bottom; y++)
	{
		for (x = rTemp.left; x <= rTemp.right; x++)
		{
			*pDest = GetMapTile(pMapData, x, y);
			pDest++;
		}
	}

    GlobalUnlock(hglbCopy); 

	if (!OpenClipboard(hWnd))
	{
		GlobalFree(hglbCopy);
		return false;
	}

	EmptyClipboard();
	SetClipboardData(g_uMapClipboardFormat, hglbCopy);
	CloseClipboard();

	return true;
}

static void	DoSampleTile(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	int col = 0;
	int row = 0;
	BYTE tile = 0;

	POINT ptMouse = pMapData->CanvasData.ptRightClick;	

	if (ScreenToTile(pMapData, ptMouse.x, ptMouse.y, col, row))
	{
		tile = GetMapTile(pMapData, col, row);
		SendMessage(pMapData->hWndPalette, WM_USER_PALETTE_SET_CUR_TILE, (WPARAM)tile, 0);
	}
}

static void	DoPopupMenu(HWND hWnd, int uMenuResId)
{
	HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(uMenuResId));
	HMENU hPopup = GetSubMenu(hMenu, 0);
	POINT pt;

	GetCursorPos(&pt);
	TrackPopupMenu(hPopup, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);
}

static void	DoHandToolRightButtonDown(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	DoPopupMenu(hWnd, MENU_CANVAS_POPUP_2);
}

static void	DoPenToolRightButtonDown(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	DoPopupMenu(hWnd, MENU_CANVAS_POPUP_3);
}

static void	DoSelectToolRightButtonDown(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	if (pMapData->CanvasData.bSelectionValid)
	{
		DoPopupMenu(hWnd, MENU_CANVAS_POPUP_1);
	}
	else
	{
		DoPopupMenu(hWnd, MENU_CANVAS_POPUP_2);
	}
}

static void	DoLeftButtonDown(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	int mouse_x = LOWORD(lParam);
	int mouse_y = HIWORD(lParam);

	pMapData->CanvasData.bDragging = false;
	pMapData->CanvasData.bSelecting = false;

	switch (pMapData->nCursorType)
	{
		case CM_TOOL_PEN_TOOL:
			EditTileWithPenTool(hWnd, pMapData, mouse_x, mouse_y);
			return;

		case CM_TOOL_HAND_TOOL:
			pMapData->CanvasData.ptMouseDown.x = mouse_x;
			pMapData->CanvasData.ptMouseDown.y = mouse_y;
			pMapData->CanvasData.bDragging = true;
			DoSetCursor(hWnd, pMapData);
			return;

		case CM_TOOL_SELECT_TOOL:
			pMapData->CanvasData.ptMouseDown.x = mouse_x;
			pMapData->CanvasData.ptMouseDown.y = mouse_y;
			ScreenToTile(pMapData, mouse_x, mouse_y, pMapData->CanvasData.rSelection.left, pMapData->CanvasData.rSelection.top);
			pMapData->CanvasData.rSelection.bottom = pMapData->CanvasData.rSelection.top;
			pMapData->CanvasData.rSelection.right = pMapData->CanvasData.rSelection.left;
			pMapData->CanvasData.bSelecting = true;
			pMapData->CanvasData.bSelectionValid = true;
			DoSetCursor(hWnd, pMapData);
			return;
	}
}

static void	DoLeftButtonUp(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	int mouse_x = pMapData->CanvasData.ptMouseUp.x = LOWORD(lParam);
	int mouse_y = pMapData->CanvasData.ptMouseUp.y = HIWORD(lParam);

	switch (pMapData->nCursorType)
	{
		case CM_TOOL_HAND_TOOL:
			pMapData->CanvasData.bDragging = false;
			DoSetCursor(hWnd, pMapData);
			break;

		case CM_TOOL_SELECT_TOOL:
			pMapData->CanvasData.bSelecting = false;
			DoSetCursor(hWnd, pMapData);
			ScreenToTile(pMapData, mouse_x, mouse_y, pMapData->CanvasData.rSelection.right, pMapData->CanvasData.rSelection.bottom);
			break;
	}

	// Set this to false in all cases, incase the state of the cursor gets out
	// of sync with the state of the mouse button.  This can occur if the user
	// begins a drag operation and then uses a keyboard shortcut to change the
	// cursor type.
	pMapData->CanvasData.bDragging = false;
	pMapData->CanvasData.bSelecting = false;
}

static void	DoRightButtonDown(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	pMapData->CanvasData.ptRightClick.x = GET_X_LPARAM(lParam);
	pMapData->CanvasData.ptRightClick.y = GET_Y_LPARAM(lParam);

	switch (pMapData->nCursorType)
	{
		case CM_TOOL_PEN_TOOL:
			DoPenToolRightButtonDown(hWnd, pMapData, wParam, lParam);
			break;

		case CM_TOOL_SELECT_TOOL:
			DoSelectToolRightButtonDown(hWnd, pMapData, wParam, lParam);
			break;

		case CM_TOOL_HAND_TOOL:
			DoHandToolRightButtonDown(hWnd, pMapData, wParam, lParam);
			break;
	}
}

static void	DoMouseMove(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	int mouse_x = LOWORD(lParam);
	int mouse_y = HIWORD(lParam);
	int col = 0;
	int row = 0;
	BYTE tile;
	TCHAR szTemp[128];
	RECT rTiles;
	COLORREF high_light = 0;

	_ASSERT(pMapData);

	switch (pMapData->nCursorType)
	{
		case CM_TOOL_PEN_TOOL:
			if (wParam & MK_LBUTTON)
			{
				EditTileWithPenTool(hWnd, pMapData, mouse_x, mouse_y);
			}
			break;

		case CM_TOOL_HAND_TOOL:
			if (pMapData->CanvasData.bDragging)
			{
				// using "row", "col" as pixel/cursor coord temps.
				col = pMapData->CanvasData.ptMouseDown.x - mouse_x;	// delta_x
				row = pMapData->CanvasData.ptMouseDown.y - mouse_y;	// delta_x

				pMapData->CanvasData.ptDisplayOffset.x += col;
				pMapData->CanvasData.ptDisplayOffset.y += row;

				ClampCanvasCoords(hWnd, pMapData);

				// Update the scroll bars.
				SetScrollBarsFromCanvasCoords(hWnd, pMapData);

				pMapData->CanvasData.ptMouseDown.x = mouse_x;
				pMapData->CanvasData.ptMouseDown.y = mouse_y;

				InvalidateRect(hWnd, NULL, false);
			}
			break;

		case CM_TOOL_SELECT_TOOL:
			if (pMapData->CanvasData.bSelecting)
			{
				pMapData->CanvasData.ptMouseUp.x = mouse_x;
				pMapData->CanvasData.ptMouseUp.y = mouse_y;
				ScreenToTile(pMapData, mouse_x, mouse_y, pMapData->CanvasData.rSelection.right, pMapData->CanvasData.rSelection.bottom);
				InvalidateRect(hWnd, NULL, false);
			}
			break;
	}

	if (ScreenToTile(pMapData, mouse_x, mouse_y, col, row))
	{
		tile = GetMapTile(pMapData, col, row);
		wnsprintf(szTemp, TCHAR_sizeof(szTemp), TEXT("Coordinate: $%02x, $%02x = $%02x"), col, row, tile);
		high_light = g_rgbHighlightColor;
	}
	else
	{
		wnsprintf(szTemp, TCHAR_sizeof(szTemp), TEXT("Coordinate: Out of bounds"));
		high_light = g_rgbOutOfBoundsColor;
	}

	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_COORD, (LPARAM)szTemp);

	rTiles.top = rTiles.bottom = row;
	rTiles.left = rTiles.right = col;

	HighlightTile(hWnd, pMapData, &rTiles, high_light);
}

static void	DoMouseWheel(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
// Need to adjust scroll position such that the screen stays in place.
	int col, row;
	
	GetCenterPoint(pMapData, col, row);

	pMapData->CanvasData.nMouseWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);

// FIXME: Find a mathematically better way to do this.
	while (pMapData->CanvasData.nMouseWheelDelta >= WHEEL_DELTA)
	{
		pMapData->CanvasData.nZoom++;
		pMapData->CanvasData.nMouseWheelDelta -= WHEEL_DELTA;
	}

	while (pMapData->CanvasData.nMouseWheelDelta <= -WHEEL_DELTA)
	{
		pMapData->CanvasData.nZoom--;
		pMapData->CanvasData.nMouseWheelDelta += WHEEL_DELTA;
	}

	while (pMapData->CanvasData.nZoom < 0)
	{
		pMapData->CanvasData.nZoom += ZOOM_FACTOR_COUNT * 256;
	}

	pMapData->CanvasData.nZoom %= ZOOM_FACTOR_COUNT;

	UpdateStatusBarZoomDisplay(pMapData);

	SetCenterPoint(pMapData, col, row);	// Will force screen redraw.

	if (!CheckMenuRadioItem (GetMenu(g_hMDIFrame), CM_ZOOM_BASE, CM_ZOOM_BASE+ZOOM_FACTOR_COUNT-1, CM_ZOOM_BASE + pMapData->CanvasData.nZoom, MF_BYCOMMAND))
	{
		FatalError(GetLastError(), TEXT("CheckMenuRadioItem() failed"));
	}
}

static void	DoMenuZoom(HWND hWnd, MAP_CONTAINER *pMapData, int nCommand)
{
	int col, row;
	
	GetCenterPoint(pMapData, col, row);

	pMapData->CanvasData.nZoom = nCommand % ZOOM_FACTOR_COUNT;

	UpdateStatusBarZoomDisplay(pMapData);

	SetCenterPoint(pMapData, col, row);	// Will force screen redraw.

	if (!CheckMenuRadioItem (GetMenu(g_hMDIFrame), CM_ZOOM_BASE, CM_ZOOM_BASE+ZOOM_FACTOR_COUNT-1, CM_ZOOM_BASE + pMapData->CanvasData.nZoom, MF_BYCOMMAND))
	{
		FatalError(GetLastError(), TEXT("CheckMenuRadioItem() failed"));
	}
}

static void	DoMapUpdate(HWND hWnd, MAP_CONTAINER *pMapData, MAP_UPDATE_STRUCT *pMUS)
{
	HDC hDC;

// FIXME: Add tons of data validation.  Including make sure that this update is
// ment for this map type.  That is, an overwold update is not updating a cave or town.

	// Only need to do an update if the update is for the same map that we are 
	// handling in this window.
	if (pMapData->MapInfo.pMap == pMUS->pMapData->MapInfo.pMap)
	{
		// Draw it on screen.
		hDC = GetDC (hWnd) ;
		RenderMapTiles(hWnd, hDC, pMapData, &(pMUS->rUpdateRect));
		ReleaseDC (hWnd, hDC) ;
	}
}

static void	LabelLocations_Overworld(HWND hWnd, HDC hDC, MAP_CONTAINER *pMapData, RECT *rTiles)
{
	int i, x, y, row, col;
	TCHAR text[64];
	SIZE size;

	int w = Zoom(pMapData, pMapData->MapInfo.tile_height);
	int h = Zoom(pMapData, pMapData->MapInfo.tile_width);

	// Determine pixel height of the text.
	lstrcpy(text, TEXT("00"));
	GetTextExtentPoint32(hDC, text, lstrlen(text), &size);

// Label cave entrences.  Put label on cave, centered at bottom.
// Skip labels if zoomed in too far.
	if (Zoom(pMapData, 4) > 1)
	{
		for (i = 0; i < CAVE_COUNT; i++)
		{
			col = g_pRom[0xf849 + i];
			row = g_pRom[0xf899 + i];

			if ((row >= rTiles->top) && (row <= rTiles->bottom) && (col >= rTiles->left) && (col <= rTiles->right))
			{
				wnsprintf(text, TCHAR_sizeof(text), TEXT("%02d"), i);
				TileToScreen(pMapData, x, y, col, row);
				y += h - size.cy;
				x += (w - size.cx) / 2;
				TextOut(hDC, x, y, text, lstrlen(text));
			}
		}
	}

// Label towns.  Put label right below town.
	for (i = 0; i < TOWN_COUNT; i++)
	{
		col = g_pRom[0x989e + i];	// upper left corner of town.
		row = g_pRom[0x98a8 + i];

		if ((row >= rTiles->top) && (row <= rTiles->bottom) && (col >= rTiles->left) && (col <= rTiles->right))
		{
			wnsprintf(text, TCHAR_sizeof(text), TEXT("%s"), g_TownInfo[i].szName);
			TileToScreen(pMapData, x, y, col, row);
			y += h * 2;
			TextOut(hDC, x, y, text, lstrlen(text));
		}
	}
}

static void	LabelLocations_Underworld(HWND hWnd, HDC hDC, MAP_CONTAINER *pMapData, RECT *rTiles)
{
	int i, x, y, row, col;
	TCHAR text[64];
	SIZE size;

	int w = Zoom(pMapData, pMapData->MapInfo.tile_height);
	int h = Zoom(pMapData, pMapData->MapInfo.tile_width);

	// Determine pixel height of the text.
	lstrcpy(text, TEXT("00"));
	GetTextExtentPoint32(hDC, text, lstrlen(text), &size);

// Label cave entrences.
	for (i = 0; i < CAVE_COUNT; i++)
	{
		col = g_pRom[0x5902 + i];
		row = g_pRom[0x5952 + i];

		if ((row >= rTiles->top) && (row <= rTiles->bottom) && (col >= rTiles->left) && (col <= rTiles->right))
		{
			wnsprintf(text, TCHAR_sizeof(text), TEXT("%02d"), i);
			TileToScreen(pMapData, x, y, col, row);
			y += h - size.cy;
			x += (w - size.cx) / 2;
			TextOut(hDC, x, y, text, lstrlen(text));
		}
	}
}

// Renders the map tiles in the range 'rTiles', inclusive.
// Useful when erasing previous selection rectangle without triggering a full repaint.
static void	RenderMapTiles(HWND hWnd, HDC hDC, MAP_CONTAINER *pMapData, RECT *rTiles)
{
	RECT rect;
	HDC hdc2;
	HBITMAP hBmpOld;
	BITMAP bm;
	int x, y, row, col, w, h;
	int src_x, src_y;
	unsigned char tile;
	int bmp_tiles_per_row;
	int prev_bk_mode;

	GetClientRect (hWnd, &rect) ;
	GetObject(pMapData->MapInfo.hPaletteBitmap, sizeof(bm), &bm);
	bmp_tiles_per_row = bm.bmWidth / pMapData->MapInfo.tile_width;
   
	hdc2 = CreateCompatibleDC(hDC);
	hBmpOld = (HBITMAP)SelectObject(hdc2, pMapData->MapInfo.hPaletteBitmap);

	prev_bk_mode = SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, g_rgbMapLabelColor);

	w = Zoom(pMapData, pMapData->MapInfo.tile_width);
	h = Zoom(pMapData, pMapData->MapInfo.tile_height);

	for (row = rTiles->top; row <= rTiles->bottom; row++)
	{
		y = Zoom(pMapData, row * pMapData->MapInfo.tile_height) - pMapData->CanvasData.ptDisplayOffset.y;

		for (col = rTiles->left; col <= rTiles->right; col++)
		{
			x = Zoom(pMapData, col * pMapData->MapInfo.tile_width) - pMapData->CanvasData.ptDisplayOffset.x;

			if ((row >= 0) && (row < pMapData->MapInfo.map_height) && (col >= 0) && (col < pMapData->MapInfo.map_width))
			{
				tile = pMapData->MapInfo.pMap[row * pMapData->MapInfo.map_width + col] -
					pMapData->MapInfo.tile_offset;
			}
			else
			{
				tile = pMapData->MapInfo.ucOutOfBoundsTile - pMapData->MapInfo.tile_offset;
			}

			if (tile < pMapData->MapInfo.tile_count)
			{
				src_x = (tile % bmp_tiles_per_row) * pMapData->MapInfo.tile_width;
				src_y = (tile / bmp_tiles_per_row) * pMapData->MapInfo.tile_height;

				// BitBlt might be faster if the zoom is at 100%
				if (pMapData->CanvasData.nZoom)
				{
					StretchBlt(hDC, x, y, w, h, 
						hdc2, src_x, src_y, pMapData->MapInfo.tile_width,
						pMapData->MapInfo.tile_height, SRCCOPY);
				}
				else
				{
					BitBlt(hDC, x, y, pMapData->MapInfo.tile_width, pMapData->MapInfo.tile_height, 
						hdc2, src_x, src_y, SRCCOPY);
				}
			}
		}
	}

	// Must draw the labels AFTER all the tiles, b/c a lable might extend beyond the tile
	// that it labels, and it would be over-writen by the next tile if drawn immediately
	// after the tile in question.
	if (pMapData->CanvasData.bLabelLocations)
	{
		if (pMapData->MapInfo.map_type == MAP_OVERWORLD)
		{
			LabelLocations_Overworld(hWnd, hDC, pMapData, rTiles);
		}
		else if (pMapData->MapInfo.map_type == MAP_UNDERWORLD)
		{
			LabelLocations_Underworld(hWnd, hDC, pMapData, rTiles);
		}
	}

	SelectObject(hdc2, hBmpOld);
	DrawSelectionRectangle(hWnd, hDC, pMapData);
	DeleteDC(hdc2);
	SetBkMode(hDC, prev_bk_mode);
}

// Implements flicker free doubel buffering, from:
// http://blogs.msdn.com/oldnewthing/archive/2006/01/03/508694.aspx
static void	DoMapPaint(HWND hWnd, MAP_CONTAINER *pMapData)
{
	PAINTSTRUCT ps;
	RECT rTiles;
	RECT rWindow;
	HDC hDC;

	GetClientRect (hWnd, &rWindow);

	rTiles.top = pMapData->CanvasData.ptDisplayOffset.y / Zoom(pMapData, pMapData->MapInfo.tile_height) - 1;
	rTiles.bottom = 1 + rTiles.top + (rWindow.bottom + Zoom(pMapData, pMapData->MapInfo.tile_height) - 1) / Zoom(pMapData, pMapData->MapInfo.tile_height);
	rTiles.left = pMapData->CanvasData.ptDisplayOffset.x / Zoom(pMapData, pMapData->MapInfo.tile_width) - 1;
	rTiles.right = 1 + rTiles.left + (rWindow.right + Zoom(pMapData, pMapData->MapInfo.tile_width) - 1) / Zoom(pMapData, pMapData->MapInfo.tile_width);
	
	hDC = BeginPaint (hWnd, &ps);
	if (GetSystemMetrics(SM_REMOTESESSION)) 
	{
		RenderMapTiles(hWnd, hDC, pMapData, &rTiles);
	} 
	else if (!IsRectEmpty(&ps.rcPaint)) 
	{
		HDC hdc = CreateCompatibleDC(ps.hdc);
		if (hdc) 
		{
			int x = ps.rcPaint.left;
			int y = ps.rcPaint.top;
			int cx = ps.rcPaint.right - ps.rcPaint.left;
			int cy = ps.rcPaint.bottom - ps.rcPaint.top;
			HBITMAP hbm = CreateCompatibleBitmap(ps.hdc, cx, cy);
			if (hbm) 
			{
				HBITMAP hbmPrev = SelectBitmap(hdc, hbm);
				SetWindowOrgEx(hdc, x, y, NULL);

				RenderMapTiles(hWnd, hdc, pMapData, &rTiles);

				BitBlt(ps.hdc, x, y, cx, cy, hdc, x, y, SRCCOPY);

				SelectObject(hdc, hbmPrev);
				DeleteObject(hbm);
			}
			DeleteDC(hdc);
		}
	}
  

	EndPaint(hWnd, &ps);
}

static void	DoRecalcCompression(HWND hWnd, MAP_CONTAINER *pMapData)
{
	if (pMapData->CanvasData.bIdleDirty)
	{
		UpdateStatusBarCompression(pMapData);
		pMapData->CanvasData.bIdleDirty = false;
	}
}

static void	DoSetCursor(HWND hWnd, MAP_CONTAINER *pMapData)
{
	switch (pMapData->nCursorType)
	{
		case CM_TOOL_HAND_TOOL:
			SetCursor(pMapData->CanvasData.bDragging ? g_hCursorHandClosed : g_hCursorHandOpen);
			break;

		case CM_TOOL_SELECT_TOOL:
			SetCursor(g_hCursorSelect);
			break;

		case CM_TOOL_PEN_TOOL:
			SetCursor(g_hCursorPen);
			break;
	}
}

static void	DoScroll(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, int fnBar)
{
	SCROLLINFO sbi;

	_ASSERT((fnBar == SB_HORZ) || (fnBar == SB_VERT));

	memset(&sbi, 0, sizeof(sbi));
	sbi.cbSize = sizeof(sbi);
	sbi.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_TRACKPOS;
	
	GetScrollInfo(hWnd, fnBar, &sbi);

	long *pFinal = NULL;
	int nLineAdj = 0;
	int nExtra = 0;

	if (fnBar == SB_HORZ)
	{
		pFinal = &(pMapData->CanvasData.ptDisplayOffset.x);
		nLineAdj = pMapData->MapInfo.tile_width;
		nExtra = pMapData->MapInfo.tile_width * EXTRA_COL_TILES;
	}
	else
	{
		pFinal = &(pMapData->CanvasData.ptDisplayOffset.y);
		nLineAdj = pMapData->MapInfo.tile_height;
		nExtra = pMapData->MapInfo.tile_height * EXTRA_ROW_TILES;
	}

	switch (LOWORD(wParam))
	{
		case SB_TOP:
			sbi.nPos = sbi.nMin;
			break;

		case SB_BOTTOM:
			sbi.nPos = sbi.nMax;
			break;

        case SB_PAGEUP: 
			sbi.nPos -= sbi.nPage;
            break; 

        case SB_PAGEDOWN: 
			sbi.nPos += sbi.nPage;
            break; 

        case SB_LINEUP:
			sbi.nPos -= nLineAdj;
            break; 

        case SB_LINEDOWN: 
            sbi.nPos += nLineAdj;
            break; 

        case SB_THUMBPOSITION: 
		case SB_THUMBTRACK:
            sbi.nPos = HIWORD(wParam); 
            break; 
	}

	ClampScrollPos(&sbi);

    sbi.fMask  = SIF_POS; 
    SetScrollInfo(hWnd, fnBar, &sbi, true);

	*pFinal = sbi.nPos - nExtra;

	InvalidateRect(hWnd, NULL, false);
}

// Call this function after each screen resize.
static void	InitScrollBars(HWND hWnd, MAP_CONTAINER *pMapData)
{
	SCROLLINFO sbi;
	RECT rect;
	int virt_width = pMapData->MapInfo.map_width + EXTRA_COL_TILES * 2;
	int virt_height = pMapData->MapInfo.map_height + EXTRA_ROW_TILES * 2;

	GetClientRect(hWnd, &rect);

	// Set horizontal scroll bar.
	memset(&sbi, 0, sizeof(sbi));
	sbi.cbSize = sizeof(sbi);
	sbi.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;

	GetScrollInfo(hWnd, SB_HORZ, &sbi);
	sbi.nPage = Zoom(pMapData, pMapData->MapInfo.tile_width) * 4;
	sbi.nMin = 0;
	sbi.nMax = Zoom(pMapData, pMapData->MapInfo.tile_width) * virt_width - rect.right + sbi.nPage ;

	ClampScrollPos(&sbi);
	SetScrollInfo(hWnd, SB_HORZ, &sbi, true);

	// Set vertical scroll bar.
	memset(&sbi, 0, sizeof(sbi));
	sbi.cbSize = sizeof(sbi);
	sbi.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	GetScrollInfo(hWnd, SB_HORZ, &sbi);

	sbi.nPage = Zoom(pMapData, pMapData->MapInfo.tile_height) * 4;
	sbi.nMin = 0;
	sbi.nMax = Zoom(pMapData, pMapData->MapInfo.tile_height) * virt_height - rect.bottom + sbi.nPage;

	ClampScrollPos(&sbi);
	SetScrollInfo(hWnd, SB_VERT, &sbi, true);
}

// The default canvas x/y values are 0,0.  However, the scroll bars are at 0,0,
// but are biased by "EXTRA_XXX_TILES".  We must adjust either the canvas or its
// scroll bars to bring them into sync.  We only do this once, during WM_CREATE.
static void		AdjustCanvasCentering(HWND hWnd, MAP_CONTAINER *pMapData)
{
	pMapData->CanvasData.ptDisplayOffset.x = 0 - Zoom(pMapData, pMapData->MapInfo.tile_width) * EXTRA_COL_TILES;
	pMapData->CanvasData.ptDisplayOffset.y = 0 - Zoom(pMapData, pMapData->MapInfo.tile_height) * EXTRA_ROW_TILES;

/*
	// FIXME: This code seems to not have the desired effect.
	// Desired: If window if larger than needed, shrink initial window.
	// Observed: Size change (ie, 'SetWindowPos()') is ignored, but returns "1" in EAX register.

	RECT r;
	int virt_width = pMapData->MapInfo.tile_width * (pMapData->MapInfo.map_width + EXTRA_COL_TILES * 2);
	int virt_height = pMapData->MapInfo.tile_height * (pMapData->MapInfo.map_height + EXTRA_ROW_TILES * 2);
	bool bNeedsAdjustment = false;

	GetClientRect(hWnd, &r);
	
	if (r.right > virt_width)
	{
		r.right = virt_width;
		bNeedsAdjustment = true;
	}

	if (r.bottom > virt_height)
	{
		r.bottom = virt_height;
		bNeedsAdjustment = true;
	}
	
	if (bNeedsAdjustment)
	{
		SetWindowPos(hWnd, NULL, 0, 0, r.right, r.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
*/
}

// With rare exception, all messages should call "DefMDIChildProc()" instead of 
// just doing a "return 0" or "return 1".
long FAR PASCAL MapEditorCanvas_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MAP_CONTAINER		*pMapData = (MAP_CONTAINER*)GetWindowLongPtr (hWnd, OFFSET_MAP_CONTAINER);

	switch (message)
	{
		case WM_CREATE:
			pMapData = (MAP_CONTAINER*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
			SetWindowLongPtr(hWnd, OFFSET_MAP_CONTAINER, (LONG_PTR)pMapData);
			InitScrollBars(hWnd, pMapData);
			AdjustCanvasCentering(hWnd, pMapData);
			UpdateStatusBarZoomDisplay(pMapData);
			break;
			
		case WM_DESTROY:
			SetWindowLongPtr(hWnd, OFFSET_MAP_CONTAINER, (LONG_PTR)NULL);
			break;

		case WM_SIZE:
			InitScrollBars(hWnd, pMapData);
			InvalidateRect(hWnd, NULL, false);
			break;
			
		case WM_PAINT:
			DoMapPaint(hWnd, pMapData);
			return 0;

		case WM_HSCROLL:
			DoScroll(hWnd, pMapData, wParam, SB_HORZ);
			return 0;

		case WM_VSCROLL:
			DoScroll(hWnd, pMapData, wParam, SB_VERT);
			return 0;

		case WM_MOUSEWHEEL:
			DoMouseWheel(hWnd, pMapData, wParam, lParam);
			return 0;

		case WM_MOUSEMOVE:
			DoMouseMove(hWnd, pMapData, wParam, lParam);
			break;

		case WM_RBUTTONDOWN:
			DoRightButtonDown(hWnd, pMapData, wParam, lParam);
			break;

		case WM_LBUTTONDOWN:
			DoLeftButtonDown(hWnd, pMapData, wParam, lParam);
			break;

		case WM_LBUTTONUP:
			DoLeftButtonUp(hWnd, pMapData, wParam, lParam);
			break;

		case WM_USER_MAP_EDITOR_UPDATE_TILE:
			DoMapUpdate(hWnd, pMapData, (MAP_UPDATE_STRUCT*)lParam);
			return 0;

		case WM_RECALC_COMPRESSION:
			DoRecalcCompression(hWnd, pMapData);
			break;

		case WM_SETCURSOR:
			DoSetCursor(hWnd, pMapData);
			return 1;	// Indicate that we handled this.

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case CM_FILL:
					DoFillTiles(hWnd, pMapData);
					return 0;

				case CM_COPY:
					CopyTilesToClipboard(hWnd, pMapData, &(pMapData->CanvasData.rSelection));
					return 0;

				case CM_PASTE:
					PasteTilesFromClipboard(hWnd, pMapData, &(pMapData->CanvasData.rSelection), false);
					return 0;

				case CM_PASTE_PATTERN:
					PasteTilesFromClipboard(hWnd, pMapData, &(pMapData->CanvasData.rSelection), true);
					return 0;

				// These are handled by the container.  They can come from a tracked-popup-menu.
				case CM_TOOL_HAND_TOOL:
				case CM_TOOL_SELECT_TOOL:
				case CM_TOOL_PEN_TOOL:
					SendMessage(pMapData->hWndContainer, message, wParam, lParam);
					return 0;

				case CM_SAMPLE_TILE:
					DoSampleTile(hWnd, pMapData, wParam, lParam);
					return 0;

				default:
					if ((LOWORD(wParam) >= CM_ZOOM_BASE) && (LOWORD(wParam) <= (CM_ZOOM_BASE + ZOOM_FACTOR_COUNT)))
					{
						DoMenuZoom(hWnd, pMapData, LOWORD(wParam) - CM_ZOOM_BASE);
						return 0;
					}
			}
			break;

	}

	return DefMDIChildProc (hWnd, message, wParam, lParam) ;
}

struct	ZOOM_FACTOR_TEMP
{
	int zf;
	int ord;
};

void	AddZoomCommandsToMenu(HMENU hMenu)
{
	TCHAR	szText[128];
	int i, j;
	ZOOM_FACTOR_TEMP t;
	ZOOM_FACTOR_TEMP zf[ZOOM_FACTOR_COUNT];

	for (i = 0; i < ZOOM_FACTOR_COUNT; i++)
	{
		zf[i].zf = (zoom_factors[i].nNumerator * 100) / zoom_factors[i].nDenominator;
		zf[i].ord = i;
	}

	// Don't want to use the STDLIB "qsort" to avoid linking in the C/C++ runtime.
	// Yeah for bubble sort!

	for (i = 0; i < ZOOM_FACTOR_COUNT; i++)
	{
		for (j = 0; j < i; j++)
		{
			if (zf[i].zf > zf[j].zf)
			{
				t = zf[i];
				zf[i] = zf[j];
				zf[j] = t;
			}
		}
	}

	for (i = 0; i < ZOOM_FACTOR_COUNT; i++)
	{
		wnsprintf(szText, TCHAR_sizeof(szText), TEXT("%d %%"), zf[i].zf);

		if (!AppendMenu(hMenu, MF_STRING, CM_ZOOM_BASE + zf[i].ord, szText))
		{
			FatalError(GetLastError(), TEXT("AppendMenu() failed."));
		}
	}

}
