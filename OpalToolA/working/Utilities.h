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

#ifndef UTILIITIES_H_
#define UTILITIES_H_

#include <string>

#define TPER_SUCCESS 0x00
#define TPER_NOT_AUTHORIZED 0x01
//OBSOLETE 0x02
#define TPER_SP_BUSY 0x03
#define TPER_SP_FAILED 0x04
#define TPER_SP_DISABLED 0x05
#define TPER_SP_FROZEN 0x06
#define TPER_NO_SESSIONS_AVAILABLE 0x07
#define TPER_UNIQUENESS_CONFLICT 0x08
#define TPER_INSUFFICIENT_SPACE 0x09
#define TPER_INSUFFICIENT_ROWS 0x0A
#define TPER_INVALID_METHOD 0x0B
#define TPER_INVALID_PARAMETER 0x0C
//OBSOLETE 0x0D
//OBSOLETE 0x0E
#define TPER_MALFUNCTION 0x0F
#define TPER_TRANSACTION_FAILURE 0x10
#define TPER_RESPONSE_OVERFLOW 0x11
#define TPER_AUTHORITY_LOCKED_OUT 0x12
#define TPER_FAIL 0x3F

BYTE ConvertHex(TCHAR Char);
std::string ErrorCodeString(DWORD TperStatus);


#endif /* UTIILITIES _H_ */

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
