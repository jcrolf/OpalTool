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
#include"GetByteTable.h"
#include"Uid.h"
#include"Memory.h"
#include"Progress.h"


/*
 * Save a byte Table to a file.
 */
static void SaveByteTableToFile(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPBYTE Uid, LPTCGAUTH pTcgAuth)
{
	OPENFILENAME	ofn;
	TCGSESSION		Session;
	HANDLE			hFile;
	LPBYTE			Buffer;
	DWORD			TableSize;
	DWORD			ReadSize;
	DWORD			BufferSize;
	DWORD			nWritten;
	DWORD			i;
	TCHAR			FileName[MAX_PATH];
	HWND			hWndDlg;
	BOOL			Result;


	/* Determine the size of the data store. */
	TableSize = GetByteTableSize(pTcgDrive, Sp, Uid);

	/* Determine, and allocate, a buffer. */
	BufferSize = min((pTcgDrive->BufferSize - 1024) & (~0x3ff), 63 * 1024);
	Buffer = (LPBYTE)MemAlloc(BufferSize);
	if(Buffer == NULL) {
		MessageBox(hWndParent, _T("Out of memory."), _T("Read Byte Table"), MB_ICONERROR | MB_OK);
		return;
	}

	/* Query the user for the file name to save the data store to. */
	memset(&ofn, 0, sizeof(ofn));
	FileName[0] = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if(GetSaveFileName(&ofn) == FALSE) {
		MemFree(Buffer);
		return;
	}

	/* Create the file. */
	hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		MessageBox(hWndParent, _T("There was an error creating the file."), _T("Read Byte Table"), MB_ICONERROR | MB_OK);
		MemFree(Buffer);
		return;
	}

	/* Authenticate to the Locking SP. */
	Result = StartSession(&Session, pTcgDrive, Sp, pTcgAuth);
	if(Result != 0) {
		MessageBox(hWndParent, _T("There was an error authenticating to the Locking SP."), _T("Read Byte Table"), MB_ICONERROR | MB_OK);
		CloseHandle(hFile);
		MemFree(Buffer);
		return;
	}

	/* Start the progress bar. */
	hWndDlg = CreateProgressBox(hWndParent, _T("Reading byte Table..."), TableSize);

	/* Samsung drives can't read the last byte of the MBR Table, so test for this and adjust accordingly. */
	Result = ReadByteTable(&Session, Uid, Buffer, TableSize-1, 1);
	if(Result != 0) {
		TableSize--;
	}

	/* Save the datastore. */
	for(i=0; i<TableSize; i+=BufferSize) {
		/* Has the user cancelled the operation? */
		if(IsProgressCancelled(hWndDlg) != FALSE) {
			break;
		}

		/* Update the progress bar. */
		UpdateProgressBox(hWndDlg, i);

		/* Process any messages for the progess dialog box. */
		ProcessMessages(hWndDlg);

		/* Read from the data store. */
		ReadSize = min(BufferSize, TableSize-i);
		Result = ReadByteTable(&Session, Uid, Buffer, i, ReadSize);
		if(Result != 0) {
			MessageBox(hWndParent, _T("There was an error reading the datastore."), _T("Read Byte Table"), MB_ICONERROR | MB_OK);
			break;
		}

		/* Write to the file. */
		nWritten = 0;
		Result = WriteFile(hFile, Buffer, ReadSize, &nWritten, NULL);
		if((Result == FALSE) || (nWritten != ReadSize)) {
			MessageBox(hWndParent, _T("There was an error writing to the file."), _T("Read Byte Table"), MB_ICONERROR | MB_OK);
			break;
		}
	}

	/* Terminate the progress bar. Wait a short amount of time so the user notices the completion. */
	UpdateProgressBox(hWndDlg, TableSize);
	Sleep(400);
	DestroyWindow(hWndDlg);

	/* Close open handles. */
	CloseHandle(hFile);

	/* Close sessions. */
	EndSession(&Session);

	/* Free memory. */
	MemFree(Buffer);
}


/*
 * Copy the contents of a file to a byte Table.
 */
