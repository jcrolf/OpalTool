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
#include"Memory.h"
#include"Packet.h"
#include"Token.h"

//#define DEBUG_RESPONSE
// TODO: Better error codes.


/*
 * Add the bytes to the end of the subpacket without processing.
 */
static void AddRawTokenBytes(LPTCGFULLPACKET ComPacket, LPBYTE Bytes, int Size)
{
	LPBYTE		Dest;

	/* Get a pointer to where the new bytes should go. */
	Dest = &(ComPacket->Payload[BE_TO_LE_32(ComPacket->SubPacket.Length)]);

	/* Copy the bytes. */
	memcpy(Dest, Bytes, Size);

	/* Update all the packet sizes. */
	ComPacket->ComPacket.Length = LE_TO_BE_32(Size + BE_TO_LE_32(ComPacket->ComPacket.Length));
	ComPacket->Packet.Length = LE_TO_BE_32(Size + BE_TO_LE_32(ComPacket->Packet.Length));
	ComPacket->SubPacket.Length = LE_TO_BE_32(Size + BE_TO_LE_32(ComPacket->SubPacket.Length));
}


/*
 * Adds a simple one-byte token to the packet.
 */
void AddSimpleToken(LPTCGFULLPACKET ComPacket, BYTE Token)
{
	AddRawTokenBytes(ComPacket, &Token, 1);
}


/*
 * Adds a string of bytes to the packet.
 */
void AddTokenBytes(LPTCGFULLPACKET ComPacket, LPBYTE Bytes, int Size)
{
	/* Handle short atoms first. */
	if(Size < 0x10) {
		AddSimpleToken(ComPacket, TCG_TOKEN_SHORTBYTESATOM | (Size & 0xff));

	/* Next handle medium atoms. */
	} else if(Size < 0x800) {
		AddSimpleToken(ComPacket, TCG_TOKEN_MEDIUMBYTESATOM | ((Size >> 8) & 0xff));
		AddSimpleToken(ComPacket, Size & 0xff);

	/* Finally handle large atoms. */
	} else {
		AddSimpleToken(ComPacket, TCG_TOKEN_LONGBYTESATOM);
		AddSimpleToken(ComPacket, (Size >> 16) & 0xff);
		AddSimpleToken(ComPacket, (Size >>  8) & 0xff);
		AddSimpleToken(ComPacket, (Size >>  0) & 0xff);
	}

	/* Now add the bytes. */
	AddRawTokenBytes(ComPacket, Bytes, Size);
}


/*
 * Adds a signed integer token to the packet.  The integer
 * is limited to 32-bits.
 */
void AddTokenSignedInt(LPTCGFULLPACKET ComPacket, int Integer)
{
	DWORD		Int2;
	LPBYTE		IntPtr;
	int			IntSize;

	/* Check for a tiny atom. */
	if((Integer >= -32) && (Integer <= 31)) {
		AddSimpleToken(ComPacket, (BYTE)((Integer & 0x3f) | 0x40));
		return;
	}

	/* Switch endianess of the integer. */
	Int2 = LE_TO_BE_32(Integer);
	IntPtr = (LPBYTE)&Int2;

	/* Determine the size of the integer. */
	if((Integer >= -0x80) && (Integer < 0x80)) {
		IntSize = 0;
	} else if((Integer >= -0x8000) && (Integer < 0x8000)) {
		IntSize = 1;
	} else if((Integer >= -0x800000) && (Integer < 0x800000)) {
		IntSize = 2;
	} else {
		IntSize = 3;
	}

	/* Add the token and the bytes. */
	AddSimpleToken(ComPacket, (BYTE)(0x90 + IntSize));
	AddRawTokenBytes(ComPacket, &IntPtr[4-IntSize], IntSize);
}


/*
 * Adds an unsigned integer token to the packet.  The integer
 * is limited to 64-bits.
 */
