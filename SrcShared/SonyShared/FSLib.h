/*******************************************************************
 * Copyright (c) 1998-2000 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FSLib.h
 *
 * Description: Sample file system library implementation.
 *
 * History:
 *		02/29/00	Created by Steve Minns
 *
 *******************************************************************/

/********************************************************************
 * Filename and Label conventions:
 *
 * All path names are absolute
 *
 * All filesystems must support filenames and labels that are up to 255 characters long,
 * using any normal character including spaces and lower case characters in any
 * character set and the following special characters:
 * $ % ' - _ @ ~ ` ! ( ) ^ # & + , ; = [ ]
 ********************************************************************
 * When creating the 8.3 name or label from a long filename or label:
 *  a) Create the name from the first 1-6 valid, non-space characters, before the last period.
 *		The only valid characters are:
 *			A-Z 0-9 $ % ' - _ @ ~ ` ! ( ) ^ # &
 *  b) the extension is the first three valid characters after the last period '.'
 *  c) the end of the 6 byte name is appended with ~1, or the next unique number.
 *
 * A label is created from the first 11 valid non-space characters.
 ********************************************************************/


#ifndef __FS_LIB_H__
#define __FS_LIB_H__

// BUILDING_APPLICATION
#if BUILDING_APP_OR_LIB_FS
	// direct link to library code
	#define FS_LIB_TRAP(trapNum)
#else
	// else someone else is including this public header file; use traps
	#define FS_LIB_TRAP(trapNum)	SYS_TRAP(trapNum)
#endif

#define FS_LIB_APIVersion				0x00000001


/********************************************************************
 * Type of FS Library database
 ********************************************************************/
#define sysFileTFileSystem		'libf'	// file type for file system libraries


/********************************************************************
 * FS library function trap ID's. Each library call gets a trap number:
 *   FSTrapXXXX which serves as an index into the library's dispatch table.
 *   The constant sysLibTrapCustom is the first available trap number after
 *   the system predefined library traps Open,Close,Sleep & Wake.
 *
 * WARNING!!! The order of these traps MUST match the order of the dispatch
 *  table in FSLibDispatch.c!!!
 ********************************************************************/

typedef enum {
	FSTrapLibAPIVersion = sysLibTrapCustom,
	FSTrapCustomControl,
	FSTrapFilesystemType,
	
	FSTrapFileCreate,
	FSTrapFileOpen,
	FSTrapFileClose,
	FSTrapFileRead,
	FSTrapFileWrite,
	FSTrapFileDelete,
	FSTrapFileRename,
	FSTrapFileSeek,
	FSTrapFileEOF,
	FSTrapFileTell,
	FSTrapFileResize,
	FSTrapFileAttributesGet,
	FSTrapFileAttributesSet,
	FSTrapFileDateGet,
	FSTrapFileDateSet,
	FSTrapFileSize,
	
	FSTrapDirCreate,
	FSTrapDirEntryEnumerate,
	
	FSTrapVolumeFormat,
	FSTrapVolumeMount,
	FSTrapVolumeUnmount,
	FSTrapVolumeInfo,
	FSTrapVolumeLabelGet,
	FSTrapVolumeLabelSet,
	FSTrapVolumeSize,
	
	FSMaxSelector = FSTrapVolumeSize,

	FSBigSelector = 0x7FFF	// Force FSLibSelector to be 16 bits.
} FSLibSelector;


/********************************************************************
 * API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
 * Standard library open, close, sleep and wake APIs:
 ********************************************************************/

extern Err FSLibOpen(UInt16 fsLibRefNum)
				FS_LIB_TRAP(sysLibTrapOpen);

extern Err FSLibClose(UInt16 fsLibRefNum)
				FS_LIB_TRAP(sysLibTrapClose);

extern Err FSLibSleep(UInt16 fsLibRefNum)
				FS_LIB_TRAP(sysLibTrapSleep);
	
extern Err FSLibWake(UInt16 fsLibRefNum)
				FS_LIB_TRAP(sysLibTrapWake);


/********************************************************************
 * Custom library APIs:
 ********************************************************************/

extern UInt32 FSLibAPIVersion(UInt16 fsLibRefNum)
				FS_LIB_TRAP(FSTrapLibAPIVersion);

extern Err FSCustomControl(UInt16 fsLibRefNum, UInt32 apiCreator, UInt16 apiSelector, 
							void *valueP, UInt16 *valueLenP)
				FS_LIB_TRAP(FSTrapCustomControl);
				
extern Err FSFilesystemType(UInt16 fsLibRefNum, UInt32 *filesystemTypeP)
				FS_LIB_TRAP(FSTrapFilesystemType);
				

/********************************************************************
 * File Stream APIs:
 ********************************************************************/
 
extern Err FSFileCreate(UInt16 fsLibRefNum, UInt16 volRefNum, const Char *pathNameP)
				FS_LIB_TRAP(FSTrapFileCreate);

extern Err FSFileOpen(UInt16 fsLibRefNum, UInt16 volRefNum, const Char *pathNameP,
	UInt16 openMode, FileRef *fileRefP)
				FS_LIB_TRAP(FSTrapFileOpen);

extern Err FSFileClose(UInt16 fsLibRefNum, FileRef fileRef)
				FS_LIB_TRAP(FSTrapFileClose);

