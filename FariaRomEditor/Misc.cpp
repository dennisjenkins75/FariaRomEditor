/* Misc.cpp */

#include "FariaRomEditor.h"

void	NormalizeRect(RECT *rect)
{
	int temp;

	if (rect->top > rect->bottom)
	{
		temp = rect->top;
		rect->top = rect->bottom;
		rect->bottom = temp;
	}

	if (rect->left > rect->right)
	{
		temp = rect->left;
		rect->left = rect->right;
		rect->right = temp;
	}
}

bool	IsTileCaveEntrence(int col, int row, int &cave)
{
	_ASSERT (g_pRom);

	int i, x, y;

	for (i = 0; i < 0x50; i++)
	{
		x = g_pRom[0xf849 + i];
		y = g_pRom[0xf899 + i];

		if ((x == col) && (y == row))
		{
			cave = i;
			return true;
		}
	}

	return false;
}

bool	IsTileCaveExit(int col, int row, int &cave)
{
	_ASSERT (g_pRom);

	int i, x, y;

	for (i = 0; i < 0x50; i++)
	{
		x = g_pRom[0x5902 + i];
		y = g_pRom[0x5952 + i];

		if ((x == col) && (y == row))
		{
			cave = i;
			return true;
		}
	}

	return false;
}


bool	IsTileATown(int col, int row, int &town)
{
	_ASSERT (g_pRom);

	int i, x, y;

	for (i = 0; i < TOWN_COUNT; i++)
	{
		x = g_pRom[0x989e + i];
		y = g_pRom[0x98a8 + i];

		if ((x == col) && (y == row))
		{
			town = i;
			return true;
		}
	}

	return false;
}