void AddTokenUnsignedInt(LPTCGFULLPACKET ComPacket, QWORD Integer)
{
	QWORD		Int2;
	LPBYTE		IntPtr;
	int			IntSize;

	/* Check for a tiny atom. */
	if(Integer < 64) {
		AddSimpleToken(ComPacket, (BYTE)Integer);
		return;
	}

	/* Switch endianess of the integer. */
	Int2 = LE_TO_BE_64(Integer);
	IntPtr = (LPBYTE)&Int2;

	/* Determine the size of the integer. */
	IntSize = 0;
	while(Integer > 0) {
		IntSize++;
		Integer >>= 8;
	}

	/* Add the token and the bytes. */
	AddSimpleToken(ComPacket, (BYTE)(0x80 + IntSize));
	AddRawTokenBytes(ComPacket, &IntPtr[sizeof(Int2)-IntSize], IntSize);
}


/*
 * Adds the bytes at the end of the ComPacket.
 */
void AddTokenEndParameters(LPTCGFULLPACKET ComPacket, BYTE Status)
{
	AddSimpleToken(ComPacket, TCG_TOKEN_ENDLIST);
	AddSimpleToken(ComPacket, TCG_TOKEN_ENDDATA);
	AddSimpleToken(ComPacket, TCG_TOKEN_STARTLIST);
	AddTokenUnsignedInt(ComPacket, Status);
	AddTokenUnsignedInt(ComPacket, 0);
	AddTokenUnsignedInt(ComPacket, 0);
	AddSimpleToken(ComPacket, TCG_TOKEN_ENDLIST);
}



/*
 * Adds the call method and the start list token.
 */
void AddTokenCall(LPTCGFULLPACKET ComPacket, LPBYTE InvokingUID, LPBYTE MethodUID)
{
	AddSimpleToken(ComPacket, TCG_TOKEN_CALL);
	AddTokenBytes(ComPacket, InvokingUID, 8);
	AddTokenBytes(ComPacket, MethodUID, 8);
	AddSimpleToken(ComPacket, TCG_TOKEN_STARTLIST);
}


/*
 * Add the tokens that correspond to the cell block to pass to the TPer.
 */
