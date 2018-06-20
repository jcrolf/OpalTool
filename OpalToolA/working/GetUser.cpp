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
#include"Utilities.h"
#include"Table.h"
#include"AtaDrive.h"
#include"Tcg.h"
#include"GetUser.h"
#include"Memory.h"
#include"Uid.h"
#include"resource.h"


typedef struct tagUserDialog {
	LPTCGAUTH	pTcgAuth;
	LPTABLE		pTable;
	LPTCGDRIVE	pTcgDrive;
	LPTSTR		pTitle;
} USERDIALOG, *LPUSERDIALOG;




/*
 * This is the dialog box message handler.
 */
static BOOL CALLBACK GetUserDialog(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LPUSERDIALOG	UserDlg;
	LPTABLECELL		Iter;
	LRESULT			Index;
	DWORD			TextLength;
	DWORD			i;
	TCHAR			AuthString[65];
	HWND			hWndCombo;
	BOOL			IsHex;
	BOOL			IsReverse;
	BOOL			IsMsid;
	BYTE			Temp;
	BYTE			Auth[32];
	int				NumRows;
	int				Row;
	int				j;


	switch(Msg) {
		case WM_INITDIALOG:
			UserDlg = (LPUSERDIALOG)lParam;
			SetWindowLong(hWnd, DWL_USER, (LONG)UserDlg);
			if(UserDlg->pTable != NULL) {
				hWndCombo = GetDlgItem(hWnd, IDC_COMBO1);
				Index = SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)_T("No User"));
				SendMessage(hWndCombo, CB_SETITEMDATA, Index, -2);
				NumRows = GetRows(UserDlg->pTable);
				for(j=0; j<NumRows; j++) {
					Iter = GetTableCell(UserDlg->pTable, j, 1);
					if(Iter != NULL) {
						Index = SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)(Iter->Bytes));
						SendMessage(hWndCombo, CB_SETITEMDATA, Index, j);
					}
				}
				SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);
			}
			if(UserDlg->pTitle != NULL) {
				SetWindowText(hWnd, UserDlg->pTitle);
			}
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
					UserDlg = (LPUSERDIALOG)GetWindowLong(hWnd, DWL_USER);
					IsMsid = SendDlgItemMessage(hWnd, IDC_USEMSID, BM_GETCHECK, 0, 0) == BST_CHECKED;
					if(IsMsid) {
						/* Read the MSID. */
						if(ReadMSID(UserDlg->pTcgDrive, UserDlg->pTcgAuth) == FALSE) {
							MessageBox(hWnd, _T("An error occurred reading the MSID."), NULL, MB_OK);
							break;
						}

						/* Indicate we have valid credentials. */
						UserDlg->pTcgAuth->IsValid = TRUE;
					} else {
						IsHex = SendDlgItemMessage(hWnd, IDC_HEX, BM_GETCHECK, 0, 0) == BST_CHECKED;
						IsReverse = SendDlgItemMessage(hWnd, IDC_REVERSE, BM_GETCHECK, 0, 0) == BST_CHECKED;
						TextLength = SendDlgItemMessage(hWnd, IDC_PASSWORD, WM_GETTEXTLENGTH, 0, 0);
						if(IsHex && (TextLength & 1)) {
							MessageBox(hWnd, _T("The hexadecimal string must be an even length."), NULL, MB_ICONERROR | MB_OK);
							break;
						}
						if(IsHex && (TextLength > 64)) {
							MessageBox(hWnd, _T("The hexadecimal string must be at most 64 characters."), NULL, MB_ICONERROR | MB_OK);
							break;
						}
						if(!IsHex && (TextLength > 32)) {
							MessageBox(hWnd, _T("The password must be at most 32 characters."), NULL, MB_ICONERROR | MB_OK);
							break;
						}
						GetDlgItemText(hWnd, IDC_PASSWORD, AuthString, TextLength+1);

						if(IsHex) {
							for(i=0; i<TextLength; i++) {
								if(ConvertHex(AuthString[i]) == 0xff) {
									MessageBox(hWnd, _T("The hexadecimal string contains invalid characters."), NULL, MB_ICONERROR | MB_OK);
									break;
								}
							}
							if(i != TextLength) break;

							TextLength /= 2;
							for(i=0; i<TextLength; i++) {
								Auth[i] = (ConvertHex(AuthString[2*i]) << 4) | ConvertHex(AuthString[2*i+1]);
							}
						} else {
							memcpy(Auth, AuthString, lstrlen(AuthString));
						}

						if(IsReverse) {
							for(i=0; i<TextLength/2; i++) {
								Temp = Auth[i];
								Auth[i] = Auth[TextLength-i-1];
								Auth[TextLength-i-1] = Temp;
							}
						}
						UserDlg->pTcgAuth->Size = TextLength;
						memcpy(UserDlg->pTcgAuth->Credentials, Auth, TextLength);
					}

					if(UserDlg->pTable != NULL) {
						hWndCombo = GetDlgItem(hWnd, IDC_COMBO1);
						Index = SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
						Row = SendMessage(hWndCombo, CB_GETITEMDATA, Index, 0);
					} else {
						Row = 0;
					}
					if(Row >= 0) {
						UserDlg->pTcgAuth->IsValid = TRUE;
					}
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
 * Given a pTable of valid SPs, query the user for the SP to authenticate to.
 */
