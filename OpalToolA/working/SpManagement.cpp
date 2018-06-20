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
#include"AtaDrive.h"
#include"Table.h"
#include"Tcg.h"
#include"SpManagement.h"
#include"Uid.h"
#include"GetSp.h"
#include"GetUser.h"
#include"resource.h"


/* Captions for error messages. */
static LPTSTR	ActivateCaption = _T("Activate SP");
static LPTSTR	RevertCaption = _T("Revert SP");
static LPTSTR	RevertLockingCaption = _T("Revert Locking SP");
static LPTSTR	RevertDriveCaption = _T("Revert Drive");


/*
 * Activate an SP.
 */
void Activate(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	TCGSESSION		Session;
	TCGAUTH      TcgAuth;
	BOOL			RetVal;
	BYTE			Result;
	BYTE			Sp[8];


	/* Query the user for the SP to activate. */
	RetVal = GetSP(hWndParent, pTcgDrive, Sp, ActivateCaption);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Query the user for the drive user/admin to use. */
	RetVal = GetUserAuthInfo(hWndParent, pTcgDrive, SP_ADMIN.Uid, NULL, ActivateCaption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Start authenticated session to the Admin SP. */
	Result = StartSession(&Session, pTcgDrive, SP_ADMIN.Uid, &TcgAuth);

	/* If there was a problem, return. */
	if(Result != 0) {
		MessageBox(hWndParent, _T("An error occurred opening a session."), ActivateCaption, MB_OK);
		return;
	}

	/* Activate the SP. */
	RetVal = Activate_SP(&Session, Sp);

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Indicate success/error message. */
	if(RetVal != 0) {
		MessageBox(hWndParent, _T("There was an error activating the SP."), ActivateCaption, MB_OK);
	} else {
		MessageBox(hWndParent, _T("The SP was activated successfully."), ActivateCaption, MB_OK);
	}
}


/*
 * Revert an SP through the Admin SP's SP pTable.
 */
void DoRevert(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	TCGSESSION		Session;
	TCGAUTH      TcgAuth;
	BOOL			RetVal;
	BYTE			Result;
	BYTE			Sp[8];


	/* Query the user for the SP to revert. */
	RetVal = GetSP(hWndParent, pTcgDrive, Sp, RevertCaption);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Query the user for the drive user/admin to use. */
	RetVal = GetUserAuthInfo(hWndParent, pTcgDrive, SP_ADMIN.Uid, NULL, RevertCaption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Start authenticated session to the Admin SP. */
	Result = StartSession(&Session, pTcgDrive, SP_ADMIN.Uid, &TcgAuth);

	/* If there was a problem, return. */
	if(Result != 0) {
		MessageBox(hWndParent, _T("An error occurred opening a session."), RevertCaption, MB_OK);
		return;
	}

	/* Revert the SP. */
	RetVal = Revert(&Session, Sp);

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Indicate success/error message. */
	if(RetVal != 0) {
		MessageBox(hWndParent, _T("There was an error reverting the SP."), RevertCaption, MB_OK);
	} else {
		MessageBox(hWndParent, _T("The SP was successfully reverted."), RevertCaption, MB_OK);
	}
}


/*
 * Revert the Locking SP through the Locking SP.
 */
void DoRevertLockingSp(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	TCGSESSION		Session;
	TCGAUTH      TcgAuth;
	BOOL			RetVal;
	BOOL			KeepGlobalRange;
	BYTE			Result;


	/* Check that the Locking SP is enabled. */
	if (IsLockingEnabled(pTcgDrive) == FALSE) {
		MessageBox(hWndParent, _T("The Locking SP is not enabled."), RevertCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Query the user for the drive user/admin to use. */
	RetVal = GetUserAuthInfo(hWndParent, pTcgDrive, SP_LOCKING.Uid, NULL, RevertCaption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Start authenticated session to the Locking SP. */
	Result = StartSession(&Session, pTcgDrive, SP_LOCKING.Uid, &TcgAuth);

	/* If there was a problem, return. */
	if(Result != 0) {
		MessageBox(hWndParent, _T("An error occurred opening a session to the Locking SP."), RevertCaption, MB_OK);
		return;
	}

	/* Ask the user whether or not to keep the global range key. */
	KeepGlobalRange = (MessageBox(hWndParent, _T("Do you wish to keep the global range key?"), _T("Global Range Key"), MB_YESNO) == IDYES);

	/* Revert the drive. */
	RetVal = RevertSP(&Session, KeepGlobalRange);

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Indicate success/error message. */
	if(RetVal != 0) {
		MessageBox(hWndParent, _T("There was an error reverting the Locking SP."), RevertCaption, MB_OK);
	} else {
		MessageBox(hWndParent, _T("The Locking SP was successfully reverted."), RevertCaption, MB_OK);
	}
}


/*
 * Revert a drive to its Original Factory State (OFS)
 */
void RevertDrive(HWND hWndParent, LPTCGDRIVE pTcgDrive)
{
	TCGSESSION		Session;
	TCGAUTH			TcgAuth;
	BOOL			RetVal;
	BYTE			Result;


	/* Query the user for the drive user/admin to use. */
	RetVal = GetUserAuthInfo(hWndParent, pTcgDrive, SP_ADMIN.Uid, NULL, RevertCaption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Start authenticated session to the Admin SP. */
	Result = StartSession(&Session, pTcgDrive, SP_ADMIN.Uid, &TcgAuth);

	/* If there was a problem, return. */
	if(Result != 0) {
		MessageBox(hWndParent, _T("An error occurred opening a session."), RevertCaption, MB_OK);
		return;
	}

	/* Revert the drive. */
	RetVal = Revert(&Session, SP_ADMIN.Uid);

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Indicate success/error message. */
	if(RetVal != 0) {
		MessageBox(hWndParent, _T("There was an error reverting the drive."), RevertCaption, MB_OK);
	} else {
		MessageBox(hWndParent, _T("The drive was successfully reverted."), RevertCaption, MB_OK);
	}
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
