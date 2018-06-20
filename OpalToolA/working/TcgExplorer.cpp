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
#include<initguid.h>
#include<winioctl.h>
#include"TcgExplorer.h"
#include"AtaDrive.h"
#include"Table.h"
#include"Tcg.h"
#include"SpManagement.h"
#include"ChangePassword.h"
#include"TableDisplay.h"
#include"Uid.h"
#include"Memory.h"
#include"TestPassword.h"
#include"ByteTable.h"
#include"CryptInfo.h"
#include"Ranges.h"
#include"MbrControl.h"
#include"Msid.h"
#include"Level0.h"
#include"Users.h"
#include"Random.h"
#include"resource.h"

#include <stdio.h> // for _snprintf_s for debugging

/* SetupAPI library needed. */
#pragma comment(lib, "setupapi.lib")

/* Level 1 information column headers. */
static LPTSTR	Level1Cols[] = {_T("Property"), _T("Value")};

/* Caption for error messages. */
static LPTSTR	Caption = _T("TCG Explorer");

/* If we can't include WinIOCtl.h, uncomment the following line. */
//DEFINE_GUID(GUID_DEVINTERFACE_DISK,                   0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

/*
 * Adds the drive to the list of drives.
 */
static void AddDriveToList(HWND hWnd, LPTCGDRIVE pTcgDrive, LPTSTR DriveString)
{
	LPTSTR	Text;
	LVITEM	Item;
	TCHAR	ItemData[10];
	DWORD	DriveNumber;
	WORD	DriveType;
	int		Index;

	ZeroMemory(&Item, sizeof(Item));
	DriveNumber = pTcgDrive->pAtaDrive->DriveNumber;
	wsprintf(ItemData, _T("%d"), DriveNumber);
	Item.iItem = DriveNumber;
	Item.pszText = ItemData;
	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Text = (LPTSTR)MemAlloc((lstrlen(DriveString) + 1) * sizeof(TCHAR));
	lstrcpy(Text, DriveString);
	Item.lParam = (LPARAM)Text;
	Index = SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_INSERTITEM, 0, (LPARAM)&Item);
	// DEBUG MemFree(Text);
	Item.mask = LVIF_TEXT;
	Item.iSubItem = 1;
	Item.pszText = pTcgDrive->pAtaDrive->Model;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_SETITEMTEXT, Index, (LPARAM)&Item);
	Item.iSubItem = 2;
	Item.pszText = pTcgDrive->pAtaDrive->Serial;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_SETITEMTEXT, Index, (LPARAM)&Item);
	Item.iSubItem = 3;
	DriveType = GetTCGDriveType(pTcgDrive);
	Item.pszText = GetFeatureCode(DriveType);
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_SETITEMTEXT, Index, (LPARAM)&Item);
	Item.iSubItem = 4;
	Item.pszText = GetDriveBusString(pTcgDrive->pAtaDrive);
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_SETITEMTEXT, Index, (LPARAM)&Item);
}


/*
 * Find all hard drives by number: PhysicalDrive0, etc
 */
static void EnumDrivesByNumber(HWND hWnd)
{
	LPTCGDRIVE	pTcgDrive;
	TCHAR		DriveString[100];
	int			i;

	for (i = 0; i<MAXSUPPORTEDDRIVE; i++) {
		wsprintf(DriveString, _T("\\\\.\\PhysicalDrive%d"), i);
		pTcgDrive = OpenTcgDrive(DriveString);
		if(pTcgDrive != NULL) {
			AddDriveToList(hWnd, pTcgDrive, DriveString);
			CloseTcgDrive(pTcgDrive);
		}
	}
}


/*
 * Find all hard drives by GUID.
 */
static void EnumDrivesByGuid(HWND hWnd)
{
	PSP_DEVICE_INTERFACE_DETAIL_DATA	pDetailData;
	SP_DEVICE_INTERFACE_DATA			DevInterfaceData;
	SP_DEVINFO_DATA						DevInfoData;
	LPTCGDRIVE							pTcgDrive;
	HDEVINFO							hDevInfo;
	DWORD								Size=0;
	DWORD								Index;
	BOOL								Result;

	hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return;
	}

	Index = 0;
	do {
		DevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		Result = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_DISK, Index, &DevInterfaceData);
		Index++;
		if (Result) {
			Size = 0;
			// Get required buffer size
			SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevInterfaceData, NULL, 0, &Size, NULL);
