/******************************************************************************
 *
 * Copyright (c) 2000 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: VFSMgr.h
 *
 * Description:
 *		Header file for VFS Manager.
 *
 * History:
 *		02/25/00	jed	Created by Jesse Donaldson.
 *
 *****************************************************************************/

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

#ifndef __VFSMGR_H__
#define __VFSMGR_H__

#include <PalmTypes.h>
#include <CoreTraps.h>
#include <SystemMgr.h>

#include "ExpansionMgr.h"

#ifdef BUILDING_AGAINST_PALMOS35
	#define sysTrapVFSMgr	sysTrapSysReserved3

	#define vfsErrorClass	0x2A00		// Post-3.5 this is defined in ErrorBase.h
#else
	#define sysTrapVFSMgr	sysTrapFileSystemDispatch
#endif



#ifdef BUILDING_VFSMGR_DISPATCH
	#define VFSMGR_TRAP(VFSMgrSelectorNum)
#else
	#define VFSMGR_TRAP(sel) \
		_SYSTEM_API(_CALL_WITH_SELECTOR)(_SYSTEM_TABLE, sysTrapVFSMgr, sel)
#endif


#define sysFileCVFSMgr		'vfsm'


#define vfsFtrIDVersion		0	// ID of feature containing version of VFSMgr.
										// Check existence of this feature to see if VFSMgr is installed.

#define vfsFtrIDTempData	1	// ID of feature used for storing temporary data

#define vfsMgrVersionNum	((UInt16)1)	// version of the VFSMgr, obtained from the feature

// MountClass constants:
#define VFSMountClass_SlotDriver		sysFileTSlotDriver
#define VFSMountClass_Simulator		sysFileTSimulator



// Base MountParamType; others such as SlotMountParamType are extensions of this base type,
// switched on value of "mountClass" parameter.  It will make more sense someday when there
// are other kinds of FileSystems...  (Trust us.  :-)
typedef struct VFSAnyMountParamTag {
	UInt16 volRefNum;				// The volRefNum of the volume.
	UInt16 reserved;
	UInt32 mountClass;			// 'libs' for slotDriver-based filesystems
	
	// Other fields here, depending on value of 'mountClass'
	
} VFSAnyMountParamType;
typedef VFSAnyMountParamType *VFSAnyMountParamPtr ;


typedef struct VFSSlotMountParamTag {
	VFSAnyMountParamType	vfsMountParam;		// mountClass = VFSMountClass_SlotDriver = 'libs'
	UInt16				slotLibRefNum;
	UInt16				slotRefNum;
} VFSSlotMountParamType;

/* For Example...
typedef struct VFSOtherMountParamTag {
	VFSAnyMountParamType	vfsMountParam;		// mountClass = 'othr' (for example)
	UInt16				otherValue;
} VFSOtherMountParamType;
*/

typedef struct FileInfoTag{
	UInt32	attributes;
	Char		*nameP;				// buffer to receive full name; pass NULL to avoid getting name
	UInt16	nameBufLen; 		// size of nameP buffer, in bytes
} FileInfoType, *FileInfoPtr;



typedef struct VolumeInfoTag{
	UInt32	attributes;			// read-only etc.
	UInt32	fsType;				// Filesystem type for this volume (defined below)
	UInt32	fsCreator;			// Creator code of filesystem driver for this volume.  For use with VFSCustomControl().
	UInt32	mountClass;			// mount class that mounted this volume
	
	// For slot based filesystems: (mountClass = VFSMountClass_SlotDriver)
	UInt16	slotLibRefNum;		// Library on which the volume is mounted
	UInt16	slotRefNum;			// ExpMgr slot number of card containing volume
	UInt32	mediaType;			// Type of card media (mediaMemoryStick, mediaCompactFlash, etc...)
	UInt32	reserved;			// reserved for future use (other mountclasses may need more space)
} VolumeInfoType, *VolumeInfoPtr;



typedef UInt32 FileRef;

