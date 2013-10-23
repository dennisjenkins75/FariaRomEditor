/*	This file contains the routines used to edit a Faria saved state RAM file. */

#include "FariaRomEditor.h"

static const int RAM_SIZE = 8192;	// expected size of RAM file.
static const int ITEM_SLOTS = 20;

static TCHAR g_szSaveGameFile[_MAX_PATH] = TEXT("faria.sav");
static TCHAR g_szDefaultSaveGameDir[_MAX_PATH] = TEXT("");
static unsigned char g_ucRamFile[RAM_SIZE];
static unsigned char *g_pCurSlot = g_ucRamFile;

static void InitializeEquipedItemComboBoxes(HWND hWnd, unsigned char *pSlotData);
static void EquipItem(unsigned char *pSlotData, int offset, const TCHAR *equipment);
static void	SetActiveSlot(HWND hWnd, int Slot);
static void	SetStartingTown(int iTown);

// We use "EN_CHANGE" notification messages to learn when the user
// has updated any widget.  However, these are also sent by the widgets when
// updated programmatically (like via WM_SETTEXT or SetDlgItemText).  So,
// we use this global flag to determine if the user made the change, or
// if we did.
static bool g_bInsideSetText = false;

enum FARIA_FORMAT
{
	FARIA_STR,
	FARIA_INT,
	FARIA_ITEM_PTR,
	FARIA_ITEM,
	FARIA_START_TOWN
};

struct	VAR_MAPPING
{
	int offset;
	int size;
	union {
		int max_value;
		int bit_mask;
	};
	int iControl;
	int bias;
	FARIA_FORMAT format;
};

static VAR_MAPPING g_VarMapping[] =
{
	{ 0x00, 6, 0, IDC_NAME, 0, FARIA_STR },
	{ 0x06, 3, 0, IDC_STARTING_TOWN, 0, FARIA_START_TOWN },

	{ 0x0e, 1, 0, IDC_SWORD, 0, FARIA_ITEM_PTR },
	{ 0x0f, 1, 0, IDC_BOW, 0, FARIA_ITEM_PTR },
	{ 0x10, 1, 0, IDC_ARMOUR, 0, FARIA_ITEM_PTR },
	{ 0x11, 1, 0, IDC_SHEILD, 0, FARIA_ITEM_PTR },

	{ 0x14, 1, 250, IDC_HP, 0, FARIA_INT },
	{ 0x15, 1, 250, IDC_MAX_HP, 0, FARIA_INT },
	{ 0x17, 3, 99999, IDC_XP, 0, FARIA_INT },
	{ 0x62, 3, 99999, IDC_XP_NEXT, 0, FARIA_INT },
	{ 0x1a, 1, 30, IDC_LEVEL, 1, FARIA_INT },
	{ 0x1b, 3, 99999, IDC_GOLD, 0, FARIA_INT },
	{ 0x1e, 1, 250, IDC_BATTERIES, 0, FARIA_INT },

	{ 0x25, 1, 250, IDC_ARROWS, 0, FARIA_INT },
	{ 0x27, 1, 250, IDC_MAGIC_SEDE, 0, FARIA_INT },
	{ 0x28, 1, 250, IDC_BOMBS, 0, FARIA_INT },
	{ 0x29, 1, 250, IDC_MAGIC_SABA, 0, FARIA_INT },

	{ 0x2f, 1, 0, IDC_ITEM_SLOT_0, 0, FARIA_ITEM },
	{ 0x31, 1, 0, IDC_ITEM_SLOT_1, 0, FARIA_ITEM },
	{ 0x33, 1, 0, IDC_ITEM_SLOT_2, 0, FARIA_ITEM },
	{ 0x35, 1, 0, IDC_ITEM_SLOT_3, 0, FARIA_ITEM },
	{ 0x37, 1, 0, IDC_ITEM_SLOT_4, 0, FARIA_ITEM },
	{ 0x39, 1, 0, IDC_ITEM_SLOT_5, 0, FARIA_ITEM },
	{ 0x3b, 1, 0, IDC_ITEM_SLOT_6, 0, FARIA_ITEM },
	{ 0x3d, 1, 0, IDC_ITEM_SLOT_7, 0, FARIA_ITEM },
	{ 0x3f, 1, 0, IDC_ITEM_SLOT_8, 0, FARIA_ITEM },
	{ 0x41, 1, 0, IDC_ITEM_SLOT_9, 0, FARIA_ITEM },
	{ 0x43, 1, 0, IDC_ITEM_SLOT_10, 0, FARIA_ITEM },
	{ 0x45, 1, 0, IDC_ITEM_SLOT_11, 0, FARIA_ITEM },
	{ 0x47, 1, 0, IDC_ITEM_SLOT_12, 0, FARIA_ITEM },
	{ 0x49, 1, 0, IDC_ITEM_SLOT_13, 0, FARIA_ITEM },
	{ 0x4b, 1, 0, IDC_ITEM_SLOT_14, 0, FARIA_ITEM },
	{ 0x4d, 1, 0, IDC_ITEM_SLOT_15, 0, FARIA_ITEM },
	{ 0x4f, 1, 0, IDC_ITEM_SLOT_16, 0, FARIA_ITEM },
	{ 0x51, 1, 0, IDC_ITEM_SLOT_17, 0, FARIA_ITEM },
	{ 0x53, 1, 0, IDC_ITEM_SLOT_18, 0, FARIA_ITEM },
	{ 0x55, 1, 0, IDC_ITEM_SLOT_19, 0, FARIA_ITEM },

	{ 0, 0, 0, 0, FARIA_INT }	// "0" for iControl marks end of array.
};