#pragma warning(push)
#pragma warning(disable: 6102)
			if (ERROR_INSUFFICIENT_BUFFER == GetLastError() && 0 < Size)
#pragma warning(pop)
			{
				pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)MemCalloc(Size);
				if (NULL == pDetailData)
				{
					MessageBox(hWnd, _T("Out of resources - not enough memory."), _T("EnumDrivesByGuid"), MB_ICONERROR | MB_OK);
					return;
				}
				 pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);  // Even though it's a variable size array
				DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
				if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevInterfaceData, pDetailData, Size, &Size, &DevInfoData))
				{
					pTcgDrive = OpenTcgDrive(pDetailData->DevicePath);
					if (pTcgDrive != NULL) {
						AddDriveToList(hWnd, pTcgDrive, pDetailData->DevicePath);
						CloseTcgDrive(pTcgDrive);
					}
				}
				MemFree(pDetailData);
			}
			else {
				TCHAR Buffer[80];
				_snprintf_s(Buffer, sizeof(Buffer), "GetLastError() returns 0x%08X", GetLastError());
				MessageBox(hWnd, Buffer, _T("SetupDiGetDeviceInterfaceDetail failed!"), MB_ICONERROR | MB_OK);
			}
		}
		else if (GetLastError() != ERROR_NO_MORE_ITEMS)
		{
			TCHAR Buffer[80];
			_snprintf_s(Buffer, sizeof(Buffer), "GetLastError() returns 0x%08X", GetLastError());
			MessageBox(hWnd, Buffer, _T("SetupDiGetDeviceInterfaceDetail failed!"), MB_ICONERROR | MB_OK);
		}
	} while(GetLastError() != ERROR_NO_MORE_ITEMS);

	SetupDiDestroyDeviceInfoList(hDevInfo);
}


/*
 * Enumerate hard drives.
 */
static void EnumDrives(HWND hWnd, WORD how)
{
	MENUITEMINFO	mii;
	HMENU			hMenu;

	hMenu = GetSubMenu(GetMenu(hWnd), 0);
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;

	switch (how)
	{
    	case IDM_REFRESH:
			GetMenuItemInfo(hMenu, IDM_BYGUID, FALSE, &mii);
			how = (mii.fState & MFS_CHECKED) ? IDM_BYGUID : IDM_BYNUMBER;
			break;

		case IDM_BYGUID:
			mii.fState = MFS_UNCHECKED;
			SetMenuItemInfo(hMenu, IDM_BYNUMBER, FALSE, &mii);
			mii.fState = MFS_CHECKED;
			SetMenuItemInfo(hMenu, IDM_BYGUID, FALSE, &mii);
			break;

		case IDM_BYNUMBER:
			mii.fState = MFS_CHECKED;
			SetMenuItemInfo(hMenu, IDM_BYNUMBER, FALSE, &mii);
			mii.fState = MFS_UNCHECKED;
			SetMenuItemInfo(hMenu, IDM_BYGUID, FALSE, &mii);
			break;

		default:
			return;
	}

	/* Remove all drives from the list. */
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_DELETEALLITEMS, 0, 0);

	(IDM_BYGUID == how ? EnumDrivesByGuid : EnumDrivesByNumber)(hWnd);
}


/*
 * Opens a drive given the index of the drive in the list.
 */
static LPTCGDRIVE OpenDriveByIndex(HWND hWnd, int Index)
{
	LVITEM	Item;

	/* Get the lParam of the item. */
	memset(&Item, 0, sizeof(Item));
	Item.mask = LVIF_PARAM;
	Item.iItem = Index;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_GETITEM, Index, (LPARAM)&Item);

	return OpenTcgDrive((LPTSTR)Item.lParam);
}


/*
 * The handler for the WM_SYSCOMMAND message.
 */
