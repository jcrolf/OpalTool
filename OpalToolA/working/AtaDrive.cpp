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
#include"AtaDrive.h"

//#define DEBUG_ATA_COMMUNICATIONS

/* The task file for passing parameters to the ATA command. */
typedef struct tagTaskFile {
	WORD		Feature;		/* Feature register contents. */
	WORD		Sectors;		/* Number of sectors to transfer. */
	QWORD		Lba;			/* Starting LBA of transfer. */
	BYTE		Device;			/* Device: Master or Slave. */
	BYTE		Cmd;			/* ATA Command. */
} TASKFILE, *LPTASKFILE;


/* Get a specific byte from a larger (e.g dword) value. */
#define GETBYTE(x, BitShift)		((BYTE)(((x) >> (BitShift)) & 0xff))

/* Sector buffers must be page-aligned. */
#define SectorAlloc(Size)			VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_READWRITE)
#define SectorFree(Buffer)			VirtualFree(Buffer, 0, MEM_RELEASE)


/*
 * Initialize the SCSI pass through direct structure.
 */
static void InitScsiPassThroughDirect(PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptd, LPBYTE Cdb, BYTE CdbLength, BYTE DataFlag, LPBYTE TransferBuffer, DWORD TransferLength)
{
	sptd->sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptd->sptd.ScsiStatus = 0;
	sptd->sptd.PathId = 0;
	sptd->sptd.TargetId = 0;
	sptd->sptd.Lun = 0;
	sptd->sptd.SenseInfoLength = SENSE_BUFFER_LENGTH;
	sptd->sptd.TimeOutValue = 20;
	sptd->sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuffer);

	sptd->sptd.CdbLength = CdbLength;
	sptd->sptd.DataIn = DataFlag;
	sptd->sptd.DataTransferLength = TransferLength;
	sptd->sptd.DataBuffer = TransferBuffer;

	memcpy(&(sptd->sptd.Cdb), Cdb, CdbLength);
}


/*
 * Send an ATA command to the drive over a USB interface.
 */
static BOOL SendAtaCommandOverUsb(LPATADRIVE pAtaDrive, USHORT Flags, DWORD DataLength, LPVOID Buffer, LPTASKFILE TaskFile)
{
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER	sptd;
	DWORD									nRead;
	BOOL									Result;
	BYTE									Protocol = 0;
	BYTE									DataFlag;
	BYTE									CdbLength;
	BYTE									Cdb[16];

	/* Set up pass through values. */
	memset(Cdb, 0, sizeof(Cdb));
	DataFlag = 0;
	if(Flags & ATA_FLAGS_DATA_IN) {
		DataFlag = SCSI_IOCTL_DATA_IN;
		Protocol = 4;
		Cdb[2] |= 8;
		Cdb[2] |= 6;
	}
	if(Flags & ATA_FLAGS_DATA_OUT) {
		DataFlag = SCSI_IOCTL_DATA_OUT;
		Protocol = 5;
		Cdb[2] |= 6;
	}
	if(Flags & ATA_FLAGS_USE_DMA) {
		Protocol += 6;
	}
	Cdb[1] = Protocol << 1;
	if(Flags & ATA_FLAGS_48BIT_COMMAND) {
		CdbLength = 16;
		Cdb[0] = 0x85;
		Cdb[1] |= 1;								/* The extend bit. */
		Cdb[3] = GETBYTE(TaskFile->Feature, 8);
		Cdb[4] = GETBYTE(TaskFile->Feature, 0);
		Cdb[5] = GETBYTE(TaskFile->Sectors, 8);
		Cdb[6] = GETBYTE(TaskFile->Sectors, 0);
		Cdb[8] = GETBYTE(TaskFile->Lba, 0);
		Cdb[10] = GETBYTE(TaskFile->Lba, 8);
		Cdb[12] = GETBYTE(TaskFile->Lba, 16);
		Cdb[7] = GETBYTE(TaskFile->Lba, 24);
		Cdb[9] = GETBYTE(TaskFile->Lba, 32);
		Cdb[11] = GETBYTE(TaskFile->Lba, 40);
		Cdb[13] = GETBYTE(TaskFile->Device, 0);
		Cdb[14] = GETBYTE(TaskFile->Cmd, 0);
	} else {
		CdbLength = 12;
		Cdb[0] = 0xA1;
		Cdb[3] = GETBYTE(TaskFile->Feature, 0);
		Cdb[4] = GETBYTE(TaskFile->Sectors, 0);
		Cdb[5] = GETBYTE(TaskFile->Lba, 0);
		Cdb[6] = GETBYTE(TaskFile->Lba, 8);
		Cdb[7] = GETBYTE(TaskFile->Lba, 16);
		Cdb[8] = GETBYTE(TaskFile->Lba, 24) & 0x0f;
		Cdb[8] |= GETBYTE(TaskFile->Device, 0) & 0xf0;
		Cdb[9] = GETBYTE(TaskFile->Cmd, 0);
	}
	InitScsiPassThroughDirect(&sptd, Cdb, CdbLength, DataFlag, (LPBYTE)Buffer, DataLength);

	/* Send the command to the drive. */
	nRead = 0;
	Result = DeviceIoControl(pAtaDrive->hDrive, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptd, sizeof(sptd), &sptd, sizeof(sptd), &nRead, NULL);

	/* Check for command failure. */
	if(nRead < sizeof(SCSI_PASS_THROUGH_DIRECT)) {
		Result = FALSE;
	}
	if(sptd.sptd.ScsiStatus != 0) {
		Result = FALSE;
	}

	/* Return the result. */
	return Result;
}