static inline VAR_MAPPING *GetVarMappingByControl(int iControl)
{
	for (VAR_MAPPING *p = g_VarMapping; p->iControl; p++)
	{
		if (p->iControl == iControl)
		{
			return p;
		}
	}

	return NULL;
}

static bool LoadSaveState(HWND hWnd, const TCHAR *szFilename)
{
	HANDLE hFile;
	DWORD dwBytes;
	TCHAR szTitle[_MAX_PATH + 80];

	if (INVALID_HANDLE_VALUE == (hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)))
	{
		MessageBox(hWnd, TEXT("Unable to open file for reading."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (!ReadFile(hFile, g_ucRamFile, RAM_SIZE, &dwBytes, NULL))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to read file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (RAM_SIZE != dwBytes)
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to read entire file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	CloseHandle(hFile);

	wnsprintf(szTitle, TCHAR_sizeof(szTitle), TEXT("Faria Save State Editor - %s"), szFilename);
	SetWindowText(hWnd, szTitle);

	lstrcpyn(g_szSaveGameFile, szFilename, TCHAR_sizeof(g_szSaveGameFile));

	return true;
}

static bool DoOpenFile(HWND hWnd)
{
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof(ofn));
	ofn.hInstance = g_hInstance;
	ofn.hwndOwner = hWnd;
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = TEXT("faria.sav");
	ofn.lpstrFile = g_szSaveGameFile;
	ofn.nMaxFile = sizeof(g_szSaveGameFile);
	ofn.lpstrInitialDir = g_szDefaultSaveGameDir;
	ofn.lpstrTitle = TEXT("NES Faria save game / battery RAM file");
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (!GetOpenFileName(&ofn))
	{
		return false;
	}

	lstrcpyn(g_szDefaultSaveGameDir, ofn.lpstrFile, ofn.nFileOffset);
	g_szDefaultSaveGameDir[ofn.nFileOffset] = 0;

	LoadSaveState(hWnd, ofn.lpstrFile);

	RegistryWriteString(g_hRegKeyRoot, TEXT("WorkingDir"), g_szWorkingDir);

	return true;
}


static bool	SaveFile(HWND hWnd)
{
	HANDLE hFile;
	DWORD dwBytes;

	// Purge all of the non-save state stuff.  Then replicate the
	// routine that makes multiple copies of the player data the same
	// way that the game does.

	// Remember, the 3 player slots are from $6980 to $6aff and that
	// $6900 = $900 in the RAM file (based at $6000).
	memset(g_ucRamFile, 0, 0x900);	// Before the save slots
	memset(g_ucRamFile + 0x900, 0, 0x80);	// The temp slot
	memset(g_ucRamFile + 0xb00, 0, 0x1fff-0xb00);	// above the save slots.

	// Now to the replication.
	memcpy(g_ucRamFile + 0xb00, g_ucRamFile + 0x980, 0x180);
	memcpy(g_ucRamFile + 0xc80, g_ucRamFile + 0x980, 0x180);

	if (INVALID_HANDLE_VALUE == (hFile = CreateFile(g_szSaveGameFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL)))
	{
		MessageBox(hWnd, TEXT("Unable to open file for writing."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (!WriteFile(hFile, g_ucRamFile, RAM_SIZE, &dwBytes, NULL))
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to write file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (RAM_SIZE != dwBytes)
	{
		CloseHandle(hFile);
		MessageBox(hWnd, TEXT("Failed to write entire file."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return false;
	}

	CloseHandle(hFile);

	return true;
}

/*
$BB88:A2 7E     LDX #$7E
$BB8A:A0 00     LDY #$00
$BB8C:84 A7     STY $A7
$BB8E:A9 27     LDA #$27
$BB90:85 A6     STA $A6
$BB92:B1 96     LDA ($96),Y
$BB94:18        CLC
$BB95:65 A6     ADC $A6
$BB97:85 A6     STA $A6
$BB99:A9 00     LDA #$00
$BB9B:65 A7     ADC $A7
$BB9D:85 A7     STA $A7
$BB9F:C8        INY
$BBA0:CA        DEX
$BBA1:D0 EF     BNE $BB92
$BBA3:60        RTS
*/

/* The routine in the NES ROM that computes the checksum is at $7b98 (iNes format).
   It is mapped to $bb88 at runtime.  The 16-bit accumulator starts at 0x27 and you 
   just add the next 126 bytes to the accumulator.  The xsum is stored as the last two
   bytes of the ram slot, in MSB order. */
static inline int	ComputeSaveStateCheckSum(unsigned char *pSlotData)
{
	int	accum = 0x27;	// From NES ROM, $7b98, mapped to $bb88 at runtime.
	int i;

	for (i = 0; i < 0x7e; accum += pSlotData[i++]);
	return accum;
}

static void	MySetText(HWND hWnd, int iControl, const TCHAR *szText)
{
	bool bSave = g_bInsideSetText;
	g_bInsideSetText = true;
	SetDlgItemText(hWnd, iControl, szText);
	g_bInsideSetText = bSave;
}

// Renders 8 rows of 16 bytes each of data into edit control.
// If 'iListViewHack' is "-1", then compute the highlist using the
// "VAR_MAPPING" array.  If "iListViewHack" is != -1, then highlight just
// that offset.
static void	RenderRawRam(HWND hWnd, unsigned char *pSlotData, int iListViewHack)
{
	TCHAR temp[2048];	// only really needs 409 bytes.
	TCHAR *p = temp;
	TCHAR szSpace[2] = TEXT(" ");
	TCHAR szCrLf[3] = TEXT("\r\n");
	HWND hWndFocus = NULL;
	VAR_MAPPING *vm = NULL;
	int nFirst = -1;		// See notes on message 'EM_SETSEL'.
	int nLast = -1;
	int i;

	// Determine the "VAR_MAPPING" struct for the control that
	// currently has focus.  In none, then set pointer to NULL.
	if (NULL != (hWndFocus = GetFocus()))
	{
		// If this is an edit control for a combo box, then use the
		// combo box as the control item.
		GetClassName(hWndFocus, temp, TCHAR_sizeof(temp));
		if (!lstrcmp(temp, TEXT("Edit")))
		{
			GetClassName(GetParent(hWndFocus), temp, TCHAR_sizeof(temp));

			if (!lstrcmp(temp, TEXT("ComboBox")))
			{
				hWndFocus = GetParent(hWndFocus);
			}
		}

		if (0 != (i = GetDlgCtrlID(hWndFocus)))
		{
			// GetDlgCtrlID() is not 100% reliable, so verify the results.
			if (hWndFocus == GetDlgItem(hWnd, i))
			{
				vm = GetVarMappingByControl(i);
			}
		}
	}

	for (i = 0; i < 128; i++, pSlotData++)
	{
		p += wsprintf(p, TEXT("%02x"), *pSlotData);

		if (iListViewHack != -1)
		{
			if (iListViewHack == i)
			{
				nFirst = p - temp - 2;
				nLast = p - temp;
			}
		}
		else if (vm)
		{
			if (vm->offset == i)
			{
				nFirst = p - temp - 2;
			}

			if (vm->offset + vm->size == i + 1)
			{
				nLast = p - temp;
			}
		}

//		*(p++) = ' ';
		*(p++) = szSpace[0];

		if ((i & 0x0f) == 0x07)
		{
//			*(p++) = ' ';
			*(p++) = szSpace[0];
		}
		else if ((i & 0x0f) == 0x0f)
		{
			p--;	// eat space added by "sprintf"

			if (i != 127)	// Don't add final CR/LF.
			{
//				*(p++) = '\r';
//				*(p++) = '\n';
				*(p++) = szCrLf[0];
				*(p++) = szCrLf[1];
			}
		}
	}
	*p = 0;

	MySetText(hWnd, IDC_RAW_RAM, temp);

	// Update the highlighting of the RAW RAM section that corresponds to the widget in focus.

	SendDlgItemMessage(hWnd, IDC_RAW_RAM, EM_SETSEL, nFirst, nLast);
}

// Return # of characters required to display a given integer.
// Ex; "255" = 3, "99999" = 5.
static int GetIntStrLen(int value)
{
	TCHAR temp[64];

	return wnsprintf(temp, TCHAR_sizeof(temp), TEXT("%d"), value);
}

static inline int FariaGetIntValue(unsigned char *p, int bytes)
{
	switch (bytes)
	{
		case 1:
			return p[0];

		case 2:
			return p[0] + (p[1] << 8);

		case 3:
			return p[0] + (p[1] << 8) + (p[2] << 16);

		default:
			return 0;
	}
}

static inline void FariaSetIntValue(unsigned char *p, int bytes, int value)
{
	switch (bytes)
	{
		case 3:
			p[2] = (value >> 16) & 0xff;
			// no break statement.

		case 2:
			p[1] = (value >> 8) & 0xff;
			// no break statement.

		case 1:
			p[0] = value & 0xff;
			break;
	}
}

static inline void FariaSetIntValue_MSB(unsigned char *p, int bytes, int value)
{
	switch (bytes)
	{
		case 3:
			p[0] = (value >> 16) & 0xff;
			p[1] = (value >> 8) & 0xff;
			p[2] = value & 0xff;
			break;

		case 2:
			p[0] = (value >> 8) & 0xff;
			p[1] = value & 0xff;
			break;

		case 1:
			p[0] = value & 0xff;
			break;
	}
}


// We expect that 'pSlotData' will point to 128 bytes from the battery file.
static void	SetWidgets(HWND hWnd, unsigned char *pSlotData)
{
	TCHAR temp[256];
	VAR_MAPPING *p = NULL;
	const TCHAR *temp_ptr = NULL;
	bool bSave = g_bInsideSetText;
	int value;

	g_bInsideSetText = true;

	for (p = g_VarMapping; p->iControl; p++)
	{
		switch (p->format)
		{
			case FARIA_INT:
				value = FariaGetIntValue(pSlotData + p->offset, p->size) + p->bias;
				SetDlgItemInt(hWnd, p->iControl, value, false);
				SendDlgItemMessage(hWnd, p->iControl, EM_LIMITTEXT, GetIntStrLen(p->max_value), 0);
				break;

			case FARIA_STR:
				FariaGetString(temp, TCHAR_sizeof(temp), pSlotData + p->offset, p->size);
				SetDlgItemText(hWnd, p->iControl, temp);
				SendDlgItemMessage(hWnd, p->iControl, EM_LIMITTEXT, p->size, 0);
				break;

			case FARIA_ITEM:
				temp_ptr = FariaGetItemName(pSlotData[p->offset]);
				if (temp_ptr[0])
				{
					SendDlgItemMessage(hWnd, p->iControl, CB_SELECTSTRING, -1, (LPARAM)temp_ptr);
				}
				else
				{
					SendDlgItemMessage(hWnd, p->iControl, CB_SETCURSEL, 0, 0);
				}
				break;

			case FARIA_ITEM_PTR:
				// Now, select the active item.
				value = pSlotData[p->offset];	// index of equiped item, *2
				temp_ptr = FariaGetItemName(pSlotData[value + 0x2f]);	// item name
				SendDlgItemMessage(hWnd, p->iControl, CB_SELECTSTRING, -1, (LPARAM)temp_ptr);
				break;

			case FARIA_START_TOWN:
				value = pSlotData[p->offset];
				temp_ptr = value < TOWN_COUNT ? g_TownInfo[value].szName : g_TownInfo[0].szName;
				SendDlgItemMessage(hWnd, p->iControl, CB_SELECTSTRING, -1, (LPARAM)temp_ptr);
				break;
		}
	}

	RenderRawRam(hWnd, pSlotData, -1);

	g_bInsideSetText = bSave;
}

static void	HandleNotifications(HWND hWnd, WPARAM wParam, unsigned char *pSlotData)
{
	VAR_MAPPING *p = NULL;
	int value = 0;
	int nFirst = 0;
	int nLast = 0;
	int iCmd = HIWORD(wParam);
	int idControl = LOWORD(wParam);
	TCHAR temp[256];

	switch (idControl)
	{
		case IDC_RADIO_SLOT_0:
			SetActiveSlot(hWnd, 0);
			return;
		case IDC_RADIO_SLOT_1:
			SetActiveSlot(hWnd, 1);
			return;
		case IDC_RADIO_SLOT_2:
			SetActiveSlot(hWnd, 2);
			return;
		case IDC_RADIO_SLOT_3:
			SetActiveSlot(hWnd, 3);
			return;
	}

	
	if (g_bInsideSetText)
	{
		return;
	}

	// See if the control is listed in the array.
	if (NULL == (p = GetVarMappingByControl(idControl)))
	{
		return;
	}

	// Update the RAM and redraw the controls.

	// We know that "g_bInsideSetText" is false right now, but
	// we must make it true.  We also don't have to save it's value.
	g_bInsideSetText = true;

	if ((iCmd == EN_KILLFOCUS) || (iCmd == CBN_KILLFOCUS) || (iCmd == CBN_SELCHANGE) || (iCmd == BN_KILLFOCUS) || (iCmd == BN_CLICKED))
	{
		switch (p->format)
		{
			case FARIA_INT:
				value = GetDlgItemInt(hWnd, p->iControl, NULL, false);
				if (value > p->max_value)
				{
					value = p->max_value;
				}
				// 'value' will be 0 on error or "0" for real.  Force redraw
				// incase 'value' is in error or we had to modify it.
				SetDlgItemInt(hWnd, p->iControl, value, false);

				FariaSetIntValue(pSlotData + p->offset, p->size, value - p->bias);
				break;

			case FARIA_STR:
				GetDlgItemText(hWnd, p->iControl, temp, TCHAR_sizeof(temp));
				FariaSetString(pSlotData + p->offset, p->size, temp, lstrlen(temp));
				FariaGetString(temp, TCHAR_sizeof(temp), pSlotData + p->offset, p->size);
				SetDlgItemText(hWnd, p->iControl, temp);
				break;

			case FARIA_ITEM:
				value = SendDlgItemMessage(hWnd, p->iControl, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hWnd, p->iControl, CB_GETLBTEXT, value, (LPARAM)temp);
				value = FariaResolveListBoxStringIndex(temp);
				pSlotData[p->offset] = value;

				InitializeEquipedItemComboBoxes(hWnd, pSlotData);
				break;

			case FARIA_ITEM_PTR:
				value = SendDlgItemMessage(hWnd, p->iControl, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hWnd, p->iControl, CB_GETLBTEXT, value, (LPARAM)temp);
				EquipItem(pSlotData, p->offset, temp);
				break;

			case FARIA_START_TOWN:
				value = SendDlgItemMessage(hWnd, p->iControl, CB_GETCURSEL, 0, 0);
				SetStartingTown(value);
				break;
		}

		value = ComputeSaveStateCheckSum(pSlotData);
		FariaSetIntValue_MSB(pSlotData + 0x7e, 2, value);
	}

	RenderRawRam(hWnd, pSlotData, -1);
	g_bInsideSetText = false;
}

static void	SetComboBoxSize(HWND hComboBox)
{
	RECT rect;

	GetClientRect(hComboBox, &rect);
	rect.bottom *= 10;	// Make the dropdown list bigger.
	SetWindowPos(hComboBox, HWND_TOP, 0, 0, rect.right, rect.bottom, SWP_NOMOVE | SWP_DEFERERASE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER | SWP_DRAWFRAME);
}

static void InitializeComboBoxes(HWND hWnd)
{
	HWND hComboBox;
	int i, j;
	const TCHAR *temp;

	for (i = IDC_ITEM_SLOT_0; i <= IDC_ITEM_SLOT_19; i++)
	{
		hComboBox = GetDlgItem(hWnd, i);

		SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
		SendMessage(hComboBox, CB_INITSTORAGE, MAX_LIST_BOX_STR_IDX, MAX_LIST_BOX_STR_IDX * 16);
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"");
		SetComboBoxSize(hComboBox);

		for (j = 0; j <= MAX_LIST_BOX_STR_IDX; j++)
		{
			temp = FariaGetItemName(j);

			if (temp && temp[0] && (temp[0] != ' '))
			{
				SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)temp);
			}
		}
	}

	hComboBox = GetDlgItem(hWnd, IDC_STARTING_TOWN);
	SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
	SendMessage(hComboBox, CB_INITSTORAGE, TOWN_COUNT, TOWN_COUNT * 10);
	SetComboBoxSize(hComboBox);
	for (i = 0; i < TOWN_COUNT; i++)
	{
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)g_TownInfo[i].szName);
	}
}

// 'offset' is offset into 'pSlotData' for the equiped item pointer. (ie,
// 0x0e for swird, 0x0f for bow, ...  'equipment' is the item the user chose
// from the combo box.
static void EquipItem(unsigned char *pSlotData, int offset, const TCHAR *equipment)
{
	int slot;
	int equip_id;

	if (-1 == (equip_id = FariaResolveListBoxStringIndex(equipment)))
	{
		return;
	}

	// Search the inventory looking for a matching item.  If found, equip it.
	for (slot = 0; slot < ITEM_SLOTS; slot++)
	{
		if (pSlotData[slot * 2 + 0x2f] == equip_id)
		{
			pSlotData[offset] = slot * 2;
			return;
		}
	}

	pSlotData[offset] = 0xfe;	// seems to be the default.
}

static void	SetStartingTown(int iTown)
{
	if ((iTown < 0) || (iTown >= TOWN_COUNT))
	{
		iTown = 0;	// Ehdo
	}

	g_pCurSlot[0x06] = iTown;
	g_pCurSlot[0x07] = g_TownInfo[iTown].x_start;
	g_pCurSlot[0x08] = g_TownInfo[iTown].y_start;
}


static void InitializeEquipedItemComboBoxesHelper(HWND hComboBox, unsigned char *pSlotData, int offset, const int *valid_array)
{
	int i, j;
	const TCHAR *temp;
	unsigned char item;

	SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
	SendMessage(hComboBox, CB_INITSTORAGE, ITEM_SLOTS, MAX_LIST_BOX_STR_IDX * ITEM_SLOTS);
	SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"");
	SetComboBoxSize(hComboBox);

	for (j = 0; j < ITEM_SLOTS; j++)
	{
		item = pSlotData[0x2f + j * 2];

		for (i = 0; valid_array[i] != -1; i++)
		{
			if (valid_array[i] == item)
			{
				temp = FariaGetItemName(item);

				if (temp && temp[0])
				{
					SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)temp);
				}

				break;
			}
		}
	}

	// Now, select the active item.
	i = pSlotData[offset];	// index of equiped item, *2
	temp = FariaGetItemName(pSlotData[i + 0x2f]);	// item name
	SendMessage(hComboBox, CB_SELECTSTRING, -1, (LPARAM)temp);
}

