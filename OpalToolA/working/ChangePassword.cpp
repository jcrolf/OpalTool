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
#include "Resource.h"
#include "Memory.h"
#include "AtaDrive.h"
#include "Table.h"
#include "Tcg.h"
#include "ChangePassword.h"
#include "GetSp.h"
#include "GetUser.h"
#include "Uid.h"
#include "debugTrace.h"


// TEST ONLY
#ifdef OPALTOOLC
#include <iostream>
#include <string>
using namespace std;
string dumpTcgAuthX(LPTCGAUTH pTcgAuth)
{
	debugTrace t;

	string result="NULL";
	char printBuffer[256];
	if ((pTcgAuth!=NULL) && (t.On(t.TRACE_INFO)))
	{
		sprintf_s(printBuffer,"IsValid=%d\n",pTcgAuth->IsValid); result=string(printBuffer);
		sprintf_s(printBuffer,"Authority= "); result+=printBuffer;
		for (unsigned int i=0;i<8;i++) {sprintf_s(printBuffer,"%02x ",pTcgAuth->Authority[i]);result+=printBuffer;}
		result+="\n";
		sprintf_s(printBuffer,"Credentials= "); result+=printBuffer;
		for (unsigned int i=0;i<((pTcgAuth->Size<32) ? pTcgAuth->Size : 32);i++) {sprintf_s(printBuffer,"%02x ",pTcgAuth->Credentials[i]);result+=printBuffer;}
		result+="\n";
		sprintf_s(printBuffer,"Size=%d\n",pTcgAuth->Size);result+=printBuffer;
	}
	
    return result;
}
#endif
// END TEST ONLY

/* The caption to display on error messages. */
static LPTSTR	Caption = _T("ChangePassword");

BYTE ChangePassword(TCGSESSION &Session,TCGAUTH &TcgAuth, TCGAUTH &NewTcgAuth)
{
	/* Get the user's PIN table entry. */
	if(ReadTableCellBytes(&Session, NewTcgAuth.Authority, 10, NewTcgAuth.Authority, NULL) == FALSE) {
		/* If there was an error, just assume it's a user and use the appropriate credentials. */
		NewTcgAuth.Authority[3] = 0x0b;
	}

#ifdef OPALTOOLC
	debugTrace t;
	if (t.On(t.TRACE_INFO)) {
		std::cout << "ChangePassword: TcgAuth = " << dumpTcgAuthX(&TcgAuth) << "\n"; 
		std::cout << "ChangePassword: NewTcgAuth = " << dumpTcgAuthX(&NewTcgAuth) << "\n";
	}
#endif
	/* Set the user's password. */
	BYTE Result = ChangeAuth(&Session, &NewTcgAuth);

	return Result;
}
/*
 * Change the password for a user or admin authority.
 */
void ChangePassword(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	TCGSESSION	Session;
	TCGAUTH		NewTcgAuth;
	TCGAUTH		TcgAuth;
	BYTE		Sp[8];
	BYTE		Result;
	BOOL		RetVal;

	/* Query the user for the SP. */
	RetVal = GetSP(hWndParent, pTcgDrive, Sp, Caption);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Get the user to use for changing the password. */
	RetVal = GetUserAuthInfo(hWndParent, pTcgDrive, Sp, NULL, Caption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Get the new password information. */
	RetVal = GetUserAuthInfo(hWndParent, pTcgDrive, Sp, _T("Authority To Change Password"), Caption, &NewTcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Start an authorized session to the SP. */
	Result = StartSession(&Session, pTcgDrive, Sp, &TcgAuth);

	/* If there was a problem, return. */
	if(Result != 0) {
		MessageBox(hWndParent, _T("An error occurred opening a session to the SP."), Caption, MB_OK);
		return;
	}

	Result=ChangePassword(Session,TcgAuth,NewTcgAuth);
	
	/* Close the session. */
	EndSession(&Session);

	/* Display the result of the change. */
	if(Result == 0) {
		MessageBox(hWndParent, _T("The password has been changed."), Caption, MB_OK);
	} else {
		MessageBox(hWndParent, _T("There was an error changing the password."), Caption, MB_OK);
	}
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
