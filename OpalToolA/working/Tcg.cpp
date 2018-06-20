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



/**********************************************
 *                                            *
 *                                            *
 **********************************************/

#include"stdafx.h"
#include"AtaDrive.h"
#include"Table.h"
#include"Tcg.h"
#include"Token.h"
#include"Uid.h"
#include"Memory.h"
// TODO: Better error codes

// OPALTOOLC
#include <iostream>
//#define DEBUG_TABLES
// END OPALTOOLC


/* The feature descriptor header. */
typedef struct tagFeatureDescriptor {
	WORD		FeatureCode;
	BYTE		Version;
	BYTE		Length;
} FEATURE, *LPFEATURE;


/* The Locking Feature Descriptor. */
typedef struct tagLockingFeature {
	FEATURE		Feature;
	BYTE		Features;
	BYTE		Padding[11];
} LOCKINGFEATURE, *LPLOCKINGFEATURE;


/* The common header for an SSC. */
typedef struct tagSSC {
	FEATURE		Feature;
	WORD		BaseComId;
	WORD		NumberComIds;
} SSC, *LPSSC;


/* The default size of a packet. */
#define DEFAULTPACKETSIZE	2048


/*
 * Send the COM Packet within the session.
 */
static BOOL TrustedSendSession(LPTCGSESSION Session)
{
	int		Length;

	/* Round up length fields to indicate padding. */
	Length = BE_TO_LE_32(Session->pTcgDrive->ComPacket->ComPacket.Length) & 3;
	if(Length != 0) {
		Session->pTcgDrive->ComPacket->ComPacket.Length = LE_TO_BE_32((BE_TO_LE_32(Session->pTcgDrive->ComPacket->ComPacket.Length) + 4 - Length));
		Session->pTcgDrive->ComPacket->Packet.Length = LE_TO_BE_32((BE_TO_LE_32(Session->pTcgDrive->ComPacket->Packet.Length) + 4 - Length));
	}

	/* Determine the length from the packet length. */
	Length = BE_TO_LE_32(Session->pTcgDrive->ComPacket->ComPacket.Length) + sizeof(Session->pTcgDrive->ComPacket->ComPacket);
	Length = (Length + 511) >> 9;

	/* Send the packet. */
	return TrustedSend(Session->pTcgDrive->pAtaDrive, Session->pTcgDrive->UseDMA, 1, Session->pTcgDrive->FirstComID, (LPBYTE)(Session->pTcgDrive->ComPacket), Length);
}


/*
 * Receive the COM Packet within the session.
 */
static BOOL TrustedReceiveSession(LPTCGSESSION Session)
{
	LPBYTE	ReceiveBuffer;
	DWORD	OutstandingData;
	DWORD	Length;
	BOOL	Result;

	/* Set the receive buffer. */
	ReceiveBuffer = (LPBYTE)(Session->pTcgDrive->ComPacket);

	/* Give the drive some time to perform the command. */
	Sleep(10);

	/* Wait until the command has completed. */
	do {
		/* Get the result of the command. */
		Result = TrustedReceive(Session->pTcgDrive->pAtaDrive, Session->pTcgDrive->UseDMA, 1, Session->pTcgDrive->FirstComID, ReceiveBuffer, 1);

		/* Convert the outstanding data to a format we can use. */
		OutstandingData = BE_TO_LE_32(Session->pTcgDrive->ComPacket->ComPacket.OutstandingData);

		/* Get the length of the ComPacket. */
		Length = BE_TO_LE_32(Session->pTcgDrive->ComPacket->ComPacket.Length);

		/* Make sure that we need to wait on the drive, and there just isn't 1 byte of outstanding data. */
		if((OutstandingData == 1) && (Length != 0)) {
			break;
		}

		/* If there's outstanding data, wait a while. */
		if(OutstandingData == 1) {
			Sleep(100);
		}
	} while((Result != FALSE) && (OutstandingData == 1));

	/* The command has completed, so return immediately on failure. */
	if(Result == FALSE) {
		return FALSE;
	}

	/* Some Samsung drives don't set the OutstandingData field correctly, so check for that. */
	if((OutstandingData == 0) && (Length > (512 - sizeof(Session->pTcgDrive->ComPacket->ComPacket)))) {
		Length = (Length + sizeof(Session->pTcgDrive->ComPacket->ComPacket) + 511) >> 9;
		return TrustedReceive(Session->pTcgDrive->pAtaDrive, Session->pTcgDrive->UseDMA, 1, Session->pTcgDrive->FirstComID, ReceiveBuffer, (WORD)Length);
	}

	/* If we received all the information, also return. */
	if(OutstandingData == 0) {
		return TRUE;
	}

	/* If we received data, advance the receive buffer. */
	if(Session->pTcgDrive->ComPacket->ComPacket.Length != 0) {
		ReceiveBuffer += 512;
	}

	/* Read the remaining data. */
	Result = TrustedReceive(Session->pTcgDrive->pAtaDrive, Session->pTcgDrive->UseDMA, 1, Session->pTcgDrive->FirstComID, ReceiveBuffer, (WORD)((OutstandingData + 511) >> 9));

	/* Return the result of the operation. */
	return Result;
}


/*
 * Send a command and receive a response in a single function call.
 */
static BOOL TrustedCommand(LPTCGSESSION Session)
{
	BOOL	Result;

	/* Send the packet. */
	Result = TrustedSendSession(Session);

	/* Get the response. */
	if(Result) {
		Result = TrustedReceiveSession(Session);
	}

	/* Return the result of the commands. */
	return Result;
}


/*
 * Perform a level 0 discovery on the drive.
 */
BOOL Level0Discovery(LPTCGDRIVE pTcgDrive)
{
	return TrustedReceive(pTcgDrive->pAtaDrive, pTcgDrive->UseDMA, 1, 1, pTcgDrive->pAtaDrive->Scratch, 1);
}


/*
 * Return a bitmask of the Locking Features.
 */