// Call this function after updating the item slots.
static void InitializeEquipedItemComboBoxes(HWND hWnd, unsigned char *pSlotData)
{
	InitializeEquipedItemComboBoxesHelper(GetDlgItem(hWnd, IDC_SWORD), pSlotData, 0x0e, g_iValidSwords);
	InitializeEquipedItemComboBoxesHelper(GetDlgItem(hWnd, IDC_BOW), pSlotData, 0x0f, g_iValidBows);
	InitializeEquipedItemComboBoxesHelper(GetDlgItem(hWnd, IDC_ARMOUR), pSlotData, 0x10, g_iValidArmours);
	InitializeEquipedItemComboBoxesHelper(GetDlgItem(hWnd, IDC_SHEILD), pSlotData, 0x11, g_iValidSheilds);
}

static const int SUBITEM_CHECKBOX = 0;
static const int SUBITEM_MEANING = 1;

struct LISTVIEW_COLUMN 
{
	const TCHAR		*	Title;
	int					Width;
};

struct LISTVIEW_COLUMN BitFlagColumns[] = 
{
	{ TEXT("Status"),			63 },	// First column has check box.
	{ TEXT("Description"),		260 },
	{ NULL,						0 }
};

struct	BIT_FLAG
{
	int		offset;
	int		mask;
	TCHAR	*desc;
};

