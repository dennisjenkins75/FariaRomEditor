/* This file contains various "hacks" used to extract data from
   the FARIA.NES file. */

#include <stdio.h>
#include <sys/stat.h>
#include "../FariaLib/FariaLib.h"

static unsigned char ines[256 * 1024 + 16];
static unsigned char *rom = ines + 16;

static unsigned char cdl_0[128 * 1024];
static unsigned char cdl_1[128 * 1024];

static unsigned char overwold_palette[32] =
{
	0x0F, 0x29, 0x11, 0x21, 0x0F, 0x29, 0x19, 0x37,
	0x0F, 0x20, 0x11, 0x21, 0x0F, 0x30, 0x21, 0x10,
	0x00, 0x36, 0x0F, 0x16, 0x00, 0x36, 0x0F, 0x16,
	0x00, 0x36, 0x01, 0x30, 0x00, 0x36, 0x0F, 0x13
};

static void	LoadFile(unsigned char *buff, int expected_size, const char *filename)
{
	int bytes;
	struct stat statbuf;
	FILE *fp;

	if (NULL == (fp = fopen(filename, "rb")))
	{
		fprintf(stderr, "Failed to open '%s' for reading.\n", filename);
		perror("fopen");
		exit(-1);
	}

	if (-1 == fstat(fileno(fp), &statbuf))
	{
		fprintf(stderr, "Failed to stat '%s'.\n", filename);
		perror("fstat");
		exit(-1);
	}

	if (statbuf.st_size != expected_size)
	{
		fprintf(stderr, "'%s' is wrong size (%d), expected %d.\n", filename, statbuf.st_size, expected_size);
		exit(-1);
	}

	if (expected_size != (bytes = fread(buff, 1, expected_size, fp)))
	{
		fprintf(stderr, "Only read %d of %d bytes.\n", bytes, expected_size);
		perror("fread");
		exit(-1);
	}

	fclose(fp);
}

static void	SaveFile(const unsigned char *buff, int expected_size, const char *filename)
{
	int bytes;
	FILE *fp;

	if (NULL == (fp = fopen(filename, "wb")))
	{
		fprintf(stderr, "Failed to open '%s' for writing.\n", filename);
		perror("fopen");
		exit(-1);
	}

	if (expected_size != (bytes = fwrite(buff, 1, expected_size, fp)))
	{
		fprintf(stderr, "Only wrote %d of %d bytes.\n", bytes, expected_size);
		perror("fread");
		exit(-1);
	}

	fclose(fp);
}


/*
$B0A8:A0 00     LDY #$00
$B0AA:A2 50     LDX #$50
$B0AC:B9 49 B8  LDA $B849,Y @ $B85E = #$5E
$B0AF:C5 C1     CMP $C1 = #$38
$B0B1:D0 07     BNE $B0BA
$B0B3:B9 99 B8  LDA $B899,Y @ $B8AE = #$40
$B0B6:C5 C2     CMP $C2 = #$7C
$B0B8:F0 05     BEQ $B0BF
$B0BA:C8        INY
$B0BB:CA        DEX
$B0BC:D0 EE     BNE $B0AC
$B0BE:60        RTS
*/

// Above code and table are mapped to "e000-ffff" and exist in ROM at same location.
// $c1 is player X coord on overworld.  $c2 is player y coord.
static void	Dump_b849_table(void)
{
	int x, y, i;

	for (i = 0; i < 0x50; i++)
	{
		x = rom[0xf849 + i];
		y = rom[0xf899 + i];

		printf("%02x, %02x\n", x, y);
	}
}

static unsigned char ucOverworldMapTiles[128*128];

// Tile #, y axis, x axis = NES palette entry.
static unsigned char ucPPU[256][8][8];

static void	ExtractPPUTiles(void)
{
	int x, y, t;

	const unsigned char *char_ptr = rom + 0x27000;
	
	memset(ucPPU, 0, sizeof(ucPPU));

	for (t = 0; t < 256; t++)
	{
		for (y = 0; y < 8; y++)
		{
			for (x = 0; x < 8; x++)
			{
				int mask = (1 << (7-x));
				int bit0 = (char_ptr[t * 16 + y] & mask) ? 1 : 0 ;
				int bit1 = (char_ptr[t * 16 + 8 + y] & mask) ? 2 : 0 ;

				ucPPU[t][y][x] = bit0 | bit1;
			}
		}
	}
}

// y axis, x axis = NES palette entry.
// map is 128 x 128 "map tiles".  Each map tile is "2x2" PPU tiles.
static unsigned char ucMainBitmap[128 * 16][128 * 16];

