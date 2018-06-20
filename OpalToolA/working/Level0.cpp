/*******************************************************************************
 * Copyright (c) 2015, The Johns Hopkins University Applied Physics Laboratory *
 * LLC All rights reserved.                                                    *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without modifica-*
 * tion, are permitted provided that the following conditions are met:         *
 *                                                                             *
 * 1. Redistributions of source code must retain the above copyright notice,   *
 *    this list of conditions and the following disclaimer.                    *
 * 2. Redistributions in binary form must reproduce the above copyright notice,*
 *    this list of conditions and the following disclaimer in the documentation*
 *    and/or other materials provided with the distribution.                   *
 * 3. Neither the name of the copyright holder nor the names of its contri-    *
 *    butors may be used to endorse or promote products derived from this      *
 *    software without specific prior written permission.                      *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" *
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE   *
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CON-    *
 * SEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE*
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) *
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT  *
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY   *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH *
 * DAMAGE.                                                                     *
 *                                                                             *
 ******************************************************************************/



/**********************************************
 *                                            *
 *                                            *
 **********************************************/

#include"stdafx.h"
#include"Memory.h"
#include"AtaDrive.h"
#include"Level0.h"
#include"resource.h"


/* The subclass property string. */
static LPTSTR SubClassString = _T("Level0 Subclass");





/*
 * Counts the number of lines in a string.
 */
static int CountLines(LPTSTR String)
{
	int		i;
	int		Count;

	Count = 1;
	for(i=0; String[i]!=0 && String[i+1]!=0; i++) {
		if((String[i] == _T('\r')) && (String[i+1] == _T('\n'))) {
			Count++;
		}
	}
	return Count;
}


/*
 * This is the subclassed function for the edit window.
 */
static LRESULT CALLBACK NewEditProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC		OrigProc;
	LRESULT		Result;

	/* Get the original procedure. */
	OrigProc = (WNDPROC)GetProp(hWnd, SubClassString);

	/* Process the messages we care about. */
	switch(Msg) {
		case WM_GETDLGCODE:
			Result = CallWindowProc(OrigProc, hWnd, Msg, wParam, lParam);
			return Result & ~DLGC_HASSETSEL;
			break;
		case WM_DESTROY:
			RemoveProp(hWnd, SubClassString);
			SetWindowLong(hWnd, GWL_WNDPROC, (LONG)OrigProc);
			break;
		default:
			break;
	}

	/* Call the original routine. */
	return CallWindowProc(OrigProc, hWnd, Msg, wParam, lParam);
}


/*
 * This is the dialog function for displaying the level 0 discovery information for the drive.
 */
static BOOL CALLBACK Level0DisplayFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC		OldWndProc;
	LPTSTR		String;
	HFONT		hFontOld;
	HFONT		hFont;
	HWND		hWndEdit;
	RECT		Rect;
	SIZE		Size;
	HDC			hDC;
	int			NumLines;

	switch(Msg) {
		case WM_INITDIALOG:
			hWndEdit = GetDlgItem(hWnd, IDC_LEVEL0INFO);
			String = (LPTSTR)lParam;
			SetWindowText(hWndEdit, String);
			hDC = GetDC(hWnd);
			hFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | FIXED_PITCH, _T("Courier"));
			ReleaseDC(hWnd, hDC);
			SendMessage(hWndEdit, WM_SETFONT, (WPARAM)hFont, FALSE);
			hDC = GetDC(hWndEdit);
			hFontOld = (HFONT)SelectObject(hDC, hFont);
			GetTextExtentPoint32(hDC, String, lstrlen(String), &Size);
			SelectObject(hDC, hFontOld);
			ReleaseDC(hWnd, hDC);
			GetClientRect(hWndEdit, &Rect);
			NumLines = CountLines(String);
			if(NumLines <= (Rect.bottom / Size.cy)) {
				ShowScrollBar(hWndEdit, SB_VERT, FALSE);
			}
			OldWndProc = (WNDPROC)GetWindowLong(hWndEdit, GWL_WNDPROC);
			SetProp(hWndEdit, SubClassString, OldWndProc);
			SetWindowLong(hWndEdit, GWL_WNDPROC, (LONG)NewEditProc);
			return TRUE;
			break;
		case WM_CTLCOLORSTATIC:
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
			break;
		case WM_SYSCOMMAND:
			switch(wParam & 0xfff0) {
				case SC_CLOSE:
					EndDialog(hWnd, 0);
					return TRUE;
					break;
			}
			break;
	}

	return FALSE;
}


