/* MapEditorTools.cpp */

#include "FariaRomEditor.h"

static void	RenderPaletteTiles(HWND hWnd, HDC hDC, MAP_CONTAINER *pMapData, int nFirstTile, int nLastTile);

static inline int ScreenToTileIndex(MAP_CONTAINER *pMapData, int x)
{
	int temp = pMapData->PaletteData.render_width + pMapData->PaletteData.tile_spacing;
	return (x - 1 + pMapData->PaletteData.x) / temp + pMapData->MapInfo.tile_offset;
}

static inline int TileIndexToScreen(MAP_CONTAINER *pMapData, int tile)
{
	int temp = pMapData->PaletteData.render_width + pMapData->PaletteData.tile_spacing;
	return (tile - pMapData->MapInfo.tile_offset) * temp - pMapData->PaletteData.x + 1;
}

// Assumes that "tiles" > 0.
static inline int GetDisplayWidth(MAP_CONTAINER *pMapData, int tiles)
{
	return pMapData->PaletteData.render_width * tiles + pMapData->PaletteData.tile_spacing * (tiles-1);
}

// rTiles should contain tile indicies, not pixels.
static void	HighlightTile(HWND hWnd, MAP_CONTAINER *pMapData, int tile, COLORREF rgb)
{
	RECT rScreen;
	RECT rWindow;
	HDC hDC;
	HBRUSH hBrush;

	GetClientRect (hWnd, &rWindow);

	// Compute bounding rectangle that includes all tiles in 'rTiles'.
	rScreen.left = TileIndexToScreen(pMapData, tile);
	rScreen.right = rScreen.left + pMapData->PaletteData.render_width;
	rScreen.top = (rWindow.bottom - pMapData->PaletteData.render_height) / 2;
	rScreen.bottom = rScreen.top + pMapData->PaletteData.render_height;

	// Want to highlight aorund the tile, not inside it.
	rScreen.left--;
	rScreen.top--;
	rScreen.bottom++;
	rScreen.right++;

	// Draw it on screen.
	hDC = GetDC (hWnd) ;

	// Restore previous palette tile(s).
	RenderPaletteTiles(hWnd, hDC, pMapData, pMapData->PaletteData.prev_highlight, pMapData->PaletteData.prev_highlight);

	hBrush = CreateSolidBrush (rgb);
	SelectObject (hDC, hBrush);

	if (tile != -1)
	{
		//Rectangle (hdc, rScreen.left, rScreen.top, rScreen.right, rScreen.bottom);
		FrameRect(hDC, &rScreen, hBrush);
	}

	ReleaseDC (hWnd, hDC);
	DeleteObject (hBrush);

	pMapData->PaletteData.prev_highlight = tile;
}

static void	DoSetCurrentTile(HWND hWnd, MAP_CONTAINER *pMapData, BYTE tile)
{
// Scroll the scrollbar and 'x' offset such that th tile is in view.
// FIXME: If tile is not on right or left edge, scroll view such
// that the selected tile is centered in the palette display.
// FIXME : If scrolling to "x_pos" would make non-existant tiles appear on the right side, 
// throttle "x_pos" back into range.

	int x_pos = TileIndexToScreen(pMapData, tile) + pMapData->PaletteData.x;

	SetScrollPos(hWnd, SB_HORZ, x_pos, true);
	pMapData->PaletteData.x = x_pos;
	InvalidateRect(hWnd, NULL, false);
	UpdateWindow(hWnd);

	HighlightTile(hWnd, pMapData, tile, g_rgbHighlightColor);

// Update the tool window to reflect the currently selected tile.
	pMapData->PaletteData.current_tile = tile;
	InvalidateRect(pMapData->hWndTools, NULL, false);
}

static void	DoLeftClick(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	int mouse_x = LOWORD(lParam);
	int mouse_y = HIWORD(lParam);
	int nTile = ScreenToTileIndex(pMapData, mouse_x);

	if ((nTile < pMapData->MapInfo.tile_offset) || (nTile >= pMapData->MapInfo.tile_offset + pMapData->MapInfo.tile_count))
	{
		return;
	}

	pMapData->PaletteData.current_tile = (BYTE)nTile;

	InvalidateRect(pMapData->hWndTools, NULL, false);
}