static BOOL SysCommandHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	/* Mask off the system bits. */
	wParam &= 0xfff0;

	/* We only handle the close message, and end the dialog box. */
	if(wParam == SC_CLOSE) {
		EndDialog(hWnd, 0);
		return TRUE;
	}

	/* We didn't handle any other messages. */
	return FALSE;
}


/*
 * The handler for the WM_INITDIALOG message.
 */
static BOOL InitDialogHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	MENUITEMINFO	mii;
	LVCOLUMN		Column;
	HICON			hIcon;
	HMENU			hMenu;
	RECT			Rect;
	int				Width;
	int				Total;

	/* Set the checkmark on the menu item. */
	hMenu = GetSubMenu(GetMenu(hWnd), 0);
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_CHECKED;
	SetMenuItemInfo(hMenu, IDM_BYGUID, FALSE, &mii);

	/* Set the icons for the dialog box. */
	hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DRIVE), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DRIVE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	/* Get the width of the report header. */
	GetWindowRect(GetDlgItem(hWnd, IDC_LISTDRIVES), &Rect);
	Width = Rect.right - Rect.left - 2*GetSystemMetrics(SM_CXEDGE);
	Total = 0;

	/* Set list view styles. */
	ListView_SetExtendedListViewStyle(GetDlgItem(hWnd, IDC_LISTDRIVES), LVS_EX_FULLROWSELECT);

	/* Add a dummy column to delete later. The first column has limited formatting. */
	ZeroMemory(&Column, sizeof(Column));
	Column.pszText = _T("Dummy");
	Column.cchTextMax = lstrlen(Column.pszText);
	Column.iSubItem = 0;
	Column.cx = 1;
	Column.fmt = LVCFMT_CENTER;
	Column.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_INSERTCOLUMN, Column.iSubItem, (LPARAM)&Column);

	/* Initialize the report header. */
	Column.pszText = _T("Drive");
	Column.cchTextMax = lstrlen(Column.pszText);
	Column.iSubItem++;
	Column.cx = Width/10;
	Column.fmt = LVCFMT_CENTER;
	Column.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_INSERTCOLUMN, Column.iSubItem, (LPARAM)&Column);
	Total += Column.cx;
	Column.pszText = _T("Model");
	Column.cchTextMax = lstrlen(Column.pszText);
	Column.iSubItem++;
	Column.cx = 45*Width/100;
	Column.fmt = LVCFMT_CENTER;
	Column.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_INSERTCOLUMN, Column.iSubItem, (LPARAM)&Column);
	Total += Column.cx;
	Column.pszText = _T("Serial");
	Column.cchTextMax = lstrlen(Column.pszText);
	Column.iSubItem++;
	Column.cx = 25*Width/100;
	Column.fmt = LVCFMT_CENTER;
	Column.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_INSERTCOLUMN, Column.iSubItem, (LPARAM)&Column);
	Total += Column.cx;
	Column.pszText = _T("Type");
	Column.cchTextMax = lstrlen(Column.pszText);
	Column.iSubItem++;
	Column.cx = Width/10;
	Column.fmt = LVCFMT_CENTER;
	Column.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_INSERTCOLUMN, Column.iSubItem, (LPARAM)&Column);
	Total += Column.cx;
	Column.pszText = _T("Bus");
	Column.cchTextMax = lstrlen(Column.pszText);
	Column.iSubItem++;
	Column.cx = Width - Total;
	Column.fmt = LVCFMT_CENTER;
	Column.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_INSERTCOLUMN, Column.iSubItem, (LPARAM)&Column);

	/* Delete the dummy column. */
	SendDlgItemMessage(hWnd, IDC_LISTDRIVES, LVM_DELETECOLUMN, 0, 0);

	/* Query each drive in turn. */
	EnumDrives(hWnd, IDM_REFRESH);

	/* Let the default control get the focus. */
	return TRUE;
}


/*
 * Internal helper routine to get the selected drive number.
 */
static int GetSelectedDrive(HWND hWnd)
{
	int		Index;

	/* Get the index of the selected row. */
	Index = ListView_GetNextItem(GetDlgItem(hWnd, IDC_LISTDRIVES), -1, LVNI_SELECTED);

	/* If nothing is selected, notify the user. */
	if(Index == -1) {
		MessageBox(hWnd, _T("No drive was selected!"), Caption, MB_ICONSTOP | MB_OK);
	}

	/* Return the index. */
	return Index;
}

