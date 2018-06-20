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

#include <iostream>
#include <string>
#include "Utilities.h"
#include "UtilitiesC.h"	
#include "debugTrace.h"

const int strerrorBufSize=2048;
extern char strerrorBuf[strerrorBufSize];

template<typename Type> struct optionWithDefault {
	optionWithDefault(Type d,Type nd): _defined(false) { _d=d; _nd=nd; }
	const Type &getValue(bool useDefaults) const { return ((_defined) ? _m : ((useDefaults)?_d:_nd)); }
	void setValue(Type v) { _m=v; _defined=true; }
	void Reset() { _defined=false; }
	optionWithDefault<Type> &operator=(const Type &v) { _m=v; _defined=true; return *this; }
	Type _m;
	Type _d;
	Type _nd;
	bool _defined;
};

class ArgumentsPW {
public:
	ArgumentsPW()
	{
		_pbufPtr=(const BYTE*)_pbuf;
		memset(_pbuf,0,32);
	}

	ArgumentsPW(const ArgumentsPW &s)
	{
		_pbufPtr=(const BYTE*)_pbuf;
		memcpy(_pbuf,s._pbuf,32);
		_password=s._password;
	}

	ArgumentsPW &operator=(const ArgumentsPW &s)
	{
		_pbufPtr=(const BYTE*)_pbuf;
		memcpy(_pbuf,s._pbuf,32);
		_password=s._password;

		return *this;
	}

	ArgumentsPW(const std::string pw)  
	{
		//set(pw);
		*this=pw;
	}

	//void set(const std::string pw)
	ArgumentsPW &operator=(const std::string &pw)
	{
		bool reverse=false;
		debugTrace t;

		if (pw.length()<3)// minimum hex spec is 0x<H> where <H> is one of 0123456789abcdefABCDEF
		{
			_password=pw;
		}
		else if ((pw.substr(0,2)=="0x") && (pw[pw.length()-1]=='R'))
		{
			reverse=true;
			_password=pw.substr(0,pw.length()-1);

			if (t.On(t.TRACE_INFO))
				std::cout << "We are reversing, dropping the R, preliminary pw becomes " << _password << "\n";
		}
		else 
		{
			_password=pw;
		}

		if (_password.substr(0,2)=="0x")
		{
			if (convertHexNumber(_password,_pbuf))
			{
				if (reverse)
				{
					unsigned int half=len()/2;
					unsigned int endB=len()-1;

					for (unsigned int i=0; i<half; i++)
					{
						BYTE t=_pbuf[i];
						_pbuf[i]=_pbuf[endB];
						_pbuf[endB--]=t;
					}
				}
			}
		}
		else
		{
			memset(_pbuf,0,32);
			memcpy(_pbuf,_password.c_str(),_password.length());
		}

		return *this;
	}

	const std::string &asString() const { return _password; }
	const BYTE* asBytes() const
	{ 
		debugTrace t;
		if (t.On(t.TRACE_ALL))
			std::cout << "_pbufPtr=" << (void*)_pbufPtr << "\n";
		return _pbufPtr;
	}

	int len() const
	{
	    if (_password.size()>1)
			if (_password.substr(0,2)=="0x")
				 return ((_password.length()/2)-1);
			else return _password.length();
		else return _password.length();
	}

	void Reset()
	{
		_password="";
		memset(_pbuf,0,32);
	}

private:
	
	std::string _password;
	BYTE _pbuf[32];
	const BYTE* _pbufPtr;
};

class Arguments {

