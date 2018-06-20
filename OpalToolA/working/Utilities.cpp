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

#include "stdafx.h"
#include "string"
#include "Utilities.h"
/*
 * Verify whether a character is a hex character.
 */
BYTE ConvertHex(TCHAR Char)
{
	if((Char >= _T('0')) && (Char <= _T('9'))) return Char - _T('0');
	if((Char >= _T('a')) && (Char <= _T('f'))) return Char - _T('a') + 0x0a;
	if((Char >= _T('A')) && (Char <= _T('F'))) return Char - _T('A') + 0x0a;

	return 0xff;
}

std::string ErrorCodeString(DWORD TperStatus)
{
        std::string ErrorString;

        switch (TperStatus) {
                case TPER_SUCCESS: {
                        ErrorString = "SUCCESS";
                        break;
                }
                case TPER_NOT_AUTHORIZED: {
                        ErrorString = "NOT_AUTHORIZED";
                        break;
                }
                case 0x2:
                case 0xD:
                case 0xE:
                {
                        ErrorString = "OBSOLETE";
                        break;
                }
                case TPER_SP_BUSY: {
                        ErrorString = "SP_BUSY";
                        break;
                }
                case TPER_SP_FAILED: {
                        ErrorString = "SP_FAILED";
                        break;
                }
                case TPER_SP_DISABLED: {
                        ErrorString = "SP_DISABLED";
                        break;
                }
                case TPER_SP_FROZEN: {
                        ErrorString = "SP_FROZEN";
                        break;
                }
                case TPER_NO_SESSIONS_AVAILABLE: {
                        ErrorString = "NO_SESSIONS_AVAILABLE";
                        break;
                }
                case TPER_UNIQUENESS_CONFLICT: {
                        ErrorString = "UNIQUENESS_CONFLICT";
                        break;
                }
                case TPER_INSUFFICIENT_SPACE: {
                        ErrorString = "INSUFFICIENT_SPACE";
                        break;
                }
                case TPER_INSUFFICIENT_ROWS: {
                        ErrorString = "INSUFFICIENT_ROWS";
                        break;
                }
                case TPER_INVALID_METHOD: {
                        ErrorString = "INVALID_METHOD";
                        break;
                }
                case TPER_INVALID_PARAMETER: {
                        ErrorString = "INVALID_PARAMETER";
                        break;
                }
                case TPER_MALFUNCTION: {
                        ErrorString = "MALFUNCTION";
                        break;
                }
                case TPER_TRANSACTION_FAILURE: {
                        ErrorString = "TRANSACTION_FAILURE";
                        break;
                }
                case TPER_RESPONSE_OVERFLOW: {
                        ErrorString = "RESPONSE_OVERFLOW";
                        break;
                }
                case TPER_AUTHORITY_LOCKED_OUT: {
                        ErrorString = "AUTHORITY_LOCKED_OUT";
                        break;
                }
                case TPER_FAIL: {
                        ErrorString = "TPER_FAIL";
                        break;
                }
                default: {
                        ErrorString = "UNKNOWN TPER ERROR STATUS";
                        break;
                }
        }

        return ErrorString;
}
