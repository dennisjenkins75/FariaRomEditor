/* AboutBox.cpp */

#include "FariaRomEditor.h"

/*
static char *HarvestVersionDataElement(void *pBuffer, const TCHAR *psztObject)
{
	WORD* langInfo;
	UINT cbLang;
	TCHAR tszVerStrName[128];
	LPVOID lpt;
	UINT cbBufSize;

	// Get the Company Name.
	// First, to get string information, we need to get language information.
	if (!VerQueryValue(pBuffer, _T("\\VarFileInfo\\Translation"), (LPVOID*)&langInfo, &cbLang))
	{
		return NULL;
	}

    //Prepare the label -- default lang is bytes 0 & 1 of langInfo
    wsprintf(tszVerStrName, _T("\\StringFileInfo\\%04x%04x\\%s"), langInfo[0], langInfo[1], psztObject);

	//Get the string from the resource data
	if (VerQueryValue(pBuffer, tszVerStrName, &lpt, &cbBufSize))
	{
		return strdup((char*)lpt);
	}

	return NULL;
}
*/

// Read VS_VERSION_INFO resource from given file.  Parses version info
// into the given structure.  Returns 'true' if successful, 'false' if it fails.
// Failure reasons hould still be in GetLastError().  'szFilename' can be NULL.
// If 'szFilename' is NULL, function will use current module.

// Reads VS_VERSION_INFO resource from current module.
// Extracts named field into user supplied buffer.

static bool	GetModuleVersionInfo(WORD *pwProductVersion)
{
	TCHAR szInternalFilename[MAX_PATH];
	void *pBuffer;
	DWORD dwSize, dwDummy, dwBufSize;
	VS_FIXEDFILEINFO *lpFFI;

	memset(pwProductVersion, 0, sizeof(WORD) * 4);

	if (!GetModuleFileName(NULL, szInternalFilename, TCHAR_sizeof(szInternalFilename)))
	{
		return false;
	}

	if ((dwSize = GetFileVersionInfoSize(szInternalFilename, &dwDummy)) == 0)
	{
		return false;
	}

	pBuffer = MyAlloc(dwSize);

	if (!GetFileVersionInfo(szInternalFilename, 0, dwSize, pBuffer))
	{
		MyFree(pBuffer);
		return false;
	}

    if (VerQueryValue(pBuffer, TEXT("\\"), (LPVOID *) &lpFFI, (UINT *) &dwBufSize))
    {
		//We now have the VS_FIXEDFILEINFO in lpFFI
	
//		wFileVersion[0] = HIWORD(lpFFI->dwFileVersionMS);
//		wFileVersion[1] = LOWORD(lpFFI->dwFileVersionMS);
//		wFileVersion[2] = HIWORD(lpFFI->dwFileVersionLS);
//		wFileVersion[3] = LOWORD(lpFFI->dwFileVersionLS);

		pwProductVersion[0] = HIWORD(lpFFI->dwProductVersionMS);
		pwProductVersion[1] = LOWORD(lpFFI->dwProductVersionMS);
		pwProductVersion[2] = HIWORD(lpFFI->dwProductVersionLS);
		pwProductVersion[3] = LOWORD(lpFFI->dwProductVersionLS);
    }

//	pszCompanyName = HarvestVersionDataElement(pBuffer, _T("CompanyName"));
//	pszProductName = HarvestVersionDataElement(pBuffer, _T("ProductName"));
//	pszCopyright = HarvestVersionDataElement(pBuffer, _T("LegalCopyright"));
//	pszInternalName = HarvestVersionDataElement(pBuffer, _T("InternalName"));

	MyFree(pBuffer);

	return true;
}



INT_PTR CALLBACK AboutBoxDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR	temp[256];
	WORD	wProductVersion[4];

	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetWindowStandards(hWnd, TEXT("Faria Editor - About"));
			GetModuleVersionInfo(wProductVersion);
			wnsprintf(temp, TCHAR_sizeof(temp), TEXT("Version: %u.%u.%u.%u"),
				wProductVersion[0], wProductVersion[1], 
				wProductVersion[2], wProductVersion[3]);
			SetDlgItemText(hWnd, IDC_VERSION, temp);
			return 1;

		case WM_DESTROY:
			return 1;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, 0);
					return 0;
			}
	}
	return 0;
}
