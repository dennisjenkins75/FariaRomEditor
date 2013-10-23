
#include "FariaRomEditor.h"

const TCHAR *FariaGetItemName(int iIndex)
{
	if ((iIndex < 0) || (iIndex > MAX_LIST_BOX_STR_IDX))
	{
		return TEXT("");
	}

	return g_szFariaListboxStrings[iIndex];
}