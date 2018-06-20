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
#pragma once

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
#include "UtilitiesC.h"
#include "DriveC.h"
#include "TableDisplayC.h"
#include "ByteTableC.h"

using namespace std;

int EnumDrivesByNumber_C();

int level0DiscoveryC(const string &drive);

int level1DiscoveryC(const string &drive);

int GetMSID_C(const string &drive);

int GetRandom_C(const string &drive,LPBYTE binary=NULL);

int GetSP_C(const string &drive);

int GetUsers_C(const Arguments &args);

int GetTable_C(const Arguments &args);

int GetTableOfTables_C(const Arguments &args);

int activateSP_C(const Arguments &args);

int enableDisableUser_C(const Arguments &args,bool enabledisable);

int changePassword_C(const Arguments &args);

int listMBRcontrol_C(const Arguments &args);

int setMBRcontrol_C(const Arguments &args);

int listRanges_C(const Arguments &args);

int setRangeLock_C(const Arguments &args);

int createRange_C(const Arguments &args);

int eraseRange_C(const Arguments &args);

int setRangeLockingUID_C(const Arguments &args,bool read=true);

int revert_C(const Arguments &args);

int revertDrive_C(const Arguments &args);

int revertLockingSP_C(const Arguments &args);

int testPassword_C(Arguments args);

int executeAction(Arguments &args);
