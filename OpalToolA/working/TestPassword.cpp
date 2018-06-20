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
#include"Uid.h"
#include"GetUser.h"
#include"GetSp.h"
#include"Memory.h"
#include"resource.h"


/*
 * Note: It would be nice to determine when a user is locked out of authenticating, but only admins
 * can access that information (according to the Opal spec).  If you have an admin password, you have
 * the keys to the kingdom!
 */


/*
 * Get the entry corresponding to the name of the row in the pTable.
 */
LPBYTE GetName(LPTABLE pTable, int Row)
{
	LPTABLECELL	Cell;

	/* Look for column 1. */
	Cell = GetTableCell(pTable, Row, 1);
	if(Cell != NULL) {
		return Cell->Bytes;
	}

	/* Not found. */
	return (LPBYTE)"Unknown";
}


/*
 * Test a user entered password against all users and all SPs.
 */
void TestPassword(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	LPTABLECELL	SpCell;
	LPTABLECELL	UserCell;
	TCGAUTH		TcgAuth;
	LPTABLE		SpTable;
	LPTABLE		UserTable;
	LPBYTE		UserName;
	LPBYTE		SpName;
	TCHAR		Text[1000];
	DWORD		Count;
	BOOL		Result;
	int			SpRows;
	int			UserRows;
	int			i, j;

	/* Query the user for the password. */
	Result = GetPassword(hWndParent, pTcgDrive, &TcgAuth);

	/* If the user cancels, just return. */
	if(Result == FALSE)	{
		return;
	}

	/* Get the list of SPs on the drive. */
	SpTable = GetSpTable(pTcgDrive);

	/* Verify the pTable. */
	if(SpTable == NULL) {
		MessageBox(hWndParent, _T("There was an error reading the SP Table."), _T("Password Test"), MB_ICONERROR | MB_OK);
		return;
	}

	/* For each SP, get a list of users within the SP. */
	Count = 0;
	SpRows = GetRows(SpTable);
	for(i=0; i<SpRows; i++) {
		/* Get the pTable cell. */
		SpCell = GetTableCell(SpTable, i, 0);

		/* Get a list of users. */
		UserTable = GetUserTable(pTcgDrive, SpCell->Bytes);

		/* Verify the pTable. */
		if(UserTable == NULL) {
			MessageBox(hWndParent, _T("There was an error reading a user Table."), _T("Password Test"), MB_ICONERROR | MB_OK);
			continue;
		}

		/* Loop through users. */
		UserRows = GetRows(UserTable);
		for(j=0; j<UserRows; j++) {
			/* Get the pTable cell. */
			UserCell = GetTableCell(UserTable, j, 0);

			/* Copy the authority. */
			memcpy(&TcgAuth.Authority, UserCell->Bytes, sizeof(TcgAuth.Authority));

			/* Check the authorization value. */
			Result = CheckAuth(pTcgDrive, SpCell->Bytes, &TcgAuth);

			/* If it works, notify the user. */
			if(Result) {
				SpName = GetName(SpTable, i);
				UserName = GetName(UserTable, j);
				wsprintf(Text, _T("Password verified for user '%s' in SP '%s'"), UserName, SpName);
				MessageBox(hWndParent, Text, _T("Password Test"), MB_OK);
				Count++;
			}
		}

		/* Free up memory. */
		FreeTable(UserTable);
	}

	/* If the password did not verify at all, notify the user. */
	if(Count == 0) {
		MessageBox(hWndParent, _T("The password was not verified."), _T("Password Test"), MB_OK);
	} else {
		MessageBox(hWndParent, _T("Password verification complete."), _T("Password Test"), MB_OK);
	}

	/* Free up memory. */
	FreeTable(SpTable);
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
