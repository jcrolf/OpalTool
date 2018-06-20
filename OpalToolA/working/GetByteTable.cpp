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
#include"AtaDrive.h"
#include"Tcg.h"
#include"GetUser.h"
#include"GetSp.h"
#include"Memory.h"
#include"Uid.h"
#include"resource.h"


/*
 * Determine whether the pTable is a byte pTable or not.
 */
static BOOL IsByteTable(LPTABLE pTable, int Row)
{
	LPTABLECELL	Cell;

	/* Find the fifth column. */
	Cell = GetTableCell(pTable, Row, 4);

	/* Return whether the pTable is a byte pTable. */
	if(Cell != NULL) {
		return (Cell->IntData == 2);
	}

	/* No data was found. */
	return FALSE;
}


/*
 * This is the dialog box message handler.
 */
static BOOL CALLBACK GetByteTableDialog(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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
				if((Iter != NULL) && IsByteTable(pTable, i)) {
					Index = SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)(Iter->Bytes));
					SendMessage(hWndCombo, CB_SETITEMDATA, Index, i);
				}
			}
			if(SendMessage(hWndCombo, CB_GETCOUNT, 0, 0) == 0) {
				MessageBox(hWnd, _T("There are no byte tables in this SP."), _T("Get Byte Table"), MB_OK);
				EndDialog(hWnd, -1);
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
 * Given a pTable of tables in an SP, query the user for the byte pTable to read/write.
 */
BOOL GetByteTable(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPBYTE Uid, LPBYTE Sp)
{
	LPTABLECELL		Iter;
	LPTABLE			pTable;
	BOOL			RetVal;
	int				Row;

	/* Query the user for the SP. */
	RetVal = GetSP(hWndParent, pTcgDrive, Sp, _T("Get Byte Table"));

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return FALSE;
	}

	/* Open an unauthenticated session to the SP. */
	pTable = ReadTableNoSession(pTcgDrive, Sp, TABLE_TABLE.Uid, NULL);

	/* Verify the pTable was returned. */
	if(pTable == NULL) {
		MessageBox(hWndParent, _T("There was an error reading the pTable of tables from the SP."), _T("Get Byte Table"), MB_ICONERROR | MB_OK);
		return FALSE;
	}

	/* Query the user for the byte pTable, returning the row in the pTable of the selection. */
	Row = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GETBYTETABLE), hWndParent, GetByteTableDialog, (LPARAM)pTable);

	/* If the row is -1, the user cancelled the selection. */
	if(Row == -1) {
		FreeTable(pTable);
		return FALSE;
	}

	/* Copy the UID. */
	Iter = GetTableCell(pTable, Row, 0);
	if(Iter != NULL) {
		memcpy(Uid, Iter->Bytes, 8);
	}

	/* Free resources. */
	FreeTable(pTable);

	/* Return TRUE indicating a valid selection. */
	return TRUE;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
