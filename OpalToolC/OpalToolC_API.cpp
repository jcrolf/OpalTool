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


// OpalToolC.cpp : A command line interface to the OpalTool functionality.
//


#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include "Level0.h"
#include "Table.h"
#include "Admin.h"
#include "AtaDrive.h"
#include "Tcg.h"
#include "GetSp.h"
#include "GetUser.h"
#include "Uid.h"
#include "ChangePassword.h"
#include "MbrControl.h"
#include "GetRange.h"
#include "TestPassword.h"

#include"Memory.h"

#include "CommandArguments.h"
#include "OpalToolC_API.h"
#include "UtilitiesC.h"
#include "DriveC.h"
#include "TableDisplayC.h"
#include "ByteTableC.h"
#include "vector"

using namespace std;

static LPTSTR emptyHdr[] = {_T("")};
static const bool UID_REQUIRED=true;
static const bool VALIDATE_UID_AGAINST_ADMIN_SP=true;

static string trim (const string& untrimmed)
{
	int len=untrimmed.length();
	int sindex=0;
	while ((sindex<len) && (isspace(untrimmed[sindex]))) sindex++;
	int eindex=len;
	while ((eindex>0) && (isspace(untrimmed[eindex-1]))) eindex--;
	return (eindex-sindex>=0) ? untrimmed.substr(sindex,eindex-sindex) : "";
}

static void DisplayDriveDataS(LPTCGDRIVE pTcgDrive, const string &DriveString, int index)
{
	cout << "Drive (" << index << "): " << DriveString << ", Model = " << trim(pTcgDrive->pAtaDrive->Model).c_str() << ",Serial = " << trim(pTcgDrive->pAtaDrive->Serial).c_str() << "\n";
}


int EnumDrivesByNumber_C()
{
#define DS_BUFFER_SIZE 100
	LPTCGDRIVE	hDrive;
	TCHAR		buffer[DS_BUFFER_SIZE];
	int			i;

	for(i=0; i<MAXSUPPORTEDDRIVE; i++) {
		TCHAR *DriveString=driveName(buffer, DS_BUFFER_SIZE, i);

		hDrive = OpenTcgDrive(DriveString);
		if(hDrive != NULL) {
			DisplayDriveDataS(hDrive,string(DriveString),i);
			CloseTcgDrive(hDrive);
		}
	}

	return 0;
}

static int Level0InfoC(LPBYTE Buffer)
{
	int rv=0;

	LPBYTE		Offset;
	LPTSTR		TextBuffer;

	/* Allocate memory for the buffer. */
	TextBuffer = (LPTSTR)MemAlloc(64*1024);
	if(TextBuffer == NULL) {
		cerr << "Out of resources - not enough memory.\n";
		rv=1;
	}
	else {
		TextBuffer[0] = 0;

		/* Iterate through the descriptors. */
		Offset = &Buffer[0x30];
		while(((LPFEATURE)Offset)->Length != 0) {
			PrintFeatureInformation((LPFEATURE)Offset, TextBuffer);
			Offset += ((LPFEATURE)Offset)->Length + sizeof(FEATURE);
		}

		cout << "Level 0 Discovery:\n\n";
		cout << TextBuffer;

		/* Free up resources. */
		MemFree(TextBuffer);
	}

	return rv;
}

int level0DiscoveryC(const string &drive)
{
	LPTCGDRIVE	pTcgDrive = openDriveByName(drive);
	int rv=1;

	if(pTcgDrive != NULL) {
		BOOL Result = Level0Discovery(pTcgDrive);

		if(Result == FALSE) {
			cerr << "There was an error retrieving Level 0 Discovery information for " << drive << ".\n";
		}
		else {
			Level0InfoC(pTcgDrive->pAtaDrive->Scratch);
			rv=0;
		}

		CloseTcgDrive(pTcgDrive);
	}

	return rv;
}

int level1DiscoveryC(const string &drive)
{
	/* Level 1 information column headers. */
	static LPTSTR	Level1Cols[] = {_T("Property"), _T("Value")};
	int rv=1;

	LPTCGDRIVE	hDrive = openDriveByName(drive);
	if(hDrive != NULL) {
		LPTABLE Table = Level1Discovery(hDrive, NULL);
		CloseTcgDrive(hDrive);

		if(Table != NULL) {
			cout << "\n";
			cout << DisplayGenericTableS(Table, _T("Level 1 Information"), Level1Cols);
			FreeTable(Table);
			rv=0;
		} else {
			cerr << "There was an error retrieving Level 1 Discovery information for " << drive << ".\n";
		}
	}

	return rv;
}


int GetMSID_C(const string &drive)
{
	LPTCGDRIVE	hDrive = openDriveByName(drive);
	int rv=1;

	if (hDrive!=NULL)
	{
		TCGAUTH	TcgAuth;

		/* Read the MSID. */
		if(ReadMSID(hDrive, &TcgAuth) == FALSE) {
			cerr << _T("An error occurred reading the MSID.\n");
		} else {
			cout << _T("MSID = ") << CredentialsAsString(&TcgAuth) << _T("\n");
			rv=0;
		}

		CloseTcgDrive(hDrive);
	}

	return rv;
}