static void	DoMouseMove(HWND hWnd, MAP_CONTAINER *pMapData, WPARAM wParam, LPARAM lParam)
{
	int mouse_x = LOWORD(lParam);
	int mouse_y = HIWORD(lParam);
	int col = 0;
	int row = 0;
	int nTile = 0;
	TCHAR szTemp[128];

	nTile = ScreenToTileIndex(pMapData, mouse_x);

	if ((nTile >= pMapData->MapInfo.tile_offset) && (nTile < pMapData->MapInfo.tile_offset + pMapData->MapInfo.tile_count))
	{
		wnsprintf(szTemp, TCHAR_sizeof(szTemp), TEXT("Tile: $%02x"), nTile);
		HighlightTile(hWnd, pMapData, nTile, g_rgbHighlightColor);
	}
	else
	{
		wnsprintf(szTemp, TCHAR_sizeof(szTemp), TEXT(""));
		HighlightTile(hWnd, pMapData, -1, g_rgbOutOfBoundsColor);
	}

	SendMessage (g_hWndStatusBar, SB_SETTEXT, SB_COL_PALETTE_TILE, (LPARAM)szTemp);
}

// Renders the map tiles in the range 'rTiles', inclusive.
// Useful when erasing previous selection rectangle without triggering a full repaint.
static void	RenderPaletteTiles(HWND hWnd, HDC hDC, MAP_CONTAINER *pMapData, int nFirstTile, int nLastTile)
{
	RECT	rect;
	RECT	rBorder;
	RECT	rTemp;
	HDC		hdc2;
	HBITMAP hBmpOld;
	HBRUSH	hBrush;
	HBRUSH	hBrushOld;
	HPEN	hPen;
	HPEN	hPenOld;
	BITMAP	bm;
	int		src_x, src_y, x, y;
	int		nTile;
	int		bmp_tiles_per_row;

	PALETTE_DATA *pPalette = &(pMapData->PaletteData);
	CANVAS_DATA *pCanvas = &(pMapData->CanvasData);

	GetClientRect (hWnd, &rect);
	y = (rect.bottom - pPalette->render_height) / 2;

	GetObject(pMapData->MapInfo.hPaletteBitmap, sizeof(bm), &bm);
	bmp_tiles_per_row = bm.bmWidth / pMapData->MapInfo.tile_width;
   
	hdc2 = CreateCompatibleDC(hDC);
	hBmpOld = (HBITMAP)SelectObject(hdc2, pMapData->MapInfo.hPaletteBitmap);

	hBrush = CreateSolidBrush(RGB(128,128,128));
	hBrushOld = (HBRUSH)SelectObject(hDC, hBrush); 

	hPen = CreatePen(PS_SOLID, 0, RGB(128,128,128));
	hPenOld = (HPEN)SelectObject(hDC, hPen);

	for (nTile = nFirstTile; nTile <= nLastTile; nTile++)
	{
		x = TileIndexToScreen(pMapData, nTile);

		if ((nTile >= pMapData->MapInfo.tile_offset) && (nTile < pMapData->MapInfo.tile_count + pMapData->MapInfo.tile_offset)) 
		{
			src_x = ((nTile - pMapData->MapInfo.tile_offset) % bmp_tiles_per_row) * pMapData->MapInfo.tile_width;
			src_y = ((nTile - pMapData->MapInfo.tile_offset) / bmp_tiles_per_row) * pMapData->MapInfo.tile_height;

			StretchBlt(hDC, x, y, pPalette->render_width, pPalette->render_height,
				hdc2, src_x, src_y, pMapData->MapInfo.tile_width, pMapData->MapInfo.tile_height, SRCCOPY);
		}
		else
		{
			rTemp.left = x;
			rTemp.right = x + pPalette->render_width;
			rTemp.top = y;
			rTemp.bottom = y + pPalette->render_height;

			FillRect(hDC, &rTemp, hBrush);
		}
		// Fill in space formerly occupied by the border.
		// LEFT, RIGHT, TOP, BOTTOM
		rBorder.top = 0;
		rBorder.left = x - pPalette->tile_spacing;
		rBorder.bottom = rect.bottom;
		rBorder.right = TileIndexToScreen(pMapData, nTile + 1);

		Rectangle(hDC, rBorder.left, rBorder.top, x, rBorder.bottom);	// left side
		Rectangle(hDC, x + pPalette->render_width, rBorder.top, rBorder.right, rBorder.bottom); // right
		Rectangle(hDC, rBorder.left, rBorder.top, rBorder.right, y);	// top
		Rectangle(hDC, rBorder.left, y + pPalette->render_height, rBorder.right, rBorder.bottom);	// bottom
	}

	SelectObject(hDC, hPenOld);
	SelectObject(hDC, hBrushOld);
	SelectObject(hdc2, hBmpOld);
	DeleteDC(hdc2);
}

