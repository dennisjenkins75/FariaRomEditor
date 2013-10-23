// This code was generated from the hacking tool.

#include "FariaLib.h"

const TCHAR *g_szFariaListboxStrings[] =
{
	TEXT("KNIFE "),		// 00: $b6e1
	TEXT("DIRK "),		// 01: $b6e7
	TEXT("IRON S "),		// 02: $b6ec
	TEXT("STEEL S "),		// 03: $b6f3
	TEXT(""),				// 04: $b6fb
	TEXT("FIRE S "),		// 05: $b704
	TEXT("MURAMASA "),	// 06: $b70b
	TEXT("LEGEND S "),	// 07: $b714
	TEXT("PAPER S "),		// 08: $b71d
	TEXT(" "),			// 09: $b725
	TEXT("WOOD B "),		// 0a: $b726
	TEXT("BAMBOO B "),	// 0b: $b72d
	TEXT("IRON B "),		// 0c: $b736
	TEXT("STEEL B "),		// 0d: $b73d
	TEXT("CROSS B "),		// 0e: $b745
	TEXT(" "),			// 0f: $b74d
	TEXT("TUNIC "),		// 10: $b74e
	TEXT("PELT A "),		// 11: $b754
	TEXT("CHAIN A "),		// 12: $b75b
	TEXT("IRON A "),		// 13: $b763
	TEXT("STEEL A "),		// 14: $b76a
	TEXT("SUPER A "),		// 15: $b772
	TEXT("WOOD SD "),		// 16: $b77a
	TEXT("PELT SD "),		// 17: $b782
	TEXT("IRON SD "),		// 18: $b78a
	TEXT("STEEL SD "),	// 19: $b792
	TEXT("MAGIC SEDE"),	// 1a: $b79b
	TEXT("MAGIC SABA"),	// 1b: $b7a5
	TEXT("30 ARROWS"),	// 1c: $b7af
	TEXT("50 ARROWS"),	// 1d: $b7b8
	TEXT("80 ARROWS"),	// 1e: $b7c1
	TEXT("250 ARROWS"),	// 1f: $b7ca
	TEXT("BALM"),			// 20: $b7d4
	TEXT("SALVE"),		// 21: $b7d8
	TEXT("POULTICE"),		// 22: $b7dd
	TEXT("ELIXIR"),		// 23: $b7e5
	TEXT("PEARL"),		// 24: $b7eb
	TEXT("GARNET"),		// 25: $b7f0
	TEXT("EMERALD"),		// 26: $b7f6
	TEXT(""),				// 27: $b7fd
	TEXT(""),				// 28: $b804
	TEXT(""),				// 29: $b804
	TEXT(""),				// 2a: $b804
	TEXT(""),				// 2b: $b804
	TEXT(""),				// 2c: $b804
	TEXT(""),				// 2d: $b804
	TEXT("BATTERY"),		// 2e: $b804
	TEXT("LIGHT"),		// 2f: $b80b
	TEXT("10 SEDE"),		// 30: $b810
	TEXT("20 SEDE"),		// 31: $b817
	TEXT("40 SEDE"),		// 32: $b81e
	TEXT("10 SABA"),		// 33: $b825
	TEXT("20 SABA"),		// 34: $b82c
	TEXT("40 SABA"),		// 35: $b833
	TEXT(""),		// 36: $b83a
	TEXT(""),		// 37: $b83a
	TEXT(""),		// 38: $b83a
	TEXT(""),		// 39: $b83a
	TEXT(""),		// 3a: $b83a
	TEXT(""),		// 3b: $b83a
	TEXT(""),		// 3c: $b83a
	TEXT(""),		// 3d: $b83a
	TEXT(""),		// 3e: $b83a
	TEXT(""),		// 3f: $b83a
	TEXT(""),		// 40: $b83a
	TEXT(""),		// 41: $b83a
	TEXT("R ANTIDOTE"),		// 42: $b83a
	TEXT("B ANTIDOTE"),		// 43: $b844
	TEXT("W ANTIDOTE"),		// 44: $b84e
	TEXT("U ANTIDOTE"),		// 45: $b858
	TEXT("PARAPO"),		// 46: $b862
	TEXT(""),		// 47: $b868
	TEXT(""),		// 48: $b868
	TEXT("FLASH BALL"),		// 49: $b868
	TEXT("SHADOW"),		// 4a: $b872
	TEXT(""),		// 4b: $b878
	TEXT("20 BOMBS"),		// 4c: $b878
	TEXT("40 BOMBS"),		// 4d: $b880
	TEXT("60 BOMBS"),		// 4e: $b888
	TEXT("250 BOMBS"),		// 4f: $b890
	TEXT(""),		// 50: $b899
	TEXT("HYPERSPEED1"),		// 51: $b899
	TEXT("HYPERSPEED2"),		// 52: $b8a4
	TEXT("HYPERSPEED3"),		// 53: $b8af
	TEXT("WINGS"),		// 54: $b8ba
	TEXT("JUMP SHOES"),		// 55: $b8bf
	TEXT(""),		// 56: $b8c9
	TEXT("LETTER"),		// 57: $b8c9
	TEXT("CAPSULE"),		// 58: $b8cf
	TEXT("MAGIC GLASS"),		// 59: $b8d6
	TEXT("MAGIC ROPE"),		// 5a: $b8e1
	TEXT("GOLD ARROW"),		// 5b: $b8eb
	TEXT("SKY SHOES"),		// 5c: $b8f5
	TEXT(""),		// 5d: $b8fe
	TEXT("RING"),		// 5e: $b909
	TEXT("THE CRYSTAL"),		// 5f: $b90d
	TEXT("GOLD STONE"),		// 60: $b918
	TEXT("SEED"),		// 61: $b922
	NULL
};

// Strings found at "176f0" (iNES format) ROM image.
// Converted from Faria char set to ASCII and Lowercased.
// Starting locations are from some Inn in that town.

const TOWN_INFO g_TownInfo[] =
{
	{ TEXT("Ehdo"), 0x05, 0x0c },
	{ TEXT("Somusa"), 0, 0 },
	{ TEXT("Karuza"), 0, 0 },
	{ TEXT("Highria"), 0, 0 },
	{ TEXT("Riria"), 0, 0 },
	{ TEXT("Teodoor"), 0x0e, 0x11 },
	{ TEXT("Tegza"), 0, 0 },		// No Inn.
	{ TEXT("Shilf"), 0, 0 },		// No Inn.
	{ TEXT("Zellia"), 0, 0 },		// No Inn.
	{ TEXT("Baig"), 0, 0 },
	{ TEXT("Castle"), 0, 0},	// Fake entry
	{ NULL, 0, 0 }
};



// The items that are actually swords, bows, etc...
const int g_iValidSwords[] =
{
	0, 1, 2, 3, 5, 6, 7, 8, -1
};

const int g_iValidBows[] =
{
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, -1
};

const int g_iValidArmours[] =
{
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, -1
};

const int g_iValidSheilds[] =
{
	0x16, 0x17, 0x18, 0x19, -1
};


