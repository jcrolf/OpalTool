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


#include "stdafx.h"
#include "windows.h"
#include "iostream"
#include "string"

#include "UtilitiesC.h"
#include "TableDisplayC.h"

#include "CommandArguments.h"

using namespace std;

string dumpUID(UID_t UID) 
{
	string result="";
	char printBuffer[15];

	for (int i=0; i<8; i++) {sprintf_s(printBuffer,"%02x ",UID[i]); result+=string(printBuffer); }

	return result;
}

string dumpTcgAuth(LPTCGAUTH TcgAuth)
{
	string result="NULL";
	char printBuffer[256];

	if (TcgAuth!=NULL)
	{
		sprintf_s(printBuffer,"IsValid=%d\n",TcgAuth->IsValid); result=string(printBuffer);
		sprintf_s(printBuffer,"Authority= "); result+=printBuffer;
		for (unsigned int i=0;i<8;i++) {sprintf_s(printBuffer,"%02x ",TcgAuth->Authority[i]);result+=printBuffer;}
		result+="\n";
		sprintf_s(printBuffer,"Credentials= "); result+=printBuffer;
		for (unsigned int i=0;i<((TcgAuth->Size<32) ? TcgAuth->Size : 32);i++) {sprintf_s(printBuffer,"%02x ",TcgAuth->Credentials[i]);result+=printBuffer;}
		result+="\n";
		sprintf_s(printBuffer,"Size=%d\n",TcgAuth->Size);result+=printBuffer;
	}
	
    return result;
}

string CredentialsAsString(LPTCGAUTH TcgAuth)
{
	TCHAR buffer[33];
	unsigned int size=(TcgAuth->Size<=32) ? TcgAuth->Size : 32;
	memset(buffer,0,33);
	memcpy(buffer,TcgAuth->Credentials,size);
	return string(buffer);
}

LPTABLE legalSPs(LPTCGDRIVE hDrive)
{
	LPTABLE result=NULL;
	if (hDrive)
	{
		LPTABLE Table = GetSpTable(hDrive);
		if (Table!=NULL) {
			result=CreateTable();
			int rows=GetRows(Table);
			int cols=GetCols(Table);
			if (cols > 1) {
				for (int i=0;i<rows;i++) {
					LPTABLECELL cell=GetTableCell(Table,i,1);
					AddCell(result,i,0,cell->IntData,cell->Bytes);
					cell=GetTableCell(Table,i,0);
					AddCell(result,i,1,cell->IntData,cell->Bytes);
				}
			}		
			else {
				cerr << _T("Invalid sp table returned: insufficent number of columns\n");
			}
			FreeTable(Table);
		}
	}

	return result;
}

bool isLegalSP(LPTCGDRIVE hDrive,const std::string sp,LPBYTE spID)
{
	bool ok=false;

	if (hDrive) {
		LPTABLE Table = legalSPs(hDrive);

		if (Table) {
			int rows=GetRows(Table);
			for (int i=0;(!ok && (i<rows));i++)
			{
				LPTABLECELL	Iter=GetTableCell(Table,i,0);
				if(Iter->Type == TABLE_TYPE_STRING) {
					ok=(strcmp((LPSTR)Iter->Bytes,sp.c_str())==0);
					if (ok) {
						Iter=GetTableCell(Table,i,1);
						memcpy(spID,Iter->Bytes,8);
					}
				}
			}
			FreeTable(Table);
		}
	}

	return ok;
}

LPTABLE legalUsers(LPTCGDRIVE hDrive, LPBYTE spID,LPTCGAUTH TcgAuth)
{
	LPTABLE Users=NULL;
	debugTrace t;

	if (hDrive)
	{
		LPTABLE UserTable = 
			(TcgAuth==NULL) ? GetUserTable(hDrive, spID) :
							  ReadTableNoSession(hDrive,spID,TABLE_AUTHORITY.Uid,TcgAuth);

		if (UserTable!=NULL)
		{
			Users=CreateTable();
			int rows=GetRows(UserTable);
			int cols=GetCols(UserTable);

			if (t.On(t.TRACE_DEBUG))
				cout << "cols = " << cols << "\n";

			bool hasEnableInfo=(cols>5) ? true: false;

			for (int i=0;(hasEnableInfo && (i<rows));i++) {
					LPTABLECELL cell=GetTableCell(UserTable,i,1);
					hasEnableInfo=(cell!=NULL);
			}

			if (cols > 1) {
				for (int i=0;i<rows;i++) {
					LPTABLECELL cell=GetTableCell(UserTable,i,1);
					AddCell (Users,i,0,cell->IntData,cell->Bytes);				
					cell=GetTableCell(UserTable,i,0);
					AddCell(Users,i,1,cell->IntData,cell->Bytes);
					if (hasEnableInfo) {
						
						cell=GetTableCell(UserTable,i,5);
						// 'Table' is somewhat inconsistent. 
						// AddCell tries to determine type from cell->Bytes.
						if (cell) {
							if (cell->Type==TABLE_TYPE_INT)
								AddCell(Users,i,2,cell->IntData,NULL);
							else AddCell(Users,i,2,cell->IntData,cell->Bytes);
						}
						else { // just in case
							AddCell(Users,i,2,0,NULL);						
						}
					}
				}
			}		
			else {
				cerr << _T("Invalid users table returned: insufficent number of columns\n");
			}
			FreeTable(UserTable);
		}
		else {
			cerr << _T("Unable to retrieve a valid table using the supplied authentication\n");
		}

	}

	return Users;
}