/*
 * Send an ATA command to the drive over an ATA interface.
 */
static BOOL SendAtaCommandOverAta(LPATADRIVE pAtaDrive, USHORT Flags, DWORD DataLength, LPVOID Buffer, LPTASKFILE TaskFile)
{
	ATA_PASS_THROUGH_DIRECT		Ptd;
	DWORD						nRead;
	DWORD						Length = 0;
	BOOL						Result;

	/* Set up pass through values. */
	memset(&Ptd, 0, sizeof(ATA_PASS_THROUGH_DIRECT));
	Ptd.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
	Ptd.TimeOutValue = 60;
	Ptd.AtaFlags = Flags | ATA_FLAGS_DRDY_REQUIRED;
	Ptd.DataTransferLength = DataLength;
	Ptd.DataBuffer = Buffer;

	/* Set up the task files. */
	Ptd.CurrentTaskFile[0] = GETBYTE(TaskFile->Feature, 0);
	Ptd.CurrentTaskFile[1] = GETBYTE(TaskFile->Sectors, 0);
	Ptd.CurrentTaskFile[2] = GETBYTE(TaskFile->Lba, 0);
	Ptd.CurrentTaskFile[3] = GETBYTE(TaskFile->Lba, 8);
	Ptd.CurrentTaskFile[4] = GETBYTE(TaskFile->Lba, 16);
	Ptd.CurrentTaskFile[6] = GETBYTE(TaskFile->Cmd, 0);
	if(Flags & ATA_FLAGS_48BIT_COMMAND) {
		Ptd.CurrentTaskFile[5] = GETBYTE(TaskFile->Device, 0);
		Ptd.PreviousTaskFile[0] = GETBYTE(TaskFile->Feature, 8);
		Ptd.PreviousTaskFile[1] = GETBYTE(TaskFile->Sectors, 8);
		Ptd.PreviousTaskFile[2] = GETBYTE(TaskFile->Lba, 24);
		Ptd.PreviousTaskFile[3] = GETBYTE(TaskFile->Lba, 32);
		Ptd.PreviousTaskFile[4] = GETBYTE(TaskFile->Lba, 40);
	} else {
		Ptd.CurrentTaskFile[5] = (GETBYTE(TaskFile->Device, 0) & 0xf0) | (GETBYTE(TaskFile->Lba, 24) & 0x0f);
	}

	/* Send the command to the drive. */
	Length = Ptd.Length + Ptd.DataTransferLength;
	nRead = 0;
	Result = DeviceIoControl(pAtaDrive->hDrive, IOCTL_ATA_PASS_THROUGH_DIRECT, &Ptd, Length, &Ptd, Length, &nRead, NULL);

	/* Check for command failure. */
	if(nRead < Ptd.Length) {
		Result = FALSE;
	}
	if(Ptd.CurrentTaskFile[6] & 0x01) {
		Result = FALSE;
	}

	/* Return the result. */
	return Result;
}

