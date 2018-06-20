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
#include"Table.h"
#include"GetTable.h"
#include"resource.h"


/*
 * This is the dialog box message handler.
 */
static BOOL CALLBACK GetTableDialog(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LPTABLECELL	Iter;
	LPTABLE		pTable;
	LRESULT		Index;
	HWND		hWndCombo;
	int			NumRows;
	int			Row;
	int			i;

	switch(Msg) {
		case WM_INITDIALOG:
			pTable = (LPTABLE)lParam;
			hWndCombo = GetDlgItem(hWnd, IDC_COMBO1);
			NumRows = GetRows(pTable);
			for(i=0; i<NumRows; i++) {
				Iter = GetTableCell(pTable, i, 1);
				if(Iter != NULL) {
					Index = SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)(Iter->Bytes));
					SendMessage(hWndCombo, CB_SETITEMDATA, Index, i);
				}
			}
			SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);
			return TRUE;
			break;
		case WM_SYSCOMMAND:
			switch(wParam & 0xfff0) {
				case SC_CLOSE:
					EndDialog(hWnd, -1);
					return TRUE;
					break;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					hWndCombo = GetDlgItem(hWnd, IDC_COMBO1);
					Index = SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
					Row = SendMessage(hWndCombo, CB_GETITEMDATA, Index, 0);
					EndDialog(hWnd, Row);
					break;
				case IDCANCEL:
					EndDialog(hWnd, -1);
					break;
			}
			return TRUE;
			break;
	}

	return FALSE;
}


/*
 * Query the user for the pTable to display.
 */
BOOL GetTable(HWND hWndParent, LPTABLE pTable, LPBYTE TableUid)
{
	LPTABLECELL	Cell;
	int			Row;

	/* Query the user for the pTable, returning the row in the pTable of the selection. */
	Row = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GETTABLE), hWndParent, GetTableDialog, (LPARAM)pTable);

	/* If the row is -1, the user cancelled the selection. */
	if(Row == -1) {
		return FALSE;
	}

	/* Copy the pTable UID. */
	Cell = GetTableCell(pTable, Row, 0);
	if(Cell != NULL) {
		memcpy(TableUid, Cell->Bytes, 8);
	}

	/* Return TRUE indicating a valid selection. */
	return TRUE;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