DWORD GetLockingFeatures(LPTCGDRIVE pTcgDrive)
{
	LPLOCKINGFEATURE	LockingFeature;
	LPFEATURE			Feature;

	/* Perform a level 0 discovery. */
	if(Level0Discovery(pTcgDrive) == FALSE) {
		return 0x80000000;
	}

	/* Search for the Locking Feature Descriptor. */
	Feature = (LPFEATURE)&(pTcgDrive->pAtaDrive->Scratch[48]);
	while((BE_TO_LE_16(Feature->FeatureCode) != 0) && (BE_TO_LE_16(Feature->FeatureCode) < 2)) {
		Feature = (LPFEATURE)((LPBYTE)Feature + Feature->Length + sizeof(FEATURE));
	}

	/* Make sure there's one. */
	if(BE_TO_LE_16(Feature->FeatureCode) != 2) {
		return 0x80000000;
	}

	/* Return the feature. */
	LockingFeature = (LPLOCKINGFEATURE)Feature;
	return LockingFeature->Features;
}


/*
 * Quick check to determine whether the Locking SP is active.
 */
BOOL IsLockingEnabled(LPTCGDRIVE pTcgDrive)
{
	/* Get the Locking feature.  Note: on error, the locking enable bit is zero.*/
	return (GetLockingFeatures(pTcgDrive) & 0x02);
}

static LPFEATURE locateSSC_FeatureRecord(LPTCGDRIVE pTcgDrive)
{
	LPFEATURE	SSC_FeatureRecord=nullptr;
	LPFEATURE	Feature=(LPFEATURE)&(pTcgDrive->pAtaDrive->Scratch[48]);
	WORD		FeatureHWM=0;

	WORD Temp=BE_TO_LE_16(Feature->FeatureCode);
	bool done=false;
	while((Temp != 0) && (!done)) {
		if ((Temp>=0x200) && (Temp<=0x203) && (Temp>FeatureHWM)) {// We will go with the highest supported OPAL version up to V2.00
			FeatureHWM=Temp;
			SSC_FeatureRecord=Feature;
		}

		if (Temp==0x203) {
			done=true;// Since OPAL 2.00 is as high as we support, if we find it, we are done
		}
		else {
			Feature = (LPFEATURE)((LPBYTE)Feature + Feature->Length + sizeof(FEATURE));
			Temp=BE_TO_LE_16(Feature->FeatureCode);
		}
	}

	return SSC_FeatureRecord;
}

/*
 * Get the drive type from the SSC.
 */
WORD GetTCGDriveType(LPTCGDRIVE pTcgDrive)
{
	LPFEATURE	Feature;
	WORD		Result=0;

	/* Send a level 0 discovery. */
	if(Level0Discovery(pTcgDrive) == FALSE) {
		return 0;
	}

	Feature = locateSSC_FeatureRecord(pTcgDrive);
	if (Feature!=nullptr)
		Result=BE_TO_LE_16(Feature->FeatureCode);

	/* Return the TCG feature code. */
	return Result;
}


/*
 * Get the first statically allocated Com ID.
 */
static WORD GetFirstComID(LPTCGDRIVE pTcgDrive)
{
	WORD			Result=0;
	LPSSC			Ssc;

	/* Send a level 0 discovery. */
	if(Level0Discovery(pTcgDrive) == FALSE) {
		return 0;
	}

	LPFEATURE SSC_FeatureRecord = locateSSC_FeatureRecord(pTcgDrive);

	/* Make sure there's one. */
	if (SSC_FeatureRecord==nullptr) {
		return 0;
	}

	/* Return the Com ID. */
	Ssc = (LPSSC)SSC_FeatureRecord;
	return BE_TO_LE_16(Ssc->BaseComId);
}


/*
 * Converts a drive type into a readable string.
 */
LPSTR GetFeatureCode(WORD Type)
{
	switch(Type) {
		case 0:
			return "N/A";

		case 0x100:
			return "Enterprise";

		case 0x200:
			return "Opal 1.0";

		case 0x203:
			return "Opal 2.00";

		default:
			return "Unknown";

	}
}


/*
 * Initialize a ComPacket with the session information.
 */
static void InitComPacket(LPTCGSESSION Session, LPBYTE InvokingUID, LPBYTE MethodUID)
{
	/* Zero out the ComPacket. */
	memset(Session->pTcgDrive->ComPacket, 0, Session->pTcgDrive->BufferSize);

	/* Set the Com ID. We don't set the Extended Com IDs size we only use static COM IDs. */
	Session->pTcgDrive->ComPacket->ComPacket.ComID = BE_TO_LE_16(Session->pTcgDrive->FirstComID);

	/* Set the session numbers. */
	Session->pTcgDrive->ComPacket->Packet.HostSessionNumber = BE_TO_LE_32(Session->HostNumber);
	Session->pTcgDrive->ComPacket->Packet.TPerSessionNumber = BE_TO_LE_32(Session->TPerNumber);

	/* Set the length fields. */
	Session->pTcgDrive->ComPacket->ComPacket.Length = LE_TO_BE_32(sizeof(TCGPACKET) + sizeof(TCGSUBPACKET));
	Session->pTcgDrive->ComPacket->Packet.Length = LE_TO_BE_32(sizeof(TCGSUBPACKET));

	/* Add the function call. */
	if(InvokingUID != NULL) {
		AddTokenCall(Session->pTcgDrive->ComPacket, InvokingUID, MethodUID);
	}
}


/*
 * Perform a Level 1 Discovery on the drive.
 */
