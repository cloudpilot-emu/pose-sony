#include "EmCommon.h"
#include "EmPatchState.h" 
#include "EmPalmFunction.h"

#include "UAE.h" 

#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"			// PARAM_VAL, etc.

#include "EmPatchModule_MsfsLib.h"	
#include "SonyWin\Platform_MsfsLib.h"	
#include "SonyShared\VFSMgr.h"
#include "SonyShared\FSLib.h"

// ======================================================================
//	Proto patch table for ExpansionManager functions.  
//  This array will be used to create a sparse array at runtime.
// ======================================================================

static void MsfsLibTrapReternValueChecker(void);

static ProtoPatchTableEntry	gProtoMsfsLibPatchTable[] =
{
	{sysLibTrapOpen,				MsfsLibHeadpatch::LibOpen,						NULL},
	{sysLibTrapClose,				MsfsLibHeadpatch::LibClose,						NULL},
	{sysLibTrapSleep,				MsfsLibHeadpatch::LibSleep,						NULL},
	{sysLibTrapWake,				MsfsLibHeadpatch::LibWake,						NULL},

	{FSTrapLibAPIVersion,			NULL,											NULL},
	{FSTrapCustomControl,			NULL,											NULL},
	{FSTrapFilesystemType,			MsfsLibHeadpatch::FSTrapFilesystemType,		    NULL},

	{FSTrapFileCreate,				MsfsLibHeadpatch::FSTrapFileCreate,				NULL},
	{FSTrapFileOpen,				MsfsLibHeadpatch::FSTrapFileOpen,				NULL},
	{FSTrapFileClose,				MsfsLibHeadpatch::FSTrapFileClose,				NULL},
	{FSTrapFileRead,				MsfsLibHeadpatch::FSTrapFileRead,				NULL},
	{FSTrapFileWrite,				MsfsLibHeadpatch::FSTrapFileWrite,				NULL},
	{FSTrapFileDelete,				MsfsLibHeadpatch::FSTrapFileDelete,				NULL},
	{FSTrapFileRename,				MsfsLibHeadpatch::FSTrapFileRename,				NULL},
	{FSTrapFileSeek,				MsfsLibHeadpatch::FSTrapFileSeek,				NULL},
	{FSTrapFileEOF,					MsfsLibHeadpatch::FSTrapFileEOF,				NULL},
	{FSTrapFileTell,				MsfsLibHeadpatch::FSTrapFileTell,				NULL},
	{FSTrapFileResize,				MsfsLibHeadpatch::FSTrapFileResize,				NULL},
	{FSTrapFileAttributesGet,		MsfsLibHeadpatch::FSTrapFileAttributesGet,		NULL},
	{FSTrapFileAttributesSet,		MsfsLibHeadpatch::FSTrapFileAttributesSet,		NULL},
	{FSTrapFileDateGet,				MsfsLibHeadpatch::FSTrapFileDateGet,			NULL},
	{FSTrapFileDateSet,				MsfsLibHeadpatch::FSTrapFileDateSet,			NULL},
	{FSTrapFileSize,				MsfsLibHeadpatch::FSTrapFileSize,				NULL},
	{FSTrapDirCreate,				MsfsLibHeadpatch::FSTrapDirCreate,				NULL},
	{FSTrapDirEntryEnumerate,		MsfsLibHeadpatch::FSTrapDirEntryEnumerate,		NULL},

	{FSTrapVolumeFormat,			MsfsLibHeadpatch::FSTrapVolumeFormat,			NULL},
	{FSTrapVolumeMount,				MsfsLibHeadpatch::FSTrapVolumeMount,			NULL},
	{FSTrapVolumeUnmount,			MsfsLibHeadpatch::FSTrapVolumeUnmount,			NULL},
	{FSTrapVolumeInfo,				MsfsLibHeadpatch::FSTrapVolumeInfo,				NULL},
	{FSTrapVolumeLabelGet,			MsfsLibHeadpatch::FSTrapVolumeLabelGet,			NULL},
	{FSTrapVolumeLabelSet,			MsfsLibHeadpatch::FSTrapVolumeLabelSet,			NULL},
	{FSTrapVolumeSize,				MsfsLibHeadpatch::FSTrapVolumeSize,				NULL},

	{-1,							NULL,											NULL}
};

