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
#include <Windows.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include "Memory.h"
#include "CommandArguments.h"
#include "DriveC.h"
#include "ByteTableC.h"

static int fileSize(string fileName)
{
	struct stat stat_buf;
    int rc = stat(fileName.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

static bool authorizeAndGetTableUID(LPTCGDRIVE hDrive,LPBYTE spID, LPBYTE TableUID,Arguments &args, LPBYTE Uid)
{
	bool ok=true;
	debugTrace t;

	if (Uid==NULL)
	{
		if (t.On(debugTrace::TRACE_DEBUG))
			cout << "trying to do GetTOT with NULL, spid = " << dumpUID(spID) << "...\n";

		LPTABLE tableList=GetTOT(hDrive,spID,NULL,args);

		if (tableList!=NULL)
		{
			if (t.On(debugTrace::TRACE_DEBUG))
				cout << "We succeeded ...\n";

			int rows=GetRows(tableList);
			bool found=false;

			for (int i=0;(ok && !found && (i<rows));i++)
			{
				LPTABLECELL tableName=GetTableCell(tableList,i,1);

				if (args.tableName()==std::string(reinterpret_cast<char*>(tableName->Bytes)))
				{
					LPTABLECELL tableType=GetTableCell(tableList,i,4);

					if (t.On(debugTrace::TRACE_DEBUG))
						cout << "table type row " << i << " is " << tableType->IntData << "\n";

					if (tableType->IntData==TABLE_TYPE_BINARY)
					{
						LPTABLECELL tableUID=GetTableCell(tableList,i,0);
						memcpy(TableUID,tableUID->Bytes,tableUID->Size);
						found=true;
					}
					else {						
						cerr << "Invalid table Type: must be binary (" << TABLE_TYPE_BINARY << ")\n";
						ok=false;
					}
				}
			}

			if (found) 
				cout << "We located " << args.tableName() << " and its uid is " << dumpUID(TableUID) << "\n";
			else cerr << "We could not find " << args.tableName() << "\n";

			if (!ok) {
				cerr << "The legal tables are:\n\n";
				for (int i=0;i<rows;i++)
				{
					LPTABLECELL cell=GetTableCell(tableList,i,4);
					if ((cell!=NULL) && (cell->IntData==TABLE_TYPE_BINARY))
					{
						cell=GetTableCell(tableList,i,1);
						cerr << "   " << (char*)(cell->Bytes) << "\n";
					}
				}
			}
			FreeTable(tableList);
		}				
	}
	else
	{
		memcpy(TableUID,Uid,8);
	}

	return ok;
}

int saveByteTable(Arguments &args,LPBYTE Uid)
{
	LPTCGDRIVE	hDrive = openDriveByName(args.drive());
	int rv=1;

	if (hDrive!=NULL)
	{
		BYTE spID[8];
		// Look in the TABLEOFTABLES to locate the specified table and check to make sure it is a byte table
		// we also get the number of bytes required.

		if (isValidSP(hDrive,spID,args))
		{
			BYTE TableUID[8];

			if (authorizeAndGetTableUID(hDrive,spID,TableUID,args,Uid)) 
			{
				LPTABLE users=legalUsers(hDrive,spID);

				TCGAUTH TcgAuth;
				if (users && isValidUser(hDrive,users,args.uid(),&TcgAuth) &&
					setTcgAuthCredentials(hDrive,&TcgAuth,args.password()) )
				{
					int TableSize = GetByteTableSize(hDrive, spID, TableUID);

					int BufferSize = min((hDrive->BufferSize - 1024) & (~0x3ff), 63*1024);
					LPBYTE Buffer = (LPBYTE)MemAlloc(BufferSize);

					if(Buffer != NULL) {
						string outFileName=args.fileName();
						cout << "opening " << outFileName << "\n";

						FILE *dumpFile = NULL;
						fopen_s(&dumpFile,outFileName.c_str(),"wb");

						if (dumpFile!=NULL) 
						{
							TCGSESSION Session;
							BYTE Result = StartSession(&Session, hDrive, spID, &TcgAuth);

							if (Result==0) {
								/* Samsung drives can't read the last byte of the MBR table, so test for this and adjust accordingly. */
								Result = ReadByteTable(&Session, TableUID, Buffer, TableSize-1, 1);
								if(Result != 0) {
									TableSize--;
								}

								cout << "Table Size is " << TableSize << ", BufferSize is " << BufferSize << "\n";
								/* Save the datastore. */
								int progressUnit=100;
								rv=0;
								for(int i=0; i<TableSize; i+=BufferSize) {
									/* Read from the data store. */
									int ReadSize = min(BufferSize, TableSize-i);
									Result = ReadByteTable(&Session, TableUID, Buffer, i, ReadSize);
									if(Result != 0) {
										cerr << "There was an error reading the datastore.\n";
										rv=1;
										break;
									}

									/* Write to the file. */
									if (--progressUnit<=0) {
										printf("progress = %4.2f%%\r",((float)i/(float)TableSize)*100.);
										progressUnit=100;
									}

									if (fwrite(Buffer,1,ReadSize,dumpFile)!=ReadSize) {
										cerr << "Error writing to " << outFileName << ":" << strerror_s(strerrorBuf,errno) << "\n";
										rv=1;	
										break;
									}
								}
								if (rv==0)
									cout << "Done.                                \n";

								EndSession(&Session);
							}
							else 
							{
								cerr << "Unable to open a session.\n";
							}

							fclose(dumpFile);
						}
						else 
						{
							cerr << "Unable to open " << args.fileName() << ":" << strerror_s(strerrorBuf,errno) << "\n";
						}

						MemFree(Buffer);
					}
					else {
						cerr << "Out of memory.\n";
					}
					FreeTable(users);			
				}
			}
		}

		CloseTcgDrive(hDrive);
	}

	return rv;
}

int loadByteTable(Arguments &args,LPBYTE Uid)
{
	LPTCGDRIVE	hDrive = openDriveByName(args.drive());
	int rv=1;

	if (hDrive!=NULL)
	{
		BYTE spID[8];

		// Look in the TABLEOFTABLES to locate the specified table and check to make sure it is a byte table
		// we also get the number of bytes required.

		if (isValidSP(hDrive,spID,args))
		{
			BYTE TableUID[8];
			
			if (authorizeAndGetTableUID(hDrive,spID,TableUID,args,Uid)) 
			{
				// NOW We have the correct and associated UID ...
				LPTABLE users=legalUsers(hDrive,spID);

				TCGAUTH TcgAuth;
				if (users && isValidUser(hDrive,users,args.uid(),&TcgAuth) &&
					setTcgAuthCredentials(hDrive,&TcgAuth,args.password()) )
				{
					int TableSize = GetByteTableSize(hDrive, spID, TableUID);
					int FileSize=fileSize(args.fileName());

					if (FileSize<=TableSize)
					{
						int BufferSize = min((hDrive->BufferSize - 1024) & (~0x3ff), 63*1024);
						LPBYTE Buffer = (LPBYTE)MemAlloc(BufferSize);

						if(Buffer != NULL) {
							string inFileName=args.fileName();

							cout << "opening " << inFileName << "\n";

							FILE *dumpFile = NULL;
							fopen_s(&dumpFile,inFileName.c_str(),"rb");

							if (dumpFile!=NULL) 
							{
								TCGSESSION Session;
								BYTE Result = StartSession(&Session, hDrive, spID, &TcgAuth);

								if (Result==0) {
									cout << "Table Size is " << TableSize << ", BufferSize is " << BufferSize << "\n";
									/* write to the datastore. */
									int progressUnit=100;
									rv=0;
									for(int i=0; i<FileSize; i+=BufferSize) {
										/* Read from the file. */
										int WriteSize = min(BufferSize, FileSize-i);
										if (fread(Buffer,1,WriteSize,dumpFile)!=WriteSize) {
											cerr << "Error reading from " << inFileName << ":" << strerror_s(strerrorBuf,errno) << "\n";
											rv=1;
											break;
										}
										if (--progressUnit<=0) {
											printf("total written = %4.2f%%\r", ((float)i/(float)FileSize)*100.);
											progressUnit=100;
										}
										/* Write to the data store. */
										Result = WriteByteTable(&Session, TableUID, Buffer, i, WriteSize);
										if(Result != 0) {
											cerr << "There was an error writing the datastore.\n";
											rv=1;
											break;
										}

									}

									if (rv=0)
										cout << "Done.                             \n";

									EndSession(&Session);
								}
								else 
								{
									cerr << "Unable to open a session.\n";
								}

								fclose(dumpFile);
							}
							else 
							{
								cerr << "Unable to open " << args.fileName() << ":" << strerror_s(strerrorBuf,errno) << "\n";
							}

							MemFree(Buffer);
						}
						else {
							cerr << "Out of memory.\n";
						}
					}
					else
					{
						cerr << "Table " << args.tableName() << " is of size " << TableSize 
							 << " which is smaller than the input file " << args.fileName() << " = " << FileSize << "\n";
					}
					
					FreeTable(users);	
				}
			}
		}

		CloseTcgDrive(hDrive);
	}

	return rv;
}
