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

#ifndef TCG_H_
#define TCG_H_

/* Needed include file. */
#include"Packet.h"


/* The structure for TCG drives. */
typedef struct tagTcgDrive {
	LPATADRIVE		pAtaDrive;			/* Pointer to the ATA drive information. */
	BOOL			UseDMA;			/* True to use DMA versions of the trusted commands, false to use the PIO ones. */
	DWORD			BufferSize;		/* The size of the TPer's transmit or receive buffer, whichever is smaller. */
	WORD			FirstComID;		/* The first com ID of the appropriate feature set. */
	BOOL			IsSamsung;		/* True if we think the drive is a first generation Samsung drive. */
	LPTCGFULLPACKET	ComPacket;		/* A pointer to the ComPacket to be transferred. */
} TCGDRIVE, *LPTCGDRIVE;


/* The structure used in sessions. */
typedef struct tagSession {
	LPTCGDRIVE			pTcgDrive;				/* Handle to the drive we're talking to. */
	DWORD				HostNumber;			/* The host's session identifier. */
	DWORD				TPerNumber;			/* The TPer's session identifier. */
} TCGSESSION, *LPTCGSESSION;


/* The structure used for authorizations. */
typedef struct tagTcgAuth {
	BOOL				IsValid;			/* True if the information is valid, false for no user specified. */
	BYTE				Authority[8];		/* The authority Uid. */
	BYTE				Credentials[32];	/* Authority's password, maximum 32 bytes per Core Spec. */
	DWORD				Size;				/* The size, in bytes, of the credentials. */
} TCGAUTH, *LPTCGAUTH;


/* The list of properties to send to the TPer. */
typedef struct tagProperties {
	LPTSTR		String;
	DWORD		Value;
} PROPERTIES, *LPPROPERTIES;


/* Function prototypes. */
BOOL Level0Discovery(LPTCGDRIVE pTcgDrive);
DWORD GetLockingFeatures(LPTCGDRIVE pTcgDrive);
BOOL IsLockingEnabled(LPTCGDRIVE pTcgDrive);
WORD GetTCGDriveType(LPTCGDRIVE pTcgDrive);
LPSTR GetFeatureCode(WORD Type);
LPTABLE Level1Discovery(LPTCGDRIVE pTcgDrive, LPPROPERTIES Properties);
BYTE StartSession(LPTCGSESSION Session, LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPTCGAUTH pTcgAuth);
BOOL CheckAuth(LPTCGDRIVE pTcgDrive, LPBYTE SpUid, LPTCGAUTH pTcgAuth);
BYTE ReadByteTable(LPTCGSESSION Session, LPBYTE TableUid, LPBYTE Buffer, DWORD Start, DWORD Size);
BYTE WriteByteTable(LPTCGSESSION Session, LPBYTE TableUid, LPBYTE Buffer, DWORD Start, DWORD Size);
LPTABLECELL ReadTableCell(LPTCGSESSION Session, LPBYTE RowUid, int Column);
BOOL ReadTableCellBytes(LPTCGSESSION Session, LPBYTE RowUid, int Column, LPBYTE Buffer, LPDWORD Size);
BOOL ReadTableCellDword(LPTCGSESSION Session, LPBYTE RowUid, int Column, LPDWORD Value);
LPTABLE ReadTable(LPTCGSESSION Session, LPBYTE TableUid);
void ReadTableRow(LPTCGSESSION Session, LPBYTE RowUid, int RowNum, LPTABLE pTable);
LPTABLE ReadTableNoSession(LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPBYTE TableUid, LPTCGAUTH pTcgAuth);
BYTE GetRandom(LPTCGSESSION Session, int Size, LPBYTE Bytes);
BYTE Activate_SP(LPTCGSESSION Session, LPBYTE Sp);
BYTE Revert(LPTCGSESSION Session, LPBYTE Sp);
BYTE RevertSP(LPTCGSESSION Session, BOOL KeepGlobalKey);
BOOL ChangeAuth(LPTCGSESSION Session, LPTCGAUTH pTcgAuth);
BYTE CreateRange(LPTCGSESSION Session, LPBYTE RangeUid, QWORD RangeStart, QWORD RangeLength, BOOL EnableReadLock, BOOL EnableWriteLock);
BYTE SetReadLock(LPTCGSESSION Session, LPBYTE RangeUid, BOOL ReadLock);
BYTE SetWriteLock(LPTCGSESSION Session, LPBYTE RangeUid, BOOL WriteLock);
BYTE EraseRange(LPTCGSESSION Session, LPBYTE Uid);
BYTE SetMbrState(LPTCGSESSION Session, BOOL IsEnable, BOOL IsDone);
BYTE ChangeUserState(LPTCGSESSION Session, LPBYTE User, BOOL Enable);
BOOL ReadMSID(LPTCGDRIVE pTcgDrive, LPTCGAUTH pTcgAuth);
DWORD GetByteTableSize(LPTCGDRIVE pTcgDrive, LPBYTE Sp, LPBYTE TableUid);
BOOL EndSession(LPTCGSESSION Session);
void CloseTcgDrive(LPTCGDRIVE pTcgDrive);
LPTCGDRIVE OpenTcgDrive(LPTSTR Drive);

BYTE setRangeLockingUID(LPTCGSESSION Session, LPBYTE Uid, BYTE rangeID,bool read=true);

#endif /* TCG_H_ */

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