int GetRandom_C(const string &drive,LPBYTE binary)
{
	LPTCGDRIVE	hDrive = openDriveByName(drive);
	int rv=1;
	
	debugTrace  t;
	if (t.On(t.TRACE_INFO))			
		cout << "We came back from the open, hDrvie = " << (void*)hDrive << "\n";

	if (hDrive!=NULL)
	{
		TCGSESSION	Session;
		BYTE		Bytes[32];
		BYTE		Result;

		/* Open a session to the admin SP. */
		Result = StartSession(&Session, hDrive, SP_ADMIN.Uid, NULL);	
		if (t.On(t.TRACE_INFO))			
			cout << "We came back from the StartSession, Result = " << (void*)Result << "\n";
		if(Result == 0) {
			/* Get random bytes. */
			Result = GetRandom(&Session, 32, Bytes);
		if (t.On(t.TRACE_INFO))			
			cout << "We came back from the GetRandom, Result = " << (void*)Result << "\n";

			/* Close the session. */
			EndSession(&Session);
			if (t.On(t.TRACE_INFO))			
				cout << "We came back from the EndSession, Result = " << (void*)Result << "\n";


			/* Display an error message, if needed. */
			if(Result != 0) {
				cerr << _T("There was an error reading random bytes from the drive.\n");
			}
			else if (binary!=NULL) {
				memcpy(binary,Bytes,32);
				rv=0;
			}
			else {
				string bytesString;
				for (int i=0;i<32;i++)
				{
					char buf[5];
					sprintf_s(buf,"%02x",Bytes[i]);
					bytesString+=buf;
				}
				
				cout << "Random Bytes: " << bytesString << "\n";
				rv=0;
			}
		}
		else {
			cerr << _T("There was an error opening a session to the Admin SP when trying to get random bytes.\n");
		}

		CloseTcgDrive(hDrive);
	}

	return rv;
}


int GetSP_C(const string &drive)
{
	LPTCGDRIVE	hDrive = openDriveByName(drive);
	int rv=1;

	if (hDrive)
	{
		LPTABLE SPs = legalSPs(hDrive);

		if (SPs) {
			cout << "\n";
			cout << DisplayGenericTableS(SPs, _T("Available SPs"), NULL,NO_TABLE_COLUMN_NAMES);
			FreeTable(SPs);
			rv=0;
		}
		CloseTcgDrive(hDrive);

	}

	return rv;
}

static bool prepareForSession(const Arguments &args,
                              LPTCGDRIVE &hDrive,
                              SPID_t &spID,
                              TCGAUTH &TcgAuth,
                              bool uidRequired=false,
                              bool validateAgainstAdminSP=false)
{
	bool ok=false;
	debugTrace t;

	if (hDrive = openDriveByName(args.drive()))
	{
		if (args.securityProvider()=="") {
			cerr << "A security provider (--SP) must be specified\n";
		}
		else if (isLegalSP(hDrive,args.securityProvider(),spID)) {
			if (t.On(t.TRACE_INFO))
				cout << args.securityProvider() << _T(" is a legal sp for ") << args.drive() << "\n";

			if (args.uid()!="")
			{
				LPTABLE Users = (validateAgainstAdminSP) ? legalUsers(hDrive,SP_ADMIN.Uid) : legalUsers(hDrive, spID);

				if (t.On(t.TRACE_DEBUG)) {
					cout << "we are going for uid " << args.uid() << " in ... \n";
					cout << DisplayGenericTableS(Users,"Users:",NULL,NO_TABLE_COLUMN_NAMES);
				}

				if (isValidUser(hDrive,Users,args.uid(),&TcgAuth))
				{
					if (setTcgAuthCredentials(hDrive,&TcgAuth,args.password()))
					{
						if (t.On(t.TRACE_INFO))
							cout << "Our TcgAuth is:\n" << dumpTcgAuth(&TcgAuth) << "\n";
						ok=true;
					}
				}

				FreeTable(Users);
			}
			else if (!uidRequired) 
			{
				ok=true;
			}
		}

		if (!ok)
			CloseTcgDrive(hDrive);
	}
						
	return ok;
}

int GetUsers_C(const Arguments &args)
{ 
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth))
	{
		LPTABLE Users=legalUsers(hDrive, spID, &TcgAuth);

		if (Users!=NULL)
		{

			cout << DisplayGenericTableS(Users,"Users:",NULL,NO_TABLE_COLUMN_NAMES);

			FreeTable(Users);
			rv=0;
		}
		else {
			cerr << _T("Unable obtain a list of users\n");
		}

		CloseTcgDrive(hDrive);
	}

	return rv;
}

static LPTABLE tableByGet(LPTCGDRIVE hDrive,
                          LPBYTE     Sp, 
                          LPBYTE     TableUid,
                          LPTCGAUTH  TcgAuth,
                          QWORD      Rows, 
                          QWORD      Cols)
{
	TCGSESSION	Session;
	LPTABLE		Table=NULL;
	BYTE		Result;
	debugTrace  t;

	/* Initialize the table, in case we don't read one. */
	Table = NULL;

	/* Start the session. */
	Result = StartSession(&Session, hDrive, Sp, TcgAuth);

	/* If there was a problem, return. */
	if (Result == 0) {
		if (t.On(t.TRACE_INFO))
			cout << "start session success ...\n";
		/* Read the table. */
	
		/* Convert the table uid into table row 0. */
		BYTE TableRow[8];

		
		memset(TableRow, 0, sizeof(TableRow));
		
		memcpy(TableRow, &TableUid[4], 4);
		memcpy(&TableRow[4], &TableUid[0], 4);
		
		Table = CreateTable();

		bool done=false;
		for (int i=0;(!done &&(i<Rows)); i++) 
		{
			if (t.On(t.TRACE_DEBUG))
				cout << "Row " << i << " of " << Rows; for (int n=0; n<8; n++) printf("%02x ",TableRow[n]); cout << "\n";

			ReadTableRow(&Session,TableRow,i,Table);
			
			if (GetCols(Table) < Cols) {
				cerr << "Table is failing to be datafilled, aborting the get ...\n";
			    done=true;
			}
			else TableRow[7]++;
		}
		/* Close the session, we're done for now. */
		EndSession(&Session);
	}
	else std::cerr << "Failed to start session ...\n";

	return Table;
}


