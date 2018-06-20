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
#include"Progress.h"
#include"resource.h"


/* The subclass property string. */
static LPTSTR SubClassString = _T("Progress Subclass");


/*
 * This is my custom progress bar drawing program.
 */
static LRESULT DrawProgressBar(HWND hWnd, HDC hDC)
{
	PAINTSTRUCT		ps;
	HBITMAP			hBitmapOld;
	HBITMAP			hBitmap;
	HFONT			hFontOld;
	HFONT			hFont;
	DWORD			Current;
	DWORD			Limit;
	DWORD			Percent;
	TCHAR			Text[10];
	RECT			ClientRect;
	RECT			NewRect;
	SIZE			Size;
	HDC				hMemDC;
	int				x, y;

	/* Start the painting process. */
	BeginPaint(hWnd, &ps);

	/* Use the specified device context, or the default one. */
	if(hDC == NULL) {
		hDC= ps.hdc;
	}

	/* Get the size of the window's client area. */
	GetClientRect(hWnd, &ClientRect);

	/* Create a memory device context, a new bitmap, and select it into the context. */
	hMemDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, ClientRect.right, ClientRect.bottom);
	hBitmapOld = (HBITMAP)SelectObject(hMemDC, hBitmap);

	/* Get the font. */
	hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	hFontOld = (HFONT)SelectObject(hMemDC, hFont);

	/* Erase the background. */
	FillRect(hMemDC, &ClientRect, GetSysColorBrush(COLOR_BTNFACE));

	/* Get the position. */
	Current = SendMessage(hWnd, PBM_GETPOS, 0, 0);
	Limit = SendMessage(hWnd, PBM_GETRANGE, 0, NULL);
	Percent = min(MulDiv(Current, 100, Limit), 100);

	/* Draw the text. */
	SetBkMode(hMemDC, TRANSPARENT);
	SetTextColor(hMemDC, GetSysColor(COLOR_WINDOWTEXT));
	wsprintf(Text, _T("%d%%"), Percent);
	GetTextExtentPoint32(hMemDC, Text, lstrlen(Text), &Size);
	x = (ClientRect.right - Size.cx)/2;
	y = (ClientRect.bottom - Size.cy)/2;
	ExtTextOut(hMemDC, x, y, 0, NULL, Text, lstrlen(Text), NULL);

	/* Draw the progress bar. */
	NewRect = ClientRect;
	NewRect.right = MulDiv(ClientRect.right, Current, Limit);
	FillRect(hMemDC, &NewRect, GetSysColorBrush(COLOR_HIGHLIGHT));

	/* Draw text again. */
	SetTextColor(hMemDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	ExtTextOut(hMemDC, x, y, ETO_CLIPPED, &NewRect, Text, lstrlen(Text), NULL);

	/* Draw the bitmap on the device context. */
	BitBlt(hDC, 0, 0, ClientRect.right, ClientRect.bottom, hMemDC, 0, 0, SRCCOPY);

	/* Clean up and remove our objects. */
	SelectObject(hMemDC, hBitmapOld);
	SelectObject(hMemDC, hFontOld);
	DeleteObject(hBitmap);
	DeleteObject(hMemDC);

	/* End the paiting process. */
	EndPaint(hWnd, &ps);

	/* Return the result. */
	return 0;
}


/*
 * This is the subclassed function for the progress bar.
 */
static LRESULT CALLBACK NewProgressProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC		OrigProc;

	/* Get the original procedure. */
	OrigProc = (WNDPROC)GetProp(hWnd, SubClassString);

	/* Process the messages we care about. */
	switch(Msg) {
		case WM_PAINT:
			return DrawProgressBar(hWnd, (HDC)wParam);
			break;
		case WM_ERASEBKGND:
			return 0;
			break;
		case WM_DESTROY:
			RemoveProp(hWnd, SubClassString);
			SetWindowLong(hWnd, GWL_WNDPROC, (LONG)OrigProc);
			break;
		default:
			break;
	}

	/* Call the original routine. */
	return CallWindowProc(OrigProc, hWnd, Msg, wParam, lParam);
}


/*
 * The dialog function for the progress bar.
 */
static BOOL CALLBACK ProgressFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC		OldWndProc;
	HWND		hWndProgress;

	switch(Msg)	{
		case WM_INITDIALOG:
			SetWindowLong(hWnd, DWL_USER, (LONG)FALSE);
			hWndProgress = GetDlgItem(hWnd, IDC_PROGRESS);
			OldWndProc = (WNDPROC)GetWindowLong(hWndProgress, GWL_WNDPROC);
			SetProp(hWndProgress, SubClassString, OldWndProc);
			SetWindowLong(hWndProgress, GWL_WNDPROC, (LONG)NewProgressProc);
			return TRUE;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					SetWindowLong(hWnd, DWL_USER, (LONG)TRUE);
					ShowWindow(hWnd, SW_HIDE);
					break;
			}
			return TRUE;
			break;
	}

	return FALSE;
}


/*
 * Process all pending message for the dialog box.
 */
void ProcessMessages(HWND hWnd)
{
	MSG		Msg;

	while(PeekMessage(&Msg, hWnd, 0, 0, PM_NOREMOVE)) {
		GetMessage(&Msg, hWnd, 0, 0);
		if(IsDialogMessage(hWnd, &Msg) == 0) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
}


/*
 * Updates the progress bar.
 */
void UpdateProgressBox(HWND hWnd, DWORD Current)
{
	SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETPOS, Current, 0);
}


/*
 * Returns true if the user cancelled the dialog box.
 */
BOOL IsProgressCancelled(HWND hWnd)
{
	return GetWindowLong(hWnd, DWL_USER);
}


/*
 * Create the progress dialog box.
 */
HWND CreateProgressBox(HWND hWndParent, LPTSTR Name, DWORD Size)
{
	HWND	hWnd;

	/* Create the dialog box. */
	hWnd = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PROGRESS), hWndParent, ProgressFunc);

	/* Set dialog box personalizations. */
	SetWindowText(hWnd, Name);
	SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETRANGE32, 0, Size);

	/* Return the handle. */
	return hWnd;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