static	char*	pMsfsLibFuncName[] =
{
	"FSTrapLibAPIVersion",
	"FSTrapCustomControl",
	"FSTrapFilesystemType",
	
	"FSTrapFileCreate",
	"FSTrapFileOpen",
	"FSTrapFileClose",
	"FSTrapFileRead",
	"FSTrapFileWrite",
	"FSTrapFileDelete",
	"FSTrapFileRename",
	"FSTrapFileSeek",
	"FSTrapFileEOF",
	"FSTrapFileTell",
	"FSTrapFileResize",
	"FSTrapFileAttributesGet",
	"FSTrapFileAttributesSet",
	"FSTrapFileDateGet",
	"FSTrapFileDateSet",
	"FSTrapFileSize",
	
	"FSTrapDirCreate",
	"FSTrapDirEntryEnumerate",
	
	"FSTrapVolumeFormat",
	"FSTrapVolumeMount",
	"FSTrapVolumeUnmount",
	"FSTrapVolumeInfo",
	"FSTrapVolumeLabelGet",
	"FSTrapVolumeLabelSet",
	"FSTrapVolumeSize",
	NULL
};

#define PARAMETER_SIZE(x)	\
	(sizeof (((StackFrame*) 0)->x))

#define PARAMETER_OFFSET(x) \
	(m68k_areg (regs, 7) + offsetof (StackFrame, x))

#define GET_PARAMETER(x)		\
	((PARAMETER_SIZE(x) == sizeof (char)) ? get_byte (PARAMETER_OFFSET(x)) :	\
	 (PARAMETER_SIZE(x) == sizeof (short)) ? get_word (PARAMETER_OFFSET(x)) :	\
											get_long (PARAMETER_OFFSET(x)))
#define SET_PARAMETER(x, v) 	\
	((PARAMETER_SIZE(x) == sizeof (char)) ? put_byte (PARAMETER_OFFSET(x), v) : \
	 (PARAMETER_SIZE(x) == sizeof (short)) ? put_word (PARAMETER_OFFSET(x), v) :	\
											put_long (PARAMETER_OFFSET(x), v))

// ======================================================================
//	Private functions
// ======================================================================

#define PRINTF	if (1) ; else LogAppendMsg


ProtoPatchTableEntry* MsfsLibHeadpatch::GetMsfsLibPatchesTable()
{
	return gProtoMsfsLibPatchTable;
}

// ========================================================================
// The following functions define a bunch of StackFrame structs.
// These structs need to mirror the format of parameters pushed
// onto the stack by the emulated code, and so need to be packed
// to 2-byte boundaries.
//
// The pragmas are reversed at the end of the file.
// ========================================================================
#include "PalmPack.h"

#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmPatchModule_MsfsLib::EmPatchModule_MsfsLib()
 *
 * DESCRIPTION:	Constructor
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/
EmPatchModule_MsfsLib::EmPatchModule_MsfsLib() :
	EmPatchModule("~MsfsLib"/*不明*/, gProtoMsfsLibPatchTable)
{
}