int GetTable_C(const Arguments &args)
{
	LPTCGDRIVE	hDrive = openDriveByName(args.drive());
	int rv=1;
	debugTrace t;

	if (hDrive)
	{
		BYTE spID[8];
		TCGAUTH TcgAuth;

		LPTABLE TOT=GetTOT(hDrive,spID,&TcgAuth,args);

		if (TOT) 
		{
			BYTE TableUID[8];
			int  TOTrow=-1;

			if (GetTableUID(TOT,TableUID,TOTrow,args))
			{
					LPTABLE RequestedTable = 
						ReadTableNoSession(hDrive, spID, TableUID, &TcgAuth);
		
					if (RequestedTable) {
						cout << DisplayGenericTableS(RequestedTable,(LPTSTR)(args.tableName().c_str()),NULL);
						FreeTable(RequestedTable);
						rv=0;
					}
					else {
						if (t.On(t.TRACE_INFO))		
							cout << "next failed, using get for " << args.tableName() << "\n";
											
						LPTABLECELL cell=GetTableCell(TOT,TOTrow,6);
						QWORD userCols = (cell) ? cell->IntData : -1;
						cell=GetTableCell(TOT,TOTrow,7);
						QWORD userRows = (cell) ? cell->IntData : -1;
						if ((userCols>1) && (userRows>0))
							RequestedTable=tableByGet(hDrive,spID,TableUID,&TcgAuth,userRows,userCols);
		
						if (RequestedTable) {
							cout << DisplayGenericTableS(RequestedTable,(LPTSTR)(args.tableName().c_str()),NULL);
							FreeTable(RequestedTable);
							rv=0;
						}
						else cerr << "Unable to get " << (LPTSTR)(args.tableName().c_str()) << "\n";
					}	
			}
			else cerr << "Unable to get TOT entry for '" << args.tableName() << "'\n"; 

			FreeTable(TOT);
		}
		else cerr << "Unable to retrieve the Table of Tables\n";
		CloseTcgDrive(hDrive);
	}

	return rv;
}


int GetTableOfTables_C(const Arguments &args)
{
	LPTCGDRIVE	hDrive = openDriveByName(args.drive());
	int rv=1;

	if (hDrive)
	{
		BYTE spID[8];
		TCGAUTH TcgAuth;

		LPTABLE TOT=GetTOT(hDrive,spID,&TcgAuth,args);

		if (TOT) 
		{
			cout << DisplayGenericTableS(TOT,"Table of Tables:",NULL);
			FreeTable(TOT);
			rv=0;
		}
		else cerr << "Unable to get the Table of Tables\n";

		CloseTcgDrive(hDrive);
	}

	return rv;
}

int activateSP_C(const Arguments &args)
{
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;
	debugTrace  t;


	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED,VALIDATE_UID_AGAINST_ADMIN_SP))
	{
		TCGSESSION	Session;

		/* Start authenticated session to the Admin SP. */
		if(StartSession(&Session, hDrive, SP_ADMIN.Uid, &TcgAuth) == 0) 
		{
			if (t.On(t.TRACE_INFO))
				cout << "Activating sp "; for (int i = 0; i<8;i++) printf("%02x ",spID[i]); cout << "\n";
			rv = Activate_SP(&Session, spID);

			if(rv != 0) {
				cerr << "There was an error activating the SP.\n";	
			} else {
				cerr << "The SP was activated successfully.\n";
			}

			/* Close the session, we're done for now. */
			EndSession(&Session);
		}
		else 
		{
			cerr << "An error occurred opening a session.\n";
		}
					
		CloseTcgDrive(hDrive);
	}
	
	return rv;
}

int enableDisableUser_C(const Arguments &args,bool enabledisable)
{ 
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		LPTABLE Users=legalUsers(hDrive, spID, &TcgAuth);
		if (Users)
		{
			int rows=GetRows(Users);
			int cols=GetCols(Users);

			if (cols >2)
			{
				TCGSESSION Session;
				rv = StartSession(&Session, hDrive, spID, &TcgAuth);
				if(rv == 0) {
					bool done=false;
					for (int i=0;(!done && (i<rows)); i++)
					{
						LPTABLECELL Iter = GetTableCell(Users, i, 0);
						if (string((char*)Iter->Bytes)==args.targetuid())
						{
							Iter = GetTableCell(Users, i, 2);
							if (!((enabledisable) && (Iter->IntData!=0)))
							{
								int newVal=((Iter->IntData!=0)? 0 : 1);
								Iter = GetTableCell(Users, i, 1);
								rv = ChangeUserState(&Session, Iter->Bytes, newVal);
								if(rv != 0) {
									cerr << "There was an error " 
											<< ((enabledisable) ? "enabling" :  "disabling") << " a user.\n";
								}
							}
						}
					}
					EndSession(&Session);
				}
				else {
					cerr << "There was an error authenticating to the SP.\n";
				}
			}
			else 
			{
				cerr << "User " << args.uid() << " is not authorized to " << ((enabledisable) ? "enable" : "disable") << " another user\n";
				rv=1;
			}

			cout << DisplayGenericTableS(Users,"Users:",NULL,NO_TABLE_COLUMN_NAMES);
			FreeTable(Users);
		}
		else {
			cerr << "Unable to get the list of valid users relative to the authenticating UID for the command.\n";
		}
		
		CloseTcgDrive(hDrive);
	}

	return rv;
}

