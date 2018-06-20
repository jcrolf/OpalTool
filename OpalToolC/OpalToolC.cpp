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
#include "Admin.h"
#include"Memory.h"

#include "CommandArguments.h"
#include "UtilitiesC.h"
#include "DriveC.h"

using namespace std;


#include "OpalToolC_API.h"



int main(int argc, char* argv[])
{	
	debugTrace t;	

	/* Make sure we're admin. */
	if(ElevateMe() == FALSE) {
		cerr << _T("You must be an administrator to run this program.\n");
		return 1;
	}

	/* Initialize memory. */
	MemInit(NULL);

	int rv=0;
	Arguments args(argc,argv);

	args.Audit();

	if (args.areOK())
	{
		rv=executeAction(args);
	}
	if (t.On(debugTrace::TRACE_DEBUG))
		std::cout << "done with action, calling MemCleanup\n";
	MemCleanup();
	if (t.On(debugTrace::TRACE_DEBUG))
		std::cout << "back from MemCleanup\n";
	return rv;
}

