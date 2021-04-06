/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef _TRAPPATCHES_H_
#define _TRAPPATCHES_H_

#include <vector>

class SessionFile;
struct SystemCallContext;


// Types.

enum CallROMType
{
	kExecuteROM,
	kSkipROM
};


// Function types for head- and Tailpatch functions.

typedef	CallROMType	(*HeadpatchProc)(void);
typedef	void		(*TailpatchProc)(void);


// Structure used to hold information about the currently
// running Palm OS application (as recorded when SysAppStartup
// is called).

struct EmuAppInfo
{
	Int16		fCmd;		// launch code
	DmOpenRef	fDB;
	UInt16		fCardNo;
	LocalID		fDBID;
	UInt16		fMemOwnerID;
	emuptr		fStackP;
	emuptr		fStackEndP;
	long		fStackSize;
	char		fName[dmDBNameLength];
	char		fVersion[256];	// <gulp> I hope this is big enough...

	// B.S. operators for VC++ so that we can put objects of
	// this class into STL collections.

	bool operator < (const EmuAppInfo& other) const
	{ return this->fDBID < other.fDBID; }

	bool operator > (const EmuAppInfo& other) const
	{ return this->fDBID > other.fDBID; }

	bool operator == (const EmuAppInfo& other) const
	{ return this->fDBID == other.fDBID; }

	bool operator != (const EmuAppInfo& other) const
	{ return this->fDBID != other.fDBID; }
};
typedef vector<EmuAppInfo>	EmuAppInfoList;


class ModulePatchTable;


extern long gMemMgrCount;

class Patches
{
	public:
		static void				Initialize			(void);
		static void				Reset				(void);
		static void				Save				(SessionFile&);
		static void				Load				(SessionFile&);
		static void				Dispose				(void);

		static void				PostLoad			(void);

		static CallROMType		HandleSystemCall		(const SystemCallContext&);

		static void				HandleInstructionBreak	(void);
		static void				InstallInstructionBreaks(void);
		static void				RemoveInstructionBreaks	(void);

		static void				GetPatches				(const SystemCallContext&,
														 HeadpatchProc& hp,
														 TailpatchProc& tp);
		static CallROMType		HandlePatches			(const SystemCallContext&,
														 HeadpatchProc hp,
														 TailpatchProc tp);

		static const ModulePatchTable*
								GetLibPatchTable		(uint16 refNum);

		static CallROMType		CallHeadpatch			(HeadpatchProc);
		static void				CallTailpatch			(TailpatchProc);

		static UInt32			OSVersion				(void);
		static UInt32			OSMajorMinorVersion		(void);
		static UInt32			OSMajorVersion			(void);
		static UInt32			OSMinorVersion			(void);

		static Bool				DatabaseInfoHasStackInfo(void) { return OSMajorVersion () <= 2; }	// Changed in 3.0

		static void				SetSwitchApp			(UInt16 cardNo, LocalID dbID);
		static Err				SwitchToApp				(UInt16 cardNo, LocalID dbID);
		static void				SetQuitApp				(UInt16 cardNo, LocalID dbID);
		static Bool				TimeToQuit				(void);

		static void				PuppetString			(CallROMType& callROM,
														 Bool& clearTimeout);

			// These functions are used to inhibit Poser from display error
			// dialogs concerning certain improper practices.  If a system is
			// known to have a certain bug (as reported by the following
			// functions) and it looks like that bug is occuring, then we
			// inhibit the dialog.  Once the bug is fixed, however, we need
			// turn the error reporting back on.

			// Bugs fixed in 3.0.
		static Bool				HasFindShowResultsBug	(void) { return OSMajorVersion () <= 2; }
		static Bool				HasSysBinarySearchBug	(void) { return OSMajorVersion () <= 2; }

			// Bugs fixed in 3.1
		static Bool				HasGrfProcessStrokeBug	(void) { return OSMajorMinorVersion () <= 30; }
		static Bool				HasMenuHandleEventBug	(void) { return OSMajorMinorVersion () <= 30; }