int changePassword_C(const Arguments &args)
{ 
	int rv=1;
	debugTrace t;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		LPTABLE Users=legalUsers(hDrive, spID, &TcgAuth);
		if (Users)
		{
			TCGAUTH newTcgAuth;
 
			// the authorizing user and password are ok, now change pw code here
			if ((isValidUser(hDrive,Users,args.targetuid(),&newTcgAuth)) &&
				(setTcgAuthCredentials(hDrive,&newTcgAuth,args.targetPassword()))
				)
			{
				if (t.On(t.TRACE_INFO))
					cout << "Our target (new) TcgAuth is:\n" << dumpTcgAuth(&newTcgAuth) << "\n";

				/* Start an authorized session to the SP. */
				TCGSESSION Session;
				BYTE Result = StartSession(&Session, hDrive, spID, &TcgAuth);
		
				if(Result == 0) {
					Result=ChangePassword(Session,TcgAuth,newTcgAuth);
	
					/* Close the session. */
					EndSession(&Session);
					rv=Result;
				}
				else {
					cerr << "An error occurred opening a session to the SP.\n";
				}
			}

			if (t.On()) 
				cout << DisplayGenericTableS(Users,"Users:",NULL,NO_TABLE_COLUMN_NAMES);

			FreeTable(Users);
		}
		else {
			cerr << "Unable to get the list of valid users relative to the authenticating UID for the password change.\n";
		}

		CloseTcgDrive(hDrive);
	}
	return rv;
}


static const DWORD MBR_ENABLE_BIT=0x10;
static const DWORD MBR_DONE_BIT=0x20;

int listMBRcontrol_C(const Arguments &args)
{ 
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		if(CheckAuth(hDrive, spID, &TcgAuth)) {
			/* Start an authorized session to the SP. */
			TCGSESSION Session;
			BYTE Result = StartSession(&Session, hDrive, spID, &TcgAuth);
		
			if(Result == 0) {

				/* Get the MBR bits to determine the state. */
				DWORD LockingBits = GetLockingFeatures(hDrive);

				/* Set the check boxes appropriately. */
				cout << "Enable: " << ((LockingBits & MBR_ENABLE_BIT) ? "true" : "false") << "\n";
				cout << "Done  : " << ((LockingBits & MBR_DONE_BIT) ? "true" : "false") << "\n";									
	
				/* Close the session. */
				EndSession(&Session);
				rv=0;
			}
			else {
				cerr << "An error occurred opening a session to the SP.\n";
			}
		}
		else 
		{
			cerr << "There was an error authenticating the user to the Locking SP.\n";
		}

		CloseTcgDrive(hDrive);
	}
	return rv;
}

int setMBRcontrol_C(const Arguments &args)
{ 
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		if(CheckAuth(hDrive, spID, &TcgAuth)) {
			/* Start an authorized session to the SP. */
			TCGSESSION Session;
			BYTE Result = StartSession(&Session, hDrive, spID, &TcgAuth);
		
			if(Result == 0) {
				cout << "Enable new value is " << args.MBRctrlEnable() << ",Done new value is " << args.MBRctrlDone() << "\n";
				Result=SetMbrState(&Session, args.MBRctrlEnable(), args.MBRctrlDone());
				if (Result!=0)
					cerr << "Unable to set the MBR control as requested.\n";
				/* Close the session. */
				EndSession(&Session);
				rv=0;
			}
			else {
				cerr << "An error occurred opening a session to the SP.\n";
			}
		}
		else 
		{
			cerr << "There was an error authenticating the user to the Locking SP.\n";
		}

		CloseTcgDrive(hDrive);
	}
	return rv;
}

int listRanges_C(const Arguments &args)
{ 
	static LPTSTR	RangeCols[] = 
	{_T("UID"), _T("Name"),
	 _T("Common Name"), _T("Start"), _T("Length"),_T("Read Lock Enabled"),_T("Write Lock Enabled") , _T("Read Locked"), _T("Write Locked"), _T("Lock On Reset"),_T("ActiveKey")
	};

	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		/* Check that the Locking SP is enabled. */
		if(IsLockingEnabled(hDrive)) {
			/* Get the range information. */
			LPTABLE Ranges = GetRangeTable(hDrive, &TcgAuth);

			if (Ranges) {
				cout << DisplayGenericTableS(Ranges, _T("Active Ranges"), RangeCols,true);
				FreeTable(Ranges);
				rv=0;
			}
			else {
				cerr << "Unable to retrieve the Ranges.\n";
			}
		}
		else {
			cerr << "The Locking SP is not enabled.\n";
		}

		CloseTcgDrive(hDrive);
	}

	return rv;
}