#ifdef DEBUG_ATA_COMMUNICATIONS
#include <stdio.h>
void dumpPacket(LPBYTE Packet, size_t Length,char *comment)
{
        //THIS IS TESTING CODE FOR NOW ...
        printf("HD::: %s --\n",comment);
        for (size_t i=0;i<Length;i++)
        {
                if (i%16==0) printf("HD::: %04x ",i);
                printf("%02x",Packet[i]);
                if ((i%16==15) || (i==Length-1))  {
                        printf(" ");
                        for (size_t j=i-i%16; j<=i;j++)
                        {
                                if (isprint(Packet[j])) printf("%c",Packet[j]);
                                else                                    printf(".");
                        }
                        printf("\n");
                }
                else if (i%4==3) printf(" ");
        }
        printf("HD:::\n");
}
#endif
/*
 * Send a command to the hard drive.
 */
static BOOL SendAtaCommand(LPATADRIVE pAtaDrive, USHORT Flags, DWORD DataLength, LPVOID Buffer, LPTASKFILE TaskFile)
{
        BOOL result=FALSE;
	if (pAtaDrive != NULL) {
#ifdef DEBUG_ATA_COMMUNICATIONS
                dumpPacket((LPBYTE)Buffer,DataLength,"Send");
#endif
		switch (pAtaDrive->BusType) {
			case BusTypeAta:
			case BusTypeSata:
				result= SendAtaCommandOverAta(pAtaDrive, Flags, DataLength, Buffer, TaskFile);
				break;
			case BusTypeUsb:
				result= SendAtaCommandOverUsb(pAtaDrive, Flags, DataLength, Buffer, TaskFile);
				break;
		}
#ifdef DEBUG_ATA_COMMUNICATIONS
                dumpPacket((LPBYTE)Buffer,DataLength,"Send result (Receive)");
#endif
	}

	return result;
}


/*
 * Close the drive.
 */
void CloseAtaDrive(LPATADRIVE pAtaDrive)
{
	if (pAtaDrive != NULL) {
		if (pAtaDrive->hDrive != INVALID_HANDLE_VALUE) {
			CloseHandle(pAtaDrive->hDrive);
		}
		if (pAtaDrive->Scratch != NULL) {
			SectorFree(pAtaDrive->Scratch);
		}
		MemFree(pAtaDrive);
	}
}


/*
 * Switch every two bytes in a buffer.
 */
static void SwitchBuffer(LPSTR Buffer, int Size)
{
	TCHAR	Temp;
	int		i;

	for(i=0; i<Size; i+=2) {
		Temp = Buffer[i+1];
		Buffer[i+1] = Buffer[i];
		Buffer[i] = Temp;
	}
}


/*
 * Opens a drive and initializes the structure.
 */