#define	vfsInvalidVolRef		0
#define	vfsInvalidFileRef		0


/************************************************************
 * File Origin constants: (for the origins of relative offsets passed to 'seek' type routines).
 *************************************************************/
#define fsOriginBeginning		0	// from the beginning (first data byte of file)
#define fsOriginCurrent			1	// from the current position
#define fsOriginEnd				2	// from the end of file (one position beyond last data byte, only negative offsets are legal)

typedef UInt16	FileOrigin;


/************************************************************
 * openMode flags passed to VFSFileOpen
 *************************************************************/
#define vfsModeExclusive			(0x0001UL)		// don't let anyone else open it
#define vfsModeRead					(0x0002UL)		// open for read access
#define vfsModeWrite					(0x0004UL | vfsModeExclusive)		// open for write access, implies exclusive
#define vfsModeReadWrite			(vfsModeWrite | vfsModeRead)		// open for read/write access

#define vfsModeAll					(0x0007UL)


/************************************************************
 * File Attributes
 *************************************************************/
#define vfsFileAttrReadOnly		(0x00000001UL)
#define vfsFileAttrHidden			(0x00000002UL)
#define vfsFileAttrSystem			(0x00000004UL)
#define vfsFileAttrVolumeLabel	(0x00000008UL)
#define vfsFileAttrDirectory		(0x00000010UL)
#define vfsFileAttrArchive			(0x00000020UL)
#define vfsFileAttrLink				(0x00000040UL)

#define vfsFileAttrAll				(0x0000007fUL)


/************************************************************
 * Volume Attributes
 *************************************************************/
#define vfsVolumeAttrSlotBased	(0x00000001UL)		// volume is associated with a slot driver
#define vfsVolumeAttrReadOnly		(0x00000002UL)		// volume is read only
#define vfsVolumeAttrHidden		(0x00000004UL)		// volume should not be user-visible.


/************************************************************
 * Date constants (for use with VFSFileDateGet/Set)
 *************************************************************/
#define vfsFileDateCreated			1
#define vfsFileDateModified		2
#define vfsFileDateAccessed		3


/************************************************************
 * Format/Mount flags (for use with VFSVolumeFormat/Mount)
 *************************************************************/
#define vfsMountFlagsUseThisFileSystem		0x01	// Mount/Format the volume with the filesystem specified
#define vfsMountFlagsReserved1				0x02	// reserved
#define vfsMountFlagsReserved2				0x04	// reserved
#define vfsMountFlagsReserved3				0x08	// reserved
#define vfsMountFlagsReserved4				0x10	// reserved
#define vfsMountFlagsReserved5				0x20	// reserved
#define vfsMountFlagsReserved6				0x40	// reserved
#define vfsMountFlagsReserved7				0x80	// reserved


/************************************************************
 * Common filesystem types.  Used by FSFilesystemType and SlotCardIsFilesystemSupported.
 *************************************************************/
#define fsFilesystemType_VFAT			'vfat'		// FAT12 and FAT16 extended to handle long file names
#define fsFilesystemType_FAT			'fats'		// FAT12 and FAT16 which only handles 8.3 file names
#define fsFilesystemType_NTFS			'ntfs'		// Windows NT filesystem
#define fsFilesystemType_HFSPlus		'hfse'		// The Macintosh extended heirarchical filesystem
#define fsFilesystemType_HFS			'hfss'		// The Macintosh standard heirarchical filesystem
#define fsFilesystemType_MFS			'mfso'		// The Macintosh original filesystem
#define fsFilesystemType_EXT2			'ext2'		// Linux filesystem
#define fsFilesystemType_FFS			'ffsb'		// Unix Berkeley block based filesystem
#define fsFilesystemType_NFS			'nfsu'		// Unix Networked filesystem
#define fsFilesystemType_AFS			'afsu'		// Unix Andrew filesystem
#define fsFilesystemType_Novell		'novl'		// Novell filesystem
#define fsFilesystemType_HPFS			'hpfs'		// OS2 High Performance filesystem


