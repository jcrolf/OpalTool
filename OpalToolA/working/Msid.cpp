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
#include"AtaDrive.h"
#include"Table.h"
#include"Tcg.h"
#include"Msid.h"
#include"Memory.h"
#include"Resource.h"


/* The caption for error messages. */
static LPTSTR	Caption = _T("MSID");

/*
 * This is the dialog function for displaying the MSID for the drive.
 */
static BOOL CALLBACK MsidDisplayFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
		case WM_INITDIALOG:
			SetDlgItemText(hWnd, IDC_MSID, (LPTSTR)lParam);
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
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hWnd, 0);
					break;
				case ID_CLIPBOARD:
				{
					DWORD		TextSize = SendDlgItemMessage(hWnd, IDC_MSID, WM_GETTEXTLENGTH, 0, 0) + 1;
					HGLOBAL		hMem = GlobalAlloc(GMEM_MOVEABLE, TextSize * sizeof(TCHAR));
					if (0 == hMem)
					{
						MessageBox(hWnd, _T("Unable to allocate memory for clipboard text."), _T("Memory Allocation Error"), MB_ICONERROR | MB_OK);
						break;
					}
					if (OpenClipboard(NULL) == FALSE) {
						MessageBox(hWnd, _T("Unable to copy MSID to the Clipboard."), _T("Clipboard Error"), MB_ICONERROR | MB_OK);
						GlobalFree(hMem);
						break;
					}
					EmptyClipboard();
					{
						LPTSTR		Text = (LPTSTR)GlobalLock(hMem);
						SendDlgItemMessage(hWnd, IDC_MSID, WM_GETTEXT, TextSize, (LPARAM)Text);
						GlobalUnlock(hMem);
						SetClipboardData(CF_TEXT, hMem);
						CloseClipboard();
					}
					break;
				}
			}
			return TRUE;
			break;
	}

	return FALSE;
}


/*
 * Get the MSID for the drive and display it to the user.
 */
void GetMSID(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	TCGAUTH	TcgAuth;
	LPTSTR	MSID[33];

	/* Read the MSID. */
	if (ReadMSID(pTcgDrive, &TcgAuth) == FALSE) {
		MessageBox(hWndParent, _T("An error occurred reading the MSID."), Caption, MB_OK);
		return;
	}

	/* Convert it to a NUL-terminated string. */
	memset(MSID, 0, sizeof(MSID));
	memcpy(MSID, TcgAuth.Credentials, TcgAuth.Size);

	/* Display the MSID. */
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MSID), hWndParent, MsidDisplayFunc, (LPARAM)MSID);
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/

