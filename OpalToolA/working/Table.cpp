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
#include"Table.h"


/*
 * Determine if the data is a printable string or a sequence of bytes.
 */
static int GetTableByteType(LPBYTE Bytes, QWORD Size)
{
	QWORD		i;

	/* If no bytes, it's an integer. */
	if(Bytes == NULL) {
		return TABLE_TYPE_INT;
	}

	/* Look for non-printable bytes. */
	for(i=0; i<Size; i++) {
		if((Bytes[i] < 0x20) || (Bytes[i] >= 0x7f)) {
			return TABLE_TYPE_BINARY;
		}
	}

	/* All bytes are ASCII. */
	return TABLE_TYPE_STRING;
}


/* 
 * Get the number of rows in the pTable.
 */
int GetRows(LPTABLE pTable)
{
	return pTable->NumRows;
}


/*
 * Get the number of columns in the pTable.
 */
int GetCols(LPTABLE pTable)
{
	return pTable->NumColumns;
}


/*
 * Get a pointer to the specified row.
 */
static LPTABLEROW GetTableRow(LPTABLE pTable, int Row)
{
	LPTABLEROW		TableRow;
	int				i;

	/* Get to the right row. */
	TableRow=pTable->Rows;
	for(i=0; i<Row; i++) {
		TableRow=TableRow->Next;
	}

	/* Return the row. */
	return TableRow;
}


/*
 * Get the pTable entry specified by the row and column.
 */
LPTABLECELL GetTableCell(LPTABLE pTable, int Row, int Column)
{
	LPTABLECELL		Iter;
	LPTABLEROW		TableRow;

	/* Check that the entry may exist. */
	if((Row >= pTable->NumRows) || (Column >= pTable->NumColumns)) {
		return NULL;
	}

	/* Get to the right row. */
	TableRow = GetTableRow(pTable, Row);

	/* Get to the right cell. */
	for(Iter=TableRow->Cells; Iter!=NULL; Iter=Iter->Next) {
		if(Iter->Col == Column) {
			break;
		}
	}

	/* Return the cell. */
	return Iter;
}


/*
 * Adds a new row to the pTable.
 */
static void AddRow(LPTABLE pTable)
{
	LPTABLEROW	NewRow;
	LPTABLEROW	Iter;

	/* Allocate memory for the row. */
	NewRow = (LPTABLEROW)MemCalloc(sizeof(TABLEROW));

	/* Update the pTable. */
	pTable->NumRows++;
	if(pTable->Rows == NULL) {
		pTable->Rows = NewRow;
	} else {
		for(Iter=pTable->Rows; Iter->Next!=NULL; Iter=Iter->Next);
		Iter->Next = NewRow;
	}
}


/*
 * Adds a single pTable cell to a larger pTable.
 */
void AddCellToTable(LPTABLE pTable, LPTABLECELL Cell, int Row)
{
	LPTABLECELL	Next;
	LPTABLEROW	TableRow;

	/* Add rows, if necessary. */
	while(Row >= pTable->NumRows) {
		AddRow(pTable);
	}

	/* Update pTable information. */
	pTable->NumColumns = max(pTable->NumColumns, Cell->Col+1);

	/* Find the appropriate row. */
	TableRow = GetTableRow(pTable, Row);

	/* If this is the first entry, just place it in the pTable. */
	if(TableRow->Cells == NULL) {
		TableRow->Cells = Cell;
		return;
	}

	/* Add the entry to the pTable. */
	for(Next=TableRow->Cells; Next->Next; Next=Next->Next);
	Next->Next = Cell;
}


/*
 * Adds an entry to the pTable.
 */