static BOOL GetUser(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPTSTR pTitle, LPTABLE pTable, LPTCGAUTH pTcgAuth)
{
	LPTABLECELL		Cell;
	USERDIALOG		UserDlg;
	int				Row;

	/* Initialize the structure to pass to the dialog box. */
	UserDlg.pTcgAuth = pTcgAuth;
	UserDlg.pTable = pTable;
	UserDlg.pTcgDrive = pTcgDrive;
	UserDlg.pTitle = pTitle;

	/* Query the user for the drive user, returning the row in the pTable of the selection. */
	Row = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GETUSER), hWndParent, GetUserDialog, (LPARAM)&UserDlg);

	/* If the row is -1, the user cancelled the selection. */
	if(Row == -1) {
		return FALSE;
	}

	/* Copy the user ID. */
	Cell = GetTableCell(pTable, Row, 0);
	if(Cell != NULL) {
		memcpy(pTcgAuth->Authority, Cell->Bytes, 8);
	}

	/* Return TRUE indicating a valid selection. */
	return TRUE;
}


/*
 * Get a default user pTable when none can be read (like early Samsung SSDs)
 */
static LPTABLE GetDefaultUserTable(LPBYTE SpUid)
{
	LPTABLE		pTable;
	BYTE		Uid[8];

	/* Create an empty pTable. */
	pTable = CreateTable();
	if(pTable == NULL) {
		return NULL;
	}

	/* Handle Admin SP. */
	if(memcmp(SpUid, SP_ADMIN.Uid, 8) == 0) {
		AddCell(pTable, 0, 0, sizeof(AUTHORITY_ANYBODY.Uid), AUTHORITY_ANYBODY.Uid);
		AddCell(pTable, 1, 0, sizeof(AUTHORITY_ADMINS.Uid), AUTHORITY_ADMINS.Uid);
		AddCell(pTable, 2, 0, sizeof(AUTHORITY_MAKERS.Uid), AUTHORITY_MAKERS.Uid);
		AddCell(pTable, 3, 0, sizeof(AUTHORITY_SID.Uid), AUTHORITY_SID.Uid);
	/* Handle Locking SP. */
	} else if(memcmp(SpUid, SP_LOCKING.Uid, 8) == 0) {
		AddCell(pTable, 0, 0, sizeof(AUTHORITY_ANYBODY.Uid), AUTHORITY_ANYBODY.Uid);
		AddCell(pTable, 1, 0, sizeof(AUTHORITY_ADMINS.Uid), AUTHORITY_ADMINS.Uid);
		memcpy(Uid, AUTHORITY_ADMIN.Uid, sizeof(Uid));
		Uid[7] = 1;
		AddCell(pTable, 2, 0, sizeof(Uid), Uid);
		AddCell(pTable, 3, 0, sizeof(AUTHORITY_USERS.Uid), AUTHORITY_USERS.Uid);
		memcpy(Uid, AUTHORITY_USER.Uid, sizeof(Uid));
		Uid[7] = 1;
		AddCell(pTable, 4, 0, sizeof(Uid), Uid);
		Uid[7] = 2;
		AddCell(pTable, 5, 0, sizeof(Uid), Uid);
		Uid[7] = 3;
		AddCell(pTable, 6, 0, sizeof(Uid), Uid);
		Uid[7] = 4;
		AddCell(pTable, 7, 0, sizeof(Uid), Uid);
	}

	/* Add text descriptions. */
	AddTextDescriptions(pTable);

	/* Return the pTable. */
	return pTable;
}