LPTABLE Level1Discovery(LPTCGDRIVE pTcgDrive, LPPROPERTIES Properties)
{
	TCGSESSION	Session;
	LPTOKENS	Tokens;
	LPTABLE		pTable;
	BYTE		Response;
	BOOL		Result;
	int		CurrentRow;
	int		i;


	/* Initialize session variables. */
	Session.HostNumber = 0;
	Session.TPerNumber = 0;
	Session.pTcgDrive = pTcgDrive;

	/* Initialize the ComPacket. */
	InitComPacket(&Session, SM.Uid, SM_PROPERTIES.Uid);

	/* Add the properties we want to communicate to the TPer. */
	if(Properties != NULL) {
		AddSimpleToken(Session.pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session.pTcgDrive->ComPacket, 0);
		AddSimpleToken(Session.pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
		for(i=0; Properties[i].String!=NULL; i++) {
			AddSimpleToken(Session.pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
			AddTokenBytes(Session.pTcgDrive->ComPacket, (LPBYTE)Properties[i].String, lstrlen(Properties[i].String));
			AddTokenUnsignedInt(Session.pTcgDrive->ComPacket, Properties[i].Value);
			AddSimpleToken(Session.pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
		}
		AddSimpleToken(Session.pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
		AddSimpleToken(Session.pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session.pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(&Session);
	if(Result == FALSE) {
		return NULL;
	}

	/* Parse the response for validity. */
	Tokens = ParseResponse(Session.pTcgDrive->ComPacket);

	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session.pTcgDrive->ComPacket);

	/* Create a blank pTable. */
	pTable = CreateTable();
	if(pTable == NULL) {
		return NULL;
	}

	/* Parse the information into a pTable. */
	CurrentRow = 0;
	if(Response == 0) {
		for(i=0; i<Tokens->Count; i++) {
			if(Tokens->Tokens[i].Type == TCG_TOKEN_STARTNAME) {
				AddCell(pTable, CurrentRow, 0, Tokens->Tokens[i+1].Integer, Tokens->Tokens[i+1].Bytes);
				AddCell(pTable, CurrentRow, 1, Tokens->Tokens[i+2].Integer, Tokens->Tokens[i+2].Bytes);
				CurrentRow++;
			}
		}
	}

	/* Free up allocated memory. */
	MemFree(Tokens);

	/* Return the pTable of information. */
	return pTable;
}


/*
 * Determine the TPer's buffer size.
 */
static void NotifyTPerBufferSize(LPTCGDRIVE pTcgDrive)
{
	LPTABLECELL		Cell;
	PROPERTIES		Properties[8];
	LPTABLE			pTable;
	int			Rows;
	int			i;

	/* Fill up the properties list. */
	Properties[0].String = _T("MaxComPacketSize");
	Properties[0].Value = pTcgDrive->BufferSize;
	Properties[1].String = _T("MaxResponseComPacketSize");
	Properties[1].Value = pTcgDrive->BufferSize;
	Properties[2].String = _T("MaxPacketSize");
	Properties[2].Value = pTcgDrive->BufferSize - 20;
	Properties[3].String = _T("MaxIndTokenSize");
	Properties[3].Value = pTcgDrive->BufferSize - 56;
	Properties[4].String = _T("MaxPackets");
	Properties[4].Value = 1;
	Properties[5].String = _T("MaxSubpackets");
	Properties[5].Value = 1;
	Properties[6].String = _T("MaxMethods");
	Properties[6].Value = 1;
	Properties[7].String = NULL;
	Properties[7].Value = 0;

	/* Perform a level 1 discovery. */
	pTable = Level1Discovery(pTcgDrive, Properties);

	/* If there was an error, do nothing. */
	if(pTable == NULL) {
		pTcgDrive->BufferSize = DEFAULTPACKETSIZE;
		return;
	}

	/* Find the values of MaxComPacketSize and MaxResponseComPacketSize. */
	Rows = GetRows(pTable);
	for(i=0; i<Rows; i++) {
		Cell = GetTableCell(pTable, i, 0);
		if((lstrcmpiA((LPSTR)Cell->Bytes, "MaxComPacketSize") == 0) || (lstrcmpiA((LPSTR)Cell->Bytes, "MaxResponseComPacketSize") == 0)) {
			Cell = GetTableCell(pTable, i, 1);
			pTcgDrive->BufferSize = min(pTcgDrive->BufferSize, (DWORD)Cell->IntData);
		}
	}

	/* Free up resources. */
	FreeTable(pTable);
}


/*
 * Start a session with an SP.
 */
static bool useSamsungMitigation=false;

BYTE StartSession(LPTCGSESSION Session, LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPTCGAUTH pTcgAuth)
{
	LPTOKENS	Tokens;
	BYTE		Response;
	BOOL		Result;

	/* Initialize session variables. */
	Session->HostNumber = 0;
	Session->TPerNumber = 0;
	Session->pTcgDrive = pTcgDrive;
	useSamsungMitigation=false;

	/* Initialize the ComPacket. */
	InitComPacket(Session, SM.Uid, SM_STARTSESSION.Uid);

	/* Add parameters. */
	Session->HostNumber = 1;
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, Session->HostNumber);
	AddTokenBytes(Session->pTcgDrive->ComPacket, Sp, 8);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);

	/* Add authentication parameters, if requested. */
	if((pTcgAuth != NULL) && (pTcgAuth->IsValid != FALSE)) {
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 0);
		AddTokenBytes(Session->pTcgDrive->ComPacket, pTcgAuth->Credentials, pTcgAuth->Size);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 3);
		AddTokenBytes(Session->pTcgDrive->ComPacket, pTcgAuth->Authority, 8);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Parse the repsonse for validity. */
	Tokens = ParseResponse(Session->pTcgDrive->ComPacket);

	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session->pTcgDrive->ComPacket);

	/* The TPer session should be the 6th non-empty token. */
	if((Response == 0) && (Tokens->Count >= 6)) {
		Session->TPerNumber = Tokens->Tokens[5].Integer;
	}

	/* Free up allocated resources. */
	MemFree(Tokens);

	/* Return the response code from the drive. */
	return Response;
}


/*
 * Check a user and password against an SP.
 */
BOOL CheckAuth(LPTCGDRIVE pTcgDrive, LPBYTE SpUid, LPTCGAUTH pTcgAuth)
{
	TCGSESSION	Session;
	BYTE		Result;

	/* Start an authorized session to the SP. */
	Result = StartSession(&Session, pTcgDrive, SpUid, pTcgAuth);

	/* If there was a problem, return. */
	if(Result != 0) {
		return FALSE;
	}

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Return success. */
	return TRUE;
}


/*
 * Retrieve the next row of a pTable.
 */
static BYTE GetNextRow(LPTCGSESSION Session, LPBYTE pTable, LPBYTE CurrentRow, LPBYTE NextRow)
{
	LPBYTE		Ptr;
	BYTE		Response;
	BOOL		Result;

#ifdef DEBUG_TABLES
	std::cout << "GetNextRow A,";
#endif
	/* Early Samsung drives don't support the Next method, so return an error. */
	//if(Session->pTcgDrive->IsSamsung != FALSE) {
	//	return 0x3f;
	//}

	/* Initialize the ComPacket. */
	InitComPacket(Session, pTable, METHOD_NEXT.Uid);

	/* Add the current row parameter. */
	if(CurrentRow != NULL) {
#ifdef DEBUG_TABLES
		std::cout << "current row = "; for (int i=0;i<8;i++) printf("%02x ",CurrentRow[i]);std::cout <<"\n";
#endif

		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 0);
		AddTokenBytes(Session->pTcgDrive->ComPacket, CurrentRow, 8);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}

	/* Add the count of rows to return. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, ((useSamsungMitigation && (CurrentRow!=NULL)) ? 2 : 1));	
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}
#ifdef DEBUG_TABLES
	std::cout << "GetNextRow B, Result = " << Result << "\n";
#endif

	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session->pTcgDrive->ComPacket);

	/* Copy the next row. */
	if(Response == 0){
		if (useSamsungMitigation)
		{
			LPTOKENS returnedUids=ParseResponse(Session->pTcgDrive->ComPacket);
			
#ifdef DEBUG_TABLES
			std::cout << "We got back " << returnedUids->Count << " tokens in response to the NEXT query\n";
			for (int i=0;i<returnedUids->Count; i++) {
				std::cout << "Token[" << i << "]->Type = ";
				if (returnedUids->Tokens[i].Type==TCG_TOKEN_TYPE_BYTES) {
					std::cout << "BYTES\n[ ";
					for (int j=0; j<returnedUids->Tokens[i].Integer; j++) {
						if ((j%40==0) && (j!=0)) printf("\n"); printf("%02x ",returnedUids->Tokens[i].Bytes[j]);
					}
					std::cout <<" ]\n";
				}
				else if (returnedUids->Tokens[i].Type==TCG_TOKEN_TYPE_INT) {
					std::cout << "INT : [ " << returnedUids->Tokens[i].Integer;
					printf("(0x%08x) ]\n",returnedUids->Tokens[i].Integer);
				}
				else printf("%02x\n",returnedUids->Tokens[i].Type);
			}
#endif
			int bytesLocation=-1;
			for (int i=0;i<returnedUids->Count; i++)
				if (returnedUids->Tokens[i].Type==TCG_TOKEN_TYPE_BYTES)
					bytesLocation=i;

			if (bytesLocation>=0)
			{
				if (CurrentRow!=NULL) {
					if (memcmp(returnedUids->Tokens[bytesLocation].Bytes,CurrentRow,8)==0)
						Response = 0x3f;
					if (returnedUids->Tokens[bytesLocation].Integer!=8)
						Response = 0x3d;
				}
			}
			else 
			{
				Response = 0x3f;
			}
			
			if (Response==0)
				memcpy(NextRow,returnedUids->Tokens[bytesLocation].Bytes,8);

			MemFree(returnedUids);
		}
		else {
			Ptr = GetResponseBytes(Session->pTcgDrive->ComPacket, NULL);
    		if(Ptr != NULL) {
#ifdef DEBUG_TABLES
				std::cout << "response row = "; for (int i=0;i<16;i++) printf("%02x ",Ptr[i]);std::cout <<"\n";
#endif			
				memcpy(NextRow, Ptr, 8);
			} else {
				Response = 0x3f;
			}
		}
	}
#ifdef DEBUG_TABLES
	std::cout << "GetNextRow C: Response = " << (unsigned int)Response << "\n";
#endif

	if (Response==0)
		if (CurrentRow!=NULL)
			if (memcmp(CurrentRow,NextRow,8)==0)
			{
				if (!useSamsungMitigation) {
#ifdef DEBUG_TABLES
	std::cout << "GetNextRow D: Activate the samsung mitigation ...\n";
#endif
					useSamsungMitigation=true;
					Response = GetNextRow(Session, pTable, CurrentRow, NextRow);
				}
			}

	/* Return the response code from the drive. */
	return Response;
}