/*
 * Prints the header information for each feature.
 */
void PrintFeatureHeader(LPFEATURE Feature, LPTSTR Header, LPTSTR Buffer)
{
	TCHAR		Text[100];
	int			i;

	/* Print the carriage return for the previous line. */
	if(Buffer[0] != 0) {
		lstrcat(Buffer, _T("\r\n\r\n"));
	}

	/* Add the header. */
	lstrcat(Buffer, _T(" "));
	lstrcat(Buffer, Header);
	lstrcat(Buffer, _T("\r\n"));

	/* Add the underline. */
	Text[0] = ' ';
	for(i=0; Header[i]; i++) {
		Text[i+1] = '-';
	}

	/* Add the version information. */
	Text[i+1] = 0;
	lstrcat(Buffer, Text);
	lstrcat(Buffer, _T("\r\n"));
	wsprintf(Text, _T(" Feature Code:       %04x\r\n"), BE_TO_LE_16(Feature->FeatureCode));
	lstrcat(Buffer, Text);
	wsprintf(Text, _T(" Version:            %d"), (Feature->Version) >> 4);
	lstrcat(Buffer, Text);
}


/*
 * To save on multiple if statments, print strings based on true or false values.
 */
void PrintBoolean(LPTSTR Buffer, LPTSTR String, BOOL Value, LPTSTR TrueString, LPTSTR FalseString)
{
	lstrcat(Buffer, String);
	if(Value) {
		lstrcat(Buffer, TrueString);
	} else {
		lstrcat(Buffer, FalseString);
	}
}


/*
 * Print information from the TPer Feature Descriptor.
 */
void PrintTPerFeatures(LPTPERFEATURE Feature, LPTSTR Buffer)
{
	/* Print the feature header. */
	PrintFeatureHeader((LPFEATURE)Feature, _T("TPer Features"), Buffer);

	/* Print the rest of the information. */
	PrintBoolean(Buffer, _T("\r\n ComID Management:   "), Feature->Features & 0x40, _T("Supported"), _T("Not Supported"));
	PrintBoolean(Buffer, _T("\r\n Streaming           "), Feature->Features & 0x10, _T("Supported"), _T("Not Supported"));
	PrintBoolean(Buffer, _T("\r\n Buffer Management:  "), Feature->Features & 0x08, _T("Supported"), _T("Not Supported"));
	PrintBoolean(Buffer, _T("\r\n ACK/NACK:           "), Feature->Features & 0x04, _T("Supported"), _T("Not Supported"));
	PrintBoolean(Buffer, _T("\r\n Async:              "), Feature->Features & 0x02, _T("Supported"), _T("Not Supported"));
	PrintBoolean(Buffer, _T("\r\n Sync:               "), Feature->Features & 0x01, _T("Supported"), _T("Not Supported"));
}


/*
 * Print information from the Locking Feature Descriptor.
 */
void PrintLockingFeatures(LPLOCKINGFEATURE Feature, LPTSTR Buffer)
{
	/* Print the feature header. */
	PrintFeatureHeader((LPFEATURE)Feature, _T("Locking Features"), Buffer);

	/* Print the rest of the information. */
	PrintBoolean(Buffer, _T("\r\n MBR Done:           "), Feature->Features & 0x20, _T("Yes"), _T("No"));
	PrintBoolean(Buffer, _T("\r\n MBR:                "), Feature->Features & 0x10, _T("Enabled"), _T("Disabled"));
	PrintBoolean(Buffer, _T("\r\n Media Encryption:   "), Feature->Features & 0x08, _T("Supported"), _T("Not Supported"));
	PrintBoolean(Buffer, _T("\r\n Locked:             "), Feature->Features & 0x04, _T("Yes"), _T("No"));
	PrintBoolean(Buffer, _T("\r\n Locking Enabled:    "), Feature->Features & 0x02, _T("Enabled"), _T("Disabled"));
	PrintBoolean(Buffer, _T("\r\n Locking Supported:  "), Feature->Features & 0x01, _T("Supported"), _T("Not Supported"));
}


