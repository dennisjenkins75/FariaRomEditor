// Samples of how to work with MDI applications.

// http://www.functionx.com/win32/howto/mdi.htm
// http://72.14.203.104/search?q=cache:SNeRWP-oUF8J:www.readol.net/books/computer/windows/Programming%2520Windows/ch19c.htm+TranslateMDISysAccel+CLIENTCREATESTRUCT&hl=en


#include "FariaRomEditor.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "version.lib")

void	FatalError(DWORD dwWin32ErrorCode, const TCHAR *fmt, ...)
{
	TCHAR temp[8192];
	va_list va;
	LPVOID lpMsgBuf = NULL;
	DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS; 
	TCHAR *sep = TEXT("\n\n");

	FormatMessage(dwFlags, NULL, dwWin32ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);

	va_start(va, fmt);
	wvnsprintf(temp, TCHAR_sizeof(temp), fmt, va);
	va_end(va);

	if (lpMsgBuf && (lstrlen(temp) + lstrlen((TCHAR*)lpMsgBuf) + lstrlen(sep) < TCHAR_sizeof(temp)))
	{
		lstrcat(temp, sep);
		lstrcat(temp, (TCHAR*)lpMsgBuf);
	}

	LocalFree( lpMsgBuf );

	MessageBox(g_hMDIFrame, temp, TEXT("Fatal Error"), MB_OK);
	ExitProcess(0);
}

void	*MyAlloc(DWORD dwBytes)
{
	void *p;

	if (NULL == (p = HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, dwBytes)))
	{
		FatalError(GetLastError(), TEXT("Failed to allocate %d bytes."), dwBytes);
	}

	return p;
}

void	MyFree(void *p)
{
	if (!HeapFree(g_hProcessHeap, 0, p))
	{
		FatalError(GetLastError(), TEXT("HeapFree of %p failed."), p);
	}
}

void	ClampScrollPos(SCROLLINFO *sbi)
{
	if (sbi->nPos < sbi->nMin)
	{
		sbi->nPos = sbi->nMin;
	}

	if (sbi->nPos > sbi->nMax)
	{
		sbi->nPos = sbi->nMax;
	}
}

void	SetWindowStandards(HWND hwnd, const TCHAR *szTitle)
{
	SetWindowText(hwnd, szTitle);

	SendMessage (hwnd, WM_SETICON, (WPARAM) ICON_BIG, LPARAM (g_hIconGaoGao));
	SendMessage (hwnd, WM_SETICON, (WPARAM) ICON_SMALL, LPARAM (g_hIconGaoGao));
}

void	ResetStatusBar(void)
{
	int		widths[1];

	widths[0] = -1;

	SendMessage (g_hWndStatusBar, SB_SETPARTS, 1, (LPARAM)widths);
	SendMessage (g_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)TEXT("Faria Editor"));
}

static BOOL CALLBACK MapUpdateProc (HWND hWnd, LPARAM lParam)
{
	TCHAR	szClassName[256];

	if (GetWindow(hWnd, GW_OWNER))
	{
		return TRUE;
	}

	GetClassName(hWnd, szClassName, TCHAR_sizeof(szClassName));

	if (!lstrcmp(szClassName, szClassMapEditorContainer))
	{
		SendMessage(hWnd, WM_USER_MAP_EDITOR_UPDATE_TILE, 0, lParam);
	}

	return TRUE;
}

static BOOL CALLBACK CloseEnumProc (HWND hwnd, LPARAM lParam)
{
     if (GetWindow (hwnd, GW_OWNER))         // Check for icon title
          return TRUE ;
     
     SendMessage (GetParent (hwnd), WM_MDIRESTORE, (WPARAM) hwnd, 0) ;
     
     if (!SendMessage (hwnd, WM_QUERYENDSESSION, 0, 0))
          return TRUE ;
     
     SendMessage (GetParent (hwnd), WM_MDIDESTROY, (WPARAM) hwnd, 0) ;
     return TRUE ;
}

