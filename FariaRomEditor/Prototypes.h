/* Prototypes.h */

#ifndef __FARIA_EDITOR_PROTOTYPES_H__
#define __FARIA_EDITOR_PROTOTYPES_H__

// AboutBox.cpp
INT_PTR CALLBACK AboutBoxDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ChooseMap.cpp
INT_PTR CALLBACK ChooseMapDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// EditSaveFile.cpp
INT_PTR CALLBACK SaveGameDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// LoadSaveRom.cpp
bool	LoadFariaRom(HWND hWnd, const TCHAR *pszFullPath);
bool	SaveFariaRom(HWND hWnd, const TCHAR *pszFullPath);
bool	BrowseForGenericFileToOpen(HWND hWnd, TCHAR *szFullPath, int nPathLen);
bool	BrowseForRomToSaveAs(HWND hWnd, TCHAR *szFullPath, int nPathLen);

// MdiChildStub.cpp
void	CreateMdiChildStubWindow(void);
void	RegisterMdiChildStubClass(void);

// Misc.cpp
void	NormalizeRect(RECT *rect);
bool	IsTileCaveEntrence(int col, int row, int &cave);
bool	IsTileCaveExit(int col, int row, int &cave);
bool	IsTileATown(int col, int row, int &town);

// OverWorldGui.cpp
void	CreateMapEditorWindow(MAP_INFO *pMapInfo);
void	RegisterMapEditorClass(void);

// Registry.cpp
void	InitRegistry(void);
void	ShutdownRegistry(void);
DWORD 	RegistryReadDword(HKEY hKey, const TCHAR *szStr);
bool	RegistryReadDword(HKEY hKey, const TCHAR *szName, DWORD &dwValue);
bool	RegistryReadString(HKEY hKey, const TCHAR *szName, TCHAR *dest, int destlen);
void	RegistryWriteDword(HKEY hKey, const TCHAR *szName, DWORD &dwValue);
void	RegistryWriteString(HKEY hKey, const TCHAR *szName, const TCHAR *szValue);
bool	RegistryDelete(HKEY hBase, const TCHAR *szKey, const TCHAR *szStr);

// RomStringTable.cpp
const TCHAR *FariaGetItemName(int iIndex);

// SanityChecks.cpp
bool	DoSanityChecks(HWND hWnd);

// WinMain.cpp
void	FatalError(DWORD dwWin32ErrorCode, const TCHAR *fmt, ...);
void	ClampScrollPos(SCROLLINFO *sbi);
void	SetWindowStandards(HWND hwnd, const TCHAR *szTitle);
void	ResetStatusBar(void);
void	*MyAlloc(DWORD dwBytes);
void	MyFree(void *p);

void	EditMap_Overworld(void);
void	EditMap_Skyworld(void);
void	EditMap_Caveworld(void);
void	EditMap_Town(int nTown);


#endif // __FARIA_EDITOR_PROTOTYPES_H__
