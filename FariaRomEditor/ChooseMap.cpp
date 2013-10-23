/* ChooseTown.cpp */

#include "FariaRomEditor.h"

// Columns are:
/*
	Name, dimensions, palette entries, RLE size, ROM addr?

*/

struct LISTVIEW_COLUMN 
{
	TCHAR *szTitle;
	int nWidth;
};

struct LISTVIEW_COLUMN MasterColumns[] = 
{
	{ TEXT("Name"),			100 },
	{ TEXT("Size"),			100 },
	{ TEXT("ROM addr"),		100 },
	{ NULL,					0 }
};

static DWORD MASTER_COLUMN_COUNT = sizeof(MasterColumns) / sizeof(LISTVIEW_COLUMN) - 1;

static const int SUBITEM_NAME = 0;
static const int SUBITEM_SIZE = 1;
static const int SUBITEM_ADDR = 2;

static void	AddMapToList(HWND hList, TCHAR *szName, int w, int h, int addr)
{
	LVITEM lvi;
	TCHAR temp[256];
	int result;

	lvi.mask = LVIF_TEXT;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.iItem = 999;			// Crappy way of appending to end of list.
	lvi.iSubItem = SUBITEM_NAME;
	lvi.iImage = NULL;
	lvi.pszText = szName;
	lvi.lParam = NULL;
	result = SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&lvi);

	wnsprintf(temp, TCHAR_sizeof(temp), TEXT("%d x %d"), w, h);
	lvi.iItem = result;
	lvi.iSubItem = SUBITEM_SIZE;
	lvi.pszText = temp;
	SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&lvi);

	wnsprintf(temp, TCHAR_sizeof(temp), TEXT("$%05x"), addr);
	lvi.iSubItem = SUBITEM_ADDR;
	lvi.pszText = temp;
	SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&lvi);
}

static void	PopulateMapList(HWND hList)
{
	LVCOLUMN lvc;
	int i;

	ListView_SetExtendedListViewStyleEx( hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
	lvc.fmt = LVCFMT_LEFT;

	for (i = 0; MasterColumns[i].szTitle; i++)
	{
		lvc.iSubItem = i;
		lvc.cx = MasterColumns[i].nWidth;
		lvc.pszText = (TCHAR*)MasterColumns[i].szTitle;

		if (ListView_InsertColumn(hList, lvc.iSubItem, &lvc) == -1)
		{
			FatalError(GetLastError(), TEXT("ListView_InsertColumn(%d) failed."), i);
		}
	}

	AddMapToList(hList, TEXT("Over World"), OW_MAP_WIDTH, OW_MAP_HEIGHT, 0);
	AddMapToList(hList, TEXT("Sky World"), SKY_MAP_WIDTH, SKY_MAP_HEIGHT, 0xf8e9);
	AddMapToList(hList, TEXT("Under World"), CAVE_MAP_WIDTH, CAVE_MAP_HEIGHT, 0x59a2);

	// Add one for the King's castle.
	for (i = 0; i < TOWN_COUNT + 1; i++)
	{
		TCHAR *szName = g_TownInfo[i].szName;

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

		AddMapToList(hList, szName, w, h, addr);

/*
($c8) is loaded from $d799+Y (lsb), $d78d+Y (msb)
($ca) is loaded from $d7b1+Y (lsb), $d7a5+Y (msb).

$d799: bd fd 2d 5d 85 ad d5 05 45 65 is at $1d7a9 in iNES file.

For Ehdo (town == 0), ($c8) = d7bd, ($ca) = d7dd
($c8 + XX) is stored at $c7 (LSB)
($c9 + XX) is stored at $c6 (MSB)

So ($c6) points to $8751, which is the RLE data for Ehdo.
code then banks switches RLE tiles into address space and
decompresses them at $c1fe
*/


	}
}

// Search listbox to see if an item is highlighted.  If so,
// open a map editor for it.
static bool DoOk(HWND hWnd)
{
	HWND hList = GetDlgItem(hWnd, IDC_MAP_LIST);
	int nCount = ListView_GetItemCount(hList);
	int i;
	bool bReturn = false;

	for (i = 0; i < nCount; i++)
	{
		if (LVIS_SELECTED & ListView_GetItemState(hList, i, LVIS_SELECTED ))
		{
			// First 3 maps are not towns.
			switch (i)
			{
				case 0:
					EditMap_Overworld();
					break;

				case 1:
					EditMap_Skyworld();
					break;

				case 2:
					EditMap_Caveworld();
					break;

				default:
					EditMap_Town(i - 3);
					break;
			}

			bReturn = true;
		}
	}

	if (!bReturn)
	{
		MessageBox(hWnd, TEXT("Please select a map to edit, then click \"Ok\"."), TEXT("Error"), MB_OK);
	}

	return bReturn;
}

static void DoSetSelection(HWND hWnd, bool bSetAll)
{
	HWND hList = GetDlgItem(hWnd, IDC_MAP_LIST);
	int nCount = ListView_GetItemCount(hList);
	int i;
	int bits = bSetAll ? LVIS_SELECTED : 0;

	for (i = 0; i < nCount; i++)
	{
		ListView_SetItemState(hList, i, bits, LVIS_SELECTED);
	}

}

INT_PTR CALLBACK ChooseMapDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetWindowStandards(hWnd, TEXT("Faria Editor - Choose Town"));
			PopulateMapList(GetDlgItem(hWnd, IDC_MAP_LIST));
			return 1;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					if (DoOk(hWnd))
					{
						EndDialog(hWnd, 1);
					}
					return 0;

				case IDCANCEL:
					EndDialog(hWnd, 0);
					return 0;

				case CM_SELECT_ALL:
					DoSetSelection(hWnd, true);
					return 0;

				case CM_CLEAR_ALL:
					DoSetSelection(hWnd, false);
					return 0;
			}
	}
	return 0;
}