static void	GenerateMainBitmap(void)
{
	int wx, wy, tx, ty, px, py;
	unsigned char ot, pt;

	memset(ucMainBitmap, 0, sizeof(ucMainBitmap));

	for (wy = 0; wy < 128; wy++)
	{
		for (wx = 0; wx < 128; wx++)
		{
			ot = ucOverworldMapTiles[wy * 128 + wx];

			for (ty = 0; ty < 2; ty++)
			{
				for (tx = 0; tx < 2; tx++)
				{
					pt = rom[0x95d4 + ot * 4 + ty * 2 + tx];

					for (py = 0; py < 8; py++)
					{
						for (px = 0; px < 8; px++)
						{
							ucMainBitmap[wy * 16 + ty * 8 + py][wx * 16 + tx * 8 + px] = ucPPU[pt][py][px];
						}
					}
				}
			}
		}
	}
}

static void	HackOverworld(void)
{
	memset(ucOverworldMapTiles, 0x5f, sizeof(ucOverworldMapTiles));

	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			ucOverworldMapTiles[y*128+x] = (y<<4) | x;
		}
	}

	FL_CompressOverworld(rom, ucOverworldMapTiles, false);
}


struct pBITMAPINFO
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD         bmiColors[256];
};

struct pLOGPALETTE
{
	WORD            palVersion;
	WORD            palNumEntries;
	PALETTEENTRY    palPalEntry[256];
};

// Creates a win32 bitmap of the same dimensions as the NES bitmap.

static HBITMAP	CreateNesBitmap
(
	HDC hDC, 
	int width, 
	int height,
	const unsigned char *pSrcBitmap,
	const NES_PAL *pNesPalette
)
{
	HBITMAP hBitmap = NULL;
	int             i;
    pBITMAPINFO BMInfo;
	pLOGPALETTE PalInfo;

	width = ((width + 7) / 8) * 8;

	memset(&BMInfo, 0, sizeof(BMInfo));
	memset(&PalInfo, 0, sizeof(PalInfo));

	BMInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BMInfo.bmiHeader.biWidth = width;
	BMInfo.bmiHeader.biHeight = -abs(height);
	BMInfo.bmiHeader.biPlanes = 1;
	BMInfo.bmiHeader.biBitCount = 8;
	BMInfo.bmiHeader.biCompression = BI_RGB;
	BMInfo.bmiHeader.biSizeImage = 0;
	BMInfo.bmiHeader.biXPelsPerMeter = 0;
	BMInfo.bmiHeader.biYPelsPerMeter = 0;
	BMInfo.bmiHeader.biClrUsed = 64;
	BMInfo.bmiHeader.biClrImportant = 64;

	for (i = 0; i < 64; i++)
	{
		BMInfo.bmiColors[i].rgbRed = nes_palette[i].r;
		BMInfo.bmiColors[i].rgbGreen = nes_palette[i].g;
		BMInfo.bmiColors[i].rgbBlue = nes_palette[i].b;
	}

	SetDIBColorTable(hDC, 0, 64, BMInfo.bmiColors);

	PalInfo.palVersion = 0x300;
	PalInfo.palNumEntries = 64;
	for (i = 0; i < 64; i++)
	{
		PalInfo.palPalEntry[i].peRed = BMInfo.bmiColors[i].rgbRed;
		PalInfo.palPalEntry[i].peGreen = BMInfo.bmiColors[i].rgbGreen;
		PalInfo.palPalEntry[i].peBlue = BMInfo.bmiColors[i].rgbBlue;
		PalInfo.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
	}

	HPALETTE        PalHan = CreatePalette((LOGPALETTE *) & PalInfo);

	SelectPalette(hDC, PalHan, FALSE);
	RealizePalette(hDC);
	/* FIXME: We must free the palette handle, but when? */
	//  DeleteObject(PalHan);

	unsigned char *double_buffer = NULL;

	hBitmap = CreateDIBSection(hDC, (BITMAPINFO *) & BMInfo, DIB_RGB_COLORS, (void **) &double_buffer, NULL, 0);

	memcpy(double_buffer, pSrcBitmap, width * height);

	return hBitmap;
}


static void	Dump_Overworld(void)
{
	ExtractPPUTiles();

	FL_DecompressOverworld(ines + 16, ucOverworldMapTiles);

	HackOverworld();

	GenerateMainBitmap();

	HDC hDC = GetDC(NULL);
	HBITMAP	hBitmap = CreateNesBitmap(hDC, 128*16, 128*16, (unsigned char*)ucMainBitmap, nes_palette);

	DumpBitmap(hDC, hBitmap, "faria.bmp");

	DeleteObject(hBitmap);

	ReleaseDC(NULL, hDC);
}