static int setRangeLocks(const Arguments &args, TCGSESSION &Session, UID_t RangeUid)
{
	int rv=0;
	debugTrace t;

	if (t.On(t.TRACE_INFO))
		cout << "readlock = " << args.readlock() << ",writelock" << args.writelock() << "\n";

	/* Perform the requested locking/unlocking functions. */
	char *lockText[]={"unlock","lock"};
	if(args.readlock() >= 0) {
		rv = SetReadLock(&Session, RangeUid, args.readlock());
		if(rv != 0) {
			cerr << "There was an error " << lockText[args.readlock()] << "ing the range for reading.\n";
		} else {
			cout << "The range has been " << lockText[args.readlock()] << "ed for reading.\n";
		}
	}

	if ((rv==0) && (args.writelock() >= 0)) {
		rv = SetWriteLock(&Session, RangeUid, args.writelock());
		if(rv != 0) {
			cerr << "There was an error " << lockText[args.writelock()] << "ing the range for writing.\n";
		} else {
			cout << "The range has been " << lockText[args.writelock()] << "ed for writing.\n";
		}
	}

	return rv;
}

int setRangeLock_C(const Arguments &args)
{ 
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		/* Check that the Locking SP is enabled. */
		if(IsLockingEnabled(hDrive)) 
		{
			TCGSESSION	Session;
			BYTE		RangeUid[8];

			/* Convert the index to a Range Uid. */
			if(args.rangeID() == 0) {
				memcpy(RangeUid, LOCKING_GLOBALRANGE.Uid, 8);
			} else {
				int Index=args.rangeID();
				memcpy(RangeUid, LOCKING_RANGE.Uid, 6);
				RangeUid[6] = (Index >> 8) & 0xff;
				RangeUid[7] = (Index >> 0) & 0xff;
			}

			/* Start an authorized session to the Locking SP. */
			rv = StartSession(&Session, hDrive, SP_LOCKING.Uid, &TcgAuth);
			if(rv == 0) {
				rv=setRangeLocks(args,Session,RangeUid);

				/* End the session. */
				EndSession(&Session);
			}
			else {
				cerr << "Unable to open a session to the Locking SP.\n";
			}
		}
		else {
			cerr << "The Locking SP is not enabled.\n";
		}

		CloseTcgDrive(hDrive);
	}

	return rv;
}

static bool auditRange(LPTABLE Ranges, const Arguments &args, vector<int> &conflict)
{
	bool ok=true;
	debugTrace  t;
	
	if ((args.rangeStart()!=0) || (args.rangeLen()!=0)) { // (0,0) is a special case ...

		if (Ranges) {

			int Rows = GetRows(Ranges);
			int Cols = GetCols(Ranges);

			if (t.On(debugTrace::TRACE_DEBUG)) {
				cout << "auditRange: Rows = " << Rows << ",Cols = " << Cols << "\n";
			}

			if (Rows==0) return false;
			if (Cols<5) return false;

			LPTABLECELL		StartCell;
			LPTABLECELL		LenCell;
			int newRangeStart = args.rangeStart();
			int newRangeEnd = newRangeStart + args.rangeLen();

			for (int i = 0;( ok && ( i < Rows )); i++)
			{
				if (i!=args.rangeID()) {

					StartCell = GetTableCell(Ranges, i, 3);
					LenCell = GetTableCell(Ranges, i, 4);

					if ((StartCell->Type != TABLE_TYPE_INT) || (LenCell->Type != TABLE_TYPE_INT)) {
						cerr << "Unexpected range start/length pair seen in range " << i << "\n";
						ok=false;
					}
					else {
						int rangeStart = StartCell->IntData;
						int rangeEnd = rangeStart + LenCell->IntData;

						// if either the new start or the new end lies within this range, we have an overlap
						if ( ((rangeStart >= newRangeStart) && (rangeStart < newRangeEnd)) || // old starts in new OR new encompasses old
							((rangeStart <= newRangeStart) && (rangeEnd > newRangeStart))     // new starts in old OR old encompasses new
						   )
						{
							ok=false;
							conflict.push_back(i);
							conflict.push_back(rangeStart);
							conflict.push_back(rangeEnd-rangeStart);
						}
					} 
				}
			}
		}

	}

	return ok;
}