/*
 * Internal helper routine to open the selected drive.
 * If no drive is selected, return NULL
 * If it is not a Tcg drive, close it and return NULL
 * Otherwise, return the pointer to the opened Tcg drive
 */
static LPTCGDRIVE OpenSelectedTcgDrive(HWND hWnd)
{
	int Index;
	LPTCGDRIVE pTcgDrive;

	/* Get the index of the selected drive */
	Index = GetSelectedDrive(hWnd);
	if (-1 == Index)
	{
		return NULL;
	}
	pTcgDrive = OpenDriveByIndex(hWnd, Index);
	if (0 == pTcgDrive->FirstComID)
	{
		MessageBox(hWnd, _T("Selected drive is not a TCG drive!"), Caption, MB_ICONSTOP | MB_OK);
		CloseTcgDrive(pTcgDrive);
		return NULL;
	}
	return pTcgDrive;
}

typedef void DRIVECOMMANDFN(HWND hWnd, LPTCGDRIVE pTcgDrive);

static void DriveCommand(HWND hWnd, DRIVECOMMANDFN fn)
{
	LPTCGDRIVE pTcgDrive;
	pTcgDrive = OpenSelectedTcgDrive(hWnd);
	if (NULL != pTcgDrive)
	{
		fn(hWnd, pTcgDrive);
		CloseTcgDrive(pTcgDrive);
	}
}

static void Level0(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	BOOL Result;
	Result = Level0Discovery(pTcgDrive);
	if (Result) {
		Level0Info(hWnd, pTcgDrive->pAtaDrive->Scratch);
	}
	else {
		MessageBox(hWnd, _T("There was an error retrieving Level 0 Discovery information."), Caption, MB_ICONERROR | MB_OK);
	}
}

static void Level1(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	LPTABLE			pTable;
	pTable = Level1Discovery(pTcgDrive, NULL);
	if (pTable != NULL) {
		DisplayGenericTable(hWnd, pTable, _T("Level 1 Information"), Level1Cols);
		FreeTable(pTable);
	}
	else {
		MessageBox(hWnd, _T("There was an error retrieving the Level 1 Discovery information."), Caption, MB_ICONERROR | MB_OK);
	}
}

static void ReadDataStore(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	ReadByteTableByUid(hWnd, pTcgDrive, TABLE_DATASTORE.Uid, SP_LOCKING.Uid);
}

static void WriteDataStore(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	WriteByteTableByUid(hWnd, pTcgDrive, TABLE_DATASTORE.Uid, SP_LOCKING.Uid);
}

static void ReadMBR(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	ReadByteTableByUid(hWnd, pTcgDrive, TABLE_MBR.Uid, SP_LOCKING.Uid);
}

static void WriteMBR(HWND hWnd, LPTCGDRIVE pTcgDrive)
{
	WriteByteTableByUid(hWnd, pTcgDrive, TABLE_MBR.Uid, SP_LOCKING.Uid);
}


/*
 * This handles the WM_COMMAND message.
 */