/*
 * Read specific columns from a row in a pTable.
 */
static BOOL ReadRowColumns(LPTCGSESSION Session, LPBYTE Row, int StartColumn, int EndColumn)
{
	/* Initialize the ComPacket. */
#ifdef DEBUG_TABLES
	std::cout << "ReadRowColumns: RowID "; for (int i=0;i<8;i++) printf("%02x ",Row[i]); 
	std::cout << "   start = " << StartColumn << ", end = " << EndColumn << "\n";
	std::cout << "\n";
#endif

	InitComPacket(Session, Row, METHOD_GET.Uid);

	/* Add the cell block information. */
	AddTokenCellBlock(Session->pTcgDrive->ComPacket, NULL, NULL, -1, -1, StartColumn, EndColumn);
	//AddTokenCellBlock(Session->pTcgDrive->ComPacket, NULL, NULL, 0, 0, StartColumn, EndColumn);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	return TrustedCommand(Session);
}


/*
 * Read bytes from a byte pTable.
 */
BYTE ReadByteTable(LPTCGSESSION Session, LPBYTE TableUid, LPBYTE Buffer, DWORD Start, DWORD Size)
{
	LPBYTE		Ptr;
	BYTE		Response;
	BYTE		TableRow[8];
	BOOL		Result;

	/* Convert the pTable uid into pTable row 0. */
	memset(TableRow, 0, sizeof(TableRow));
	memcpy(TableRow, &TableUid[4], 4);

	/* Initialize the ComPacket. */
	InitComPacket(Session, TableRow, METHOD_GET.Uid);

	/* Add the data information. */
	AddTokenCellBlock(Session->pTcgDrive->ComPacket, NULL, NULL, Start, Start+Size-1, -1, -1);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session->pTcgDrive->ComPacket);

	/* Save off buffer. */
	if(Response == 0) {
		Ptr = GetResponseBytes(Session->pTcgDrive->ComPacket, NULL);
		if(Ptr != NULL) {
			memcpy(Buffer, Ptr, Size);
		} else {
			Response = 0x3f;
		}
	}

	/* Return the response code from the drive. */
	return Response;
}