/*
	CDL log files are just a mask of the PRG-ROM; that is, they are the
	same size as the PRG-ROM, and each byte represents the corresponding
	byte of the PRG-ROM.
	The format of each byte is like so (in binary):

		xPdcAADC

		C  = Whether it was accessed as code.
		D  = Whether it was accessed as data.
		AA = Into which ROM bank it was mapped when last accessed:
			00 = $8000-$9FFF	01 = $A000-$BFFF
			10 = $C000-$DFFF	11 = $E000-$FFFF
		c  = Whether indirectly accessed as code.
			(e.g. as the destination of a JMP ($nnnn) instruction)
		d  = Whether indirectly accessed as data.
			(e.g. as the destination of an LDA ($nn),Y instruction)
		P  = If logged as PCM audio data.
		x  = unused.
*/
// Dumps list of memory locations that have been accessed in CDL1 and not CDL0
static void CdlDiff(void)
{
	int i;
	int cpu_loc;
	int bank;
	char c,d;

	for (i = 0; i < sizeof(cdl_0); i++)
	{
		if ((!(cdl_0[i] & 0x02) && (cdl_1[i] & 0x02)) || 
		    (!(cdl_0[i] & 0x01) && (cdl_1[i] & 0x01)))
		{
			bank = ((cdl_1[i] >> 2) & 0x03);
			c = cdl_1[i] & 0x01 ? 'c' : ' ';
			d = cdl_1[i] & 0x02 ? 'd' : ' ';
			cpu_loc = (i & 0x1fff) + 0x8000 + bank * 0x2000;
			printf("%c%c: %06x, %06x, %04x\n", c, d, i, i + 16, cpu_loc);
		}
	}
}

static void	ExtractListBoxStrings(void)
{
	int bank_offset = 0xc000;	// CPU b61b maps to ROM 1761b.
	unsigned char *cpu_b61b = rom + bank_offset + 0xb61b;
	unsigned char *cpu_b67e = rom + bank_offset + 0xb67e;
	char temp[258];

	printf("const char *g_szFariaListboxStrings[] =\n{\n");

	for (int idx = 0; idx <= 0x61; idx++)
	{
		int lsb = cpu_b61b[idx];
		int msb = cpu_b67e[idx];
		int len = cpu_b61b[idx+1] - lsb;

		unsigned char *str = rom + bank_offset + lsb + (msb << 8);

		FariaGetString(temp, sizeof(temp), str, len);

		printf("\t\"%s\",\t\t// %02x: $%04x\n", temp, idx, lsb + (msb << 8));
	}

	printf("\tNULL\n};\n\n");
}

static void	EnumClipboardFormats(void)
{
	int i = 0;
	char name[128];

	OpenClipboard(NULL);

//	int f = RegisterClipboardFormat(TEXT("djenkins"));
//	printf("New format = %d\n", f);

	do
	{
		if (0 != (i = EnumClipboardFormats(i)))
		{
			if (GetClipboardFormatName(i, name, sizeof(name)))
			{
				printf("%5d: %s\n", i, name);
			}
			else
			{
				printf("%5d: <failed to get name>\n", i);
			}
		}
	} while (i);

	CloseClipboard();
}

static void	Dump_Floor(int addr)
{
	int x, y;
	unsigned char *ptr = rom + addr;

	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			printf("%02x%02x ", ptr[0], ptr[1]);
			ptr++;
			ptr++;
		}
		printf("\n");
	}
	printf("\n");
}

static int get_addr(const char *str)
{
	int i = 0x7fffffff;

	if (*str == '$') str++;
	sscanf(str, "%x", &i);
	return i;
}

int		main(int argc, char *argv[])
{
	char *filename = NULL;
	char *cdl0 = NULL;
	char *cdl1 = NULL;
	int addr = 0;
	int i;

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-dump-item-list"))
		{
			filename = argv[++i];
			LoadFile(ines, sizeof(ines), filename);
			ExtractListBoxStrings();
			return 0;
		}
		else if (!strcmp(argv[i], "-dump-b849-table"))
		{
			filename = argv[++i];
			LoadFile(ines, sizeof(ines), filename);
			Dump_b849_table();
			return 0;
		}
		else if (!strcmp(argv[i], "-dump-overworld"))
		{
			filename = argv[++i];
			LoadFile(ines, sizeof(ines), filename);
			Dump_Overworld();
			return 0;
		}
		else if (!strcmp(argv[i], "-cdl-diff"))
		{
			cdl0 = argv[++i];
			cdl1 = argv[++i];

			LoadFile(cdl_0, sizeof(cdl_0), cdl0);
			LoadFile(cdl_1, sizeof(cdl_1), cdl1);

			CdlDiff();
			return 0;
		}
		else if (!strcmp(argv[i], "-enum-clipboard-formats"))
		{
			(void)EnumClipboardFormats();
			return 0;
		}
		else if (!strcmp(argv[i], "-dump-floor"))
		{
			filename = argv[++i];
			addr = get_addr(argv[++i]);
			LoadFile(ines, sizeof(ines), filename);
			Dump_Floor(addr);
			return 0;
		}
	}
	

	return 0;
}