void AddTokenCellBlock(LPTCGFULLPACKET ComPacket, LPBYTE pTable, LPBYTE StartRowUid, int StartRowInt, int EndRow, int StartColumn, int EndColumn)
{
	AddSimpleToken(ComPacket, TCG_TOKEN_STARTLIST);
	if(pTable != NULL) {
		AddSimpleToken(ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(ComPacket, 0);
		AddTokenBytes(ComPacket, pTable, 8);
		AddSimpleToken(ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(StartRowUid != NULL) {
		AddSimpleToken(ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(ComPacket, 1);
		AddTokenBytes(ComPacket, StartRowUid, 8);
		AddSimpleToken(ComPacket, TCG_TOKEN_ENDNAME);
	} else if(StartRowInt != -1) {
		AddSimpleToken(ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(ComPacket, 1);
		AddTokenUnsignedInt(ComPacket, StartRowInt);
		AddSimpleToken(ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(EndRow != -1) {
		AddSimpleToken(ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(ComPacket, 2);
		AddTokenUnsignedInt(ComPacket, EndRow);
		AddSimpleToken(ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(StartColumn != -1) {
		AddSimpleToken(ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(ComPacket, 3);
		AddTokenUnsignedInt(ComPacket, StartColumn);
		AddSimpleToken(ComPacket, TCG_TOKEN_ENDNAME);
	}
	if(EndColumn != -1) {
		AddSimpleToken(ComPacket, TCG_TOKEN_STARTNAME);
		AddTokenUnsignedInt(ComPacket, 4);
		AddTokenUnsignedInt(ComPacket, EndColumn);
		AddSimpleToken(ComPacket, TCG_TOKEN_ENDNAME);
	}
	AddSimpleToken(ComPacket, TCG_TOKEN_ENDLIST);
}


/*
 * Skip empty tokens.
 */
static LPBYTE SkipEmptyTokens(LPBYTE Buffer)
{
	while(*Buffer == TCG_TOKEN_EMPTY) {
		Buffer++;
	}
	return Buffer;
}


/*
 * Get the size of the current token, including the token itself.
 */
static int GetTokenSize(LPBYTE Buffer)
{
	BYTE		Token;

	/* Get the token. */
	Token = *Buffer;

	/* Check for a short atom. */
	if((Token >= 0x80) && (Token <= 0xBF)) {
		return 1 + (Token & 0x0f);
	}

	/* Check for medium atoms. */
	if((Token >= 0xC0) && (Token <= 0xDF)) {
		return 2 + ((((int)(Token & 0x07)) << 8) | Buffer[1]);
	}

	/* Check for long atoms. */
	if((Token >= 0xE0) && (Token <= 0xE3)) {
		return (BE_TO_LE_32(*(DWORD *)Buffer) & 0xffffff) + 4;
	}

	/* The remainder are all one byte tokens. */
	return 1;
}


/*
 * Given a buffer, retrieve a pointer to the first token, skipping empty ones.
 */
static LPBYTE GetFirstToken(LPBYTE Buffer)
{
	/* Really just skipping the empty tokens. */
	return SkipEmptyTokens(Buffer);
}


/*
 * Skip the token pointer and any empty tokens, and point to the next token.
 */
static LPBYTE GetNextToken(LPBYTE Buffer)
{
	/* Skip this token. */
	Buffer += GetTokenSize(Buffer);

	/* Skip any empty tokens as well. */
	return SkipEmptyTokens(Buffer);
}


/*
 * Parse a response packet, breaking out all the tokens.
 */
#include <iostream>
//#define DUMP_TOKEN_STREAM
LPTOKENS ParseResponse(LPTCGFULLPACKET ComPacket)
{
	LPTOKENS	Tokens;
	LPBYTE		Payload;
	int			Count;
	int			Size;
	int			i;

	/* Get the size of the response. */
	Size = BE_TO_LE_32(ComPacket->SubPacket.Length);
#ifdef DUMP_TOKEN_STREAM
	std::cout << "ParseResponse: Size = " << Size << "\n";
#endif

	/* Remove trailing zeros.  */
	Payload = ComPacket->Payload;
	while((Size > 0) && (Payload[Size-1] == 0)) {
		Size--;
	}

	/* Count the number of non-empty tokens. */
	Count = 0;
	i = 0;
	while(i < Size) {
		/* Skip empty tokens. */
		while((i<Size) && (*Payload == TCG_TOKEN_EMPTY)) {
			Payload++;
			i++;
		}

		/* Count the next token. */
		if(i < Size) {
			Count++;
			i += GetTokenSize(Payload);
			Payload += GetTokenSize(Payload);
		}
	}

	/* Allocate space for the processed tokens and set ALL contents to 0/NULL. */
	Tokens = (LPTOKENS)MemCalloc(sizeof(TOKENS) + Count*sizeof(TOKEN));
	if(Tokens == NULL) {
		return NULL;
	}

	/* Process the tokens. */
	Payload = GetFirstToken(ComPacket->Payload);
	Tokens->Count = Count;
	for(i=0; i<Count; i++) {
		/* Set the token type.  0x00 = integer, 0xA0 = bytes, 0xE4 or greater is the control token itself. */
#ifdef DUMP_TOKEN_STREAM
		std::cout << "ParseResponse: token type= " << (void*)*Payload << ",addr = " << (void*)Payload << "\n";
#endif

		if((*Payload >= 0xA0) && (*Payload <= 0xBF)) {
			Tokens->Tokens[i].Type = TCG_TOKEN_TYPE_BYTES;
			Tokens->Tokens[i].Bytes = &Payload[1];
			Tokens->Tokens[i].Integer = *Payload & 0x0f;
		} else if((*Payload >= 0xD0) && (*Payload <= 0xDF)) {
			Tokens->Tokens[i].Type = TCG_TOKEN_TYPE_BYTES;
			Tokens->Tokens[i].Bytes = &Payload[2];
			Tokens->Tokens[i].Integer = (((int)(*Payload & 0x03)) << 8) | Payload[1];
		} else if((*Payload >= 0xE2) && (*Payload <= 0xE3)) {
			Tokens->Tokens[i].Type = TCG_TOKEN_TYPE_BYTES;
			Tokens->Tokens[i].Bytes = &Payload[4];
			Tokens->Tokens[i].Integer = BE_TO_LE_32(*(DWORD *)Payload) & 0xffffff;
		} else if(*Payload >= 0xE4) {
			Tokens->Tokens[i].Type = *Payload;
		} else {
			Tokens->Tokens[i].Type = TCG_TOKEN_TYPE_INT;
                        Tokens->Tokens[i].longInt = 0;

			if(*Payload < 0x40) {
				Tokens->Tokens[i].Integer = *Payload;
                        } else if((*Payload > 0x80) && (*Payload <= 0x88)) {
#ifdef DUMP_TOKEN_STREAM
                		std::cout << "ParseResponse: int payload of = " << (void*)*Payload << "\n";
#endif
				switch(*Payload) {
					case 0x81:
						Tokens->Tokens[i].Integer = Payload[1];
						break;
					case 0x82:
						Tokens->Tokens[i].Integer = ((unsigned int)Payload[1] << 8) | Payload[2];
						break;
					case 0x83:
						Tokens->Tokens[i].Integer = ((unsigned int)Payload[1] << 16) | ((unsigned int)Payload[2] << 8) | Payload[3];
						break;
					case 0x84:
						Tokens->Tokens[i].Integer = ((unsigned int)Payload[1] << 24) | ((unsigned int)Payload[2] << 16) | ((unsigned int)Payload[3] << 8) | Payload[4];
						break;
                                        case 0x85:
                                                Tokens->Tokens[i].Integer = ((unsigned int)Payload[5] << 24) | ((unsigned int)Payload[6] << 16) | ((unsigned int)Payload[7] << 8) | Payload[8];
                                                Tokens->Tokens[i].Integer64 = (unsigned long long)Payload[1]<<32 | ((unsigned long long)Tokens->Tokens[i].Integer);
                                                Tokens->Tokens[i].longInt=1;
                                                break;
                                        case 0x86:
                                                Tokens->Tokens[i].Integer = ((unsigned int)Payload[5] << 24) | ((unsigned int)Payload[6] << 16) | ((unsigned int)Payload[7] << 8) | Payload[8];
                                                Tokens->Tokens[i].Integer64 = ((unsigned long long)Payload[1] << 40) | ((unsigned long long)Payload[2]<<32) |
                                                                                 ((unsigned long long)Tokens->Tokens[i].Integer);
                                                Tokens->Tokens[i].longInt=1;
                                                break;
                                        case 0x87:
                                                Tokens->Tokens[i].Integer = ((unsigned int)Payload[5] << 24) | ((unsigned int)Payload[6] << 16) | ((unsigned int)Payload[7] << 8) | Payload[8];
                                                Tokens->Tokens[i].Integer64 = ((unsigned long long)Payload[1] << 48) | ((unsigned long long)Payload[2] << 40) |
										 ((unsigned long long)Payload[3]<<32) | ((unsigned long long)Tokens->Tokens[i].Integer);
                                                Tokens->Tokens[i].longInt=1;
                                                break;
                                        case 0x88:
                                                Tokens->Tokens[i].Integer = ((unsigned int)Payload[5] << 24) | ((unsigned int)Payload[6] << 16) | ((unsigned int)Payload[7] << 8) | Payload[8];
                                                Tokens->Tokens[i].Integer64 = ((unsigned long long)Payload[1] << 56) | ((unsigned long long)Payload[2] << 48) |
										 ((unsigned long long)Payload[3] << 40) | ((unsigned long long)Payload[4]<<32) |
										    ((unsigned long long)Tokens->Tokens[i].Integer);
                                                Tokens->Tokens[i].longInt=1;
                                                break;
				}
#ifdef DUMP_TOKEN_STREAM
		                std::cout << "ParseResponse: int content: i=" << i << ",Integer = " << Tokens->Tokens[i].Integer 
					  << ",Integer64 = " << Tokens->Tokens[i].Integer64
					  << ", longInt = " << Tokens->Tokens[i].longInt << "\n";
#endif
                                if (Tokens->Tokens[i].longInt && (Tokens->Tokens[i].Integer64 & 0xffffffff00000000))
                                        std::cerr << "WARNING: potential lost information on returned 64 bit integer value: "
						  << Tokens->Tokens[i].Integer64 << " .vs. (32 bit) " 
						  << (unsigned)Tokens->Tokens[i].Integer << "\n";

			} else if((*Payload & 0xC0) == 0x40) {
				Tokens->Tokens[i].Integer = *Payload & 0x3f;
				if(Tokens->Tokens[i].Integer & 0x20) {
					Tokens->Tokens[i].Integer |= 0xffffffc0;
				}
			} else if((*Payload > 0x90) && (*Payload <= 0x94)) {
				switch(*Payload) {
					case 0x91:
						Tokens->Tokens[i].Integer = Payload[1];
						if(Tokens->Tokens[i].Integer & 0x80) {
							Tokens->Tokens[i].Integer |= 0xffffff00;
						}
						break;
					case 0x92:
						Tokens->Tokens[i].Integer = ((unsigned int)Payload[1] << 8) | Payload[2];
						if(Tokens->Tokens[i].Integer & 0x8000) {
							Tokens->Tokens[i].Integer |= 0xffff0000;
						}
						break;
					case 0x93:
						Tokens->Tokens[i].Integer = ((unsigned int)Payload[1] << 16) | ((unsigned int)Payload[2] << 8) | Payload[3];
						if(Tokens->Tokens[i].Integer & 0x800000) {
							Tokens->Tokens[i].Integer |= 0xff000000;
						}
						break;
					case 0x94:
						Tokens->Tokens[i].Integer = ((unsigned int)Payload[1] << 24) | ((unsigned int)Payload[2] << 16) | ((unsigned int)Payload[3] << 8) | Payload[4];
						break;
				}
			}
		}

		/* Process the next token. */
		Payload = GetNextToken(Payload);
	}

	/* Return the tokens. */
	return Tokens;
}


/*
 * Parse a response packet, checking that the session was closed.
 */
BOOL CheckResponseForClose(LPTCGFULLPACKET ComPacket)
{
	LPBYTE	Payload;

	/* Get the first non-empty token. */
	Payload = GetFirstToken(ComPacket->Payload);

	/* Check that it's the close session token. */
	return (*Payload == TCG_TOKEN_ENDSESSION);
}


/*
 * Parse a response packet for the result code.
 */
BYTE GetResponseResultCode(LPTCGFULLPACKET ComPacket)
{
	LPBYTE	Payload;
	LPBYTE	EndPtr;
	DWORD	Size;

	/* Get the size of the response. */
	Size = BE_TO_LE_32(ComPacket->SubPacket.Length);

	/* Get a pointer to the end of the response. */
	EndPtr = &(ComPacket->Payload[Size]);

	/* Get the first non-empty token. */
	Payload = GetFirstToken(ComPacket->Payload);

	/* Continue through the packet until we hit the end-of-data token. */
	while((Payload < EndPtr) && (*Payload != TCG_TOKEN_ENDDATA)) {
		Payload = GetNextToken(Payload);
	}

	/* If we never found the end data token, there was a problem. */
	if(Payload >= EndPtr) {
		return 0x3f;
	}

	/* Verify the next token. */
	Payload = GetNextToken(Payload);
	if(*Payload != TCG_TOKEN_STARTLIST) {
		return 0x3f;
	}

	/* Return the next token. */
	Payload = GetNextToken(Payload);
	return *Payload;
}


/*
 * Determine whether a token is a byte string token. If it is a byte
 * token, returns the size of the token in bytes.
 */
static int IsByteToken(BYTE Token)
{
	if((Token >= 0xA0) && (Token <= 0xBF)) {
		return 1;
	} else if((Token >= 0xD0) && (Token <= 0xDF)) {
		return 2;
	} else if((Token >= 0xE2) && (Token <= 0xE3)) {
		return 4;
	} else {
		return 0;
	}
}


/*
 * Determine whether a token is an integer token.
 */
static int IsIntToken(BYTE Token)
{
	if((IsByteToken(Token) != 0) || (Token >= 0xE4)) {
		return 0;
	}
	return 1;
}


/*
 * For a response that contains a byte sequence (Uid, byte pTable bytes), return
 * a pointer to those bytes.  This saves us from parsing and allocating memory
 * when we only require a single token.
 */
LPBYTE GetResponseBytes(LPTCGFULLPACKET ComPacket, LPDWORD Size)
{
	LPBYTE	Payload;
	LPBYTE	EndPtr;
	DWORD	ResponseSize;

	/* Set the size in case the bytes are not found. */
	if(Size != NULL) {
		*Size = 0;
	}

	/* Get the size of the response. */
	ResponseSize = BE_TO_LE_32(ComPacket->SubPacket.Length);


	/* Get a pointer to the end of the response. */
	EndPtr = &(ComPacket->Payload[ResponseSize]);

#ifdef DEBUG_RESPONSE
	std::cout << "Raw response = : ";
	for (LPBYTE c=ComPacket->Payload;c!=EndPtr;c++) {
		if (((EndPtr-c)%40)==0) std::cout << "\n";
		printf("%02x ",*c);
	}
	std::cout << "\n";
#endif

	/* Get the first non-empty token. */
	Payload = GetFirstToken(ComPacket->Payload);

	/* Scan through all tokens until we find a byte string. */
	while((Payload < EndPtr) && (IsByteToken(*Payload) == 0)) {
		Payload = GetNextToken(Payload);
	}

	/* If we're at the end, there were no bytes. */
	if(Payload >= EndPtr) {
		return NULL;
	}

	/* Determine the size of the bytes. */
	if(Size != NULL) {
		*Size = GetTokenSize(Payload) - IsByteToken(*Payload);
	}

	/* Return a pointer to the bytes. */
	return Payload + IsByteToken(*Payload);
}


/*
 * For a response that contains an integer, return that integer.
 * This saves us from parsing and allocating memory when we only
 * require a single integer.
 */
BOOL GetResponseInt(LPTCGFULLPACKET ComPacket, LPQWORD Value)
{
	LPBYTE	Payload;
	LPBYTE	EndPtr;
	DWORD	ResponseSize;

	/* Get the size of the response. */
	ResponseSize = BE_TO_LE_32(ComPacket->SubPacket.Length);

	/* Get a pointer to the end of the response. */
	EndPtr = &(ComPacket->Payload[ResponseSize]);

	/* Get the first non-empty token. */
	Payload = GetFirstToken(ComPacket->Payload);

	/* Scan through all tokens until we find an integer. */
	while((Payload < EndPtr) && (IsIntToken(*Payload) == 0)) {
		Payload = GetNextToken(Payload);
	}

	/* If we're at the end, there were no bytes. */
	if(Payload >= EndPtr) {
		return FALSE;
	}

	/* Set the integer. */
	*Value = 0;

	/* Return success. */
	return TRUE;
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
