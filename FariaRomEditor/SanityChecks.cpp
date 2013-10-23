/* SanityChecks.cpp */

#include "FariaRomEditor.h"

/*
Compressed map overflows allocated space.

Town tiles not on town entry location.

Town entry locations not on town tiles.

Town tiles are not in proper format.

Cave entrences are not in data list.

Cave entrences are missing from data list.

Path-finding from final tower to "main land" (any city) fails.
*/

// Returns 'true' if everything is safe to save.
// Returns 'false' if it is not safe to save the map.
bool	DoSanityChecks(HWND hWnd)
{
	int rle_bytes = FL_CompressOverworld(g_pRom, g_OverworldMap, true);

	if (rle_bytes > OVERWORLD_RLE_MAX_LEN)
	{
		MessageBox(hWnd, TEXT("Cannot create new ROM image; internal data is not safe.  Most likely, a map won't compress to fit in a fixed size buffer."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}
