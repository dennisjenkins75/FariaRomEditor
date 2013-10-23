/* MapEditor.h */

#ifndef __FARIA_EDITOR_MAP_EDITOR_H__
#define __FARIA_EDITOR_MAP_EDITOR_H__

struct	MAP_CONTAINER;

enum	MAP_TYPE
{
	MAP_OVERWORLD,
	MAP_SKYWORLD,
	MAP_UNDERWORLD,
	MAP_TOWN
};

// This structure is passed to the map editor via the WM_CREATE message
// (as a pointer in lParam).  It is used to tell the map editor the details
// of the map being edited.  A deep copy of this struct is made, so the caller
// call allocate it on their stack.
struct	MAP_INFO
{
	MAP_TYPE	map_type;
	TCHAR		*szTitle;

	int		map_width;		// Width of map in tiles.
	int		map_height;		// Height of map in tiles.

	BYTE	*pMap;				// DO NOT MANAGE.  Should point to a global, static buffer

	BYTE	ucOutOfBoundsTile;	// Tile # to draw when we are out of bounds.

	HBITMAP	hPaletteBitmap;	// Handle of bitmap containing the tiles in l->r, t->b order.

	int		tile_width;		// In pixels, of the raw tiles in the palette bitmap.
	int		tile_height;
	int		tile_count;		// # of tiles in the palette.
	int		tile_offset;	// # of tiles to "skip" before dereferencing the palette (used for King's Castle).
};


struct	PALETTE_DATA
{
	int		x;					// Virtual pixel of tile palette on left edge of window.
	int		render_width;		// # pixels wide each tile is, when drawn on screen.
	int		render_height;		// # pixels each tile is tall.
	int		tile_spacing;		// # of pixels between each tile.
	int		prev_highlight;		// Tile index.
	BYTE	current_tile;		// Currently active tile.
};

struct	CANVAS_DATA
{
// "Virtual pixel offsets" may be negative, and they may extend beyond bounds of
// the map.  In these cases, they should be drawn as tile 0x00 (water).
	POINT	ptDisplayOffset;	// pixel coords of upper left corner of displayed bitmap.

	RECT	prev_highlight;		// TILES, not pixels, of last "HighLight"

	RECT	rSelection;			// Tiles, not pixels.  Not normalized.  top/left = "down", right/bottom = "up"/"cur"

	POINT	ptMouseDown;	//	Coord where user began dragging canvas with the "hand cursor" or "selection tool".
	POINT	ptMouseUp;		//	Coord where mouse was let up.
	POINT	ptRightClick;	//	Coord where right click was last done.

	bool	bDragging;	// 'true' if the hand tool is dragging the canvas around.
	bool	bSelecting;	// 'true' if the selection tool is selecting tiles.
	bool	bSelectionValid;
	bool	bLabelLocations;
	bool	bIdleDirty;		// 'true' if map was edited and needs compression recalculated.

	int		nZoom;				// Index into a table of zoom values.
	int		nMouseWheelDelta;	// Current wheel delta accumulator, used to set "nZoom".
};

// How many tiles we are allowed to scroll beyond the ends of the map.
static const int EXTRA_COL_TILES = 3;
static const int EXTRA_ROW_TILES = 3;

// Held in second "GetWindowLongPtr()"
struct	MAP_CONTAINER
{
	HWND	hWndContainer;
	HWND	hWndPalette;
	HWND	hWndTools;
	HWND	hWndCanvas;

	int		nCursorType;

	CANVAS_DATA		CanvasData;
	PALETTE_DATA	PaletteData;
	MAP_INFO		MapInfo;
};

// The map container window class has extra bytes for storing the pointers
// to various structs.  We will define the byte offsets for these structs
// here.
static const int OFFSET_MAP_CONTAINER = 0 * sizeof(void*);
static const int MAP_CONTAINER_EXTRA_BYTES = 1 * sizeof(void*);

// Send via lParam of "WM_USER_MAP_EDITOR_UPDATE_TILE" 
struct	MAP_UPDATE_STRUCT
{
	RECT		rUpdateRect;
	MAP_CONTAINER	*pMapData;
};

struct	MAP_CLIPBOARD_FORMAT
{
	RECT		rOrigin;
	MAP_TYPE	map_type;
	BYTE		ucTiles[1];		// variable length
};

// Status Bar columns.
static const int SB_COLUMN_COUNT = 5;
static const int SB_COL_TITLE = 0;
static const int SB_COL_COORD = 1;
static const int SB_COL_COMPRESSION = 2;
static const int SB_COL_PALETTE_TILE = 3;
static const int SB_COL_ZOOM = 4;

struct	ZOOM_FACTORS
{
	int nNumerator;
	int nDenominator;
};

static const int ZOOM_FACTOR_COUNT = 8;
extern ZOOM_FACTORS zoom_factors[ZOOM_FACTOR_COUNT];

// Message sent to "Palette" window to set current tile.
// Tile # is sent in wParam.
static const int WM_USER_PALETTE_SET_CUR_TILE = WM_USER + 0x400;

// Send to Frame window when any map editor updates a tile.
// Frame window should send same message to all MapCanvas child windows.
// wParam is empty, as we can only pass lParam into EnumChildWindows().
// lParam contains pointer to MAP_UPDATE_STRUCT
static const int WM_USER_MAP_EDITOR_UPDATE_TILE = WM_USER + 0x401;


// Posted to the MapCanvas window when the map inside has been modified and 
// needs to recompute compression.
static const int WM_RECALC_COMPRESSION = WM_USER + 0x402;


long FAR PASCAL MapEditorCanvas_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
long FAR PASCAL MapEditorPalette_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
long FAR PASCAL MapEditorTools_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void UpdateStatusBarCompression(MAP_CONTAINER *pData);
void AddZoomCommandsToMenu(HMENU hMenu);

#endif // __FARIA_EDITOR_MAP_EDITOR_H__