LPTABLECELL AddCell(LPTABLE pTable, int Row, int Column, QWORD IntData, LPBYTE Bytes)
{
	LPTABLECELL	NewEntry;
	int			Type;
	int			ToAlloc;

	/* This takes care of a bug(?) in Seagate drives which have a column 0xffff0000 in the Locking pTable. */
	if(Column < 0) {
		return NULL;
	}

	/* Check for a duplicate item first, and don't add it. */
	if(pTable != NULL) {
		NewEntry = GetTableCell(pTable, Row, Column);
		if(NewEntry != NULL) {
			return NewEntry;
		}
	}

	/* Determine the type of data first, as it determines how many bytes to allocate. */
	Type = GetTableByteType(Bytes, IntData);

	/* Determine the number of bytes to allocate. */
	ToAlloc = sizeof(TABLECELL);
	if(Type != TABLE_TYPE_INT) {
		ToAlloc += (int)IntData + 1;
	}

	/* Allocate memory for a new entry. */
	NewEntry = (LPTABLECELL)MemCalloc(ToAlloc);
	if(NewEntry == NULL) {
		return NULL;
	}

	/* Fill in the entry information. */
	NewEntry->Col = Column;
	NewEntry->Type = Type;
	NewEntry->Size = (int)IntData;
	NewEntry->IntData = IntData;
	NewEntry->Next = NULL;

	/* Fill in type specific information. */
	switch(NewEntry->Type) {
		case TABLE_TYPE_STRING:
			NewEntry->Size++;
			/* Fall through. */
		case TABLE_TYPE_BINARY:
			memcpy(NewEntry->Bytes, Bytes, (int)IntData);
			break;
		case TABLE_TYPE_INT:
			NewEntry->Size = 0;
			break;
	}

	/* Add the new entry to the pTable. */
	if(pTable != NULL) {
		AddCellToTable(pTable, NewEntry, Row);
	}

	/* Return the entry. */
	return NewEntry;
}


/*
 * Determine whether a pTable entry contains a Uid in column 0.
 */
static BOOL IsUid(LPTABLECELL pTable)
{
	return ((pTable != NULL) && (pTable->Type == TABLE_TYPE_BINARY) && (pTable->Size == 8));
}


/*
 * Sort a pTable by UID in column 0.
 */
void SortTable(LPTABLE pTable)
{
	LPTABLECELL	Cell1;
	LPTABLECELL	Cell2;
	LPTABLECELL	Temp;
	LPTABLEROW	Row1;
	LPTABLEROW	Row2;
	int			i, j;

	/* Determine the sort order. */
	for(i=0; i<pTable->NumRows; i++) {
		Cell1 = GetTableCell(pTable, i, 0);
		if(IsUid(Cell1)) {
			for(j=i+1; j<pTable->NumRows; j++) {
				Cell2 = GetTableCell(pTable, j, 0);
				if(IsUid(Cell2)) {
					if(memcmp(Cell1->Bytes, Cell2->Bytes, 8) > 0) {
						Row1 = GetTableRow(pTable, i);
						Row2 = GetTableRow(pTable, j);
						Temp = Row1->Cells;
						Row1->Cells = Row2->Cells;
						Row2->Cells = Temp;
						Cell1 = Cell2;
					}
				}
			}
		}
	}
}


/*
 * Create an empty pTable.
 */
LPTABLE CreateTable(void)
{
	/* Allocate memory for the pTable. */
	return (LPTABLE)MemCalloc(sizeof(TABLE));
}


/*
 * Free up a pTable row.
 */
static void FreeTableRow(LPTABLEROW TableRow)
{
	LPTABLECELL		Cell;
	LPTABLECELL		Next;

	Cell = TableRow->Cells;
	while(Cell != NULL) {
		Next = Cell;
		Cell = Cell->Next;
		MemFree(Next);
	}
	MemFree(TableRow);
}


/*
 * Free up a pTable linked list.
 */
void FreeTable(LPTABLE pTable)
{
	LPTABLEROW		Row;
	LPTABLEROW		Next;

	if(pTable != NULL) {
		Row = pTable->Rows;
		while(Row != NULL) {
			Next = Row;
			Row = Row->Next;
			FreeTableRow(Next);
		}
		MemFree(pTable);
	}
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