/*
 * Write bytes to a byte pTable.
 */
BYTE WriteByteTable(LPTCGSESSION Session, LPBYTE TableUid, LPBYTE Buffer, DWORD Start, DWORD Size)
{
	BYTE	TableRow[8];
	BOOL	Result;

	/* Convert the pTable uid into pTable row 0. */
	memset(TableRow, 0, sizeof(TableRow));
	memcpy(TableRow, &TableUid[4], 4);

	/* Initialize the ComPacket. */
	InitComPacket(Session, TableRow, METHOD_SET.Uid);

	/* Add the data information. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 0);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, Start);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddTokenBytes(Session->pTcgDrive->ComPacket, Buffer, Size);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Read a single cell from a pTable.
 */
LPTABLECELL ReadTableCell(LPTCGSESSION Session, LPBYTE RowUid, int Column)
{
	LPTABLECELL	TableCell=NULL;
	LPTOKENS	Tokens;
	BYTE		Response;
	int			i;

	/* Read the specific column. */
	if(ReadRowColumns(Session, RowUid, Column, Column) == FALSE) {
		return NULL;
	}
#ifdef DEBUG_TABLES
	std::cout << "We got the column it seems ...\n";
#endif
	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session->pTcgDrive->ComPacket);

	/* Check the response first. */
	if(Response != 0) {
		return NULL;
	}
#ifdef DEBUG_TABLES
	std::cout << "We are success in terms of result Code ...\n";
#endif
	/* Find the contents of the cell. */
	Tokens = ParseResponse(Session->pTcgDrive->ComPacket);
	for(i=0; i<Tokens->Count; i++) {

#ifdef DEBUG_TABLES
		std::cout << "We have a token: Type=" << (void*)Tokens->Tokens[i].Type << ", Integer = " << (void*)Tokens->Tokens[i].Integer << "\n";
		if (Tokens->Tokens[i].Integer > 0) {
			std::cout << "Bytes = ";
			for (int k=0;k<Tokens->Tokens[i].Integer;k++) printf("%02x ",Tokens->Tokens[i].Bytes[k]);
			std::cout << "\n";
		}
#endif

		if(Tokens->Tokens[i].Type == TCG_TOKEN_STARTNAME) {
			TableCell = AddCell(NULL, 0, Column, Tokens->Tokens[i+2].Integer, Tokens->Tokens[i+2].Bytes);
			break;
		}
	}
	MemFree(Tokens);

	/* Return the pTable cell. */
	return TableCell;
}


/*
 * Read a single cell from a pTable containing only bytes.
 */
BOOL ReadTableCellBytes(LPTCGSESSION Session, LPBYTE RowUid, int Column, LPBYTE Buffer, LPDWORD Size)
{
	LPBYTE	Ptr;
	DWORD	LocalSize;

	/* Set the size in case there's an error. */
	if(Size != NULL) {
		*Size = 0;
	}

	/* Read the specific column. */
	if(ReadRowColumns(Session, RowUid, Column, Column) == FALSE) {
		return FALSE;
	}

	/* Check the response code from the drive. */
	if(GetResponseResultCode(Session->pTcgDrive->ComPacket) != 0) {
		return FALSE;
	}

	/* Find the contents of the cell. */
	Ptr = GetResponseBytes(Session->pTcgDrive->ComPacket, &LocalSize);

	/* Let the user know if there were no bytes. */
	if(Ptr == NULL) {
		return FALSE;
	}

	/* Copy the bytes. */
	memcpy(Buffer, Ptr, LocalSize);

	/* Set the size. */
	if(Size != NULL) {
		*Size = LocalSize;
	}

	/* Return success. */
	return TRUE;
}


/*
 * Read a single cell from a pTable containing an integer (32-bit).
 */
BOOL ReadTableCellDword(LPTCGSESSION Session, LPBYTE RowUid, int Column, LPDWORD Value)
{
	LPTOKENS	Tokens;
	BYTE		Response;
	int			i;

	/* Read the specific column. */
	if(ReadRowColumns(Session, RowUid, Column, Column) == FALSE) {
		return FALSE;
	}

	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session->pTcgDrive->ComPacket);

	/* Check the response first. */
	if(Response != 0) {
		return FALSE;
	}

	/* Find the contents of the cell. */
	Tokens = ParseResponse(Session->pTcgDrive->ComPacket);
	for(i=0; i<Tokens->Count; i++) {
		if(Tokens->Tokens[i].Type == TCG_TOKEN_STARTNAME) {
			*Value = (DWORD)(Tokens->Tokens[i+2].Integer);
			break;
		}
	}
	MemFree(Tokens);

	/* Return success. */
	return TRUE;
}


/*
 * Read a row of a pTable into memory.
 */