	enum Actions 
	{NOACTION=0,LIST,LISTSPS,LISTUSERS,LEVEL0DISCOVERY,LEVEL1DISCOVERY,LISTMSID,LISTMBRCTRL,
	 TABLEOFTABLES,DUMPTABLEBYNAME,LISTRANGES,
	 SAVEBYTETABLE,LOADBYTETABLE,SAVEDATASTORE,LOADDATASTORE,
	 SAVESHADOWMBR,LOADSHADOWMBR,
	 ACTIVATESP,ENABLEUSER,DISABLEUSER,CHANGEPASSWORD,
	 MBRCONTROL,SETRANGELOCK,CREATERANGE,ERASERANGE,
	 SETRDLOCKEDUID,SETWRLOCKEDUID,
	 REVERT,REVERTDRIVE,REVERTLOCKINGSP,
	 REBOOT,RESETOPTIONS,TESTPASSWORD,RANDOM,
	 MAXACTION
	};

public:
	Arguments(int argc, const char *argv[]);
	Arguments(int argc, char *argv[]);
	void version();
	bool Audit();
	void Reset();
	void setDefaults();
	void clearDefaults();
	bool list() { return (_action==LIST); }
	bool listSps() { return (_action==LISTSPS); }
	bool listUsers() { return (_action==LISTUSERS); }
	bool listMBRctrl() { return (_action==LISTMBRCTRL); }
	bool listRanges() { return (_action==LISTRANGES); }
	bool level0Discovery() { return (_action==LEVEL0DISCOVERY); }
	bool level1Discovery() { return (_action==LEVEL1DISCOVERY); }
	bool MSID() { return (_action==LISTMSID); }
	bool TableOfTables() { return (_action==TABLEOFTABLES); }
	bool dumpTableByName() { return (_action==DUMPTABLEBYNAME); }
	bool saveByteTable() { return (_action==SAVEBYTETABLE); }
	bool loadByteTable() { return (_action==LOADBYTETABLE); }
	bool saveDataStore() { return (_action==SAVEDATASTORE); }
	bool loadDataStore() { return (_action==LOADDATASTORE); }
	bool saveShadowMBR() { return (_action==SAVESHADOWMBR); }
	bool loadShadowMBR() { return (_action==LOADSHADOWMBR); }
	bool activateSP() { return (_action==ACTIVATESP); }
	bool enableUser() { return (_action==ENABLEUSER); }
	bool disableUser() { return (_action==DISABLEUSER); }
	bool changePassword() { return (_action==CHANGEPASSWORD); }
	bool MBRcontrol() { return (_action==MBRCONTROL); }
	bool setRangeLock() { return (_action==SETRANGELOCK); }
	bool createRange() { return (_action==CREATERANGE); }
	bool eraseRange() { return (_action==ERASERANGE); }
	bool setRdLockedUID() { return (_action==SETRDLOCKEDUID); }
	bool setWrLockedUID() { return (_action==SETWRLOCKEDUID); }
	bool revert() { return (_action==REVERT); }
	bool revertDrive() { return (_action==REVERTDRIVE); }
	bool revertLockingSP() { return (_action==REVERTLOCKINGSP); }
	bool reboot() { return (_action==REBOOT); }
	bool resetOptions() { return (_action==RESETOPTIONS); }
	bool testPassword() { return (_action==TESTPASSWORD); }
	bool getRandom() { return (_action==RANDOM); }