int createRange_C(const Arguments &args)
{
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;
	debugTrace  t;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		/* Check that the Locking SP is enabled. */
		if(IsLockingEnabled(hDrive)) {
			/* Get the range information. */
			TCGSESSION	Session;
			BYTE		RangeUid[8];

			/* Convert the index to a Range Uid. */
			if(args.rangeID() == 0) {
				memcpy(RangeUid, LOCKING_GLOBALRANGE.Uid, 8);
			} else {
				int Index=args.rangeID();
				memcpy(RangeUid, LOCKING_RANGE.Uid, 6);
				RangeUid[6] = (Index >> 8) & 0xff;
				RangeUid[7] = (Index >> 0) & 0xff;
			}

			/* Start an authorized session to the Locking SP. */
			rv = StartSession(&Session, hDrive, SP_LOCKING.Uid, &TcgAuth);
			if(rv == 0) {
				if (t.On(t.TRACE_INFO)) {
					cout << "calling create range: " << args.rangeStart() << "," << args.rangeLen() << "," << args.readLockEnable()<< "," << args.writeLockEnable() << "\n";
					cout << "RangeUID = "; for (int i = 0; i<8;i++) printf("%02x ",RangeUid[i]); printf("\n");
				}

				/* Read the range table. */
				LPTABLE Ranges = ReadTable(&Session, TABLE_LOCKING.Uid);
				vector<int> conflict;

				if (auditRange(Ranges,args,conflict)) {

					/*  
					 * The actual command allows all parameters to be set at once, but some drives do not respond 
					 * to all the parameters sent at one time, so for now we send them all separately. We also sometimes
					 * need to send them separately, e.g. when setting start and len to zero.
					 */   
					if (t.On(t.TRACE_INFO))
						cout << "Trying to set " << args.rangeStart() << "," << args.rangeLen() << "\n";

					int Result1,Result2,Result3,Result4;

					if ((args.rangeStart()==0) && (args.rangeLen()==0)) { // when forcing to zero, must do len first
						Result2 = CreateRange(&Session, RangeUid, -1, args.rangeLen(), -1, -1);
						Result1 = CreateRange(&Session, RangeUid, args.rangeStart(), -1, -1, -1);
					}
					else {
						Result1 = CreateRange(&Session, RangeUid, args.rangeStart(), -1, -1, -1);
						Result2 = CreateRange(&Session, RangeUid, -1, args.rangeLen(), -1, -1);
					}

					Result3 = CreateRange(&Session, RangeUid, -1, -1, args.readLockEnable(), -1);
					Result4 = CreateRange(&Session, RangeUid, -1, -1, -1, args.writeLockEnable());
				
					if (t.On(t.TRACE_INFO))
						cout << "Results: (" << Result1 << "," << Result2 << "," << Result3 << "," << Result4 << ")\n";

					rv = (Result1 | Result2 | Result3 | Result4);

					if (rv!=0) {
						if (Result1!=TPER_SUCCESS)
							cerr << "Set of rangestart failed: " << ErrorCodeString(Result1) << "( " << Result1 << ")\n";
						if (Result2!=TPER_SUCCESS)
							cerr << "Set of rangelen failed: " << ErrorCodeString(Result2) << " (" << Result2 << ")\n";
						if (Result3!=TPER_SUCCESS)
							cerr << "Set of read lock enable failed: " << ErrorCodeString(Result3).c_str() << " (" << Result3 << ")\n";
						if (Result4!=TPER_SUCCESS)
							cerr << "Set of write lock enable failed: " << ErrorCodeString(Result4).c_str() << " (" << Result4 << ")\n";
					}

					if (rv==0) // if the user specified a readlock or write lock value, we will want to set it now ...
					{
						if (t.On(t.TRACE_DEBUG))
							cout << "RV == 0 for createRange, now trying to do the locks\n";
						rv=setRangeLocks(args,Session,RangeUid);
					}
					else if (t.On(t.TRACE_DEBUG)) 
					{ 
						cout << "RV for createRange = " << rv << "\n";
					}

				}
				else if (conflict.size()>0) 
				{
					cerr << "The requested range parameters (" << args.rangeStart() << "," << args.rangeLen() 
						 << ") for range " << args.rangeID() << " are in conflict with range " 
						 << conflict[0] << " (" << conflict[1] << "," << conflict[2] << ")\n";
				}
			
				if (Ranges)
					FreeTable(Ranges);

				/* End the session. */
				EndSession(&Session);
			}
			else {
				cerr << "Unable to open a session to the Locking SP.\n";
			}

		}
		else {
			cerr << "The Locking SP is not enabled.\n";
		}

		CloseTcgDrive(hDrive);
	}
	return rv;
}

int eraseRange_C(const Arguments &args)
{
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;
	debugTrace  t;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		/* Check that the Locking SP is enabled. */
		if(IsLockingEnabled(hDrive)) {
			/* Get the range information. */
			TCGSESSION	Session;
			BYTE		RangeUid[8];

			/* Convert the index to a Range Uid. */
			if(args.rangeID() == 0) {
				memcpy(RangeUid, LOCKING_GLOBALRANGE.Uid, 8);
			} else {
				int Index=args.rangeID();
				memcpy(RangeUid, LOCKING_RANGE.Uid, 6);
				RangeUid[6] = (Index >> 8) & 0xff;
				RangeUid[7] = (Index >> 0) & 0xff;
			}

			/* Start an authorized session to the Locking SP. */
			rv = StartSession(&Session, hDrive, SP_LOCKING.Uid, &TcgAuth);
			if(rv == 0) {
				UID_t KeyUid;
				if (ReadTableCellBytes(&Session, RangeUid, 10, KeyUid, NULL) != FALSE) {
					if (t.On(t.TRACE_DEBUG))
						cout << "key table row uid = " << dumpUID(KeyUid) << "\n";

					rv = EraseRange(&Session, KeyUid);
					if (rv!=0) {
						cerr << "Unable to erase range " << args.rangeID() << ": uid = " << dumpUID(RangeUid) << "\n";
					}
				}
				else { 
					cerr << "Can't get they key table row uid ...\n";
				}
				/* End the session. */
				EndSession(&Session);
			}
			else {
				cerr << "Unable to open a session to the Locking SP.\n";
			}

		}
		else {
			cerr << "The Locking SP is not enabled.\n";
		}

		CloseTcgDrive(hDrive);
	}
	return rv;
}

