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

#ifndef STORAGE_H_
#define STORAGE_H_


/* This is required if a Windows DDK is not installed. */
#ifndef IOCTL_STORAGE_QUERY_PROPERTY
#define IOCTL_STORAGE_QUERY_PROPERTY	CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _STORAGE_ADAPTER_DESCRIPTOR {
	DWORD	Version;
	DWORD	Size;
	DWORD	MaximumTransferLength;
	DWORD	MaximumPhysicalPages;
	DWORD	AlignmentMask;
	BOOLEAN	AdapterUsesPio;
	BOOLEAN	AdapterScansDown;
	BOOLEAN	CommandQueueing;
	BOOLEAN	AcceleratedTransfer;
	BYTE	BusType;
	WORD	BusMajorVersion;
	WORD	BusMinorVersion;
} STORAGE_ADAPTER_DESCRIPTOR, *PSTORAGE_ADAPTER_DESCRIPTOR;


typedef enum _STORAGE_PROPERTY_ID {
	StorageDeviceProperty		= 0,
	StorageAdapterProperty,
	StorageDeviceIdProperty,
	StorageDeviceUniqueIdProperty,
	StorageDeviceWriteCacheProperty,
	StorageMiniportProperty,
	StorageAccessAlignmentProperty,
	StorageDeviceSeekPenaltyProperty,
	StorageDeviceTrimProperty,
	StorageWriteAggregationProperty
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;


typedef enum _STORAGE_QUERY_TYPE {
	PropertyStandardQuery	= 0,
	PropertyExistsQuery,
	PropertyMaskQuery,
	PropertyQueryMaxDefined
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;


typedef struct _STORAGE_PROPERTY_QUERY {
	STORAGE_PROPERTY_ID		PropertyId;
	STORAGE_QUERY_TYPE		QueryType;
	UCHAR					AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;


typedef enum _STORAGE_BUS_TYPE {
	BusTypeUnknown				= 0x00,
	BusTypeScsi					= 0x01,
	BusTypeAtapi				= 0x02,
	BusTypeAta					= 0x03,
	BusType1394					= 0x04,
	BusTypeSsa					= 0x05,
	BusTypeFibre				= 0x06,
	BusTypeUsb					= 0x07,
	BusTypeRAID					= 0x08,
	BusTypeiSCSI				= 0x09,
	BusTypeSas					= 0x0a,
	BusTypeSata					= 0x0b,
	BusTypeMaxReserved			= 0x7f
} STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;


#endif /* IOCTL_STORAGE_QUERY_PROPERTY */
#endif /* STORAGE_H_ */

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
