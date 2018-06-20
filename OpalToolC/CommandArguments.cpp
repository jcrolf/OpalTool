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

#include <iostream>
#include <string.h>
#include "Utilities.h"
#include "UtilitiesC.h"
#include "CommandArguments.h"
#include "OpalToolC_API.h"

using namespace std;

char strerrorBuf[strerrorBufSize];

#ifdef TRACEON
unsigned int debugTrace::_traceVal=TRACE_ALL;
#else
unsigned int debugTrace::_traceVal=TRACE_OFF;
#endif

void scanForDebug(int argc, char* argv[])
{
	for (int i=1;i<argc; i++)
		if (strcmp(argv[i],"--TRACE")==0)
		{
			debugTrace t;
			t.setDebugTrace(argv[++i]);
			break;
		}
}

void scanForDebug(int argc, const char* argv[])
{
	for (int i=1;i<argc; i++)
		if (strcmp(argv[i],"--TRACE")==0)
		{
			debugTrace t;
			t.setDebugTrace(argv[++i]);
			break;
		}
}

Arguments::Arguments(int argc, char* argv[])
	:_action(NOACTION),_MBRctrlEnable(-1),_MBRctrlDone(-1),
	_password(ArgumentsPW("MSID"),ArgumentsPW("")),
	_uid("Anybody",""),
	_securityProvider("Admin",""),
	_rangeID(0),_rangeStart(-1),_rangeLen(-1),
	_readlock(-1),_writelock(-1),
	_readlockenable(-1),_writelockenable(-1),
	_useDefaults(true),_timer(20,0),_keepGlobalRangeKey(1,-1),
	_ok(true)
{
	debugTrace trace;

	scanForDebug(argc,argv);

	if (trace.On(trace.TRACE_DEBUG)) {
		cout << "password default = " << password().asString() << "\n";
		cout << "uid default = " << uid() << "\n";
		cout << "securityProvider default = " << securityProvider() << "\n";
		cout << "timer default = " << timer() << "\n";
		cout << "keepGlobalRangeKey default = " << keepGlobalRangeKey() << "\n";
	}

	if (argc > 1) {
		for (int i=1;i<argc;i++)
		{
			_ok&=processArgument(argv[i],argv[i+1],i);// up to 1 parameter per argument ...
		}
		if (_fileName=="")
		{
			_fileName=actionName()+".bytedump";
		}
	}
	else {
		_ok=false;
	}
	
	if (!_ok)
		Usage(argv[0]);
}
Arguments::Arguments(int argc, const char* argv[])
	:_action(NOACTION),_MBRctrlEnable(-1),_MBRctrlDone(-1),
	_password(ArgumentsPW("MSID"),ArgumentsPW("")),
	_uid("Anybody",""),
	_securityProvider("Admin",""),
	_rangeID(0),_rangeStart(-1),_rangeLen(-1),
	_readlock(-1),_writelock(-1),
	_readlockenable(-1),_writelockenable(-1),
	_useDefaults(true),_timer(20,0),_keepGlobalRangeKey(1,-1),
	_ok(true)
{
	debugTrace trace;

	scanForDebug(argc,argv);

	if (trace.On(trace.TRACE_DEBUG)) {
		cout << "password default = " << password().asString() << "\n";
		cout << "uid default = " << uid() << "\n";
		cout << "securityProvider default = " << securityProvider() << "\n";
		cout << "timer default = " << timer() << "\n";
		cout << "keepGlobalRangeKey default = " << keepGlobalRangeKey() << "\n";
	}

	if (argc > 1) {
		for (int i=1;i<argc;i++)
		{
			_ok&=processArgument(argv[i],argv[i+1],i);// up to 1 parameter per argument ...
		}
		if (_fileName=="")
		{
			_fileName=actionName()+".bytedump";
		}
	}
	else {
		_ok=false;
	}
	
	if (!_ok)
		Usage(argv[0]);
}

void Arguments::Reset()
{
	_password.Reset();
	_targetPassword.Reset();
	_uid.Reset();
	_timer.Reset();
	_targetuid="";
	_securityProvider.Reset();
	_drive="";
	_tableName="";
	_fileName="";
	_action=NOACTION;
	_MBRctrlEnable=-1;
	_MBRctrlDone=-1;
	_rangeID=0;
	_rangeStart=-1;
	_rangeLen=-1;
	_readlock=-1;
	_writelock=-1;
	_readlockenable=-1;
	_writelockenable=-1;
	_keepGlobalRangeKey.Reset();
	_ok=true;
}

