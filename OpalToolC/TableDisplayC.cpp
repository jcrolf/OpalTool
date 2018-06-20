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


#include <stdafx.h>
#include <iostream>
#include <string>
#include <vector>
#include "Table.h"
#include "AtaDrive.h"
#include "Tcg.h"

#include "CommandArguments.h"

using namespace std;


const string DisplayGenericTableS(LPTABLE Table, LPTSTR heading, LPTSTR *Columns,bool namedColumns)
{
	string returnedText="NULL";
	LPTABLECELL		Iter;
	debugTrace t;


	char tempBuffer[256];

	int Rows = GetRows(Table);
	int Cols = GetCols(Table);

	if (t.On(debugTrace::TRACE_DEBUG)) {
		cout << "DisplayGenericTableS: Rows = " << Rows << ",Cols = " << Cols << "\n";
	}

	if (Rows==0) return returnedText;

	if ((Columns!=NULL) && !namedColumns) namedColumns=true;

	returnedText = heading + string("\r\n");
	vector<int> colWidths;
	for (int i = 0; i< Cols; i++)
	{
		colWidths.push_back(0);

		int firstType=-1;
		for (int j = 0; j< Rows; j++)
		{
			Iter = GetTableCell(Table, j, i);

			if(Iter != NULL) {
				if (firstType<0) 
					firstType=Iter->Type;
				else if (firstType!=Iter->Type) {
					cerr << "change in cell type row " << j << ", col " << i << "\n";
					cerr << "was " << firstType << " now is " << Iter->Type << "\n";
				}
				if(Iter->Type == TABLE_TYPE_INT) {
					colWidths[i]=10;
				} else if(Iter->Type == TABLE_TYPE_STRING) {
					if ((int)strlen((LPSTR)Iter->Bytes)+1 > colWidths[i])
						colWidths[i]=(int)strlen((LPSTR)Iter->Bytes)+1;
				} else if(Iter->Type == TABLE_TYPE_BINARY) {
					if (Iter->Size > colWidths[i])
						colWidths[i]=Iter->Size*3;
				}
			}
		}
	}
	

	for (int i = 0; (namedColumns && (i< Cols)); i++)
	{
		char columnIDBuffer[32];
		char *columnID;
		if (Columns==NULL)
		{
			sprintf_s(columnIDBuffer,"Column %2d",i);
			columnID=columnIDBuffer;
		}
		else
		{
			columnID=Columns[i];
		}
		
		returnedText += ((i==0) ? string("") : string(",")) + string(columnID);
		if (colWidths[i] < (int)strlen(columnID)) {
			colWidths[i]=(int)strlen(columnID)+1;
		}
		else {
			for (int j=strlen(columnID); j<colWidths[i]-1;j++)
				returnedText+=" ";
		}

	}
	
	returnedText+=(namedColumns) ? "\r\n\r\n":"\r\n";

	for (int i = 0; i< Rows; i++)
	{
		for (int j = 0; j< Cols; j++)
		{
			Iter = GetTableCell(Table, i, j);
			int lengthShouldBe=returnedText.length()+colWidths[j];
			if(Iter != NULL) {
				if (t.On(debugTrace::TRACE_DEBUG)) {
					cout << "Row " << i << ",Col " << j << ", Type= " << Iter->Type << ", Size = " << Iter->Size << "\n";
				}

				if(Iter->Type == TABLE_TYPE_INT) {
					wsprintf(tempBuffer, _T("%9d "), Iter->IntData);
					returnedText += tempBuffer;
				} else if(Iter->Type == TABLE_TYPE_STRING) {
					returnedText += string((LPSTR)Iter->Bytes);
					for (int k=strlen((LPSTR)Iter->Bytes); k<colWidths[j];k++)
						returnedText+=" ";	
				} else if(Iter->Type == TABLE_TYPE_BINARY) {
					for(int k=0; k<Iter->Size; k++) {
						wsprintf(tempBuffer, _T("%02X "), Iter->Bytes[k]);
						returnedText += tempBuffer;
					}
				}
			} else if (t.On(debugTrace::TRACE_DEBUG)) { 
				cout << "Row " << i << ",Col " << j << ":NULL\n"; 
			}

			for (int k = returnedText.length(); k<lengthShouldBe; k++)
				returnedText+=" ";
		}
		returnedText += "\r\n";
	}

	return returnedText;
}

const std::string DisplayTableC(const Arguments &args,LPTCGDRIVE hDrive)
{
	std::string tableDisplayed;

	return tableDisplayed;
}
