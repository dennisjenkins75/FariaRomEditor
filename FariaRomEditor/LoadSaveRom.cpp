/* LoadSaveRom.cpp */

#include "FariaRomEditor.h"

// Returns "true" if the function exits with a valid ROM loaded into memory.
// Can be a new ROM or a previous ROM if the user hits "cancel" and a ROM
// was previously loaded.

// Returns "false" if no ROM is loaded on exit.

bool	LoadFariaRom(HWND hWnd, const TCHAR *pszFullPath)
{
	HANDLE hFile;
	DWORD dwBytes;
	TCHAR szTitle[_MAX_PATH + 80];
	int i;

	if (g_bRomDirty)
	{
		if (IDYES != MessageBox(hWnd, TEXT("You have unsaved changes.  Do you wish to open a ROM and lose those changes?"), TEXT("Warning"), MB_YESNO | MB_ICONWARNING))
		{
			return true;
		}
	}

	if (!pszFullPath)
	{
		OPENFILENAME ofn;

		memset(&ofn, 0, sizeof(ofn));
		ofn.hInstance = g_hInstance;
		ofn.hwndOwner = hWnd;
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400 ;//sizeof(ofn);
		ofn.lpstrFilter = TEXT("faria.nes");
		ofn.lpstrFile = g_szRomFile;
		ofn.nMaxFile = TCHAR_sizeof(g_szRomFile);
		ofn.lpstrInitialDir = g_szWorkingDir;
		ofn.lpstrTitle = TEXT("NES Faria ROM (iNES format)");
		ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

		if (!GetOpenFileName(&ofn))
		{
			return NULL != g_pRom;
		}

		lstrcpyn(g_szWorkingDir, ofn.lpstrFile, ofn.nFileOffset);
		g_szWorkingDir[ofn.nFileOffset] = 0;
	}
	else
	{
		lstrcpyn(g_szRomFile, pszFullPath, sizeof(g_szRomFile));
	}

	g_pRom = NULL;	// Indicate that we do not have an open file.

	if (INVALID_HANDLE_VALUE == (hFile = CreateFile(g_szRomFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)))
	{
		MessageBox(hWnd, TEXT("Unable to open file for reading."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (!ReadFile(hFile, g_iNES, INES_SIZE, &dwBytes, NULL))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to read file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (INES_SIZE != dwBytes)
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to read entire file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	CloseHandle(hFile);

	wnsprintf(szTitle, TCHAR_sizeof(szTitle), TEXT("Faria ROM Editor - %s"), g_szRomFile);
	SetWindowText(hWnd, szTitle);

	g_pRom = g_iNES + 16;

	FL_DecompressOverworld(g_pRom, g_OverworldMap);
	FL_DecompressSkyworld(g_pRom, g_SkyworldMap);
	FL_DecompressCaveworld(g_pRom, g_CaveworldMap);

	for (i = 0; i < TOWN_COUNT+1; i++)
	{
		int w, h, b;

		FL_DecompressTown(g_pRom, i, g_TownMaps[i], &w, &h, &b);

		g_TownSizes[i].cx = w;
		g_TownSizes[i].cy = h;
	}

	RegistryWriteString(g_hRegKeyRoot, TEXT("WorkingDir"), g_szWorkingDir);

	return true;
}

bool	SaveFariaRom(HWND hWnd, const TCHAR *pszFullPath)
{
	HANDLE hFile = NULL;
	DWORD dwBytes = 0;

	FL_CompressOverworld(g_pRom, g_OverworldMap, false);
//	FL_CompressSkyworld(g_pRom, g_SkyworldMap);
//	FL_CompressCaveworld(g_pRom, g_CaveworldMap);

	if (INVALID_HANDLE_VALUE == (hFile = CreateFile(pszFullPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL)))
	{
		MessageBox(hWnd, TEXT("Unable to open file for writing."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (!WriteFile(hFile, g_iNES, INES_SIZE, &dwBytes, NULL))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to write file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (INES_SIZE != dwBytes)
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to write entire file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	CloseHandle(hFile);
	return true;
}


// szFullPath MUST be "_MAX_PATH" chars in length
bool BrowseForGenericFileToOpen(HWND hWnd, TCHAR *szFullPath, int nPathLen)
{
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof(ofn));
	ofn.hInstance = g_hInstance;
	ofn.hwndOwner = hWnd;
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;//sizeof(ofn);
	ofn.lpstrFilter = TEXT("FARIA.SAV;FARIA.NES");
	ofn.lpstrFile = szFullPath;
	ofn.nMaxFile = nPathLen;
	ofn.lpstrInitialDir = g_szWorkingDir;
	ofn.lpstrTitle = TEXT("Faria.NES or FARIA.SAV");
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (!GetOpenFileName(&ofn))
	{
		return false;
	}

	lstrcpyn(g_szWorkingDir, ofn.lpstrFile, ofn.nFileOffset);
	g_szWorkingDir[ofn.nFileOffset] = 0;

	RegistryWriteString(g_hRegKeyRoot, TEXT("WorkingDir"), g_szWorkingDir);

	return true;
}

// szFullPath MUST be "_MAX_PATH" chars in length
bool BrowseForRomToSaveAs(HWND hWnd, TCHAR *szFullPath, int nPathLen)
{
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof(ofn));
	ofn.hInstance = g_hInstance;
	ofn.hwndOwner = hWnd;
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;//sizeof(ofn);
	ofn.lpstrFilter = TEXT("*.NES");
	ofn.lpstrFile = szFullPath;
	ofn.nMaxFile = nPathLen;
	ofn.lpstrInitialDir = g_szWorkingDir;
	ofn.lpstrTitle = TEXT("Faria.NES");
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST;

	if (!GetSaveFileName(&ofn))
	{
		return false;
	}

	lstrcpyn(g_szWorkingDir, ofn.lpstrFile, ofn.nFileOffset);
	g_szWorkingDir[ofn.nFileOffset] = 0;

	RegistryWriteString(g_hRegKeyRoot, TEXT("WorkingDir"), g_szWorkingDir);

	return true;
}