void Arguments::version()
{
	cout << "1.1.1\n";
}

void Arguments::Usage(const char *command)
{
	cerr << "Usage: " << command << " <options>\n";
	cerr << "\n";
	version();
	cerr << "\n";
	cerr << "NOTE: several options (notably <uid>, <password> and <sp>) have defaults if not specified,\n";
	cerr << "thus these parameters are always in a sense 'optional' in that they take on the default\n";
	cerr << "value when not specified\n";
	cerr << "\n";
	cerr << "\n";
	cerr << "--version                 : display the current version\n";
	cerr << "--verbose                 : display extra information logging certain attributes of communication with the drive\n";
	cerr << "--list                    : list the set of connected drives\n";
	cerr << "--listSPs <drive>         : list the set of available sp's for the specified drive\n";
	cerr << "--listUsers <drive> <sp>  <<uid>>\n";
	cerr << "                          : list the set of available users for the specified <sp> for the specified drive\n";
	cerr << "                          : if an optional <<uid>> is provided AND that <uid> is authorized, the output will\n";
	cerr << "                          : include a field with the enable bit for each <uid>\n";
	cerr << "--listMSID <drive>        : list the MSID of the specified drive\n";
	cerr << "--listMBRctrl <drive> <sp> <uid> <password>\n";
	cerr << "                          : list the current values of the shadow MBR control bits\n";
	cerr << "--listRanges <drive> <sp> <uid> <password>\n";
	cerr << "                          : list the current ranges associated with the specified drive\n";
    cerr << "--enableUser <drive> <sp> <uid> <password> <targetuid>\n";
	cerr << "                          : enable <targetuid>, iff <sp> <uid> are authorized\n";
    cerr << "--disableUser <drive> <sp> <uid> <password> <targetuid>\n";
	cerr << "                          : disable <targetuid>, iff <sp> <uid> are authorized\n";
	cerr << "--changePassword <drive> <sp> <uid> <password> <targetuid> <target_password>\n";
	cerr << "                          : change the password of the <targetuid> to <target_password>. <uid> and <password>\n";
	cerr << "                          : in this case are those of the user authorized to make the password change.\n";
	cerr << "--TableOfTables <drive> [<sp> <<uid>> <password>]\n" 
		 << "                          : list the TableOfTables for the selected <sp>, password, and drive\n"
	     << "                          : the <sp>, <<uid>> and <password> are position independent\n";
	cerr << "--level0Discovery <drive> : list the level 0 discovery for the specified drive\n";
	cerr << "--level1Discovery <drive> : list the level 1 discovery for the specified drive\n";
	cerr << "--activateSP <drive> <sp> <uid> <password>\n";
	cerr << "                          : activate/enable the specified <sp>\n";
	cerr << "--dumpTableByName <table> <drive> <sp> <uid> <password>\n";
	cerr << "                          : list the table contents of the specified table for the associated <sp> and drive\n";
	cerr << "--saveByteTable <table> <drive> <sp> <uid> <password> <filename>\n";
	cerr << "                          : save the specified byte table to the named file\n";
	cerr << "--loadByteTable <table> <drive> <sp> <uid> <password> <filename>\n";
	cerr << "                          : load/write the specified byte table from the named file\n";
	cerr << "--saveDataStore <drive> <sp> <uid> <password> <filename>\n";
	cerr << "                          : save the DATASTORE byte table to the named file\n";
	cerr << "--loadDataStore <drive> <sp> <uid> <password> <filename>\n";
	cerr << "                          : load/write the DATASTORE byte table from the named file\n";
	cerr << "--saveShadowMBR <drive> <sp> <uid> <password> <filename>\n";
	cerr << "                          : save the contents of the shadow MBR to the named file\n";
	cerr << "--loadShadowMBR <drive> <sp> <uid> <password> <filename>\n";
	cerr << "                          : load/write the contents of the shadow MBR from the named file\n";
	cerr << "--MBRcontrol <drive> <sp> <uid> <password> <bitaction>\n";
	cerr << "                          : set or clear the specified MBR control bit. Mulitiple directives can be specified\n";
	cerr << "                          : on the same command line.\n";
	cerr << "--setRangeLock <drive> <uid> <password> <rangeid> {<readwrite>}+\n";
	cerr << "                          : set the read or write lock bits for the specified range\n";
	cerr << "--createRange <drive> <uid> <password> <rangeid> <range_spec> {<readwriteenable>}+ {<readwrite>}*\n";
	cerr << "                          : create a range. Optionally adjust the read/write enable or read/write lock bits\n";
	cerr << "                          : during the create operation.\n";
	cerr << "--eraseRange <drive> <uid> <password> <rangeid>\n";
	cerr << "                          : cryptograpically erase the specified range (calls GENKEY on the AES Key for the range)\n";
	cerr << "--setRdLockedUID <drive> <uid> <password> <targetuid> <rangeid>\n";
	cerr << "                          : assign authority for changing the specified range's readlock bit to the specified\n";
	cerr << "                          : target uid.\n";
	cerr << "--setWrLockedUID <drive> <uid> <password> <targetuid> <rangeid>\n";
	cerr << "                          : assign authority for changing the specified range's writelock bit to the specified\n";
	cerr << "                          : target uid.\n";
	cerr << "--revert <drive> <sp> <uid> <password>\n";
	cerr << "                          : revert the drive relative to the specified sp authenticated by uid/password.\n";
	cerr << "--revertDrive <drive> <uid> <password>\n";
	cerr << "                          : revert the drive authenticated by uid/password.\n";
	cerr << "                          : sp will be forced to Admin\n";
	cerr << "--revertLockingSP <drive> <uid> <password> <<keepSpec>>\n";
	cerr << "                          : revert the drive Locking SP authenticated by uid/password.\n";
	cerr << "                          : sp will be forced to Locking\n";
	cerr << "--reboot <timer>          : reboot in <timer> seconds\n";
	cerr << "--testPassword <drive> <password>\n";
	cerr << "                          : determine which SP/User combinations are authorized by the supplied password.\n";
	cerr << "--random <drive>\n";
	cerr << "                          : give back a random number derived from the drive entropy.\n";
	cerr << "\n";
	cerr << "Generalized Control:\n";
	cerr << "\n";
	cerr << "--noDefaults              : Do not adopt default values for any of the options\n";
	cerr << "--useDefaults             : adopt the internal defaults if a value has not been specified\n";
	cerr << "--batch <batchFile>       : read a sequence of options and/or commands from <batchFile>\n";
	cerr << "--resetOptions            : clears the current optionset. Defaults will depend on the most\n";
	cerr << "                          : recent instance of --noDefaults/--useDefaults seen\n";
	cerr << "\n";
	cerr << "Standard regular expression conventions: + implies 1 or more, * implies 0 or more occurances\n";
	cerr << "<sp>                      : --SP <spID>, one of the legal sps for the drive (e.g. Admin or Locking), DEFAULT: Admin\n";
	cerr << "<uid>                     : --UID <text>, a legal uid for the specified <sp>, DEFAULT: Anybody\n";
	cerr << "<targetuid>               : --TUID <text>,a legal uid for the specified <sp>\n";
	cerr << "<password>                : --PW <MSID | <text> | <hex><<R>> >,  DEFAULT: MSID\n";
	cerr << "<target_password>         : --TPW <MSID | <text> | <hex><<R>> >, no default\n";
	cerr << "<drive>                   : --DRIVE <driveID from --list> | <drive # from most recent --list>, no default\n";
	cerr << "<table>                   : --TABLE <table name>, a legal table name, no default\n";
	cerr << "<filename>                : --FILE <path>, path to an input or output file, DEFAULT: <commandAction>.data\n";
	cerr << "<text>                    : printable characters\n";
	cerr << "<driveID>                 : typically \\\\.\\PhysicalDrive<#>\n";
	cerr << "<rangeid>                 : --GLOBALRANGE | --RANGE<#>\n";
	cerr << "<range_spec>              : --RANGESTART <lba> --RANGELEN <block_count>\n";
	cerr << "<keepSpec>                : KEEPGLOBALRANGEKEY | RESETGLOBALRANGEKEY,  DEFAULT: KEEPGLOBALRANGEKEY\n";
	cerr << "<timer>                   : --TIMER <#>, delay the action <#> seconds. DEFAULT: 20\n";
	cerr << "                          : NOTE: Only commands which list a <timer> will honor the <timer>\n";
	cerr << "\n";
	cerr << "<lba>                     : <#> | <hex>, the starting logica block address (lba)\n";
	cerr << "<block_count>             : <#> | <hex>, the number of 512 bytes blocks\n";
	cerr << "<#>                       : [0-9]+\n";
	cerr << "<<R>>                     : R - optional reverse indicator allowed for some hexadecimal entry types\n";
	cerr << "<hex>                     : 0x{[<A-F,a-f,0-9>]+}\n";
	cerr << "<readwrite>               : One of READLOCKED, READUNLOCKED, WRITELOCKED, WRITEUNLOCKED\n";
	cerr << "<readwriteenable>         : One of READLOCKENABLE, READLOCKDISABLE, WRITELOCKENABLE, WRITELOCKDISABLE\n";
	cerr << "<bitaction>               : One of SETENABLE, CLEARENABLE, SETDONE, CLEARDONE\n";
	cerr << "<commandAction>           : One of:\n";
	cerr << "\n";
	for (int i=NOACTION+1; i<MAXACTION; i++)
	cerr << "                            " << actionName(i) << "\n";
	cerr << "\n";
	cerr << "NOTE: To create a password using the text \"MSID\", since MSID is reserved, one would use 0x4D534944\n";

}