static void	FrameSizeHandler(HWND hWnd)
{
	RECT cr; // client rect
	RECT stbr; // status bar rect

	GetClientRect(g_hWndStatusBar,&stbr);
	GetClientRect(hWnd,&cr);

	SetWindowPos(
		g_hMDIClient,
		NULL,
		0,
		0,
		cr.right,
		cr.bottom-stbr.bottom,
		SWP_NOZORDER
		);

	SetWindowPos(
		g_hWndStatusBar,			// Window to move.
		NULL,						// Window to place after.
		0, 							// X
		cr.bottom-stbr.bottom,		// Y
		cr.right,					// cx
		stbr.bottom,				// cy
		SWP_NOZORDER				// flags
		);

}

static void DoSave(HWND hWnd)
{
	TCHAR szFullPath[_MAX_PATH];

	if (!DoSanityChecks(hWnd))
	{
		return;
	}

	lstrcpyn(szFullPath, g_szWorkingDir, TCHAR_sizeof(szFullPath));
	PathAppend(szFullPath, TEXT("Faria.nes"));
	if (!BrowseForRomToSaveAs(hWnd, szFullPath, TCHAR_sizeof(szFullPath)))
	{
		return;
	}

	SaveFariaRom(hWnd, szFullPath);
}

static void DoGenericOpenFile(HWND hWnd)
{
	TCHAR szFullPath[_MAX_PATH];
	TCHAR *pszExt;

	lstrcpyn(szFullPath, g_szWorkingDir, TCHAR_sizeof(szFullPath));
	PathAppend(szFullPath, TEXT("Faria.nes"));
	if (BrowseForGenericFileToOpen(hWnd, szFullPath, TCHAR_sizeof(szFullPath)))
	{
		if (NULL != (pszExt = PathFindExtension(szFullPath)))
		{
			if (!lstrcmpi(pszExt, TEXT(".SAV")))
			{
				DialogBoxParam(g_hInstance, MAKEINTRESOURCE(DIALOG_SAVE_GAME), hWnd, SaveGameDlgProc, (LPARAM)szFullPath);
			}
			else if (!lstrcmpi(pszExt, TEXT(".NES")))
			{
				if (LoadFariaRom(hWnd, szFullPath))
				{
					EditMap_Overworld();
//					EditMap_Skyworld();
//					EditMap_Caveworld();
//					EditMap_Town(10);
				}
			}
		}
	}
}


static void	DoFrameCreate(HWND hWnd)
{
	CLIENTCREATESTRUCT ccs; 
	RECT	rStatusBar;
	RECT	rFrame;

	// Set "g_hMDIFrame" here because this function might call another that uses the global
	// "g_hMDIFrame".  Previously, "g_hMDIFrame" was not set until the call to "CreateWindowEx()",
	// (which triggers this "DoFrameCreate()") completes.
	g_hMDIFrame = hWnd;

	if (NULL == (g_hWndStatusBar = CreateStatusWindow(SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, TEXT("StatusBar"), hWnd, STATUS_BAR_CTRL_ID)))
	{
		FatalError(GetLastError(), TEXT("CreateStatusWindow() failed."));
	}

	GetClientRect(g_hWndStatusBar, &rStatusBar);
	GetClientRect(hWnd, &rFrame);

	ccs.hWindowMenu  = GetSubMenu(GetMenu(hWnd), 3);
	ccs.idFirstChild = ID_MDI_FIRSTCHILD;

	if (NULL == (g_hMDIClient = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("MDICLIENT"), NULL,
		WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
		0, 0, rFrame.right, rFrame.bottom - rStatusBar.bottom,
		hWnd, (HMENU)0, g_hInstance, (LPVOID)&ccs)))
	{
		FatalError(GetLastError(), TEXT("CreateWindowEx() failed to create MDIClient window."));
	}

	ShowWindow(g_hMDIClient, SW_SHOW);

	DoGenericOpenFile(hWnd);
}

void	EditMap_Overworld(void)
{
	MAP_INFO	mi;

	memset(&mi, 0, sizeof(mi));
	mi.map_type = MAP_OVERWORLD;
	mi.szTitle = TEXT("Faria Over World");
	mi.map_width = OW_MAP_WIDTH;
	mi.map_height = OW_MAP_HEIGHT;
	mi.pMap = g_OverworldMap;
	mi.ucOutOfBoundsTile = 0x00;
	mi.hPaletteBitmap = g_hOverworldMapTiles;
	mi.tile_width = 16;
	mi.tile_height = 16;
	mi.tile_count = 128;
	mi.tile_offset = 0;

	CreateMapEditorWindow(&mi);
}

