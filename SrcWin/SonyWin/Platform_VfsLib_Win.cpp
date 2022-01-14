/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "Platform_VfsLib.h"

#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"
#include "Platform.h"			// AllocateMemory

#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <tchar.h>

#define MAX_FFS_PATH		256

#define PRINTF	if (1) ; else LogAppendMsg


/************************************************************
 * No			: 3.24
 *  FUNCTION    : VFSImportDatabaseFromFile()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     :
 *************************************************************/
Err Platform_VfsLib::VFSImportDatabaseFromFile(UInt16 nVolRefNum, char*pPathName, UInt16 *pCardNo, LocalID *pDbID)
{
	Err retval = errNone;
	return retval;
}


/************************************************************
 * No			: 3.25
 *  FUNCTION    : VFSExportDatabaseToFile()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     :
 *************************************************************/
/*
Err Platform_VfsLib::VFSExportDatabaseToFile(UInt16 nVolRefNum, char*pPathName, UInt16 nCardNo, LocalID nDbID)
{
	Err retval = errNone;
	DmOpenRef	dbR;

	UInt16		attributes, version;
	UInt32		crDate, modDate, bckUpDate;
	UInt32		type, creator;
	UInt32		modNum;
	LocalID		appInfoID, sortInfoID;
	DatabaseHdrPtr	hdrP;
	char		name[32];

	retval = ::DmDatabaseInfo(nCardNo, nDbID, name,
				&attributes, &version, &crDate,
				&modDate, &bckUpDate,
				&modNum, &appInfoID,
				&sortInfoID, &type,
				&creator);
	Errors::ThrowIfPalmError(retval);
	
	dbR = DmOpenDatabase(nCardNo, nDbID, dmModeReadOnly);
	if (!dbR) 
		Errors::ThrowIfPalmError(DmGetLastErr());

	FileRef	fileNo;
	retval = VFSFileOpen(nVolRefNum, pPathName, vfsModeReadWrite, &fileNo);
	if (retval)
	{
		DmCloseDatabase(dbR);
		Errors::ThrowIfPalmError(retval);
	}

	//	if (PrvIsResources(attributes)) 
//	{
//	}
//	else
	{
		UInt16		attr, numRecords;
		UInt32		uniqueID, size, offset, i, wrSize;
		MemHandle	srcH;
		
		numRecords = DmNumRecords(dbR);
		size = sizeof(DatabaseHdrType) + numRecords * sizeof(RecordEntryType);
		StMemory	outP(size);
		
		// Fill in header
		hdrP = (DatabaseHdrPtr)outP.Get ();
		
		strcpy((char*)hdrP->name, name);
		hdrP->attributes = attributes;
		hdrP->version = version;
		hdrP->creationDate = crDate;
		hdrP->modificationDate = modDate;
		hdrP->lastBackupDate = bckUpDate;
		hdrP->modificationNumber = modNum;
		hdrP->appInfoID = 0;
		hdrP->sortInfoID = 0;
		hdrP->type = type;
		hdrP->creator = creator;
	
		hdrP->recordList.nextRecordListID = 0;
		hdrP->recordList.numRecords = numRecords;
		
	
		// Get the size of the appInfo and sort Info if they exist
		offset = size;
		MemHandle	appInfoH=0, sortInfoH=0;
		UInt32		appInfoSize=0, sortInfoSize=0;
		if (appInfoID) 
		{
			hdrP->appInfoID = offset;
			appInfoH = (MemHandle)MemLocalIDToGlobal(appInfoID, nCardNo);
		
			if (!appInfoH) 
			{
				Errors::ThrowIfPalmError(-1);
			}
			appInfoSize = MemHandleSize(appInfoH);
			offset += appInfoSize;
		}
			
		if (sortInfoID) 
		{
			hdrP->sortInfoID = offset;
			sortInfoH = (MemHandle)MemLocalIDToGlobal(sortInfoID, nCardNo);
			if (!sortInfoH) 
			{
				Errors::ThrowIfPalmError(-1);
			}
			sortInfoSize = MemHandleSize(sortInfoH);
			offset += sortInfoSize;
		}			
	
		// Fill in the info on each resource into the header
		RecordEntryPtr	entryP = (RecordEntryPtr)(&hdrP->recordList.firstEntry);
		for (i=0; i<numRecords; i++)  
		{
			retval = DmRecordInfo(dbR, i, &attr, &uniqueID, 0);
			if (retval) 
			{
				Errors::ThrowIfPalmError(-1);
			}
			
			srcH = DmQueryRecord(dbR, i);

			entryP->localChunkID = offset;
			entryP->attributes = attr;
			entryP->uniqueID[0] = (uniqueID >> 16) & 0x00FF;	// vmk 10/16/95 fixed: && 0x00FF --> & 0x00FF
			entryP->uniqueID[1] = (uniqueID >> 8) & 0x00FF;
			entryP->uniqueID[2] = uniqueID	& 0x00FF;
			
			if (srcH)
				offset += MemHandleSize(srcH);
			entryP++;
		}

		// Byteswap the resource entry headers
		entryP	= (RecordEntryPtr)(&hdrP->recordList.firstEntry);
		for (i=0; i<hdrP->recordList.numRecords; i++)
		{
			Canonical(*entryP);
			entryP++;
		}
		Canonical(*hdrP);	// Now in big-endian format
	
		// Write out  entry table
		VFSFileWrite(fileNo, size, outP.Get(), &wrSize);

		// Write out the appInfo followed by sortInfo, if they exist
		if (appInfoID && appInfoSize) 
		{
			UInt32		srcP;
			StMemory	outP(appInfoSize);
			srcP = (UInt32)MemHandleLock(appInfoH);
			uae_memcpy((void*) outP.Get(), srcP, appInfoSize);
			MemPtrUnlock((MemPtr)srcP);
			VFSFileWrite(fileNo, appInfoSize, outP.Get(), &wrSize);
		}
			
		if (sortInfoID && sortInfoSize) 
		{
			UInt32		srcP;
			StMemory	outP(sortInfoSize);
			srcP = (UInt32)MemHandleLock(sortInfoH);
			uae_memcpy((void*) outP.Get(), srcP, sortInfoSize);
			MemPtrUnlock((MemPtr)srcP);
			VFSFileWrite(fileNo, sortInfoSize, outP.Get(), &wrSize);
		}
	
		// Write out each record
		for (i=0; i<numRecords; i++) 
		{
			UInt32		recSize;
			
			retval = DmRecordInfo(dbR, i, &attr, &uniqueID, 0);
			if (retval) 
			{
				Errors::ThrowIfPalmError(-1);
			}
				
			srcH = DmQueryRecord(dbR, i);
				
			if (srcH) 
			{
				UInt32	srcP;
				
				recSize = MemHandleSize(srcH);
				StMemory	outP(recSize);
				srcP = (UInt32)MemHandleLock(srcH);
				uae_memcpy((void*) outP.Get(), srcP, recSize);
				MemPtrUnlock((MemPtr)srcP);
				VFSFileWrite(fileNo, recSize, outP.Get(), &wrSize);
			}
		}
	}
	VFSFileClose(fileNo);
	DmCloseDatabase(dbR);
	return retval;
}
*/