static BOOL CommandHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	WORD menuID = LOWORD(wParam);
	
	switch (menuID) {
		case IDM_EXIT:
			EndDialog(hWnd, 0);
			break;
		case IDM_REFRESH:
		case IDM_BYNUMBER:
		case IDM_BYGUID:
			EnumDrives(hWnd, menuID);
			break;
		case IDM_LEVEL0:
			DriveCommand(hWnd, Level0);
			break;
		case IDM_LEVEL1:
			DriveCommand(hWnd, Level1);
			break;
		case IDM_LOCKRANGES:
			DriveCommand(hWnd, LockRanges);
			break;
		case IDM_ERASERANGE:
			DriveCommand(hWnd, EraseRanges);
			break;
		case IDM_RANGE:
			DriveCommand(hWnd, ModifyRanges);
			break;
		case IDM_ENCRYPTION:
			DriveCommand(hWnd, CryptInfo);
			break;
		case IDM_TESTPASSWORD:
			DriveCommand(hWnd, TestPassword);
			break;
		case IDM_READDATASTORE:
			DriveCommand(hWnd, ReadDataStore);
			break;
		case IDM_WRITEDATASTORE:
			DriveCommand(hWnd, WriteDataStore);
			break;
		case IDM_READMBR:
			DriveCommand(hWnd, ReadMBR);
			break;
		case IDM_WRITEMBR:
			DriveCommand(hWnd, WriteMBR);
			break;
		case IDM_READARBTABLE:
			DriveCommand(hWnd, ReadArbitraryByteTable);
			break;
		case IDM_WRITEARBTABLE:
			DriveCommand(hWnd, WriteArbitraryByteTable);
			break;
		case IDM_MBRCONTROL:
			DriveCommand(hWnd, MbrControl);
			break;
		case IDM_TABLES:
			DriveCommand(hWnd, DisplayTable);
			break;
		case IDM_ACTIVATE:
			DriveCommand(hWnd, Activate);
			break;
		case IDM_GETMSID:
			DriveCommand(hWnd, GetMSID);
			break;
		case IDM_CHANGEPASS:
			DriveCommand(hWnd, ChangePassword);
			break;
		case IDM_REVERT:
			DriveCommand(hWnd, DoRevert);
			break;
		case IDM_REVERTLOCKINGSP:
			DriveCommand(hWnd, DoRevertLockingSp);
			break;
		case IDM_REVERTDRIVE:
			DriveCommand(hWnd, RevertDrive);
			break;
		case IDM_USERS:
			DriveCommand(hWnd, Users);
			break;
		case IDM_RANDOM:
			DriveCommand(hWnd, Random);
			break;
	}

	/* We handle these messages. */
	return TRUE;
}


/*
 * This is the handler for the WM_NOTIFY message.
 */
static BOOL NotifyHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LPNMLISTVIEW	ListView;
	LVITEM			Item;
	LPNMHDR			Hdr;
	POINT			Point;
	HMENU			hMenu;

	/* Make sure it's for the list view control. */
	if(wParam != IDC_LISTDRIVES) {
		return TRUE;
	}

	/* Handle right click messages. */
	Hdr = (LPNMHDR)lParam;
	switch(Hdr->code) {
		case NM_RCLICK:
			/* If nothing is selected, return. */
			if(ListView_GetNextItem(GetDlgItem(hWnd, IDC_LISTDRIVES), -1, LVNI_SELECTED) == -1) {
				return TRUE;
			}

			/* Get the popup menu we wish to display. */
			hMenu = GetSubMenu(GetMenu(hWnd), 1);

			/* Get the location of the mouse pointer in screen coordinates. */
			GetCursorPos(&Point);

			/* Display the pop-up menu. */
			TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, Point.x, Point.y, hWnd, NULL);
		break;
		case LVN_DELETEALLITEMS:
			return FALSE;
			break;
		case LVN_DELETEITEM:
			ListView = (LPNMLISTVIEW)lParam;
			ZeroMemory(&Item, sizeof(Item));
			Item.iItem = ListView->iItem;
			Item.mask = LVIF_PARAM;
			ListView_GetItem(GetDlgItem(hWnd, IDC_LISTDRIVES), &Item);
			MemFree((LPVOID)Item.lParam);
			break;
		default:
			break;
	}

	/* Always return true, since we process this message. */
	return TRUE;
}


/*
 * The message handler for the TCG Explorer dialog box.
 */
static BOOL CALLBACK ExplorerFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	/* Handle our messages here. */
	switch(Msg) {
		case WM_INITDIALOG:
			return InitDialogHandler(hWnd, wParam, lParam);
			break;
		case WM_SYSCOMMAND:
			return SysCommandHandler(hWnd, wParam, lParam);
			break;
		case WM_COMMAND:
			return CommandHandler(hWnd, wParam, lParam);
			break;
		case WM_NOTIFY:
			return NotifyHandler(hWnd, wParam, lParam);
			break;
	}

	/* Indicate we didn't process the message, or we still want the system to process it. */
	return FALSE;
}


/*
 * Start the TCG Explorer dialog box.
 */
void TcgExplorer(void)
{
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAIN), HWND_DESKTOP, ExplorerFunc);
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
