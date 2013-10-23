#include "FariaLib.h"

void	DumpBitmap(HDC hDC, HBITMAP hBitmap, const TCHAR *szFilename)
{
	TCHAR		temp[1024];
	BITMAP		bmp; 
	WORD		cClrBits; 
	PBITMAPINFO pbmi = NULL;
	HANDLE		hf = INVALID_HANDLE_VALUE; // file handle
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	PBITMAPINFOHEADER pbmih;     // bitmap info-header 
	LPBYTE		lpbmits;              // memory pointer 
	DWORD		dwTotal;              // total count of bytes 
	DWORD		cb;                   // incremental count of bytes 
	BYTE		*hp;                   // byte pointer 
	DWORD		dwTmp; 

	if (!GetObject(hBitmap, sizeof(bmp), &bmp))
	{
		return;
	}

	wsprintf(temp, TEXT("%p: %d, %d, %d\n\r"), hBitmap, bmp.bmWidth, bmp.bmHeight, bmp.bmBitsPixel);
	OutputDebugString(temp);

    // Convert the color format to a count of bits. 
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
    if (cClrBits == 1) 
        cClrBits = 1; 
    else if (cClrBits <= 4) 
        cClrBits = 4; 
    else if (cClrBits <= 8) 
        cClrBits = 8; 
    else if (cClrBits <= 16) 
        cClrBits = 16; 
    else if (cClrBits <= 24) 
        cClrBits = 24; 
    else cClrBits = 32; 

    // Allocate memory for the BITMAPINFO structure. (This structure 
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
    // data structures.) 

     if (cClrBits != 24) 
	 {
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER) + 
                    sizeof(RGBQUAD) * (1<< cClrBits)); 
	 }
     // There is no RGBQUAD array for the 24-bit-per-pixel format. 
     else 
	 {
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER)); 
	 }

    // Initialize the fields in the BITMAPINFO structure. 

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
    pbmi->bmiHeader.biWidth = bmp.bmWidth; 
    pbmi->bmiHeader.biHeight = bmp.bmHeight; 
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
    if (cClrBits < 24) 
	{
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 
	}

    // If the bitmap is not compressed, set the BI_RGB flag. 
    pbmi->bmiHeader.biCompression = BI_RGB; 

    // Compute the number of bytes in the array of color 
    // indices and store the result in biSizeImage. 
    // For Windows NT, the width must be DWORD aligned unless 
    // the bitmap is RLE compressed. This example shows this. 
    // For Windows 95/98/Me, the width must be WORD aligned unless the 
    // bitmap is RLE compressed.
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                  * pbmi->bmiHeader.biHeight; 
    // Set biClrImportant to 0, indicating that all of the 
    // device colors are important. 
     pbmi->bmiHeader.biClrImportant = 0; 

//void CreateBMPFile(HWND hwnd, LPTSTR pszFile, pbmiTMAPINFO pbmi, HBITMAP hBMP, HDC hDC) 
// { 

    pbmih = (PBITMAPINFOHEADER) pbmi; 
    lpbmits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbmih->biSizeImage);

    if (!lpbmits) 
	{
		OutputDebugString(TEXT("GlobalAlloc() failed."));
	}

    // Retrieve the color table (RGBQUAD array) and the bits 
    // (array of palette indices) from the DIB. 
	int temp_biClrUsed = pbmi->bmiHeader.biClrUsed;
    if (!GetDIBits(hDC, hBitmap, 0, (WORD) pbmih->biHeight, lpbmits, pbmi, DIB_RGB_COLORS)) 
    {
		OutputDebugString(TEXT("GetDIBits() failed."));
    }
	pbmi->bmiHeader.biClrUsed = temp_biClrUsed;

    // Create the .BMP file. 
    if (INVALID_HANDLE_VALUE == (hf = CreateFile(szFilename, 
                   GENERIC_READ | GENERIC_WRITE, 
                   (DWORD) 0, 
                    NULL, 
                   CREATE_ALWAYS, 
                   FILE_ATTRIBUTE_NORMAL, 
                   (HANDLE) NULL)))
	{
		OutputDebugString(TEXT("CreateFile() failed."));
		return;
	}

    hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
    // Compute the size of the entire file. 
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
                 pbmih->biSize + pbmih->biClrUsed 
                 * sizeof(RGBQUAD) + pbmih->biSizeImage); 
    hdr.bfReserved1 = 0; 
    hdr.bfReserved2 = 0; 

    // Compute the offset to the array of color indices. 
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
                    pbmih->biSize + pbmih->biClrUsed 
                    * sizeof (RGBQUAD); 

    // Copy the BITMAPFILEHEADER into the .BMP file. 
    if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
        (LPDWORD) &dwTmp,  NULL)) 
    {
       OutputDebugString(TEXT("WriteFile"));
    }

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
    if (!WriteFile(hf, (LPVOID) pbmih, sizeof(BITMAPINFOHEADER) 
                  + pbmih->biClrUsed * sizeof (RGBQUAD), 
                  (LPDWORD) &dwTmp, ( NULL)))
	{
       OutputDebugString(TEXT("WriteFile"));
	}

    // Copy the array of color indices into the .BMP file. 
    dwTotal = cb = pbmih->biSizeImage; 
    hp = lpbmits; 
    if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL))
	{
		OutputDebugString(TEXT("WriteFile"));
	}

    // Close the .BMP file. 
	if (!CloseHandle(hf)) 
	{
		OutputDebugString(TEXT("CloseHandle"));
	}

    // Free memory. 
    GlobalFree((HGLOBAL)lpbmits);
}