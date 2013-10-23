/* TownCompression.cpp */

#include "FariaLib.h"

/*
		int w = g_iNES[0xb63d + i] + 1;
		int h = g_iNES[0xb64a + i] + 1;

		int zp_c8 = g_iNES[0xd799 + i + 0x10010];	// CPU addr, Y reg, iNES offset
		int zp_c9 = g_iNES[0xd78d + i + 0x10010];

		int zp_ca = g_iNES[0xd7b1 + i + 0x10010];
		int zp_cb = g_iNES[0xd7a5 + i + 0x10010];

		int ptr_c8 = zp_c8 + (zp_c9 << 8);
		int ptr_ca = zp_ca + (zp_cb << 8);

		int msb = g_iNES[ptr_c8 + 0x10010];
		int lsb = g_iNES[ptr_ca + 0x10010];

		int addr = lsb + (msb << 8) - 0x3ff0;
*/


// Decompresses the RLE-encoded overworld map from the ROM into a 16K buffer.
// Map tiles that do not decompress are "00" (ocean).
// If the compressed data would overflow the buffer, it is silently truncated.

void	FL_DecompressTown
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image, no iNES header.
	int	town,					// [IN] 0-10 (10 towns + castle).
	unsigned char *buffer,		// [OUT] Pointer to large enough buffer
	int *width,					// [OUT] Width (in tiles) of town.
	int *height,				// [OUT] Height (in tiles) of town.
	int *rle_bytes				// [OUT] # of bytes of RLE data used.
)
{
	int x, y;
	int lsb = 0;
	int msb = 0;
	int tile = 0;
	int count = 0;
	int tile_idx = 0;
	const unsigned char *rle_data = NULL;

	_ASSERT(rom);
	_ASSERT(buffer);
	_ASSERT(width);
	_ASSERT(height);
	_ASSERT(rle_bytes);
	_ASSERT((town >= 0) && (town < TOWN_COUNT+1));

	*width = rom[0xb62d + town] + 1;
	*height = rom[0xb63a + town] + 1;
	*rle_bytes = 0;

	int zp_c8 = rom[0x1d799 + town];	// CPU addr, Y reg
	int zp_c9 = rom[0x1d78d + town];

	int zp_ca = rom[0x1d7b1 + town];
	int zp_cb = rom[0x1d7a5 + town];

	int ptr_c8 = zp_c8 + (zp_c9 << 8) + 0x10000;
	int ptr_ca = zp_ca + (zp_cb << 8) + 0x10000;

	const unsigned char *town_rle = rom + 0x1cbed;

	for (y = 0; y < *height; y++)
	{
		rle_data = rom + (rom[ptr_c8 + y] << 8) + rom[ptr_ca + y] - 0x4000;

		for (x = 0; x < *width; rle_data++, (*rle_bytes)++)
		{
			if (*rle_data < 0x80)
			{
				buffer[y * *width + x] = *rle_data;
				x++;
			}
			else
			{
				count = (*rle_data & 0x07) + 2;
				tile = (*rle_data >> 3) & 0x0f;

				while (count && (x < *width))
				{
					buffer[y * *width + x] = town_rle[tile];
					count--;
					x++;
				}
			}
		}
	}
}