bool Arguments::setAction(Arguments::Actions A)
{
	if (_action!=NOACTION) {
		if (Audit())
		{
			int rv= executeAction(*this);
			_ok=(rv==0);
		}
	}
	_action=A;

	return _ok;
}
bool Arguments::processArgument(const char* argType, const char* argValue,int &i)
{
	debugTrace t;

	if (strcmp(argType,"--list")==0)
	{
		setAction(LIST);
	}
	else if (strcmp(argType,"--version")==0)
	{
		version();
	}
	else if (strcmp(argType,"--verbose")==0)
	{
		debugTrace t;
		t.setDebugTrace(debugTrace::TRACE_INFO);
	}
	else if (strcmp(argType,"--SP")==0)
	{
		//_securityProvider.setValue(std::string(argValue));
		_securityProvider=std::string(argValue);
		i++;
	}	
	else if (strcmp(argType,"--UID")==0)
	{
		//_uid.setValue(std::string(argValue));
		_uid=std::string(argValue);
		i++;
	}	
	else if (strcmp(argType,"--TUID")==0)
	{
		_targetuid=std::string(argValue);
		i++;
	}	
	else if (strcmp(argType,"--PW")==0)
	{
		//_password.setValue(ArgumentsPW(argValue));
		_password=ArgumentsPW(argValue);
		i++;
	}	
	else if (strcmp(argType,"--TPW")==0)
	{
		//_targetPassword.set(argValue);
		_targetPassword = argValue;
		i++;
	}	
	else if (strcmp(argType,"--listSPs")==0)
	{
		setAction(LISTSPS);
	}
	else if (strcmp(argType,"--listUsers")==0)
	{
		setAction(LISTUSERS);
	}
	else if (strcmp(argType,"--listMSID")==0)
	{
		setAction(LISTMSID);
	}
	else if (strcmp(argType,"--listMBRctrl")==0)
	{
		setAction(LISTMBRCTRL);
	}
	else if (strcmp(argType,"--listRanges")==0)
	{
		setAction(LISTRANGES);
	}
	else if (strcmp(argType,"--DRIVE")==0)
	{
		if (strstr(argValue, "PhysicalDrive")==NULL) {
			TCHAR *drive=driveName(NULL,0,atoi(argValue));
			_drive=string(drive);
			delete [] drive;
		}
		else {
			_drive=std::string(argValue);
			if (t.On(t.TRACE_INFO))
				cout << "Drive = '" << _drive << "'\n";
		}

		i++;
	}
	else if (strcmp(argType,"--level0Discovery")==0)
	{
		setAction(LEVEL0DISCOVERY);
	}
	else if (strcmp(argType,"--level1Discovery")==0)
	{
		setAction(LEVEL1DISCOVERY);
	}
	else if (strcmp(argType,"--TableOfTables")==0)
	{
		setAction(TABLEOFTABLES);
	}
	else if (strcmp(argType,"--dumpTableByName")==0)
	{
		setAction(DUMPTABLEBYNAME);
	}
	else if (strcmp(argType,"--saveByteTable")==0)
	{
		setAction(SAVEBYTETABLE);
	}
	else if (strcmp(argType,"--loadByteTable")==0)
	{
		setAction(LOADBYTETABLE);
	}
	else if (strcmp(argType,"--saveDataStore")==0)
	{
		setAction(SAVEDATASTORE);
	}
	else if (strcmp(argType,"--loadDataStore")==0)
	{
		setAction(LOADDATASTORE);
	}
	else if (strcmp(argType,"--saveShadowMBR")==0)
	{
		setAction(SAVESHADOWMBR);
	}
	else if (strcmp(argType,"--loadShadowMBR")==0)
	{
		setAction(LOADSHADOWMBR);
	}
	else if (strcmp(argType,"--activateSP")==0)
	{
		setAction(ACTIVATESP);
	}
	else if (strcmp(argType,"--enableUser")==0)
	{
		setAction(ENABLEUSER);
	}
	else if (strcmp(argType,"--disableUser")==0)
	{
		setAction(DISABLEUSER);
	}
	else if (strcmp(argType,"--changePassword")==0)
	{
		setAction(CHANGEPASSWORD);
	}
	else if (strcmp(argType,"--MBRcontrol")==0)
	{
		setAction(MBRCONTROL);
	}
	else if (strcmp(argType,"--setRangeLock")==0)
	{
		setAction(SETRANGELOCK);
	}
	else if (strcmp(argType,"--createRange")==0)
	{
		setAction(CREATERANGE);
	}
	else if (strcmp(argType,"--eraseRange")==0)
	{
		setAction(ERASERANGE);
	}
	else if (strcmp(argType,"--setRdLockedUID")==0)
	{
		setAction(SETRDLOCKEDUID);
	}
	else if (strcmp(argType,"--setWrLockedUID")==0)
	{
		setAction(SETWRLOCKEDUID);
	}
	else if (strcmp(argType,"--revert")==0)
	{
		setAction(REVERT);
	}
	else if (strcmp(argType,"--revertDrive")==0)
	{
		setAction(REVERTDRIVE);
	}
	else if (strcmp(argType,"--revertLockingSP")==0)
	{
		setAction(REVERTLOCKINGSP);
	}
	else if (strcmp(argType,"--useDefaults")==0)
	{
		_useDefaults=true;
	}
	else if (strcmp(argType,"--noDefaults")==0)
	{
		_useDefaults=false;
	}
	else if (strcmp(argType,"SETDONE")==0)
	{
		_MBRctrlDone=1;
	}
	else if (strcmp(argType,"CLEARDONE")==0)
	{
		_MBRctrlDone=0;
	}
	else if (strcmp(argType,"SETENABLE")==0)
	{
		_MBRctrlEnable=1;
	}
	else if (strcmp(argType,"CLEARENABLE")==0)
	{
		_MBRctrlEnable=0;
	}
	else if (strcmp(argType,"READLOCKED")==0)
	{
		_readlock=1;
	}
	else if (strcmp(argType,"READUNLOCKED")==0)
	{
		_readlock=0;
	}
	else if (strcmp(argType,"WRITELOCKED")==0)
	{
		_writelock=1;
	}
	else if (strcmp(argType,"WRITEUNLOCKED")==0)
	{
		_writelock=0;
	}
	else if (strcmp(argType,"READLOCKENABLE")==0)
	{
		_readlockenable=1;
	}
	else if (strcmp(argType,"READLOCKDISABLE")==0)
	{
		_readlockenable=0;
	}
	else if (strcmp(argType,"WRITELOCKENABLE")==0)
	{
		_writelockenable=1;
	}
	else if (strcmp(argType,"WRITELOCKDISABLE")==0)
	{
		_writelockenable=0;
	}
	else if (strcmp(argType,"KEEPGLOBALRANGEKEY")==0)
	{
		_keepGlobalRangeKey=1;
	}
	else if (strcmp(argType,"RESETGLOBALRANGEKEY")==0)
	{
		_keepGlobalRangeKey=0;
	}
	else if (strcmp(argType,"--GLOBALRANGE")==0)
	{
		_rangeID=0;
	}
	else if (strcmp(argType,"--TRACE")==0)
	{
		debugTrace t;
		t.setDebugTrace(argValue);
		i++;
	}
	else if (strcmp(argType,"--resetOptions")==0)
	{
		setAction(RESETOPTIONS);
	}
	else if (strcmp(argType,"--testPassword")==0)
	{
		setAction(TESTPASSWORD);
	}
	else if (strcmp(argType,"--random")==0)
	{
		setAction(RANDOM);
	}
	else if (strcmp(argType,"--TIMER")==0)
	{
		//_timer.setValue(atoi(argValue));
		_timer=atoi(argValue);
		i++;
	}
	else if (strcmp(argType,"--reboot")==0)
	{
		setAction(REBOOT);
	}
	else if (strcmp(argType,"--RANGESTART")==0)
	{
		if (strncmp(argValue,"0x",2)==0) {
			 convertHexNumber(argValue,_rangeStart);
		} 
		else {
			_rangeStart=atoi(argValue);
		}

		i++;
	}
	else if (strcmp(argType,"--RANGELEN")==0)
	{
		if (strncmp(argValue,"0x",2)==0) {
			 convertHexNumber(argValue,_rangeLen);
		}
		else {
			_rangeLen=atoi(argValue);
		}

		i++;
	}
	// MUST COME AFTER ALL ARGUMENTS THAT START WITH --RANGE!!!!
	else if (strncmp(argType,"--RANGE",7)==0)
	{
		_rangeID=atoi(&argType[7]);
	}
	else if (strcmp(argType,"--TABLE")==0)
	{
		_tableName=std::string(argValue);
		i++;
	}
	else if (strcmp(argType,"--FILE")==0)
	{
		_fileName=std::string(argValue);
		i++;
	}
	else if (strcmp(argType,"--batch")==0)
	{
		_ok=processBatchFile(std::string(argValue));
		i++;
	}
	else {
		cerr << "\nUnknown argument " << argType << "\n\n";
		_ok=false;
	}

	return _ok;
}