	int  rangeID() const { return _rangeID; }
	QWORD  rangeStart() const { return _rangeStart; }
	QWORD  rangeLen() const { return _rangeLen; }
	BOOL MBRctrlEnable() const { return _MBRctrlEnable; }
	BOOL MBRctrlDone() const { return _MBRctrlDone; }
	BOOL readlock() const { return _readlock; }
	BOOL writelock() const { return _writelock; }
	BOOL readLockEnable() const { return _readlockenable; }
	BOOL writeLockEnable() const { return _writelockenable; }
	const std::string &drive() const { return _drive; }
	void			   setSP(std::string &spValue) { _securityProvider.setValue(spValue); }
	const std::string &securityProvider() const { return _securityProvider.getValue(_useDefaults); }
	const std::string &uid() const { return _uid.getValue(_useDefaults); }
	const std::string &targetuid() const { return _targetuid; }
	const ArgumentsPW &password() const { return _password.getValue(_useDefaults); }
	const ArgumentsPW &targetPassword() const { return _targetPassword; }
	const std::string &tableName() const { return _tableName; }
	const std::string &fileName() const { return _fileName; }
	const DWORD timer() const { return _timer.getValue(_useDefaults); }
	BOOL keepGlobalRangeKey() const { return _keepGlobalRangeKey.getValue(_useDefaults); }
	bool areOK() { return _ok; }
	std::string actionName() { return actionName(_action); }
	std::string actionName(int i)
	{
		switch (i) {
		case NOACTION:			return std::string("NOACTION");
		case LIST:				return std::string("LIST");
		case LISTSPS:			return std::string("LISTSPS");
		case LISTUSERS:			return std::string("LISTUSERS");
		case LISTMBRCTRL:		return std::string("LISTMBRCTRL");
		case LISTRANGES:		return std::string("LISTRANGES");
		case LEVEL0DISCOVERY:   return std::string("LEVEL0DISCOVERY");
		case LEVEL1DISCOVERY:	return std::string("LEVEL1DISCOVERY");
		case LISTMSID:			return std::string("LISTMSID");
		case TABLEOFTABLES:     return std::string("TABLEOFTABLES");
		case DUMPTABLEBYNAME:   return std::string("DUMPTABLEBYNAME");
		case LOADBYTETABLE:     return std::string("LOADBYTETABLE");
		case SAVEBYTETABLE:     return std::string("SAVEBYTETABLE");
		case LOADDATASTORE:     return std::string("LOADDATASTORE");
		case SAVEDATASTORE:     return std::string("SAVEDATASTORE");
		case LOADSHADOWMBR:     return std::string("LOADSHADOWMBR");
		case SAVESHADOWMBR:     return std::string("SAVESHADOWMBR");
		case ACTIVATESP:        return std::string("ACTIVATESP");
		case ENABLEUSER:        return std::string("ENABLEUSER");
		case DISABLEUSER:       return std::string("DISABLEUSER");
		case CHANGEPASSWORD:    return std::string("CHANGEPASSWORD");
		case MBRCONTROL:		return std::string("MBRCONTROL");
		case SETRANGELOCK:		return std::string("SETRANGELOCK");
		case CREATERANGE:		return std::string("CREATERANGE");
		case ERASERANGE:		return std::string("ERASERANGE");
		case SETRDLOCKEDUID:	return std::string("SETRDLOCKEDUID");
		case SETWRLOCKEDUID:	return std::string("SETWRLOCKEDUID");
		case REVERT:	        return std::string("REVERT");
		case REVERTDRIVE:		return std::string("REVERTDRIVE");
		case REVERTLOCKINGSP:	return std::string("REVERTLOCKINGSP");
		case REBOOT:			return std::string("REBOOT");
		case RESETOPTIONS:		return std::string("RESETOPTIONS");
		case TESTPASSWORD:		return std::string("TESTPASSWORD");
		case RANDOM:			return std::string("RANDOM");
		default:				return std::string("Undefined");
		}
	}

private:
	void Usage(const char* command);
	bool processArgument(const char* argType, const char* argValue,int &i);
	bool processBatchFile(std::string batchFileName);
	bool setAction(Actions A);

	Actions _action;

	// member data
	optionWithDefault<ArgumentsPW> _password;
	ArgumentsPW _targetPassword;
	optionWithDefault<std::string> _uid;
	std::string _targetuid;
	optionWithDefault<std::string> _securityProvider;
	std::string _drive;
	std::string _tableName;
	std::string _fileName;
	int  _rangeID;
	QWORD _rangeStart;
	QWORD _rangeLen;
	BOOL _readlock;// tri-state: -1 not set, 0=false, 1=true
	BOOL _writelock;// tri-state: -1 not set, 0=false, 1=true
	BOOL _readlockenable;// tri-state: -1 not set, 0=false, 1=true
	BOOL _writelockenable;// tri-state: -1 not set, 0=false, 1=true
	BOOL _MBRctrlDone;// tri-state: -1 not set, 0=false, 1=true
	BOOL _MBRctrlEnable;// tri-state: -1 not set, 0=false, 1=true
	unsigned int defaultMask;
	bool _useDefaults;
	optionWithDefault<DWORD> _timer;
	optionWithDefault<BOOL> _keepGlobalRangeKey;
	bool _ok;
};
