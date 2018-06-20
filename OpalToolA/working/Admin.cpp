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
#include"Admin.h"


/* Internal admin status. */
#define ADMIN_STATUS_NORMAL			1
#define ADMIN_STATUS_ELEVATED		2
#define ADMIN_STATUS_NONELEVATED	3


/*
 * Determine whether the process is elevated.  If not, determine whether it can be elevated.
 */
static int GetAdminStatus(void)
{
	TOKEN_ELEVATION_TYPE	tet;
	TOKEN_ELEVATION			te;
	HANDLE					hToken;
	DWORD					dwReturnLength;


	/* Open a handle to our token. */
	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		return ADMIN_STATUS_NORMAL;
	}

	/* Get elevation type information. */
	GetTokenInformation(hToken, TokenElevationType, &tet, sizeof(tet), &dwReturnLength);
	GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &dwReturnLength);

	/* Close the handle to the token. */
	CloseHandle(hToken);

	/* The following can only happen if UAC is enabled and the user is an admin. */
	if(tet == TokenElevationTypeFull) {
		return ADMIN_STATUS_ELEVATED;
	}
	if(tet == TokenElevationTypeLimited) {
		return ADMIN_STATUS_NONELEVATED;
	}

	/* If we get here, either UAC is enabled and we're a normal user, or UAC is disabled. */
	if(te.TokenIsElevated) {
		return ADMIN_STATUS_ELEVATED;
	} else {
		return ADMIN_STATUS_NORMAL;
	}
}


/*
 * If we're not elevated, but can be, execute this process again at an elevated status and terminate this process.
 */
BOOL ElevateMe(void)
{
	STARTUPINFO		StartupInfo;
	DWORD			nShow;
	TCHAR			CurrentDirectory[MAX_PATH] = { 0 };
	TCHAR			CurrentExecutable[MAX_PATH] = { 0 };
	DWORD           nSize;

	/* Get our admin status. */
	switch(GetAdminStatus()) {
		case ADMIN_STATUS_ELEVATED:
			return TRUE;				/* Already elevated. */
			break;
		case ADMIN_STATUS_NORMAL:
			return FALSE;				/* Not elevated, can't elevate. */
			break;
	}

	/* Get our startup information so we can replicate it. */
	GetStartupInfo(&StartupInfo);
	nSize = GetCurrentDirectory(MAX_PATH, CurrentDirectory);
	if (!(nSize != 0 && nSize < MAX_PATH))
		return FALSE;
	nSize = GetModuleFileName(NULL, CurrentExecutable, MAX_PATH);
	if (!(nSize != 0))
		return FALSE;

	/* Determine the default window state. */
	if(StartupInfo.dwFlags & STARTF_USESHOWWINDOW) {
		nShow = StartupInfo.wShowWindow;
	} else {
		nShow = SW_NORMAL;
	}

	/* Execute. */
	ShellExecute(NULL, _T("runas"), CurrentExecutable, NULL, CurrentDirectory, nShow);

	/* Terminate ourselves. */
	ExitProcess(0);

	/* Make compiler happy. */
	return FALSE;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