static BIT_FLAG g_BitFlags[] =
{
	{ 0x5b, 0x01, TEXT("Gelve tower completed.") },
	{ 0x5b, 0x02, TEXT("Broww tower completed, have ring..") },
	{ 0x5b, 0x04, TEXT("North tower completed, have sky shoes, can enter sky world.") },
	{ 0x5b, 0x08, TEXT("Phantom Tower destroyed (real Princess follows player).") },
	{ 0x5b, 0x10, TEXT("Final Tower destroyed.") },
	{ 0x5b, 0x20, TEXT("Phantom Tower revealed.") },
	{ 0x5b, 0x40, TEXT("Have King's permission to cross bridge.") },
	{ 0x5b, 0x80, TEXT("People of Ehdo have been poisoned.") },

	{ 0x5c, 0x01, TEXT("Have spoken to nurse in Ehdo about the poisoning.") },
	{ 0x5c, 0x02, TEXT("Highria elder gives you Mysterious Seed.") },
	{ 0x5c, 0x04, TEXT("Have purchased a Wonder Capsule.") },
	{ 0x5c, 0x08, TEXT("Saved Ehdo using Wonder Capsule.") },
	{ 0x5c, 0x10, TEXT("Spoke to Imposter after saving Ehdo (see $5e:04).") },
	{ 0x5c, 0x20, TEXT("Received Golden Arrow from lady in pond.") },
	{ 0x5c, 0x40, TEXT("Elephant creature killed.") },
	{ 0x5c, 0x80, TEXT("Teodoor sage gives you Translation Machine.") },

	{ 0x5d, 0x01, TEXT("Zelos gives you Super Armour (outside of Highria).") },
	{ 0x5d, 0x02, TEXT("Letter from Tegza to use a boat.") },
	{ 0x5d, 0x04, TEXT("Have rope from Sky World (can not re-enter).") },
	{ 0x5d, 0x08, TEXT("Zellia sage gave you Crystal of Truth.") },
	{ 0x5d, 0x10, TEXT("Wizard dead, sun blocked, player is male.") },	// checked every game frame.
	{ 0x5d, 0x20, TEXT("") },
	{ 0x5d, 0x40, TEXT("Brought real princess home.") },
	{ 0x5d, 0x80, TEXT("(masked out when entering a town)") },

	{ 0x5e, 0x01, TEXT("Killed wizard in King's castle.") },
	{ 0x5e, 0x02, TEXT("Received golden stone from sage in cave near Karuza.") },
	{ 0x5e, 0x04, TEXT("Spoke to Imposter after saving Ehdo (same as $5c:10)") },
	{ 0x5e, 0x08, TEXT("") },
	{ 0x5e, 0x10, TEXT("") },
	{ 0x5e, 0x20, TEXT("") },
	{ 0x5e, 0x40, TEXT("(masked out when leaving a town)") },
	{ 0x5e, 0x80, TEXT("(masked out when leaving a town)") },

	{ 0x5f, 0x01, TEXT("") },
	{ 0x5f, 0x02, TEXT("") },
	{ 0x5f, 0x04, TEXT("") },
	{ 0x5f, 0x08, TEXT("") },
	{ 0x5f, 0x10, TEXT("") },
	{ 0x5f, 0x20, TEXT("") },
	{ 0x5f, 0x40, TEXT("") },
	{ 0x5f, 0x80, TEXT("") },

	{ 0x60, 0x01, TEXT("Visited Ehdo.") },
	{ 0x60, 0x02, TEXT("Visited Somusa.") },
	{ 0x60, 0x04, TEXT("Visited Karuza.") },
	{ 0x60, 0x08, TEXT("Visited Highria.") },
	{ 0x60, 0x10, TEXT("Visited Riria.") },
	{ 0x60, 0x20, TEXT("Visited Teodoor.") },
	{ 0x60, 0x40, TEXT("Visited Tegza.") },
	{ 0x60, 0x80, TEXT("Visited Shilf.") },

	{ 0x61, 0x01, TEXT("Visited Zellia.") },
	{ 0x61, 0x02, TEXT("Visited Baig.") },
	{ 0x61, 0x04, TEXT("") },
	{ 0x61, 0x08, TEXT("") },
	{ 0x61, 0x10, TEXT("") },
	{ 0x61, 0x20, TEXT("") },
	{ 0x61, 0x40, TEXT("") },
	{ 0x61, 0x80, TEXT("") },

	// $6965 and up are bitflags for various treasures taken.
	// $6976, bit 0x02 = bag of $$ north of Highria.
	// $696d, bit 0x01 = have hyperspeed 3 from phantom tower.

	{ -1, -1, NULL }
};