void	EditMap_Skyworld(void)
{
	MAP_INFO	mi;

	memset(&mi, 0, sizeof(mi));
	mi.map_type = MAP_SKYWORLD;
	mi.szTitle = TEXT("Faria Sky World");
	mi.map_width = SKY_MAP_WIDTH;
	mi.map_height = SKY_MAP_HEIGHT;
	mi.pMap = g_SkyworldMap;
	mi.ucOutOfBoundsTile = 0x00;
	mi.hPaletteBitmap = g_hSkyworldMapTiles;
	mi.tile_width = 16;
	mi.tile_height = 16;
	mi.tile_count = 4;
	mi.tile_offset = 0;

	CreateMapEditorWindow(&mi);
}

void	EditMap_Caveworld(void)
{
	MAP_INFO	mi;

	memset(&mi, 0, sizeof(mi));
	mi.map_type = MAP_UNDERWORLD;
	mi.szTitle = TEXT("Faria Under World");
	mi.map_width = CAVE_MAP_WIDTH;
	mi.map_height = CAVE_MAP_HEIGHT;
	mi.pMap = g_CaveworldMap;
	mi.ucOutOfBoundsTile = 0x03;
	mi.hPaletteBitmap = g_hCaveworldMapTiles;
	mi.tile_width = 16;
	mi.tile_height = 16;
	mi.tile_count = 4;
	mi.tile_offset = 0;

	CreateMapEditorWindow(&mi);
}

void	EditMap_Town(int nTown)
{
	MAP_INFO	mi;
	TCHAR		szTitle[128];

	_ASSERT((nTown >= 0) && (nTown < TOWN_COUNT + 1));

	wnsprintf(szTitle, TCHAR_sizeof(szTitle), TEXT("Faria Town: %s"), g_TownInfo[nTown].szName);

	memset(&mi, 0, sizeof(mi));
	mi.map_type = MAP_TOWN;
	mi.szTitle = szTitle;
	mi.map_width = g_TownSizes[nTown].cx;
	mi.map_height = g_TownSizes[nTown].cy;
	mi.pMap = g_TownMaps[nTown];
	mi.ucOutOfBoundsTile = 0x00;
	mi.hPaletteBitmap = g_hTownMapTiles;
	mi.tile_width = 16;
	mi.tile_height = 16;
	mi.tile_count = 128;
	mi.tile_offset = 0;

	if (nTown == TOWN_COUNT)	// Is it the castle?
	{
		mi.tile_offset = 0x60;
		mi.hPaletteBitmap = g_hCastleMapTiles;
		mi.tile_count = 32;
		mi.ucOutOfBoundsTile = 0x60;
	}

	CreateMapEditorWindow(&mi);
}


