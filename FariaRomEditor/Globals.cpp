/* Globals.cpp */

#include "FariaRomEditor.h"

// "g_iNES" holds entire ROM file + 16 byte INES header.
unsigned char g_iNES[INES_SIZE];

// "g_pROM" points 16 bytes into g_iNES, is used for convienience.
unsigned char *g_pRom = NULL;

// "g_bRomDirty" is true if "g_iNES" has been modified and not saved.
bool g_bRomDirty = false;

// "g_szWorkingDir" holds path of directory for ROM and SAV files.
TCHAR g_szWorkingDir[_MAX_PATH];

// g_szRomFile" holds full path of the ROM file loaded into "g_iNES"
TCHAR g_szRomFile[_MAX_PATH];

// "g_OverworldMap" holds the decompressed overworld map.  This data structure
// is manipulated by the MapEditorCanvas.cpp file.
BYTE g_OverworldMap[OW_MAP_WIDTH * OW_MAP_HEIGHT];

// "g_SkyworldMap" holds the decompressed sky-world map.
BYTE g_SkyworldMap[SKY_MAP_WIDTH * SKY_MAP_HEIGHT];

BYTE g_CaveworldMap[CAVE_MAP_WIDTH * CAVE_MAP_HEIGHT];

BYTE g_TownMaps[TOWN_COUNT+1][MAX_TOWN_WIDTH * MAX_TOWN_HEIGHT];

SIZE g_TownSizes[TOWN_COUNT+1];


HINSTANCE	g_hInstance = NULL;
HFONT		g_hFontMono = NULL;
HICON		g_hIconGaoGao = NULL;
HICON		g_hIconLand = NULL;
HICON		g_hIconTower = NULL;
HWND		g_hMDIFrame = NULL;
HWND		g_hMDIClient = NULL;
HWND		g_hWndStatusBar = NULL;
HANDLE		g_hProcessHeap = NULL;
HBITMAP		g_hOverworldMapTiles = NULL;
HBITMAP		g_hSkyworldMapTiles = NULL;
HBITMAP		g_hCaveworldMapTiles = NULL;
HBITMAP		g_hTownMapTiles = NULL;
HBITMAP		g_hCastleMapTiles = NULL;
HBITMAP		g_hGaoGaoMonster = NULL;
HMENU		g_hMenuFrame = NULL;
HMENU		g_hMenuMapEditor = NULL;
HMENU		g_hMenuMdiStub = NULL;
HMENU		g_hMenuFrame_Window = NULL;
HMENU		g_hMenuMapEditor_Window = NULL;
HMENU		g_hMenuMdiStub_Window = NULL;
HKEY		g_hRegKeyRoot = NULL;
HCURSOR		g_hCursorHandOpen = NULL;
HCURSOR		g_hCursorHandClosed = NULL;
HCURSOR		g_hCursorHandPointing = NULL;
HCURSOR		g_hCursorPen = NULL;
HCURSOR		g_hCursorSelect = NULL;
UINT		g_uMapClipboardFormat = 0;



const TCHAR szClassFrame[] = TEXT("FARIAEDITOR");
const TCHAR szClassMapEditorCanvas[] = TEXT("FariaMapEditorCanvas");
const TCHAR szClassMapEditorContainer[] = TEXT("FariaMapEditorContainer");
const TCHAR szClassMapEditorPalette[] = TEXT("FariaMapEditorPalette");
const TCHAR szClassMapEditorTools[] = TEXT("FariaMapEditorTools");
const TCHAR szClassMdiStub[] = TEXT("FARIAMDISTUB");
const TCHAR szMapClipboardFormat[] = TEXT("FariaMapTiles");


COLORREF	g_rgbHighlightColor = RGB(255,0,0);		// red
COLORREF	g_rgbOutOfBoundsColor = RGB(128,0,128);	// dark purple
COLORREF	g_rgbMapLabelColor = RGB(255, 128, 0);	// medium orange