int setRangeLockingUID_C(const Arguments &args,bool read)
{
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;
	debugTrace  t;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		/* Check that the Locking SP is enabled. */
		if(IsLockingEnabled(hDrive)) {
			/* Get the range information. */
			TCGSESSION	Session;
			BYTE		RangeUid[8];

			/* Convert the index to a Range Uid. */
			if(args.rangeID() == 0) {
				memcpy(RangeUid, LOCKING_GLOBALRANGE.Uid, 8);
			} else {
				int Index=args.rangeID();
				memcpy(RangeUid, LOCKING_RANGE.Uid, 6);
				RangeUid[6] = (Index >> 8) & 0xff;
				RangeUid[7] = (Index >> 0) & 0xff;
			}

			LPTABLE Users=legalUsers(hDrive, spID, &TcgAuth);

			if (Users)
			{
				/* Start an authorized session to the Locking SP. */
				rv = StartSession(&Session, hDrive, SP_LOCKING.Uid, &TcgAuth);
				if(rv == 0) {

					int rows=GetRows(Users);
					int cols=GetCols(Users);

					if (cols >2)
					{
						bool done=false;
						for (int i=0;(!done && (i<rows)); i++)
						{
							LPTABLECELL Iter = GetTableCell(Users, i, 0);
							if (string((char*)Iter->Bytes)==args.targetuid())
							{
								Iter = GetTableCell(Users, i, 1);
								if (t.On(t.TRACE_INFO))
									cout << "We are using target uid = " << dumpUID(Iter->Bytes) << " to setRdLocked for range " << args.rangeID() << "\n";

								rv= setRangeLockingUID(&Session, Iter->Bytes,args.rangeID(),read);
								if (rv!=0) {
									cerr << "Unable to authorize " << args.targetuid() << " to set the " << ((read) ? "read" : "write") 
										 << "lock bit for range " << args.rangeID() << ", rc = " << rv << "\n";
								}
							}
						}
					}

					/* End the session. */
					EndSession(&Session);
				}
				else {
					cerr << "Unable to open a session to the Locking SP.\n";
				}

				FreeTable(Users);
			}
		}
		else {
			cerr << "The Locking SP is not enabled.\n";
		}

		CloseTcgDrive(hDrive);
	}
	return rv;
}

int revert_C(const Arguments &args)
{
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		TCGSESSION	Session;

		/* If there was a problem, return. */
		if(StartSession(&Session, hDrive, SP_ADMIN.Uid, &TcgAuth) == 0) {
			rv = Revert(&Session, spID);

			/* Close the session, we're done for now. */
			EndSession(&Session);

			/* Indicate success/error message. */
			if(rv != 0) {
				cerr << _T("There was an error reverting the SP.\n");
			} else {
				cerr << _T("The SP was successfully reverted.\n");
			}
		}
		else {
			cerr << _T("An error occurred opening a session.\n");
		}
	}
	
	return rv;
}

int revertDrive_C(const Arguments &args)
{
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		TCGSESSION	Session;

		/* If there was a problem, return. */
		if(StartSession(&Session, hDrive, SP_ADMIN.Uid, &TcgAuth) == 0) {
			rv = Revert(&Session, SP_ADMIN.Uid);

			/* Close the session, we're done for now. */
			EndSession(&Session);

			/* Indicate success/error message. */
			if(rv != 0) {
				cerr << _T("There was an error reverting the SP.\n");
			} else {
				cerr << _T("The SP was successfully reverted.\n");
			}
		}
		else {
			cerr << _T("An error occurred opening a session.\n");
		}
	}

	return rv;
}

int revertLockingSP_C(const Arguments &args)
{
	int rv=1;

	LPTCGDRIVE	hDrive;
	TCGAUTH		TcgAuth;
	SPID_t		spID;

//	cerr << "revertLockingSP not implemented Yet ...\n";

	if (prepareForSession(args,hDrive,spID,TcgAuth,UID_REQUIRED))
	{
		if(IsLockingEnabled(hDrive) == FALSE) {
			TCGSESSION	Session;

			/* If there was a problem, return. */
			if(StartSession(&Session, hDrive, SP_LOCKING.Uid, &TcgAuth) == 0) {

				/* Revert the drive. */
				rv = RevertSP(&Session, args.keepGlobalRangeKey());

				/* Close the session, we're done for now. */
				EndSession(&Session);

				/* Indicate success/error message. */
				if(rv != 0) {
					cerr << _T("There was an error reverting the SP.\n");
				} else {
					cerr << _T("The SP was successfully reverted.\n");
				}
			}
			else {
				cerr << _T("An error occurred opening a session to the Locking SP.\n");
			}
		}
	}
	
	return rv;
}

