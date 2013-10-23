/* FariaLib/OverWorld.cpp */

#include "FariaLib.h"

// List of tiles that are compressible in runs of 2-17.
static unsigned char rle_16[3] = { 0x00, 0x52, 0x5f };

// Tiles that are compressible in runs of 2-9 are in ROM at offset 0x1cbe3 (10 of them).
static unsigned char rle_8[10] = { 0x62, 0x07, 0x51, 0x59, 0x67, 0x6a, 0x21, 0x4e, 0x5e, 0x4b };


// Decompresses the RLE-encoded overworld map from the ROM into a 16K buffer.
// Map tiles that do not decompress are "00" (ocean).
// If the compressed data would overflow the buffer, it is silently truncated.

void	FL_DecompressOverworld
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image, no iNES header.
	unsigned char *buffer		// [OUT] Pointer to 16K (128x128 @1byte/tile).
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

	memcpy(rle_8, rom + 0x1cbe3, 10);

	memset(buffer, 0, 128 * 128 * sizeof(unsigned char));
	for (y = 0; y < 128; y++)
	{
		lsb = rom[0x1d70d + y];
		msb = rom[0x1d68d + y] - 0x80;

		rle_data = rom + (msb << 8) + lsb;

		for (x = 0; x < 128; rle_data++)
		{
			if (*rle_data < 0x80)
			{
				buffer[y * 128 + x] = *rle_data;
				x++;
			}
			else if (*rle_data < 0xd0)
			{
				count = (*rle_data & 0x07) + 2;
				tile_idx = (*rle_data >> 3) & 0x0f;
				while (count && (x < 128))
				{
					buffer[y * 128 + x] = rle_8[tile_idx];
					count--;
					x++;
				}
			}
			else
			{
				count = (*rle_data & 0x0f) + 2;
				tile_idx = ((*rle_data >> 4) & 0x03) - 1;
				while (count && (x < 128))
				{
					buffer[y * 128 + x] = rle_16[tile_idx];
					count--;
					x++;
				}
			}
		}
	}
}

#if 0
#define TEST() if (rom[rle_idx-1] != ow_rle[rle_idx-1]) { TCHAR temp[128]; \
	wsprintf(temp, TEXT("%d, %d, %d, %02x, %02x\n"), x, y, rle_idx, rom[rle_idx-1], ow_rle[rle_idx-1]); \
	OutputDebugString(temp); }
#else
#define TEST()
#endif


// Returns # of bytes used to compress world.  Caller should test to see if it
// is out of bounds.  If too big, function will NOT modify the ROM image.
int		FL_CompressOverworld
(
	unsigned char *rom,				// [OUT] Pointer to 256K Faria ROM, no iNES header.
	const unsigned char *buffer,	// [IN] Pointer to 16K overworld (128x128x1)
	bool bTestOnly					// [IN] If true, don't modify ROM, just check boundries.
)
{
	int x, y, rle_idx, count, i, run_len;
	unsigned char *ow_rle = new unsigned char [128 * 128];	// enough space for 0 compression.
	unsigned char *cols = new unsigned char [128 * 2];
	unsigned char tile = 0;
	unsigned char max_rle[128];

	memset(ow_rle, 0, 128 * 128);	// Wipe out the RLE region.
	memset(cols, 0, 256);			// Wipe out the column pointer table.

	// Create reverse map of RLE codes for faster lookup.
	memset(max_rle, 0, sizeof(max_rle));
	for (x = 0; x < sizeof(rle_8); max_rle[rle_8[x++]] |= 8);
	for (x = 0; x < sizeof(rle_16); max_rle[rle_16[x++]] |= 16);

	rle_idx = 0;	// current index into "ow_rle" (over-world).

	for (y = 0; y < 128; y++)
	{
		// Set LSB and MSB pointers in column pointer table.
		cols[y] = (rle_idx >> 8) + 0x80;	// set MSB
		cols[y+128] = (rle_idx & 0xff);		// set LSB

		for (x = 0; x < 128; )
		{
			// Determine if current tile is compressible and how many we have.
			tile = buffer[y * 128 + x];
			for (count = 0; ((count+x) < 128) && (tile == buffer[y * 128 + x + count]); count++);

			if ((max_rle[tile] & 16) && (count > 1))
			{
				// Locate index into "rle_16" for the tile.
				for (i = 0; (i < sizeof(rle_16)) && (rle_16[i] != tile); i++);
				_ASSERT(i < sizeof(rle_16));

				while (count > 1)
				{
					run_len = (count > 17) ? 17 : count;
					ow_rle[rle_idx++] = 0xd0 + (i << 4) + (run_len - 2);
					TEST();
					count -= run_len;
					x += run_len;
				}

				// we might have 0 or 1 of the tile left over.
				_ASSERT((count == 0) || (count == 1));
				if (count)
				{
					ow_rle[rle_idx++] = tile;
					TEST();
					x++;
				}
			}
			else if ((max_rle[tile] & 8) && (count > 1))
			{
				// Locate index into "rle_8" for the tile.
				for (i = 0; (i < sizeof(rle_8)) && (rle_8[i] != tile); i++);
				_ASSERT(i < sizeof(rle_8));

				while (count > 1)
				{
					run_len = (count > 9) ? 9 : count;
					ow_rle[rle_idx++] = 0x80 + (i << 3) + (run_len - 2);
					TEST();
					count -= run_len;
					x += run_len;
				}

				// we might have 0 or 1 of the tile left over.
				_ASSERT((count == 0) || (count == 1));
				if (count)
				{
					ow_rle[rle_idx++] = tile;
					TEST();
					x++;
				}
			}
			else
			{
				ow_rle[rle_idx++] = tile;
				TEST();
				x++;
			}
		}
	}

	if (!bTestOnly && (rle_idx < OVERWORLD_RLE_MAX_LEN))
	{
		memcpy(rom, ow_rle, OVERWORLD_RLE_MAX_LEN);
		memcpy(rom + 0x1d68d, cols, 256);
	}

	delete [] ow_rle;
	delete [] cols;

	return rle_idx;
}