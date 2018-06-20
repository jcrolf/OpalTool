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

#include "stdafx.h"
#include "Memory.h"

HANDLE g_hHeap = NULL;


#ifdef _DEBUG

/* Magic check value. */
#define MEMORYMAGIC		0xdeadfeed

/* Keep track of the number of allocations. */
static int		g_Allocs = 0;
static HANDLE	hFile = INVALID_HANDLE_VALUE;
static char		FileMsg[1024];


/*
 * Add a line of text to the log file.
 */
static void AddLog(LPSTR String)
{
	DWORD	nWritten;

	if(hFile != INVALID_HANDLE_VALUE) {
		WriteFile(hFile, String, lstrlenA(String), &nWritten, NULL);
	}
}


/*
 * Add the file name and the line number to the log file.
 */
static void PrintFileAndLine(LPSTR File, int Line)
{
	AddLog("File ");
	AddLog(File);
	wsprintfA(FileMsg, ", line %d: ", Line);
	AddLog(FileMsg);
}


/*
 * Initialize memory.  If a file is specified, open it for logging.
 */
void DbgMemInit(LPTSTR FileName)
{
	SYSTEMTIME	Time;

	g_hHeap = GetProcessHeap();
	if(FileName != NULL) {
		hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
		if(hFile == INVALID_HANDLE_VALUE) {
			MessageBox(NULL, _T("Error opening memory debug file."), _T("Memory Initialization"), MB_OK);
		} else {
			SetFilePointer(hFile, 0, NULL, FILE_END);
		}
	}

	GetLocalTime(&Time);
	GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &Time, "'\r\nMemory initialized at 'h':'mm':'sstt 'on' ", FileMsg, sizeof(FileMsg)/sizeof(FileMsg[0]));
	AddLog(FileMsg);
	GetDateFormatA(LOCALE_USER_DEFAULT, 0, &Time, "ddd',' MMMM dd yyyy'.\r\n'", FileMsg, sizeof(FileMsg)/sizeof(FileMsg[0]));
	AddLog(FileMsg);
}


/*
 * Allocate memory.
 */
LPVOID DbgMemAlloc(DWORD Size, DWORD Flags, LPSTR File, int Line)
{
	LPBYTE	Ptr;

	/* Allocate memory, plus a little more for a check value. */
	Ptr = (LPBYTE)HeapAlloc(g_hHeap, Flags, Size + sizeof(DWORD));

	/* If memory was allocated, indicate this. */
	PrintFileAndLine(File, Line);
	if(Ptr != NULL) {
		g_Allocs++;
		*(LPDWORD)&Ptr[Size] = MEMORYMAGIC;
		wsprintfA(FileMsg, "%d bytes of memory allocated at 0x%08X.\r\n", Size, (long)(void *)Ptr);
	} else {
		wsprintfA(FileMsg, "%d bytes of memory was not allocated.\r\n", Size);
	}
	AddLog(FileMsg);

	/* Return the pointer. */
	return (LPVOID)Ptr;
}


/*
 * Free memory.
 */
void DbgMemFree(LPVOID Ptr, LPSTR File, int Line)
{
	LPBYTE	NewPtr;
	DWORD	Size;

	/* Make sure we're not freeing a NULL pointer. */
	PrintFileAndLine(File, Line);
	if(Ptr == NULL) {
		AddLog("Attempt to free a NULL pointer.\r\n");
		assert(Ptr != NULL);
		return;
	}

	/* Verify the check value. */
	Size = HeapSize(g_hHeap, 0, Ptr);
	NewPtr = (LPBYTE)Ptr;
	if((Size < 4) || (*(LPDWORD)&NewPtr[Size - sizeof(DWORD)] != MEMORYMAGIC)) {
		wsprintfA(FileMsg, "Memory at 0x%08X has overflowed.\r\n", (unsigned long)Ptr);
		AddLog(FileMsg);
		assert((Size >= 4) && (*(LPDWORD)&NewPtr[Size - sizeof(DWORD)] == MEMORYMAGIC));
	}

	/* Free the memory. */
	HeapFree(g_hHeap, 0, Ptr);
	wsprintfA(FileMsg, "Memory at 0x%08X has been freed.\r\n", (unsigned long)Ptr);
	AddLog(FileMsg);

	/* Change the number of allocations. */
	g_Allocs--;
}


/*
 * Close the handle to the file and verify all memory was released.
 */
void DbgMemCleanup()
{
	if(g_Allocs != 0) {
		wsprintfA(FileMsg, "There are %d allocations that were not released.\r\n", g_Allocs);
		AddLog(FileMsg);
		assert(g_Allocs == 0);
	}

	AddLog("Memory cleanup\r\n");
	if(hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
}


#endif

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