int testPassword_C(Arguments args)
{
	int rv=1;

	LPTABLECELL	SpCell;
	LPTABLECELL	UserCell;
	TCGAUTH		TcgAuth;
	LPTABLE		SpTable;
	LPTABLE		UserTable;
	LPBYTE		UserName;
	LPBYTE		SpName;
	DWORD		Count;
	BOOL		Result;
	int			SpRows;
	int			UserRows;
	int			i, j;

	LPTCGDRIVE hDrive = openDriveByName(args.drive());
	
	if (hDrive)
	{
		if (setTcgAuthCredentials(hDrive,&TcgAuth,args.password())) {

			/* Get the list of SPs on the drive. */
			SpTable = GetSpTable(hDrive);

			/* Verify the table. */
			if(SpTable) {
				/* For each SP, get a list of users within the SP. */
				Count = 0;
				SpRows = GetRows(SpTable);
				for(i=0; i<SpRows; i++) {
					/* Get the table cell. */
					SpCell = GetTableCell(SpTable, i, 0);

					if (SpCell==NULL) continue;

					/* Get a list of users. */
					UserTable = GetUserTable(hDrive, SpCell->Bytes);

					/* Verify the table. */
					if(UserTable == NULL) {
						cerr <<  _T("There was an error reading a user table.\n");
						continue;
					}

					/* Loop through users. */
					UserRows = GetRows(UserTable);
					for(j=0; j<UserRows; j++) {
						/* Get the table cell. */
						UserCell = GetTableCell(UserTable, j, 0);

						if (UserCell!=NULL) {
						/* Copy the authority. */
						memcpy(TcgAuth.Authority, UserCell->Bytes, sizeof(TcgAuth.Authority));

						/* Check the authorization value. */
						Result = CheckAuth(hDrive, SpCell->Bytes, &TcgAuth);

						/* If it works, notify the user. */
						if(Result) {
							SpName = GetName(SpTable, i);
							UserName = GetName(UserTable, j);
							cerr << _T("Password verified for user '") << UserName 
								 << _T("' in SP '") << SpName << _T("'\n");
							Count++;
						}
						}
						else {
							cerr << _T("Error getting user data for user row ") << j << _T("\n");
						}

					}

					/* Free up memory. */
					FreeTable(UserTable);
				}

				/* If the password did not verify at all, notify the user. */
				if(Count == 0) {
					cerr <<  _T("The password was not verified.\n");
				} 
				else {
					cerr << _T("Password verification complete.\n");
					rv=0;
				}

				/* Free up memory. */
				FreeTable(SpTable);
			}
			else {
				cerr << _T("There was an error reading the SP table.\n");
			}
		}
		CloseTcgDrive(hDrive);
	}

	return rv;
}

int executeAction(Arguments &args)
{
	int rv=0;
	const bool WRITE=false;

	if (args.list()) {
		rv=EnumDrivesByNumber_C();
	}
	else if (args.level0Discovery()) {
		rv=level0DiscoveryC(args.drive());
	}
	else if (args.level1Discovery()) {
		rv=level1DiscoveryC(args.drive());
	}
	else if (args.MSID()) {
		rv=GetMSID_C(args.drive());
	}
	else if (args.listSps()) {
		rv=GetSP_C(args.drive());
	}
	else if (args.listUsers()) {
		rv=GetUsers_C(args);
	}
	else if (args.TableOfTables()) {
		rv=GetTableOfTables_C(args);
	}
	else if (args.dumpTableByName()) {
		rv=GetTable_C(args);
	}
	else if (args.saveByteTable()) {
		rv=saveByteTable(args);
	}
	else if (args.loadByteTable()) {
		rv=loadByteTable(args);
	}
	else if (args.saveDataStore()) {
		rv=saveByteTable(args,TABLE_DATASTORE.Uid);
	}
	else if (args.loadDataStore()) {
		rv=loadByteTable(args,TABLE_DATASTORE.Uid);
	}
	else if (args.saveShadowMBR()) {
		rv=saveByteTable(args,TABLE_MBR.Uid);
	}
	else if (args.loadShadowMBR()) {
		rv=loadByteTable(args,TABLE_MBR.Uid);
	}
	else if (args.activateSP()) {
		rv=activateSP_C(args);
	}
	else if (args.enableUser()) {
		rv=enableDisableUser_C(args,true);
	}
	else if (args.disableUser()) {
		rv=enableDisableUser_C(args,false);
	}
	else if (args.changePassword()) {
		rv=changePassword_C(args);
	}
	else if (args.listMBRctrl()) {
		rv=listMBRcontrol_C(args);
	}
	else if (args.MBRcontrol()) {
		rv=setMBRcontrol_C(args);
	}
	else if (args.listRanges()) {
		rv=listRanges_C(args);
	}
	else if (args.setRangeLock()) {
		rv=setRangeLock_C(args);
	}
	else if (args.createRange()) {
		rv=createRange_C(args);
	}
	else if (args.eraseRange()) {
		rv=eraseRange_C(args);
	}
	// NOTE: Both of these probably should optionally allow the uid to be added to the existing set
	//       of authorized users. This would involve the capacity to expand the postfix ACE boolean
	//       expression, so not trivial. 
	// NOTE: Determine if "a | a" is the only way to add a single user to the authorization for setting
	//       the associated bit.
	else if (args.setRdLockedUID()) {
		rv=setRangeLockingUID_C(args);
	}
	else if (args.setWrLockedUID()) {
		rv=setRangeLockingUID_C(args,WRITE);
	}
	else if (args.revert()) {
		rv=revert_C(args);
	}
	else if (args.revertDrive()) {
		rv=revertDrive_C(args);
	}
	else if (args.revertLockingSP()) {
		rv=revertLockingSP_C(args);
	}
	else if (args.testPassword()) {
		rv=testPassword_C(args);
	}
	else if (args.getRandom()) {
		rv=GetRandom_C(args.drive());
	}
	else if (args.resetOptions()) {
		args.Reset();
	}
	else if (args.reboot()) {
		char buffer[64];
		cout << "Shutdown/Reboot in " << args.timer() << " seconds ...\n";
		sprintf_s(buffer,"shutdown /r /t %lu",args.timer());
		system(buffer);
	}

	return rv;
}
