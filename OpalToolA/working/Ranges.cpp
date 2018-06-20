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
#include"Ranges.h"
#include"Uid.h"
#include"Memory.h"
#include"GetUser.h"
#include"GetRange.h"
#include"resource.h"


/* The caption for error messages. */
static LPTSTR ModifyCaption	= _T("Modify Ranges");
static LPTSTR EraseCaption = _T("Erase Ranges");
static LPTSTR LockCaption = _T("Lock/Unlock Ranges");


/* The structure to contain the range information. */
typedef struct tagRangeInfo {
	QWORD		Start;				/* The start of the range. */
	QWORD		Length;				/* The length of the range. */
	BOOL		ReadLockEnabled;		/* True if read lock is enabled. */
	BOOL		WriteLockEnabled;		/* True if write lock is enabled. */
	BOOL		ReadLocked;			/* True if locked for reading. */
	BOOL		WriteLocked;			/* True if locked for writing. */
} RANGEINFO, *LPRANGEINFO;


/* The structure to pass information to the dialog box. */
typedef struct tagUserDlg {
	LPTCGAUTH	pTcgAuth;			/* User authorization information. */
	LPTCGDRIVE	pTcgDrive;			/* Drive on which we are modifying ranges. */
	int			NumRanges;		/* The number of ranges, not including the global range. */
	LPRANGEINFO	RangeInfo;			/* Information on the ranges. */
	LPRANGEINFO	NewRangeInfo;			/* Altered information on the ranges. */
} UserDlg, *LPUserDlg;


/*
 * Similar to SetDlgItemInt, sets the edit control text to a 64-bit unsigned number.
 */
static void SetDlgItemInt64(HWND hWnd, int Item, QWORD Number)
{
	TCHAR	Text[21];

	/* Print the number to the text array. */
	wsprintf(Text, _T("%I64u"), Number);

	/* Set the item's text. */
	SendDlgItemMessage(hWnd, Item, WM_SETTEXT, 0, (LPARAM)Text);
}


/*
 * Determine whether a character is a digit.
 */
static BOOL IsDigit(TCHAR c)
{
	return ((c >= _T('0')) && (c <= _T('9')));
}


/*
 * Similar to GetDlgItemInt, gets the 64-bit unsigned number from the edit control.
 */
static QWORD GetDlgItemInt64(HWND hWnd, int Item)
{
	QWORD	Result;
	TCHAR	Text[21] = { 0 };
	int	i;

	/* Get the item's text. */
	SendDlgItemMessage(hWnd, Item, WM_GETTEXT, sizeof(Text)/sizeof(Text[0]), (LPARAM)Text);

	/* Convert the text to a number. */
	Result = 0;
	for(i=0; IsDigit(Text[i]); i++) {
		Result = (Result * 10) + (Text[i] - _T('0'));
	}

	/* Return the number. */
	return Result;
}


/*
 * Initializes the controls in the dialog box with the range information.
 */
static void InitControls(HWND hWnd, LPRANGEINFO RangeInfo, BOOL IsGlobal)
{
	int		State;

	/* Set the start and length fields. */
	SetDlgItemInt64(hWnd, IDC_START, RangeInfo->Start);
	SetDlgItemInt64(hWnd, IDC_LENGTH, RangeInfo->Length);

	/* Disable the edit controls for the global range, enable them for others. */
	EnableWindow(GetDlgItem(hWnd, IDC_START), !IsGlobal);
	EnableWindow(GetDlgItem(hWnd, IDC_LENGTH), !IsGlobal);

	/* Set the check marks. */
	if(RangeInfo->ReadLockEnabled == FALSE) {
		State = BST_UNCHECKED;
	} else {
		State = BST_CHECKED;
	}
	SendDlgItemMessage(hWnd, IDC_READLOCKENABLE, BM_SETCHECK, State, 0);
	if(RangeInfo->ReadLocked == FALSE) {
		State = BST_UNCHECKED;
	} else {
		State = BST_CHECKED;
	}
	SendDlgItemMessage(hWnd, IDC_READLOCKED, BM_SETCHECK, State, 0);

	if(RangeInfo->WriteLockEnabled == FALSE) {
		State = BST_UNCHECKED;
	} else {
		State = BST_CHECKED;
	}
	SendDlgItemMessage(hWnd, IDC_WRITELOCKENABLE, BM_SETCHECK, State, 0);
	if(RangeInfo->WriteLocked == FALSE) {
		State = BST_UNCHECKED;
	} else {
		State = BST_CHECKED;
	}
	SendDlgItemMessage(hWnd, IDC_WRITELOCKED, BM_SETCHECK, State, 0);
}