LPATADRIVE OpenAtaDrive(LPTSTR Drive)
{
	STORAGE_ADAPTER_DESCRIPTOR	Descriptor;
	STORAGE_PROPERTY_QUERY		Query;
	STORAGE_DEVICE_NUMBER		sdn;
	LPATADRIVE					pAtaDrive;
	TASKFILE					TaskFile;
	DWORD						nReturned;
	BOOL						Result;

	/* Allocate memory. */
	pAtaDrive = (LPATADRIVE)MemCalloc(sizeof(ATADRIVE));
	if (pAtaDrive == NULL) {
		return NULL;
	}
	pAtaDrive->hDrive = INVALID_HANDLE_VALUE;

	/* Allocate scratch sector. */
	pAtaDrive->Scratch = (LPBYTE)SectorAlloc(512);
	if (pAtaDrive->Scratch == NULL) {
		CloseAtaDrive(pAtaDrive);
		return NULL;
	}

	/* Open the drive. */
	pAtaDrive->hDrive = CreateFile(Drive, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (pAtaDrive->hDrive == INVALID_HANDLE_VALUE) {
		CloseAtaDrive(pAtaDrive);
		return NULL;
	}

	/* Determine the drive number. */
	memset(&sdn, 0, sizeof(sdn));
	DeviceIoControl(pAtaDrive->hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &nReturned, NULL);
	pAtaDrive->DriveNumber = sdn.DeviceNumber;

	/* Determine drive interface. */
	memset(&Query, 0, sizeof(Query));
	Query.PropertyId = StorageAdapterProperty;
	Query.QueryType = PropertyStandardQuery;
	memset(&Descriptor, 0, sizeof(Descriptor));
	Descriptor.Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
	Result = DeviceIoControl(pAtaDrive->hDrive, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(STORAGE_PROPERTY_QUERY), &Descriptor, Descriptor.Size, &nReturned, NULL);
	pAtaDrive->BusType = Result ? Descriptor.BusType : BusTypeUnknown;

	/* Identify Device. */
	memset(&TaskFile, 0, sizeof(TaskFile));
	TaskFile.Cmd = 0xEC;
	TaskFile.Sectors = 1; /* Needed for SCSI ATA_PASS_THROUGH */
	Result = SendAtaCommand(pAtaDrive, ATA_FLAGS_DATA_IN, 512, pAtaDrive->Scratch, &TaskFile);
	if(Result == FALSE) {
		CloseAtaDrive(pAtaDrive);
		return NULL;
	}

	/* Get the hard drive model number. */
	memcpy((LPBYTE)(pAtaDrive->Model), &(pAtaDrive->Scratch[54]), 40);
	SwitchBuffer(pAtaDrive->Model, 40);
	pAtaDrive->Model[40] = 0;

	/* Get the hard drive serial number. */
	memcpy((LPBYTE)(pAtaDrive->Serial), &(pAtaDrive->Scratch[20]), 20);
	SwitchBuffer(pAtaDrive->Serial, 20);
	pAtaDrive->Serial[20] = 0;

	/* Return the structure. */
	return pAtaDrive;
}


/*
 * Issue the trusted send command.
 */
BOOL TrustedSend(LPATADRIVE pAtaDrive, BOOL UseDMA, BYTE ProtocolID, WORD ComID, LPBYTE Packet, WORD Length)
{
	TASKFILE	TaskFile;
	USHORT		Flags;

	/* Set the taskfile appropriately. */
	memset(&TaskFile, 0, sizeof(TASKFILE));
	if(UseDMA != FALSE) {
		TaskFile.Cmd = 0x5F;
	} else {
		TaskFile.Cmd = 0x5E;
	}
	TaskFile.Feature = ProtocolID;
	TaskFile.Sectors = Length & 0xff;
	TaskFile.Lba = ((DWORD)ComID << 8) | ((Length >> 8) & 0xff);

	/* Set the flags. */
	Flags = ATA_FLAGS_DATA_OUT;
	if(UseDMA != FALSE) {
		Flags |= ATA_FLAGS_USE_DMA;
	}

	/* Send the command. */
	return SendAtaCommand(pAtaDrive, Flags, Length * 512, Packet, &TaskFile);
}


/*
 * Issue the trusted receive command.
 */
BOOL TrustedReceive(LPATADRIVE pAtaDrive, BOOL UseDMA, BYTE ProtocolID, WORD ComID, LPBYTE Packet, WORD Length)
{
	TASKFILE	TaskFile;
	USHORT		Flags;

	/* Set the taskfile appropriately. */
	memset(&TaskFile, 0, sizeof(TASKFILE));
	if(UseDMA != FALSE) {
		TaskFile.Cmd = 0x5D;
	} else {
		TaskFile.Cmd = 0x5C;
	}
	TaskFile.Feature = ProtocolID;
	TaskFile.Sectors = Length & 0xff;
	TaskFile.Lba = ((DWORD)ComID << 8) | ((Length >> 8) & 0xff);

	/* Set the flags. */
	Flags = ATA_FLAGS_DATA_IN;
	if(UseDMA != FALSE) {
		Flags |= ATA_FLAGS_USE_DMA;
	}

	/* Send the command. */
	return SendAtaCommand(pAtaDrive, Flags, Length * 512, Packet, &TaskFile);
}


/*
 * Converts a bus type into a readable string.
 */
LPTSTR GetDriveBusString(LPATADRIVE pAtaDrive)
{
	switch (pAtaDrive->BusType) {
		case BusTypeScsi:
			return _T("SCSI");
			break;
		case BusTypeAtapi:
			return _T("ATAPI");
			break;
		case BusTypeAta:
			return _T("ATA");
			break;
		case BusType1394:
			return _T("1394");
			break;
		case BusTypeSsa:
			return _T("SSA");
			break;
		case BusTypeFibre:
			return _T("Fibre");
			break;
		case BusTypeUsb:
			return _T("USB");
			break;
		case BusTypeRAID:
			return _T("RAID");
			break;
//		case BusTypeiSCSI:
//			return _T("iSCSI");
//			break;
		case BusTypeSas:
			return _T("SAS");
			break;
		case BusTypeSata:
			return _T("SATA");
			break;
		case BusTypeUnknown:
		default:
			return _T("Unknown");
			break;
	}
}

/**********************************************
 *                                            *
 *                                            *
 **********************************************/