void ReadTableRow(LPTCGSESSION Session, LPBYTE RowUid, int RowNum, LPTABLE pTable)
{
	LPTOKENS	Tokens;
	BYTE		Response;
	int			i;

	/* Add the row to the pTable separately, in case we couldn't read it. */
	AddCell(pTable, RowNum, 0, 8, RowUid);

	/* Read the entire row. */
	if(ReadRowColumns(Session, RowUid, -1, -1) == FALSE) {
		return;
	}

	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session->pTcgDrive->ComPacket);

	/* Save row information. */
	if(Response == 0) {
		Tokens = ParseResponse(Session->pTcgDrive->ComPacket);
#ifdef DEBUG_TABLES
		std::cout << "We found " << Tokens->Count << " values for the rows\n";
#endif
		for(i=0; i<Tokens->Count; i++) {
			if(Tokens->Tokens[i].Type == TCG_TOKEN_STARTNAME) {
#ifdef DEBUG_TABLES
				std::cout << "Adding a cell to a row: " 
						  << Tokens->Tokens[i+1].Integer << ", "
						  << (int)Tokens->Tokens[i+2].Type << ","
						  << Tokens->Tokens[i+2].Integer << "\n";
#endif
				AddCell(pTable, RowNum, Tokens->Tokens[i+1].Integer, Tokens->Tokens[i+2].Integer, Tokens->Tokens[i+2].Bytes);
			}
		}
		MemFree(Tokens);
	}
}


/*
 * Read an entire pTable into memory.
 */
LPTABLE ReadTable(LPTCGSESSION Session, LPBYTE TableUid)
{
	LPTABLE		pTable;
	BYTE		RealTableUid[8];
	BYTE		Row[8];
	BYTE		NextRow[8];
	BYTE		Result;
	int			CurrentRow;

	/* Convert the pTable UID to the actual UID. */
	memset(&RealTableUid[4], 0, 4);
	memcpy(RealTableUid, &TableUid[4], 4);
#ifdef DEBUG_TABLES
	std::cout << "read pTable A -> real pTable uid = ";
	for (int i = 0; i<8; i++) printf("%02x ",RealTableUid[i]); std::cout << "\n";
#endif
	/* Get the first row of the pTable. */
	Result = GetNextRow(Session, RealTableUid, NULL, Row);
#ifdef DEBUG_TABLES
	std::cout << "GetNextRow Result = " << (unsigned int)Result << "\n";
#endif

	/* Initialize variables. */
	pTable = CreateTable();
	if(pTable == NULL) {
		return NULL;
	}
	CurrentRow = 0;

	while(Result == 0) {
		/* Read the entire row. */
		ReadTableRow(Session, Row, CurrentRow, pTable);

		/* Get the next row of the pTable. */
		Result = GetNextRow(Session, RealTableUid, Row, NextRow);

		if (Result==0) memcpy(Row,NextRow,8);
#ifdef DEBUG_TABLES
		std::cout << "Read Table Row (b): "; for(int i =0; i<8;i++) printf("%02x ",Row[i]); std::cout << "\n";
		printf("Result  = %02x\n",Result);
#endif
		/* Go to the next row. */
		CurrentRow++;
	}

	/* Check for no pTable entries. */
	if((pTable->NumRows == 0) || (pTable->NumColumns == 0)) {
		FreeTable(pTable);
		return NULL;
	}
#ifdef DEBUG_TABLES
	std::cout << "read pTable B. rows = " << Table->NumRows << ",cols = " << Table->NumColumns << "\n";
#endif

	/* Add text descriptions, if they are missing. */
	AddTextDescriptions(pTable);

	/* Sort the pTable by UID. */
	SortTable(pTable);

	/* Return the pTable information. */
	return pTable;
}


/*
 * Read a pTable from the drive without already having an open session.
 */

LPTABLE ReadTableNoSession(LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPBYTE TableUid, LPTCGAUTH pTcgAuth)
{
	TCGSESSION	Session;
	LPTABLE		pTable;
	BYTE		Result;

	/* Initialize the pTable, in case we don't read one. */
	pTable = NULL;
	
	/* Start the session. */
	Result = StartSession(&Session, pTcgDrive, Sp, pTcgAuth);

	/* If there was a problem, return. */
	if(Result == 0) {
#ifdef DEBUG_TABLES
		std::cout << "start session success ...\n";
#endif
		/* Read the pTable. */
		pTable = ReadTable(&Session, TableUid);

		/* Close the session, we're done for now. */
		EndSession(&Session);
	}
#ifdef DEBUG_TABLES
	else std::cerr << "Failed to start session ...\n";
#endif

	/* Return the pTable. */
	return pTable;
}


/*
 * Get random bytes from the drive.  This is an optional command in the
 * current specification, although Seagate supports it.  Further, Seagate's
 * support is limited to returning up to 32 bytes.
 */
BYTE GetRandom(LPTCGSESSION Session, int Size, LPBYTE Bytes)
{
	LPBYTE		Ptr;
	BYTE		Response;
	BOOL		Result;

	/* Early Samsung drives don't support the Random method, so quit now. */
	if(Session->pTcgDrive->IsSamsung != FALSE) {
		return 0x3f;
	}

	/* Initialize the ComPacket. */
	InitComPacket(Session, SP_THIS.Uid, METHOD_RANDOM.Uid);

	/* Request random bytes from the drive. */
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, Size);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Get the response code from the drive. */
	Response = GetResponseResultCode(Session->pTcgDrive->ComPacket);

	/* Copy the returned bytes. */
	if(Response == 0) {
		Ptr = GetResponseBytes(Session->pTcgDrive->ComPacket, NULL);
		if(Ptr != NULL) {
			memcpy(Bytes, Ptr, Size);
		} else {
			Response = 0x3f;
		}
	}

	/* Return the response code from the drive. */
	return Response;
}


/*
 * Activates an SP.
 */
BYTE Activate_SP(LPTCGSESSION Session, LPBYTE Sp)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, Sp, METHOD_OPAL_ACTIVATE.Uid);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Revert an SP through the Admin SP.
 */
BYTE Revert(LPTCGSESSION Session, LPBYTE Sp)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, Sp, METHOD_OPAL_REVERT.Uid);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Revert an SP through the specified SP.
 */
BYTE RevertSP(LPTCGSESSION Session, BOOL KeepGlobalKey)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, SP_THIS.Uid, METHOD_REVERTSP.Uid);

	/* Convert the boolean to a 0/1 value. */
	if(KeepGlobalKey != 0) {
		KeepGlobalKey = 1;
	}

	/* Add the parameter to keep the global key. Although an optional parameter, some Samsung drives require it. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 0x60000);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, KeepGlobalKey);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Change the user's authorization value.
 */
