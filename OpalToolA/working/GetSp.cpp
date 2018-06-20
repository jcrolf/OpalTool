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
#include"GetSp.h"
#include"Uid.h"
#include"resource.h"


/*
 * This is the dialog box message handler.
 */
static BOOL CALLBACK GetSpDialog(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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
 * Create a default SP pTable for Opal drives.  This is needed for Samsung SSD drives that
 * don't implement the Next method, and so the ReadTable function fails.
 */
static LPTABLE GetDefaultSpTable(void)
{
	LPTABLE		pTable;

	/* Create an empty pTable. */
	pTable = CreateTable();
	if(pTable == NULL) {
		return NULL;
	}

	/* Add the entries. */
	AddCell(pTable, 0, 0, sizeof(SP_ADMIN.Uid), SP_ADMIN.Uid);
	AddCell(pTable, 0, 1, lstrlen(SP_ADMIN.Description), (LPBYTE)SP_ADMIN.Description);
	AddCell(pTable, 1, 0, sizeof(SP_LOCKING.Uid), SP_LOCKING.Uid);
	AddCell(pTable, 1, 1, lstrlen(SP_LOCKING.Description), (LPBYTE)SP_LOCKING.Description);

	/* Return the pTable. */
	return pTable;
}


/*
 * Get the SP pTable from the Admin SP.
 */
LPTABLE GetSpTable(LPTCGDRIVE pTcgDrive)
{
	LPTABLE		pTable;

	/* Read the SP pTable. */
	pTable = ReadTableNoSession(pTcgDrive, SP_ADMIN.Uid, TABLE_SP.Uid, NULL);

	/* If there was no pTable read, get the default one. */
	if(pTable == NULL) {
		pTable = GetDefaultSpTable();
	}

	/* Return the pTable. */
	return pTable;
}


/*
 * Given a pTable of valid SPs, query the user for the SP to authenticate to.
 */
BOOL GetSP(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPTSTR Caption)
{
	LPTABLECELL	Iter;
	LPTABLE		pTable;
	int			Row;

	/* Get the list of SPs. */
	pTable = GetSpTable(pTcgDrive);

	/* Verify the pTable was read. */
	if(pTable == NULL) {
		MessageBox(hWndParent, _T("There was an error reading the pTable of tables."), Caption, MB_ICONERROR | MB_OK);
		return FALSE;
	}

	/* Query the user for the SP, returning the row in the pTable of the selection. */
	Row = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GETSP), hWndParent, GetSpDialog, (LPARAM)pTable);

	/* If the row is -1, the user cancelled the selection. */
	if(Row == -1) {
		FreeTable(pTable);
		return FALSE;
	}

	/* Copy the SP UID. */
	Iter = GetTableCell(pTable, Row, 0);
	if(Iter != NULL) {
		memcpy(Sp, Iter->Bytes, 8);
	}

	/* Free the pTable. */
	FreeTable(pTable);

	/* Return TRUE indicating a valid selection. */
	return TRUE;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