/************************************************************
 * Error codes
 *************************************************************/
#define vfsErrBufferOverflow			(vfsErrorClass | 1)	// passed in buffer is too small
#define vfsErrFileGeneric				(vfsErrorClass | 2)	// Generic file error.
#define vfsErrFileBadRef				(vfsErrorClass | 3)	// the fileref is invalid (has been closed, or was not obtained from VFSFileOpen())
#define vfsErrFileStillOpen			(vfsErrorClass | 4)	// returned from FSFileDelete if the file is still open
#define vfsErrFilePermissionDenied	(vfsErrorClass | 5)	// The file is read only
#define vfsErrFileAlreadyExists		(vfsErrorClass | 6)	// a file of this name exists already in this location
#define vfsErrFileEOF					(vfsErrorClass | 7)	// file pointer is at end of file
#define vfsErrFileNotFound				(vfsErrorClass | 8)	// file was not found at the path specified
#define vfsErrVolumeBadRef				(vfsErrorClass | 9)	// the volume refnum is invalid.
#define vfsErrVolumeStillMounted		(vfsErrorClass | 10)	// returned from FSVolumeFormat if the volume is still mounted
#define vfsErrNoFileSystem				(vfsErrorClass | 11)	// no installed filesystem supports this operation
#define vfsErrBadData					(vfsErrorClass | 12)	// operation could not be completed because of invalid data (i.e., import DB from .PRC file)
#define vfsErrDirNotEmpty				(vfsErrorClass | 13)	// can't delete a non-empty directory
#define vfsErrBadName					(vfsErrorClass | 14)	// invalid filename, or path, or volume label or something...
#define vfsErrVolumeFull				(vfsErrorClass | 15)	// not enough space left on volume
#define vfsErrUnimplemented			(vfsErrorClass | 16)	// this call is not implemented
#define vfsErrNotADirectory			(vfsErrorClass | 17)	// This operation requires a directory
#define vfsErrIsADirectory          (vfsErrorClass | 18) // This operation requires a regular file, not a directory
#define vfsErrDirectoryNotFound		(vfsErrorClass | 19) // Returned from VFSFileCreate when the path leading up to the new file does not exist


/************************************************************
 * Selectors for routines found in the VFS manager. The order
 * of these selectors MUST match the jump table in VFSMgr.c.
 *************************************************************/
typedef enum {
	vfsTrapInit = 0,
	vfsTrapCustomControl,
	
	vfsTrapFileCreate,		// 2
	vfsTrapFileOpen,		// 3
	vfsTrapFileClose,		// 4
	vfsTrapFileReadData,	// 5
	vfsTrapFileRead,		// 6
	vfsTrapFileWrite,		// 7
	vfsTrapFileDelete,		// 8
	vfsTrapFileRename,		// 9
	vfsTrapFileSeek,		// 10
	vfsTrapFileEOF,			// 11
	vfsTrapFileTell,		// 12
	vfsTrapFileResize,		//13
	vfsTrapFileAttributesGet,	// 14
	vfsTrapFileAttributesSet,	// 15
	vfsTrapFileDateGet,			// 16
	vfsTrapFileDateSet,			// 17
	vfsTrapFileSize,			// 18
	
	vfsTrapDirCreate,					// 19
	vfsTrapDirEntryEnumerate,			// 20
	vfsTrapGetDefaultDirectory,			// 21
	vfsTrapRegisterDefaultDirectory,	// 22
	vfsTrapUnregisterDefaultDirectory,	// 22
	
	vfsTrapVolumeFormat,	// 23
	vfsTrapVolumeMount,		// 24
	vfsTrapVolumeUnmount,	// 25
	vfsTrapVolumeEnumerate,	// 26
	vfsTrapVolumeInfo,		// 27
	vfsTrapVolumeLabelGet,	// 28
	vfsTrapVolumeLabelSet,	// 29
	vfsTrapVolumeSize,		// 30
	
	vfsTrapInstallFSLib,	// 31
	vfsTrapRemoveFSLib,		// 32
	vfsTrapImportDatabaseFromFile,	// 33
	vfsTrapExportDatabaseToFile,	// 34
	vfsTrapFileDBGetResource,		// 35
	vfsTrapFileDBInfo,				// 36
	vfsTrapFileDBGetRecord,			// 37
		
	vfsMaxSelector = vfsTrapFileDBGetRecord,

	vfsBigSelector = 0x7FFF	// Force VFSMgrSelector to be 16 bits.
} VFSMgrSelector;


