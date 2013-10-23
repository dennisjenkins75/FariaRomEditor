/* MapEditorTools.cpp */

#include "FariaRomEditor.h"

static void	DoToolsPaint(HWND hWnd, MAP_CONTAINER *pMapData)
{
	PAINTSTRUCT ps;
	RECT rect;
	HDC hDC;
	HDC hdc2;
	HBITMAP hBmpOld;
	BITMAP bm;
	int src_x, src_y, x, y;
	unsigned char tile;
	int nTile;
	int bmp_tiles_per_row;

	PALETTE_DATA *pPalette = &(pMapData->PaletteData);
	CANVAS_DATA *pCanvas = &(pMapData->CanvasData);
	MAP_INFO *pInfo = &(pMapData->MapInfo);

	memset(&bm, 0, sizeof(bm));
	GetObject(pInfo->hPaletteBitmap, sizeof(bm), &bm);
	bmp_tiles_per_row = bm.bmWidth / pMapData->MapInfo.tile_width;

	hDC = BeginPaint (hWnd, &ps);

	GetClientRect (hWnd, &rect);
	x = (rect.right - pPalette->render_width) / 2;
	y = (rect.bottom - pPalette->render_height) / 2;
   
	hdc2 = CreateCompatibleDC(hDC);
	hBmpOld = (HBITMAP)SelectObject(hdc2, pInfo->hPaletteBitmap);

	nTile = pPalette->current_tile;

	if ((nTile >= pMapData->MapInfo.tile_offset) && (nTile < pMapData->MapInfo.tile_count + pMapData->MapInfo.tile_offset)) 
	{
		tile = (BYTE)nTile - pMapData->MapInfo.tile_offset;
	}
	else
	{
		tile = pInfo->ucOutOfBoundsTile - pMapData->MapInfo.tile_offset;
	}

//	tile = ((nTile >= 0) && (nTile < pInfo->tile_count)) ? (BYTE)nTile: pInfo->ucOutOfBoundsTile;

	// Need to convert "tile" (index into palette) into coords in the palette bitmap.
	src_x = (tile % bmp_tiles_per_row) * pMapData->MapInfo.tile_width;
	src_y = (tile / bmp_tiles_per_row) * pMapData->MapInfo.tile_height;

	StretchBlt(hDC, x, y, pPalette->render_width, pPalette->render_height,
		hdc2, src_x, src_y, pInfo->tile_width, pInfo->tile_height, SRCCOPY);

	SelectObject(hdc2, hBmpOld);
	DeleteDC(hdc2);
	EndPaint(hWnd, &ps);
}


// With rare exception, all messages should call "DefMDIChildProc()" instead of 
// just doing a "return 0" or "return 1".
long FAR PASCAL MapEditorTools_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MAP_CONTAINER		*pData = (MAP_CONTAINER*)GetWindowLongPtr (hWnd, 0) ;

	switch (message)
	{
		case WM_CREATE:
			pData = (MAP_CONTAINER*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)pData);
//			InitScrollBars(hWnd, pData);
			break;
			
		case WM_DESTROY:
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)NULL);
			break;

		case WM_SIZE:
//			InitScrollBars(hWnd, pData);
			InvalidateRect(hWnd, NULL, false);
			break;

		case WM_PAINT:
			DoToolsPaint(hWnd, pData);
			return 0;
/*
		case WM_HSCROLL:
			DoScroll(hWnd, pData, wParam, SB_HORZ);
			return 0;

		case WM_VSCROLL:
			DoScroll(hWnd, pData, wParam, SB_VERT);
			return 0;

		case WM_MOUSEMOVE:
			DoMouseMove(hWnd, pData, lParam, wParam);
			break;
*/
	}

	return DefMDIChildProc (hWnd, message, wParam, lParam) ;
}