bool isValidUser(LPTCGDRIVE hDrive, LPTABLE LegalUsers, string uid, LPTCGAUTH TcgAuth)
{
	bool ok=false;
	debugTrace t;

	if (TcgAuth==NULL)
	{
		ok=true;
	}
	else if (LegalUsers!=NULL)
	{
		int rows=GetRows(LegalUsers);
		int cols=GetCols(LegalUsers);

		if (t.On(t.TRACE_DEBUG))
			cout << "looking for " << uid << ",rows= " << rows << ",cols= " << cols << "\n";

		if (cols > 1) {
			if (uid=="") {
				uid="Anybody";
				if (t.On(t.TRACE_INFO))
					cout << "forcing to " << uid << "\n";
			}
		
			for (int i=0;(!ok && (i<rows));i++) {
				if (t.On(t.TRACE_DEBUG))
					cout << "row " << i << "\n";

				LPTABLECELL cell=GetTableCell(LegalUsers,i,0);

				if (t.On(t.TRACE_DEBUG))
					cout << "cell->IntData = " << cell->IntData << "\n";

				if (cell->Type==TABLE_TYPE_STRING) {
					if (t.On(t.TRACE_DEBUG))
						cout << "looking at " << (char*)cell->Bytes << "\n";

					if (strcmp(uid.c_str(),(char*)cell->Bytes)==0) {
						if (TcgAuth!=NULL)
						{
							cell=GetTableCell(LegalUsers,i,1);

							if (t.On(t.TRACE_DEBUG))
								cout << "Our cell value is " << (void*)cell << "\n";

							memcpy(TcgAuth->Authority,cell->Bytes,8);

							if (t.On(t.TRACE_DEBUG))
								cout << "memcpy is ok ...\n";
						}
						ok=true;
					}
				}
			}
		}
		else {
			cerr << _T("Invalid users table seen: insufficent number of columns\n");
		}
	}

	return ok;
}

bool isValidSP(LPTCGDRIVE hDrive,BYTE *spID,const Arguments &args)
{
	bool ok=false;

	if (args.securityProvider()=="") {
		cerr << "A security provider (--SP) must be specified\n";
	}
	else {
		ok=isLegalSP(hDrive,args.securityProvider(),spID);
	}

	return ok;
}

bool setTcgAuthCredentials(LPTCGDRIVE hDrive,LPTCGAUTH TcgAuth,const ArgumentsPW &pw)
{
	bool ok=true;
	debugTrace t;

	if (TcgAuth!=NULL)
	{
		TcgAuth->IsValid=FALSE;

		if (pw.len()>0)
		{
			TcgAuth->IsValid=TRUE;
			if (pw.asString()=="MSID")
			{
				if (t.On(t.TRACE_INFO))
					cout << "We have elected to use the MSID as a password\n";

				if (ReadMSID(hDrive,TcgAuth) != FALSE)
				{
					if (t.On(t.TRACE_DEBUG)) {
						cout << "We have the MSID = ";
						string MSIDasString;
						for (unsigned int i = 0; i<TcgAuth->Size; i++) printf("%02x ",TcgAuth->Credentials[i]);
						cout << "\n";
						cout << " or in text ... '" << CredentialsAsString(TcgAuth) << "'\n";
					}
				}
				else 
				{
					TcgAuth->IsValid=FALSE;
					cerr << "Unable to get the MSID ...\n";
				}
			}
			else 
			{
				if (t.On(t.TRACE_INFO)) {
					cout << "Not MSID but rather " << pw.asString() << ", len = " << pw.len() << "\n";
					cout << "TcgAuth = " << (void*)TcgAuth << "\n";
				}
				memcpy(TcgAuth->Credentials,pw.asBytes(),pw.len());
				
				if (t.On(t.TRACE_ALL))
					cout << "after copy ...\n";

				TcgAuth->Size=pw.len();

				if (t.On(t.TRACE_INFO))
					cout << dumpTcgAuth(TcgAuth) << "\n";
			}
		}
		else {
			cerr << "Unable to set TcgAuth - 0 size password.\n";
		}

		return (ok=(TcgAuth->IsValid==TRUE));
	}
	else return ok;
}