Err VFSInit(void)
		VFSMGR_TRAP(vfsTrapInit);

// if you pass NULL for fsCreator, VFS will iterate through 
// all installed filesystems until it finds one that does not return an error.
Err VFSCustomControl(UInt32 fsCreator, UInt32 apiCreator, UInt16 apiSelector, 
							void *valueP, UInt16 *valueLenP)
		VFSMGR_TRAP(vfsTrapCustomControl);

Err VFSFileCreate(UInt16 volRefNum, const Char *pathNameP)
		VFSMGR_TRAP(vfsTrapFileCreate);

Err VFSFileOpen(UInt16 volRefNum, const Char *pathNameP,
	UInt16 openMode, FileRef *fileRefP)
		VFSMGR_TRAP(vfsTrapFileOpen);

Err VFSFileClose(FileRef fileRef)
		VFSMGR_TRAP(vfsTrapFileClose);

Err VFSFileReadData(FileRef fileRef, UInt32 numBytes, void *bufBaseP, 
						UInt32 offset, UInt32 *numBytesReadP)
		VFSMGR_TRAP(vfsTrapFileReadData);

Err VFSFileRead(FileRef fileRef, UInt32 numBytes, void *bufP, UInt32 *numBytesReadP)
		VFSMGR_TRAP(vfsTrapFileRead);

Err VFSFileWrite(FileRef fileRef, UInt32 numBytes, const void *dataP, UInt32 *numBytesWrittenP)
		VFSMGR_TRAP(vfsTrapFileWrite);

// some file routines work on directories
Err VFSFileDelete(UInt16 volRefNum, const Char *pathNameP)
		VFSMGR_TRAP(vfsTrapFileDelete);

Err VFSFileRename(UInt16 volRefNum, const Char *pathNameP, const Char *newNameP)
		VFSMGR_TRAP(vfsTrapFileRename);

Err VFSFileSeek(FileRef fileRef, FileOrigin origin, Int32 offset)
		VFSMGR_TRAP(vfsTrapFileSeek);

Err VFSFileEOF(FileRef fileRef)
		VFSMGR_TRAP(vfsTrapFileEOF);

Err VFSFileTell(FileRef fileRef, UInt32 *filePosP)
		VFSMGR_TRAP(vfsTrapFileTell);

Err VFSFileResize(FileRef fileRef, UInt32 newSize)
		VFSMGR_TRAP(vfsTrapFileResize);

Err VFSFileAttributesGet(FileRef fileRef, UInt32 *attributesP)
		VFSMGR_TRAP(vfsTrapFileAttributesGet);

Err VFSFileAttributesSet(FileRef fileRef, UInt32 attributes)
		VFSMGR_TRAP(vfsTrapFileAttributesSet);

Err VFSFileDateGet(FileRef fileRef, UInt16 whichDate, UInt32 *dateP)
		VFSMGR_TRAP(vfsTrapFileDateGet);

Err VFSFileDateSet(FileRef fileRef, UInt16 whichDate, UInt32 date)
		VFSMGR_TRAP(vfsTrapFileDateSet);

Err VFSFileSize(FileRef fileRef, UInt32 *fileSizeP)
		VFSMGR_TRAP(vfsTrapFileSize);


Err VFSDirCreate(UInt16 volRefNum, const Char *dirNameP)
		VFSMGR_TRAP(vfsTrapDirCreate);