bool Arguments::processBatchFile(std::string batchFileName)
{
	bool ok=true;
	FILE *bFile=NULL;
	fopen_s(&bFile,batchFileName.c_str(),"r");

	if (bFile!=NULL)
	{
		char inBuffer[1024];
		while (fgets(inBuffer,1023,bFile)!=NULL)
		{
			int dummyIdx=0;
			char *context=NULL;
			
			char *argType=strtok_s(inBuffer," \t\f\r\n",&context);
			char *argValue=strtok_s(NULL," \t\f\r\n",&context);
			while(ok && argType)
			{
				if (ok=processArgument(argType,argValue,dummyIdx))
				{
					if (dummyIdx==0) {
						argType=argValue;
						argValue=(argType!=NULL) ? strtok_s(NULL," \t\f\r\n",&context) : NULL;
					}
					else if (dummyIdx==1) {
						argType=strtok_s(NULL," \t\f\r\n",&context);
						argValue=(argType!=NULL) ? strtok_s(NULL," \t\f\r\n",&context): NULL;
					}
					else {
						cerr << "Batch arguments processing failed to process argument '" << argType << "'\n";
						ok=false;
					}
				}
				dummyIdx=0;
			}
		}

		fclose(bFile); // No need to check errors on the close - this file drives the command set only.
	}
	else
	{
		cerr << "Unable to open batch file '" << batchFileName << "':" << strerror_s(strerrorBuf,errno) << "\n";
		ok=false;
	}

	return ok;
}