static long FAR PASCAL MPFrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
		case WM_CREATE: 
			DoFrameCreate(hWnd);
			break; 

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case CM_FILE_EXIT:
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					break;

				case CM_FILE_OPEN:
					DoGenericOpenFile(hWnd);
					break;

				case CM_FILE_SAVE:
					DoSave(hWnd);
					break;

				case CM_HELP_ABOUT:
					DialogBox(g_hInstance, MAKEINTRESOURCE(DIALOG_ABOUT_BOX), hWnd, AboutBoxDlgProc);
					break;

				case CM_EDIT_MAP:
					DialogBox(g_hInstance, MAKEINTRESOURCE(DIALOG_CHOOSE_MAP), hWnd, ChooseMapDlgProc);
					break;

				case CM_EDIT_OVERWORLD:
					if (g_pRom)
					{
						EditMap_Overworld();
					}
					break;

				case CM_EDIT_SKYWORLD:
					if (g_pRom)
					{
						EditMap_Skyworld();
					}
					break;

				case CM_EDIT_CAVEWORLD:
					if (g_pRom)
					{
						EditMap_Caveworld();
					}
					break;

				case CM_MDI_STUB:
					CreateMdiChildStubWindow();
					break;

				case IDM_WINDOW_TILE:
					SendMessage (g_hMDIClient, WM_MDITILE, 0, 0) ;
					return 0 ;

				case IDM_WINDOW_CASCADE:
					SendMessage (g_hMDIClient, WM_MDICASCADE, 0, 0) ;
					return 0 ;

				case IDM_WINDOW_ARRANGE:
					SendMessage (g_hMDIClient, WM_MDIICONARRANGE, 0, 0) ;
					return 0 ;

				case IDM_WINDOW_CLOSEALL:     // Attempt to close all children
					EnumChildWindows (g_hMDIClient, CloseEnumProc, 0) ;
					return 0 ;					

				// Handle MDI Window commands
				default:
				{
					if(LOWORD(wParam) >= ID_MDI_FIRSTCHILD)
					{
						DefFrameProc(hWnd, g_hMDIClient, message, wParam, lParam);
					}
					else 
					{
						HWND hChild = (HWND)SendMessage(g_hMDIClient, WM_MDIGETACTIVE,0,0);
						if(hChild)
						{
							SendMessage(hChild, WM_COMMAND, wParam, lParam);
						}
					}
				}
			}
			break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
			break;

        case WM_DESTROY:
            PostQuitMessage(0);
			break;

		case WM_SIZE:
			FrameSizeHandler(hWnd);
			// We are handling WM_SIZE, so do NOT call "DefFrameProc"
			return 0;

		case WM_USER_MAP_EDITOR_UPDATE_TILE:
			EnumChildWindows(g_hMDIClient, MapUpdateProc, lParam);
			return 0;

		default:
			return DefFrameProc(hWnd, g_hMDIClient, message, wParam, lParam);
	}

	return DefFrameProc(hWnd, g_hMDIClient, message, wParam, lParam);
}