static void WriteByteTableFromFile(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPBYTE Uid, LPTCGAUTH pTcgAuth)
{
	OPENFILENAME	ofn;
	TCGSESSION		Session;
	HANDLE			hFile;
	LPBYTE			Buffer;
	DWORD			TableSize;
	DWORD			FileSize;
	DWORD			BufferSize;
	DWORD			nRead;
	DWORD			i;
	TCHAR			FileName[MAX_PATH];
	HWND			hWndDlg;
	BOOL			Result;


	/* Determine the size of the data store. */
	TableSize = GetByteTableSize(pTcgDrive, Sp, Uid);

	/* Determine, and allocate, a buffer. */
	BufferSize = min((pTcgDrive->BufferSize - 1024) & (~0x3ff), 63 * 1024);
	Buffer = (LPBYTE)MemAlloc(BufferSize);
	if(Buffer == NULL) {
		MessageBox(hWndParent, _T("Out of memory."), _T("Read Byte Table"), MB_ICONERROR | MB_OK);
		MemFree(Buffer);
		return;
	}

	/* Query the user for the file name to save the data store to. */
	memset(&ofn, 0, sizeof(ofn));
	FileName[0] = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if(GetOpenFileName(&ofn) == FALSE) {
		MemFree(Buffer);
		return;
	}

	/* Open the file. */
	hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		MessageBox(hWndParent, _T("There was an error creating the file."), _T("Write Byte Table"), MB_ICONERROR | MB_OK);
		MemFree(Buffer);
		return;
	}

	/* Get the file size. */
	FileSize = GetFileSize(hFile, NULL);

	/* Check that the file can fit in the datastore. */
	if(FileSize > TableSize) {
		MessageBox(hWndParent, _T("The file is too big for the datastore."), _T("Write Byte Table"), MB_ICONWARNING | MB_OK);
		CloseHandle(hFile);
		MemFree(Buffer);
		return;
	}
	TableSize = FileSize;

	/* Authenticate to the Locking SP. */
	Result = StartSession(&Session, pTcgDrive, Sp, pTcgAuth);
	if(Result != 0) {
		MessageBox(hWndParent, _T("There was an error authenticating to the Locking SP."), _T("Write Byte Table"), MB_ICONERROR | MB_OK);
		CloseHandle(hFile);
		MemFree(Buffer);
		return;
	}

	/* Start the progress bar. */
	hWndDlg = CreateProgressBox(hWndParent, _T("Writing byte Table..."), TableSize);

	/* Write the datastore. */
	for(i=0; i<TableSize; i+=BufferSize) {
		/* Has the user cancelled the operation? */
		if(IsProgressCancelled(hWndDlg) != FALSE) {
			break;
		}

		/* Update the progress bar. */
		UpdateProgressBox(hWndDlg, i);

		/* Process any messages for the progess dialog box. */
		ProcessMessages(hWndDlg);

		/* Read from the file. */
		nRead = 0;
		Result = ReadFile(hFile, Buffer, BufferSize, &nRead, NULL);
		if(Result == FALSE) {
			MessageBox(hWndParent, _T("There was an error reading from the file."), _T("Write Byte Table"), MB_ICONERROR | MB_OK);
			break;
		}

		/* Write to the data store. */
		Result = WriteByteTable(&Session, Uid, Buffer, i, nRead);
		if(Result != 0) {
			MessageBox(hWndParent, _T("There was an error writing to the datastore."), _T("Write Byte Table"), MB_ICONERROR | MB_OK);
			break;
		}
	}

	/* Terminate the progress bar. Wait a short amount of time so the user notices the completion. */
	UpdateProgressBox(hWndDlg, TableSize);
	Sleep(400);
	DestroyWindow(hWndDlg);

	/* Close open handles. */
	CloseHandle(hFile);

	/* Close sessions. */
	EndSession(&Session);

	/* Free memory. */
	MemFree(Buffer);
}


/*
 * Save the byte pTable to a file.
 */
void ReadByteTableByUid(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPBYTE Uid, LPBYTE Sp)
{
	TCGAUTH	TcgAuth;
	BOOL	Result;

	/* Get the user and the user's auth info. */
	Result = GetUserAuthInfo(hWndParent, pTcgDrive, Sp, NULL, _T("Read Byte Table"), &TcgAuth);
	if(Result != FALSE) {
		/* Save the byte pTable to the file. */
		SaveByteTableToFile(hWndParent, pTcgDrive, Sp, Uid, &TcgAuth);
	}
}


/*
 * Write the data store from a file.
 */
void WriteByteTableByUid(HWND hWndParent, LPTCGDRIVE pTcgDrive, LPBYTE Uid, LPBYTE Sp)
{
	TCGAUTH	TcgAuth;
	BOOL	Result;

	/* Get the user and the user's auth info. */
	Result = GetUserAuthInfo(hWndParent, pTcgDrive, Sp, NULL, _T("Write Byte Table"), &TcgAuth);
	if(Result != FALSE) {
		/* Write a file to the pTable. */
		WriteByteTableFromFile(hWndParent, pTcgDrive, Sp, Uid, &TcgAuth);
	}
}


/*
 * Save the byte pTable to a file.
 */
void ReadArbitraryByteTable(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	BYTE	Uid[8];
	BYTE	Sp[8];
	BOOL	Result;

	/* Get the byte pTable to read. */
	Result = GetByteTable(hWndParent, pTcgDrive, Uid, Sp);
	if(Result == FALSE) {
		return;
	}

	/* Read the pTable. */
	ReadByteTableByUid(hWndParent, pTcgDrive, Uid, Sp);
}


/*
 * Write the data store from a file.
 */
void WriteArbitraryByteTable(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	BYTE	Uid[8];
	BYTE	Sp[8];
	BOOL	Result;

	/* Get the byte pTable to write. */
	Result = GetByteTable(hWndParent, pTcgDrive, Uid, Sp);
	if(Result == FALSE) {
		return;
	}

	/* Write the byte pTable. */
	WriteByteTableByUid(hWndParent, pTcgDrive, Uid, Sp);
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
