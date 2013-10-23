/*	Common header file that should be included by all source files
	in this program. */

#ifndef __FARIA_LIB_H__
#define __FARIA_LIB_H__

#pragma comment(lib, "FariaLib.lib")

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

#define STRICT
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <tchar.h>

#include <stdio.h>
#include <crtdbg.h>

struct	NES_PAL
{
	unsigned char r, g, b;
};

extern const NES_PAL nes_palette[64];


struct	TOWN_INFO
{
	TCHAR *szName;
	int x_start;
	int y_start;
};


static const int MAX_LIST_BOX_STR_IDX = 0x61;
extern const TCHAR *g_szFariaListboxStrings[];

//static const int MAX_TOWN_NAMES_IDX = 9;
static const int TOWN_COUNT = 10;
extern const TOWN_INFO g_TownInfo[];

static const int CAVE_COUNT = 0x50;

extern const int g_iValidSwords[];
extern const int g_iValidBows[];
extern const int g_iValidArmours[];
extern const int g_iValidSheilds[];


int		FariaGetString(TCHAR *dest, int dest_len, const unsigned char *faria, int faria_len);
int		FariaSetString(unsigned char *faria, int faria_len, const TCHAR *src, int src_len);
int		FariaResolveListBoxStringIndex(const TCHAR *str);

void	DumpBitmap(HDC hDC, HBITMAP hBitmap, const TCHAR *szFilename);

// OverWorld.cpp

// Max size of compressed overworld map data.
static const int OVERWORLD_RLE_MAX_LEN = 0x1d13;

void	FL_DecompressOverworld
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image.
	unsigned char *buffer		// [OUT] Pointer to 16K (128x128 @1byte/tile).
);

// Returns # of bytes used to compress map.
int		FL_CompressOverworld
(
	unsigned char *rom,				// [OUT] Pointer to 256K Faria ROM, no iNES header.
	const unsigned char *buffer,	// [IN] Pointer to 16K overworld (128x128x1)
	bool bTestOnly					// [IN] If true, don't modify ROM, just check boundries.
);

void	FL_DecompressSkyworld
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image.
	unsigned char *buffer		// [OUT] Pointer to 1K (64x64 @ 4 tiles / byte).
);

void	FL_DecompressCaveworld
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image.
	unsigned char *buffer		// [OUT] Pointer to 1600 bytes (80x80 @ 4 tiles / byte).
);

void	FL_DecompressTown
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image, no iNES header.
	int	town,					// [IN] 0-10 (10 towns + castle).
	unsigned char *buffer,		// [OUT] Pointer to large enough buffer
	int *width,					// [OUT] Width (in tiles) of town.
	int *height,				// [OUT] Height (in tiles) of town.
	int *rle_bytes				// [OUT] # of bytes of RLE data used.
);


#endif // __FARIA_LIB_H__