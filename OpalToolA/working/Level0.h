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

#ifndef LEVEL0_H_
#define LEVEL0_H_

/* The feature descriptor header. */
typedef struct tagFeatureDescriptor {
	WORD		FeatureCode;
	BYTE		Version;
	BYTE		Length;
} FEATURE, *LPFEATURE;


/* The TPer Feature Descriptor. */
typedef struct tagTperFeature {
	FEATURE		Feature;
	BYTE		Features;
	BYTE		Padding[11];
} TPERFEATURE, *LPTPERFEATURE;


/* The Locking Feature Descriptor. */
typedef struct tagLockingFeature {
	FEATURE		Feature;
	BYTE		Features;
	BYTE		Padding[11];
} LOCKINGFEATURE, *LPLOCKINGFEATURE;


/* The Geometry Reporting Feature Descriptor. */
typedef struct tagGeometryReportingFeature {
      FEATURE         Feature;
      BYTE            Align;
      BYTE            Padding[7];
      DWORD           LogicalBlockSize;
      QWORD           AlignmentGranularity;
      QWORD           LowestAlignedLBA;
} GEOMETRYREPORTINGFEATURE, *LPGEOMETRYREPORTINGFEATURE;


/* The common header for an SSC. */
typedef struct tagOPALSSC {
	FEATURE		Feature;
	WORD		BaseComId;
	WORD		NumberComIds;
	BYTE		RangeCrossing;
} OPALSSC, *LPOPALSSC;


typedef struct tagOPAL2SSC {
      OPALSSC         SSCFeatureHeader;
      WORD            NumberOfLockingSPAdminAuthoritiesSupported;
      WORD            NumberOfLockingSPUserAuthoritiesSupported;
      BYTE            InitialC_PIN_SIDPINIndicator;
      BYTE            BehaviorOfC_PIN_SIDPINUponTPerRevert;
      BYTE            Reserved[5];
} OPAL2SSC, *LPOPAL2SSC;


void PrintFeatureHeader(LPFEATURE Feature, LPTSTR Header, LPTSTR Buffer);
void PrintBoolean(LPTSTR Buffer, LPTSTR String, BOOL Value, LPTSTR TrueString, LPTSTR FalseString);
void PrintTperFeatures(LPTPERFEATURE Feature, LPTSTR Buffer);
void PrintLockingFeatures(LPLOCKINGFEATURE Feature, LPTSTR Buffer);
void PrintOpalFeatures(LPOPALSSC Feature, LPTSTR Buffer);
void PrintFeatureInformation(LPFEATURE Feature, LPTSTR Buffer);

void Level0Info(HWND hWnd, LPBYTE Buffer);


#endif /* LEVEL0_H_ */

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
