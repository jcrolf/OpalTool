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
#include"Atadrive.h"
#include"Table.h"
#include"Tcg.h"
#include"Uid.h"
#include"GetRange.h"
#include"Memory.h"


/*
 * Get the number of ranges, not including the global range.
 */
int GetNumberRanges(LPTCGSESSION Session)
{
	int			NumRanges;

	/* Early Samsung drives don't support anything other than the global range. */
	if (Session->pTcgDrive->IsSamsung != FALSE) {
		return 0;
	}

	/* Set the default value for the number of ranges. */
	NumRanges = 4;

	/* Read the cell containing the number of ranges. */
	ReadTableCellDword(Session, LOCKINGINFO.Uid, 4, (LPDWORD)&NumRanges);

	/* Return the value. */
	return NumRanges;
}


/*
 * Read a row of the locking pTable cell by cell.
 */
static void ReadEntireRow(LPTCGSESSION Session, LPBYTE RowUid, int Row, LPTABLE pTable)
{
	LPTABLECELL	NewEntry;
	int			i;

	/* Add the row UID to the pTable. */
	AddCell(pTable, Row, 0, 8, RowUid);

	/* Read each of the cells that contains variable information. */
	for(i=3; i<9; i++) {
		NewEntry = ReadTableCell(Session, RowUid, i);
		if(NewEntry != NULL) {
			AddCellToTable(pTable, NewEntry, Row);
		}
	}
}


/*
 * Read the range pTable row by row, if it's not accessible all at once.
 */
static LPTABLE ReadDefaultRangeTable(LPTCGSESSION Session)
{
	LPTABLE		pTable;
	BYTE		Uid[8];
	int			NumRanges;
	int			i;

	/* Determine the number of ranges we're dealing with. */
	NumRanges = GetNumberRanges(Session);

	/* Add in the global range. */
	pTable = CreateTable();
	ReadEntireRow(Session, LOCKING_GLOBALRANGE.Uid, 0, pTable);

	/* Add in the other ranges. */
	for(i=1; i<=NumRanges; i++) {
		memcpy(Uid, LOCKING_RANGE.Uid, sizeof(Uid));
		Uid[6] = (i >> 8) & 0xff;
		Uid[7] = (i >> 0) & 0xff;
		ReadEntireRow(Session, Uid, i, pTable);
	}

	/* Add text descriptions. */
	AddTextDescriptions(pTable);

	/* Return the pTable. */
	return pTable;
}


/*
 * Get the range pTable from the Locking SP.
 */
LPTABLE GetRangeTable(LPTCGDRIVE pTcgDrive, LPTCGAUTH pTcgAuth)
{
	TCGSESSION	Session;
	LPTABLE		pTable;
	BYTE		Result;

	/* Start an unauthorized session to the SP. */
	Result = StartSession(&Session, pTcgDrive, SP_LOCKING.Uid, pTcgAuth);

	/* If there was a problem, return. */
	if(Result != 0) {
		return NULL;
	}

	/* Read the range pTable. */
	pTable = ReadTable(&Session, TABLE_LOCKING.Uid);

	/* If there was an error, read the default pTable. */
	if(pTable == NULL) {
		pTable = ReadDefaultRangeTable(&Session);
	}

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Return the pTable. */
	return pTable;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
