/* Globals.h */

#ifndef __FARIA_EDITOR_GLOBALS_H__
#define __FARIA_EDITOR_GLOBALS_H__

static const int ROM_SIZE = (256 * 1024);
static const int INES_SIZE = (ROM_SIZE + 16);

static const int OW_MAP_WIDTH = 128;
static const int OW_MAP_HEIGHT = 128;

static const int SKY_MAP_WIDTH = 64;
static const int SKY_MAP_HEIGHT = 64;

static const int CAVE_MAP_WIDTH = 80;
static const int CAVE_MAP_HEIGHT = 80;

static const int MAX_TOWN_WIDTH = 32;
static const int MAX_TOWN_HEIGHT = 32;

extern unsigned char g_iNES[INES_SIZE];
extern unsigned char *g_pRom;
extern bool			g_bRomDirty;
extern TCHAR		g_szWorkingDir[_MAX_PATH];
extern TCHAR		g_szRomFile[_MAX_PATH];
extern BYTE			g_OverworldMap[OW_MAP_WIDTH * OW_MAP_HEIGHT];
extern BYTE			g_SkyworldMap[SKY_MAP_WIDTH * SKY_MAP_HEIGHT];
extern BYTE			g_CaveworldMap[CAVE_MAP_WIDTH * CAVE_MAP_HEIGHT];
extern BYTE			g_TownMaps[TOWN_COUNT+1][MAX_TOWN_WIDTH * MAX_TOWN_HEIGHT];

extern SIZE			g_TownSizes[TOWN_COUNT+1];

extern HINSTANCE	g_hInstance;
extern HFONT		g_hFontMono;
extern HICON		g_hIconGaoGao;
extern HICON		g_hIconLand;
extern HICON		g_hIconTower;
extern HWND			g_hMDIFrame;
extern HWND			g_hMDIClient;
extern HWND			g_hWndStatusBar;
extern HANDLE		g_hProcessHeap;
extern HBITMAP		g_hOverworldMapTiles;
extern HBITMAP		g_hSkyworldMapTiles;
extern HBITMAP		g_hCaveworldMapTiles;
extern HBITMAP		g_hTownMapTiles;
extern HBITMAP		g_hCastleMapTiles;
extern HBITMAP		g_hGaoGaoMonster;
extern HMENU		g_hMenuFrame;
extern HMENU		g_hMenuMapEditor;
extern HMENU		g_hMenuMdiStub;
extern HMENU		g_hMenuFrame_Window;
extern HMENU		g_hMenuMapEditor_Window;
extern HMENU		g_hMenuMdiStub_Window;
extern HKEY			g_hRegKeyRoot;
extern HCURSOR		g_hCursorHandOpen;
extern HCURSOR		g_hCursorHandClosed;
extern HCURSOR		g_hCursorHandPointing;
extern HCURSOR		g_hCursorPen;
extern HCURSOR		g_hCursorSelect;
extern UINT			g_uMapClipboardFormat;

extern const TCHAR szClassFrame[];
extern const TCHAR szClassMapEditorCanvas[];
extern const TCHAR szClassMapEditorContainer[];
extern const TCHAR szClassMapEditorPalette[];
extern const TCHAR szClassMapEditorTools[];
extern const TCHAR szClassMdiStub[];
extern const TCHAR szMapClipboardFormat[];

extern COLORREF	g_rgbHighlightColor;
extern COLORREF	g_rgbOutOfBoundsColor;
extern COLORREF g_rgbMapLabelColor;

#endif // __FARIA_EDITOR_GLOBALS_H__