BOOL ChangeAuth(LPTCGSESSION Session, LPTCGAUTH pTcgAuth)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, pTcgAuth->Authority, METHOD_SET.Uid);

	/* Add the column to change and its new value. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 3);
	AddTokenBytes(Session->pTcgDrive->ComPacket, pTcgAuth->Credentials, pTcgAuth->Size);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Creates or modifies a range.
 */
BYTE CreateRange(LPTCGSESSION Session, LPBYTE RangeUid, QWORD RangeStart, QWORD RangeLength, BOOL EnableReadLock, BOOL EnableWriteLock)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, RangeUid, METHOD_SET.Uid);

	/* Ensure the boolean values are 0/1 values. */
	if((EnableReadLock != 0) && (EnableReadLock != -1)) {
		EnableReadLock = 1;
	}
	if((EnableWriteLock != 0) && (EnableWriteLock != -1)) {
		EnableWriteLock = 1;
	}

	/* Add the information to change and its new values. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	if(RangeStart != -1) {
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 3);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, RangeStart);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(RangeLength != -1) {
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 4);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, RangeLength);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(EnableReadLock != -1) {
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 5);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, EnableReadLock);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(EnableWriteLock != -1) {
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 6);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, EnableWriteLock);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Set or clear the read lock value for a range.
 */
BYTE SetReadLock(LPTCGSESSION Session, LPBYTE RangeUid, BOOL ReadLock)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, RangeUid, METHOD_SET.Uid);

	/* Ensure the boolean values are 0/1 values. */
	if(ReadLock != 0) {
		ReadLock = 1;
	}

	/* Add the information to change and its new values. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 7);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, ReadLock);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Set or clear the write lock value for a range.
 */
BYTE SetWriteLock(LPTCGSESSION Session, LPBYTE RangeUid, BOOL WriteLock)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, RangeUid, METHOD_SET.Uid);

	/* Ensure the boolean values are 0/1 values. */
	if(WriteLock != 0) {
		WriteLock = 1;
	}

	/* Add the information to change and its new values. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 8);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, WriteLock);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}

/*
 * Set range rdLocked UID.
 */
BYTE setRangeLockingUID(LPTCGSESSION Session, LPBYTE Uid, BYTE rangeID, bool read)
{
		BOOL		Result;

	/* Initialize the ComPacket. */
	BYTE RangeSpecificSETRWLOCKEDUID[8];
	
	if (read)
		  memcpy(RangeSpecificSETRWLOCKEDUID,ACE_LOCKINGRANGESETRDLOCKED.Uid,8);
	else  memcpy(RangeSpecificSETRWLOCKEDUID,ACE_LOCKINGRANGESETWRLOCKED.Uid,8);

	RangeSpecificSETRWLOCKEDUID[7]=rangeID;

	InitComPacket(Session, RangeSpecificSETRWLOCKEDUID, METHOD_SET.Uid);

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, 0x01);// vyyalues

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, 0x03);// boolean Expr

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);

	BYTE HalfUID_Authority_object_ref[4]={0x00,0x00,0x0c,0x05};

	AddTokenBytes(Session->pTcgDrive->ComPacket, HalfUID_Authority_object_ref, 4);
	AddTokenBytes(Session->pTcgDrive->ComPacket, Uid, 8);

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);

	AddTokenBytes(Session->pTcgDrive->ComPacket, HalfUID_Authority_object_ref, 4);
	AddTokenBytes(Session->pTcgDrive->ComPacket, Uid, 8);

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);

	BYTE HalfUID_boolean_ACE[4]={0x00, 0x00, 0x04, 0x0e};
	AddTokenBytes(Session->pTcgDrive->ComPacket, HalfUID_boolean_ACE, 4);
	AddSimpleToken(Session->pTcgDrive->ComPacket, 0x01);// vyyalues

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);

	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);


	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	LPBYTE bytes = &Session->pTcgDrive->ComPacket->Payload[0];
	for (int i=0;i<64;i++) {
		printf("%02x ",bytes[i]);
		if (i%32==31) printf("\n");
	}
	printf("\n");
	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}

/*
 * CryptoErase a range.
 */