bool Arguments::Audit()
{
	if ((_action==SAVESHADOWMBR) ||
		(_action==LOADSHADOWMBR) ||
		(_action==SAVEDATASTORE) ||
		(_action==LOADDATASTORE) ||
		(_action==LISTRANGES) ||
		(_action==SETRANGELOCK) ||
		(_action==CREATERANGE) ||
		(_action==SETRDLOCKEDUID) ||
		(_action==SETWRLOCKEDUID) ||
		(_action==ERASERANGE) ||
		(_action==REVERTLOCKINGSP) ||
		(_action==MBRCONTROL)
		)
	{
		if (securityProvider()!="Locking")
		{
				cerr << "INFO: " << actionName() << " the security provider must be 'Locking', you selected " << securityProvider() << "\n";
				if (_action==REVERTLOCKINGSP) {
					cerr << "      SP forced to 'Locking' ...\n";
					//_securityProvider.setValue("Locking");
					_securityProvider="Locking";
				}
				else {
					_ok=false;
				}
		}
	}
	else if ((_action==REVERT) ||
			 (_action==REVERTDRIVE)
			)
	{
		if (securityProvider()!="Admin")
		{
			cerr << "for action " << actionName() << " the security provider must be 'Admin', you selected " << securityProvider() << "\n";
			_ok=false;
		}
	}

	return _ok;
}

