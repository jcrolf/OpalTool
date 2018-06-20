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


// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

// TODO: reference additional headers your program requires here

/**********************************************
 *                                            *
 *                                            *
 **********************************************/

#pragma once
#pragma comment(lib, "comctl32.lib")

#include "targetver.h"

#define STRICT
#include <windows.h>
#include <assert.h>
#include <commctrl.h>
#include <shellapi.h>
#include <setupapi.h>
#include <stddef.h>
#include "Passthrough.h"
#include "Storage.h"
#include "tchar.h"

typedef unsigned __int64	QWORD;
typedef QWORD				*LPQWORD;

#define MAXSUPPORTEDDRIVE	128

#define	BE_TO_LE_16(x)		((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#define LE_TO_BE_16(x)		BE_TO_LE_16(x)
#define BE_TO_LE_32(x)		((((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | (((x) & 0xff00) << 8) | (((x) & 0xff) << 24))
#define LE_TO_BE_32(x)		BE_TO_LE_32(x)
#define BE_TO_LE_64(x)		((((x) >> 56) & 0xff) | (((x) >> 40) & 0xff00) | (((x) >> 24) & 0xff0000) | (((x) >> 8) & 0xff000000) | (((x) & 0xff000000) << 8) | (((x) & 0xff0000) << 24) | (((x) & 0xff00) << 40) | (((x) & 0xff) << 56))
#define LE_TO_BE_64(x)		BE_TO_LE_64(x)

/**********************************************
 *                                            *
 *                                            *
 **********************************************/