/*
 * Detemine whether a specific range has any changes.
 */
static BOOL HasRangeInfoChanged(LPUserDlg UserDlg, int Index)
{
	if(UserDlg->RangeInfo[Index].Start != UserDlg->NewRangeInfo[Index].Start) {
		return TRUE;
	}
	if(UserDlg->RangeInfo[Index].Length != UserDlg->NewRangeInfo[Index].Length) {
		return TRUE;
	}
	if(UserDlg->RangeInfo[Index].ReadLocked != UserDlg->NewRangeInfo[Index].ReadLocked) {
		return TRUE;
	}
	if(UserDlg->RangeInfo[Index].ReadLockEnabled != UserDlg->NewRangeInfo[Index].ReadLockEnabled) {
		return TRUE;
	}
	if(UserDlg->RangeInfo[Index].WriteLocked != UserDlg->NewRangeInfo[Index].WriteLocked) {
		return TRUE;
	}
	if(UserDlg->RangeInfo[Index].WriteLockEnabled != UserDlg->NewRangeInfo[Index].WriteLockEnabled) {
		return TRUE;
	}

	return FALSE;
}


/*
 * Enables or disables the apply button depending on whether there's any unsaved information.
 */
static void CheckApplyButton(HWND hWnd, LPUserDlg UserDlg)
{
	BOOL	Change;
	int		i;

	Change = FALSE;
	for(i=0; i<=(UserDlg->NumRanges) && (Change==FALSE); i++) {
		Change = HasRangeInfoChanged(UserDlg, i);
	}

	EnableWindow(GetDlgItem(hWnd, IDAPPLY), Change);
}


/*
 * Applies the changes that were made.
 */