/***********************************************************************
 * FUNCTION:	MsfsLibTrapReternValueChecker
 *
 * DESCRIPTION:	ROM側のMSFS関数実行後の戻り値を確認するための関数
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

void MsfsLibTrapReternValueChecker(void)
{
	Err retval = m68k_dreg (regs, 0);
}

/***********************************************************************
 *
 * FUNCTION:	LibOpen()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::LibOpen (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("LibOpen");

	CALLED_SETUP ("Err", "UInt16 libRefNum");

	CALLED_GET_PARAM_VAL(UInt16, libRefNum);

	Platform_MsfsLib::LibOpen (libRefNum);

	return kExecuteROM;
}

/***********************************************************************
 *
 * FUNCTION:	LibClose()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipExecute
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::LibClose (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("LibClose");

	CALLED_SETUP ("Err", "UInt16 libRefNum");

	CALLED_GET_PARAM_VAL(UInt16, libRefNum);

	Platform_MsfsLib::LibClose (libRefNum);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	LibSleep()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kExecuteROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::LibSleep (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("LibSleep");

	CALLED_SETUP ("Err", "UInt16 libRefNum");

	CALLED_GET_PARAM_VAL(UInt16, libRefNum);

	Platform_MsfsLib::LibSleep (libRefNum);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	LibWake()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kExecuteROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::LibWake (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("LibWake");

	CALLED_SETUP ("Err", "UInt16 libRefNum");

	CALLED_GET_PARAM_VAL(UInt16, libRefNum);

	Platform_MsfsLib::LibWake (libRefNum);

	return kExecuteROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFilesystemType()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFilesystemType (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileSystemType");

	CALLED_SETUP ("Err", "UInt16 fsLibRefNum, UInt32* filesystemTypeP");

	CALLED_GET_PARAM_VAL(UInt16, fsLibRefNum);
	CALLED_GET_PARAM_REF(UInt32, filesystemTypeP, Marshal::kInOut);

	UInt32*	pFilesystemType = filesystemTypeP;
	*pFilesystemType = fsFilesystemType_VFAT;

	m68k_dreg (regs, 0) = errNone;

	filesystemTypeP.Put();

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileCreat()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileCreate (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileCreate");

	CALLED_SETUP ("Err", "UInt16 fsLibRefNum, UInt16 volRefNum, char* pathNameP");

	CALLED_GET_PARAM_VAL(UInt16, volRefNum);
	CALLED_GET_PARAM_STR(char, pathNameP);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileCreate (volRefNum, pathNameP);

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileOpen()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileOpen (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileOpen");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, UInt16 volRefNum"
						", char* pathNameP, UInt16 openMode"
						", FileRef* fileRefP"
	);

	CALLED_GET_PARAM_VAL(UInt16,	volRefNum);
	CALLED_GET_PARAM_STR(char,		pathNameP);
	CALLED_GET_PARAM_VAL(UInt16,	openMode);
	CALLED_GET_PARAM_REF(FileRef, fileRefP, Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileOpen (volRefNum, pathNameP, openMode, fileRefP);
	
	fileRefP.Put();

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileClose()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileClose (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileClose");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, FileRef fileRef");

	CALLED_GET_PARAM_VAL(FileRef, fileRef);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileClose (fileRef);

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileRead()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileRead (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileRead");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, FileRef fileRef"
						", UInt32 numBytes, void* bufBaseP"
						", UInt32 offset, Boolean dataStoreBased"
						", UInt32* numBytesReadP"
	);

	CALLED_GET_PARAM_VAL(UInt16,	fsLibRefNum);
	CALLED_GET_PARAM_VAL(FileRef,	fileRef);
	CALLED_GET_PARAM_VAL(UInt32,	numBytes);
	CALLED_GET_PARAM_VAL(UInt32,	offset);
	CALLED_GET_PARAM_VAL(Boolean,	dataStoreBased);
	CALLED_GET_PARAM_REF(UInt32,	numBytesReadP,	Marshal::kInOut);

	CALLED_GET_PARAM_PTR(void,		bufBaseP, numBytes + offset, Marshal::kInOut);

	Err retval = Platform_MsfsLib::FSFileReadData (fileRef, numBytes, bufBaseP, offset, numBytesReadP);
	m68k_dreg (regs, 0) = retval;

	bufBaseP.Put();
	numBytesReadP.Put();
	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileWrite()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileWrite (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileWrite");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef"
						", UInt32 numBytes, void* bufBaseP"
						", UInt32* numBytesWriteP"
	);

	CALLED_GET_PARAM_VAL(FileRef,	fileRef);
	CALLED_GET_PARAM_VAL(UInt32,	numBytes);
	CALLED_GET_PARAM_PTR(void,		bufBaseP, numBytes & 0xFFFF, Marshal::kInOut);
	CALLED_GET_PARAM_REF(UInt32,	numBytesWriteP,	Marshal::kInOut);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileWrite (fileRef, numBytes, bufBaseP, numBytesWriteP);

	numBytesWriteP.Put();
	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileDelete()
 *
 * DESCRIPTION: 指定ファイル／ディレクトリの削除	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileDelete (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileDelete");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, UInt16 volRefNum, char* pathNameP");

	CALLED_GET_PARAM_VAL(UInt16, volRefNum);
	CALLED_GET_PARAM_STR(char, pathNameP);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileDelete (volRefNum, pathNameP);

	return kSkipROM;
}


/***********************************************************************
  * FUNCTION:	FSTrapFileRename()
 *
 * DESCRIPTION: 指定ファイル／ディレクトリの名前変更	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileRename (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileRename");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, UInt16 volRefNum"
						", char* pathNameP, char* newNameP"
	);

	CALLED_GET_PARAM_VAL(UInt16,	volRefNum);
	CALLED_GET_PARAM_STR(char,		pathNameP);
	CALLED_GET_PARAM_STR(char,		newNameP);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileRename (volRefNum, pathNameP, newNameP);

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileSeek()
 *
 * DESCRIPTION:	オープンしたファイルの現在位置を設定する
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileSeek (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileSeek");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef"
						", FileOrigin origin, Int32 offset"
	);

	CALLED_GET_PARAM_VAL(FileRef,		fileRef);
	CALLED_GET_PARAM_VAL(FileOrigin,	origin);
	CALLED_GET_PARAM_VAL(Int32,			offset);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileSeek(fileRef, origin, offset);

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileEOF()
 *
 * DESCRIPTION:	オープンしたファイルのEndOfFileの状態を取得する
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileEOF (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileEOF");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef");

	CALLED_GET_PARAM_VAL(FileRef,		fileRef);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileEOF(fileRef);

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileTell()
 *
 * DESCRIPTION:	オープンしたファイルの現在位置を取得するる
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileTell (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileTell");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef, UInt32* filePosP");

	CALLED_GET_PARAM_VAL(FileRef,		fileRef);
	CALLED_GET_PARAM_REF(UInt32,		filePosP,	Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileTell (fileRef, filePosP);

	filePosP.Put();

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileAttributesGet()
 *
 * DESCRIPTION:	オープンしたファイルの属性を取得する
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileAttributesGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileAttributeGet");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef, UInt32* attrP");

	CALLED_GET_PARAM_VAL(FileRef,		fileRef);
	CALLED_GET_PARAM_REF(UInt32,		attrP,	Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileAttributesGet(fileRef, attrP);

	attrP.Put();

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileAttributeSet()
 *
 * DESCRIPTION:	オープンしたファイルの属性を設定する
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileAttributesSet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileAttributeSet");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef, UInt32 attr");

	CALLED_GET_PARAM_VAL(FileRef,		fileRef);
	CALLED_GET_PARAM_VAL(UInt32,		attr);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileAttributesSet (fileRef, attr);

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileDateGet()
 *
 * DESCRIPTION:	オープンしたファイルの作成／更新日付を取得する
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileDateGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileDateGet");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef, UInt16 whichDate, UInt32* dateP");

	CALLED_GET_PARAM_VAL(FileRef,		fileRef);
	CALLED_GET_PARAM_VAL(UInt16,		whichDate);
	CALLED_GET_PARAM_REF(UInt32,		dateP,	Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileDateGet (fileRef, whichDate, dateP);

	dateP.Put();

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileDateSet()
 *
 * DESCRIPTION:	オープンしたファイルの作成／更新日付を設定する
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapFileDateSet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileDateSet");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef, UInt16 whichDate, UInt32 data");

	CALLED_GET_PARAM_VAL(FileRef,		fileRef);
	CALLED_GET_PARAM_VAL(UInt16,		whichDate);
	CALLED_GET_PARAM_VAL(UInt32,		date);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileDateSet	(fileRef, whichDate, date);

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapFileSize()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapFileSize (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileSize");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef, UInt32* fileSizeP");

	CALLED_GET_PARAM_VAL(FileRef, fileRef);
	CALLED_GET_PARAM_REF(UInt32, fileSizeP, Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileSize (fileRef, fileSizeP);

	fileSizeP.Put();

	return kSkipROM;
}

/***********************************************************************
 * FUNCTION:	FSTrapFileResize()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapFileResize (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapFileResize");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, FileRef fileRef, UInt32 newSize");

	CALLED_GET_PARAM_VAL(FileRef,	fileRef);
	CALLED_GET_PARAM_VAL(UInt32,	newSize);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSFileResize (fileRef, newSize);

	return kSkipROM;
}

/***********************************************************************
 * FUNCTION:	FSTrapDirCreate()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapDirCreate (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapDirCreate");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, UInt16 volRefNum, char* dirNameP");

	CALLED_GET_PARAM_VAL(UInt16,	volRefNum);
	CALLED_GET_PARAM_STR(char,		dirNameP);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSDirCreate (volRefNum, dirNameP);

	return kSkipROM;
}

/***********************************************************************
 * FUNCTION:	FSTrapDirEntryEnumerate
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapDirEntryEnumerate (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapDirEntryEnumerate");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, FileRef dirRef"
						", UInt32* dirEntryP, FileInfoType* infoP"
	);

	CALLED_GET_PARAM_VAL(UInt16,		fsLibRefNum);
	CALLED_GET_PARAM_VAL(FileRef,		dirRef);
	CALLED_GET_PARAM_REF(UInt32,		dirEntryP,	Marshal::kInOut);
	CALLED_GET_PARAM_REF(FileInfoType,	infoP,		Marshal::kInOut);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSDirEntryEnumerate(dirRef, dirEntryP, infoP);

	dirEntryP.Put();
	infoP.Put();

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapVolumeFormat
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapVolumeFormat (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeFormat");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, VFSAnyMountParamType* vfsMountParamP");

	CALLED_GET_PARAM_VAL(UInt16,				fsLibRefNum);
	CALLED_GET_PARAM_REF(VFSAnyMountParamType,	vfsMountParamP,		Marshal::kInput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeFormat(vfsMountParamP);

	return kSkipROM;
}

#if 0
// 現状,VolumeEnumerateの機能自体がMSFSのレイヤではなくなっため、
// この関数が呼び出されることはない。
/***********************************************************************
 * FUNCTION:	FSTrapVolumeEnumerate
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapVolumeEnumerate (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeEnumerate");

	struct StackFrame
	{
		UInt16*			volRefNumP;
		UInt16*			volIteratorP;
	};

	PARAM_REF(UInt16,	volRefNumP,		Marshal::kOutput);
	PARAM_REF(UInt16,	volIteratorP,	Marshal::kInOut);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeEnumerate(volRefNumP, volIteratorP);

	volRefNumP.Put();
	volIteratorP.Put();

	return kSkipROM;
}
#endif

/***********************************************************************
 * FUNCTION:	FSTrapVolumeInfo
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapVolumeInfo (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeInfo");
#ifdef _DEBUG
	OutputDebugString("MsfsLib Call -- FSVolumeInfo\n");
#endif

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, UInt16 volRefNum, VolumeInfoType* infoP");

	CALLED_GET_PARAM_VAL(UInt16,			fsLibRefNum);
	CALLED_GET_PARAM_VAL(UInt16,			volRefNum);
	CALLED_GET_PARAM_REF(VolumeInfoType,	infoP,		Marshal::kInOut);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeInfo(volRefNum, infoP);

	infoP.Put();

	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	FSTrapVolumeSize
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType MsfsLibHeadpatch::FSTrapVolumeSize (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeSize");

	CALLED_SETUP("Err", "UInt16	fsLibRefNum, UInt16 volRefNum"
						", UInt32* volumeUsedP, UInt32* volumeTotalP"
	);

	CALLED_GET_PARAM_VAL(UInt16,	volRefNum);
	CALLED_GET_PARAM_REF(UInt32,	volumeUsedP,	Marshal::kOutput);
	CALLED_GET_PARAM_REF(UInt32,	volumeTotalP,	Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeSize(volRefNum, volumeUsedP, volumeTotalP);

	volumeUsedP.Put();
	volumeTotalP.Put();

	return kSkipROM;
}

CallROMType MsfsLibHeadpatch::FSTrapVolumeMount (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeMount");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, VFSAnyMountParamType* vfsMountParamP");

	CALLED_GET_PARAM_VAL(UInt16, fsLibRefNum);
	CALLED_GET_PARAM_REF(VFSAnyMountParamType, vfsMountParamP, Marshal::kInOut);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeMount(vfsMountParamP);

	return kSkipROM;
}

/***********************************************************************
 * FUNCTION:	FSTrapVolumeUnmount
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapVolumeUnmount (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeUnmount");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, UInt16 volRefNum");

	CALLED_GET_PARAM_VAL(UInt16, fsLibRefNum);
	CALLED_GET_PARAM_VAL(UInt16, volRefNum);

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}

/***********************************************************************
 * FUNCTION:	FSTrapVolumeLabelSet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapVolumeLabelSet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeLabelSet");

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, UInt16 volRefNum, char* labelP");

	CALLED_GET_PARAM_VAL(UInt16,	fsLibRefNum);
	CALLED_GET_PARAM_VAL(UInt16,	volRefNum);
	CALLED_GET_PARAM_STR(char,		labelP);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeLabelSet(volRefNum, labelP);

	return kSkipROM;
}

/***********************************************************************
 * FUNCTION:	FSTrapVolumeLabelGet
 *
 * DESCRIPTION:	MSカードのボリュームラベルの取得
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType MsfsLibHeadpatch::FSTrapVolumeLabelGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("FSTrapVolumeLabelGet");

	struct StackFrame
	{
		UInt16			fsLibRefNum;
		UInt16			volRefNum;
		char*			labelP;
		UInt16			bufLen;
	};

	CALLED_SETUP("Err", "UInt16 fsLibRefNum, UInt16 volRefNum, char* labelP, UInt16 bufLen");

	CALLED_GET_PARAM_VAL(UInt16, fsLibRefNum);
	CALLED_GET_PARAM_VAL(UInt16, volRefNum);
	CALLED_GET_PARAM_STR(char,	labelP);
	CALLED_GET_PARAM_VAL(UInt16, bufLen);

	emuptr	label = GET_PARAMETER(labelP);

	char*	tmp;

	if ((tmp = (char*)Platform::AllocateMemory (bufLen)) == NULL)
	{
		m68k_dreg (regs, 0) = -1;
	}
	else
	{
		m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeLabelGet(volRefNum, tmp, bufLen);
		EmMem_memcpy (label, (void*)tmp, (UInt16)bufLen);
		Platform::DisposeMemory (tmp);
	}

	return kSkipROM;
}

#include "PalmPackPop.h"

