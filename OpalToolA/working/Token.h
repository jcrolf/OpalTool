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

#ifndef TOKEN_H_
#define TOKEN_H_

/* Tokens used in subpackets. */
#define	TCG_TOKEN_TINYATOM			0x00
#define	TCG_TOKEN_TINYSIGNEDATOM	0x40
#define	TCG_TOKEN_SHORTATOM			0x80
#define	TCG_TOKEN_SHORTSIGNEDATOM	0x90
#define	TCG_TOKEN_SHORTBYTESATOM	0xa0
#define	TCG_TOKEN_MEDIUMATOM		0xc0
#define	TCG_TOKEN_MEDIUMSIGNEDATOM	0xc8
#define	TCG_TOKEN_MEDIUMBYTESATOM	0xd0
#define	TCG_TOKEN_LONGATOM			0xe0
#define	TCG_TOKEN_LONGSIGNEDATOM	0xe1
#define	TCG_TOKEN_LONGBYTESATOM		0xe2
#define	TCG_TOKEN_STARTLIST			0xf0
#define	TCG_TOKEN_ENDLIST			0xf1
#define	TCG_TOKEN_STARTNAME			0xf2
#define	TCG_TOKEN_ENDNAME			0xf3
#define	TCG_TOKEN_CALL				0xf8
#define	TCG_TOKEN_ENDDATA			0xf9
#define	TCG_TOKEN_ENDSESSION		0xfa
#define	TCG_TOKEN_STARTTRANSACTION	0xfb
#define	TCG_TOKEN_ENDTRANSACTION	0xfc
#define	TCG_TOKEN_EMPTY				0xff
/* My own internal token types. */
#define	TCG_TOKEN_TYPE_INT			0x00
#define TCG_TOKEN_TYPE_BYTES		0xa0


/* Internal structure used for parsing tokens. */
typedef struct tagInternalToken {
        // all current uses of this 'type' are actually more or less old school initialized to these values,
        // so this constructor would never get used even if it was unfurled ...
        tagInternalToken(): Type(TCG_TOKEN_TYPE_INT),Integer(0),Integer64(0),Bytes(NULL),longInt(0) {}

	BYTE		Type;			/* Token type, integer, bytes or control. */
	int		Integer;		/* If an integer, this holds the token value. If bytes, the number of bytes. */
        long long	Integer64;		/* If the data comes back 64 bit, here is the value ... */
        LPBYTE          Bytes;                  /* If bytes, this points to them. */
        BOOL            longInt;		/* If the data was 64 bits coming back (future use) */
} TOKEN, *LPTOKEN;


/* Internal structure used for parsing tokens. */
typedef struct tagTokens {
	int		Count;			/* Count of all tokens. */
	TOKEN		Tokens[1];		/* List of tokens. */
} TOKENS, *LPTOKENS;


/* Function prototypes. */
void AddSimpleToken(LPTCGFULLPACKET ComPacket, BYTE Token);
void AddTokenBytes(LPTCGFULLPACKET ComPacket, LPBYTE Bytes, int Size);
void AddTokenSignedInt(LPTCGFULLPACKET ComPacket, int Integer);
void AddTokenUnsignedInt(LPTCGFULLPACKET ComPacket, QWORD Integer);
void AddTokenEndParameters(LPTCGFULLPACKET ComPacket, BYTE Status);
void AddTokenCall(LPTCGFULLPACKET ComPacket, LPBYTE InvokingId, LPBYTE MethodId);
void AddTokenCellBlock(LPTCGFULLPACKET ComPacket, LPBYTE pTable, LPBYTE StartRowUid, int StartRowInt, int EndRow, int StartColumn, int EndColumn);
LPTOKENS ParseResponse(LPTCGFULLPACKET ComPacket);
BOOL CheckResponseForClose(LPTCGFULLPACKET ComPacket);
BYTE GetResponseResultCode(LPTCGFULLPACKET ComPacket);
LPBYTE GetResponseBytes(LPTCGFULLPACKET ComPacket, LPDWORD Size);


#endif /* TOKEN_H_ */

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