static void DoApply(HWND hWnd)
{
	TCGSESSION	Session;
	LPUserDlg	UserDlg;
	TCHAR		Text[100];
	BYTE		Result1;
	BYTE		Result2;
	BYTE		Result3;
	BYTE		Result4;
	BYTE		Uid[8];
	int			i;

	/* Get the dialog information. */
	UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);

	/* Start a session. */
	Result1 = StartSession(&Session, UserDlg->pTcgDrive, SP_LOCKING.Uid, UserDlg->pTcgAuth);
	if(Result1 != 0) {
		MessageBox(hWnd, _T("There was an error opening a session to the Locking SP."), ModifyCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Check the global range. */
	if(HasRangeInfoChanged(UserDlg, 0) != FALSE) {
		Result1 = CreateRange(&Session, LOCKING_GLOBALRANGE.Uid, -1, -1, -1, UserDlg->NewRangeInfo[0].WriteLockEnabled);
		Result2 = CreateRange(&Session, LOCKING_GLOBALRANGE.Uid, -1, -1, UserDlg->NewRangeInfo[0].ReadLockEnabled, -1);
		if((Result1 != 0) || (Result2 != 0)) {
			MessageBox(hWnd, _T("There was an error modifying the Global Range information."), ModifyCaption, MB_ICONERROR | MB_OK);
		} else {
			UserDlg->RangeInfo[0].Start = UserDlg->NewRangeInfo[0].Start;
			UserDlg->RangeInfo[0].Length = UserDlg->NewRangeInfo[0].Length;
			UserDlg->RangeInfo[0].ReadLockEnabled = UserDlg->NewRangeInfo[0].ReadLockEnabled;
			UserDlg->RangeInfo[0].WriteLockEnabled = UserDlg->NewRangeInfo[0].WriteLockEnabled;
			if(UserDlg->NewRangeInfo[0].ReadLocked != UserDlg->RangeInfo[0].ReadLocked) {
				Result1 = SetReadLock(&Session, LOCKING_GLOBALRANGE.Uid, UserDlg->NewRangeInfo[0].ReadLocked);
				if(Result1 != 0) {
					wsprintf(Text, _T("There was an error setting the ReadLocked state for the Global Range."));
					MessageBox(hWnd, Text, ModifyCaption, MB_ICONERROR | MB_OK);
				} else {
					UserDlg->RangeInfo[0].ReadLocked = UserDlg->NewRangeInfo[0].ReadLocked;
				}
			}
			if(UserDlg->NewRangeInfo[0].WriteLocked != UserDlg->RangeInfo[0].WriteLocked) {
				Result1 = SetWriteLock(&Session, LOCKING_GLOBALRANGE.Uid, UserDlg->NewRangeInfo[0].WriteLocked);
				if(Result1 != 0) {
					wsprintf(Text, _T("There was an error setting the WriteLocked state for the Global Range."));
					MessageBox(hWnd, Text, ModifyCaption, MB_ICONERROR | MB_OK);
				} else {
					UserDlg->RangeInfo[0].WriteLocked = UserDlg->NewRangeInfo[0].WriteLocked;
				}
			}
		}
	}

	/* Check additional ranges. */
	for(i=1; i<=UserDlg->NumRanges; i++) {
		if(HasRangeInfoChanged(UserDlg, i) != FALSE) {
			memcpy(Uid, LOCKING_RANGE.Uid, sizeof(Uid));
			Uid[6] = (i >> 8) & 0xff;
			Uid[7] = i & 0xff;
			Result1 = CreateRange(&Session, Uid, UserDlg->NewRangeInfo[i].Start, -1, -1, -1);
			Result2 = CreateRange(&Session, Uid, -1, UserDlg->NewRangeInfo[i].Length, -1, -1);
			Result3 = CreateRange(&Session, Uid, -1, -1, UserDlg->NewRangeInfo[i].ReadLockEnabled, -1);
			Result4 = CreateRange(&Session, Uid, -1, -1, -1, UserDlg->NewRangeInfo[i].WriteLockEnabled);
			if((Result1 != 0) || (Result2 != 0) || (Result3 != 0) || (Result4 != 0)) {
				wsprintf(Text, _T("There was an error modifying Range %d information."), i);
				MessageBox(hWnd, Text, ModifyCaption, MB_ICONERROR | MB_OK);
				continue;
			}
			UserDlg->RangeInfo[i].Start = UserDlg->NewRangeInfo[i].Start;
			UserDlg->RangeInfo[i].Length = UserDlg->NewRangeInfo[i].Length;
			UserDlg->RangeInfo[i].ReadLockEnabled = UserDlg->NewRangeInfo[i].ReadLockEnabled;
			UserDlg->RangeInfo[i].WriteLockEnabled = UserDlg->NewRangeInfo[i].WriteLockEnabled;
			if(UserDlg->NewRangeInfo[i].ReadLocked != UserDlg->RangeInfo[i].ReadLocked) {
				Result1 = SetReadLock(&Session, Uid, UserDlg->NewRangeInfo[i].ReadLocked);
				if(Result1 != 0) {
					wsprintf(Text, _T("There was an error setting the ReadLocked state for Range %d."), i);
					MessageBox(hWnd, Text, ModifyCaption, MB_ICONERROR | MB_OK);
				} else {
					UserDlg->RangeInfo[i].ReadLocked = UserDlg->NewRangeInfo[i].ReadLocked;
				}
			}
			if(UserDlg->NewRangeInfo[i].WriteLocked != UserDlg->RangeInfo[i].WriteLocked) {
				Result1 = SetWriteLock(&Session, Uid, UserDlg->NewRangeInfo[i].WriteLocked);
				if(Result1 != 0) {
					wsprintf(Text, _T("There was an error setting the WriteLocked state for Range %d."), i);
					MessageBox(hWnd, Text, ModifyCaption, MB_ICONERROR | MB_OK);
				} else {
					UserDlg->RangeInfo[i].WriteLocked = UserDlg->NewRangeInfo[i].WriteLocked;
				}
			}
		}
	}

	/* End the session. */
	EndSession(&Session);

	/* Disable the button, if all changes were saved. */
	CheckApplyButton(hWnd, UserDlg);
}


/*
 * Erase a range.
 */
static void DoErase(HWND hWnd, LPUserDlg UserDlg, int Index)
{
	TCGSESSION	Session;
	DWORD		Ignored;
	BYTE		Result;
	BYTE		KeyUid[8];
	BYTE		Uid[8];

	/* Start an authorized session to the Locking SP. */
	Result = StartSession(&Session, UserDlg->pTcgDrive, SP_LOCKING.Uid, UserDlg->pTcgAuth);
	if(Result != 0) {
		MessageBox(hWnd, _T("Unable to open a session to the Locking SP."), EraseCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Attempt to get the key UID from the appropriate pTable. At the same time, set up the key uid in case we need it later. */
	if(Index == 0) {
		memcpy(Uid, LOCKING_GLOBALRANGE.Uid, 8);
		memcpy(KeyUid, K_AES_128_GLOBALRANGEKEY.Uid, 8);
	} else {
		memcpy(Uid, LOCKING_RANGE.Uid, 6);
		Uid[6] = (Index >> 8) & 0xff;
		Uid[7] = (Index >> 0) & 0xff;
		memcpy(KeyUid, K_AES_128_RANGEKEY.Uid, 8);
		KeyUid[6] = (Index >> 8) & 0xff;
		KeyUid[7] = (Index >> 0) & 0xff;
	}

	/* Read the key uid.  If not successful, determine whether we need the AES 256 key. */
	if(ReadTableCellBytes(&Session, Uid, 10, KeyUid, NULL) == FALSE) {
		if(ReadTableCellDword(&Session, K_AES_256_GLOBALRANGEKEY.Uid, 4, &Ignored) != FALSE) {
			/* We only need the pTable Uid portion of the Uid. */
			memcpy(KeyUid, K_AES_256_GLOBALRANGEKEY.Uid, 4);
		}
	}

	/* Cryptoerase the range. */
	Result = EraseRange(&Session, KeyUid);

	/* End the session. */
	EndSession(&Session);

	/* Notify the user of the result. */
	if(Result != 0) {
		MessageBox(hWnd, _T("There was an error erasing the range."), EraseCaption, MB_ICONERROR | MB_OK);
	} else {
		MessageBox(hWnd, _T("The range has been erased."), EraseCaption, MB_OK);
	}
}


/*
 * Lock or unlock a range.
 */
static void DoLock(HWND hWnd, LPUserDlg UserDlg, int Index, BOOL ReadLock, BOOL ReadUnlock, BOOL WriteLock, BOOL WriteUnlock)
{
	TCGSESSION	Session;
	BYTE		Result;
	BYTE		RangeUid[8];

	/* Convert the index to a Range Uid. */
	if(Index == 0) {
		memcpy(RangeUid, LOCKING_GLOBALRANGE.Uid, 8);
	} else {
		memcpy(RangeUid, LOCKING_RANGE.Uid, 6);
		RangeUid[6] = (Index >> 8) & 0xff;
		RangeUid[7] = (Index >> 0) & 0xff;
	}

	/* Start an authorized session to the Locking SP. */
	Result = StartSession(&Session, UserDlg->pTcgDrive, SP_LOCKING.Uid, UserDlg->pTcgAuth);
	if(Result != 0) {
		MessageBox(hWnd, _T("Unable to open a session to the Locking SP."), LockCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Perform the requested locking/unlocking functions. */
	if(ReadLock != FALSE) {
		Result = SetReadLock(&Session, RangeUid, TRUE);
		if(Result != 0) {
			MessageBox(hWnd, _T("There was an error locking the range for reading."), LockCaption, MB_ICONERROR | MB_OK);
		} else {
			MessageBox(hWnd, _T("The range has been locked for reading."), LockCaption, MB_OK);
		}
	}
	if(ReadUnlock != FALSE) {
		Result = SetReadLock(&Session, RangeUid, FALSE);
		if(Result != 0) {
			MessageBox(hWnd, _T("There was an error unlocking the range for reading."), LockCaption, MB_ICONERROR | MB_OK);
		} else {
			MessageBox(hWnd, _T("The range has been unlocked for reading."), LockCaption, MB_OK);
		}
	}
	if(WriteLock != FALSE) {
		Result = SetWriteLock(&Session, RangeUid, TRUE);
		if(Result != 0) {
			MessageBox(hWnd, _T("There was an error locking the range for writing."), LockCaption, MB_ICONERROR | MB_OK);
		} else {
			MessageBox(hWnd, _T("The range has been locked for writing."), LockCaption, MB_OK);
		}
	}
	if(WriteUnlock != FALSE) {
		Result = SetWriteLock(&Session, RangeUid, FALSE);
		if(Result != 0) {
			MessageBox(hWnd, _T("There was an error unlocking the range for writing."), LockCaption, MB_ICONERROR | MB_OK);
		} else {
			MessageBox(hWnd, _T("The range has been unlocked for writing."), LockCaption, MB_OK);
		}
	}

	/* End the session. */
	EndSession(&Session);
}


/*
 * This is the dialog function for displaying and altering range information.
 */
static BOOL CALLBACK RangeFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LPUserDlg	UserDlg;
	TCHAR		Text[100];
	int			Reply;
	int			Index;
	int			i;

	switch(Msg) {
		case WM_INITDIALOG:
			/* Get the dialog information. */
			UserDlg = (LPUserDlg)lParam;

			/* Save the dialog information for future use. */
			SetWindowLong(hWnd, DWL_USER, (LONG_PTR)UserDlg);

			/* Restrict the input to 19 digits, just under 2^63. */
			SendDlgItemMessage(hWnd, IDC_START, EM_LIMITTEXT, 19, 0);
			SendDlgItemMessage(hWnd, IDC_LENGTH, EM_LIMITTEXT, 19, 0);

			/* Initialize the combo box. */
			SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_ADDSTRING, 0, (LPARAM)_T("Global Range"));
			for(i=1; i<=UserDlg->NumRanges; i++) {
				wsprintf(Text, _T("Range #%d"), i);
				SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_ADDSTRING, 0, (LPARAM)Text);
			}
			SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_SETCURSEL, 0, 0);

			/* Initialize the rest of the controls. */
			InitControls(hWnd, &(UserDlg->NewRangeInfo[0]), TRUE);

			/* Disable the button. */
			EnableWindow(GetDlgItem(hWnd, IDAPPLY), FALSE);

			/* Return true to set the focus to the first control. */
			return TRUE;
			break;
		case WM_SYSCOMMAND:
			switch(wParam & 0xfff0) {
				case SC_CLOSE:
					SendMessage(hWnd, WM_COMMAND, IDCANCEL, 0);
					return TRUE;
					break;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					if(IsWindowEnabled(GetDlgItem(hWnd, IDAPPLY)) != FALSE) {
						DoApply(hWnd);
					}
					EndDialog(hWnd, 0);
					break;
				case IDCANCEL:
					Reply = IDYES;
					if(IsWindowEnabled(GetDlgItem(hWnd, IDAPPLY)) != FALSE) {
						Reply = MessageBox(hWnd, _T("Do you want to save your changes?"), ModifyCaption, MB_YESNOCANCEL | MB_ICONWARNING);
						if(Reply == IDYES) {
							DoApply(hWnd);
						}
					}
					if(Reply != IDCANCEL) {
						EndDialog(hWnd, 0);
					}
					break;
				case IDAPPLY:
					DoApply(hWnd);
					break;
				case IDC_READLOCKENABLE:
					UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
					Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
					UserDlg->NewRangeInfo[Index].ReadLockEnabled = SendDlgItemMessage(hWnd, IDC_READLOCKENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED;
					if(UserDlg->NewRangeInfo[Index].ReadLockEnabled != FALSE) {
						UserDlg->NewRangeInfo[Index].ReadLockEnabled = TRUE;
					}
					CheckApplyButton(hWnd, UserDlg);
					break;
				case IDC_READLOCKED:
					UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
					Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
					UserDlg->NewRangeInfo[Index].ReadLocked = SendDlgItemMessage(hWnd, IDC_READLOCKED, BM_GETCHECK, 0, 0) == BST_CHECKED;
					if(UserDlg->NewRangeInfo[Index].ReadLocked != FALSE) {
						UserDlg->NewRangeInfo[Index].ReadLocked = TRUE;
					}
					CheckApplyButton(hWnd, UserDlg);
					break;
				case IDC_WRITELOCKENABLE:
					UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
					Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
					UserDlg->NewRangeInfo[Index].WriteLockEnabled = SendDlgItemMessage(hWnd, IDC_WRITELOCKENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED;
					if(UserDlg->NewRangeInfo[Index].WriteLockEnabled != FALSE) {
						UserDlg->NewRangeInfo[Index].WriteLockEnabled = TRUE;
					}
					CheckApplyButton(hWnd, UserDlg);
					break;
				case IDC_WRITELOCKED:
					UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
					Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
					UserDlg->NewRangeInfo[Index].WriteLocked = SendDlgItemMessage(hWnd, IDC_WRITELOCKED, BM_GETCHECK, 0, 0) == BST_CHECKED;
					if(UserDlg->NewRangeInfo[Index].WriteLocked != FALSE) {
						UserDlg->NewRangeInfo[Index].WriteLocked = TRUE;
					}
					CheckApplyButton(hWnd, UserDlg);
					break;
				case IDC_START:
					UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
					Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
					UserDlg->NewRangeInfo[Index].Start = GetDlgItemInt64(hWnd, IDC_START);
					CheckApplyButton(hWnd, UserDlg);
					break;
				case IDC_LENGTH:
					UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
					Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
					UserDlg->NewRangeInfo[Index].Length = GetDlgItemInt64(hWnd, IDC_LENGTH);
					CheckApplyButton(hWnd, UserDlg);
					break;
				case IDC_LISTRANGES:
					switch(HIWORD(wParam)) {
						case CBN_SELCHANGE:
							UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
							Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
							InitControls(hWnd, &(UserDlg->NewRangeInfo[Index]), Index==0);
							break;
					}
					break;
			}
			break;
	}

	return FALSE;
}


/*
 * This is the dialog function for crypt erasing ranges.
 */
static BOOL CALLBACK EraseRangeFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LPUserDlg	UserDlg;
	TCHAR		Text[100];
	int			Reply;
	int			Index;
	int			i;

	switch(Msg) {
		case WM_INITDIALOG:
			/* Get the dialog information. */
			UserDlg = (LPUserDlg)lParam;

			/* Save the dialog information for future use. */
			SetWindowLong(hWnd, DWL_USER, (LONG_PTR)UserDlg);

			/* Initialize the combo box. */
			SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_ADDSTRING, 0, (LPARAM)_T("Global Range"));
			for(i=1; i<=UserDlg->NumRanges; i++) {
				wsprintf(Text, _T("Range #%d"), i);
				SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_ADDSTRING, 0, (LPARAM)Text);
			}
			SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_SETCURSEL, 0, 0);

			/* Return true to set the focus to the first control. */
			return TRUE;
			break;
		case WM_SYSCOMMAND:
			switch(wParam & 0xfff0) {
				case SC_CLOSE:
					SendMessage(hWnd, WM_COMMAND, IDCANCEL, 0);
					return TRUE;
					break;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDERASE:
					Reply = MessageBox(hWnd, _T("Do you want to erase this range?"), EraseCaption, MB_YESNO | MB_ICONWARNING);
					if(Reply == IDYES) {
						UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
						Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
						DoErase(hWnd, UserDlg, Index);
					}
					break;
				case IDCANCEL:
					EndDialog(hWnd, 0);
					break;
			}
			break;
	}

	return FALSE;
}