static void	DoPalettePaint(HWND hWnd, MAP_CONTAINER *pMapData)
{
	PAINTSTRUCT ps;
	RECT rect;
	HDC hDC;
	int nFirstTile = 0;
	int nLastTile = 0-1;

	GetClientRect(hWnd, &rect);

	nFirstTile = ScreenToTileIndex(pMapData, 0) - 1;
	nLastTile = ScreenToTileIndex(pMapData, rect.right) + 1;

	hDC = BeginPaint (hWnd, &ps);
	RenderPaletteTiles(hWnd, hDC, pMapData, nFirstTile, nLastTile);
	EndPaint(hWnd, &ps);
}

static void	DoScroll(HWND hWnd, PALETTE_DATA *pPalette, WPARAM wParam)
{
	SCROLLINFO sbi;

	memset(&sbi, 0, sizeof(sbi));
	sbi.cbSize = sizeof(sbi);
	sbi.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_TRACKPOS;
	
	GetScrollInfo(hWnd, SB_HORZ, &sbi);

	int nLineAdj = pPalette->render_width + pPalette->tile_spacing;

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
    SetScrollInfo(hWnd, SB_HORZ, &sbi, true);

	pPalette->x = sbi.nPos;

	InvalidateRect(hWnd, NULL, false);
}

// Call this function after each screen resize.
static void	InitScrollBars(HWND hWnd, MAP_CONTAINER *pMapData)
{
	SCROLLINFO sbi;
	RECT rect;

	GetClientRect(hWnd, &rect);

	// Set horizontal scroll bar.
	memset(&sbi, 0, sizeof(sbi));
	sbi.cbSize = sizeof(sbi);
	sbi.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;

	GetScrollInfo(hWnd, SB_HORZ, &sbi);
	sbi.nPage = GetDisplayWidth(pMapData, 8);
	sbi.nMin = 0;
	sbi.nMax = GetDisplayWidth(pMapData, pMapData->MapInfo.tile_count) - rect.right + sbi.nPage;

	ClampScrollPos(&sbi);
	SetScrollInfo(hWnd, SB_HORZ, &sbi, true);
}

// With rare exception, all messages should call "DefMDIChildProc()" instead of 
// just doing a "return 0" or "return 1".
long FAR PASCAL MapEditorPalette_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MAP_CONTAINER		*pMapData = (MAP_CONTAINER*)GetWindowLongPtr (hWnd, 0) ;

	switch (message)
	{
		case WM_CREATE:
			pMapData = (MAP_CONTAINER*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)pMapData);
			InitScrollBars(hWnd, pMapData);
			break;
			
		case WM_DESTROY:
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)NULL);
			break;

		case WM_SIZE:
			InitScrollBars(hWnd, pMapData);
			InvalidateRect(hWnd, NULL, false);
			break;
			
		case WM_PAINT:
			DoPalettePaint(hWnd, pMapData);
			return 0;

		case WM_HSCROLL:
			DoScroll(hWnd, &(pMapData->PaletteData), wParam);
			return 0;

		case WM_MOUSEMOVE:
			DoMouseMove(hWnd, pMapData, wParam, lParam);
			break;

		case WM_LBUTTONUP:
			DoLeftClick(hWnd, pMapData, wParam, lParam);
			break;

		case WM_USER_PALETTE_SET_CUR_TILE:
			DoSetCurrentTile(hWnd, pMapData, (BYTE)wParam);
			break;
	}

	return DefMDIChildProc (hWnd, message, wParam, lParam) ;
}

