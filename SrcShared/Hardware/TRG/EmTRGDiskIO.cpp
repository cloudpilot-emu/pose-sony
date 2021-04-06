/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2000-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmTRGDiskIO.h"


//-------------------------------------------------------------------------
//  This file implements card I/O emulation by performing read and write
//  operations on a file on the PC.  Note that this file doesn't know 
//  anything about ATA registers ... it's more of a state machine for
//  a continuous series of reads and writes
//
//  These functions will create a disk file on the PC ... if one doesn't
//  exist, it will create it in a formatted state.
//
//  The code should work even if the disk file is replaced by an image
//  from a real card ... there is no additional info stored with the card
//  however, the tuple info won't agree with the card
//---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::EmDiskIO
// ---------------------------------------------------------------------------
EmDiskIO::EmDiskIO (void)
{
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::~EmDiskIO
// ---------------------------------------------------------------------------
EmDiskIO::~EmDiskIO (void)
{
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::~EmDiskIO
// ---------------------------------------------------------------------------
void EmDiskIO::Reset(void)
{
	State.NumSectorsRequested = 0;
	State.NumSectorsCompleted = 0;
	State.SectorIndex         = 0;
	State.Status              = DIO_SUCCESS;
	State.Error               = DIO_ERR_NONE;
	myFile = NULL;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::Initialize
// ---------------------------------------------------------------------------
void EmDiskIO::Initialize (EmDiskTypeID ID)
{
	DiskTypeID     = ID;
	Reset();
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::CloseFile
// ---------------------------------------------------------------------------
void EmDiskIO::CloseFile(void)
{
	if (myFile != NULL)
	{
		fclose(myFile);
		myFile = NULL;
	}
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::Dispose
// ---------------------------------------------------------------------------
void EmDiskIO::Dispose (void)
{
	CloseFile();
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::GetFileName
// ---------------------------------------------------------------------------
char * EmDiskIO::GetFileName(void)
{
	#ifdef PLATFORM_WINDOWS
	static char tmp[MAX_PATH];

	_snprintf(tmp, sizeof(tmp), "%s\\%s", getenv("WINDIR"), DISKFILE_NAME);
	return(tmp);
	#else
	return DISKFILE_NAME;
	#endif
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::TryToFormat
// ---------------------------------------------------------------------------
DiskIOStatus EmDiskIO::TryToFormat(void)
{
	uint32           num;
	LogicalBlockAddr lba;

	num = CurrDisk.GetNumSectors(DiskTypeID);
	for (lba=0; lba<num; lba++)
	{
		CurrDisk.GetSector(DiskTypeID,
	                   	lba,
	                   	&State.Sector);		                   

		// The most probable error condition is
		// attempting to write to a full drive ... it could also
		// be write-protected, or on a disconnected network drive.
		if (fwrite((const char *)State.Sector.Bytes,
                     	            SECTOR_SIZE, 1, myFile) == 0)
			return DIO_ERROR;
	}
	return DIO_SUCCESS;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::Format
// ---------------------------------------------------------------------------
DiskIOStatus EmDiskIO::Format(void)
{
	DiskIOStatus     retVal;	

	CloseFile();
	retVal = DIO_ERROR;
   	myFile = fopen(GetFileName(), "wb");
	if (myFile != NULL)
	{
		retVal = TryToFormat();
		CloseFile();
	}
	return retVal;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::TryToRead
// ---------------------------------------------------------------------------
DiskIOStatus EmDiskIO::TryToRead(void)
{
	uint32       offset;
	DiskIOStatus retVal;

	offset = State.Lba * SECTOR_SIZE;
	retVal = DIO_ERROR;
	myFile = fopen(GetFileName(), "rb");
	if (myFile != NULL)
	{	
		// the error condition here would be the file doesn't exist,
		// which will happen when the program is first run, or
		// something accidentally deletes our data file from the disk
		//
		// there is an extremely unlikely possibility that another file exists
		// with our name 
		if (fseek(myFile, offset, SEEK_SET) == 0)
		{
			if (fread((char *)State.Sector.Bytes, SECTOR_SIZE, 1, myFile) != 0)
				retVal = DIO_SUCCESS;
		}
		CloseFile();
	}
	return retVal;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::ReadSector
// ---------------------------------------------------------------------------
DiskIOStatus EmDiskIO::ReadSector(void)
{
	DiskIOStatus retVal;

	retVal = DIO_SUCCESS;
	if (TryToRead() == DIO_ERROR)
	{
		// If we can't read ... reformat the drive and try again
		if ((retVal=Format()) != DIO_ERROR)
			retVal = TryToRead();
	}
	return retVal;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::TryToWrite
// ---------------------------------------------------------------------------
DiskIOStatus EmDiskIO::TryToWrite(void)
{
	uint32       offset;
	DiskIOStatus retVal;

	offset = State.Lba * SECTOR_SIZE;
	retVal = DIO_ERROR;

	// the error conditions here would indicate the drive is full
	// or write-protected
	myFile = fopen(GetFileName(), "r+b");
	if (myFile != NULL)
	{	
		if (fseek(myFile, offset, SEEK_SET) == 0)
		{
			if (fwrite((char *)State.Sector.Bytes, SECTOR_SIZE, 1, myFile) != 0)
				retVal = DIO_SUCCESS;
		}
		CloseFile();
	}
	return retVal;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::WriteSector
// ---------------------------------------------------------------------------
DiskIOStatus EmDiskIO::WriteSector(void)
{
	DiskIOStatus retVal;

	retVal = DIO_SUCCESS;
	if (TryToWrite() == DIO_ERROR)
	{
		if ((retVal=Format()) != DIO_ERROR)
			retVal = TryToWrite();
	}
	return(retVal);
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::ReadNextDataByte
// ---------------------------------------------------------------------------
void EmDiskIO::ReadNextDataByte(uint8 * val)
{
	// this first statement will likely not be called except when someone's
	// dumping memory at our address range
	if (State.NumSectorsCompleted >= State.NumSectorsRequested)
		*val = 0;
	else
	{
		*val = State.Sector.Bytes[State.SectorIndex++];
		if (State.SectorIndex >= SECTOR_SIZE)
		{
			if (++State.NumSectorsCompleted < State.NumSectorsRequested)
			{
				State.Lba++;
				State.SectorIndex = 0;
				State.Status = ReadSector();
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::WriteNextDataByte
// ---------------------------------------------------------------------------
void EmDiskIO::WriteNextDataByte(uint8 val)
{
	DiskIOStatus status;

	status = DIO_SUCCESS;
	if (State.NumSectorsCompleted < State.NumSectorsRequested)
	{
		State.Sector.Bytes[State.SectorIndex++] = val;
		if (State.SectorIndex >= SECTOR_SIZE)
		{
			if (WriteSector() == DIO_ERROR)
				status = DIO_ERROR;
			else if (++State.NumSectorsCompleted < State.NumSectorsRequested)
			{
				status = DIO_SUCCESS;
				State.Lba++;
				State.SectorIndex = 0;
			}
		}
	}
	State.Status = status;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::GetSectorCount
// ---------------------------------------------------------------------------
uint32 EmDiskIO::GetSectorCount(void)
{
	return State.NumSectorsCompleted;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::StartDriveID
// ---------------------------------------------------------------------------
void EmDiskIO::StartDriveID(void)
{
	CurrDisk.GetDriveID(DiskTypeID,
                            &State.Sector);
	State.NumSectorsRequested = 1;
	State.NumSectorsCompleted = 0;
	State.SectorIndex    = 0;
	State.Status         = DIO_SUCCESS;
	State.Error  = DIO_ERR_NONE;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::StartRead
// ---------------------------------------------------------------------------
void EmDiskIO::StartRead(DiskIOParams * params)
{
	State.Lba = params->Lba;
	State.NumSectorsRequested = params->SectorCnt;
	State.NumSectorsCompleted = 0;
	State.SectorIndex    = 0;
	State.Error  = DIO_ERR_NONE;
	State.Status = ReadSector();
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::StartWrite
// ---------------------------------------------------------------------------
void EmDiskIO::StartWrite(DiskIOParams * params)
{
	State.Lba = params->Lba;
	State.NumSectorsRequested = params->SectorCnt;
	State.NumSectorsCompleted = 0;
	State.SectorIndex    = 0;
	State.Status = DIO_SUCCESS;
	State.Error  = DIO_ERR_NONE;
}

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::GetStatus
// ---------------------------------------------------------------------------
void EmDiskIO::GetStatus(DiskIOStatus *   status,
                         DiskDataStatus * dataStatus)
{
	*status = State.Status;
	if (State.NumSectorsCompleted == State.NumSectorsRequested)
		*dataStatus = DIO_DATA_COMPLETE;
	else
		*dataStatus = DIO_MORE_DATA;
}	

// ---------------------------------------------------------------------------
//		¥ EmDiskIO::GetError
// ---------------------------------------------------------------------------
DiskIOError EmDiskIO::GetError(void)
{
	if (State.Status == DIO_SUCCESS)
		return(DIO_ERR_NONE);
	else
	// at this point, all errors are pretty much the same ... we can't
	// create our emulation file ... we probably will need to include
	// some more refined error codes at some point
		return(DIO_ERR_GENERIC);
}