int	__stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSEX wc; 

	g_hInstance = hInstance;
	g_hProcessHeap = GetProcessHeap();

	InitCommonControls();
	InitRegistry();

	g_hIconGaoGao = (HICON)LoadImage (g_hInstance, MAKEINTRESOURCE (ICON_GAOGAO), IMAGE_ICON, 64, 64, 0);
	g_hIconLand = (HICON)LoadImage (g_hInstance, MAKEINTRESOURCE (ICON_LAND), IMAGE_ICON, 32, 32, 0);
	g_hIconTower = (HICON)LoadImage (g_hInstance, MAKEINTRESOURCE (ICON_TOWER), IMAGE_ICON, 32, 32, 0);

	g_hMenuFrame = LoadMenu(g_hInstance, MAKEINTRESOURCE(MENU_MAIN));
	g_hMenuMapEditor = LoadMenu(g_hInstance, MAKEINTRESOURCE(MENU_MAP_EDITOR));
	g_hMenuMdiStub = LoadMenu(g_hInstance, MAKEINTRESOURCE(MENU_STUB));

	g_hMenuFrame_Window  = GetSubMenu (g_hMenuFrame,   WINDOW_POS_MENU_MAIN);
	g_hMenuMapEditor_Window = GetSubMenu (g_hMenuMapEditor, WINDOW_POS_MENU_MAP_EDITOR);
	g_hMenuMdiStub_Window  = GetSubMenu (g_hMenuMdiStub,   WINDOW_POS_MENU_STUB);
	AddZoomCommandsToMenu(GetSubMenu(g_hMenuMapEditor, ZOOM_POS_MENU_MAP_EDITOR));

	g_hCursorHandOpen = LoadCursor(g_hInstance, MAKEINTRESOURCE(CURSOR_HAND_OPEN));
	g_hCursorHandClosed = LoadCursor(g_hInstance, MAKEINTRESOURCE(CURSOR_HAND_CLOSED));
	g_hCursorHandPointing = LoadCursor(g_hInstance, MAKEINTRESOURCE(CURSOR_HAND_POINTING));
	g_hCursorPen = LoadCursor(g_hInstance, MAKEINTRESOURCE(CURSOR_PEN));
	g_hCursorSelect = LoadCursor(g_hInstance, MAKEINTRESOURCE(CURSOR_SELECT));

	if (0 == (g_uMapClipboardFormat = RegisterClipboardFormat(szMapClipboardFormat)))
	{
		FatalError(GetLastError(), TEXT("Failed to register a clipboard format: %s"), szMapClipboardFormat);
	}

    // Register the frame window class. 
	memset(&wc, 0, sizeof(wc));

	wc.cbSize		 = sizeof(wc); 
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MPFrameWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = g_hIconGaoGao;
    wc.hCursor       = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1); 
    wc.lpszMenuName  = MAKEINTRESOURCE(MENU_MAIN);
    wc.lpszClassName = szClassFrame; 
	wc.hIconSm		 = NULL;
 
    if (!RegisterClassEx (&wc)) 
	{
		FatalError(GetLastError(), TEXT("RegisterClassEx('%s') failed"), wc.lpszClassName);
	}

	RegisterMapEditorClass();
	RegisterMdiChildStubClass();

	// This font is used in the hexidecimal editor / viewer.
	if (NULL == (g_hFontMono = CreateFont(
		12,				// height of font
		10,				// average character width
		0,				// angle of escapement
		0,				// base-line orientation angle
		FW_DONTCARE,			// font weight
		false,				// italic attribute option
		false,				// underline attribute option
		false,				// strikeout attribute option
		DEFAULT_CHARSET,		// character set identifier
		OUT_DEFAULT_PRECIS,		// output precision
		CLIP_DEFAULT_PRECIS,		// clipping precision
		DEFAULT_QUALITY,		// output quality
		FF_DONTCARE | DEFAULT_PITCH,	// pitch and family
		TEXT("Courier New"))))				// typeface name
	{
		FatalError(GetLastError(), TEXT("Failed to create font."));
	}

	if (NULL == (g_hOverworldMapTiles = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BITMAP_OW_TILES))))
	{
		FatalError(GetLastError(), TEXT("Failed to load Overworld Tile Bitmap."));
	}

	if (NULL == (g_hSkyworldMapTiles = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BITMAP_SKY_TILES))))
	{
		FatalError(GetLastError(), TEXT("Failed to load Sky Tile Bitmap."));
	}

	if (NULL == (g_hCaveworldMapTiles = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BITMAP_CAVE_TILES))))
	{
		FatalError(GetLastError(), TEXT("Failed to load Cave Tile Bitmap."));
	}

	if (NULL == (g_hTownMapTiles = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BITMAP_TOWN_TILES))))
	{
		FatalError(GetLastError(), TEXT("Failed to load Town Tile Bitmap."));
	}

	if (NULL == (g_hCastleMapTiles = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BITMAP_CASTLE_TILES))))
	{
		FatalError(GetLastError(), TEXT("Failed to load Castle Tile Bitmap."));
	}

	if (NULL == (g_hGaoGaoMonster = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BITMAP_GAOGAO))))
	{
		FatalError(GetLastError(), TEXT("Failed to load GaoGao Monster Bitmap."));
	}

	if (NULL == (g_hMDIFrame = CreateWindow(szClassFrame, szClassFrame, 
		WS_OVERLAPPEDWINDOW | MDIS_ALLCHILDSTYLES ,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, g_hInstance, NULL)))
	{
		FatalError(GetLastError(), TEXT("Failed to create MDI frame window."));
	}

	SetWindowStandards(g_hMDIFrame, TEXT("Faria Editor"));
    ShowWindow(g_hMDIFrame, nShowCmd);
    UpdateWindow(g_hMDIFrame);

	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else 
		{ 
			if (!TranslateMDISysAccel(g_hMDIClient, &msg))
			{ 
				TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			} 
		} 
	}

	DeleteObject(g_hOverworldMapTiles);
	DeleteObject(g_hSkyworldMapTiles);
	DeleteObject(g_hCaveworldMapTiles);
	DeleteObject(g_hTownMapTiles);
	DeleteObject(g_hCastleMapTiles);
	DeleteObject(g_hGaoGaoMonster);
	ShutdownRegistry();
	return 0;
}