LPTABLE GetTOT(LPTCGDRIVE hDrive,LPBYTE spID,LPTCGAUTH TcgAuth,const Arguments &args)
{
	LPTABLE ToT=NULL;
	debugTrace t;

	if (isValidSP(hDrive,spID,args)) 
	{
		LPTABLE UserTable = legalUsers(hDrive, spID);
		if (UserTable!=NULL) 
		{
			if (t.On(t.TRACE_DEBUG))
				cout << DisplayGenericTableS(UserTable,"users:",NULL);

			// IF the user does not specify a User, then we use the tag from 'Anybody' (the first tag), otherwise we
			// use the tag correlating to the specified user ...
			if (isValidUser(hDrive,UserTable,args.uid(),TcgAuth))
			{
				if (t.On(t.TRACE_INFO)) {
					if (TcgAuth!=NULL)
						cout << "we have selected uid tag: " << dumpUID(TcgAuth->Authority) << "\n";
					else cout << "TcgAuth is NULL\n";
				}

				if (setTcgAuthCredentials(hDrive,TcgAuth,args.password()))
				{
					/* Read the table of tables. */
					ToT = ReadTableNoSession(hDrive, spID, TABLE_TABLE.Uid, TcgAuth);
				}
			}
			FreeTable(UserTable);
		}
	}

	return ToT;
}

bool GetTableUID(LPTABLE TOT,LPBYTE TableUID,int &TOTrow, const Arguments &args)
{
	bool found=false;
	int rows=GetRows(TOT);

	for (int i = 0; (!found && (i<rows)); i++)
	{
		LPTABLECELL nameCell=GetTableCell(TOT, i, 1); /* get the table name ... */
		if (nameCell!=NULL)
		{
			
			if (strncmp((char*)nameCell->Bytes,args.tableName().c_str(),nameCell->Size)==0) 
			{
				LPTABLECELL uidCell=GetTableCell(TOT, i, 0);
				if (uidCell!=NULL) {
					/* we have our table */
					TOTrow=i;
					found =true;
					memcpy(TableUID,uidCell->Bytes,uidCell->Size);
				}
				else {
					cerr << "Unable to get the UID of the table entry for ToT row " << i << "\n";
				}
			}
		}
		else {
			cerr << "Unable to get the name of the table entry for ToT row " << i << "\n";
		}
	}

	return found;
}

TCHAR *driveName(TCHAR *DriveString, int bufSize, int idx)
{
	if ((DriveString==NULL) || (bufSize < 32)) {
		DriveString = new TCHAR[32];
	}
	
	if (DriveString!=NULL) {
		wsprintf(DriveString, _T("\\\\.\\PhysicalDrive%d"), idx);
	}

	return DriveString;
}

bool convertHexNumber(const char* hexString, unsigned char* buffer)
{
	bool ok=false;

	if (buffer && hexString)
	{

		const char* pwCore=(strncmp(hexString,"0x",2)==0) ? hexString+2: hexString;
		unsigned int len=strlen(pwCore);
		unsigned loopLimit = ((len<64) ? len : 64);
		for (unsigned int i=0; (i<loopLimit) ;i+=2)
		{
			buffer[i/2]=(ConvertHex(pwCore[i]) << 4) | ConvertHex(pwCore[i+1]);
		}

		ok=true;
	}

	return ok;
}

bool convertHexNumber(std::string &hexString, unsigned char* buffer)
{
	return convertHexNumber(hexString.c_str(),buffer);
}


bool convertHexNumber(const char* hexString, unsigned int &number)
{
	bool ok=true;

	if (hexString)
	{
		number=0;
		const char* pwCore=(strncmp(hexString,"0x",2)==0) ? hexString+2: hexString;
		int len=strlen(pwCore);
		const int maxlen=sizeof(unsigned int)*2;
		unsigned int loopLimit = ((len<maxlen) ? len : maxlen);
		unsigned int nibble;
		for (unsigned int i=0; (ok && (i<loopLimit)) ;i++)
		{
			number <<= 4;
			nibble = ConvertHex(pwCore[i]);
			if (nibble!=0xff)
				number |= nibble;
			else ok=false;
		}
	}

	return ok;
}
bool convertHexNumber(std::string &hexString, unsigned int &number)
{
	return convertHexNumber(hexString.c_str(),number);
}

bool convertHexNumber(const char* hexString, QWORD &number)
{
	bool ok=true;

	if (hexString)
	{
		number=0;
		const char* pwCore=(strncmp(hexString,"0x",2)==0) ? hexString+2: hexString;
		int len=strlen(pwCore);
		const int maxlen=sizeof(QWORD)*2;
		unsigned int loopLimit = ((len<maxlen) ? len : maxlen);
		QWORD nibble=0;
		for (unsigned int i=0; (ok && (i<loopLimit)) ;i++)
		{
			number <<= 4;
			nibble = ConvertHex(pwCore[i]); 
			if (nibble!=0xff)
				number |= nibble;
			else ok=false;
		}
	}

	return ok;
}
bool convertHexNumber(std::string &hexString, QWORD &number)
{
	return convertHexNumber(hexString.c_str(),number);
}