Err VFSDirEntryEnumerate(FileRef dirRef, UInt32 *dirEntryIteratorP, FileInfoType *infoP)
		VFSMGR_TRAP(vfsTrapDirEntryEnumerate);


Err VFSGetDefaultDirectory(UInt16 volRefNum, const Char *fileTypeStr,
			Char *pathStr, UInt16 *bufLenP)
		VFSMGR_TRAP(vfsTrapGetDefaultDirectory);

Err VFSRegisterDefaultDirectory(const Char *fileTypeStr, UInt32 mediaType, 
			const Char *pathStr)
		VFSMGR_TRAP(vfsTrapRegisterDefaultDirectory);

Err VFSUnregisterDefaultDirectory(const Char *fileTypeStr, UInt32 mediaType)
		VFSMGR_TRAP(vfsTrapUnregisterDefaultDirectory);



Err VFSVolumeFormat(UInt8 flags, UInt16 fsLibRefNum, VFSAnyMountParamPtr vfsMountParamP)
		VFSMGR_TRAP(vfsTrapVolumeFormat);

Err VFSVolumeMount(UInt8 flags, UInt16 fsLibRefNum, VFSAnyMountParamPtr vfsMountParamP)
		VFSMGR_TRAP(vfsTrapVolumeMount);

Err VFSVolumeUnmount(UInt16 volRefNum)
		VFSMGR_TRAP(vfsTrapVolumeUnmount);

Err VFSVolumeEnumerate(UInt16 *volRefNumP, UInt32 *volIteratorP)
		VFSMGR_TRAP(vfsTrapVolumeEnumerate);

Err VFSVolumeInfo(UInt16 volRefNum, VolumeInfoType *volInfoP)
		VFSMGR_TRAP(vfsTrapVolumeInfo);

Err VFSVolumeLabelGet(UInt16 volRefNum, Char *labelP, UInt16 bufLen)
		VFSMGR_TRAP(vfsTrapVolumeLabelGet);

Err VFSVolumeLabelSet(UInt16 volRefNum, const Char *labelP)
		VFSMGR_TRAP(vfsTrapVolumeLabelSet);

Err VFSVolumeSize(UInt16 volRefNum, UInt32 *volumeUsedP, UInt32 *volumeTotalP)
		VFSMGR_TRAP(vfsTrapVolumeSize);

Err VFSInstallFSLib(UInt32 creator, UInt16 *fsLibRefNumP)
		VFSMGR_TRAP(vfsTrapInstallFSLib);

Err VFSRemoveFSLib(UInt16 fsLibRefNum)
		VFSMGR_TRAP(vfsTrapRemoveFSLib);

Err VFSImportDatabaseFromFile(UInt16 volRefNum, const Char *pathNameP, 
							UInt16 *cardNoP, LocalID *dbIDP)
		VFSMGR_TRAP(vfsTrapImportDatabaseFromFile);

Err VFSExportDatabaseToFile(UInt16 volRefNum, const Char *pathNameP, 
							UInt16 cardNo, LocalID dbID)
		VFSMGR_TRAP(vfsTrapExportDatabaseToFile);

Err VFSFileDBGetResource(FileRef ref, DmResType type, DmResID resID, MemHandle *resHP)
		VFSMGR_TRAP(vfsTrapFileDBGetResource);

Err VFSFileDBInfo(FileRef ref, Char *nameP,
					UInt16 *attributesP, UInt16 *versionP, UInt32 *crDateP,
					UInt32 *modDateP, UInt32 *bckUpDateP,
					UInt32 *modNumP, MemHandle *appInfoHP,
					MemHandle *sortInfoHP, UInt32 *typeP,
					UInt32 *creatorP, UInt16 *numRecordsP)
		VFSMGR_TRAP(vfsTrapFileDBInfo);

Err VFSFileDBGetRecord(FileRef ref, UInt16 recIndex, MemHandle *recHP, 
									UInt8 *recAttrP, UInt32 *uniqueIDP)
		VFSMGR_TRAP(vfsTrapFileDBGetRecord);

#endif	// __VFSMGR_H__