extern Err FSFileRead(UInt16 fsLibRefNum, FileRef fileRef, UInt32 numBytes, 
						void *bufBaseP, UInt32 offset, Boolean dataStoreBased, 
						UInt32 *numBytesReadP)
				FS_LIB_TRAP(FSTrapFileRead);

extern Err FSFileWrite(UInt16 fsLibRefNum, FileRef fileRef, UInt32 numBytes, 
						const void *dataP, UInt32 *numBytesWrittenP)
				FS_LIB_TRAP(FSTrapFileWrite);

extern Err FSFileDelete(UInt16 fsLibRefNum, UInt16 volRefNum, const Char *pathNameP)
				FS_LIB_TRAP(FSTrapFileDelete);

extern Err FSFileRename(UInt16 fsLibRefNum, UInt16 volRefNum, const Char *pathNameP, const Char *newNameP)
				FS_LIB_TRAP(FSTrapFileRename);

extern Err FSFileSeek(UInt16 fsLibRefNum, FileRef fileRef, FileOrigin origin, Int32 offset)
				FS_LIB_TRAP(FSTrapFileSeek);

extern Err FSFileEOF(UInt16 fsLibRefNum, FileRef fileRef)
				FS_LIB_TRAP(FSTrapFileEOF);

extern Err FSFileTell(UInt16 fsLibRefNum, FileRef fileRef, UInt32 *filePosP)
				FS_LIB_TRAP(FSTrapFileTell);

extern Err FSFileResize(UInt16 fsLibRefNum, FileRef fileRef, UInt32 newSize)
				FS_LIB_TRAP(FSTrapFileResize);

extern Err FSFileAttributesGet(UInt16 fsLibRefNum, FileRef fileRef, UInt32 *attributesP)
				FS_LIB_TRAP(FSTrapFileAttributesGet);

extern Err FSFileAttributesSet(UInt16 fsLibRefNum, FileRef fileRef, UInt32 attributes)
				FS_LIB_TRAP(FSTrapFileAttributesSet);

extern Err FSFileDateGet(UInt16 fsLibRefNum, FileRef fileRef, UInt16 whichDate, UInt32 *dateP)
				FS_LIB_TRAP(FSTrapFileDateGet);

extern Err FSFileDateSet(UInt16 fsLibRefNum, FileRef fileRef, UInt16 whichDate, UInt32 date)
				FS_LIB_TRAP(FSTrapFileDateSet);

extern Err FSFileSize(UInt16 fsLibRefNum, FileRef fileRef, UInt32 *fileSizeP)
				FS_LIB_TRAP(FSTrapFileSize);


/********************************************************************
 * Directory APIs:
 ********************************************************************/
 
extern Err FSDirCreate(UInt16 fsLibRefNum, UInt16 volRefNum, const Char *dirNameP)
				FS_LIB_TRAP(FSTrapDirCreate);


/************************************************************
 *
 *  MACRO:			FSDirDelete
 *
 *  DESCRIPTION:	Delete a closed directory.
 *
 *  PARAMETERS:	fsLibRefNum				-- FS library reference number
 *				volRefNum				-- Volume reference number returned by FSVolumeMount
 *				pathNameP				-- Full path of the directory to be deleted
 *
 *  RETURNS:	errNone					-- no error
 *				expErrNotOpen			-- FS driver library has not been opened
 *				vfsErrFileStillOpen		-- Directory is still open
 *				vfsErrFileNotFound		-- the file could not be found 
 *				vfsErrVolumeBadRef		-- the volume has not been mounted with FSVolumeMount
 *
 *************************************************************/
#define FSDirDelete(fsLibRefNum, volRefNum, dirNameP)	\
		FSFileDelete(fsLibRefNum, volRefNum, dirNameP)

extern Err FSDirEntryEnumerate(UInt16 fsLibRefNum, FileRef dirRef, UInt32 *dirEntryIteratorP, FileInfoType *infoP)
				FS_LIB_TRAP(FSTrapDirEntryEnumerate);

/********************************************************************
 * Volume APIs:
 ********************************************************************/
 

extern Err FSVolumeFormat(UInt16 fsLibRefNum, VFSAnyMountParamPtr vfsMountParamP)
				FS_LIB_TRAP(FSTrapVolumeFormat);

extern Err FSVolumeMount(UInt16 fsLibRefNum, VFSAnyMountParamPtr vfsMountParamP)
				FS_LIB_TRAP(FSTrapVolumeMount);

extern Err FSVolumeUnmount(UInt16 fsLibRefNum, UInt16 volRefNum)
				FS_LIB_TRAP(FSTrapVolumeUnmount);

extern Err FSVolumeInfo(UInt16 fsLibRefNum, UInt16 volRefNum, VolumeInfoType *volInfoP)
				FS_LIB_TRAP(FSTrapVolumeInfo);

extern Err FSVolumeLabelGet(UInt16 fsLibRefNum, UInt16 volRefNum, Char *labelP, UInt16 bufLen)
				FS_LIB_TRAP(FSTrapVolumeLabelGet);

extern Err FSVolumeLabelSet(UInt16 fsLibRefNum, UInt16 volRefNum, const Char *labelP)
				FS_LIB_TRAP(FSTrapVolumeLabelSet);

extern Err FSVolumeSize(UInt16 fsLibRefNum, UInt16 volRefNum, UInt32 *volumeUsedP, UInt32 *volumeTotalP)
				FS_LIB_TRAP(FSTrapVolumeSize);


#ifdef __cplusplus 
}
#endif


#endif	// __FS_LIB_H__