			// Bugs fixed in 3.2
		static Bool				HasBackspaceCharBug		(void) { return OSMajorMinorVersion () <= 31; }
		static Bool				HasDeletedStackBug		(void) { return OSMajorMinorVersion () <= 31; }
		static Bool				HasFindSaveFindStrBug	(void) { return OSMajorMinorVersion () <= 31; }
		static Bool				HasFldDeleteBug			(void) { return OSMajorMinorVersion () <= 31; }
		static Bool				HasFntDefineFontBug		(void) { return OSMajorMinorVersion () <= 31; }
		static Bool				HasNetPrvTaskMainBug	(void) { return OSMajorMinorVersion () <= 31; }
		static Bool				HasNetPrvSettingSetBug	(void) { return OSMajorMinorVersion () <= 31; }

		static Bool				HasWellBehavedMemSemaphoreUsage (void);

			// Bugs fixed in 3.3
		static Bool				HasECValidateFieldBug	(void) { return OSMajorMinorVersion () <= 32; }

			// Bugs fixed in 3.5
		static Bool				HasConvertDepth1To2BWBug(void) { return OSMajorMinorVersion () <= 33; }

			// Bugs fixed in 4.0
		static Bool				HasSelectItemBug		(void) { return OSMajorMinorVersion () == 35; }
		static Bool				HasSyncNotifyBug		(void) { return OSMajorVersion () <= 3; }

			// Bugs fixed in ???
		static Bool				HasPrvDrawSliderControlBug	(void)	{ return OSMajorMinorVersion () <= 40; }

			// ==== Done with bugs section ====

		static Bool				IsPCInMemMgr			(void) { return gMemMgrCount > 0; }
		static void				EnterMemMgr				(const char*);
		static void				ExitMemMgr				(const char*);

		static Bool				IsInSysBinarySearch		(void);
		static void				EnterSysBinarySearch	(void);
		static void				ExitSysBinarySearch		(void);

		static Bool				UIInitialized			(void);
		static void				SetUIInitialized		(Bool);

		static Bool				UIReset					(void);
		static void				SetUIReset				(Bool);

		static Bool				HeapInitialized			(void);
		static void				SetHeapInitialized		(Bool);

		static Bool				EvtGetEventCalled		(void);

		static Bool				AutoAcceptBeamDialogs	(Bool b);

		static Bool				TurningJapanese			(void);

		static EmuAppInfo		CollectCurrentAppInfo	(emuptr appInfoP);
		static EmuAppInfo		GetCurrentAppInfo		(void);
		static EmuAppInfo		GetRootAppInfo			(void);

	private:
		static void				SetupForTailpatch		(TailpatchProc tp,
														 const SystemCallContext&);
		static TailpatchProc	RecoverFromTailpatch	(emuptr oldpc);
};


// ======================================================================
// At compile time, the list of functions we want to head- and tailpatch
// are stored in an array of ProtoPatchTableEntries.  At runtime, these
// compact arrays are expanded by ModulePatchTable into a sparse array
// so that the function dispatch number can be used to look up the
// patch function.
// ======================================================================

struct ProtoPatchTableEntry
{
	uint16			fTrapWord;
	HeadpatchProc	fHeadpatch;
	TailpatchProc	fTailpatch;
};


// Defined in TrapPatches_NetLib.cpp, used in TrapPatches.cpp
extern ProtoPatchTableEntry	gProtoNetLibPatchTable[];


// Defined in TrapPatches_MemMgr.cpp, used in TrapPatches.cpp
extern ProtoPatchTableEntry	gProtoMemMgrPatchTable[];

#ifdef SONY_ROM
#include "SonyShared\TrapPatches_ExpMgr.h"
#include "SonyShared\TrapPatches_VfsLib.h"
#include "SonyShared\TrapPatches_MsfsLib.h"
#include "SonyShared\TrapPatches_SlotDrvLib.h"
#endif

#endif /* _TRAPPATCHES_H_ */

