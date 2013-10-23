/* Registry.cpp */

#include "FariaRomEditor.h"

void	InitRegistry(void)
{
	long lResult = 0;
	DWORD dwDisposition;
	HKEY	hKeyTemp;

	if (ERROR_SUCCESS != (lResult = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\eColiGames"), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKeyTemp, &dwDisposition)))
	{
		FatalError(lResult, TEXT("Unable to create registry key: HKCU/Software/eColiGames"));
	}

	if (ERROR_SUCCESS != (lResult = RegCreateKeyEx(hKeyTemp, TEXT("FariaEditor"), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &g_hRegKeyRoot, &dwDisposition)))
	{
		FatalError(lResult, TEXT("Unable to create registry key: HKCU/Software/eColiGames/FariaEditor"));
	}

	RegistryReadString(g_hRegKeyRoot, TEXT("WorkingDir"), g_szWorkingDir, TCHAR_sizeof(g_szWorkingDir));
}

void	ShutdownRegistry(void)
{
	RegCloseKey(g_hRegKeyRoot);
}

DWORD 	RegistryReadDword(HKEY hKey, const TCHAR *szStr)
{
	DWORD dwValue = 0;
	DWORD dwType = 0;
	DWORD dwSize = sizeof(dwValue);

	if (ERROR_SUCCESS != RegQueryValueEx(hKey, szStr, NULL, &dwType, (BYTE*)&dwValue, &dwSize))
	{
		return 0;
	}

	if (dwType != REG_DWORD)
	{
		return 0;
	}

	return dwValue;
}

bool	RegistryReadDword(HKEY hKey, const TCHAR *szName, DWORD &dwValue)
{
	long lResult;
	DWORD dwType = 0;
	DWORD dwSize = sizeof(dwValue);

	if (ERROR_SUCCESS == (lResult = RegQueryValueEx(hKey, szName, NULL, &dwType, (BYTE*)&dwValue, &dwSize)))
	{
		return dwType == REG_DWORD;
	}

	return false;
}

bool	RegistryReadString(HKEY hKey, const TCHAR *szName, TCHAR *dest, int destlen)
{
	long lResult;
	DWORD dwType = 0;
	DWORD dwSize = destlen;

	dest[0] = 0;

	if (ERROR_SUCCESS == (lResult = RegQueryValueEx(hKey, szName, NULL, &dwType, (BYTE*)dest, &dwSize)))
	{
		return dwType == REG_SZ;
	}

	return false;
}

void	RegistryWriteDword(HKEY hKey, const TCHAR *szName, DWORD &dwValue)
{
	RegSetValueEx(hKey, szName, NULL, REG_DWORD, (BYTE*)&dwValue, sizeof(dwValue));
}

void	RegistryWriteString(HKEY hKey, const TCHAR *szName, const TCHAR *szValue)
{
	if (szValue && szValue[0])
	{
		RegSetValueEx(hKey, szName, NULL, REG_SZ, (BYTE*)szValue, lstrlen(szValue) * sizeof(TCHAR));
	}
	else
	{
		RegSetValueEx(hKey, szName, NULL, REG_SZ, (BYTE*)NULL, 0);
	}
}

bool	RegistryDelete(HKEY hBase, const TCHAR *szKey, const TCHAR *szStr)
{
	long	lErrorCode;
	HKEY	hKey;
	bool	ret;

	if ((lErrorCode = RegOpenKeyEx(hBase, szKey, 0, KEY_ALL_ACCESS, &hKey)) != ERROR_SUCCESS)
	{
		return false;
	}

	ret = ERROR_SUCCESS == RegDeleteValue(hKey, szStr);

	RegCloseKey(hKey);

	return ret;
}
