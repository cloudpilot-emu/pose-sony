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
#include "TrapPatches.h"

#include "Logging.h"					// LogAppendMsg
#include "Marshal.h"					// PARAM_VAL, etc.
#include "TrapPatches_VfsLib.h"	
#include "SonyWin\Platform_VfsLib.h"	// Platform_VfsLib
#include "SonyWin\Platform_MsfsLib.h"	// FSVolumeEnumerate()


/***************************************************************************
 * MS FAT API library function trap ID's. Each library call gets a trap number:
 *   VfsLibTrapXXXX which serves as an index into the library's dispatch
 *   table. The constant sysLibTrapCustom is the first available trap number
 *   after the system predefined library traps Open,Close,Sleep & Wake.
 ****************************************************************************/


// ======================================================================
//	Proto patch table for VfsMgr functions.  This array will be used
//	to create a sparse array at runtime.
// ======================================================================
static void VfsLibTrapReternValueChecker(void);

static ProtoPatchTableEntry	gProtoVfsMgrPatchTable[] =
{
	{vfsTrapVolumeInfo,					NULL/*VfsMgrLibHeadpatch::VfsLibTrapVolumeInfo*/,			NULL},
	{vfsTrapVolumeMount,				VfsMgrLibHeadpatch::VfsLibTrapVolumeMount,					NULL},
	{vfsTrapVolumeEnumerate,			NULL/*VfsMgrLibHeadpatch::VfsLibTrapVolumeEnumerate*/,				VfsLibTrapReternValueChecker},
//	{vfsTrapImportDatabaseFromFile,		VfsMgrLibHeadpatch::VfsLibTrapImportDatabaseFromFile,	NULL},
//	{vfsTrapExportDatabaseToFile,		VfsMgrLibHeadpatch::VfsLibTrapExportDatabaseToFile,	NULL},
	{vfsTrapVolumeSize,					VfsMgrLibHeadpatch::VfsLibTrapVolumeSize,					NULL},
	{0xFFFF,							NULL,												NULL}
};

static	char*	pVfsLibFuncName[] =
{
	"VFSInit",
	"VFSCustomControl",
	"VFSFileCreate",
	"VFSFileOpen",
	"VFSFileClose",
	"VFSFileReadData",
	"VFSFileRead",
	"VFSFileWrite",
	"VFSFileDelete",
	"VFSFileRename",	
	"VFSFileSeek",	
	"VFSFileEOF",		
	"VFSFileTell",		
	"VFSFileResize",	
	"VFSFileAttributesGet",
	"VFSFileAttributesSet",	
	"VFSFileDateGet",			
	"VFSFileDateSet",		
	"VFSFileSize",		
	
	"VFSDirCreate",				
	"VFSDirEntryEnumerate",		
	"VFSGetDefaultDirectory",		
	"VFSRegisterDefaultDirectory",
	"VFSUnregisterDefaultDirectory",
	
	"VFSVolumeFormat",	
	"VFSVolumeMount",	
	"VFSVolumeUnmount",	
	"VFSVolumeEnumerate",
	"VFSVolumeInfo",	
	"VFSVolumeLabelGet",
	"VFSVolumeLabelSet",
	"VFSVolumeSize",			
	
	"VFSInstallFSLib",			
	"VFSRemoveFSLib",			
	"VFSImportDatabaseFromFile",
	"VFSExportDatabaseToFile",	
	"VFSFileDBGetResource",			
	"VFSFileDBInfo",			
	"VFSFileDBGetRecord",		
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

Bool VfsMgrLibHeadpatch::GetVfsMgrPatches(	
		const SystemCallContext& context,
		HeadpatchProc& hp,
		TailpatchProc& tp)
{
	if (context.fTrapWord != sysTrapVFSMgr)
		return false;

	if (context.fExtra > vfsMaxSelector)
		return false;

	hp = NULL;
	tp = NULL;
	for (int i=0; gProtoVfsMgrPatchTable[i].fTrapWord!=0xFFFF; i++)
	{
		if (gProtoVfsMgrPatchTable[i].fTrapWord == context.fExtra)
		{
			hp = gProtoVfsMgrPatchTable[i].fHeadpatch;
			tp = gProtoVfsMgrPatchTable[i].fTailpatch;
			break;
		}
	}
	return true;
}

/***********************************************************************
 * FUNCTION:	VfsLibTrapReternValueChecker
 *
 * DESCRIPTION:	ROM側のVFS関数実行後の戻り値を確認する
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

void VfsLibTrapReternValueChecker(void)
{
	Err retval = m68k_dreg (regs, 0);
}

/***********************************************************************
 * FUNCTION:	VfsLibTrapVolumeMount
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kExecuteRom
 ***********************************************************************/

CallROMType VfsMgrLibHeadpatch::VfsLibTrapVolumeMount (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("VfsLibTrapVolumeMount");
	struct StackFrame
	{
		UInt8					flags;
		UInt16					volRefNum;
		VFSAnyMountParamType*	vfsMountParamP;
	};

	PARAM_VAL(UInt16,				volRefNum);
	PARAM_REF(VFSAnyMountParamType, vfsMountParamP, Marshal::kInOut);

//	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeMount(vfsMountParamP);

	return kExecuteROM;
}


/***********************************************************************
 * VfsFunc 3.23
 * FUNCTION:	VfsLibTrapVolumeSize
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType VfsMgrLibHeadpatch::VfsLibTrapVolumeSize (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("VfsLibTrapVolumeSize");
	struct StackFrame
	{
		UInt16			volRefNum;
		UInt32*			volUsedP;
		UInt32*			volTotalP;
	};

	PARAM_VAL(UInt16,	volRefNum);
	PARAM_REF(UInt32,	volUsedP,	Marshal::kOutput);
	PARAM_REF(UInt32,	volTotalP,	Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_MsfsLib::FSVolumeSize(volRefNum, volUsedP, volTotalP);

	volUsedP.Put();
	volTotalP.Put();

	return kSkipROM;
}

/*
CallROMType VfsMgrLibHeadpatch::VfsLibTrapImportDatabaseFromFile(void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("VfsLibTrapImportDatabaseFromFile");

	struct StackFrame
	{
		UInt16			volRefNum;
		Char*			pathNameP;
		UInt16*			cardNoP;
		LocalID*		dbIDP;
	};

	PARAM_VAL(UInt16,	volRefNum);
	PARAM_STR(Char,		pathNameP);
	PARAM_REF(UInt16,	cardNoP,	Marshal::kOutput);
	PARAM_REF(LocalID,	dbIDP,		Marshal::kOutput);

	m68k_dreg (regs, 0) = Platform_VfsLib::VFSImportDatabaseFromFile(volRefNum, pathNameP, cardNoP, dbIDP);

	return kSkipROM;
}


CallROMType VfsMgrLibHeadpatch::VfsLibTrapExportDatabaseToFile(void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("VfsLibTrapExportDatabaseToFile");

	struct StackFrame
	{
		UInt16			volRefNum;
		Char*			pathNameP;
		UInt16			cardNo;
		LocalID			dbID;
	};

	PARAM_VAL(UInt16,	volRefNum);
	PARAM_STR(Char,		pathNameP);
	PARAM_VAL(UInt16,	cardNo);
	PARAM_VAL(LocalID,	dbID);

	m68k_dreg (regs, 0) = Platform_VfsLib::VFSExportDatabaseToFile(volRefNum, pathNameP, cardNo, dbID);

	return kSkipROM;
}
*/

#include "PalmPackPop.h"