static void		HandleBitFlagListNotification(HWND hList, NMLISTVIEW *nm)
{
	int offset = g_BitFlags[nm->iItem].offset;
	int mask = g_BitFlags[nm->iItem].mask;
	int state = ListView_GetCheckState(hList, nm->iItem) ? 1 : 0;
	int xsum = 0;

	if (state)
	{
		g_pCurSlot[offset] |= mask;
	}
	else
	{
		g_pCurSlot[offset] &= ~mask;
	}

	xsum = ComputeSaveStateCheckSum(g_pCurSlot);
	FariaSetIntValue_MSB(g_pCurSlot + 0x7e, 2, xsum);
	RenderRawRam(GetParent(hList), g_pCurSlot, offset);
}

static void	PopulateBitFlagListView(HWND hList)
{
	for (int i = 0; g_BitFlags[i].desc; i++)
	{
		int state = g_pCurSlot[g_BitFlags[i].offset] & g_BitFlags[i].mask ? 1 : 0;

		ListView_SetCheckState(hList, i, state);
	}
}

// This function should only be called once, from WM_INITDIALOG
static void	InitializeBitFlagsListView(HWND hList)
{
//	COLORREF rgb = GetSysColor(COLOR_GRAYTEXT);
	LVCOLUMN lvc;
	int i;
	LVITEM lvi;
	TCHAR temp[256];

	ListView_SetExtendedListViewStyleEx( hList, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
	ListView_SetExtendedListViewStyleEx( hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
//	ListView_SetBkColor(hList, rgb);
//	ListView_SetTextBkColor(hList, rgb);

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
	lvc.fmt = LVCFMT_LEFT;

	for (i = 0; BitFlagColumns[i].Title; i++)
	{
		lvc.iSubItem = i;
		lvc.cx = BitFlagColumns[i].Width;
		lvc.pszText = (TCHAR*)BitFlagColumns[i].Title;

		if (ListView_InsertColumn(hList, lvc.iSubItem, &lvc) == -1)
		{
			MessageBox(GetParent(hList), TEXT("ListView_InsertColumn() failed."), TEXT("Error"), MB_OK);
			ExitProcess(-1);
		}
	}

	for (i = 0; g_BitFlags[i].desc; i++)
	{
		lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
		lvi.state = 0;
		lvi.stateMask = 0;
		lvi.iItem = i;
		lvi.iSubItem = SUBITEM_CHECKBOX;
		lvi.iImage = NULL;
		wnsprintf(temp, TCHAR_sizeof(temp), TEXT("$%02x:$%02x"), g_BitFlags[i].offset, g_BitFlags[i].mask);
		lvi.pszText = temp;
		lvi.lParam = NULL;
		SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&lvi);

		lvi.iSubItem = SUBITEM_MEANING;
		lvi.pszText = g_BitFlags[i].desc;
		SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&lvi);
	}
}