/*
 * Print information from the Opal or Enterprise Feature Descriptor.
 */
void PrintOpalFeatures(LPOPALSSC Feature, LPTSTR Buffer)
{
	TCHAR	Text[100];

	/* Print the feature header. */
	PrintFeatureHeader((LPFEATURE)Feature, _T("Opal Features"), Buffer);

	/* Print the rest of the information. */
	wsprintf(Text, _T("\r\n Base ComID:         0x%04x"), BE_TO_LE_16(Feature->BaseComId));
	lstrcat(Buffer, Text);
	wsprintf(Text, _T("\r\n Number of ComIDs:   %d"), BE_TO_LE_16(Feature->NumberComIds));
	lstrcat(Buffer, Text);
	PrintBoolean(Buffer, _T("\r\n Range Crossing:     "), Feature->RangeCrossing & 0x01, _T("Not Supported"), _T("Supported"));
}


/*
 * Prints information for each feature into the buffer.
 */
void PrintFeatureInformation(LPFEATURE Feature, LPTSTR Buffer)
{
	WORD FeatureCode = BE_TO_LE_16(Feature->FeatureCode);
	switch( FeatureCode ) {
		case 0x0001:
			PrintTPerFeatures((LPTPERFEATURE)Feature, Buffer);
			break;
		case 0x0002:
			PrintLockingFeatures((LPLOCKINGFEATURE)Feature, Buffer);
			break;
		case 0x0003:
//			PrintGeometryReportingFeatures((LPGEOMETRYREPORTINGFEATURE)Feature, Buffer);
			break;
		case 0x0100:
//                      PrintEnterpriseFeatures((LPENTERPRISESSC)Feature, Buffer);
                        break;
		case 0x0200:
			PrintOpalFeatures((LPOPALSSC)Feature, Buffer);
			break;
#ifndef OPALTOOLC
		case 0x0201:
//???                   PrintOpalFeatures((LPOPALSSC)Feature, Buffer);
			break;
		case 0x0202:
//???                   PrintOpalFeatures((LPOPALSSC)Feature, Buffer);
			break;
		case 0x0203:
//			PrintOpalV2Features((LPOPALSSC)Feature, Buffer);
			break;
#endif
		default:
			if( FeatureCode < 0xc000) {
				PrintFeatureHeader(Feature, _T("Unknown Features"), Buffer);
			} else {
				PrintFeatureHeader(Feature, _T("Unknown Vendor Features"), Buffer);
			}
			break;
	}
}


/*
 * Display level 0 information about the drive.
 */
void Level0Info(HWND hWnd, LPBYTE Buffer)
{
	LPBYTE		Offset;
	LPTSTR		TextBuffer;

	/* Allocate memory for the buffer. */
	TextBuffer = (LPTSTR)MemAlloc(64*1024);
	if(TextBuffer == NULL) {
		MessageBox(hWnd, _T("Out of resources - not enough memory."), _T("Level 0 Discovery Information"), MB_ICONERROR | MB_OK);
	}
	TextBuffer[0] = 0;

	/* Iterate through the descriptors. */
	Offset = &Buffer[0x30];
	while(((LPFEATURE)Offset)->Length != 0) {
		PrintFeatureInformation((LPFEATURE)Offset, TextBuffer);
		Offset += ((LPFEATURE)Offset)->Length + sizeof(FEATURE);
	}

	/* Display the information. */
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LEVEL0), hWnd, Level0DisplayFunc, (LPARAM)TextBuffer);

	/* Free up resources. */
	MemFree(TextBuffer);
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