BYTE EraseRange(LPTCGSESSION Session, LPBYTE Uid)
{
	BOOL		Result;

	/* Early Samsung drives don't support the GenKey method, so quit now. */
	if(Session->pTcgDrive->IsSamsung != FALSE) {
		return 0x3f;
	}

	/* Initialize the ComPacket. */
	InitComPacket(Session, Uid, METHOD_GENKEY.Uid);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Set or clear the write lock value for a range.
 */
BYTE SetMbrState(LPTCGSESSION Session, BOOL IsEnable, BOOL IsDone)
{
	BOOL		Result;

	/* If there's nothing to do, return success. */
	if((IsEnable == -1) && (IsDone == -1)) {
		return 0;
	}

	/* Initialize the ComPacket. */
	InitComPacket(Session, MBRCONTROL.Uid, METHOD_SET.Uid);

	/* Ensure the boolean values are 0/1/-1 values. */
	if((IsEnable != 0) && (IsEnable != -1)) {
		IsEnable = 1;
	}
	if((IsDone != 0) && (IsDone != -1)) {
		IsDone = 1;
	}

	/* Add the information to change and its new values. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	if(IsEnable != -1) {
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, IsEnable);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(IsDone != -1) {
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 2);
		AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, IsDone);
		AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	}
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Enable or disable a user.
 */
BYTE ChangeUserState(LPTCGSESSION Session, LPBYTE User, BOOL Enable)
{
	BOOL		Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, User, METHOD_SET.Uid);

	/* Ensure the boolean values are 0/1 values. */
	if(Enable != 0) {
		Enable = 1;
	}

	/* Add the information to change and its new values. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 1);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_STARTNAME);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, 5);
	AddTokenUnsignedInt(Session->pTcgDrive->ComPacket, Enable);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDNAME);

	/* Signify end of parameters and expected result. */
	AddTokenEndParameters(Session->pTcgDrive->ComPacket, 0);

	/* Send the packet and get the response. */
	Result = TrustedCommand(Session);
	if(Result == FALSE) {
		return 0x3f;
	}

	/* Return the response code from the drive. */
	return GetResponseResultCode(Session->pTcgDrive->ComPacket);
}


/*
 * Read the MSID for the drive..
 */
BOOL ReadMSID(LPTCGDRIVE pTcgDrive, LPTCGAUTH pTcgAuth)
{
	TCGSESSION		Session;
	BYTE			Result;
	BOOL			Response;

	/* Start an unauthenticated session to the Admin SP. */
	Result = StartSession(&Session, pTcgDrive, SP_ADMIN.Uid, NULL);

	/* If there was a problem, return. */
	if(Result != 0) {
		return FALSE;
	}

	/* Read the MSID. */
	Response = ReadTableCellBytes(&Session, C_PIN_MSID.Uid, 3, pTcgAuth->Credentials, &(pTcgAuth->Size));

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Return the result. */
	return Response;
}


/*
 * Get the size of the byte pTable from the SP.
 */
DWORD GetByteTableSize(LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPBYTE TableUid)
{
	TCGSESSION	Session;
	DWORD		TableSize;
	BYTE		Result;

	/* Start an unauthorized session to the SP. */
	Result = StartSession(&Session, pTcgDrive, Sp, NULL);

	/* If there was a problem, return. */
	if(Result != 0) {
		return 0;
	}

	/* Read the pTable of tables. */
	TableSize = 0;
	ReadTableCellDword(&Session, TableUid, 7, &TableSize);

	/* Close the session, we're done for now. */
	EndSession(&Session);

	/* Return the size. */
	return TableSize;
}


/*
 * Ends an open session with an SP.
 */
BOOL EndSession(LPTCGSESSION Session)
{
	BOOL	Result;

	/* Initialize the ComPacket. */
	InitComPacket(Session, NULL, NULL);

	/* Add the end of session token. */
	AddSimpleToken(Session->pTcgDrive->ComPacket, TCG_TOKEN_ENDSESSION);

	/* Send the end of session token and get the response. */
	Result = TrustedCommand(Session);

	/* Check that the TPer ended the session. */
	if(Result) {
		Result = CheckResponseForClose(Session->pTcgDrive->ComPacket);
	}

	/* Return the result of the end session. */
	return Result;
}


/*
 * Close a TCG drive, freeing up resources.
 */
void CloseTcgDrive(LPTCGDRIVE pTcgDrive)
{
	if(pTcgDrive != NULL) {
		if(pTcgDrive->ComPacket != NULL) {
			VirtualFree(pTcgDrive->ComPacket, 0, MEM_RELEASE);
		}
		CloseAtaDrive(pTcgDrive->pAtaDrive);
		MemFree(pTcgDrive);
	}
}


/*
 * Allocate a large static buffer.
 */
static void GetBuffer(LPTCGDRIVE pTcgDrive)
{
	/* Start with a 128K buffer. */
	pTcgDrive->BufferSize = 256*1024;
	pTcgDrive->ComPacket = NULL;

	/* Try to allocate buffers. */
	while((pTcgDrive->BufferSize >= 2048) && (pTcgDrive->ComPacket == NULL)) {
		pTcgDrive->BufferSize >>= 1;
		pTcgDrive->ComPacket = (LPTCGFULLPACKET)VirtualAlloc(NULL, pTcgDrive->BufferSize, MEM_COMMIT, PAGE_READWRITE);
	}
}


/*
 * Open a TCG drive and initialize the TCG parameters.
 */
LPTCGDRIVE OpenTcgDrive(LPTSTR Drive)
{
	TCGSESSION	Session;
	LPTCGDRIVE	pTcgDrive;
	LPATADRIVE	pAtaDrive;
	BYTE		Result;

	/* Open the ata drive. */
	pAtaDrive = OpenAtaDrive(Drive);
	if(pAtaDrive == NULL) {
		return NULL;
	}

	/* Create the structure. */
	pTcgDrive = (LPTCGDRIVE)MemCalloc(sizeof(TCGDRIVE));
	if(pTcgDrive == NULL) {
		CloseAtaDrive(pAtaDrive);
		return NULL;
	}

	/* Fill in the rest of the structure with default values. */
	pTcgDrive->pAtaDrive = pAtaDrive;
	pTcgDrive->UseDMA = TRUE;
	pTcgDrive->BufferSize = DEFAULTPACKETSIZE;

	/* Get the first Com ID. */
	pTcgDrive->FirstComID = GetFirstComID(pTcgDrive);

	/* If there was an error, try a different DMA setting. */
	if(pTcgDrive->FirstComID == 0) {
		pTcgDrive->UseDMA = FALSE;
		pTcgDrive->FirstComID = GetFirstComID(pTcgDrive);
	}

	/* If there's still an error, the drive is not a TCG drive. */
	if(pTcgDrive->FirstComID == 0) {
		return pTcgDrive;
	}

	/* Get the buffer size we can use. */
	GetBuffer(pTcgDrive);
	if(pTcgDrive->ComPacket == NULL) {
		CloseTcgDrive(pTcgDrive);
		return NULL;
	}

	/* Notify the TPer of our settings. */
	if(pTcgDrive->ComPacket != NULL) {
		NotifyTPerBufferSize(pTcgDrive);
	}

	/* Determine if the drive is a first generation Samsung drive. */
	pTcgDrive->IsSamsung = FALSE;
	Result = StartSession(&Session, pTcgDrive, SP_ADMIN.Uid, NULL);
	if(Result == 0) {
		if(ReadRowColumns(&Session, C_PIN_MSID.Uid, -1, -1) != FALSE) {
			pTcgDrive->IsSamsung = (GetResponseResultCode(Session.pTcgDrive->ComPacket) == 1);
		}
		EndSession(&Session);
	}

	/* Return the handle to the TCG drive. */
	return pTcgDrive;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
