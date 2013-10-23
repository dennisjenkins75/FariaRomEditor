/*	Common header file that should be included by all source files
	in this program. */

#ifndef __FARIA_COMMON_H__
#define __FARIA_COMMON_H__

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define STRSAFE_NO_DEPRECATE
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shlwapi.h>

#include <stdio.h>
#include <crtdbg.h>
#include <strsafe.h>
#include <tchar.h>

#include "../FariaLib/FariaLib.h"

#include "resource.h"

#define TCHAR_sizeof(x) (sizeof(x)/sizeof(TCHAR))

static const int STATUS_BAR_CTRL_ID = 30000;
static const int CM_ZOOM_BASE = 29000;

static const int WINDOW_POS_MENU_MAIN = 2;
static const int WINDOW_POS_MENU_MAP_EDITOR = 4;
static const int WINDOW_POS_MENU_STUB = 2;
static const int ZOOM_POS_MENU_MAP_EDITOR = 3;

#include "Globals.h"
#include "MapEditor.h"
#include "Prototypes.h"

#endif // __FARIA_COMMON_H__