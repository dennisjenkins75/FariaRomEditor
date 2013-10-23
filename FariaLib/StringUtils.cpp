/* Contains routines common to the text hacking tools and to the main program. */

#include "FariaLib.h"

static const char *str_b9 = ".,!?\"\": -'";

int		FariaGetString(TCHAR *dest, int dest_len, const unsigned char *faria, int faria_len)
{
	int	i;

	for (i = 0; (i < dest_len-1) && (i < faria_len); i++, faria++)
	{
		if ((*faria >= 0x90) && (*faria <= 0x99))
		{
			dest[i] = *faria - 0x90 + '0';
		}
		else if ((*faria >= 0xb9) && (*faria <= 0xc2))
		{
			dest[i] = str_b9[*faria - 0xb9];
		}
		else if ((*faria >= 0xc3) && (*faria <= 0xdc))
		{
			dest[i] = *faria - 0xc3 + 'a';
		}
		else if ((*faria >= 0xdd) && (*faria <= 0xf6))
		{
			dest[i] = *faria - 0xdd + 'A';
		}
		else
		{
			dest[i] = ' ';
		}
	}

	dest[i] = 0;

	return i;
}

int		FariaSetString(unsigned char *faria, int faria_len, const TCHAR *src, int src_len)
{
	int	i, j;

	memset(faria, 0xc0, faria_len);	// fill string with Faria space character.

	for (i = 0; (i < src_len) && (i < faria_len) && *src; i++, faria++, src++)
	{
		if ((*src >= '0') && (*src <= '9'))
		{
			*faria = *src - '0' + 0x90;
		}
		else if ((*src >= 'a') && (*src <= 'z'))
		{
			*faria = *src - 'a' + 0xc3;
		}
		else if ((*src >= 'A') && (*src <= 'Z'))
		{
			*faria = *src - 'A' + 0xdd;
		}
		else 
		{
			*faria = 0xc0;	// default character (space) if none others match.

			for (j = 0; str_b9[j]; j++)
			{
				if (str_b9[j] == *src)
				{
					*faria = j + 0xb9;
					break;
				}
			}
		}
	}

	return i;
}

int		FariaResolveListBoxStringIndex(const TCHAR *str)
{
	if (!str || !str[0])
	{
		return 0xff;
	}

	for (int i = 0; i <= MAX_LIST_BOX_STR_IDX; i++)
	{
		if (!lstrcmpi(g_szFariaListboxStrings[i], str))
		{
			return i;
		}
	}

	return -1;
}