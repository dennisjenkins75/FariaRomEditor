/* Skyworld.cpp */

static const int SKY_WORLD_WIDTH = 64;
static const int SKY_WORLD_HEIGHT = 64;

void	FL_DecompressSkyworld
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image.
	unsigned char *buffer		// [OUT] Pointer to 4K (64x64 @1byte/tile).
)
{
	const unsigned char *pSrc = rom + 0xf8e9;
	unsigned char *pDest = buffer;
	int i;

	// One byte of SRC becomes 4 tiles.  We just hack out the bits,
	// in the form aabbccdd for the 4 tiles.
	for (i = 0; i < SKY_WORLD_HEIGHT * SKY_WORLD_WIDTH / 4; i++)
	{
		*(pDest++) = (*pSrc >> 6) & 0x03;
		*(pDest++) = (*pSrc >> 4) & 0x03;
		*(pDest++) = (*pSrc >> 2) & 0x03;
		*(pDest++) = (*pSrc >> 0) & 0x03;
		pSrc++;
	}
}

static const int CAVE_WORLD_WIDTH = 80;
static const int CAVE_WORLD_HEIGHT = 80;

void	FL_DecompressCaveworld
(
	const unsigned char *rom,	// [IN] Pointer to 256K Faria ROM image.
	unsigned char *buffer		// [OUT] Pointer to 1600 bytes (80x80 @ 4 tiles / byte).
)
{
	const unsigned char *pSrc = rom + 0x59a2;
	unsigned char *pDest = buffer;
	int i;

	// One byte of SRC becomes 4 tiles.  We just hack out the bits,
	// in the form aabbccdd for the 4 tiles.
	for (i = 0; i < CAVE_WORLD_HEIGHT * CAVE_WORLD_WIDTH / 4; i++)
	{
		*(pDest++) = (*pSrc >> 6) & 0x03;
		*(pDest++) = (*pSrc >> 4) & 0x03;
		*(pDest++) = (*pSrc >> 2) & 0x03;
		*(pDest++) = (*pSrc >> 0) & 0x03;
		pSrc++;
	}
}