/*
 * Get the User pTable from the SP.
 */
LPTABLE GetUserTable(LPTCGDRIVE pTcgDrive, LPBYTE SpUid)
{
	LPTABLE		pTable;

	/* Read the User pTable. */
	pTable = ReadTableNoSession(pTcgDrive, SpUid, TABLE_AUTHORITY.Uid, NULL);

	/* If we don't have a pTable, get a default one. */
	if(pTable == NULL) {
		pTable = GetDefaultUserTable(SpUid);
	}

	/* Return the pTable. */
	return pTable;
}


/*
 * Get the user and the user's authorization information.
 */
#ifdef DEBUG_EXTRA_INFO
#include <stdio.h>
#endif
// END TEST ONLY
BOOL GetUserAuthInfo(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPTSTR pTitle, LPTSTR Caption, LPTCGAUTH pTcgAuth)
{
	LPTABLE		UserTable;
	BOOL		Result;

	/* Get the list of users from the SP. */

#ifdef DEBUG_EXTRA_INFO
	        TCHAR output[256];
			for (int i = 0; i<8; i++)
				sprintf(&output[i*3],"%02x ",Sp[i]);
			MessageBox(hWndParent, _T(output), _T("debug display of Sp content"), MB_ICONERROR | MB_OK);
#endif

	UserTable = GetUserTable(pTcgDrive, Sp);
	if(UserTable == NULL) {
		MessageBox(hWndParent, _T("There was an error reading the user list from the SP."), Caption, MB_ICONERROR | MB_OK);
		return FALSE;
	}

	/* Initialize the credentials structure. */
	pTcgAuth->IsValid = FALSE;

	/* Query the user for the password. */
	Result = GetUser(hWndParent, pTcgDrive, pTitle, UserTable, pTcgAuth);

#ifdef DEBUG_EXTRA_INFO
			for (int i = 0; i<8; i++)
				sprintf(&output[i*3],"%02x ",pTcgAuth->Authority[i]);
			MessageBox(hWndParent, _T(output), _T("debug display of pTcgAuth->Authority content"), MB_ICONERROR | MB_OK);
#endif

	/* Free up the user pTable - we don't need it anymore. */
	FreeTable(UserTable);

	/* Return the result. */
	return Result;
}


/*
 * Get the password from the user.
 */
BOOL GetPassword(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPTCGAUTH pTcgAuth)
{
	USERDIALOG	UserDlg;
	int			Result;

	/* Set up the structure to pass to the dialog box. */
	UserDlg.pTcgAuth = pTcgAuth;
	UserDlg.pTcgDrive = pTcgDrive;
	UserDlg.pTable = NULL;
	UserDlg.pTitle = NULL;

	/* Initialize the credentials structure. */
	pTcgAuth->IsValid = FALSE;

	/* Query the user for the new password. */
	Result = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GETPASSWORD), hWndParent, GetUserDialog, (LPARAM)&UserDlg);

	/* If the result is -1, the user cancelled. */
	if(Result == -1) {
		return FALSE;
	}

	/* Return TRUE indicating a valid entry. */
	return TRUE;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