static void	SetActiveSlot(HWND hWnd, int Slot)
{
	_ASSERT(Slot >= 0 && Slot < 4);
	g_pCurSlot = g_ucRamFile + 0x900 + Slot * 0x80;
	SetWidgets(hWnd, g_pCurSlot);

	InitializeEquipedItemComboBoxes(hWnd, g_pCurSlot);

	PopulateBitFlagListView(GetDlgItem(hWnd, IDC_BIT_FLAG_LIST));
}

INT_PTR CALLBACK SaveGameDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetWindowStandards(hWnd, TEXT("Faria Save Game Editor"));
			SendDlgItemMessage(hWnd, IDC_RADIO_SLOT_1, BM_SETCHECK, BST_CHECKED, 0);
			SendDlgItemMessage(hWnd, IDC_RAW_RAM, WM_SETFONT, (WPARAM)g_hFontMono, 0);
			InitializeComboBoxes(hWnd);
			InitializeBitFlagsListView(GetDlgItem(hWnd, IDC_BIT_FLAG_LIST));

			if (lParam)
			{
				if (!LoadSaveState(hWnd, (TCHAR *)lParam))
				{
					EndDialog(hWnd, 0);
				}
			}
			else
			{
				if (!DoOpenFile(hWnd))
				{
					EndDialog(hWnd, 0);
				}
			}

			SetActiveSlot(hWnd, 1);
			return 1;

		case WM_DESTROY:
			return 1;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hWnd, 0);
					return 0;

				case IDC_OPEN_FILE:
					if (DoOpenFile(hWnd))
					{
						SetActiveSlot(hWnd, 1);
					}
					return 0;

				case IDC_SAVE_FILE:
					SaveFile(hWnd);
					return 0;

				case IDC_RADIO_SLOT_0:
				case IDC_RADIO_SLOT_1:
				case IDC_RADIO_SLOT_2:
				case IDC_RADIO_SLOT_3:
					if (HIWORD(wParam) == BN_CLICKED)
					{
						HandleNotifications(hWnd, wParam, g_pCurSlot);
					}
					return 0;

				default:
					switch (HIWORD(wParam))
					{
						case EN_KILLFOCUS:
						case EN_SETFOCUS:
						case CBN_KILLFOCUS:
						case CBN_SETFOCUS:
						case CBN_SELCHANGE:
						case BN_SETFOCUS:
						case BN_KILLFOCUS:
						case BN_CLICKED:
							HandleNotifications(hWnd, wParam, g_pCurSlot);
							return 0;
					}
					return 0;
			}

		case WM_NOTIFY:
			if ((LOWORD(wParam) == IDC_BIT_FLAG_LIST) && !g_bInsideSetText)
			{
				NMLISTVIEW *nm = (NMLISTVIEW*)lParam;

				if (nm->hdr.code == LVN_ITEMCHANGED)
				{
					HandleBitFlagListNotification(GetDlgItem(hWnd, IDC_BIT_FLAG_LIST), nm);
				}
			}
			return 0;

	}
	return 0;
}
