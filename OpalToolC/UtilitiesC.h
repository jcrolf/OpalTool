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


#pragma once

#include "Utilities.h"

#include "Table.h"
#include "Uid.h"
#include "Admin.h"
#include "AtaDrive.h"
#include "Tcg.h"
#include "GetSp.h"
#include "GetUser.h"
#include "Table.h"

bool convertHexNumber(std::string &hexString, unsigned char* buffer);
bool convertHexNumber(std::string &hexString, unsigned int &number);
bool convertHexNumber(std::string &hexString, QWORD &number);
bool convertHexNumber(const char* hexString, unsigned char* buffer);
bool convertHexNumber(const char* hexString, unsigned int &number);
bool convertHexNumber(const char* hexString, QWORD &number);

class Arguments;
class ArgumentsPW;

using namespace std;

typedef BYTE SPID_t[8];
typedef BYTE UID_t[8];

string dumpUID(UID_t UID); 
string dumpTcgAuth(LPTCGAUTH TcgAuth);
string CredentialsAsString(LPTCGAUTH TcgAuth);
LPTABLE legalSPs(LPTCGDRIVE hDrive);
bool isLegalSP(LPTCGDRIVE hDrive,const std::string sp,LPBYTE spID);
LPTABLE legalUsers(LPTCGDRIVE hDrive, LPBYTE spID,LPTCGAUTH TcgAuth=NULL);
bool isValidUser(LPTCGDRIVE hDrive, LPTABLE LegalUsers, string uid, LPTCGAUTH TcgAuth=NULL);
bool isValidSP(LPTCGDRIVE hDrive,BYTE *spID,const Arguments &args);
bool setTcgAuthCredentials(LPTCGDRIVE hDrive,LPTCGAUTH TcgAuth,const ArgumentsPW &pw);
LPTABLE GetTOT(LPTCGDRIVE hDrive,LPBYTE spID,LPTCGAUTH TcgAuth,const Arguments &args);
bool GetTableUID(LPTABLE TOT,LPBYTE TableUID,int &TOTrow, const Arguments &args);
TCHAR *driveName(TCHAR *DriveString, int bufSize, int idx);
