/* OverWorldGui.cpp */

#include "FariaRomEditor.h"

struct	DATA
{
	int unused;
};

static DATA*	CreateDataStruct(void)
{
	DATA  *pData = (DATA*)MyAlloc(sizeof(DATA));
	pData->unused = 0;

	return pData;
}

static void DestroyDataStruct(DATA *pData)
{
	_ASSERT(pData);
	MyFree(pData);
}

static void	DoPaint(HWND hWnd, DATA *pData)
{
	RECT rect;
	HDC hdc;
	HDC hdc2;
	HBITMAP hBmpOld;
	PAINTSTRUCT ps;
	int x, y;

	hdc = BeginPaint (hWnd, &ps) ;
	GetClientRect (hWnd, &rect) ;
   
	hdc2 = CreateCompatibleDC(hdc);
	hBmpOld = (HBITMAP)SelectObject(hdc2, g_hOverworldMapTiles);

	x = (rect.right - 256) / 2;
	y = (rect.bottom - 128) / 2;

	BitBlt(hdc, 0, 0, 256, 128, hdc2, 0, 0, SRCCOPY);

	SelectObject(hdc2, hBmpOld);
	DeleteDC(hdc2);

	EndPaint (hWnd, &ps) ;
}

static void	InitStatusBar(void)
{
	int		widths[10];

	widths[0] = 100;
	widths[1] = 200;
	widths[2] = -1;

	SendMessage (g_hWndStatusBar, SB_SETPARTS, 3, (LPARAM)widths);
	SendMessage (g_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)TEXT("Mdi Stub"));
	SendMessage (g_hWndStatusBar, SB_SETTEXT, 1, (LPARAM)TEXT("Nothing"));
	SendMessage (g_hWndStatusBar, SB_SETTEXT, 2, (LPARAM)TEXT("This space intentionally left blank."));
}

// With rare exception, all messages should call "DefMDIChildProc()" instead of 
// just doing a "return 0" or "return 1".
static long FAR PASCAL MdiChildStubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DATA		*pData = (DATA*)GetWindowLongPtr (hWnd, 0) ;

	switch (message)
	{
		case WM_CREATE:
			pData = CreateDataStruct();
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)pData);
			break;
			
		case WM_DESTROY:
			DestroyDataStruct(pData);
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)NULL);
			break;

		case WM_PAINT:
			DoPaint(hWnd, pData);
			return 0;

		case WM_MDIACTIVATE:
			// Set our menu if we are gaining focus.
			if (lParam == (LPARAM) hWnd)
			{
				SendMessage (GetParent(hWnd), WM_MDISETMENU, (WPARAM) g_hMenuMdiStub, (LPARAM) g_hMenuMdiStub_Window);
				InitStatusBar();
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
	}

	return DefMDIChildProc (hWnd, message, wParam, lParam) ;
}

void	CreateMdiChildStubWindow(void)
{
	DWORD dwStyle = WS_HSCROLL | WS_VSCROLL | WS_MAXIMIZEBOX;

	HWND hWnd = CreateMDIWindow(
		szClassMdiStub,				// lpClassName
		TEXT("MDI Child Stub"),		// lpWindowName
		dwStyle,					// dwStyle
		CW_USEDEFAULT,				// X
		CW_USEDEFAULT,				// Y
		256,//CW_USEDEFAULT,				// nWidth
		128,//CW_USEDEFAULT,				// nHeight
		g_hMDIClient,				// hWndParent
		g_hInstance,				// hInstance
		NULL);						// lParam

	if (NULL == hWnd)
	{
		FatalError(GetLastError(), TEXT("CreateMDIWindow() failed in CreateMdiChildStubWindow."));
	}
}

void	RegisterMdiChildStubClass(void)
{
    WNDCLASSEX wc; 

	memset(&wc, 0, sizeof(wc));

	wc.cbSize		 = sizeof(wc); 
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MdiChildStubWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(DATA*);
    wc.hInstance     = g_hInstance;
    wc.hIcon         = g_hIconLand;
    wc.hCursor       = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1); 
    wc.lpszMenuName  = (LPCTSTR)NULL;//MAKEINTRESOURCE(MENU_MAIN);
    wc.lpszClassName = szClassMdiStub; 
	wc.hIconSm		 = NULL;
 
    if (!RegisterClassEx (&wc)) 
	{
		FatalError(GetLastError(), TEXT("RegisterClassEx('%s') failed"), wc.lpszClassName);
	}
}