/*
 * This is the dialog function for locking or unlocking a range.
 */
static BOOL CALLBACK LockRangeFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LPUserDlg	UserDlg;
	TCHAR		Text[100];
	BOOL		ReadLock;
	BOOL		ReadUnlock;
	BOOL		WriteLock;
	BOOL		WriteUnlock;
	int			Index;
	int			i;

	switch(Msg) {
		case WM_INITDIALOG:
			/* Get the dialog information. */
			UserDlg = (LPUserDlg)lParam;

			/* Save the dialog information for future use. */
			SetWindowLong(hWnd, DWL_USER, (LONG_PTR)UserDlg);

			/* Initialize the combo box. */
			SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_ADDSTRING, 0, (LPARAM)_T("Global Range"));
			for(i=1; i<=UserDlg->NumRanges; i++) {
				wsprintf(Text, _T("Range #%d"), i);
				SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_ADDSTRING, 0, (LPARAM)Text);
			}
			SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_SETCURSEL, 0, 0);

			/* Return true to set the focus to the first control. */
			return TRUE;
			break;
		case WM_SYSCOMMAND:
			switch(wParam & 0xfff0) {
				case SC_CLOSE:
					SendMessage(hWnd, WM_COMMAND, IDCANCEL, 0);
					return TRUE;
					break;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					UserDlg = (LPUserDlg)GetWindowLong(hWnd, DWL_USER);
					Index = SendDlgItemMessage(hWnd, IDC_LISTRANGES, CB_GETCURSEL, 0, 0);
					ReadLock = SendDlgItemMessage(hWnd, IDC_READLOCK, BM_GETCHECK, 0, 0) == BST_CHECKED;
					ReadUnlock = SendDlgItemMessage(hWnd, IDC_READUNLOCK, BM_GETCHECK, 0, 0) == BST_CHECKED;
					WriteLock = SendDlgItemMessage(hWnd, IDC_WRITELOCK, BM_GETCHECK, 0, 0) == BST_CHECKED;
					WriteUnlock = SendDlgItemMessage(hWnd, IDC_WRITEUNLOCK, BM_GETCHECK, 0, 0) == BST_CHECKED;
					DoLock(hWnd, UserDlg, Index, ReadLock, ReadUnlock, WriteLock, WriteUnlock);
					SendDlgItemMessage(hWnd, IDC_READLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					SendDlgItemMessage(hWnd, IDC_READUNLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					SendDlgItemMessage(hWnd, IDC_WRITELOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					SendDlgItemMessage(hWnd, IDC_WRITEUNLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					break;
				case IDCANCEL:
					EndDialog(hWnd, 0);
					break;
				case IDC_READLOCK:
					if(SendDlgItemMessage(hWnd, IDC_READLOCK, BM_GETCHECK, 0, 0) == BST_CHECKED) {
						SendDlgItemMessage(hWnd, IDC_READUNLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					}
					break;
				case IDC_READUNLOCK:
					if(SendDlgItemMessage(hWnd, IDC_READUNLOCK, BM_GETCHECK, 0, 0) == BST_CHECKED) {
						SendDlgItemMessage(hWnd, IDC_READLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					}
					break;
				case IDC_WRITELOCK:
					if(SendDlgItemMessage(hWnd, IDC_WRITELOCK, BM_GETCHECK, 0, 0) == BST_CHECKED) {
						SendDlgItemMessage(hWnd, IDC_WRITEUNLOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					}
					break;
				case IDC_WRITEUNLOCK:
					if(SendDlgItemMessage(hWnd, IDC_WRITEUNLOCK, BM_GETCHECK, 0, 0) == BST_CHECKED) {
						SendDlgItemMessage(hWnd, IDC_WRITELOCK, BM_SETCHECK, BST_UNCHECKED, 0);
					}
					break;
			}
			break;
	}

	return FALSE;
}


/*
 * Convert a range UID to an index: Global = 0, Range1 = 1, etc
 */
static int GetRangeIndex(LPBYTE Uid)
{
	int		Range;

	/* Check for the global range first. */
	if(memcmp(Uid, LOCKING_GLOBALRANGE.Uid, 8) == 0) {
		return 0;
	}

	/* Check for the other ranges. */
	if(memcmp(Uid, LOCKING_RANGE.Uid, 6) != 0) {
		return -1;
	}

	/* Determine the range number. */
	Range = ((int)Uid[6] << 8) + Uid[7];

	/* Return the range. */
	return Range;
}


/*
 * Get the number of ranges, not including the global range.
 */
static int GetNumberOfRanges(HWND hWnd, LPTCGDRIVE pTcgDrive, LPTSTR Caption)
{
	TCGSESSION	Session;
	BYTE		Result;
	int			NumRanges;

	/* Start an unauthenticated session to the Locking SP. */
	Result = StartSession(&Session, pTcgDrive, SP_LOCKING.Uid, NULL);
	if(Result != 0) {
		return 4;
	}

	/* Get the number of ranges. */
	NumRanges = GetNumberRanges(&Session);

	/* End the session. */
	EndSession(&Session);

	/* Return the value. */
	return NumRanges;
}



/*
 * Create or modify any number of ranges.
 */
void ModifyRanges(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	LPRANGEINFO		NewRangeInfo;
	LPRANGEINFO		RangeInfo;
	LPTABLECELL		Iter;
	LPTABLECELL		Cell;
	TCGAUTH			TcgAuth;
	UserDlg			UserDlg;
	LPTABLE			pTable;
	BYTE			RetVal;
	int			NumRanges;
	int			Index;
	int			Rows;
	int			i;

	/* Check that the Locking SP is enabled. */
	if(IsLockingEnabled(pTcgDrive) == FALSE) {
		MessageBox(hWnd, _T("The Locking SP is not enabled."), ModifyCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Get the number of ranges. */
	NumRanges = GetNumberOfRanges(hWnd, pTcgDrive, ModifyCaption);

	/* Query the user for the drive user/admin to use. */
	RetVal = GetUserAuthInfo(hWnd, pTcgDrive, SP_LOCKING.Uid, NULL, ModifyCaption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Get the range information. */
	pTable = GetRangeTable(pTcgDrive, &TcgAuth);

	/* Check that a pTable was returned. */
	if(pTable == NULL) {
		MessageBox(hWnd, _T("There was an error reading range information."), ModifyCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Check that we read enough - some users can read minimal information. */
	if((GetRows(pTable) <= NumRanges) || (GetCols(pTable) < 9)) {
		MessageBox(hWnd, _T("There was an error reading range information."), ModifyCaption, MB_ICONERROR | MB_OK);
		FreeTable(pTable);
		return;
	}

	/* Allocate memory for the range information. */
	RangeInfo = (LPRANGEINFO)MemAlloc((NumRanges + 1) * sizeof(RANGEINFO));
	NewRangeInfo = (LPRANGEINFO)MemAlloc((NumRanges + 1) * sizeof(RANGEINFO));
	if((RangeInfo == NULL) || (NewRangeInfo == NULL)) {
		MessageBox(hWnd, _T("The system is low on resources - out of memory."), ModifyCaption, MB_ICONERROR | MB_OK);
		FreeTable(pTable);
		if(RangeInfo != NULL) {
			MemFree(RangeInfo);
		}
		if(NewRangeInfo != NULL) {
			MemFree(NewRangeInfo);
		}
		return;
	}

	/* Parse the pTable into the range information structure. */
	Rows = GetRows(pTable);
	for(i=0; i<Rows; i++) {
		Iter = GetTableCell(pTable, i, 0);
		Index = GetRangeIndex(Iter->Bytes);
		if(Index != -1) {
			Cell = GetTableCell(pTable, i, 3);
			RangeInfo[Index].Start = Cell->IntData;
			Cell = GetTableCell(pTable, i, 4);
			RangeInfo[Index].Length = Cell->IntData;
			Cell = GetTableCell(pTable, i, 5);
			RangeInfo[Index].ReadLockEnabled = (int)Cell->IntData;
			Cell = GetTableCell(pTable, i, 6);
			RangeInfo[Index].WriteLockEnabled = (int)Cell->IntData;
			Cell = GetTableCell(pTable, i, 7);
			RangeInfo[Index].ReadLocked = (int)Cell->IntData;
			Cell = GetTableCell(pTable, i, 8);
			RangeInfo[Index].WriteLocked = (int)Cell->IntData;
		}
	}
	memcpy(NewRangeInfo, RangeInfo, (NumRanges + 1) * sizeof(RANGEINFO));

	/* Free up the pTable. */
	FreeTable(pTable);

	/* Set up the structure to pass to the dialog box. */
	UserDlg.pTcgAuth = &TcgAuth;
	UserDlg.NumRanges = NumRanges;
	UserDlg.RangeInfo = RangeInfo;
	UserDlg.NewRangeInfo = NewRangeInfo;
	UserDlg.pTcgDrive = pTcgDrive;

	/* Display the dialog box. */
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_RANGES), hWnd, RangeFunc, (LPARAM)&UserDlg);

	/* Free up resources. */
	MemFree(RangeInfo);
	MemFree(NewRangeInfo);
}


/*
 * Erase one or more ranges.
 */
void EraseRanges(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	TCGAUTH		TcgAuth;
	UserDlg		UserDlg;
	BYTE		RetVal;
	int			NumRanges;

	/* Check that the Locking SP is enabled. */
	if(IsLockingEnabled(pTcgDrive) == FALSE) {
		MessageBox(hWnd, _T("The Locking SP is not enabled."), EraseCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Get the number of ranges. */
	NumRanges = GetNumberOfRanges(hWnd, pTcgDrive, EraseCaption);

	/* Query the user for the drive user/admin to use. */
	RetVal = GetUserAuthInfo(hWnd, pTcgDrive, SP_LOCKING.Uid, NULL, EraseCaption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Set up the structure to pass to the dialog box. */
	UserDlg.pTcgAuth = &TcgAuth;
	UserDlg.NumRanges = NumRanges;
	UserDlg.RangeInfo = NULL;
	UserDlg.NewRangeInfo = NULL;
	UserDlg.pTcgDrive = pTcgDrive;

	/* Display the dialog box. */
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ERASERANGES), hWnd, EraseRangeFunc, (LPARAM)&UserDlg);
}


/*
 * Lock or unlock a range.
 */
void LockRanges(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	TCGAUTH		TcgAuth;
	UserDlg		UserDlg;
	BYTE		RetVal;
	int			NumRanges;

	/* Check that the Locking SP is enabled. */
	if(IsLockingEnabled(pTcgDrive) == FALSE) {
		MessageBox(hWnd, _T("The Locking SP is not enabled."), LockCaption, MB_ICONERROR | MB_OK);
		return;
	}

	/* Get the number of ranges. */
	NumRanges = GetNumberOfRanges(hWnd, pTcgDrive, LockCaption);

	/* Query the user for the drive user/admin to use. */
	RetVal = GetUserAuthInfo(hWnd, pTcgDrive, SP_LOCKING.Uid, NULL, LockCaption, &TcgAuth);

	/* If the user cancelled the query, return. */
	if(RetVal == FALSE) {
		return;
	}

	/* Set up the structure to pass to the dialog box. */
	UserDlg.pTcgAuth = &TcgAuth;
	UserDlg.NumRanges = NumRanges;
	UserDlg.RangeInfo = NULL;
	UserDlg.NewRangeInfo = NULL;
	UserDlg.pTcgDrive = pTcgDrive;

	/* Display the dialog box. */
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LOCKRANGES), hWnd, LockRangeFunc, (LPARAM)&UserDlg);
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
